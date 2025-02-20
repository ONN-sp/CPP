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
    void DBImpl::CompactRange(const Slice* begin, const Slice* end) {
        int max_level_with_files = 1;
        {
            MutexLock l(&mutex);
            Version* base = versions_->current();
            for(int level = 1;level<config::kNumLevels;level++) {
                if(base->OverlapInLevel(level, begin, end))
                    max_level_with_files = level;
            }
        }
        TEST_CompactMemTable();
        for(int level=0;level<max_level_with_files;level++) 
            TEST_CompactMemTable(level, begin, end);
    }
    // 下面是两个测试程序TEST_CompactRange()、TEST_CompactMemTable()
    void DBImap::TEST_CompactRange(int level, const Slice* begin, const Slice* end) {
        assert(level>=0);
        assert(level+1<config::kNumLevels);
        InternalKey begin_storage, end_storage;
        ManualCompaction manual;
        manual.level = level;
        manual.done = false;
        if(begin==nullptr)
            manual.begin = nullptr;
        else {
            begin_storage = InternalKey(*begin, kMaxSequenceNumber, kValueTypeForSeek);
            manual.begin = &begin_storage;
        }
        if (end == nullptr)
            manual.end = nullptr;
        else {
            end_storage = InternalKey(*end, 0, static_cast<ValueType>(0));
            manual.end = &end_storage;
        }
        MutexLock l(&mutex_);
        while(!manual.done && !shutting_down_.load(std::memory_order_acquire) && bg_error_.ok()) {
            if(manual_compaction_ == nullptr) {
                manual_compaction_ = &manual;
                MaybeScheduleCompaction();
            }
            else 
                background_work_finished_signal_.Wait();
        }
         while (background_compaction_scheduled_) 
            background_work_finished_signal_.Wait();
        if (manual_compaction_ == &manual)
            manual_compaction_ = nullptr;
    }
    Status DBImpl::TEST_CompactMemTable() {
        Status s = Write(WriteOptions(), nullptr);
        if (s.ok()) {
            MutexLock l(&mutex_);
            while (imm_ != nullptr && bg_error_.ok()) 
                background_work_finished_signal_.Wait();
            if (imm_ != nullptr) 
                s = bg_error_;
        }
        return s;
    }
    void DBImpl::RecordBackgroundError(const Status& s) {
        mutex_.AssertHeld();
        if(bg_error_.ok()) {
            bg_error_ = s;
            background_work_finished_signal_.SignalAll();
        }
    }
    void DBImpl::MaybeScheduleCompaction() {
        mutex_.AssertHeld();
        if(background_compaction_scheduled_)

        else if(shutting_down_.load(std::memory_order_acquire))

        else if(!bg_error_.ok())

        else if(imm_==nullptr && manual_compaction_ == nullptr && !versions_->NeedsCompaction())

        else {
            background_compaction_scheduled_ = true;
            env_->Schedule(&DBImpl::BGWork, this);
        }
    }
    void DBImpl::BGWork(void* db) {
        reinterpret_cast<DBImpl*>(db)->BackgroundCall();
    }

}