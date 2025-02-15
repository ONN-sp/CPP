#ifndef LEVELDB_INCLUDE_DB_H
#define LEVELDB_INCLUDE_DB_H

#include <cstdint>
#include <cstdio>

#include "export.h"// 导出符号的宏定义
#include "iterator.h"// 迭代器接口
#include "options.h"// 配置选项

namespace leveldb {
    // 前向声明,避免头文件依赖
    struct options;
    struct ReadOptions;
    struct WriteOptions;
    class WriteBatch;

    class LEVELDB_EXPORT Snapshot {
        protected:
            virtual ~Snapshot();
    };
    // 表示一个键的范围[start, limit)
    struct LEVELDB_EXPORT Range {
        Range() = default;
        Range(const Slice& s, const Slice& l) : start(s), limit(l) {}
        Slice start;// 范围的起始键
        Slice limit;// 范围的结束键
    };
    class LEVELDB_EXPORT DB {
        public:
            // 打开指定名称的数据库
            // 成功时，将堆分配的数据库指针存储在 *dbptr 中并返回 OK
            // 失败时，将 nullptr 存储在 *dbptr 中并返回非 OK 状态
            static Status Open(const Options& options, const std::string& name, DB** dbptr);// name用于指定数据库的名称
            DB() = default;
            DB(const DB&) = delete;// 禁用拷贝构造函数
            DB& operator=(const DB&) = delete;// 禁用赋值运算符
            virtual ~DB();
            // 将键key对应的数据库条目设置为value.成功时返回OK,失败返回非OK
            // 注意：建议将 options.sync 设置为 true
            virtual Status Put(const WriteOptions& options, const Slice& key, const Slice& value) = 0;
            // delete操作可以看成一种特殊的数据库写操作.LevelDB中的删除操作并不是直接将数据从磁盘中清楚,而是对应位置插入一个key的删除标志,然后在后续的Compaction过程中才最终去除这条key-value记录
            // 注意：建议将 options.sync 设置为 true
            virtual Status Delete(const WriteOptions& options, WriteBatch* updates) = 0;
            // 注意：建议将 options.sync 设置为 true
            virtual Status Write(const WriteOptions& options, WriteBatch* updates) = 0;
            // 注意：建议将 options.sync 设置为 true
            virtual Status Get(const ReadOptions& options, const Slice& key, std::string* value) = 0;
            // 创建一个新的迭代器对象.需要注意的是:该接口返回的Iterator对象并不能直接使用,只有在调用相应的Seek方法之后才能进行对应的迭代操作
            // 当不在需要使用迭代器Iterator对象时,需要使用delete对该指针对象进行删除,以免造成内存泄漏
            virtual Iterator* NewIterator(const ReadOptions& options) = 0;
            // 返回当前数据库状态的句柄.使用此句柄创建的迭代器将观察到当前数据库状态的稳定快照
            virtual const Snapshot* GetSnapshot() = 0;
            // 释放先前获取的快照
            virtual void ReleaseSnapshot(const Snapshot* snapshot) = 0;
            // 获取数据库的某个属性
            virtual bool GetProperty(const Slice& property, std::string* value) = 0;
            // 获取指定范围内键的近似大小
            virtual void GetApproximateSizes(const Range* range, int n, uint64_t* sizes) = 0;
            // 压缩键范围 [*begin,*end] 的底层存储
            virtual void CompactRange(const Slice* begin, const Slice* end) = 0;
            // 销毁指定数据库的内容
            LEVELDB_EXPORT Status DestroyDB(const std::string& name, const Options& options);
            // 如果无法打开数据库，可以尝试调用此方法以尽可能恢复数据库内容
            // 可能会丢失一些数据，因此在包含重要信息的数据库上调用此函数时要小心
            LEVELDB_EXPORT Status RepairDB(const std::string& dbname, const Options& options);
    };
}

#endif