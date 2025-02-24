#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <set>
#include <string>
#include <vector>

#include "db_impl.h"
#include "builder.h"
#include "db_iter.h"
#include "dbformat.h"
#include "filename.h"
#include "log_reader.h"
#include "log_writer.h"
#include "memtable.h"
#include "table_cache.h"
#include "version_set.h"
#include "write_batch_internal.h"
#include "../include/leveldb/db.h"
#include "../include/leveldb/env.h"
#include "../include/leveldb/status.h"
#include "../include/leveldb/table.h"
#include "../include/leveldb/table_builder.h"
#include "../port/port.h"
#include "../table/block.h"
#include "../table/merger.h"
#include "../table/two_level_iterator.h"
#include "../util/coding.h"
#include "../util/logging.h"
#include "../util/mutexlock.h"

namespace leveldb {
    const int kNumNonTableCacheFiles = 10;// 为其它用途预留的文件数
    // 表示一个等待写入的写操作
    struct DBImpl:Writer {
        explicit Writer(port::Mutex* mu) : batch(nullptr), sync(fasle), cv(mu) 
        {}
        Status status;// 记录写操作的结果状态
        WriteBatch* batch;// 指向待写入的批量操作
        bool sync;// 指示是否需要同步写入
        bool done;// 指示写操作是否已完成
        port::CondVar cv;// 条件变量,用于线程间同步,确保写操作按顺序执行
    };
    // 表示一个压缩操作的状态
    struct DBImpl::CompactionState {
        struct Output {
            uint64_t number;// 文件编号
            uint64_t file_size;// 文件大小
            InternalKey smallest, larget;// 文件中最小、最大的键
        };
        Output* current_output() { return &outputs[outputs.size()-1];}// 获取当前输出的文件信息
        explicit CompactionState(Compaction* c)
            : compaction(c),
              smallest_snapshot(0),
              outfile(nullptr),
              builder(nullptr),
              total_bytes(0)
            {}
        Compaction* const compaction;// 当前的压缩操作
        SequenceNumber smallest_snapshot;// 小于 smallest_snapshot 的序列号不重要，因为不会为小于 smallest_snapshot 的快照提供服务.因此，如果看到一个序列号 S <= smallest_snapshot，可以丢弃相同键的所有序列号 < S 的条目
        std::vector<Output> outputs;// 压缩生成的所有文件
        WritableFile* outfile;// 当前输出的文件
        TableBuilder* builder;// 用于构建 SSTable 的构建器
        uint64_t total_bytes;// 当前压缩操作的总字节数
    };
    /**
     * @brief 将指针ptr指向的值限制在[minvalue, maxvalue]范围内
     * 
     * @tparam T 
     * @tparam V 
     * @param ptr 
     * @param minvalue 
     * @param maxvalue 
     */
    template<class T, class V>
    static void ClipToRange(T* ptr, V minvalue, V maxvalue) {
        if(static_cast<V>(*ptr) > maxvalue)
            *ptr = maxvalue;
        if(static_cast<V>(*ptr) < minvalue)
            *ptr = minvalue;
    }
    /**
     * @brief 调整用户提供的选项，确保其值在合理范围内，并设置默认值
     * 
     * @param dbname 
     * @param icmp 
     * @param ipolicy 
     * @param src 
     * @return Options 
     */
    Options SanitizeOptions(const std::string& dbname,
                            const InternalKeyComparator* icmp,
                            const InternalFilterPolicy* ipolicy,
                            const Options& src) 
            {
                Options result = src;// 复制用户提供的选项
                result.comparator = icmp;// 设置内部键比较器
                result.filter_policy = (src.filter_policy!=nullptr) ? ipolicy : nullptr;// 设置过滤器策略
                ClipToRange(&result.max_open_files, 64+kNumNonTableCacheFiles, 50000);// 调整 max_open_files 选项，确保在合理范围内
                ClipToRange(&result.write_buffer_size, 64<<10, 1<<30);// 调整 write_buffer_size 选项，确保在合理范围内
                ClipToRange(&result.max_file_size, 1<<20, 1<<30);// 调整 max_file_size 选项，确保在合理范围内
                ClipToRange(&result.block_size, 1<<10, 4<<20);// 调整 block_size 选项，确保在合理范围内
                if(result.info_log==nullptr) {// 如果用户没有提供 info_log，则创建一个新的日志文件
                    src.env->CreateDir(dbname);// 创建数据库目录（如果不存在）
                    src.env->RenameFile(InfoLogFileName(dbname,), OldInfoLogFileName(dbname));// 重命名旧日志文件
                    Status s = src.env->NewLogger(InfoLogFileName(dbname), &result.info_log);// 创建新的日志文件
                    if(!s.ok())// 如果创建日志文件失败，则不使用日志
                        result.info_log = nullptr;
                }
                if(result.block_cache == nullptr)// 如果用户没有提供 block_cache，则创建一个新的 LRU 缓存
                    result.block_cache = NewLRUCache(8<<20);// 默认缓存大小为 8MB
                return result;
            }
    /**
     * @brief 计算表缓存的大小
     * 表缓存不会占用所有文件描述符,会为日志文件、清单文件等其它文件预留空间,因此要减去kNumNonTableCacheFiles
     * @param sanitized_options 
     * @return int 
     */
    static int TableCacheSize(const Options& sanitized_options) {
        return sanitized_options.max_open_files - kNumNonTableCacheFiles;
    }
    DBImpl::DBImpl(const Options& raw_options, const std::string& dbname)
        : env_(raw_options.env),// 初始化环境接口
        internal_comparator_(raw_options.comparator),// 初始化内部键比较器
        internal_filter_policy_(raw_options.filter_policy),// 初始化内部过滤器策略
        options_(SanitizeOptions(dbname, &internal_comparator_,
                                &internal_filter_policy_, raw_options)),// 调整并初始化选项
        owns_info_log_(options_.info_log != raw_options.info_log),// 是否拥有 info_log
        owns_cache_(options_.block_cache != raw_options.block_cache),// 是否拥有 block_cache
        dbname_(dbname),// 初始化数据库名称
        table_cache_(new TableCache(dbname_, options_, TableCacheSize(options_))),// 初始化表缓存
        db_lock_(nullptr),// 初始化数据库锁为空
        shutting_down_(false),// 初始化关闭标志为 false
        background_work_finished_signal_(&mutex_),// 初始化后台工作完成信号量
        mem_(nullptr),// 初始化当前内存表(memtable)为空
        imm_(nullptr),// 初始化待压缩的内存表(Immutable memtable)为空
        has_imm_(false),// 初始化是否有待压缩的内存表为false
        logfile_(nullptr),// 初始化日志文件为空
        logfile_number_(0),// 初始化日志文件编号为 0
        log_(nullptr),// 初始化日志写入器为空
        seed_(0),
        tmp_batch_(new WriteBatch),// 初始化临时批量写操作
        background_compaction_scheduled_(false),// 初始化后台压缩调度标志为 false
        manual_compaction_(nullptr),// 初始化手动压缩为空
        versions_(new VersionSet(dbname_, &options_, table_cache_,
                                &internal_comparator_))// 初始化版本集合
    {}
    DBImpl::~DBImpl() {
        mutex_.Lock();// 加锁
        shutting_down_.store(true, std::memory_order_release);// 设置关闭标志
        while(background_compaction_scheduled_) {
            background_work_finished_signal_.Wait();// 等待后台压缩完成
        }
        mutex_.Unlock();// 解锁
        if(db_lock_!=nullptr)
            env_->UnlockFile(db_lock_);// 释放数据库锁
        delete versions_;// 删除版本集合
        if(mem_!=nullptr)
            mem_->Unref();// 释放当前内存表
        if(imm_!=nullptr)
            imm_->Unref();// 释放待压缩的内存表
        delete tmp_batch_;// 删除临时批量写操作
        delete log_;// 删除日志写入器
        delete logfile_;// 删除日志文件
        delete table_cache_;// 删除表缓存
        if(owns_info_log_)
            delete options_.info_log;// 如果拥有 info_log，则删除
        if(owns_cache_)
            delete options_.block_cache;// 如果拥有 block_cache，则删除
    }
    /**
     * @brief 创建一个新的数据库
     * 
     * @return Status 
     */
    Status DBImpl::NewDB() {
        VersionEdit new_db;// 创建一个新的版本编辑对象
        new_db.SetComparatorName(user_comparator()->Name());// 设置比较器名称
        new_db.SetLogNumber(0);// 设置日志文件编号为0
        new_db.SetNextFile(2);// 设置下一个文件编号为2
        new_db.SetLastSequence(0);// 设置最后一个序列号为0
        const std::string manifest = DescriptorFileName(dbname_, 1);// 生成清单文件名
        WritableFile* file;
        Status s = env_->NewWritableFile(manifest, &file);// 创建清单文件
        if(!s.ok())
            return s;
        log::Writer log(file);// 创建日志写入器
        std::string record;// 定义记录字符串
        new_db.EncodeTo(&record);// 将版本编辑对象编码为记录
        s = log.AddRecord(record);// 将记录写入日志
        if(s.ok())
            s = file->Sync();// 同步文件，确保数据写入磁盘
        if(s.ok())
            s = file->Close();
        delete file;
        if(s.ok())
            s = SetCurrentFile(env_, dbname_, 1);// 创建Current文件,指向新的清单文件
        else
            env_->RemoveFile(manifest);
        return s;
    }
    /**
     * @brief 根据配置决定是否忽略错误
     * 
     * @param s 
     */
    void DBImpl::MaybeIgnoreError(Status* s) const {
        if(s->ok() || options_paranoid_checks)// 如果状态正常s->ok()或启用了严格检查options_.paranoid_checks,则不进行任何处理

        else {
            Log(options_.info_log, "Ignoring error %s", s->ToString().c_str());// 记录错误信息
            *s = Status::OK();
        }
    }
    /**
     * @brief 删除不再需要的旧文件
     * 
     */
    void DBImpl::RemoveObsoleteFiles() {
        mutex_.AssertHeld();// 确保当前线程持有互斥锁
        if(!bg_error_.ok())// 如果后台任务发生了错误，无法确定是否有新版本被提交，因此不能安全地进行垃圾回收,则直接return
            return;
        std::set<uint64_t> live = pending_outputs_;// 正在进行的压缩操作生成的文件 pending_outputs_存储了当前正在进行的压缩操作生成的文件
        versions_->AdLiveFiles(&live);// 从版本集合中添加活跃的文件
        stdLLvector<std::string> filenames;
        env_->GetChildren(dbname_, &filenames);// 获取数据库目录下的所有文件
        uint64_t number;// 文件编号
        FileType type;// 文件类型
        std::vector<std::string> files_to_delete;// 待删除的文件列表
        for(std::string& filename : filenames) {
            if(ParseFileName(filename, &number, &type)) {// 解析文件名，获取文件编号和类型
                bool keep = true;// 是否保留该文件
                switch(type) {
                    case kLogFile:
                        // 保留当前日志文件或前一个日志文件
                        keep = (((number>=versions_->LogNumber())) ||
                                (number==versions_->PrevLogNumber()));
                        break;
                    case kDescriptorFile:
                        // 保留当前的清单文件或更新的清单文件
                        keep = (number>=versions_->ManifestFileNumber());
                        break;
                    case kTableFile:
                        // 保留活跃的 SSTable 文件
                        keep = (live.find(number)!=live.end());
                        break;
                    case kTempFile:
                        // 保留正在写入的临时文件
                        keep = (live.find(number)!=live.end());
                        break;
                    case kCurrentFile:
                    case kDBLockFile:
                    case kInfoLogFile:
                        // 始终保留 CURRENT 文件、锁文件和信息日志文件
                        keep = true;
                        break;
                }
                if(!keep) {// 不保留的文件就加入到待删除文件列表files_to_delete中
                    files_to_delete.push_back(std::move(filename));
                    if(type==kTableFile)// 如果是不需要保留文件且是SSTable文件,则从表缓存中移除
                        table_cache_->Evict(number);
                    Log(options_.info_log, "Delete type=%d #%lld\n", static_cast<int>(type), static_cast<unsigned long long>(number));// 记录删除操作 
                }
            }
        }
        mutex_.Unlock();// 释放当前线程持有的锁
        for(const std::string& filename : files_to_delete) // 遍历 files_to_delete 列表，删除所有需要删除的文件
            env_->RemoveFile(dbname_+"/"+filename);// 删除文件
        mutex_.Lock();// 重新加锁
    }
    /**
     * @brief 用于在数据库启动时进行恢复操作，处理可能存在的日志文件、版本信息等，确保数据库处于一致状态
     * 日志文件->MemTable(SST)
     * @param edit 
     * @param save_manifest 
     * @return Status 
     */
    Status DBImpl::Recover(VersionEdit* edit, bool* save_manifest) {
        mutex_.AssertHeld();
        env_->CreateDir(dbname_);// 创建数据库目录（允许目录已存在）
        assert(db_lock_==nullptr);// 确保数据库锁初始为空
        Status s = env_->LockFile(LockFileName(dbname_), &db_lock_);// 尝试锁定数据库文件
        if(!s.ok())
            return s;// 上锁失败直接返回错误
        // 检查 CURRENT 文件是否存在
        if(!env_->FileExists(CurrentFileName(dbname_))) {// CURRENT文件不存在
            if(options_.create_if_missing) {
                Log(options_.info_log, "Creating DB %s since it was missing.", dbname_.c_str());
                s = NewDB();// 创建新的数据库
                if(!s.ok())
                    return s;
            }
            else
                return Status::InvalidArgument(dbname_, "does not exist (create_if_missing is false)");
        }
        else if(options_.error_if_exists) 
                return Status::InvalidArgument(dbname_, "exists (error_if_exists if true)");
        s = versions_->Recover(save_manifest);// 从 MANIFEST 恢复版本信息
        if(!s.ok())
            return s;
        SequenceNumber max_sequence(0);
        const uint64_t min_log = versions_->LogNumber(); // 当前日志编号
        const uint64_t prev_log = versions_->PrevLogNumber();;// 旧日志编号
        std::vector<std::string> filenames;
        s = env_->GetChildren(dbname_, &filenames);// 获取数据库目录下所有文件名,存储在filenames中
        if(!s.ok())
            return s;
        std::set<uint64_t> expected;
        versions_->AddLiveFiles(&expected);// expected存储当前版本(version)中所有存活的SSTable文件编号.这些文件编号是 LevelDB 在崩溃前通过 Manifest 记录的有效数据文件，恢复时必须确保它们全部存在于磁盘上，否则会触发数据损坏(Corruption)错误
        uint64_t number;
        FileType type;
        std::vector<uint64_t> logs;// 存储需要恢复的日志文件编号
        for(size_t i=0;i<filenames.size();i++) {
            if(ParseFileName(filenames[i], &number, &type)) {
                expected.erase(number);// 移除实际存在的文件
                if(type==kLogFile && ((number>=min_log) || (number==prev_log)))
                    logs.emplace_back(number);
            }
        }
        if(!expected.empty()) {// 存在缺失文件
            char buf[50];
            std::snprintf(buf, sizeof(buf), "%d missing files; e.g.", static_cast<int>(expected.size()));
            return Status::Corruption(buf, TableFileName(dbname_, *(expected.begin())));
        }
        std::sort(logs.begin(), logs.end());// 按编号升序排序
        for(size_t i=0;i<logs.size();++i) {
            s = RecoverLogFile(logs[i], (i==logs.size()-1), save_manifest, edit, &max_sequence);
            if(!s.ok())
                return s;
        }
        versions_->MarkFileNumberUsed(logs[i]);
        if(versions_->LastSequence() < max_sequence)// 确保后续写入操作的序列号大于已恢复的所有操作
            versions_->SetLastSequence(max_sequence);
        return Status::OK();
    }
    /**
     * @brief 用于从指定的日志文件中恢复数据到内存表(MemTable),确保数据库重启时未持久化的操作能正确恢复.核心流程包括日志解析、数据写入 MemTable、触发内存表持久化到 Level 0 的 SSTable，并处理日志文件的复用
     * 
     * @param log_number 需恢复的日志文件编号
     * @param last_log 标记是否为当前最新的日志文件
     * @param save_manifest 输出参数，指示是否需要更新 Manifest 文件
     * @param edit 版本编辑对象，记录 SSTable 的增删操作
     * @param max_sequence 输出当前恢复过程中最大的序列号
     * @return Status 
     */
    Status DBImpl::RecoverLogFile(uint64_t log_number, bool last_log, bool* save_manifest, VersionEdit* edit, SequenceNumber* max_sequence) {
        struct LogReporter : public log::Reader::Reporter {
            Env* env;// 环境接口（用于文件操作）
            Logger* info_log;// 日志记录器
            const char* fname;// 当前处理的日志文件名
            Status* statue;// 错误状态指针
            void Corruption(size_t bytes, const Status& s) override {// 日志损坏时的回调函数
                Log(info_log, "%s%s: dropping %d bytes; %s", (this->status==nullptr?"(ignoring error) " : ""), fname, 
                    static_cast<int>(bytes), s.ToString().c_str());
                if(this->status!=nullptr && this->status->ok())
                    *this->status = s;
            }
        };
        mutex_.AssertHeld();
        std::string fname = LogFileName(dbname_, log_number);
        SequentialFile* file;
        Status status = env_->NewSequentialFile(fname, &file);
        if(!status.ok()) {
            MaybeIgnoreError(&status);// 根据配置决定是否忽略错误（如文件不存在）
            return status;
        }
        // 初始化日志读取器,强制启用校验和检查
        LogReporter reporter;
        reporter.env = env_;
        reporter.info_log = options_.info_log;
        reporter.fname = fname.c_str();
        reporter.status = (options_.paranoid_checks ? &status:nullptr);
        log::Reader reader(file, &reporter, true, 0);
        Log(options_.info_log, "Recovering log #%llu", (unsigned long long)long_number);
        // 读取日志记录并应用到MemTable中
        std::string scratch;
        Slice record;
        WriteBatch batch;
        int compactions = 0;// 记录触发的 Compaction 次数
        MemTable* mem = nullptr;
        while(reader.ReadRecord(&record, &scratch)&&status.ok()) {
            if(record.size() < 12) {// 校验记录长度（每条记录至少包含 12 字节的头部信息）
                reporter.Corruption(record.size(), Status::Corruption("log record too small"));
                continue;
            }
            WriteBatchInternal::SetContents(&batch, record);// 将日志文件中的记录反序列化解析到batch
            if(mem==nullptr) {// 如果当前没有 MemTable，则创建一个新的 MemTable
                mem = new MemTable(internal_comparator_);
                mem->Ref();
            }
            status = WriteBatchInternal::InsertInto(&batch, mem);// 将WriteBatch插入MemTable
            MaybeIgnoreError(&status);
            if(!status.ok())
                break;
            const SequenceNumber last_seq = WriteBatchInternal::Sequence(&batch) + WriteBatchInternal::Count(&batch)-1;
            if(last_seq > *max_sequence)// 更新最大序列号
                *max_sequence = last_seq;
            if(mem->ApproximateMemoryUsage() > options_.write_buffer_size) {// 若 MemTable 超过写缓冲区大小，触发持久化到 Level 0
                compactions++;// MemTable->SST会触发一次压缩操作
                *save_manifest = true;// 标记需要更新 Manifest
                status = WriteLevel0Table(mem, edit, nullptr);// 写入 SSTable
                mem->Unref();
                mem = nullptr;
                if(!status.ok())
                    break;
            }
        }
        delete file;// 关闭日志文件
        // 日志复用:最后一个日志若满足特定条件,那么后续日志文件可以直接在最后一条日志文件上追加,而不是新建
        if(status.ok() && options_.reuse_logs && last_log && compactions==0) {
            assert(logfile_ == nullptr);
            assert(log_ == nullptr);
            assert(mem_ == nullptr);
            uint64_t lfile_size;
            if(env_->GetFileSize(fname, &lfile_size).ok() && env_->NewAppendableFile(fname, &logfile_).ok()) {
                Log(options_.info_log, "Reusing old log %s \n", fname.c_str());
                log_ = new log::Writer(logfile_, lfile_size);
                logfile_number_ = log_number;
                if(mem!=nullptr) {
                    mem_ = mem;
                    mem = nullptr;
                }
                else {
                    mem_ = new MemTable(internal_comparator_);
                    mem_->Ref();
                }
            }
        }
        // 若循环结束后仍有未处理的 MemTable，强制持久化到 Level 0
        if(mem!=nullptr) {
            if(status.ok()) {
                *save_manifest = true;
                status = WriterLevel0Table(mem, edit, nullptr);
            }
            mem->Unref();
        }
        return status;
    }
    /**
     * @brief 将内存(MemTable)中的数据写入到磁盘上的SST文件中,并将其添加到数据库的Level0层
     * 1. 将内存表数据写入磁盘;2. 更新数据库版本(将新生成的 SST 文件添加到数据库的版本管理中,即写到Manifest(通过 VersionEdit));3. 更新压缩统计信息
     * @param mem 
     * @param edit 
     * @param base 
     * @return Status 
     */
    Status DBImpl::WriteLevel0Table(MemTable* mem, VersionEdit* edit, Version* base) {
        mutex_.AssertHeld();// 确保当前线程持有互斥锁
        const uint64_t start_micros = env_->NowMicros();// 获取当前时间
        FileMetaData meta;// 文件元数据
        meta.number = versions_->NewFileNumber();// 为新文件分配一个唯一的编号
        pending_outputs-.insert(meta.number);// 将该文件编号加入到待输出文件集合中
        Iterator* iter = mem->NewIterator();// 创建内存表的迭代器
        Log(options_.info_log, "Level 0 table #%llu: started", (unsigned long long)meta.number);// 记录日志，表示开始创建 Level-0 表
        Status s;
        {
            mutex_.Unlock();// 释放互斥锁，以允许其他线程访问共享资源
            s = BuildTable(dbname_, env_, options_, table_cache_, iter, &meta);// 构建 SST 文件
            mutex_.Lock();// 重新获取互斥锁
        }
        Log(options_.info_log, "Level 0 table #%llu: %lld bytes %s", (unsigned long long)meta.number, (unsigned long long)meta.file_size,
            s.ToString().c_str());// 记录日志，表示 Level-0 表的创建结果
        delete iter;
        pending_outputs_.erase(meta.number);// 从待输出文件集合中移除该文件编号,因为已经把该文件构建到SST了
        int level = 0;
        if(s.ok() && meta.file_size>0) {// 如果创建成功且文件大小大于零
            const Slice min_user_key = meta.smallest.user_key();// 获取文件中最小的用户键
            const Slice max_user_key = meta.largest.user_key();// 获取文件中最大的用户键
            if(base != nullptr) 
                level = base->PickLevelForMemTableOutput(min_user_key, max_user_key);
            edit->AddFile(level, meta.number, meta.file_size, meta.smallest, meta.largest);// 更新数据库版本
        }
        // 更新压缩统计信息
        CompactionStats stats;
        stats.micros = env_->NowMicros()-start_micros;// 计算总耗时
        stats.bytes_written = meta.file_size;// 记录写入的字节数
        stats_[level].Add(stats);// 将统计信息添加到对应 Level 的统计中
        return s;
    }
    /**
     * @brief 将不可变内存表imm_压缩成一个新的SST文件,并更新版本信息
     * 
     */
    void DBImpl::CompactMemTable() {
        mutex_.AssertHeld();// 确保当前线程持有互斥锁，保证线程安全
        assert(imm_!=nullptr);// 断言当前存在一个待处理的不可变内存表
        VersionEdit edit;// 创建一个版本编辑器(VersionEdit),用于记录版本变更信息
        Version* base = versions_->current();// 获取当前版本(version),并增加其引用计数
        base->Ref();
        Status s = WriteLevel0Table(imm_, &edit, base);// 将不可变内存表(imm_)写入到一个新的Level 0 SST文件中
        base->Unref();
        if(s.ok() && shutting_down_.load(std::memory_order_acquire))// 如果数据库正在关闭过程中，设置错误状态
            s = Status::IOError("Deleting DB during memtable compaction");
        if(s.ok()) {// 如果写入 SST 文件成功,更新版本编辑器(VersionEdit)信息
        // 这里写的编辑器更新和WriteLevel0Table()中的编辑器更新是不一样的(WriteLevel0Table()中仅仅是添加文件信息,还需要更新其它版本状态信息,如前一个日志文件编号和当前日志文件编号)
            edit.SetPreLogNumber(0);// 设置前一个日志文件编号为 0
            edit.SetLogNumber(logfile_number_);// 设置当前日志文件编号
            s = versions_->LogAndApply(&edit, &mutex_);// 将版本编辑器中的变更应用到版本集合（VersionSet）中
        }
        if(s.ok()) {
            // 减少不可变内存表的引用计数，并将其置为 nullptr
            imm_->Unref();
            imm_ = nullptr;
            has_imm_.store(false, std::memory_order_release);// 标记当前没有不可变内存表
            RemoveObsoleteFiles();// 移除不再需要的过时文件
        }
        else    
            RecordBackgroundError(s);// 如果发生错误，记录后台错误信息
    }
    /**
     * @brief 将指定范围[begin, end)内的数据从多个层级(如level 0到max_level_with_files)进行合并
     * 通过紧凑化操作,减少数据的冗余,优化存储结构,提高查询性能
     * 
     * @param begin 
     * @param end 
     */
    void DBImpl::CompactRange(const Slice* begin, const Slice* end) {
        int max_level_with_files = 1;// 初始化最高层级为1
        {
            MutexLock l(&mutex);
            Version* base = versions_->current();// 获取当前版本
            for(int level = 1;level<config::kNumLevels;level++) {
                if(base->OverlapInLevel(level, begin, end))// 如果 level 的文件与 [begin, end) 范围重叠，更新最高层级
                    max_level_with_files = level;
            }
        }
        TEST_CompactMemTable();// 触发内存表(MemTable)的持久化操作
        for(int level=0;level<max_level_with_files;level++)// 对每个层级执行紧凑化操作
            TEST_CompactRange(level, begin, end);
    }
    /**
     * @brief 用于手动触发紧凑化操作的内部测试函数.它允许开发者或用户显式地指定某个层级level以及键值范围[begin, end),并对该范围内的数据进行紧凑化处理
     * 
     * @param level 
     * @param begin 
     * @param end 
     */
    void DBImap::TEST_CompactRange(int level, const Slice* begin, const Slice* end) {
        assert(level>=0);// 断言 level 非负
        assert(level+1<config::kNumLevels);// 断言 level 不超过层级总数减一
        InternalKey begin_storage, end_storage;
        ManualCompaction manual;// 手动紧凑化对象
        manual.level = level;// 设置待操作的层级
        manual.done = false;// 标记操作未完成
        if(begin==nullptr)
            manual.begin = nullptr;// 如果 begin 为空，设置 manual.begin 为空
        else {
            begin_storage = InternalKey(*begin, kMaxSequenceNumber, kValueTypeForSeek);// 创建内部键
            manual.begin = &begin_storage;// 设置手动紧凑化对象的起始键
        }
        if (end == nullptr)
            manual.end = nullptr;// 如果 end 为空，设置 manual.end 为空
        else {
            end_storage = InternalKey(*end, 0, static_cast<ValueType>(0));// 设置手动紧凑化对象的结束键
            manual.end = &end_storage;
        }
        MutexLock l(&mutex_);// 加锁，确保线程安全
        // 等待手动紧凑化完成
        while(!manual.done && !shutting_down_.load(std::memory_order_acquire) && bg_error_.ok()) {
            if(manual_compaction_ == nullptr) {
                manual_compaction_ = &manual;// 设置当前手动紧凑化对象
                MaybeScheduleCompaction();// 将紧凑化任务添加到后台任务
            }
            else 
                background_work_finished_signal_.Wait();// 等待后台线程完成工作
        }
        // 等待后台紧凑化调度完成
         while(background_compaction_scheduled_) 
            background_work_finished_signal_.Wait();// 等待后台用于紧凑化的线程完成工作
        if(manual_compaction_ == &manual)
            manual_compaction_ = nullptr;
    }
    /**
     * @brief 触发内存表的持久化操作,并等待后台线程完成处理
     * 
     * @return Status 
     */
    Status DBImpl::TEST_CompactMemTable() {
        Status s = Write(WriteOptions(), nullptr);// 调用 Write 方法，可能触发内存表的持久化操作
        if(s.ok()) {
            MutexLock l(&mutex_);
            while(imm_ != nullptr && bg_error_.ok())// 等待后台线程完成不可变内存表的处理
                background_work_finished_signal_.Wait();// 等待后台工作完成
            if(imm_ != nullptr) 
                s = bg_error_;// 更新状态为后台错误
        }
        return s;
    }
    /**
     * @brief 记录后台线程的错误状态,并通知等待线程
     * 
     * @param s 
     */
    void DBImpl::RecordBackgroundError(const Status& s) {
        mutex_.AssertHeld();// 确保锁已被持有
        if(bg_error_.ok()) {
            bg_error_ = s;
            background_work_finished_signal_.SignalAll();// 通知所有等待线程
        }
    }
    /**
     * @brief 根据当前状态(如后台任务是否已调度、是否存在错误等)决定是否调度后台紧凑化任务
     * 
     */
    void DBImpl::MaybeScheduleCompaction() {
        mutex_.AssertHeld();
        if(background_compaction_scheduled_)// 如果后台紧凑化已调度

        else if(shutting_down_.load(std::memory_order_acquire))// 如果数据库正在关闭

        else if(!bg_error_.ok())// 如果后台存在错误

        else if(imm_==nullptr && manual_compaction_ == nullptr && !versions_->NeedsCompaction())// 如果没有不可变内存表、没有手动紧凑化任务且当前版本不需要紧凑化

        else {
            env_->Schedule(&DBImpl::BGWork, this);// 调度后台紧凑化任务,将BGWork加入后台任务
            background_compaction_scheduled_ = true;// 标记后台紧凑化已调度
        }
    }
    /**
     * @brief 调度后台任务,执行紧凑化或其它后台操作
     * 
     * @param db 
     */
    void DBImpl::BGWork(void* db) {
        reinterpret_cast<DBImpl*>(db)->BackgroundCall();// 调用后台处理函数
    }
    /**
     * @brief 在后台线程中执行紧凑化操作,确保数据库的存储结构保持高效和一致.实际执行函数是调用BackgroundCompaction()
     * 
     */
    void DBImpl::BackgroundCall() {
        MutexLock l(&mutex_);
        assert(background_compaction_scheduled_);// 确保后台紧凑化任务已被调度
        if(shutting_down_.load(std::memory_order_acquire))// 检查数据库是否正在关闭

        else if(!bg_error_.ok())// 检查后台是否已经记录了错误

        else// 如果数据库未关闭且后台没有错误，执行紧凑化操作
            BackgroundCompaction();// 调用 BackgroundCompaction 函数执行实际的紧凑化逻辑
        background_compaction_scheduled_ = false;// 标记后台紧凑化任务已完成
        MaybeScheduleCompaction();// 检查是否需要调度新的紧凑化任务(根据当前状态决定是否启动新的紧凑化任务)
        background_work_finished_signal_.SignalAll();// 通过条件变量通知所有等待线程，后台工作已完成
    }
    /**
     * @brief 后台执行紧凑化（Compaction）的核心函数，处理Minor和Major Compaction
     * 
     */
    void DBImpl::BackgroundCompaction() {
        mutex_.AssertHeld();
        // 1. 优先处理Minor Compaction(内存数据MemTable刷盘)
        if(imm_!=nullptr) {
            CompactionMemTable();// 将Immutable MemTable写入Level 0
            return;
        }
        Compaction* c;
        bool is_manual = (manual_compaction_ != nullptr);// 判断是否手动触发
        InternalKey manual_end;
        // 2. 处理手动触发的Major Compaction(manual Compaction)
        if (is_manual){
            ManualCompaction* m = manual_compaction_;
            // 根据手动指定的层级和键范围生成Compaction任务
            c = versions_->CompactRange(m->level, m->begin, m->end);
            m->done = (c == nullptr);// 标记是否完成
            if (c != nullptr) 
                manual_end = c->input(0, c->num_input_files(0) - 1)->largest;// 记录手动Compaction的结束键（用于断点续处理）
            // 输出日志：记录手动Compaction的范围和状态
            Log(options_.info_log,
                "Manual compaction at level-%d from %s .. %s; will stop at %s\n",
                m->level, (m->begin ? m->begin->DebugString().c_str() : "(begin)"),
                (m->end ? m->end->DebugString().c_str() : "(end)"),
                (m->done ? "(end)" : manual_end.DebugString().c_str()));
        } 
        else
            c = versions_->PickCompaction();// 3. 自动触发：选择需要Compaction的层级和文件(Size/Seek触发)
        Status status;
        if (c == nullptr)
        // 4. 处理Trivial Move：文件直接移动到下一层级（无需合并）
        // 当前层文件与下一层无键范围重叠，可直接移动文件而无需合并
        else if (!is_manual && c->IsTrivialMove()) {
            assert(c->num_input_files(0) == 1);
            FileMetaData* f = c->input(0, 0);// 获取待移动的文件元数据
            c->edit()->RemoveFile(c->level(), f->number);// 从当前层级删除文件
            c->edit()->AddFile(c->level() + 1, f->number, f->file_size, f->smallest,
                            f->largest);// 添加到下一层级
            status = versions_->LogAndApply(c->edit(), &mutex_);// 应用版本变更并记录日志
            if (!status.ok())
                RecordBackgroundError(status);// 记录错误
            VersionSet::LevelSummaryStorage tmp;
            // 输出日志：记录文件移动详情
            Log(options_.info_log, "Moved #%lld to level-%d %lld bytes %s: %s\n",
                static_cast<unsigned long long>(f->number), c->level() + 1,
                static_cast<unsigned long long>(f->file_size),
                status.ToString().c_str(), versions_->LevelSummary(&tmp));
        } 
        else {// 5. 执行常规Major Compaction
            CompactionState* compact = new CompactionState(c);创建合并状态对象
            status = DoCompactionWork(compact);// 执行实际合并操作（核心逻辑）
            if (!status.ok()) 
                RecordBackgroundError(status);// 记录错误
            CleanupCompaction(compact);// 清理资源：释放输入文件、删除临时文件等
            c->ReleaseInputs();
            RemoveObsoleteFiles();// 清理过期文件
        }
        delete c;// 释放Compaction对象
        // 6. 处理Compaction结果状态
        if (status.ok())
   
        else if (shutting_down_.load(std::memory_order_acquire))

        else 
            Log(options_.info_log, "Compaction error: %s", status.ToString().c_str());
        if (is_manual) {// 7. 手动Compaction的后续处理
            ManualCompaction* m = manual_compaction_;
            if (!status.ok())
                m->done = true;// 标记为完成（即使出错）
            if (!m->done) {// 更新断点续处理的起始键
                m->tmp_storage = manual_end;
                m->begin = &m->tmp_storage;
            }
            manual_compaction_ = nullptr;// 清空手动Compaction任务
        }
    }
    /**
     * @brief 负责在 Compaction 完成或失败时，安全释放临时资源、更新全局状态，并保证系统的一致性
     * 
     * @param compact 
     */
    void DBImpl::CleanupCompaction(CompactionState* compact) {
        mutex_.AssertHeld();
        if(compact->builder!=nullptr) {// 清理未完成的 SSTable 构建器builder
            compact->builder->Abandon();// 中止当前构建器的工作（丢弃未完成的数据，不刷盘）
            delete compact->builder;// 删除构建器对象
        }
        else
            assert(compact->outfile==nullptr);// 如果构建器不存在，确保输出文件句柄也应为空
        delete compact->outfile; // 关闭并释放文件句柄
        for(size_t i=0;i<compact->outputs.size();++i) {
            const CompactionState::Output& out = compact->outputs[i];
            pending_outputs_.erase(out.number);// 从 pending_outputs_ 集合中移除文件编号，标记为不再被占用
        }
        delete compact;// 释放 CompactionState 占用的内存
    }
    /**
     * @brief 在 Compaction 过程中创建一个新的 SSTable 输出文件，并初始化其构建器（TableBuilder）
     * 该函数还未写入键值对数据,写入过程是后续通过Add()
     * @param compact 
     * @return Status 
     */
    Status DBImpl::OpenCompactionOutputFile(CompactionState* compact) {
        assert(compact != nullptr);
        assert(compact->builder == nullptr);
        uint64_t file_number;// 新生成的SSTable文件编号
        {
            mutex_.Lock();
            file_number = versions_->NewFileNumber();// 从 VersionSet 分配新的文件编号
            pending_outputs_.insert(file_number);// 将新文件编号标记为「正在生成」，防止被其他线程误删
            CompactionState::Output out;// 初始化输出文件的元数据
            out.number = file_number;// 文件编号
            out.smallest.Clear();// 最小键（初始为空，后续在写入数据时更新）
            out.largest.Clear();// 最大键（同上）
            compact->outputs.push_back(out);// 添加到输出文件列表
            mutex_.Unlock();
        }
        std::string fname = TableFileName(dbname_, file_number);// 生成新 SSTable 文件的完整路径
        Status s = env_->NewWritableFile(fname, &compact->outfile);// 创建可写文件句柄(用于管理SST文件的写入过程)  compact->outfile持有该文件的句柄
        if (s.ok())
            compact->builder = new TableBuilder(options_, compact->outfile);// (builder是用于构建SST文件的)初始化 SSTable 构建器，关联到新创建的文件句柄
        return s;
    }
    /**
     * @brief 完成当前 Compaction 输出文件的写入，关闭文件句柄，并进行必要的数据验证和日志记录
     * 写入元数据并刷盘
     * @param compact 
     * @param input 
     * @return Status 
     */
    Status DBImpl::FinishCompactionOutputFile(CompactionState* compact, Iterator* input) {
        // 确保传入的压缩状态对象、输出文件对象和 SSTable 构造器对象都不为空
        assert(compact!=nullptr);
        assert(compact->outfile!=nullptr);
        assert(compact->builder!=nullptr);
        const uint64_t output_number = compact->current_output()->number;// 获取当前输出文件的编号
        assert(output_number!=0);// 确保文件编号不为 0
        Status s = input->status();// 获取输入迭代器的状态  输入迭代器是用于检查输入的键值对是否有效的
        const uint64_t current_entries = compact->builder->NumEntries();// 获取当前 SSTable 构造器中已写入的条目数量
        if(s.ok())// 如果输入迭代器状态良好，完成构建（写入索引块、元数据等）
            s = compact->builder->Finish();// 在构建的SST文件中写入(Add())键值对
        else// 如果输入迭代器状态不良好，则放弃构建 SSTable
            compact->builder->Abandon();
        const uint64_t current_bytes = compact->builder->FileSize();// 获取构建完成的 SSTable 文件大小
        compact->current_output()->file_size = current_bytes;// 更新当前输出文件的大小
        compact->total_bytes += current_bytes;// 累加本次压缩的总字节数
        delete compact->builder;// 删除 SSTable 构造器对象
        compact->builder = nullptr;
        if(s.ok())// 如果状态良好，同步输出文件到磁盘
            s = compact->outfile->Sync();
        if(s.ok())// 如果状态良好，关闭输出文件
            s = compact->outfile->Close();
        delete compact->outfile;
        compact->outfile = nullptr;// 删除输出文件对象  通过compact->outfile句柄来控制删除输出
        if(s.ok() && current_entries>0) {// 如果状态良好且有条目写入，则验证生成的 SSTable 文件
            Iterator* iter = table_cache_->NewIterator(ReadOptions(), output_number, current_bytes);// 创建一个迭代器来读取刚刚生成的 SSTable 文件
        s = iter->status();// 获取迭代器的状态
        delete iter;// 删除迭代器对象
        if(s.ok()) 
            Log(options_.info_log, "Generated table #%llu@%d: %lld keys, %lld bytes",
                (unsigned long long)output_number, compact->compaction->level(),
                (unsigned long long)current_entries,
                (unsigned long long)current_bytes);
        }
        return s;
    }
    /**
     * @brief 将 Compaction 的结果提交到数据库的版本系统VersionSet中，更新元数据并删除旧文件
     * 
     * @param compact 
     * @return Status 
     */
    Status DBImpl::InstallCompactionResults(CompactionState* compact) {
        mutex_.AssertHeld();// 确保当前线程持有互斥锁（保证原子性和线程安全）
        // 记录 Compaction 摘要日志（输入文件数、层级、输出总大小等）
        Log(options_.info_log, "Compacted %d@%d + %d@%d files => %lld bytes",
            compact->compaction->num_input_files(0), compact->compaction->level(),// 当前层输入文件数+当前层级
            compact->compaction->num_input_files(1), compact->compaction->level() + 1,// 下一层输入文件数+下一层级
            static_cast<long long>(compact->total_bytes));// 输出文件总大小
        compact->compaction->AddInputDeletions(compact->compaction->edit());// 通过 VersionEdit 记录需要删除的文件(当前层和下一层参与合并的文件) (通过 compaction 操作,将多层的数据合并到下一层,生成新的文件.旧的文件中的数据已经被新的文件所替代,因此旧文件不再需要,需要删除以避免数据冗余)  
        const int level = compact->compaction->level(); // 当前层级
        for (size_t i = 0; i < compact->outputs.size(); i++) {
            const CompactionState::Output& out = compact->outputs[i];// 获取输出文件元数据
            compact->compaction->edit()->AddFile(level + 1, out.number, out.file_size,
                                                out.smallest, out.largest);// 将输出文件添加到下一层（level + 1）的 VersionEdit 中
        }
        return versions_->LogAndApply(compact->compaction->edit(), &mutex_);// 将 VersionEdit 应用到 VersionSet，生成新版本并持久化到 MANIFEST 文件
    }
    /**
     * @brief 执行实际的 Compaction 工作:合并输入文件,生成新的有序 SSTable 文件,清理冗余数据
     * 
     * @param compact 
     * @return Status 
     */
    Status DBImpl::DoCompactionWork(CompactionState* compact) {
        const uint64_t start_micros = env_->NowMicros();// 记录 Compaction 开始时间
        int64_t imm_micros = 0;// 处理 Immutable MemTable 的时间（用于统计）
        // 记录 Compaction 操作日志（输入文件数及层级）
        Log(options_.info_log, "Compacting %d@%d + %d@%d files",
            compact->compaction->num_input_files(0), compact->compaction->level(),// 当前层输入文件数+当前层级
            compact->compaction->num_input_files(1),// 下一层输入文件数
            compact->compaction->level() + 1);// 下一层级
        assert(versions_->NumLevelFiles(compact->compaction->level()) > 0);// 当前层必须有文件
        assert(compact->builder == nullptr);// 构建器必须未初始化
        assert(compact->outfile == nullptr);// 输出文件句柄必须未打开
        if(snapshots_.empty())// 无活跃快照时，使用当前最大序列号进行后续保留操作   如果没有活跃快照，当前可以认为没有用户依赖于旧版本的数据
            compact->smallest_snapshot = versions_->LastSequence();
        else// 有快照时，使用最旧快照的序列号   使用最旧快照的序列号作为 smallest_snapshot,是为了保留所有可能被快照引用的数据
            compact->smallest_snapshot = snapshots_.oldest()->sequence_number();
        Iterator* input = versions_->MakeInputIterator(compact->compaction);// 生成一个归并排序的迭代器,每次选取最小的键写入文件
        mutex_.Unlock();
        input->SeekToFirst();// 定位到第一个键值对
        Status status;
        ParsedInternalKey ikey;
        std::string current_user_key;// 当前处理的用户键（用于去重）
        bool has_current_user_key = false; // 是否已记录当前用户键
        SequenceNumber last_sequence_for_key = kMaxSequenceNumber;// 当前用户键的最大序列号
        // 遍历所有输入键值对
        while(input->Valid() && !shutting_down_.load(std::memory_order_acquire)) {
            if(has_imm_.load(std::memory_order_relaxed)) {// 优先处理 Immutable MemTable（若存在）
                const uint64_t imm_start = env_NowMicros();
                mutex_.Lock();
                if(imm_!=nullptr) {
                    CompactMemTable();// 刷新 Immutable MemTable 到 Level 0
                    background_work_finished_signal_.SignalAll();// 唤醒等待线程
                }
                mutex_.Unlock();
                imm_micros += (env_->NowMicros()-imm_start);// 累计处理时间
            }
            Slice key = input->key();// 当前键
            if(compact->compaction->ShouldStopBefore(key) && compact->builder!=nullptr) {// 检查是否需要停止当前输出文件并创建新文件(如:避免新文件与祖父层文件重叠过多)
                status = FinishCompactionOutputFile(compact, input);// 完成当前文件
                if(!status.ok())
                    break;
            }
            // 解析键并判断是否需要丢弃当前键值对
            bool drop = false;// 标记是否丢弃
            if(!ParseInternalKey(kt, &ikey)) {// 解析失败（数据损坏）
                current_user_key.clear();
                has_current_user_key = false;
                last_sequence_for_key = kMaxSequenceNumber;
            }
            else {
                // 判断用户键是否变化（新键或重复键）
                if(!has_current_user_key || user_comparator()->Compare(ikey.user_key, Slice(current_user_key))!=0) {
                    current_user_key.assign(ikey.user_key.data(), ikey.user_key.size());// 覆盖键(新键或重复,重复键会直接覆盖,新键的话就是创建)
                    has_current_user_key = true;
                    last_sequence_for_key = kMaxSequenceNumber;// 重置序列号
                }
                // 判断是否应丢弃当前键值对：
                // 序列号 <= 最小快照序列号（旧数据）  要丢弃
                // 有删除标记且无更高层数据需要保留    要丢弃
                if(last_sequence_for_key<=compact->smallest_snapshot)// 序列号 <= 最小快照序列号（旧数据）
                    drop = true;
                // 有删除标记且无更高层数据需要保留    要丢弃
                else if(ikey.type==kTypeDeletion && ikey.sequence<=compact->smallest_snapshot && compact->compaction->IsBaseLevelForKey(ikey.user_key))
                    drop = true;
                last_sequence_for_key = ikey.sequence;// 更新当前键的最大序列号
            }
            #if 0
                Log(options_.info_log,
                    ""  Compact: %s, seq %d, type: %d %d, drop: %d, is_base: %d, "
                    "%d smallest_snapshot: %d",
                    ikey.user_key.ToString().c_str(),
                    (int)ikey.sequence, ikey.type, kTypeValue, drop,
                    compact->compaction->IsBaseLevelForKey(ikey.user_key),
                    (int)last_sequence_for_key, (int)compact->smallest_snapshot);
                }
            #endif
            if(!drop) {// 处理有效键（非丢弃状态）
                if(compact->builder==nullptr) {// 若当前无构建器
                    status = OpenCompactionOutputFile(compact);// 创建新文件,即这个SST文件是不需要丢弃的
                    if(!status.ok())
                        break;
                }
                // 更新输出文件的键范围
                if(compact->builder->NumEntries()==0)
                    compact->current_output()->smallest.DecodeFrom(key);// 最小键
                compact->current_output()->largest.DecodeFrom(key);// 最大键
                compact->builder->Add(key, input->value());// 写入键值对到内存缓冲区,不是直接写入到SST中
                if(compact->builder->FilerSize()>=compact->compaction->MaxOutFileSize()) {// 检查文件大小是否超限，若超限则完成当前文件
                    status = FinishCompactionOutputFile(compact, input);
                    if(!status.ok())
                        break;
                }
            }
            input->Next();// 移动到下一个键值对
        }
        if(status.ok() && shutting_down_.load(std::memory_order_acquire))// 检查数据库是否正在关闭
            status = Status::IOError("Deleting DB during compaction");
        if (status.ok() && compact->builder != nullptr)// 完成最后一个输出文件的工作  主循环可能是由于输入迭代器没有数据了(数据以及写入到输出文件中,但可能还没构建SST完成)或数据库关闭而退出,此时需要将输出文件构建成SST,即调用FinishCompactionOutputFile()
            status = FinishCompactionOutputFile(compact, input);
        if (status.ok())
            status = input->status();// 检查输入迭代器的最终状态
        delete input;// 释放输入迭代器
        input = nullptr;
        // 统计 Compaction 耗时和 I/O 数据量
        CompactionStats stats;
        stats.micros = env_->NowMicros()-start_micros-imm_micros;// 总耗时（扣除 Immutable 处理时间）
        for(int which=0;which<2;++which) {// 统计读取量（当前层和下一层输入文件的总大小）
            for(int i=0;i<compact->compaction->num_input_files(which);++i) 
                stats.bytes_read += compact->compaction->input(which, i)->file_size;
        }
        for(size_t i=0;i<compact->outputs.size();++i) // 统计写入量（所有输出文件的总大小）
            stats.bytes_written += compact->outputs[i].file_size;
        // 记录统计信息并提交 Compaction 结果
        mutex_.Lock();
        stats_[compact->compaction->level()+1].Add(stats);// 更新层级统计
        if(status.ok())
            status = InstallCompactionResults(compact);// 提交版本变更
        if(!status.ok())
            RecordBackgroundError(status);// 记录后台错误
        // 记录 Compaction 完成后的层级状态
        VersionSet::LevelSummaryStorage tmp;
        Log(options_.info_log, "compacted to: %s", versions_->LevelSummary(&tmp));
        return status;
    }
    namespace {
        /**
         * @brief 保存使用迭代器过程中所需的共享信息,确保这些信息生命周期内的线程安全
         * 
         */
        struct IterState {
            port::Mutex* const mu;// 互斥锁，保护 version/mem/imm 的访问
            Version* const version GUARDED_BY(mu);// 当前数据库版本（SSTable 文件元数据）
            MemTable+* const mem GUARDED_BY(mu);// 可写的内存表（活跃 MemTable）
            MemTable* const imm GUARDED_BY(mu);// 不可变的内存表（待刷盘的 Immutable MemTable）
            IterState(port::Mutex* mutex, MemTable* mem, MemTable* imm, Version* version)
                : mu(mutex), version(version), mem(mem), imm(imm)
            {}
        };
        /**
         * @brief 迭代器销毁时的回调，释放迭代器关联的共享状态(mu, version, mem, imm)
         * 
         * @param arg1 
         * @param arg2 
         */
        static void CleanupIteratorState(void* arg1, void* arg2) {
            IterState* state = reinterpret_cast<IterState*>(arg1);
            state->mu._Lock();
            state->mem->Unref();// 释放 mem 的引用
            if(state->imm!=nullptr)
                state->imm->Unref();// 释放 imm 的引用（若存在）
            state->version->Unref();// 释放 version 的引用
            state->mu->Unlock();
            delete state;// 释放 IterState 自身内存
        }
    }
    /**
     * @brief 创建一个内部迭代器,用于合并内存表(MemTable)、不可变内存表(Immutable MemTable)和磁盘SSTable文件中的数据
     * 
     * @param options 
     * @param latest_snapshot 
     * @param seed 
     * @return Iterator* 
     */
    Iterator* DBImpl::NewInternalIterator(const ReadOptions& options, SequenceNumber* latest_snapshot, uint32_t* seed) {
        mutex_.Lock();
        *latest_snapshot = versions_->LastSequence();// 获取当前数据库的最新序列号，用于快照
        std::vector<Iterator*> list;// 创建一个迭代器列表，用于存储多个输入迭代器
        list.emplace_back(mem_->NewIterator());// 添加当前内存表（MemTable）的迭代器
        mem_->Ref();// 引用计数，防止内存表在使用过程中被销毁
        if(imm_!=nullptr) {// 如果存在不可变内存表（Immutable MemTable），将其迭代器添加到列表
            list.emplace_back(imm_->NewIterator());
            imm_->Ref();
        }
        // 添加所有层级 SSTable 文件的迭代器
        versions_->current()->AddIterators(options, &list);// 为每个 SSTable 文件创建迭代器
        Iterator* internal_iter = NewMergingIterator(&internal_comparator_, &list[0], list.size());// 创建一个合并迭代器，按照内部比较器（InternalComparator）合并多个输入迭代器(MemTable迭代器、Immutable MemTable迭代器、SSTable迭代器)
        versions_->current()->Ref();
        IterState* cleanup = new IterState(&mutex_, mem_, imm_, versions_->current());//  绑定迭代器清理逻辑（确保迭代器销毁时释放资源）
        internal_iter->RegisterCleanup(CleanupIteratorState, cleanup, nullptr);// 注册清理回调（当迭代器销毁时调用 CleanupIteratorState）
        *seed = ++seed_;// 生成一个唯一的种子值，可能用于某些随机操作或标识符
        mutex_.Unlock();
        return internal_iter;// 返回合并的内部迭代器
    }
    /**
     * @brief 返回下一个层级的文件与当前层级文件的重叠字节数
     * 
     * @return int64_t 
     */
    int64_t DBImpl::TEST_MaxNextLevelOverlappingBytes() {
        MutexLock l(&mutex_);
        return versions_->MaxNextLevelOverlappingBytes();// 调用版本管理系统（VersionSet）的方法，返回下一个层级的文件与当前层级文件的重叠字节数
    }
    /**
     * @brief 从数据库中获取指定键和相应序列号对应的值
     * 
     * @param options 
     * @param key 
     * @param value 
     * @return Status 
     */
    Status DBImpl::Get(const ReadOptions& options, const Slice& key, std::string* value) {
        Status s;
        MutexLock l(&mutex_);
        SequenceNumber snapshot;// 获取快照的序列号
        if(options.snapshot!=nullptr)// 如果用户提供了快照，使用该快照的序列号
            snapshot = static_cast<const SnapshotImpl*>(options.snapshot)->sequence_number();
        else// 否则，使用当前版本管理器的最后一个序列号    
            snapshot = versions_->LastSequence();
        MemTable* mem = mem_;// 获取当前的内存表
        MemTable* imm = imm_;// 获取当前不可变内存表
        Version* current = versions_->current();
        mem->Ref();// 增加引用计数，确保内存表使用期间不会被销毁
        if(imm!=nullptr)
            imm->Ref();
        current->Ref();// // 增加引用计数，确保版本管理器在使用期间不会被销毁
        bool have_stat_update = false;// 初始化统计信息标记
        Version::GetStats stats;
        {
            mutex_.Unlock();
            LookupKey lkey(key, snapshot);// 构造查找键，包含用户提供的键和快照的序列号
            if(mem->Get(lkey, value, &s))// 尝试从当前内存表中查找键值对
            
            else if(imm!=nullptr&&imm->Get(lkey, value, &s))// 否则，尝试从不可变内存表中查找
            
            else {// 否则，从磁盘上的 SSTable 文件中查找
                s = current->Get(options, lkey, value, &stats);
                have_stat_update = true;
            }
            mutex_.Lock();
        }
        if(have_stat_update && current->UpdateStats(stats))// 根据收集到的统计信息来决定是否可能进行紧凑化处理
            MaybeScheduleCompaction();
        // 减少引用计数，释放内存表和版本管理器
        mem->Unref();
        if(imm!=nullptr)
            imm->Unref();
        current->Unref();
        return s;
    }
    /**
     * @brief 方便地创建一个迭代器，用于遍历数据库中的数据，同时支持快照和性能优化
     * 
     * @param options 
     * @return Iterator* 
     */
    Iterator* DBImpl::NewIterator(const ReadOptions& options) {
        SequenceNumber latest_snapshot;
        uint32_t seed;
        Iterator* iter = NewInternalIterator(options, &latest_snapshot, *seed);// 调用 NewInternalIterator 创建一个内部迭代器
        // 使用 NewDBIterator 方法封装内部迭代器
        // NewDBIterator 会创建一个更高层次的迭代器，提供用户友好的接口，并管理快照和上下文信息
        return NewDBIterator(this, user_comparator(), iter,
                                (options.snapshot!=nullptr
                                    ? static_cast<const SnapshotImpl*>(options.snapshot)
                                        ->sequence_number():latest_snapshot), seed);
    }
    /**
     * @brief 记录键的读取样本，用于触发可能的 Compaction 优化
     * 
     * @param key 
     */
    void DBImpl::RecordReadSample(Slice key) {
        MutexLock l(&mutex_);
        if(versions_->current()->RecordReadSample(key))// 调用 Version 的统计方法，若返回 true 表示需要触发 Compaction
            MaybeScheduleCompaction();// 调度后台 Compaction
    }
    /**
     * @brief 创建一个数据库快照，用于读取一致性视图
     * 
     * @return const Snapshot* 
     */
    const Snapshot* DBImpl::GetSnapshot() {
        MutexLock l(&mutex_);
        return snapshots_.New(versions_->LastSequence());// 生成新快照，基于当前最新序列号（原子性获取）
    }
    /**
     * @brief 释放不再使用的快照资源
     * 
     * @param snapshot 
     */
    void DBImpl::ReleaseSnapshot(const Snapshot* snapshot) {
        MutexLock l(&mutex_);
        snapshots_.Delete(static_cast<const SnapshotImpl*>(snapshot));// 将基类快照指针转换为内部实现类，并标记删除
    }   
    /**
     * @brief 写入键值对的便捷接口（继承自基类 DB）
     * 
     * @param options 
     * @param key 
     * @return Status 
     */
    Status DBImpl::Put(const WriteOptions& o, const Slice& key, const Slice& val) {
        return DB::Put(o, key, val);
    }
    /**
     * @brief 删除指定键的便捷接口（继承自基类 DB）
     * 
     * @param options 
     * @param key 
     * @return Status 
     */
    Status DBImpl::Delete(const WriteOptions& options, const Slice& key) {
        return DB::Delete(options, key);
    }
    Status DBImpl::Write(const WriteOptions& options, WriteBatch* updates) {
        Writer w(&mutex_);// 封装写入请求和同步工具
        w.batch = updates;// 待写入的批量操作
        w.sync = options.sync;// 是否同步刷盘
        w.done = false;// 标记是否完成
        MutexLock l(&mutex_);
        writers_.emplace_back(&w);// 将当前写入请求加入等待队列
        while(!w.done && &w!=writers_.front())// 等待条件：当前请求不是队首或尚未完成
            w.cv.Wait();
        if(w.done)// 若已被其他线程处理完成（合并写入），直接返回状态
            return w.status;
        Status status = MakeRoomForWrite(updates==nullptr);// 确保有足够空间进行写入（可能触发 MemTable 切换或 Compaction）
        uint64_t last_sequence = versions_->LastSequence();// 当前最新序列号
        Writer* last_writer = &w;// 记录最后一个待处理的写入请求
        // 仅处理非空批量写入（空批量用于 Compaction 等内部操作）
        if(status.ok() && updates!=nullptr) {
            WriterBatch* write_batch = BuildBatchGroup(&last_writer);// 合并多个等待中的写入请求为一个批量（减
            WriteBatchInternal::SetSequence(write_batch, last_sequence + 1);// 为批量中的每个操作分配递增的序列号
            last_sequence += WriteBatchInternal::Count(write_batch);// 更新序列号
            // 写入 WAL 日志并应用至 MemTable
            {
            mutex_.Unlock();
            status = log_->AddRecord(WriteBatchInternal::Contents(write_batch));// 写日志
            bool sync_error = false;
            if (status.ok() && options.sync) {// 若需同步，确保日志刷盘
                status = logfile_->Sync();
                if (!status.ok()) 
                    sync_error = true;
            }
            // 将批量操作插入 MemTable
            if (status.ok()) 
                status = WriteBatchInternal::InsertInto(write_batch, mem_);
            mutex_.Lock();
            if (sync_error)
                RecordBackgroundError(status);
            }
            // 清空临时合并批量（复用内存）
            if (write_batch == tmp_batch_) 
                tmp_batch_->Clear();
            versions_->SetLastSequence(last_sequence);// 更新全局序列号
        }
        // 由于合并写入操作一次可能会处理多个writers_队列中的元素,因此此处将所有已经处理的元素状态进行变更,并且发送signal信号
        while (true) {
            Writer* ready = writers_.front();
            writers_.pop_front();
            if (ready != &w) {
                ready->status = status;
                ready->done = true;
                ready->cv.Signal();
            }
            if (ready == last_writer) 
                break;
        }
        if (!writers_.empty()) 
            writers_.front()->cv.Signal();
        return status;
    }

}