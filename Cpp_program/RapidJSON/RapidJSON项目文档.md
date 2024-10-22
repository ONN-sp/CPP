1. 本项目是对腾讯的`RapidJSON`项目的一个学习,其最初灵感来源于`RapidXML`
2. 本项目的关键性能优化方法:
   * 使用模板及内联函数降低函数调用的开销
   * SIMD指令优化
   * 自定义内存分配器
   * 零拷贝技术
   * 跳过空白字符的优化算法:利用SIMD实现的
   * 字符串处理算法:优化算法处理转义字符
   * 使用经过优化的Grisu2算法(`dtoa.h`)和专门的浮点数解析算法->`double-conversion`中自定义的`DIY-FP`算法(`diyfp.h strtod.h`) (标准库的`strtod()`函数较缓慢(因为它是面向广泛情况,即包括了异常值、极端情况、错误处理等,所以较为缓慢),因此自己重新实现了) (大多数`JSON`文档中的浮点数往往是常见的简单小数,而不需要处理极端情况.相比之下,`strtod()`在处理任何浮点数时都必须走标准的复杂路径,导致性能开销较大)
# JSON
1. `JSON`是一种轻量级的数据交换格式,常用于客户端和服务器之间的数据传输:
   * 易于阅读和编写
   * 广泛支持:`JSON`与语言无关
   * 轻量级:`JSON`比`XML`更简洁,传输数据时占用的带宽更少
   * 易于解析
2. 为什么要使用`JSON`(`XML`)?
   * `JSON XML`都是纯文本格式,这意味着它们可以在不同的平台、编程语言和系统之间轻松传输和解析.这使得它们成为网络通信、`Web`服务、移动应用等领域的标准选择
   * `JSON XML`都是国际标准或广泛接受的标准格式,确保数据可以在多种应用程序、系统和服务之间无缝交换
   * `JSON XML`都十分易于阅读和理解,适合开发者直接查看和边集
   * 无论`JSON XML`,都有大量的库、工具和框架支持它们的解析和生成
# XML
1. `XML`:指的是可扩展性标记语言(`XML`格式与`HTML`类似)(`HTML`就是一种标记语言),`XML`的关注焦点是数据的内容(但它也可展示在浏览器中,但通常没有颜色、可视化处理等等,它只关注传输的内容);而`HTML`的关注焦点是数据的展示(通过浏览器打开`HTML`就能看出)  `HTML`旨在显示信息,而`XML`旨在传输信息
2. `XML`标签只能自定义(`HTML`标签不能自定义):
   ```xml
   <root>
   <user>
   LJJ
   </user>
   <msg>
   111
   </msg>
   </root>
   ```
3. `XML`数据格式最主要的功能就是数据传输,还常用于配置文件、存储数据(充当小型数据库).`XML`规范了数据格式,使数据具有结构性,易读易处理
4. `XML`与`JSON`:
   * `XML`由于大量的标签和属性,其文档往往很冗长;`JSON`更简洁,可读性更好
   * `XML`是基于标签的格式,`JSON`是基于键值对的格式
   * `XML`适合文档型数据,广泛用于配置文件、文档交换等,它可以有注释等;`JSON`更适合数据传输,尤其是在`Web`应用中作为服务器和客户端之间的数据交换格式
# DOM
1. 文档对象模型(`DOM`)是一个网络文档(`HTML XML`)的编程接口(本项目也支持`DOM SAX`的`API`,这只是一种类似说法,本项目的`DOM API`通过自定义的一个`Document`对象,提供了与`XML`的`DOM`类似的功能,使得可以方便地解析、访问、修改和重新生成`JSON`文档).如:`HTML DOM`定义了访问和操作`HTML`文档的标准方法(`XML`类似).`DOM`以树结构表达文档,树的每个结点表示了文档的标签或标签内的文本项,`DOM`树结构精确地描述了文档中标签间的相互关联性. <mark>文档->`DOM`的过程称为解析</mark>,`DOM`模型不仅描述了文档的结构,还定义了结点对象的行为,利用对象的方法和属性,可以方便地访问、修改、添加和删除`DOM`树的结点和对应内容(说白了,`DOM`就是为了更好的描述文档,并且可以更方便、统一方法来进行操作文档内容)
2. `DOM`的特点:
   * 树状结构:`DOM`将文档表示为一棵树,其中每个节点代表文档的一部分.如,在`HTML`中,节点可以是元素、属性、文本内容等.对于`JSON`,节点可以是对象、数组、字符串、数字、布尔值和`null`等
   * 随机访问:通过`DOM`模型,程序可以随机访问文档中的任何部分.你可以通过路径或键名直接访问特点节点,而不需要从头到尾顺序读取整个文档
   * 可修改性:`DOM`允许不仅读取文档,还可以修改其结构和内容,可以添加、删除、或更改节点,然后将修改后的文档重新序列化为字符串
   * `DOM`是将整个文档加载到内存中(将整个`JSON`文档解析到内存中),因此对于大型文档可能会占用较多的内存
# SAX
1. `SAX`是一种基于事件的解析技术,最早用于解析`XML`文档.它是一种流式解析方法,与`DOM`不同,`SAX`不会将整个文档加载到内存中,而是逐行读取文档,并在遇到特定标记时触发事件
2. `SAX`解析器逐行扫描`XML`文档,每当遇到特定的结构(如开始标签、结束标签、字符数据等)时,就会触发相应的事件(如`startElement`、`endElement`、`characters`等).开发者需要实现这些事件的处理器(`Handler`),以便在事件发生时执行特定的逻辑
3. 由于`SAX`解析器不需要将整个文档加载到内存中,因此它的内存占用非常小,特别适合处理大型文档或内存有限的场景
4. `SAX`是顺序处理的,它无法回溯或随机访问文档的某个部分
# Allocator
1. <mark>`Allocator`中的`CrtAllocator``MemoryPoolAllocator`其实就是对标准库的`std::allocator`默认内存分配器的重新实现,使用了高效的内存池策略等</mark>
2. `CrtAllocator`为什么要用`C`来定义内存块分配函数:
   * 兼容性:C语言的标准库函数`malloc`、`realloc`和`free`是跨平台、广泛使用的内存管理工具.通过使用这些函数,CrtAllocator可以在几乎所有支持`C`的环境中运行,不依赖于`C++`特定的内存管理机制.这确保了`RapidJSON`在不同平台上的兼容性
   * 性能与开销:`C`语言的内存管理函数执行开销较小,并且可以直接分配或释放任意大小的内存块.与`C++`的`new`和`delete`相比,`malloc/free`的内存分配机制通常更加直接且底层,适合处理大量、快速的内存分配需求,如`RapidJSON`中频繁的内存操作
3. 内存池分配器`MemoryPoolAllocator`是如何实现高效的内存块分配的:
   * 预先分配大块内存=>减少系统(`malloc free`)调用
   * 内存池中每个内存块的链表管理
   * 按需分配新内存块=>当内存池中的当前内存块不足以满足某个内存请求时,分配器会按需分配一个新的内存块,而不是立即分配大量的内存.这种策略保证了内存的使用效率,并避免了内存浪费
   * 内存对齐操作
   * 避免内存碎片
   * `Free`操作的简化
   * `Clear`操作
4. 内存池分配器`MemoryPoolAllocator`通过统一管理内存块,减少了内存碎片的产生.内存池一次性分配较大的内存块,内部的小对象分配都来自同一块连续的内存区域
5. 内存池分配器`MemoryPoolAllocator`的`Free`实际是一个空操作,即内存块的回收并不是立即执行的.相反,内存块在整个内存池分配器被销毁时才会统一释放,这样避免了频繁的内存释放操作,进一步提升性能 当前内存池被消耗=>内存池中定义的内存块自然也消耗
6. 在某些情况下,用户可能需要手动释放内存池中的内存块.而`Clear()`函数会释放内存池中除了第一个块之外的所有内存块,确保内存池在这之后仍然可以被使用
7. 对齐内存边界重要性:对齐操作在计算机系统中非常重要,特别是在处理器访问内存时.处理器从内存读取数据时,通常是按块进行的,最常见的是按4字节(32位)或8字节(64位)读取.对齐的内存访问可以使得处理器只需要一次内存访问即可读取完整的数据,而不对齐的访问可能会导致处理器需要进行两次内存读取.如果数据没有对齐到合适的边界,处理器需要进行两次或更多次内存读取,将结果合并后才能得到所需的数据,这会增加延迟并降低性能
8. 本项目的对齐内存操作:
   ```C++
   1. 计算掩码
   const uintptr_t mask = sizeof(void*) - 1;// 若sizeof(void*)为8,则mask为7(0b111)
   2. 判断是否需要对齐
   if (ubuf & mask)// 若结果为0,则表示地址已经与32(64)位系统边界对齐了;否则,就需要对齐
   3. 计算对齐后的地址
   const uintptr_t abuf = (ubuf + mask) & ~mask;// 将地址向上(地址增大)对齐到最近的指针大小的倍数
   4. 调整内存块的剩余内存大小
   size -= abuf-ubuf;// 因为我们是向上对齐地址,因此对其操作会减少原始的剩余内存大小  则更新剩余的可用内存大小=原始剩余的可用大小-因为对齐而损失的内存大小
   ```
9. `MemoryPoolAllocator`预先分配大块内存:内存池分配器的核心思想是预先分配较大的内存块,然后在这些内存块中按需分配小对象.通过这种方式,内存池避免了频繁的动态内存分配调用,如`malloc`或`new`,从而显著提升了内存分配的效率.(当程序开始使用分配器时,内存池分配器会申请一个大块内存(如64KB).每次请求内存时,如果内存块内存大小足够,则内存池分配器会直接从预分配的内存块中分配;否则,会在`MemoryPoolAllocator`中重新分配一个内存块)
   ![](markdown图像集/2024-10-07-09-25-27.png)
10. 内存块(`ChunkHeader`表示的就是一块内存块)的链表结构:<mark>本项目的内存池分配器使用链表来管理每个内存块,`ChunkHeader *chunkHead`指向链表头部,链表中的每个节点(`ChunkHeader`)表示一个内存块.通过这个指针,分配器可以轻松地遍历所有的内存块,以便分配和释放内存.其中每个内存块都有一个头部`ChunkHeader`结构体来记录其容量和已分配的内存大小以及指向下一个内存块的指针(这个指针就实现了链表管理)</mark>
11. `ownBuffer`:
    * `ownBuffer=true`:表示内存池分配器自己管理该内存块.也就是说,内存块的分配和释放都由 `MemoryPoolAllocator`自己负责.此时,当分配器销毁时,它会负责释放这些内存块
    * `ownBuffer=false`:表示内存块是由用户提供的，并且`MemoryPoolAllocator`不会负责释放这些内存.这种情况适用于用户已经有一块现成的内存(如堆或栈上的内存),并希望将其提供给分配器使用.在这种情况下,当分配器销毁时,它不会试图释放由用户提供的内存块  此时当`MemoryPoolAllocator`析构时,它不会释放由用户提供的缓冲区,因为它不拥有这些内存
12. `SharedData`:
    ```C++
     struct SharedData {
        ChunkHeader *chunkHead;
        BaseAllocator* ownBaseAllocator; 
        size_t refcount;
        bool ownBuffer;
     };
     // size_t refcount:这个计数器用于跟踪有多少个MemoryPoolAllocator实例共享同一个SharedData对象.当一个分配器被复制(如,通过拷贝构造函数或赋值操作符)时,引用计数会增加;当一个分配器被销毁时,引用计数会减少.只有当引用计数减少到零时，才会真正释放SharedData所管理的内存,即内存池
     // bool ownBuffer:该字段指示内存块是否由 MemoryPoolAllocator自己管理.通过这个字段,分配器可以判断是否需要在析构时释放内存,避免错误地释放由用户提供的内存
     ```
13. 本项目中,每个内存池`MemoryPoolAllocator`实例通常会拥有自己独立的`SharedData`结构体,即<mark>一个内存池与一个`SharedData`一一对应</mark>
14. 本项目中<mark>一个内存块和一个`ChunkHeader`结构体是一一对应的</mark>
15. 一个内存池可以管理多个内存块,即一个`SharedData`结构体管理多个`ChunkHeader`结构体
16. <mark>内存池通过`SharedData`结构体,内存块通过`ChunkHeader`结构体设计来管理其元数据,而具体的`data`紧跟在头部之后,形成一个连续的线性空间.这种结构可以高效地进行内存分配和管理,同时支持多个内存块的链式组织,形成完整的内存池</mark>
17. `void *`是一种通用指针类型,可以指向任何类型的数据.在`C/C++`中,`void *`被广泛用于函数参数和返回值,这样可以允许代码处理各种数据类型而不需要显式指定类型.如:在内存块分配器`CrtAllocator`中,`void*`可以用于分配和返回任意类型的内存块，使得分配器更灵活
18. `static inline ChunkHeader *GetChunkHead(SharedData *shared)`:从内存池结构体`SharedData`中获取第一个内存块头`chunkHead`的起始地址,假设当前内存池`SharedData`的对齐大小为32字节,那么`shared`的地址从0开始,+32字节的偏移后得到 `ChunkHeader`的起始地址
19. `static inline uint8_t *GetChunkBuffer(SharedData *shared)`:返回当前内存块中`data`区域的起始地址,即跳过当前内存块的`chunkHead`.假设`ChunkHeader`的对齐大小为16字节,而`shared->chunkHead`的地址为0x1020,那么加上16字节后,数据区起始地址就是0x1030
    ![](markdown图像集/2024-10-06-23-12-39.png)
20. 只有最后一个分配的内存块(或者说是内存块的最后一个分配的内存对象)才能安全地扩展(这是因为它紧邻着当前内存块的未分配空间),这里的最后一个指的是当前内存块链表中最新分配的那块内存,它在当前内存块的已分配部分的末尾.假设某个内存块不是最后一个分配的块,而是中间的某个块,试图扩展它会破坏后续分配的内存块结构,导致内存重叠或越界
21. `if (originalPtr == GetChunkBuffer(shared_) + shared_->chunkHead->size - originalSize)`:如果相等,则表示需要重新分配的内存块的起始地址就是当前内存池分配的最新的(最后一个)内存块起始地址`originalPtr`是传入的需要重新分配或扩展的内存块的起始地址  `GetChunkBuffer(shared_)`返回当前内存块(`shared_->chunkHead`)的起始地址,这是当前内存池中最新分配的内存块的起始地址  `shared_->chunkHead->size`是当前内存块已经分配出去的内存大小  `originalSize`是要重新分配的内存块的原始大小.如:
    ![](markdown图像集/2024-10-07-09-21-48.png)
    ![](markdown图像集/2024-10-07-09-22-07.png)
    ![](markdown图像集/2024-10-07-09-22-17.png)
22. <mark>内存池中的内存块的分配是线性的</mark>
23. 不能说成在一个大内存块继续分配小内存块,因为内存块之间是独立的,都有一个自己的头部结构体`ChunkHeader`;只能说在一个大内存块上分配小的内存对象
24. 在`CrtAllocator`和`MemoryPoolAllocator`中分别定义`Malloc`和`Realloc`函数,是因为它们具有不同的内存分配策略和用途.虽然函数名相同,但这两个分配器的实现细节和设计目标是不同的,它们分别用于标准堆分配和优化的内存池分配,不同的实现方式为不同的应用场景提供了灵活的选择
    * `CrtAlocator::Malloc() CrtAllocator::Realloc()`:简单地调用C标准库的`malloc realloc`,提供了一种直接使用标准库内存管理机制的旋转.它的优点是简单直接,依赖于系统提供的内存管理方式,适合那些不需要频繁分配/释放内存的小型应用程序
    * `MemoryPoolAllocator::Malloc() MemoryPoolAllocator::Realloc()`:它的主要优势是性能优化,特别是在需要频繁分配和释放小内存时.这种方式避免了频繁调用系统分配器的高开销,提高了效率
25. <mark>为什么要定义`StdAllocator`类:`StdAllocator`继承自标准库的`std::allocator<T>`?</mark>
    其主要设计目的是让`RapidJSON`的分配器能与标准库容器(如`std::vector` `std::map`等)一起使用.标准库的许多容器支持自定义分配器,而默认情况下使用的是`std::allocator`.`CrtAlocator MemoryPoolAllocator`虽然提供了内存分配功能,但它们并不遵循标准库分配器的接口规范(即`allocate deallocate construct destroy`等接口),而`StdAllocator`提供了一个符合标准接口的分配器
26. `StdAllocator`自定义的标准内存分配器可以直接使用`std::allocator CrtAllocator`作为基础分配器(`template <typename T, typename BaseAllocator = std::allocator<T>>``template <typename T, typename BaseAllocator = CrtAllocator>`),`std::allocator`是标准库容器默认的内存分配器(标准库的许多容器支持自定义分配器,而默认情况下使用的是`std::allocator`.`StdAllocator`是为了解决在使用这些容器时能够无缝地与`RapidJSON`的分配器系统结合起来.虽然内存块和内存池分配器可以高效地分配内存,但它们并不直接与`STL`容器兼容)  
27. <span style="color:red;">需要注意的是:`StdAllocator`能以`MemoryPoolAllocator`为底层内存分配器,但是前提是必须先给`MemoryPoolAllocator`实例化一个底层分配器后才能将`MemoryPoolAllocator`作为`StdAllocator`的内存分配器:</span>
   ```C++
   1. 不能直接将MemoryPoolAllocator作为底层内存分配器  依赖关系的循环问题
   StdAllocator<int, MemoryPoolAllocator<CrtAllocator>> allocator;
   // 定义一个使用 StdAllocator 的 vector
   std::vector<int, StdAllocator<int, MemoryPoolAllocator<CrtAllocator>>> vec(allocator);
   // 报错:error: type/value mismatch at argument 2 in template parameter list for ‘template<class T, class BaseAllocator> class rapidjson::StdAllocator’8 |     StdAllocator<int, MemoryPoolAllocator> allocator;     =>编译器期望StdAllocator的第二个模板参数是一个可以直接使用的分配器,比如 CrtAllocator.但当你传递MemoryPoolAllocator时,编译器遇到了类型和值的不匹配,因为MemoryPoolAllocator需要先被实例化(带有底层的BaseAllocator参数)才能作为其他分配器的底层分配器
   // 为什么不能直接将MemoryPoolAllocator作为底层内存分配器?
   因为MemoryPoolAllocator并不是一个独立的分配器,它需要一个底层的BaseAllocator来处理内存分配(比如CrtAllocator或其他简单分配器).这意味着如果你尝试直接将 MemoryPoolAllocator作为StdAllocator的底层分配器,它就会引发依赖关系的循环问题
   2. 提前确保为MemoryPoolAllocator提供了一个适当的底层分配器,而不是直接使用默认值
   MemoryPoolAllocator<CrtAllocator> poolAllocator;
   StdAllocator<int, MemoryPoolAllocator<CrtAllocator>> allocator;
   // 定义一个使用 StdAllocator 的 vector
   std::vector<int, StdAllocator<int, MemoryPoolAllocator<CrtAllocator>>> vec(allocator);
   ```
28.  `StdAllocator`通过使用`rebind()`用于将当前内存分配器直接绑定到不同类型的容器上,而不需要每一种类型的容器都去定义一个内存分配器,如:
   ```C++
   #include <iostream>
   #include <memory> // for std::allocator
   #include "rapidjson/allocator.h" // 假设已包含 RapidJSON 的 allocator 头文件
   int main() {
      // 创建一个 StdAllocator<int> 分配器
      StdAllocator<int> intAllocator;
      // 通过 rebind 将分配器绑定到 double 类型
      StdAllocator<double>::rebind<int>::other reboundAllocator = intAllocator;
      // 使用 reboundAllocator 分配内存
      int* p = reboundAllocator.allocate(5); // 为 5 个 int 分配内存
      // 为分配的内存赋值并输出
      for (int i = 0; i < 5; ++i) {
         p[i] = i * 2;
         std::cout << p[i] << " ";  // 输出：0 2 4 6 8
      }
      // 释放分配的内存
      reboundAllocator.deallocate(p, 5);
      return 0;
   }
   ```
29.  <mark>`std::allocator`:它是`C++`标准库中提供的一个默认的内存分配器,主要用于管理动态内存分配.它为标准容器提供了内存分配和释放的机制,它是一个通用的分配器类,可以为任何类型分配内存.通常,`C++`容器类(如`std::vector`,`std::list`等)都使用分配器来管理底层内存.`std::allocator`不是只能用于容器的内存分配,它是`C++`标准库中的一种通用内存分配器,用作处理动态内存分配和对象的构造与销毁,可以想作类似`new delete<=>allocate deallocate`来动态分配内存.</mark>其模板为:
   ```C++
   template <typename T>
   class allocator {
   public:
      using value_type = T;
      allocator() noexcept;
      template <typename U> allocator(const allocator<U>&) noexcept;
      T* allocate(std::size_t n);// 用于分配未初始化的内存块
      void deallocate(T* p, std::size_t n);// 用于释放之前通过 allocate 分配的内存
      template <typename U, typename... Args>
      void construct(U* p, Args&&... args);// 用于在分配的内存上构造对象(调用构造函数)
      template <typename U>
      void destroy(U* p);// 用于在内存中销毁对象(调用析构函数)
   };
   ```
   ```C++
   1. std::allocator不是只能用于容器的内存分配
   #include <iostream>
   #include <memory> // for std::allocator
   int main() {
      std::allocator<int> alloc; // 创建一个 std::allocator<int> 分配器
      // 分配内存
      int* p = alloc.allocate(5); // 分配内存，足够容纳 5 个 int  分配的是一个int数组  而非容器
      // 构造对象
      for (int i = 0; i < 5; ++i) {
         alloc.construct(&p[i], i * 2); // 在内存上构造对象
      }
      // 打印构造的对象
      for (int i = 0; i < 5; ++i) {
         std::cout << p[i] << " ";  // 输出：0 2 4 6 8
      }
      std::cout << std::endl;
      // 销毁对象
      for (int i = 0; i < 5; ++i) {
         alloc.destroy(&p[i]); // 调用析构函数，销毁对象
      }
      // 释放内存
      alloc.deallocate(p, 5); // 释放分配的内存
      return 0;
   }
   2.
   int main() {
    // 使用 std::allocator 自定义分配器
    std::vector<int, std::allocator<int>> vec;// 其实不用写内存分配器,因为容器里默认就是使用std::allocator
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    for (auto i : vec) {
        std::cout << i << " ";  // 输出：1 2 3
    }
    std::cout << std::endl;
    return 0;
   }
   ```
30.  <mark>`std::allocator_traits`:这是`C++11`引入的一个模板工具类,用于提取分配器类型的相关特性.它为不同的分配器提供了一个统一的接口,封装了各种复杂的底层内存分配器的操作,即它是底层内存分配器的上层抽象层.`std::allocator_traits`不仅支持 `std::allocator`,还支持用户定义的分配器.无论你使用什么样的分配器,`std::allocator_traits`都能统一调用这些分配器的成员函数,而不需要为每个分配器单独实现.假设你实现了一个自定义分配器,`allocator_traits`可以无缝地处理它(只需要将自定义内存分配器传入这个工具类即可),而不需要更改容器或分配相关代码.需要注意的是:`std::allocator_traits`只是用于操作和抽象分配器的接口,而不是实际的分配器实现,也不负责具体的内存分配(底层的内存分配器本身才负责分配和释放内存的操作).即如:调用`std::allocator_traits`的`construct()`函数时,它会检查传入的底层分配器是否有相应的成员函数,如果有,就会去调用内存分配器的相应函数;如果没有就会编译报错.因此`std::allocator_traits`只是提供一个抽象统一的接口,不做具体的处理:</mark>
    ![](markdown图像集/2024-10-13-19-08-57.png)
31.  `StdAllocator`中利用了`std::allocator_traits<std::allocator<T>>`这个抽象的统一接口层来定义和标准库兼容的内存分配器.其中给这个工具类`std::allocator_traits`传入的内存分配器是`std::allocator<T>`(`typedef std::allocator_traits<std::allocator<T>> traits_type`),即在`StdAllocator`中定义`construct() destroy() max_size()`函数其实是调用的`std::allocator<T>`这个内存分配器的相应函数
32. `StdAllocator::allocate()`和`StdAllocator::deallocate()`函数没有直接使用`std::allocator_traits`工具类的接口,而是调用的基础内存分配器的`Malloc()`和`Free()`函数来实现
33. 为什么要定义`class StdAllocator<void, BaseAllocator> : public std::allocator<void>`?
    在`C++11`及更早的标准中,`std::allocator<void>`是合法的(那么就可能有很多外部调用`RapidJSON`的项目就支持了`std::allocator<void>`),并用于支持泛型编程中的某些特性(尽管`void`不能直接分配内存).为了保持与这些旧标准的兼容性,`RapidJSON`提供了一个特化的`StdAllocator<void, BaseAllocator>`.然而,在`C++17`中,`std::allocator<void>`被弃用和移除,因为`void`作为分配器类型没有实际意义.因此,使用条件编译来确保只有在不支持`C++17`时,才定义`StdAllocator<void, BaseAllocator>`,以避免在`C++17`环境中产生编译错误
34. <mark>为什么`class StdAllocator<void, BaseAllocator> : public std::allocator<void>`中用于`void`类型也要定义`rebind()`?</mark>
   `STL`容器和算法期望所有分配器都实现`rebind`,以便可以灵活地为不同类型分配内存.这是`C++`分配器接口的一部分,即使是特化为`void`类型的分配器,也必须遵循这一接口要求.在很多实际场景中,虽然分配器可能一开始被定义为`void`类型,但在运行时,我们需要为具体类型的对象(如`int`或`double`)分配内存.通过`rebind`,`StdAllocator<void, BaseAllocator>`可以动态生成适用于其他类型的分配器
35. `struct IsRefCounted<T, typename internal::EnableIfCond<T::kRefCounted>::Type> : public TrueType;internal::EnableIfCond<T::kRefCounted>::Type`:这段代码是结构体模板的特化版本,它在某些条件满足时被使用.这里判断条件满足时用了`SFINAE`机制,即:当模板的某个替换导致语法错误时,编译器不会报错,而是会尝试其它的模板匹配(如:通用模板,而非特化模板了).因此,`IsRefCounted<T>`会在类型`T`拥有`kRefCounted`成员且为`true`时进行替换,如果`T`没有`kRefCounted`成员或不为`true`,则这个特化模板会被忽略,编译器会选择默认的通用`IsRefCounted`实现,即`struct IsRefCounted : public FalseType`  `::kRefCounted`类似`std::is_integral`中的`::value`
36. `template <bool Condition, typename T = void> struct EnableIfCond  { typedef T Type; };template <typename T> struct EnableIfCond<false, T> {};`:虽然对于`template <typename T> struct EnableIfCond<false, T> {};`这个特化模板在调用时不用显示指定参数`T`,因为`C++`的模板参数可以在调用时根据上下文推导.如果某个模板参数有默认值,而其它参数能够被推导出,编译器就可以在不需要显示指定所有模板参数的情况下进行推导,上面这个就是在前面`template <bool Condition, typename T = void>`已经设了默认值了:
   ```C++
   template <bool Condition, typename T = void>
   struct EnableIfCond {
      typedef T Type;
   };
   // 当Condition为false时的特化
   template <typename T>
   struct EnableIfCond<false, T> {};// 这个false就代表了通用模板中的Condition的具体特化,即具体到false这个值上了
   // 调用
   EnableIfCond<false> myCondition; // 这里省略了类型T
   // 调用时不用显示指定模板参数T,编译器会使用EnableIfCond中typename T = void的默认值，实际上就变成了 EnableIfCond<false, void>
   ```
37. `using propagate_on_container_move_assignment = std::true_type;using propagate_on_container_swap = std::true_type;`:这是定义在`C++11`标准库中内存分配器工具类(`std::allocator_traits`)中,`propagate_on_container_move_assignment`用于指示当容器A移动赋值到容器B时,是否允许相应的容器A的内存分配器传播到容器B的内存分配器中去(`std::true_type`表示要将容器A的分配器传到容器B中去);同理,`propagsate_on_container_swap`表示的就是当两个容器进行交换操作时,是否允许容器之间的分配器也进行交换(容器交换:指的是交换两个容器的内容,`std::swap()`将使得容器的元素和内部状态(如大小、容量等)相互调换)
# 内存流 memorybuffer.h/memorystream.h
1. `memorybuffer.h`中`GenericMemoryBuffer`的作用是提供内存中的输出字节流管理,而我们利用栈这个数据结构来管理内存缓冲区(内存缓冲区可以基于不同的数据结构实现,比如数组、队列或栈.其目的是存储数据,稍后进行读取、处理或传输,`memorybuffer.h`利用栈这种数据结构来实现的当前内存缓冲区)
2. `memorybuffer.h`其实是一个输出字节流,即写入流.<span style="color:red;">需要注意的是:创建一个`MemoryBuffer`对象并不会立即分配`kDefaultCapacity`大小的内存缓冲区,因为它的构造函数里面只有`stack_(allocator, capacity)`的初始化,而对于`Stack`对象,它的构造函数` Stack(Allocator* allocator, size_t stackCapacity) : allocator_(allocator), ownAllocator_(0), stack_(0), stackTop_(0), stackEnd_(0), initialCapacity_(stackCapacity){}`也是没有具体的内存分配操作的(即没有`malloc()`),具体的内存的分配是在`Push()`操作时发生的,而不是在构造`GenericMemoryBuffer`时发生</span>
3. 栈与内存缓冲区的区别:栈可以被用来管理内存缓冲区,但它并不等同于内存缓冲区.内存缓冲区更侧重于存储数据的具体内存区域,而栈是一种管理这些数据的方式.你可以通过栈来有序地放入和取出数据,但栈的本质是一种数据管理的策略,而非具体的内存块,栈可以理解为实现内存缓冲区的一种方式
4. `memorybuffer.h`中使用`template`关键字来消除普通成员函数给模板成员函数的调用可能带来的歧义
5. 其实`memorybuffer.h`创建的内存缓冲区是由对应的`stack_`管理分配创建的,而`stack_`栈本质的内存分配操作又是依赖`allocator`内存分配器的`malloc`操作实现的
6. `memorystream.h`定义一个内存字节流,用于处理内存中的输入字节流.与文件流(`filereadstream.h`)不同,它操作的是内存缓冲区`memorybuffer`.`MemoryStream.h`用于处理输入字节流,即提供了一系列的接口,如:`Peek()、Take()、Tell()、Peek4()`.而`Put()、Flush()、PutEnd()`是给流提供写入功能的,`MemoryStream.h`是不支持的
7. `MemoryBuffer`用于管理内存中的输出字节流,即写入流.它能够在内存中分配和管理(`stack_`来管理)数据,支持将字节数据写入到内存缓冲区中
8. `MemoryStream`是一个只读流,不支持写操作(`MemoryStream`适用于处理那些已经在内存中存在的数据,比如在某些场景下我们需要从内存中读取二进制数据或编码检测)
# 文件流 filereadstream.h/filewritestream.h
1. `filereadstream.h`:用于从文件中读取数据的输入流;`filewritestream.h`:用于向文件中写入数据的输出流  它们的操作都是基于文件句柄(`FILE*`),底层通过调用`C`标准库函数`fread() fwrite()`实现数据流操作
2. <mark>`filereadstream.h`通过将文件分批读取到缓冲区,再从缓冲区中提供数据,这样可以提高读取效率,避免频繁的文件访问.实际上这个缓冲区`buffer_`是被重复使用的:首先,从文件中读取数据到缓冲区(此时不一定读完了文件,可能只读了`bufferSize_`个字节的文件数据,也有可能读完了);然后,缓冲区中的数据等待被其他地方读取,这样设计可以大大减少磁盘I/O的操作,而内存读取操作会很快.如果缓冲区中的数据没有被读取完,这个缓冲区是不会重新从文件中读取数据的,即`if(current_ < bufferLast_) ++current_;`;只有当缓冲区的数据被完全读取完时,才会触发新的文件读取操作`else if(!eof_){count_ += readCount_;readCount_ = std::fread(buffer_, 1, bufferSize_, fp_);...}`(新的文件读取操作还是将文件读取到刚刚那个缓冲区)</mark>
3. `FileReadStream::Take() FileReadStream::Peek() FileReadStream::Tell()`:这些函数不是文件读取的相关操作,而是从缓冲区中读取的相关操作;`FileReadStream::Read()`:这是从文件读取到缓冲区的函数(利用`std::fread`)
4. `filereadstream.h`中不要把对文件的读取和缓冲区的读取弄混了,如:
   ```C++
   char* current_;// 当前缓冲区的读取位置的指针
   size_t readCount_;// 本次读取到缓冲区的字符数量
   size_t count_;// 已经从缓冲区中读取走的字节总数
   ```
5. <mark>为什么`FileReadStream::Tell()`不直接返回`count_`,而是`return count_ + static_cast<size_t>(current_-buffer_);`?</mark>
   因为`count_`虽然表示的是已经从缓冲区中读取走的字节总数,但是`count_`不会统计当前缓冲区中已经被读取的部分数据(此时当前缓冲区可能没有被读取完),`count_`只有在当前缓冲区整体被读取完才会去更新,因此要加上还没统计当前缓冲区中被读取的字节数`current_-buffer_`
6. `FileWriteStream::Put() FileWriteStream::PutN()`:这是将字符数据写入到缓冲区;而`FileWriteStream::Flush()`才会将缓冲区数据写入到文件中
7. <mark>在重复写n个字符的时候,即`PutN()`时,使用了`std::memset()`:底层由C实现,使用了高度优化的汇编代码来操作内存,并利用了硬件特性,在批量数据填充中,效率很高.注意:`std::memset()`不是`gcc`内置函数,这和`memcpy()`不同</mark>
# 通用输入输出流 istreamwrapper.h/ostreamwrapper.h
1. `istreamwrapper.h`实际上就是一个通用的输入流包装器,它可以处理任何符合`std::basic_istream`接口的输入流,即它可以把任何继承自`std::istream`的类(如`std::istringstream、std::stringstream、std::ifstream、std::fstream`)包装成`RapidJSON`的输入流  `istreamwrapper.h`的`BasicIStreamWrapper`实现了一个封装,不是像`stream.h`中的`GenericStreamWrapper`这种的完全抽象接口封装,`BasicIStreamWrapper`通常作为输入流处理的封装而直接使用,而不会用来继承
2. <mark>`istreamwrapper.h`传入`std::ifstream`就可以实现文件输入流的包装,虽然这实现了类似`filereadstream.h`的功能(因此,可以从程序看出`istreamwrapper.h`和`filereadstream.h`的代码很相似),但是`istreamwrapper.h`的性能会差一些,它没有针对专用流(如文件流)进行特别优化,如`istreamwrapper.h`中读取流数据使用的是流接口`stream_.read()`,而不是像`filereadstream.h`中的`std::fread()`这种`C`标准库函数;前者的读取性能往往更差</mark>
3. `istreamwrapper.h`默认构造函数(即不传入用户缓冲区)使用的是`Ch`类型较小的`Ch peekBuffer_[4]`缓冲区(`Ch`是根据`StreamType`的字符类型定义的,比如`StreamType`是 `std::istream`时,`Ch`就是`char`,而`StreamType`是`std::wistream`时,`Ch`就是`wchar_t`.这使得`BasicIStreamWrapper`可以支持不同字符类型的输入流).如果用户没有提供自己的缓冲区,那么`BasicIStreamWrapper`就会使用内部定义的`Ch peekBuffer_[4]`作为默认的缓冲区.这个`peekBuffer_`非常小,只有4个`Ch`类型的字符大小,因此在流处理时,效率较低,适合小规模的流操作或简单的`Peek`操作.而用户自定义的缓冲区是一个`char`类型的`char* buffer`,即会将流中的数据从`Ch`类型转换到`char*`的缓冲区中,不管流的字符类型是`char`还是`wchar_t`,或者是其它
4. <span style="color:red;">本项目中流中所使用的缓冲区和标准输入输出流中对应的输入缓冲区、输出缓冲区是不一样的.对于一个传入的流对象`StreamType& stream`,它本身是有自己的内部缓冲区的(输入流对象就有输入缓冲区、输出流就有输出缓冲区),这个缓冲区通常由`C++`标准库自动管理,这个缓冲区是用于优化系统调用与实际输入设备(如文件、键盘、屏幕等)之间的数据传输(有一个缓冲区作为中介,可以匹配磁盘等的读取速率慢而内存读取速度快的匹配问题).`istreamwrapper.h`中自定义的缓冲区`buffer_`不是传入流对象的对应输入缓冲区,而是一个后一层的缓冲区,它允许用户自己管理缓冲区大小和内存位置,是一个应用级的,不是`C++`标准库自动管理的,可以理解为:`istreamwrapper.h`其实是传入的流对象的输入缓冲区向自定义缓冲区`buffer_`输入数据的过程</span>
   ![](markdown图像集/image-4.png)
   ![](markdown图像集/2024-10-18-13-44-00.png)
5. `istreamwrapper.h`输入流:从键盘(`std::iostream`)、文件(`std::ifstream`)等输入到输入缓冲区`buffer_`;`ostreamwrapper.h`输出流:从传入的输出流`stream_`的输出缓冲区输出到目标地方(如文件、屏幕等)
6. <mark>`ostreamwrapper.h`只是一个输出流的包装器,它本身是没有定义一个类似`istreamwrapper.h`中应用级的缓冲区`buffer_`,因为它是直接将流对象的输出缓冲区数据输出到屏幕、文件等地方的,而不是:自定义缓冲区->流对象输出缓冲区->屏幕、文件,因此`ostreamwrapper.h`只是起到了一个输出流对象的上层封装的作用</mark>
![](markdown图像集/2024-10-18-13-44-13.png)
# stringbuffer.h
1. `typedef typename Encoding::Ch Ch`:`Encoding`是一个模板参数,`Encoding::Ch`是一个依赖于`Encoding`的嵌套成员,编译器在处理模板时不能提前知道`Ch`是什么——它可能是一个类型,也可能是一个值.因此,如果不使用`typename`,编译器会认为`Encoding::Ch`是一个变量或静态成员,而不是一个类型.所以,这里的`typename`是为了消除歧义
2. 存储类型`char*`和编码格式的区别:`char*`更像是字符数据的容器,而其编码格式是将字符转换为二进制字节的规则.`char*`是字节的存储方式,它只是存储字节数据,不管这些字节是用哪种编码方式表示的字符.编码包括字节的排列规则,而`char*`只是存储这些字节的容器,`char*`本身不包含关于编码的信息.<mark>字符编码格式可以说是`char*`对应的内存缓冲区中存储的底层编码规则.所以说同样都是`char*`内存缓冲区,其底层编码格式可以不同</mark>
3. (单纯从本项目的`stringbuffer.h`和`memorybuffer.h`中出发考虑)<span style="color:red;">`stringbuffer.h`其实可以理解为就是增加了字符编码处理能力的`memorybuffer.h`,后者只能处理原始字节流`char*`,此时这个`char*`是可以是任意编码格式的字节数据的,`memorybuffer.h`是没有考虑编码格式的.而`stringbuffer.h`增加了字符编码的支持,通过模板参数`Encoding`来决定缓冲区中存储的字符类型(如`UTF-8`、`UTF-16`等),此时的`Encoding::Ch`指的可能是`UFT8::char`或`UTF16::char`等,这使得`StringBuffer`可以处理多种不同指定的字符编码,即可以指定编码进行存储,虽然从缓冲区存储上看可能都是`char*`,但是底层的编码格式可能是不同的</span>  `memorybuffer.h`的最初设计目的是为了处理原始二进制数据,如字节流,此时不涉及编码什么的,虽然在多数上层应用场景`stringbuffer.h`是可以代替`memorybuffer.h`进行使用的,但是为了保持类的独立性和每个类的用途,就保留了`memorybuffer.h`
4. <mark>为什么`GetString()`函数实现了以`'\0'`结尾的字符串输出,明明`push() pop()`不应该什么都不操作吗?</mark>
   因为`Push()`操作返回的是栈顶指针,此时把栈顶位置赋值`'\0'`,然后`+1`栈顶指针;而`Pop()`操作不会删除栈顶内容`'\0'`,而只是把栈顶指针往下移,因此实现了以`'\0'`结尾的字符串输出
5. `PutReserve()  PutUnsafe`是对`GenericStringBuffer::Reserve()  GenericStringBuffer::PutUnsafe()`更高层次API接口的封装
# cursorstreamwrapper.h
1. `cursorStreamWrapper`继承自`stream.h`中的`GenericStreamWrapper`,`GenericStreamWrapper`虽然有`Peek() Take() PutBegin() Put() Flush() PutEnd() Peek4() GetType() HasBOM()`接口函数,但是只要在代码中不会实际调用某些方法,那么在传入的时候这个输入流对象可以不用定义这些方法,因此`cursorStreamWrapper`可以接受`istreamwrapper.h`、`filereadstream.h`和`memorystream.h`这些作为参数`InputStream`传入(如接受`istreamwrapper.h`时,在代码中不调用`GetType() HasBOM()`时就可以将`istreamwrapper.h`作为`cursorStreamWrapper`的参数传入)
2. <mark>`istreamwrapper.h`禁用了拷贝构造,为什么`cursorStreamWrapper()`还可以以`BasicIStreamWrapper`为输入流对象传入呢?
   因为拷贝构造只有在按值传递时才会被调用,按引用传递时是直接传入对象的引用的操作,而不是拷贝构造.</mark>如:
   ```C++
   BasicIStreamWrapper isw;  // 创建一个 BasicIStreamWrapper 对象
   CursorStreamWrapper(isw);  // 错误：按值传递,触发拷贝构造  因为stream.h的GenericStreamWrapper的构造函数中出现了is_(is)
   BasicIStreamWrapper isw;  // 创建一个 BasicIStreamWrapper 对象
   BasicIStreamWrapper& sw = isw;  // sw 是 isw 的引用
   CursorStreamWrapper(sw);  // 正确：按引用传递，未触发拷贝构造
   ```
3. `cursorStreamWrapper.h`的主要作用是为`RapidJSON`提供一个增强的输入流封装器,允许在解析`JSON`数据时跟踪字符的位置(行`line_`和列`col_`),这样就可以在解析`JSON`数据时提供有关错误位置的详细信息,即方便错误定位.在解析`JSON`数据时,通过调用`cursorStreamWrapper::Take()`就能一直更新解析位置,当出错就能迅速定位
# encodings.h/encodedstream.h
1. `C++`标准库本身并没有专门定义针对`UTF8 UTF16`等其它编码格式的类.标准库主要提供了`std::string std::wstring`类型,用于处理字符串,但这些类型并不明确规定内部的编码格式.而因为不同`JSON`数据存储时可能有不同的编码格式,本项目为了支持不同编码格式,因此需要定义如`UTF8 UTF16`等编码格式的类
2. 在`C++`中,如果不明确指定编码格式,字符串字面量(如`char*`或`std::string`)通常以`UTF-8`编码存储,尤其是在现代应用中,因为`UTF-8`与`ASCII`向后兼容
3. <mark>不同的编码格式其实就是对于同样的数据在存储的时候按照二进制的编码位数的不同</mark>
4. `Unicode`:它是一个字符编码标志,旨在为全球所有的文字、符号和字符提供统一的编码方案,其主要特点:
   * 全球覆盖:`Unicode`支持几乎所有的书写系统,包括拉丁文、汉字、阿拉伯文、希腊文、日文、韩文等
   * 唯一的代码点:每个字符在`Unicode`中都有一个唯一的代码点`codepoint`,通常表示为`U+XXXX`的形式,其中`XXXX`是4个十六进制数.如,拉丁字母`A`的代码点是`U+0041`,汉字`汉`的代码点是`U+6C49`
   * 编码方式:`Unicode`有多种编码方式,包括`UTF8 UTF16 UTF32`
5. <mark>`Unicode`代码点`codepoint`:指的是`Unicode`字符集中每个字符的唯一数字标识符.它们用来表示全球所有文字和符号.代码点是一个非负整数,表示特定字符在`Unicode`标准中的位置.它通常以`U+`后接十六进制数字的形式表示</mark>
6. 单字节字符=`ASCII`字符
7. 字节顺序标记`BOM`:它是一种用于指示文本文件的字节顺序和编码格式的特殊字符,它主要用于`UTF`编码,帮助软件识别文件的编码方式.如:在`UTF16`和`UTF32`编码中,`BOM`可以指示字节序(大端或小端).例如,`UTF16`的`BOM`在大端顺序中表示为 `0xFEFF`,在小端顺序中则表示为`0xFFFE`.在`UTF8`编码中,`BOM`为`0xEF 0xBB 0xBF`,虽然`UTF8`中的`BOM`不能表示字节序,但是也可以作为它是`UTF8`编码的标志
8. <mark>`UTF8`没有字节序(`BOM`)问题,可以直接读入;而`UTF16 UTF32`有字节序问题,需要在读取时把字节转换为字符,以及在写入时把字符转换为字节(如小端字节序下,字符的低字节在前,高字节在后,则字符`U+0041`的小端字节表示为`41 00 00 00`,则写入时要转换为`41 00 00 00`写入)</mark>
9. <mark>在`C++`中,`char`通常用于表示`UTF8`编码的字符,而`wchar_t`则用于表示`UTF16`编码的字符.这并不是严格的规则,但在许多情况下,它们的使用是这样的</mark>
10. 如果考虑多种编码类型,那么对于不同的编码类型:输入流->检测并处理`BOM`(读取数据时,输入流会首先检测文件开头是否存在`BOM`;如果存在`BOM`,输入流会解析并跳过这个`BOM`,然后处理后续`data`区域)(处理`BOM`的好处是流可以自动适应不同的字节序格式,而不需要用户指定编码类型);输出流->在输出流中,编码的第一个步骤通常是根据目标编码写入对应的`BOM`,以便后续的读取操作能够识别文件的编码格式
11. 本项目只涉及`UTF8`编码,因此没有原`RapidJSON`项目中的`AutoUTFInputStream  AutoUTFOutputStream`,它们的主要功能是处理不同的`UTF`编码(`UTF8 UTF16 UTF32`等)和根据`BOM`自动检测编码类型(即由数据的`BOM`=>`type_`)
# stream.h
1. `stream.h`中指出:
   - 对于只读流:只需要实现 `Peek()`、`Take()` 和 `Tell()` 函数
   - 对于只写流:只需实现 `Put()` 和 `Flush()` 函数
   - 对于读写流:需要实现所有函数
2. <span style="color:red;">`stream.h`定义了一个通用的流接口以及一些具体的流实现.`GenericStringStream`:只读字符串流,用于从内存中的字符串读取数据,通常用于解析内存中的`JSON`文本(常用于`reader.h`的流对象);`GenericInsituStringStream`:就地读写字符串流,允许在解析时直接修改输入字符串.适合需要边解析边修改数据的场景,比如`JSON`文本的就地解析;`GenericStreamWrapper`:通用的流包装器,提供对其他流类型的封装,用于扩展或修改流的行为.`stream.h`中虽然有一些方法的具体实现,但是它实现的是一个通用实现,在具体的场景下的性能需求、内存管理方式以及数据交互方式,还需要特定的流类型,如`memorystream.h  memorybuffer.h`等</span>
3. `stream.h`中的`GenericStringStream()`其实和`stringbuffer.h`一样,只是它是对`memorystream.h`的不同字符编码的扩展(这是从仅考虑功能的角度说的,`memorystream.h`是针对于内存块的,因此它会多对于内存块的管理(如内存块大小、是否到达内存缓冲区末尾等),并且`memorystream.h`的最初设计目的是用于不涉及编码的原始二进制字节流的,而不是字符串的)(PS:`memorystream.h`和`memorybuffer.h`感觉基本不会被使用,因为它们两个在大多数场景都能被`StringBuffer`和 `GenericStringStream`代替)
4. `stream.h`的`GenericStreamWrapper`的设计意图是提供一个通用的接口,以支持多种输入流类型和编码方式,它不提供具体的实现,它是一个完全抽象的上层封装接口,它都是被用作基类进行继承(这个输入流包装器和`istreamwrapper.h`这个用于输入流的包装器不一样,后者是提供了具体的实现,并不是完全抽象层)
5. <mark>`StreamTraits`结构体是用于控制流复制的优化,当`copyOptimization=1`,则支持本地副本的创建,这样会将流复制到本地变量中,以减少频繁的指针访问,从而提高性能;反之亦然</mark>
   ```C++
   template<typename Stream, int = StreamTraits<Stream>::copyOptimization>
   class StreamLocalCopy;
   template<typename Stream>
   class StreamLocalCopy<Stream, 1> {
   public:
      StreamLocalCopy(Stream& original) : s(original), original_(original) {}
      ~StreamLocalCopy() { original_ = s; }
      Stream s;// 非引用类型,因此会创建副本
   private:
      StreamLocalCopy& operator=(const StreamLocalCopy&) /* = delete */;
      Stream& original_;
   };
   template<typename Stream>
   class StreamLocalCopy<Stream, 0> {
   public:
      StreamLocalCopy(Stream& original) : s(original) {}
      Stream& s;// 引用类型,不会创建副本
   private:
      StreamLocalCopy& operator=(const StreamLocalCopy&) /* = delete */;
   };
   ```
6. <mark>`GenericInsituStringStream`实现了就地读写操作,与`GenericStringStream`只能读不同,它可以在原地读写.即可以在同一个内存缓冲区中同时进行读取和写入操作,而不需要为修改后的数据分配额外的内存,这对于减少内存占用和提高性能非常有用.其核心思想:使用同一个内存缓冲区,通过不同的指针(`src_`和`dst_`)来分别管理读取和写入操作;使用`PutBegin()`、`PutEnd()`这样的接口明确写入的开始和结束位置;通过`Push()`和`Pop()`提高了灵活的字符管理</mark>
   ```C++
   char json[] = "{\"key\":\"value\"}";  // 原始 JSON 字符串
   GenericInsituStringStream<UTF8<>> stream(json);  // 创建就地流
   stream.Take();  // 跳过第一个字符 '{'
   stream.Take();  // 跳过第二个字符 '"'
   stream.PutBegin();  // 开始写操作
   stream.Put('n');    // 替换字符
   stream.Put('e');
   stream.Put('w');
   stream.Put('_');
   stream.Put('v');
   stream.Put('a');
   stream.Put('l');
   stream.Put('u');
   stream.Put('e');
   stream.PutEnd(stream.PutBegin());  // 结束写操作
   ```


