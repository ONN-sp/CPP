1. 本项目是对`Google`公司的`LevelDB`项目的学习
2. `LevelDB`是基于`LSM`树进行存储
3. `LeveldDB`项目是对`Google`的`Bigtable`技术原理的体现,`LevelDB`是`Bigtable`的单机版
4. `LevelDB`是一个`C++`编写的高效键值嵌入式数据库,目前对亿级的数据有着非常好的读写性能
5. `LevelDB`的优点:
   * `key`和`value`采用字符串形式,且长度没有限制
   * 数据能持久化存储,同时也能将数据缓存到内存,实现快速读取
   * 基于`key`按序存放数据,并且`key`的排序比较函数可以根据用户需求进行定制
   * 支持简易的操作接口`API`,如`Put()、Get()、Delete()`,并支持批量写入
   * 可以针对数据创建数据内存快照
   * 支持前向、后向的迭代器
   * 采用`Google`的`Snappy`压缩算法对数据进行压缩,以减少存储空间
   * 基本不依赖其它第三方模块,可非常容易地移植到`Windows、Linux、UNIX、Android、iOS`
6. `LevelDB`的缺点:
   * 不是传统的关系数据库,不支持`SQL`(用于管理和操作关系型数据库的标准编程语言)查询与索引
   * 只支持单进程,不支持多进程
   * 不支持多种数据类型
   * 不支持客户端-服务器的访问模式.用户在应用时,需要自己进行网络服务的封装
   * 不支持分布式存储,无法直接扩展到多台机器,即为单机存储
7. `LevelDB`的两个衍生产品——`RocksDB`(`Facebook`)、`SSDB`     
8. `LevelDB`的核心操作:
   * 写操作
      - 写操作首先写入`Log`文件
      - 然后将数据插入到`MeeTable`中
      - 当`MemTable`达到一定大小时,会转换为`Immutable MemTable`,并出触发后台线程将其写入磁盘,生成`SSTable`文件
   * 读操作
      - 首先在`MemTable`中查找数据
      - 如果未找到,则在`Immutable MemTable`中查找
      - 如果仍未找到,则在各级`SSTable`文件中查找(从`Level 0`开始逐级查找)   
9. <mark>`LevelDB`中磁盘数据读取与缓存均以块为单位,并且实际存储中所有的数据记录均以`key`进行顺序存储.根据排序结果,相邻的`key`所对应的数据记录一般均会存储在同一个块中.因此,在针对需要经常同时访问的数据时,其`key`在命名时,可以通过将这些`key`设置相同的前缀保证这些数据的`key`是相邻近的,从而使这些数据可存储在同一个块内</mark>        
# Slice
1. `Slice`是`LevelDB`中的一种基本的、以字节为继承的数据存储单元,既可以存储`key`,也可以存储数据(`data`)
2. `Slice`是一种包含字节长度与指针的简单数据结构
3. 本项目为什么要使用`Slice`?
   一般来讲,`Slice`作为函数的返回值.相比于`C++`的`string`类型,如果在返回时采用`Slice`类型,则只需要返回长度与指针,而不需要复制长度较长的`key`和`value`.此外,`Slice`不以`'\0'`作为字符的终止符,可以存储值为`'\0'`的数据
# LSM树
1. `LSM`树是实现`LevelDB`的核心,其原理可以参考`"The log-structured merge-tree(LSM-tree)`论文
2. <mark>一般而言,在常规的物理硬盘存储介质上,顺序写比随机写速度要快,而`LSM`树正是充分利用了这一物理特性,从而实现对频繁、大量数据写操作的支持</mark>
3. </mark>`LSM`原理:下图中主要由常驻内存的`C0`树和保留在磁盘的`C1`树两个模块组成.虽然`C1`树保存在磁盘,但是会将`C1`树种一些需要频繁访问的数据保存在数据缓存中,从而保证高频访问数据的快速收敛.当需要插入一条新的数据记录时,首先在`Log`文件末尾顺序添加一条数据插入的记录,然后将这个新的数据添加到驻留在内存的`C0`树种,当`C0`树的数据大小达到某个阈值时,则会自动将数据迁移、保留在磁盘中的`C1`树,这就是`LSM`树的数据写的全过程.而对于数据读过程,`LSM`树将首先从`C0`树中进行索引查询,如果不存在,则转向`C1`中继续进行查询,直至找到最终的数据记录.对于`LevelDB`来说,`C0`树采用了`MemTable`数据结构实现,而`C1`树采用了`SSTable`</mark>
   ![](markdown图像集/2025-02-15-10-53-03.png)
# 快照Snopshot
1. 快照是数据存储某一时刻的状态记录
2. `DB->GetSnapshot()`回创建一个快照,实际上就是生成一个当前最新的`SequeceNumber`对应的快照节点,然后插入快照双向链表中
# MANIFEST
1. `MANIFEST`文件记录了数据库的元数据,包括:数据库的层级结构(`Level 0`到`Level N`);每个层级包含的 `SSTable`文件;每个`SSTable`文件的键范围(最小键和最大键);数据库的版本信息;日志文件的状态.`MANIFEST`文件是一个日志格式的文件
2. `MANIFEST`文件存储的其实是`versionEdit`信息,即版本信息变化的内容.一个`versionEdit`数据会被编码成一条记录,写入`MANIFEST`中.如下图就是一个`MANIFEST`文件的示意图,其中包含3条`versionEdit`记录,每条记录包括:(1)新增哪些`sst`文件;(2)删除哪些`sst`文件;(3)当前`compaction`的下标;(4)日志文件编号;(5)操作`seqNumber`等信息
   ![](markdown图像集/2025-02-17-21-53-40.png)
3. <mark>`Manifest`文件不存储在`MemTable`或`SSTable`中,而是与日志文件、`SSTable`文件共同构成数据库的持久化存储体系(即存储在磁盘上)</mark>
# CURRENT
1. `CURRENT`文件是一个指向当前正在使用的`MANIFEST`文件的指针.`LevelDB`通过`CURRENT`文件快速找到最新的`MNIFEST`文件,从而加载数据库的元数据
# db.h
1. 接口函数的函数参数为什么要使用引用常量?
   一般而言,`C++`函数输入/输出参数均采用传值的方式.然而在传值过程中需要调用参数对象的复制构造函数来创建相应的副本,这样传值必然有一定开销,进而影响代码的执行效率.传入的是引用对象,因而不会创建新的对象,所以不会存在构造函数与析构函数的钓调用,因而执行效率大大提升.另外,通过`const`进行常量声明,保证了引用参数在函数执行过程中不会被调用者修改
2. `db.h`是`LevelDB`对外暴露的主要`API`,用户可以通过该头文件中定义的类和接口来操作`LevelDB`数据库.`db.h`是`LevelDB`的核心抽象层,它定义了数据库的行为和接口,而不涉及具体的实现细节(具体的实现细节在`dp_impl.c`中实现)
# db_impl.h/db_impl.c  
1. `db_impl`主要是封装一些供客户端应用进行调用的接口,即头文件中的相关`API`函数接口,主要有15个函数接口,其中4个接口用于测试用途    
2. 为什么在`DBImpl::RemoveObsoleteFiles()`中最后删除文件的操作前要先释放锁?
   一方面,因为文件删除操作(`env_->RemoveFile`)涉及磁盘`I/O`,通常比较耗时.如果在持有锁的情况下执行删除操作,其他线程可能会被阻塞,导致系统并发性能下降.另一方面,在`RemoveObsoleteFiles`函数中,删除操作的目标是磁盘上的文件,而不是内存中的共享数据结构,因此删除操作不会影响数据库的核心状态(如内存表、版本信息等),所以不需要在锁的保护下进行   
3. 在`LevelDB`中,`DBImpl::Recover()`是从磁盘上的持久化数据(主要是`Manifest`文件(`Recover()`通过读取`Manifest`文件,恢复数据库的层级结构和文件信息,见下图)和日志文件)中恢复数据库的状态,并将其恢复到内存(`MemTable、SST`)中,`Recover()`会调用`RecoverLogFile()`
   ![](markdown图像集/2025-02-18-21-56-39.png)     
4. `Recover()`全过程举例:
   ![](markdown图像集/2025-02-18-21-59-06.png)
   ![](markdown图像集/2025-02-18-21-59-16.png)
   ![](markdown图像集/2025-02-18-21-59-42.png)
5. `DBImpl::Recover(VersionEdit* edit, bool* save_manifest)`中的`expected`存储的是什么?
   `versions_->AddLiveFiles(&expected)`会从`VersionSet`(版本集合)中提取当前所有活跃的`SSTable`文件的编号,并将这些编号插入到`expected`集合中.`VersionSet`是`LevelDB`管理版本的核心组件,记录了所有有效的`SSTable`文件及其层级信息."存活的"(`Live`)文件是指尚未被删除或合并(`Compaction`)的`SSTable`文件,它们属于当前数据库的最新快照.在恢复过程中,`LevelDB`会遍历数据库目录下的所有文件,并检查`expected`集合中的每个文件编号是否都有对应的物理文件(磁盘上的文件)存在,如:
   ![](markdown图像集/2025-02-17-22-37-46.png)
6. <mark>`Recover()`依赖`Manifest`恢复数据库的持久化数据,而`RecoverLogFile()`通过日志恢复内存中的临时数据</mark>
7. <mark>`RecoverLogFile()`函数的作用是从日志文件(`WAL`,`Write-Ahead Log`)中恢复数据,并将这些数据恢复到内存表(`MemTable`)中,最终可能写入到`SST`文件中</mark>.如:
   ![](markdown图像集/2025-02-18-13-57-58.png)
8. `RecoverLogFile()`中的日志复用:日志文件复用是一种优化机制,目的是减少日志文件的数量,避免频繁创建和删除日志文件,从而提高性能并减少文件系统的碎片化.日志文件复用的核心思想是:如果某个日志文件在恢复过程中没有被完全消耗(即其中的数据没有完全写入到`SST`文件中),并且满足特定条件,那么可以继续在该日志文件上追加新的日志记录,而不是创建新的日志文件.如:
   ![](markdown图像集/2025-02-18-14-00-47.png)
9. <span style="color:red;">数据恢复的过程就是依次应用`VersionEdit`的过程(`VersionEdit`表示一个`Version`到另一个`Version`的变更,为了避免进程崩溃或者机器宕机导致数据丢失,`LevelDB`把各个`VersionEdit`都持久化到磁盘,形成`MANIFEST`文件),其恢复的详细过程(`DBImpl::Recover()->VersionSet::Recover()`)如下:</span>
    ![](markdown图像集/2025-02-19-22-45-51.png)
    ![](markdown图像集/2025-02-19-22-46-04.png)
    需要注意的是,宕机后开始恢复时的`version`是空的,即它是从空的`version`按照`Manifest`中的一条条`VersionEdit`恢复到最新的`version`的

# version_set/version_edit
1. `LevelDB`用`Version`表示一个版本的元信息,主要是每个`Level`的`.ldb`文件.除此之外,`Version`还记录了触发`Compaction` 相关的状态信息,这些状态信息会在响应读写请求或者`Compaction`的过程中被更新.`VersionEdit`表示一个`Version`到另一个 `Version`的变更,为了避免进程崩溃或者机器宕机导致数据丢失,`LevelDB`把各个`VersionEdit`都持久化到磁盘,形成`MANIFEST`文件.数据恢复的过程就是依次应用`VersionEdit`的过程.`VersionSet`表示`LevelDB`历史`Version`信息,所有的`Version`都按照双向链表的形式链接起来.`VersionSet`和`Version`的大体布局如下：
   ![](markdown图像集/2025-02-19-22-50-29.png)