#ifndef LEVELDB_DB_DB_IMPL_H_
#define LEVELDB_DB_DB_IMPL_H_

#include <atomic>
#include <deque>
#include <set>
#include <string>

#include "dbformat.h"
#include "log_writer.h"
#include "snapshot.h"
#include "../include/leveldb/db.h"
#include "../include/leveldb/env.h"
#include "../port/port.h"
#include "../port/thread_annotations.h"

namespace leveldb {
    class MemTable;
    class TableCache;
    class Version;
    class VersionEdit;
    class VersionSet;
    // DBImpl 是 LevelDB 的核心实现类，继承自 DB 接口类
    class DBImpl : public DB {
        public:
            DBImpl(const Options& options, const std::string& dbname);
            // 禁止拷贝构造函数和赋值操作符
            DBImpl(const DBImpl&) = delete;
            DBImpl& operator=(const DBImpl&) = delete;
            ~DBImpl() override;
            // 实现 DB 接口中的方法
            // 插入键值对
            Status Put(const WriteOptions&, const Slice& key,
                        const Slice& value) override;
            // 删除指定键
            Status Delete(const WriteOptions&, const Slice& key) override;
            // 批量写入操作
            Status Write(const WriteOptions& options, WriteBatch* updates) override;
            // 获取指定键的值
            Status Get(const ReadOptions& options, const Slice& key,
                        std::string* value) override;
            // 创建一个迭代器，用于遍历数据库中的键值对
            Iterator* NewIterator(const ReadOptions&) override;
            // 获取当前数据库的快照
            const Snapshot* GetSnapshot() override;
            // 释放快照
            void ReleaseSnapshot(const Snapshot* snapshot) override;
            // 获取数据库的某个属性
            bool GetProperty(const Slice& property, std::string* value) override;
            // 获取指定范围内键的近似大小
            void GetApproximateSizes(const Range* range, int n, uint64_t* sizes) override;
            // 压缩指定范围内的键
            void CompactRange(const Slice* begin, const Slice* end) override;
            // 以下方法用于测试，不在公共接口中
            // 压缩指定层级中与 [*begin,*end] 重叠的文件
            void TEST_CompactRange(int level, const Slice* begin, const Slice* end);
            // 强制将当前内存表内容压缩到磁盘
            Status TEST_CompactMemTable();
            // 返回一个内部迭代器，用于遍历数据库的当前状态
            Iterator* TEST_NewInternalIterator();
            // 返回下一层级中与当前层级文件重叠的最大字节数
            int64_t TEST_MaxNextLevelOverlappingBytes();
            // 记录在指定内部键处读取的字节数样本
            void RecordReadSample(Slice key);
        private:
            friend class DB;// 接口定义类DB,即父类
            struct CompactionState;
            struct Writer;
            // 手动压缩的信息
            struct ManualCompaction {
                int level;  // 压缩的层级
                bool done;  // 压缩是否完成
                const InternalKey* begin;  // 压缩的起始键，null 表示键范围的开始
                const InternalKey* end;    // 压缩的结束键，null 表示键范围的结束
                InternalKey tmp_storage;   // 用于跟踪压缩进度
            };
            // 每层压缩的统计信息
            struct CompactionStats {
                CompactionStats() : micros(0), bytes_read(0), bytes_written(0) {}
                // 累加压缩统计信息
                void Add(const CompactionStats& c) {
                this->micros += c.micros;
                this->bytes_read += c.bytes_read;
                this->bytes_written += c.bytes_written;
                }
                int64_t micros;        // 压缩所花费的时间（微秒）
                int64_t bytes_read;    // 压缩过程中读取的字节数
                int64_t bytes_written; // 压缩过程中写入的字节数
            };
            // 创建一个内部迭代器
            Iterator* NewInternalIterator(const ReadOptions&,
                                            SequenceNumber* latest_snapshot,
                                            uint32_t* seed);
            // 创建一个新的数据库
            Status NewDB();
            // 从持久化存储中恢复数据库描述符
            Status Recover(VersionEdit* edit, bool* save_manifest)
                EXCLUSIVE_LOCKS_REQUIRED(mutex_);
            // 忽略某些错误
            void MaybeIgnoreError(Status* s) const;
            // 删除不需要的文件和过时的内存条目
            void RemoveObsoleteFiles() EXCLUSIVE_LOCKS_REQUIRED(mutex_);
            // 将内存中的写缓冲区压缩到磁盘
            void CompactMemTable() EXCLUSIVE_LOCKS_REQUIRED(mutex_);
            // 恢复日志文件
            Status RecoverLogFile(uint64_t log_number, bool last_log, bool* save_manifest,
                                    VersionEdit* edit, SequenceNumber* max_sequence)
                EXCLUSIVE_LOCKS_REQUIRED(mutex_);
            // 将内存表写入 Level 0 的 SSTable
            Status WriteLevel0Table(MemTable* mem, VersionEdit* edit, Version* base)
                EXCLUSIVE_LOCKS_REQUIRED(mutex_);
            // 为写操作腾出空间
            Status MakeRoomForWrite(bool force)
                EXCLUSIVE_LOCKS_REQUIRED(mutex_);
            // 构建批量写操作组
            WriteBatch* BuildBatchGroup(Writer** last_writer)
                EXCLUSIVE_LOCKS_REQUIRED(mutex_);
            // 记录后台错误
            void RecordBackgroundError(const Status& s);
            // 可能调度压缩操作
            void MaybeScheduleCompaction() EXCLUSIVE_LOCKS_REQUIRED(mutex_);
            // 后台工作线程的入口函数
            static void BGWork(void* db);
            // 后台调用函数
            void BackgroundCall();
            // 后台压缩操作
            void BackgroundCompaction() EXCLUSIVE_LOCKS_REQUIRED(mutex_);
            // 清理压缩状态
            void CleanupCompaction(CompactionState* compact)
                EXCLUSIVE_LOCKS_REQUIRED(mutex_);
            // 执行压缩工作
            Status DoCompactionWork(CompactionState* compact)
                EXCLUSIVE_LOCKS_REQUIRED(mutex_);
            // 打开压缩输出文件
            Status OpenCompactionOutputFile(CompactionState* compact);
            // 完成压缩输出文件
            Status FinishCompactionOutputFile(CompactionState* compact, Iterator* input);
            // 安装压缩结果
            Status InstallCompactionResults(CompactionState* compact)
                EXCLUSIVE_LOCKS_REQUIRED(mutex_);
            // 返回用户定义的比较器
            const Comparator* user_comparator() const {
                return internal_comparator_.user_comparator();
            }
            Env* const env_;  // 环境接口，用于文件操作等
            const InternalKeyComparator internal_comparator_;  // 内部键比较器
            const InternalFilterPolicy internal_filter_policy_;  // 内部过滤器策略
            const Options options_;  // 数据库选项
            const bool owns_info_log_;  // 是否拥有 info_log_
            const bool owns_cache_;  // 是否拥有 cache_
            const std::string dbname_;  // 数据库名称
            TableCache* const table_cache_;  // 表缓存，用于管理 SSTable 文件
            // 保护持久化数据库状态的锁
            FileLock* db_lock_;
            port::Mutex mutex_;  // 互斥锁，用于保护共享状态
            std::atomic<bool> shutting_down_;  // 数据库是否正在关闭
            port::CondVar background_work_finished_signal_ GUARDED_BY(mutex_);  // 后台工作完成信号
            MemTable* mem_;  // 当前的内存表
            MemTable* imm_ GUARDED_BY(mutex_);  // 正在被压缩的内存表
            std::atomic<bool> has_imm_;  // 后台线程可以检测到非空的 imm_
            WritableFile* logfile_;  // 当前的日志文件
            uint64_t logfile_number_ GUARDED_BY(mutex_);  // 当前日志文件的编号
            log::Writer* log_;  // 日志写入器
            uint32_t seed_ GUARDED_BY(mutex_);  // 用于采样
            // 写操作队列
            std::deque<Writer*> writers_ GUARDED_BY(mutex_);
            WriteBatch* tmp_batch_ GUARDED_BY(mutex_);  // 临时批量写操作
            SnapshotList snapshots_ GUARDED_BY(mutex_);  // 快照列表
            // 正在进行的压缩操作中需要保护的文件集合
            std::set<uint64_t> pending_outputs_ GUARDED_BY(mutex_);
            // 是否已经调度或正在运行后台压缩
            bool background_compaction_scheduled_ GUARDED_BY(mutex_);
            ManualCompaction* manual_compaction_ GUARDED_BY(mutex_);  // 手动压缩信息
            VersionSet* const versions_ GUARDED_BY(mutex_);  // 版本集合
            // 在 paranoid 模式下是否遇到过后台错误
            Status bg_error_ GUARDED_BY(mutex_);
            // 每层的压缩统计信息
            CompactionStats stats_[config::kNumLevels] GUARDED_BY(mutex_);
    };
    // 清理数据库选项
    Options SanitizeOptions(const std::string& db,
                            const InternalKeyComparator* icmp,
                            const InternalFilterPolicy* ipolicy,
                            const Options& src);

    }
#endif