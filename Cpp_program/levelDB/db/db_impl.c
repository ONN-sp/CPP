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


}