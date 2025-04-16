1. `ptmalloc`:`glibc`默认分配器
      - 内存管理单元：
         * 分为主分配区和非主分配区,主分配区使用`brk`扩展堆,非主分配区使用`mmap`
         * 通过`fast bins`(小内存单链表)、`small bins`(固定大小双向链表)、`large bins`(范围块链表)管理空闲内存块 
      - 内存回收：
         * 延迟合并:释放的内存先放入`unsorted bin`,避免频繁合并影响性能
         * `top chunk`机制:优先从堆顶分配,仅在连续空闲内存超过阈值时归还操作系统  
  
2. `tcmalloc`
    * `tcmalloc`:`LevelDB`的非内存池的内存分配就是用的`tcmalloc`,而不是普通的`malloc`,它比`ptmalloc2`性能更好
      - 多级缓存:
         * 线程本地缓存:小对象(<=32KB)无锁分配,减少竞争
         * 中心缓存:大对象(>32KB)通过自旋锁分配,按页对齐管理

3. `jemalloc` 