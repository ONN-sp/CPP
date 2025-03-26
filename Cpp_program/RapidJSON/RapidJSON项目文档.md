1. 本项目是对腾讯的`RapidJSON`项目的一个学习,其最初灵感来源于`RapidXML`
2. 本项目的关键性能优化方法:
   * 使用模板及内联函数降低函数调用的开销
   * SIMD指令优化
   * 自定义内存分配器
   * 短字符串优化技术
   * 跳过空白字符的优化算法:利用SIMD实现的
   * 字符串处理算法:优化算法处理转义字符
   * 本项目中对于`Value`<=>`GenericValue`对象使用了转移语义(`Move()`).这样做对性能十分友好,这样就可以减少大量没必要的内存分配/释放过程,以及内存复制
   * 使用经过优化的Grisu2算法(`dtoa.h`)和专门的浮点数解析算法->`double-conversion`中自定义的`DIY-FP`算法(`diyfp.h strtod.h`) (标准库的`strtod()`函数较缓慢(因为它是面向广泛情况,即包括了异常值、极端情况、错误处理等,所以较为缓慢),因此自己重新实现了) (大多数`JSON`文档中的浮点数往往是常见的简单小数,而不需要处理极端情况.相比之下,`strtod()`在处理任何浮点数时都必须走标准的复杂路径,导致性能开销较大)
3. 本项目不支持`JSON Schema`(在当今的数据驱动时代,`JSON(JavaScript Object Notation)`已经成为数据交换和存储的常用格式(`schema.h`主要用于`JSON Schema`校验,它为 `RAPIDJSON`提供了一些用于验证`JSON`数据是否符合特定模式的功能.使用`JSON Schema`校验,你可以确保传入的`JSON`数据符合预定的结构、类型、格式等约束条件).然而,`JSON`本身并不包含任何数据验证规则,这可能导致数据不一致或格式错误的问题.为了解决这个问题,`JSON Schema`诞生了.它为`JSON`数据提供了强有力的验证机制,确保数据的完整性和准确性.`JSON Schema`是一种用于验证`JSON`数据的规范,它可以帮助我们确保数据的完整性和准确性).`JSON Schema`提供了一种约定和工具来确保`JSON`数据符合特定的结构和规则,`JSON Schema`本身就是一个`JSON`文档.总的来说,`JSON Schema`就相当于提前规定了一个统一的规范,即提前为每个字段分配了规则,如果不符合条件就会验证错误,从而不需要开发人员一个一个去检查字段.在没有`JSON Schema`的情况下,你可能需要手动检查数据的正确性,这会导致维护成本高、出错风险大.使用`JSON Schema`可以提高开发效率、减少错误,并确保数据的一致性和可维护性.举一个`JSON Schema`:
   ```json
      {
   "type": "object",
   "properties": {
      "age": {
         "type": "integer",
         "minimum": 18
      },
      "name": {
         "type": "string"
      }
   },
   "required": ["age", "name"]
      }
      // 在这个例子中，JSON Schema 明确指定了 age 必须是一个整数，且必须大于等于 18，name 必须是字符串，并且这两个字段都是必需的
   ```
4. <mark>`Reader`和`Value`是直接调用`Handler`的,一般是会从外部传入一个`Handler`;而`Writer`和`Document`本身就可以作为`Handler`进行调用</mark>
   ![](markdown图像集/2024-12-24-13-03-28.png)
5. 给出一个`SAX`和`DOM`的`API`都依赖的`Allocator/Encoding/Stream`
   ![](markdown图像集/2024-12-24-13-04-08.png)
6. 迭代解析`JSON`的状态图
   ![](markdown图像集/2024-12-24-13-04-46.png)
7. <span style="color:red;">本项目支持递归解析和迭代解析:
   * 迭代式解析是`Reader::IterativeParse()`,它是逐个解析`JSON`文档中每个`token`决定下一步的状态(`Predict()`).状态转移(`Transit()`)后,根据新的状态继续解析,直到文档解析完成
   * 递归解析是`Reader::Parse()`(除去会执行`Reader::IterativeParse()`)的部分,这一部分调用了`ParseValue()`方法,它是递归解析的核心.`ParseValue()`会根据当前的`token`来递归地解析`JSON`文档中的各种值(比如对象、数组、数字、字符串等).这一过程本质上是一个递归解析,因为它涉及到嵌套的结构.例如,在解析一个对象时,会递归地解析对象的成员,直到整个对象结构被完全解析.这种解析方法正是`DOM`解析,因为它会生成一个完整的`JSON`树.而`Reader::Parse()`这个递归解析就是直接被`Document::Parse()`调用的,即`DOM`树的解析就是用的递归解析</span>
8. 对于一个`JSON`对象`{"s":"a\u0000b"}`,其中`"a\u0000b"`不能直接用`std::strLen()`统计长度,如果用的话就为`length=1`,这是不对的;所以我们需要先对`JSON`文档进行解析`Parse()`,然后键值对的值`"a\u0000b"`中间的转义字符就会被转义出来,此时再用`GetStringLength()`就等于3了
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
3. `JSON`中所有数据都必须嵌套在数字和对象这两种结构中,不能凌驾于之外
4. `JSON`中的对象:
   * 对象用花括号{}包裹:对象内容必须包含在一对大括号内
   * 对象内的数据以键值对的形式存储,每对键和值之间用冒号分隔
   * 键必须为字符串类型,放在双引号" "内
   * 值可以是以下几种`JSON`支持的类型:字符串、数字、布尔值、对象、数组或`null`
   * 键值对之间用逗号分隔:多个键值对按顺序排列,每对之间用逗号分割
5. `JSON`数组:
   * 数组用中括号[]包裹
   * 值之间用逗号分隔
6. `JSON`使用斜杠`\`来转义字符
7. <mark>在`JSON`中,根元素指的是整个`JSON`数据结构的最外层部分</mark>.每个`JSON`文档有且仅有一个根元素,根元素可以为对象或者数组:
   ```json
   [
    {
        "name": "Alice",
        "age": 30
    },
    {
        "name": "Bob",
        "age": 25
    }
   ]
   ```
   上述例子的根元素指的是最外层的数组`[]`,包含了两个对象;但是单独看其中一个对象,不能把它叫做根元素,根元素在这里是整个数组,而不是数组中的每个对象.根元素必须指的是最外层的结构
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
7. <mark>对齐内存边界重要性:对齐操作在计算机系统中非常重要,特别是在处理器访问内存时.处理器从内存读取数据时,通常是按块进行的,最常见的是按4字节(32位)或8字节(64位)读取.对齐的内存访问可以使得处理器只需要一次内存访问即可读取完整的数据,而不对齐的访问可能会导致处理器需要进行两次内存读取.如果数据没有对齐到合适的边界,处理器需要进行两次或更多次内存读取,将结果合并后才能得到所需的数据,这会增加延迟并降低性能</mark>
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
4. `Unicode`:它是一个字符编码标志,旨在为全球所有的文字、符号和字符提供统一的编码方案.它只是一个符号集,定义了每个字符的唯一代码点,但没有规定具体的编码实现方式,其主要特点:
   * 全球覆盖:`Unicode`支持几乎所有的书写系统,包括拉丁文、汉字、阿拉伯文、希腊文、日文、韩文等
   * 唯一的代码点:每个字符在`Unicode`中都有一个唯一的代码点`codepoint`,通常表示为`U+XXXX`的形式,其中`XXXX`是4个十六进制数.如,拉丁字母`A`的代码点是`U+0041`,汉字`汉`的代码点是`U+6C49`
   * 编码方式:`Unicode`有多种编码方式,包括`UTF8 UTF16 UTF32`(没有`ASCII`)
5. <mark>`Unicode`代码点`codepoint`:指的是`Unicode`字符集中每个字符的唯一数字标识符.它们用来表示全球所有文字和符号.代码点是一个非负整数,表示特定字符在`Unicode`标准中的位置.它通常以`U+`后接十六进制数字的形式表示</mark>
6. 单字节字符=`ASCII`字符,只需1字节编码
7. 字节顺序标记`BOM`:它是一种用于指示文本文件的字节顺序和编码格式的特殊字符,它主要用于`UTF`编码,帮助软件识别文件的编码方式.如:在`UTF16`和`UTF32`编码中,`BOM`可以指示字节序(大端或小端).例如,`UTF16`的`BOM`在大端顺序中表示为 `0xFEFF`,在小端顺序中则表示为`0xFFFE`.在`UTF8`编码中,`BOM`为`0xEF 0xBB 0xBF`,虽然`UTF8`中的`BOM`不能表示字节序,但是也可以作为它是`UTF8`编码的标志
8. <mark>`UTF8`没有字节序(`BOM`)问题,可以直接读入;而`UTF16 UTF32`有字节序问题,需要在读取时把字节转换为字符,以及在写入时把字符转换为字节(如小端字节序下,字符的低字节在前,高字节在后,则字符`U+0041`的小端字节表示为`41 00 00 00`,则写入时要转换为`41 00 00 00`写入)</mark>
9. <mark>在`C++`中,`char`通常用于表示`UTF8`编码的字符,而`wchar_t`则用于表示`UTF16`编码的字符.这并不是严格的规则,但在许多情况下,它们的使用是这样的</mark>
10. 如果考虑多种编码类型,那么对于不同的编码类型:<mark>输入流->检测并处理`BOM`(读取数据时,输入流会首先检测文件开头是否存在`BOM`;如果存在`BOM`,输入流会解析并跳过这个`BOM`,然后处理后续`data`区域)(处理`BOM`的好处是流可以自动适应不同的字节序格式,而不需要用户指定编码类型);输出流->在输出流中,编码的第一个步骤通常是根据目标编码写入对应的`BOM`,以便后续的读取操作能够识别文件的编码格式</mark>
11. 本项目只涉及`UTF8`编码,因此没有原`RapidJSON`项目中的`AutoUTFInputStream  AutoUTFOutputStream`,它们的主要功能是处理不同的`UTF`编码(`UTF8 UTF16 UTF32`等)和根据`BOM`自动检测编码类型(即由数据的`BOM`=>`type_`)
12. `class EncodedInputStream<UTF8<>, MemoryStream>`:是用于专门处理内存中且编码为`Encoding=UTF8`的数据的特化版本,使用`MemoryStream`而不是直接使用`InputByteStream` 是因为`MemoryStream`专门针对内存中的数据进行了优化,能够提供更高效、简洁的操作.它不需要处理文件`I/O`或其他外部数据源的复杂性,在处理已经加载到内存中的数据时更加合适.`InputByteStream`是一个更为通用的接口,适合处理多种输入源,但在内存数据处理的场景下,`MemoryStream`是更简洁的选择
13. <mark>`ASCII`码不支持`Unicode`编码,因为`ASCII`码对超过127范围的字符就无法直接表示(因此无法表示`Unicode`(它需要能表示所有字符))</mark>
14. `Unicode`是一个字符集,它定义了每个字符的唯一编号(码点),但不定义如何存储或传输这些字符.`UTF ASCII`等是具体的编码方式
15. `UTF8<><=>UTF<char>`
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
      ~StreamLocalCopy() { original_ = s; }// 在对象销毁时将副本的状态更新回original,确保原流的状态保存一致
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
   // 在构造函数中,会将流保存一个副本,然后可能在后续会对这个流进行各种操作,此时本地副本在访问时能避免Peek()或Take()带来的解引用指针操作,从而提高了性能(通过将流数据复制到栈上的副本中,后续的操作可以直接访问栈上的数据,而不需要每次都解引用指针)
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
# Writer.h/Prettywriter.h
1. <mark>`writer.h`在整个项目中合起来使用时,主要用于将`RapidJSON`的文档对象(`Document`或`Value`)转换为`JSON`格式字符串,即`JSON`数据(本项目由`Document`或`Value`构建)->`JSON`字符串</mark>
   ```C++
   1. 构建JSON数据:通过Document和Value构建JSON数据
   Document d;
   d.SetObject();
   Document::AllocatorType& allocator = d.GetAllocator();
   // 添加键值对
   d.AddMember("name", "John", allocator);
   d.AddMember("age", 30, allocator);
   // 添加嵌套对象
   Value hobbies(kObjectType);
   address.AddMember("city", "New York", allocator);
   address.AddMember("zip", "10001", allocator);
   d.AddMember("address", address, allocator);
   2. 得到JSON字符串
   {
   "name": "John",
   "age": 30
   }
   {"name":"John","age":30}
   ```
2. <span style="color:red;">`JSON`数据是指在内存中存储的数据结构,它是对`JSON`字符串的解析结果,保存在内存中,它是数据本身,是一种轻量级的数据交换格式,它是在内存中的数据结构(如本项目中利用`Document/Value`构建的树形结构),它不是层级式的文本.在`Python`中,一个列表`[]`或一个字典`{}`就可以被称作`JSON`数据(因为它们本质就是一种树形结构).在`C++`中,由于没有直接等价于`JSON`数据结构的内置容器(即没有类似列表和字典这种),所以通过`Document`或`Value`来实现了这种类似功能,即此时表示的也就是`JSON`数据.`JSON`数据通常是采用树形结构(因此使用`Document/Value`),但理论上`JSON`数据可以存储在任何支持层级关系的结构.`JSON`字符串是`JSON`数据的一种文本表示形式,通常用于在网络上传输或存储`JSON`数据(层级式文本`JSON`字符串在传输中可读性很强)</span>
   ```json
   1. JSON字符串(下面这个是JSON字符串(层级式文本了),不是JSON数据)
   {
    "name": "John",
    "age": 30,
    "isStudent": false,
    "address": {
        "city": "New York",
        "zip": "10001"
    }
   }
   2. 对应在内存中的JSON数据会是一个结构化的对象或树形结构,不再是层级式的文本,而是更适合编程语言存储的形式,以本项目为例,上述JSON字符串对应的JSON数据为这个树形的内存结构
   Document d;
   d.SetObject();
   Document::AllocatorType& allocator = d.GetAllocator();
   // 添加键值对
   d.AddMember("name", "John", allocator);
   d.AddMember("age", 30, allocator);
   // 添加嵌套对象
   Value hobbies(kObjectType);
   address.AddMember("city", "New York", allocator);
   address.AddMember("zip", "10001", allocator);
   d.AddMember("address", address, allocator);
   StringBuffer buffer;
   Writer<StringBuffer> writer(buffer);
   d.Accept(writer);
   // 输出 JSON 字符串
   std::cout << buffer.GetString() << std::endl;
   ```
3. `JSON`数据通常使用树形结构来表示,但并不是必须采用严格的树形数据结构.其关键在于`JSON`数据具有层次化、嵌套的特性,这使树形结构成为实现`JSON`数据表示的一种自然选择
4. <mark>本项目中的`Writer`和`Reader`都是一种事件驱动的解析方式,它们不会将整个文档加载到内存中,而是通过触发不同的事件(如`Reader::StartObject()`:遇到对象开始`{` 时调用;`Writer`对于每个元素会触发相应的写入事件,如`null`元素就会触发`Writer::NUll()`),因此`Writer`和`Reader`都是采用的`SAX`风格的解析和序列化方式,事件驱动,不会一次性加载整个`JSON`文档到内存中,而不是采用`DOM`风格</mark>
5. `Writer`在项目使用中往往是通过`Document/Value`来进行传入`JSON`数据的
6. <mark>本项目中的`JSON`数据是通过`Document/Value`对象创建的</mark>
7. `Level`结构体的本质作用就是帮助`Writer`区分当前正在处理的是`JSON`对象({})还是数组([]),并跟踪当前层级的状态
8.  <mark>`JSON`序列化指的是将程序中的数据结构(如对象、数组、字典等)转换为`JSON`字符串的过程(`Writer`并不实现`JSON`序列化的完整过程,因为它不负责构建或管理`JSON`数据的树形结构——这些工作由`Document`(或`Value`对象完成)</mark>
9.  `RawValue()`用于处理已经序列化的`JSON`数据,即将已经序列化的`JSON`数据直接写入输出流:
   ```C++
   1. 序列化前的原始数据结构
   struct Person {
    std::string name;
    int age;
   };
   Person p = {"Alice", 30};
   2. 序列化后的JSON字符串
   {"name": "Alice", "age": 30}
   ```
10. 8位字符->`FF`;16位字符->`FFFF`;8位对应一个字节
11. <mark>`Unicode`代码点的范围为`U+0000`到`U+10FFFF`.`Unicode`大多数常用字符的代码点在`U+0000`到`U+FFFF`之间,这个范围被称为基本多语言平面(BMP)(从`U+0000`到`U+FFFF`,这是`Unicode`中的第一个平面,包含大多数常用字符,如大多数拉丁字符、汉字、阿拉伯数字等).处于BMP范围的字符可以用16位单元表示;从`U+10000`到`U+10FFFF`,这个范围包含了许多不常用的字符,包括某些特殊符号、表情符号等.这部分字符使用代理对进行编码,以适应`UTF-16`</mark>
12. `PutUnsafe(*os_, '\\');`:在字符串中表示一个实际的反斜杠字符,需要使用双反斜杠`\\`,而编译器会将传入的这两个反斜杠视为一个反斜杠字符
13. `Unicode`转义序列:`\uxxxx`
14. </span>`writer`向输出流写入时,对于转义字符且`escape[static_cast<unsigned char>(c)]=='u'`时和不支持`Unicode&&非ASCII码`的两种情况都是转换为`Unicode`转义序列写入输出流;对于常规字符(除了前面两种情况的字符),是直接将其写入到输出流,而不是转换为`Unicode`转义序列</span>
15. <mark>`static_cast<unsigned char>`:这就是求字符的ASCII码</mark>
16. <span style="color:red;">对于输入流中显示给出的字符,如`"Hello,\nWorld"`,这个`\n`会被视为两个字符:`\`这是转义字符,`escape[static_cast<unsigned char>('\')]='\'`,`WriteString()`会将其转义为`\\`,而`n`是常规字符,所以在经过`WriteString()`后,向输出流写入的JSON字符串结果为`"Hello,\\nWorld"`</span>
17. <span style="color:red;">若`"Hello,\nWorld"`末尾有一个不可见的换行符,这个字符是被视为一个字符,而不是两个,它的ASCII码=10,因此`static_cast<unsigned char>('\n')=10`,对应`escape[10]=n`,因此这个不可见的换行符是以`\\n`的方式写入输出流,而不是以JSON转义序列`\uxxxx`的方式写入</span>
18. 从`escape`数组可知,当字符的`ASCII`码值在`escape`中对应为`u`时才会以`Unicode`转义序列方式写入输出流.`ASCII`码=0-7时对应字符`u`,而这个范围内的`ASCII`码对应的字符是控制字符,如`6->ACK字符`,主要用于控制设备(如打印机等)
19. <mark>为什么要使用`Unicode`转义序列?</mark>
    * `Unicode`转义序列(`\uxxxx`)是一种通用的表示方式,独立于具体的字符编码方式.它可以在各种环境下使用,无论是`Unicode`编码,还是不支持`Unicode`编码
    * 在某些环境中,如果字符编码不支持`Unicode`,直接包含非`ASCII`字符(如中文、阿拉伯文等)可能会导致乱码或解析失败.将字符转换为`Unicode`转义序列,能让解析器在不知道具体编码的情况下仍然能够正确解码并显示原字符
    * `JSON`的规范规定了在`JSON`字符串中可以使用`Unicode`转义序列来表示字符
    * 有些非可打印字符和控制字符会在字符串解析过程中引起歧义或错误(对应`excape`数组中的`u`元素).使用 `Unicode`转义序列将这些字符明确地标识出来,有助于解析器准确地处理它们
    * 对于`JSON`解析器(`reader.h`),处理`Unicode`转义序列相对简单,避免了复杂的字符编码检测和转换逻辑.解析器只需要将`\uXXXX`转换为相应的`Unicode`字符(`XXXX`->`Unicode`码点->`Unicode`字符)无需关心当前环境是否支持`Unicode`
20. <mark>目标编码不支持`Unicode`:意思是目标系统或格式使用的字符编码无法直接表示或处理`Unicode`字符,如`ASCII`编码和`ISO-8859-1`编码等</mark>
21. <mark>对于`ASCII`码字符,无论目标编码是否支持`Unicode`,此时都可以直接在目标中解析成功,而不需要`Unicode`转义序列,因为像`ISO-8859-1、UTF8、UTF16、UTF32`等编码都向下兼容`ASCII`,所以就算目标编码不支持`Unicode`,都能解析出`ASCII`字符</mar>
22. 对于目标编码不支持`Unicode&&!ASCII`,此时要使用`Unicode`转义序列来表示输入流中的`Unicode`字符,进而写入不支持`Unicode`的目标输出流.对于`BMP`内的字符,直接使用`JSON`标准的定义的一个`\uxxxx`表示字符就行,而对于超出`BMP`的字符,不能直接用`\uxxxx`表示,此时为了符合`JSON`标准对`Unicode`字符的转义要求符合特定的格式.在`JSON`中,所有`Unicode`字符的编码形式(即使是超出`BMP`的字符)都需要转成`\uXXXX`格式才能写入字符串,因此超出`BMP`的字符需要分解成两个`\uxxxx`来表示,即一个高代理一个低代理,所以这里使用了`UTF16`中的代理对的方法.<mark>在输入流和输出流都是`UTF-8`的情况下,使用`UTF-16`作为中间步骤的编码处理(`UTF16`来处理`codepoint`的高低代理对)并不会影响最终的输出结果,`Unicode`码点是中立的,它与具体的编码方式无关</mark>
    ![](markdown图像集/2024-11-10-11-07-52.png)
23. <mark>本项目使用了`SIMD`指令集`SSE NEON`,用于实现了特化版本`ScanWriteUnescapedString`函数的并行加速处理,提升性能</mark>
24. <mark>`SSE`和`NEON`都是`SIMD`指令集,允许在单个指令中并行处理多个数据元素,即一次可以并行处理16字节数据,从而达到加速处理的目的.`SSE`是由`intel`开发的`SIMD`指令集,主要用于`x86 x86-64`架构处理器上;`NEON`是`ARM`架构的`SIMD`指令集,专为移动和嵌入式设计</mark>
25. <mark>`SSE`处理为什么需要16字节对齐,而不是其它对齐呢?
   因为`SSE`和`NEON`指令集都是使用128位(16字节)宽的寄存器.这意味着它们可以一次性处理128位的数据,即同时处理16字节.因此,将数据对齐到16字节的边界可以让数据以最快的速度加载到`SIMD`寄存器中</mark>
26. 对于使用`SSE NEON`中,代码没有进行额外的地址对齐移动操作,只是在遇到未对齐的部分时,逐字节处理并写入输出流,而在处理对齐的部分时,使用了`SIMD`来加速操作
27. <span style="color:red;">`bool ScanWriteUnescapedString(GenericStringStream<SourceEncoding>& is, size_t length)`和经过`SIMD`指令集加速的`inline bool Writer<StringBuffer>::ScanWriteUnescapedString(StringStream& is, size_t length)`,前者只是进行了判断当前读取位置是否小于给定长度,以此确定是否达到了字符串末尾,而没有提前进行任何的写入输出流或检查特殊字符的操作;对于使用`SIMD`加速的后者,它通过字节对齐和`SIMD`指令加速扫描,尽可能快地检查并写入非特殊字符到输出流中,遇到特殊字符就使`is.src_`指向了此位置(利用`SIMD`指令加速的`ScanWriteUnescapedString`函数直接在内层完成了非特殊字符的写入和特殊字符的检测,目的是减少在上层(如`WriteString()`等)的额外处理操作,显著提升解析和写入性能),然后结束函数</span>
28. `ScanWriteUnescapedString`中的特殊字符为双引号、反斜杠、控制字符(`ASCII`码小于`0X1F`(即31)的,如换行符、退格符等)
29. <mark>`Writer`中利用`SSE/NEON`加速的`ScanWriteUnescapedString`主要用于加速写入未转义的`JSON`字符串内容.它扫描字符串内容,直到遇到需要转义的特殊字符,当遇到这些特殊字符时,`ScanWriteUnescapedString`会停止扫描,即退出函数,因为接下来需要进行转义或特殊处理.`Reader`这利用`SSE/NEON`加速的`SkipWhitespace_SIMD`用于跳过`JSON`字符串开头或中间的空白字符,使得`JSON`解析器可以快速定位到第一个有效字符,当遇到非空白字符时,`SkipWhitespace_SIMD`会停止扫描并返回该字符的位置,以便继续解析操作</mark>
30. `NEON`指令集中对退格符`\b`单独进行检测了,而不是像`SSE`纳入控制字符中被统一检测
31. `Writer`中对`SSE4.2/SSE2`一起考虑处理的,其实在处理时用的是`SSE2`的指令(`SSE4.2`指令包括`SSE2`指令),即没有用`SSE4.2`新引入的指令(如`_mm_cmpistri`等),所以`Writer`中的`SSE`加速和`Reader`中的`SSE4.2`加速有点不同
32. <mark>`PrettyWriter`相较于`Writer`其实就是多了格式化处理+前缀处理(其实就是在前缀中多了格式化处理),即:多了换行、缩进等格式化美化(可读性增强)处理+`Prefix()->PrettyPrefix()`</mark>
33. `vcltq_u8`:比较是否逐字节小于;`vceqq_u8`:比较是否逐字节相等
34. <mark>`vrev64q_u8`执行的是位置对调,8字节数据每两字节进行位置对调.
35. 代码解释:
    ```C++
    if(low==0){// low为全零,表示低8字节无特殊字符
        if(high!=0){// high非零,则通过clzll计算零位数,即算无特殊字符部分的长度
            uint32_t lz = internal::clzll(high);
            return p+8+(lz>>3);// 高8字节出现特殊字符,所以这里要+8  lz>>3<=>lz/8(因为p指针直接加减的是字节)
        }
    }
    // lz>>3<=>lz/8
    ```
36. 由`Writer`构建的`JSON`编写器是一个紧凑的编写器,即不会进行格式化输出处理,这样更节省空间、减少解析时间等
37. `PrettyWriter`与`Writer`的不同展示:
   ```C++
   1. 原始输入流数据
   std::string name = "Alice";
   int age = 30;
   std::vector<std::string> hobbies = {"reading", "hiking", "coding"};
   2. 使用Writer->紧凑的
   {"name":"Alice","age":30,"hobbies":["reading","hiking","coding"]}
   3. 使用PrettyWriter->易读的
   {// 第0级->根元素
    "name": "Alice",// 第一级
    "age": 30,// 第一级->对象
    "hobbies": [// 第一级
        "reading",// 第二级
        "hiking",// 第二级
        "coding"// 第二级
    ]// 第一级
   }// 第0级
   ```
38. 若只需把一些数据转换成`JSON`,可以直接使用`Writer`,这样会比先建立一个`Document`然后用`Writer`把它转换成`JSON`更加方便
# rapidjson.h/fwd.h
1. `!@cond RAPIDJSON_HIDDEN_FROM_DOXYGEN`:`!@cond`用于`Doxygen`文档生成工具,标记这段代码在生成文档时隐藏.结束位置要用`//!@endcond`
2. <mark>现代CPU使用分支预测器来预测条件语句的结果,预测成功时,CPU可以继续顺序执行下一条指令;预测失败(预测的结果和实际结果不符)就会导致流水线清空,即此时CPU需要重新加载指令,产生额外的性能开销</mark>
3. <mark>`RAPIDJSON_LIKELY() RAPIDJSON_UNLIKELY()`:`RAPIDJSON_LIKELY(c)/RAPIDJSON_UNLIKELY(c)`表示的是条件`c`大概率为`true/false`,这是一种帮助提升`CPU`基于分支预测机制时的性能.`RAPIDJSON_LIKELY`和`RAPIDJSON_UNLIKELY`通过帮助`CPU`更好地进行分支预测,从而提升代码执行效率.`RAPIDJSON_LIKELY`和`RAPIDJSON_UNLIKELY`通过使用`__builtin_expect()`的`GCC/Clang`内建函数,`__builtin_expect(!!(x), 1)`告诉编译器`x`更可能为`true`,即最初的预测为`true`.由于现代处理器是通过分支预测机制来提高指令执行效率的,而分支预测会猜测下一条要执行的是什么指令,对于`RAPIDJSON_LIKELY(x_<=>__builtin_expect(!!(x), 1)`,它会将`true`后的指令预先加载到处理器中,当该条件确实为真时,处理器就可以顺序地执行后续指令,此时就会减少指令取值的延迟,从而实现了性能提升;当条件真实为假时,此时由于错误的预先预测,处理器会清空流水线中的指令,因为此时流水线中的指令大多数是基于错误的预先预测而加载的,此时会造成性能损失,但是由于`RAPIDJSON_LIKELY()`大概率是`true`,因此大多数情况下能提高性能.`if(RAPIDJSON_UNLIKELY(condition)`:表示条件`condition`确实为`true`,即`RAPIDJSON_UNLIKELY`预测正确,则`if(RAPIDJSON_UNLIKELY(condition))`此时就是`true`.需要注意的是`RAPIDJSON_UNLIKELY/RAPIDJSON_LIKELY`只是在编译器的优化层面影响,具体`if`语句的真假,还是直接有`condition`决定</mark>
4. `rapidjson.h`的主要作用是:其实就是对一些基本功能和操作定义成了适配本项目的宏的形式,如`RAPIDJSON_MALLOC RAPIDJSON_FREE`等
   * 类型定义:定义了`JSON`中的基本类型,如`kNullType`、`kFalseType`、`kTrueType`、`kObjectType`、`kArrayType`、`kStringType`和`kNumberType`.这些类型用于表示`JSON`的不同数据结构
   * 内存管理:提供了自定义的内存分配和释放接口,比如`RAPIDJSON_MALLOC`、`RAPIDJSON_FREE`,允许用户根据需要重定义内存分配策略
   * 异常处理:定义了断言和静态断言的宏,确保在开发过程中能够捕获错误
   * 版本信息:提供了版本控制的宏,以便用户在代码中检查项目的版本
   * 特性检查:检测编译器是否支持某些`C++`特性,如`C++11`、`C++17`的特性,以便进行相应的条件编译
5. `fwd.h`是为本项目提供前向声明,以便在某些头文件若只是用一个头文件所定义的类(结构体等)这个类型,而不需要知道它的具体定义时为代码之间减少包含依赖,进而提高编译效率
# reader.h
1. `reader.h`将`JSON`字符串->内存中的`JSON`数据(如:在`Reader.Parse()`解析时,当解析到一个数组,然后将这个数组元素通过`Handler`的接口函数将其存储到`vector`内存中,即实现了`JSON`字符串到`JSON`数据的转换)
2. <mark>最后解析完的`JSON`数据怎么表示出来,取决于`Handler`,如:</mark>
   ```C++
   1. 直接输出每一步的解析得到的JSON数据:此时解析到的数据不会存储在内存中,而会直接输出
   class MyHandler : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, MyHandler> {
   public:
      bool StartObject() {
         std::cout << "Start Object\n";
         return true;
      }
      bool EndObject(size_t memberCount) {
         std::cout << "End Object, member count: " << memberCount << "\n";
         return true;
      }
      bool StartArray() {
         std::cout << "Start Array\n";
         return true;
      }
      bool EndArray(size_t elementCount) {
         std::cout << "End Array, element count: " << elementCount << "\n";
         return true;
      }
      bool Key(const char* str, rapidjson::SizeType length, bool copy) {
         std::cout << "Key: " << std::string(str, length) << "\n";
         return true;
      }
      bool String(const char* str, rapidjson::SizeType length, bool copy) {
         std::cout << "String Value: " << std::string(str, length) << "\n";
         return true;
      }
      bool Int(int i) {
         std::cout << "Int Value: " << i << "\n";
         return true;
      }
      bool Uint(unsigned u) {
         std::cout << "Uint Value: " << u << "\n";
         return true;
      }
      bool Bool(bool b) {
         std::cout << "Bool Value: " << (b ? "true" : "false") << "\n";
         return true;
      }
      bool Null() {
         std::cout << "Null Value\n";
         return true;
      }
   };
   2. 直接利用writer作为Handler,进而此时Reader解析到的JSON数据会传递到writer的输出流中保存,可能保存为一个字符串形式
   StringStream s(json); 
   StringBuffer buffer;
   Writer<StringBuffer> writer(buffer);
   Reader reader; 
   reader.Parse<kParseFullPrecisionFlag>(s, writer); 
   ```
3. 一般来说,`Reader`不会直接输出`JSON`数据结构;而是需要一个`Handler`类来接收/处理解析的内容,`Reader`解析的`JSON`数据怎样形式展示出来取决于这个`Handler`   
4. `DOM`解析:`Document::Parse()`;`SAX`解析:`Reader::Parse()`
5. <mark>本项目中的`DOM`的解析`Document::Parse()`,`Document::Parse()`使用`DOM`风格的解析.它将整个`JSON`文档(在本项目中,`JSON`文档就是`JSON`字符串)解析并构建成一个树形的内存结构,也就是`Document`对象(即`JSON`数据).在这个解析过程中,整个`JSON`文档会被加载到内存中并转化为一个数据结构,可以通过`Document`提供的接口进行访问(如:`Document d;...;d["name"];`,直接访问这个树形结构的`JSON`数据).`Reader::Parse()`使用`SAX`风格的解析,它采用事件驱动的方式解析`JSON`数据.这意味着它逐步处理`JSON`数据,遇到`JSON`数据的不同部分时,触发相应的事件并调用`Handler`中定义的回调函数(`Null() RawNumber()`等).`SAX`解析不会将整个`JSON`文档加载到内存中,而是根据输入流逐个触发事件(如遇到`null`就触发`Null()`)来处理数据.在`Reader::Parse()`的回调函数中,你可以选择将每个解析到的部分数据存储到内存中.如:对于每个对象或数组,可以通过回调函数将它们存储在`std::map`或`std::vector`等数据结构中,即可以单独处理每一段`JSON`数据</mark>
6. <mark>`SkipWhitespace_SIMD`是用于加速跳过连续的`JSON`空白字符(包括空格、换行符、回车符和制表符).它利用了`SIMD`指令集来进行并行处理,以一次比较16个字节,从而显著提高性能</mark>
7. 在本项目中,`Writer::ScanWriteUnescapedString`和`Reader::SkipWhitespace_SIMD`函数的确是使用`SSE`指令来加速字符串处理,并且在遇到特定字符时终止循环或返回
8. <mark>为什么`inline const char *SkipWhitespace_SIMD(const char* p, const char* end)`没有内存对齐?</mark>
   不管是`Writer::ScanWriteUnescapedString`还是`Reader::SkipWhitespace_SIMD`中使用的内存对齐,都是对所处理的字符串的开头进行内存对齐,即与`nextAligned`进行对齐,此时就保证了后续每一次都可以16字节为一批的进行读取成功(`Writer::ScanWriteUnescapedString`中给定了`length`,所以要保证不越界,即`p!=endAligned`).这里不内存对齐的原因是,他直接从开头取16字节为一批进行处理,此时很有可能最后剩小于16字节的数据处理不了(因为形成不了一批),所以此时的`for`循环终止条件为`p<=end-16`,此时剩余的字符用`SkipWhitespace(p, end)`单独处理就行
9.  `SSE4.2`在`SSE2`的基础上引入了新指令,如`_mm_cmpistri`.所以`Reader`中对这两种单独进行讨论了
10. <mark>`stack_`主要作用是用于存储临时数据,而不是直接用来跟踪层次结构.具体来说,它在处理复杂的`JSON`值时提供临时存储(如非原地解析(非原地解析才会用`stack_`来构建中间解析结果)的字符串中的转义字符/非转义字符(如`\n`转为`'\n'`)会暂存到`stack_`中,`ParseStringToStream`对普通字符也压入`stack_`(通过`Transcode`)中)</mark>
11. 在本项目中一个`JSON`文档只能有一个根对象,不然会出错.在`Reader::Parse()`中,当`kParseStopWhenDoneFlag`标志位被设置时,解析器会在解析第一个完整的根对象后停止,不会继续向后检查内容.这意味着允许单一根对象解析完成后立即结束,不会报错.而`kParseStopWhenDoneFlag`标志位未被设置时(即`!(parseFlags & kParseStopWhenDoneFlag)`为`true`),解析器会在解析第一个完整根对象后继续读取剩余字符,当发现多余非空字符,即此时有多个根元素,所以解析器返回错误(假设输入`JSON`为`{ "name": "John" } { "age": 30 }`,包含两个根对象`{ "name": "John" }`和`{ "age": 30 }`:解析器首先调用`ParseValue`解析第一个对象`{ "name": "John" }`.跳过空白和注释后,`is.Peek()`读取到第二个`{`,导致 `RAPIDJSON_PARSE_ERROR_NORETURN(kParseErrorDocumentRootNotSingular, is.Tell())`被触发.错误被记录到`parseResult_`,解析提前终止)
12. <span style="color:red;">`if (!IsIterativeParsingDelimiterState(n))`:不是分隔符状态则直接返回`true`,此时没有解析完成,若要继续解析用户可以继续执行下一次迭代解析.此时可以在解析一个`JSON`片段后就返回,如解析一个字符串就可以返回,这样为用户带来了灵活性.这种设计允许用户在迭代模式下逐步解析`JSON`,解析出一个完整的片段(如一个键、一个值、一个数组元素)后暂停解析.用户可以基于每个独立片段做一些中间处理,再继续调用 `IterativeParseNext`进行后续解析.这对于流式解析、增量解析,或处理较大的 JSON 文档时非常实用</span>
13. `ParseValue()`是一个通用解析函数,它可以解析`null true false string object array number`,由于对象的键必须是字符串,所以直接用`ParseString()`,而没有用`ParseValue()`
14. <mark>`RAPIDJSON_FORCEINLINE`表示强制内联,提示编译器尽可能将此函数内联化(`inline`),优化性能</mark>
15. <mark>为什么`ParseHex4()`要`codepoint <<= 4;`?</mark>
    因为每个16进制字符表示4位,所以解析了一个字符后,需要左移4位,来为后面的16进制字符腾出位置
16. `ParseHex4()`举个例:
   ![](markdown图像集/2024-11-09-15-49-41.png)
17. `ParseString()`实现了就地解析和非就地解析,对于非就地解析,需要借助`stackStream`对象中的栈来临时存储数据
18. `ParseStringToStream()`中将`Writer::WriteString()`中的转义字符分成了三种情况处理:简单转义字符、`Unicode`转义字符和控制字符,并且`ParseStringToStream()`将控制字符视为非法字符
20. <mark>在`Unicode`的`UTF16`编码中(当`Unicode`码点超出基本多语言平面就会使用`UTF16`代理对来处理了),代理对必须遵循高代理在前、低代理在后的顺序,因此不会出现只有低代理而没有高代理的情况(出现了,就是错的)</mark>
21. <mark>`ScanCopyUnescapedString(InsituStringStream& is, InsituStringStream& os)`中明明前面已经判断了输入流对象和输出流对象是否一致,从而知道是原地解析了,那为什么还需要判断`if(is.src_==is.dst_)`?</mark>
    即使是同一对象(原地解析),`is.src_`和`is.dst_`可能在具体位置上不同.`src_`和`dst_`分别表示当前读取和写入的位置,随着字符串的解析,它们可能会错开.即使输入流对象`is`和输出流对象`os`相同(即原地处理),如果读取位置`is.src_`和写入位置`is.dst_`不同,那么确实需要执行拷贝操作.这种情况下,字符串解析的过程中会将`src_`指向的内容逐字复制到`dst_`,直到遇到特殊字符或整个字符串解析完毕
22. `NumberStream()`的三个特化版本:
    * `class NumberStream<InputStream, StackCharacter, false, false>`:无备份、无推入,适合纯解析,不需要存储数据
    * `class NumberStream<InputStream, StackCharacter, true, false>`:备份、无推入,适合解析时需要备份,但不是立即存储到栈中(即不是立即解析完一个数值字符串就存入栈中)
    * `class NumberStream<InputStream, StackCharacter, true, true>`:备份、即时推入,适合边解析边备份的需求(即在此特化版本中在`ParseNumber()`解析数值字符串时,调用`Take()`就会起到`TakePush()`的作用(本项目的`ParseNumber()`是直接调用的`TakePush()`))
23. <mark>`Consume()`函数:用于从输入流`InputStream`中读取一个字符,并检查是否于期望字符相等.如果相等,就把它从输入流中移除,返回`true`;不匹配就返回`false`.因此它称消费函数,即匹配时就会消费掉一个字符</mark>
24. <mark>`vrev64q_u8`是一个`ARMv8-A`指令集中的`SIMD`指令,它的作用是对128位的向量进行字节级别的逆序操作,具体来说,是将128位数据中的每64位部分交换,如,前64位和后64位交换.通过`vrev64q_u8`的处理,可以优化后续对检测结构掩码的处理.即交换后,通过`vgetq_lane_u64()`计算的`low`和`high`会更加方便地提取相应掩码值(`clzll`会提取掩码值),从而加速判断的计算.如:假设原始掩码为:`[1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1]`,此时对前8字节`low`和后8字节`high`分别用`clzll`,因为无法直接判断高位和低位的非空字符(`'0'`)是否连续,因此需要额外的边界判断逻辑去合并处理;但是如果反转后(`[0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0]`)就会避免这个边界判断了</mark>
25. `std::numeric_limits<double>::quiet_NaN()`用于生成一个表示"非数值"的`double`类型值
26. 当设置了`kParseNumbersAsStringsFlag`这个标志时,`RapidJSON`不会对数字字符串进行数值解析,而是直接将数字字符串原样保留,传递给相应的回调函数(如`handler.RawNumber()`)
27. <span style="color:red;">本项目的`ParseNumber()`在解析数值字符串时是通过逐步解析和转换来处理的:首先开始时会尝试解析数值字符到`int32`的变量`i`中,如果数值超出了`int32`范围,则使用`int64`的变量`i64`进行保存解析的结果(当`i`的值接近`int32`的最大值(如`2^31-1`或`2147483647`)时,程序会判断是否需要使用 `int64`来存储更大的数值(如大于`2147483647`或负数超出`-2147483648`).如果是,就将数值存储在`int64`类型的`i64`中);如果数值超出了`int64`的范围,则使用`double`类型的变量`d`来保存解析结果.总的来说就是,`int32->int64->double`(`i->i64->d`).整个解析过程是逐步递进的,依赖于当前数字的大小决定是否需要从 `int32`到`int64`再到`double`进行转换.这样可以高效地处理不同范围的数字并保证不丢失精度</span>
28. <mark>`int64`和`double`都是64位,但`double`的表示的正数范围是从`10^(-308)`到`10^(308)`(对于`double`,1位表示符号位,11位表示指数部分((2^10=1024),而10^308≈1023),52位表示尾数,加上隐含的1位,总共53位二进制有效数字,即若要用`double`精确表示数值的话,只能是53位有效数字以内的数值,大于了53位的就不能精确表示(<=>十进制的17位),超出精度的部分会被丢弃或四舍五入,(以四舍五入为例)如`999999999999999999999`利用`double`表示就等于`1e+21`.总的来说,`double`虽然可以存储比`int64`大很多的数值,但是会出现不精确的问题</mark>
    ![](markdown图像集/2024-11-14-23-43-38.png)
29. `ParseNumber()`对于小数的处理:分成了三部分处理:(假设输入流中数值字符串为`123456789012345678901234567890`,那么最终存储在`d`中的是`123456789012345680000000000000`,后方的0就是表示的丢弃的值)
    * `if(!useDouble)`中处理的是小数点前的整数部分没用`double`类型存储的时候,此时可以直接用`double`类型的`d`来存储整个数值字符串(尾数不大于53位的情况)(整数部分+小数部分),此时用`double`存储是精确的
    * `while(RAPIDJSON_LIKELY(s.Peek()>='0'&&s.Peek()<='9')){if(significandDigit < 17)`:处理的是最初小数点前的整数部分用了`double`类型存储,此时处理十进制有效位数不超过17位的情况(<=>尾数不大于53位),此时用`double`存储也是精确的
    * 超过17位十进制的直接使用`TakePush()`消耗字符但不存储,此时是通过丢弃超过部分数字(在`d`中就体现为0),而会丧失精度.通常有被丢弃和四舍五入两种情况,本项目使用的是丢弃这种方法
30. `ParseNumber()`对于含有指数部分的处理:对于指数部分的解析,`maxExp`是用于防止指数的值过大或过小的一个限制.对于负指数:负指数表示小数点向左移,从而缩小数值.`maxExp`用于限制负指数的绝对值,以防止过小的指数值导致浮点数下溢.`exp(int)`的最小值`INT_MIN(-2147483648)`,并且`exp+expFrac`不能小于这个值,因此`int maxExp = (expFrac + 2147483639) / 10;`;同理,对于正指数:在浮点数表示范围内,约为`10^(308)`是`double`能够表示的最大正数.因此,`int maxExp = 308 - expFrac;`
31. `ParseNumber()`中处理一个浮点数字符串的解析时是用`double`类型的`d`来存储整个数值,即其整数部分和小数部分
32. `Reader::Predict()`:基于当前的解析状态和令牌类型,预测下一步的解析状态.使用了一个uuy预定义的状态转移表G(`G[state][token]`:表示当前状态`state`遇到令牌`token`时的下一个状态)
33. <mark>`Reader::Transit()`:`Transit()`是基于有限状态机设计的,它通过当前状态和输入的`JOSN`令牌标记来决定下一步的操作和状态转移.`cur_state+cur_token=>next_state`</mark>,如:
    ![](markdown图像集/2024-11-17-20-24-39.png)
34. `Reader`是`JSON`的`SAX`风格解析器,而`Writer`则是`JSON`的`SAX`风格生成器
35. `Reader::ParseString()`中的`RAPIDJSON_ASSERT(s.Peek()=='\"');`:这个断言的作用是确保在解析字符串时,输入流的当前字符是双引号`"`(`\"`被编译器处理为`"`,这个`\`会被编译器给识别后处理掉),这是`JSON`格式的要求,而不是要求输入流中的双引号必须以转义形式存在(此时可以以转义形式存在`"{\"key\": \"value\" }"`;也可以原始字符串形式`R"({ "key": "value" })"`)
36. `strtod()`可以将字符串解析为`double`,但它毕竟缓慢.本项目使用的是`internal::StrtodNormalPrecision()`和`internal::StrtodFullPrecision()`
## 测试程序
1. `const char json[] = " { \"hello\" : \"world\", \"t\" : true , \"f\" : false, \"n\": null, \"i\":123, \"pi\": 3.1416, \"a\":[1, 2, 3, 4] } ";`和`const char json[] = R"({ "hello" : "world", "t" : true, "f" : false, "n": null, "i":123, "pi": 3.1416, "a":[1, 2, 3, 4] })";`:`C++`的原始字符串是为了方便程序员定义字符串,允许字符串内容中包含未转义的特殊字符.原始字符串在代码中表现为不需要转义,但在内存中,它和普通字符串的内容是完全一样的、
2. 测试结果:
   ![](markdown图像集/2024-11-20-22-26-31.png)
# document.h
1. `Document`对象<=>`DOM`树
2. `Document::Parse()`是用于将`JSON`字符串解析为`JSON DOM`的方法.它接受一个`JSON`字符串,将其解析为树状结构,供用户访问和操作`JSON`数据
3. <mark>通过`Document/Value`构建的对象一定是树形结构</mark>,所以说`Document/Value`实现了`DOM`编程接口
4. 在本项目中,`JSON`数据的内存表示就是`Document`对象.`Document`类是 `RapidJSON`实现的一个`DOM`结构,它用于在内存中构建和操作`JSON`数据
5. `Document`类实现了`RapidJSON`中的`DOM`编程接口.利用`Document`构建的数据结构其实就是一个树形结构,并且`Document`可以直接通过`AddMember() AddArray()`等接口直接操作和修改数据
6. 本项目中`Document`对象就是`JSON`数据
7. `GenericMember`是用于表示`JSON`对象中的一个成员(即一个键值对)
8. `GenericMemberIterator`为`JSON`对象的成员(对象的键值对)提供标准化的迭代器功能(类似标准库的`<Iterator>`,`vector<int> s;s.begin()`,此项目中就是`Value v;v.MemberBegin();v.MemberEnd()`).`GenericMemberIterator`是一个随机访问迭代器,符合`ISO/IEC C++`标准对迭代器的要求.通过实现标准迭代器接口,`GenericMemberIterator`可以轻松地与标准算法(如`std::sort、std::find`等)兼容,从而简化开发流程.此时不需要直接操作指向对象的成员的底层指针,而是对这个对象的成员用迭代器进行访问
9.  为什么`GenericMemberIterator`定义的迭代器就能像`<Iterator>`标准库中的迭代器一样直接可以用于`std::sort()`等函数上?
   因为它严格遵守了`C++`迭代器的概念规范(如:可拷贝和可赋值、递增\递减、随机访问、比较操作、`iterator_traits`支持等),并实现了随机访问迭代器的所有必要接口
10. `GenericValue`用来表示一个`JSON值`,可以是任意`JSON`数据类型(这里不是指的对象中的键值对中的值)对应的值,如:
   ```C++
   1.
   typedef GenericValue<UTF8<>> Value; 
   Value v1(10);// 将10这个整数值存储在v1这个表示JSON值的Value对象,它实际存储在v1这个对象的内部成员变量data_这个联合体中的整数存储区中
   Value v1;
   v1.SetInt(10);// 类似上面,只是用的是接口函数
   2.
   Document document;
   document.SetObject(); // 初始化为一个空对象
   Document::AllocatorType& allocator = document.GetAllocator();
   // 创建字符串类型的键和值
   GenericValue<UTF8<>> nameKey("name", allocator);// 创建一个表示键值对的键的JSON值
   GenericValue<UTF8<>> nameValue("John Doe", allocator);// 创建一个表示键值对的值的JSON值
   // 创建整数类型的键和值
   GenericValue<UTF8<>> ageKey("age", allocator);
   GenericValue<UTF8<>> ageValue(30);
   // 创建布尔类型的键和值
   GenericValue<UTF8<>> marriedKey("married", allocator);
   GenericValue<UTF8<>> marriedValue(true);
   document.AddMember(nameKey, nameValue, allocator);
   document.AddMember(ageKey, ageValue, allocator);
   document.AddMember(marriedKey, marriedValue, allocator);
   ```
11. <mark>`placement new`:它是`C++`中一种特殊的`new`运算符,允许在已分配的内存地址上构造对象.与标准`new`运算符不同,`placement new`并不负责内存的分配,而是将对象的构造操作直接放置在已分配的内存中</mark>
    ```C++
    new (pointer) Type(args);// 在一个已分配内存地址pointer上构造一个Type(args)对象
    ```
12. `uint64_t`:`0-2^64-1`;`int64_t`:`-2^63-2^63-1`.对于一个整数做`flags`标记时,它可能可以同时被标记为几个`flag`,如:
   ```C++
   1.
   data.f.flags = (u&0x80000000) ? kNumberUintFlag : (kNumberUintFlag|kIntFlag|kInt64Flag);
   // (u&0x80000000)取的是u的最高位,若最高位为1那么只设置kNumberUintFlag标志(因为此时的u就超过了int可以表示的正数范围,即此时u不能用int表示了);若最高位为0,则此时u可以视为int来处理(因为int包括了此时u的范围),那么此时可以被视为无符号整数标志、带符号整数标志或int标志
   2.
   if(i64>=0){// 非负的64位整数,
   data.f.flags |= kNumberUint64Flag;// 此时int64可以视为uint64来处理(因为uint64包括了此时i64的范围),设置无符号uint64_t标记
   if(!(static_cast<uint64_t>(i64)&RAPIDJSON_UINT64_C2(0xFFFFFFFF, 0x00000000)))// 查看i64的高32位是否为0,若为0则i64在uint32范围内
      data.f.flags |= kUintFlag;// 设置为uint无符号
   if(!(static_cast<uint64_t>(i64)&RAPIDJSON_UINT64_C2(0xFFFFFFFF, 0x80000000)))// 查看i64的高32位和低32位,高32位=0,低32位的第一位=0,此时在int32有符号范围内
      data_.f.flags |= kIntFlag;// 设置为int有符号
   }
   ```
13. <mark>本项目中`JSON数组`既可以用`GenericArray`表示,也可以用`GenericValue`这种更通用的方式表示.`GenericValue`是一个通用的数据类型,可以表示`JSON`数组、对象、数字、字符串等.它通过`flags`字段来标识当前数据类型.而`GenericArray`是专门用于表示和操作`JSON`数组的类,这个类操作起来更直观,`GenericArray`主要提供了数组级别的操作,如增加、删除元素,访问数组中的元素等.它是一个专用的容器类型,类似于`C++`标准库中的`std::vector`</mark>
    ```C++
    1. 利用GenericValue表示JSON数组
    GenericValue<UTF8<>, Allocator> jsonArray(kArrayType);
    // 向数组中添加元素
    jsonArray.PushBack(GenericValue<UTF8<>, Allocator>(1), allocator);// 添加整数 1
    // 此时是用GenericValue直接表示一个JSON数组jsonArray,此时调用的PushBack方法其实是往这个表示JSON数组的GenericValue对象jsonArray中添加JSON整数值1这个GenericValue对象,即宏观看是往GenericValue中添加GenericValue
    2. 利用GenericArray表示JSON数组
    GenericArray<GenericValue<UTF8<>, Allocator>> jsonArray;
    jsonArray.PushBack(GenericValue<UTF8<>, Allocator>(1), allocator);
    // 直接用GenericArray表示JSON数组,它其实就是一个容器类,封装了GenericValue的数组,可以更方便地进行数组操作,即GenericArray其实是一个GenericValue数组,其每个数组元素就是一个GenericValue对象.这个PushBack()宏观上看就是一个往GenericArray中添加GenericValue的操作了
    ```
14. `IsLosslessFloat()`中既然是把当前对象先转换为`double`类型的`a`,再进行判断转换为`float`是否无损.为什么不先判断是否可以无损表示为`double`,而直接判断转换后的`double a`是否可以无损转换为`float`呢?
   因为如果是有损表示为`double`,那么当用这个有损的`double`值来判断得出可以无损转换为`float`,此时也说明可以无损转换为`float`(即使`int64_t`到`double`有损,这并不一定意味着它无法无损表示为`float`)
15. `RAPIDJSON_DISABLEIF_RETURN((internal::NotExpr<internal::IsSame<typename internal::RemoveConst<T>::Type, Ch>>), (GenericValue&))`:`internal::NotExpr`:取反;`internal::IsSame`:检查两个参数是否相同;`internal::RemoveConst`:若`T`有`const`,则去掉`T`中的`const`.此行代码的含义是:当去掉`const`后的`T`与`Ch`一样,则条件不成立.此时模板函数的返回类型为`GenericValue&`;若条件成立,则禁用这个模板函数
16. 本项目中`JSON`对象可以用迭代器访问,也可以用索引`[]`方式访问;而数组没有用迭代器访问的方法,只有索引访问的方式(`GenericValue`中对于对象和数组都分别设计了重载的索引方式访问`[]`),数组中的`ValueIterator`其实就是指针,即数组只有指针访问的形式
17. `RemoveMember()`和`EraseMember()`是实现的类似`vector`的`std::remove()`和`vec.erase()`方法,最终底层调用的函数传入的参数都是迭代器.即这样设计就是为了将本项目中的`JSON`对象访问其成员时,设计成类似`std::vector`中通过迭代器而对元素删除的高效操作(`std::remove() erase()`)
18. <mark>对于`DoRemoveMember()`(它返回的是指向删除位置的迭代器),它的删除逻辑采用了"如果对象中有多个成员且删除的不是最后一个,则将最后一个成员移动到被删除的位置",为什么要这样?</mark>
   这样做虽然会破坏内部的顺序,但是只需要一次覆盖操作(如果整体往前移,就需要多次覆盖).这种涉及不需要频繁地移动大量数据,尤其当对象中有很多成员时,这种方法可以显著提升性能
19. `C++`允许非`const`参数向`const`参数隐式转换,因此`EraseMember(const GenericValue<Encoding, SourceAllocator>& name)`中的`EraseMember(m);`,`m`虽然是`MemberIterator`,但在调用`EraseMember(m);`时会隐式转换为`ConstMemberIterator`,即最终会匹配`MemberIterator EraseMember(ConstMemberIterator pos)`这个函数
20. <span style="color:red;">`C++`的引用折叠:在`C++11`中,右值引用可以与左值引用相互转换,特别是当它们作为函数参数传递时,允许临时对象或优质对象作为左值引用传递给参数</span>
   ```C++
   GenericValue& PushBack(GenericValue&& value, Allocator& allocator){
      return PushBack(value, allocator);// 这里使用了C++11的引用折叠
   }
   // 这里的PushBack(value, allocator);调用了底层的GenericValue& PushBack(GenericValue& value, Allocator& allocator),这里使用了引用折叠,即将右值引用(&&)参数直接绑定到左值引用(&)参数上了,这不会报错
   ```
21. `std::memmove`:`void* memmove(void* dest, const void* src, size_t count);`:将内存从`src`位置拷贝到`dest`位置,拷贝的内存大小是`count`
22. <mark>在`GetDouble()`中,因为在`C++`中,`int`和`unsigned`类型到`double`类型的转换是安全的隐式类型转换(这是因为`double`可以表示`int`和`unsigned`的所有可能值,且`double`的范围比`int`和`unsigned`大得多,则此时不会发生精度损失的情况),所以不用`static_cat<double>`;然而,对于`int64_t`和`uint64_t`的位数更大,它转换为`double`类型可能会出现精度损失的情况,所以它不会自动进行从`int64_t`或`uint64_t`到`double`的隐式转换,此时就需要显示地使用`static_cast<double>`</mark>
23. <span style="color:red;">`GenericValue::Accept()`:接受并处理传入的`Handler`对象,它对当前`GenericValue`对象进行遍历并根据不同的数据类型执行指定的`Handler`操作,`Handler`通常是用户自定义从外部传入的.对于`JSON`对象的键值对的值和`JSON`数组的元素,它们可以是任意类型,所以在遍历的时候需要进一步调用`Accept()`,即如`for(ConstMemberIterator m=MemberBegin();m!=MemberEnd();++m) if(RAPIDJSON_UNLIKELY(!m->value.Accept(handler))) return false;`<span>
24. `GeneriValue`中其实就是表示一个`JSON`值(注意:这里说的`JSON`值不是指的`JSON`对象中键值对的值,而是指的所有`JSON`的可能值,如:数字、字符串、逻辑值、数字、对象、`null`的等),它可以是`JSON`数组、对象、基本类型元素等,其具体的就是操作`Data`这个共用体(即针对`Data`中不同的`JSON`值进行不同的重载函数操作)
25. <mark>标志位枚举类中为什么`kTypeMask=0x07`,它有什么用?</mark>
    `kTypeMask`用于在标志中提取类型信息的掩码,通过最低3位来标识数据的基本类型(从`rapidjson.h`中的枚举类`Type`中可知,所有类型可以用3位二进制表示完,因为只有7种),则等于`0x07=0x00000111`,这样就能提取中标志位的最低3位,对于标志位来说最低3位都是不用来表示`flag`的,如`kBoolFlag=0x0008,kInlineStrFlag=0x1000`,最低3位就是八进制中最后一位,都是0,因此可以直接从标志位中通过`kTypeMask`按位与就能获得类型
26. `Flag`结构体中`payload`用于存储与指针相关的内存数据和其它优化信息,对于不同的系统,使用了不同的填充字节,如:64位系统中`char payload[sizeof(SizeType)*2+sizeof(void*)+6];`:用了6个填充字节,这是因为`sizeof(SizeType)*2+sizeof(void*)`=16字节,而`flags`又占2字节,因此这个结构体位18字节,64位系统中内存对齐通常是8字节,此时18字节未对齐,因此填充了6个字节;32位系统也是同理的.至于为什么要用`sizeof(SizeType)*2+sizeof(void*)`:个人猜测是因为`payload`后续会被用在`ShortString`的定义中,而从`String`结构体可知,通常由两个`SizeType`+一个`char*`指针构成,因此`payload`设置成这样
27. <mark>短字符串优化技术`ShortString`需注意:</mark>
    * 在字符串`Ch str[MaxChars];`中,最后一个字节存储的是该字符串长度的信息
    * `MaxChars = sizeof(static_cast<Flag*>(0)->payload)/sizeof(Ch),MaxSize = MaxChars-1,LenPos = MaxSize`和`Ch str[MaxChars];`:对于32位系统,`str`就是有13字节的有效字符的字符串;64位就对应的是21字节
    * 在`ShortString`中,我们使用的是反向方式来间接存储该字符串的长度,即`str[LenPos]`保存的不是直接的该字符串的长度,而是最大有效字节数`MaxSize`减去该字符串的长度.这样做的好处:
      - 节省存储空间:这样做就不需要单独用一个字节去表示字符串的实际长度,而是直接用`MaxSize-len`
      - 防止直接暴露长度信息:这样做隐藏了实际的字符串长度,从而为开发者带来一些"安全性" 
    * `Ch str[MaxChars];`这是直接存储在`ShortString`这个结构体里了,所以称作内联存储,因此对应的是`kInlineStrFlag`标志(`ShortString`的字符串直接存储在这个结构体内存中,而不是像`String`中:它表示的字符串的存储位置用一个指向其他内存位置的指针来表示)  
28. <mark>`GetMapLayoutSize()`是用于计算内存布局后存储`Map`映射这种数据结构所使用的内存字节数,这里使用了对齐操作`RAPIDJSON_ALIGN()`.我们定义的内存布局为`[Map*][SizeType][Member[capacity]][MapIterator[capacity]]`,即在内存中映射数据结构以这种方式存储在内存中,具体实现这样存储的操作是`GetMapCapacity() GetMapMembers() GetMapIterators()`三个函数实现的.对于一个`Map* maps;`,要通过该映射找成员指针`Member*=GetMapMembers(maps)`,`maps`指的是`Map`的指针,而后续`Member*`或`MapIterator*`要按照内存布局中存储的顺序往后就能找到</mark>
29. 按照此内存布局,映射表、成员数组、迭代器是相关联的,即对应同一个`JSON`键值对的索引在三个地方都是一样的,即`Map* map;Member* members;MapIterator* Iterators;`中,`map->begin()(第一个键值对);memebers[0](第一个成员);Iterators[0](第一个迭代器)`都是指的第一个`JSON`键值对
30. <mark>如何能确保`GetMapMembers()/GetMapIterators()/GetMap()`得到的`Member*/MapIterator*/Map*`就是指向的传入的映射表中的键值对,即如何让`Member[]`数组存储的映射表的键值对?</mark>
    这个过程是通过`DoAddMember()`实现的,我们规定了内存布局,然后通过`DoAddMember()`会将给定的成员的`name`和`value`存储再`Member[]`中.同时,它还会更新映射表`map`,以及迭代器`MapIterator`.此函数通过`GetMap(members)`保证了将成员`insert`的就是内存布局中与`Member*`相关联的映射表;`GetMapIterator(map)`保证了拷贝到的迭代器就是`map`映射表在内存布局中对应的迭代器(因此,若要使用此内存布局形式的映射表、成员数组、迭代器,要先使用`DoAddMember()`初始化这个内存布局,即把成员放到该放的地方,并使映射表、成员数组、迭代器三个部分都与传入的`JSON`键值对相关联)
31.  `GetMapCapacity() GetMapMembers() GetMapIterators()`这三个函数共同作用,通过精确的内存偏移计算确保`Map*、SizeType、Member[]`和`MapIterator[]`在内存中的顺序.`Map*`在最前面;紧随其后的是`SizeType`,即表示容量的字段;`Member[]`紧接在`SizeType`后面;最后是`MapIterator[]`,在`Member[]`之后
32. `GetMapMembers() GetMapIterators()`不仅确保了`Member[]`和`MapIterator[]`在内存布局中的位置,而且返回得到的`Member* MapIterator*`就是传入的`map`的键值对指针和迭代器
33. `DropMapIterator()`明明返回的新迭代器和传入的旧迭代器是一样的,为什么要换成新的迭代器进行返回?
   这样做可以返回一个安全的迭代器,即这个新的迭代器当前是没有被外部持有的,而传入的旧的迭代器可能在外部某个地方被持有了,我们在这个函数内就将旧的给析构了,然后重新用一个外部目前每持有的新的迭代器,就算这个迭代器和之前的旧迭代器一样也是有意义的
34. 本项目对于`JSON`数组和`JSON`对象中的成员的扩容操作都是使"用当前容量的加倍策略来增加容量",类似于`vector`
35. <mark>`typedef std::multimap<Data, SizeType, Less, StdAllocator<Pair, Allocator>> Map;``Map`中存储的是`JSON`对象成员的键(`key`)与该成员在对象中的位置索引,而不是`JSON`键与`JSON`值的映射</mark>
36. `DoEraseMembers()`中将`first-last`的成员删除后,还需要将`last`后未被删除的成员移到已删除的位置,即往前移,移到`first`位置
37. <mark>本项目在处理`JSON`对象成员时,使用了两种方式,一种是使用`std::multimap`构建一个映射表,进而可以直接调用这个容器的一些处理方法(`erase find`等)(`C++11`),需要注意的是这个映射表的内容是`JSON`对象成员的键(`key`)与该成员在对象中的位置索引的映射(只有`JSON`对象才设计映射表处理);另一种就是不使用映射表,直接使用迭代器、指针等进行删除、添加等方法</mark>
38. `GenericDocument`中的`ownAllocator_`和`allocator_`在外部没传入内存分配器时,不就是在程序中会设为一样的吗,那么为什么还要单独用`ownAllocator_`?
    `allocator_`是外部传入的内存分配器,表示用户在创建`GenericDocument`时提供的分配器,如果用户没有提供分配器,那么会`new`一个分配器然后赋给`allocator_`.`ownAllocator_`是用来管理内部分配器的,即若外部提供了,那么`ownAllocator_`就没用了,虽然在内部`new`分配器时,`ownAllocator_=allocator_`,但是它们两个设计的目的不同,`ownAllocator_`的存在确保了`GenericDocument`即使在没有外部分配器时,依然能够自动创建并管理一个内部分配器,以确保内存能够正确地分配和释放(从`Destory()`中的`RAPIDJSON_DELETE(ownAllocator_);`可以看出)
39. `GenericDocument::~GenericDocument()`中的`ValueType::SetNull()`其作用是将当前`JSON`文档的内容清空,设置为`Null`,而不会说把这个`JSON`文档内存给释放,因为在`SetNull`中,其实会重新分配内存,只是此时是空的而已.`Destory()`的作用是销毁`ownAllocator_`指向的内存,即释放`ownAllocator_`指向的分配器本身的内存(因为动态`new`了一个内存分配器),而不会释放由分配器管理的内存
40. 在派生类`GenericDocument::Swap`方法中,首先要调用基类`GenericValue`的 `Swap`方法来交换基类的成员数据(`ValueType::Swap(rhs);`).这样做是为了确保基类的数据能够正确交换,因为基类的数据是派生类对象的一部分.基类的数据交换完毕后,接下来才是交换派生类特有的成员数据.这样可以确保派生类特有的数据在基类部分已经交换之后再进行交换
41. `Populate(Generator& g)`:通过生成器填充当前`GenericDocument`对象,函数中的`g(*this)`会向当前`GenericDocument`对象发送`SAX`事件,即解析过程中触发回调事件时就会调用`GenericDocument`的`Handler`操作,然后通过这个`Handler`操作把解析的`JSON`数据保存到当前`GenericDocument`对象的`stack_`中.最后,`ValueType::operator=(*stack_.template Pop<ValueType>(1));`调用了当前`GenericDocument`对象`stack_`栈中的赋值操作符,即从栈中弹出解析的`JSON`数据,然后将其赋值到当前`GenericDocument`对象对应的基类`GenericValue`的数据`data_`中
42. `GenericDocument::ParseStream()`实际底层是根据不同解析标志调用`Reader::Parse`
43. 为什么需要`operator ParseResult()`?
    这是一个类型转换运算符,可以将一个`GenericDocument`对象直接转换为`ParseResult`类型对象.对于如果一个`GenericDocument`对象包含了相应的`ParseResult`,那么可以直接通过类型转换运算符访问解析状态.这样就不需要显式地访问`parseResult_`成员变量了
44. 利用`GenericDocument`的`Parse`会将当前`GenericDocument`对象作为调用底层`Reader::Parse()`时的`Handler`进行传入,然后在解析过程中,会回调`GenericDocument`实现的`Handler`的`API`,如`Key() EndObject()`等.从实现的`API`函数可知,用`GenericDocument`作为`Handler`,会把解析的`JSON`数据先存储在当前`GenericDocument`对象的`stack_`中,然后再通过`SetObjectRaw() SetArrayRaw()`传到当前这个`GenericValue`对象中,重新组织成了一个完整的`JSON`对象或数组
45. <span style="color:red;">`ValueType::operator=(*stack_.template Pop<ValueType>(1));`:这一句程序是实现`GenericDocument`将解析后的`JSON`数据存储在当前`GenericDocument`对象中的关键,因为`GenericDocument::Parse()`会在解析过程先用`stack_`暂存`JSON`解析过程中产生的`JSON`数据,最终解析到`EndObject() EndArray()`时,将栈中的数据构建成完整的对象或数组(栈中逐渐构成了一个树状结构`DOM`树),然后通过`SetObjectRaw()`或`SetArrayRaw()`将它们关联到当前的对象或数组(这个当前对象和当前数值位于`stack_`此时的栈顶).因此,利用`GenericDocument`解析得到的最终`JSON`结果会存储在`stack_`的栈顶,这个栈顶元素就是最终的解析结果,代表整个`JSON`文档(它可能是一个对象或者数组,内部也嵌套了很多对象或数组).那么,`ValueType::operator=(*stack_.template Pop<ValueType>(1));`就实现了在解析成功时,将存储在栈顶的`JSON`结果,赋值到当前这个`GenericDocument`对象(由于`GenericDocument`继承自`GenericValue`,它的内部成员`data_`会持有`JSON`解析后的数据),即当前`GenericDocument`对象也是一个存储了`JSON`解析结果的`DOM`树</span>
46. `ValueType::operator=(*stack_.template Pop<ValueType>(1));`这是一个成员函数的调用,即等价于`ValueType temp = *stack_.template Pop<ValueType>(1);this->operator=(temp);`.而重载的赋值运算符也可以写作`A=B`的形式,即`*this = *stack_.template Pop<ValueType>(1);`
47. `stack_.ShrinkToFit();`:确保栈在`Empty()`后会调整其内部的内存分配,以释放未使用的空间,即释放`stack_`这个栈数据结构占据的内存
48. 为什么`GenericDocument::StartObject()`和`Reader::StartObject()`为什么和`Writer::StartObject()`不同,`Writer::StartObject()`需要写入一个`{`,而其余两种不用,为什么?
    主要原因是`GenericDocument::StartObject()`和`Reader::StartObject()`并没有涉及实际的输出操作,它只是在解析`JSON`字符串时创建的一个新的对象并将相应解析出的数据进行`Handler`操作(如推入`stack_`中);而`Writer::StartObject()`是用于生成`JSON`字符串并写入传入的`JSON`数据,所以要输出`{`
49. <mark>`GenericArray::PushBack()`会直接调用`GenericValue::PushBack()`,那定义一个`GenericArray`对象怎么保证其中的`value_`一定是`kArrayFlag`,明明在`GenericArray`构造函数中没有定义`value_`的`data_.flags`啊?</mark>
   因为在使用`GenericArray`时不是直接定义这个容器对象的,而是通过使用`GenericValue::SetArray()`或`GenericValue::GetArray()`来得到一个`GenericArray`对象的,这样就能保证`kArrayFlag`,如`Document d1;d1.SetArray().PushBack(1, a).PushBack(2, a);`和`Value z;z.SetArray();`
50. <mark>`GenericArray`其实就是定义的一个容器,然后把`GenericValue`中对于`flags =kArrayFlag`的相应方法封装到`GenericArray`这个容器中,即专门定义了一个处理`JSON`数组的容器类.`value_`就是有着`kArrayFlag`设置的一个`GenericValue`对象,所以`GenericArray`表示的是`JSON`数组</mark>
51. `JSON`对象添加成员和`JSON`数组添加元素不一样,`JSON`数组添加/移除元素是像`vector`容器一样用`PushBack/PopBack`,而`JSON`对象是用`AddMember/RemoveMember
52. `GenericObject`和`GenericArray`实现的目的一样,设计的过程基本也一样
53. <mark>既然引用类型必须在定义时就要初始化,即绑定到一个已经存在的对象中,那么为什么在`GenericArray/GenericObject`中可以直接`ValueType& value_;`?</mark>
    引用类型定义时必须初始化,对于`GenericArray/GenericObject`,因为这两个类的构造函数都会对`value_`进行初始化,所以定义和初始化其实是一起的,不是分离的,所以这是可行的
54. <span style="color:red;">`GenericValue`中提供两个`String`的存储策略.一个是`copy-string`:分配缓冲区,然后把源数据复制至它;一个是`const-string`:简单地存储字符串的指针(字符串字面量的优化,此时无需额外分配).这两种存储方式分别是用`SetStringRaw(StringRefType s)`和`SetStringRaw(StringRefType s, Allocator& allocator)`管理的,前一个表示`const-string`存储,即不需要`allocator`,只是按简单传递字面量</span>
55. <mark>对于`JSON`数组,我们采用的是流畅接口,可以链式调用,即几个`PushBack()`写在一起,如`PushBack()/PopBack()`,它们返回的是当前`GenericArray`对象,而不像`STL`库中的`std::vector`等的`push_back`返回的是`void`类型,即不能链式调用</mark>
56. `CopyFrom`实现了对当前`DOM`树深拷贝的功能(为新对象分配新的内存)赋值
57. 本项目中对于`Value`<=>`GenericValue`对象使用了转移语义(`Move()`).这样做对性能十分友好,这样就可以减少大量没必要的内存分配/释放过程,以及内存复制
   ```C++
   GenericValue& operator=(GenericValue&& rhs) RAPIDJSON_NOEXCEPT{
                     return *this = rhs.Move();// 调用rhs.Move()将右值rhs转换为左值,然后使用前面重载的拷贝赋值操作符将其赋给当前对象this
                  }
   ```
58. `document.h`可以解析一个`JSON`至`DOM`,然后就可以轻松查询及修改`DOM`,并最终转换回`JSON`(`writer.h/prettywriter.h`)
59. 举一个例子:
   ```json
   {
   "hello": "world",
   "t": true ,
   "f": false,
   "n": null,
   "i": 123,
   "pi": 3.1416,
   "a": [1, 2, 3, 4]
   }
   ```
   解析成一个`DOM`树后:
   ![](markdown图像集/2024-12-07-21-34-37.png)
60. 由于解析后的`string`长度总是短于或等于原来存储再`JSON`的`string`,因此可以原地解析
    ![](markdown图像集/2024-12-28-14-38-23.png)
61. `Document`的解析完全依靠`Reader`.实际上`Document`是一个`Handler`,在解析`JSON`时接收事件去建立一个`DOM`.使用者可以直接使用`Reader`去建立其它数据结构,而`Document`时利用`Reader`建立的一个`DOM`数据结构
## 测试结果
1. ![](markdown图像集/2024-12-07-17-18-48.png)
# pointer
1. `Pointer`是一个用于表示和解析`JSON`文档中路径的机制,类似`XPath`(用于`XML`文档的路径查询语言).它提供了一种通过路径精确定位`JSON`文档(`DOM`树)中某个值的方法.如:
   ```C++
   const char* json = R"({
      "book": {
         "title": "RAPIDJSON Tutorial",
         "author": "John Doe",
         "chapters": [
            { "title": "Introduction", "page": 1 },
            { "title": "Advanced Topics", "page": 50 }
         ]
      }
      })";
   Document doc;
   doc.Parse(json);
   Pointer pointer("/book/chapters/0/title");// 指向doc["book"]["chapters"][0]["title"]
   ```
2. `Pointer`解析的字符串(`source`)有两种表示方式:
   * 使用路径表达式,如:`"/foo/0"`
   * 使用`URI`片段表达式,如:`"#/foo/0"`
3. `GetUri()`的一个例子,根据一个`JSON`文档(`DOM`树)和一系列`URI`片段(通常路径),解析得到一个最终的`URI`
   ```C++
   const char* json = R"({
   "article": {
      "id": "/article/123",
      "details": {
         "id": "/details"
      }
   },
   "image": {
      "id": "/image/456"
   }
   })";
   Document doc;
   doc.Parse(json);
   // 假设 rootUri 是基础 URI
   UriType rootUri = UriType("http://example.com", allocator);
   // 假设 tokens_ 包含路径片段 ["article", "details"]  GetUri实际就是找每一个token对应的id字段,然后将其添加在当前rootUri后面
   UriType finalUri = Pointer("/article/details").GetUri(doc, rootUri);
   // 打印解析后的 URI
   std::cout << finalUri.ToString() << std::endl;
   =>
   // http://example.com/article/123/details
   ```
4. `GenericPointer`对象传入的路径`source`会被解析到`tokens_`
5. `GenericPointer`对象的令牌可以有两种形式,一种是针对于对象的`key`字符串(那么该令牌的`token.name`是一个指向该成员名称的字符串,此时`token.index`设为`kPointerInvalidIndex`);一种是针对于数组的索引,这个索引会在对应的`token.name`和`token.index`都设置(即对应的`token`的`name`会被设置为数字索引对应的字符串;`token.index`就被设置为对应的数值),如:`Pointer pointer("/book/chapters/0/title");指向doc["book"]["chapters"][0]["title"]`
6. `Token`结构体:
   ```C++
   struct Token {
       const Ch* name;// 令牌名称,即JSON Pointer中当前部分的字符串表示
       SizeType length;// 表示令牌名称的长度,不包括终止的空字符
       SizeType index;// 如果令牌表示的是JSON数组的索引,这个字段就存储该索引的值.如果该令牌表示一个JSON对象的名称,则index=kPointerInvalidIndex(如果不等于kPointerInvalidIndex,则为有效的JSON数组的索引)
   };
   ```
7. `token`的`index==kPointerInvalidIndex`:表示当前令牌名称(即这个`token.name`)是对象的名称,而不是数组索引
8. `tokens_`就是用于路径解析中的令牌数组,即它的每一个元素就是一个令牌,每个令牌指向`JSON`数据结构(如`JSON`文档)中的某个节点,即`DOM`树的某个节点
9.  <mark>`GenericPointer`有两类构造函数,一类是传入一个字符串或`URI`片段(即`source`),解析得到对应的`tokens`令牌(此时会创建动态内存`nameBuffer_`);另一类是直接传入设定好的`tokens`令牌</mark>
10. <mark>`nameBuffer_`只有在构造`Pointer`对象时传入`Ch* source`时才会使用,对于传入`tokens`的构造对象时不会使用.`nameBuffer_`是一个内存缓冲区,用来存储多个路径片段字符串(即每一个`token.name`),这个指针指向的内存是`tokens_`这个结构体数组内存后面紧接着的内存地址,即`nameBuffer_`紧随`tokens_`后面,保证它们之间没有内存冲突.具体让`nameBuffer_[i]=tokens_[i].name`是在`Parse()`中实现的</mark>,如:`CopyFromRaw()`
   ```C++
   // 分配内存:首先为tokens分配内存,再为nameBuffer分配内存(包含extraNameBufferSize指定的额外空间)
   tokens_ = static_cast<Token*>(allocator_->Malloc(tokenCount_*sizeof(Token)+(nameBufferSize+extraNameBufferSize)*sizeof(Ch)));
   nameBuffer_ = reinterpret_cast<Ch*>(tokens_+tokenCount_);// nameBuffer是存储所有token名称(token->name)的缓冲区,分配空间紧接在tokens_之后
   ```
11. <mark>`GenericPointer Append(SizeType index, Allocator* allocator=nullptr)`执行的是传入`index`索引的追加`token`操作,即处理的是传入一个`JSON`数组索引的令牌`token`,并将其追加到当前`GenericPointer`中.会根据这个`index`,创建一个`token`(`name`对应`index`转换的字符串,`length`对应转换字符串的长度,`Token`结构体中的`index`就对应传入的`index`这个值)</mark>
12. <mark>`GenericPointer Append(SizeType index, Allocator* allocator=nullptr)`中为什么要将`index`转换为字符数组`char* buffer`后,还要讨论`Ch`的字节类型?</mark>
   因为需要`Append()`到当前`GenericPointer`对象,所以要构建一个`Token`结构体,但是结构体中的`name`是`Ch`类型,所以要`Ch`的字节类型,若是单字节,则可以直接将`char* buffer`转换为`Ch* name`;如果是多字节,就需要一个字节一个字节的将`buffer[i]`转换为`name[i]`
13. <sapn style="color:red;">`GenericPointer`的`JSON Pointer`有两种表示方式:字符串表示方式、`URI`片段表示方式.下图中两种表示方式都对应的同一种`Pointer token`</span>
    ![](markdown图像集/2024-12-12-22-16-34.png)
    从图中可以看出,`URI`片段表示中第一个字符一定是`#`,对于属于`URI`非安全的字符会使用`%+十六进制`码的方式进行表示
14. <mark>`GenericPointer::Create()`:根据当前`Pointer`对象中的`tokens_`,动态地在`root`(一般是一个`Document`对象,即`DOM`树)中创建或查找相应的节点值,如果查找不到,就会在`tokens_`表示的路径处对应的`JSON`文档处创建一个空节点(`ValueType()`)</mark>
15. 在`Create()`中,`-`负号表示数组最后元素的下一个元素,即相当于要把此时传入的`value`给`PushBack`进这个数组中
   ```C++
   Document d;
   d.Parse("{\"foo\":[123]}");
   SetValueByPointer(d, "/foo/-", 456);// 等价于Value::PushBack()  { "foo" : [123, 456] }
   ```
16. `GetUri()`它是按照当前`Pointer`对象的`tokens_`找`uri`,并和给定的`rootUri`合成在一起.在这个合成的过程中,遵循的是:按照这个`tokens_`路径查找的时候,是逐层找是否有`id`字段,然后会把每一层的`id`字段的`uri`值都合成到当前的`uri`(从`rootUri`开始,在`GetUri()`中是`base`)后面,即一进来它会先检查传入的`root`这个`DOM`树根节点是否有`id`这个成员,如果有,就把它的`uri`值合成到`rootUri`后面,然后往下一层走(注意`GetUri()`不是只将通过`tokens_`路径找到的最后一个`id`对应的`uri`,而是将每一层路径都合成到当前`rooturi`中)
17. <mark>`Uri`只会存在于`JSON`对象的键值对中,即等于`id`这个键对应的值中,因此`GetUri()`只会在`kObjectType`中处理(`Resolve()`)`uri`.如:`UriType finalUri = Pointer("/article/details").GetUri(doc, rootUri);`:表示的是从当前`JSON`文档`doc`中按`["article","details"]`路径进行合成`uri`,即首先会找`doc`这个根节点中是否有`id`成员,如果有,就先把最外面的`uri`合成到`rootUri`中;然后,再找`doc["article"]`对应的`GenericValue`对象是否是一个`JSON`对象且是否有`id`成员,如果有,就把这一层的`uri`合成到当前的`rootUri`中;最后,再找`doc["article"]["details"]`对应的`GenericValue`对象是否是一个`JSON`对象且是否有`id`成员,如果有,就把这一层的`uri`合成到当前的`rootUri`中</mark>
18. <mark>明明`uri`信息只会存在于`JSON`对象的成员中,那么`GetUri()`为什么要处理数组类型?</mark>
   如果不处理数组,解析类似"/person/friends/0"(要找`doc["person"]["friends"][0]`路径上的`id`对应的`uri`,如果不处理数组索引,就处理不了`tokens_`中表示的数组索引)这样的路径时,函数将无法正确地处理数组索引,导致解析失败.通过数组的处理,`GetUri()`可以在路径中遇到数组时正确地解析出索引对应的值
19. `Get()`:根据当前`Pointer`对象中的`tokens_`返回要找的节点,如果某个`token`路径片段找不到就会返回`nullptr`
20. `CopyFrom()`:该函数实现了从另一个`GenericPointer (rhs)`复制所有的 `token`和名称`token.name`到当前对象.它确保了每个`token`的`name`成员指向新的`nameBuffer_`中相应的名称,同时分配了足够的内存来处理额外的`token`(`extraToken`)和名称空间.最后返回的是新的`nameBuffer_`的结束位置,这通常用于方便后续的操作或内存管理
21. `std::ptrdiff_t`:这是`C++`标准库中定义的整数类型,用于表示指针之间的差值(即两个指针相减的结果),通常用来表示内存中的地址偏移量
   ```C++
   typedef long ptrdiff_t; // 在某些平台上，可能是 long 类型
   ```
22. `NeedPercentEncode()`:确定字符`c`是否属于`URI`中不需要编码的安全字符集(`'0'-'9'、'a'-'z'、'A'-'Z'、'-'、'.'、'_'、'~'`为安全字符集).安全字符集不用`URL`编码(也叫百分号编码)(`%`+`16进制`码值)
23. `Parse()`:解析一个源字符串`source`,并将结果保存在`tokens_`和`nameBuffer_`中,`source`可能是普通的字符串表示,也可能是`URI`片段表示方式.从`Parse()`中可知:
    * 字符串表示和`URI`片段表示第一个字符一定是`'/'`
    * 以`#`为开头,表示传入的这个`source`是`URI`片段表示形式的字符串
    * 对于`URI`片段表示中,需要对`%16进制`(`%XY`)进行百分号解码,解码后写入到`token`对应的`name`中
    * `URI`片段表示的转义字符:`'~0' -> '~', '~1' -> '/'`
24. `goto error`:这是`C/C++`中用于跳转到指定标签的一种控制流语句.它被用来在发生错误时跳转到特定的错误处理区域,这样做可以简化错误处理
25. `PercentEncodeStream()`:将传入的字符流中的字符,按照`URL`编码方式转换成`%XY`格式的编码,其中`X`和`Y`是十六进制数.`X`为高四位(一个`F`对应四个二进制位`1111`),`Y`为低四位
26. `PercentDecodeStream()`:由`URL`编码得到的解码后的字符串序列
# uri.h
1. `URI`是一个用于标识字眼的字符串,`URI`分为不同的部分:`scheme://user:password@host:port/path?query#fragment`.`scheme`指定如何访问资源的协议或方法(如`http`等);`user info`指定访问资源时的用户名和密码;`host`指定资源所在的服务器或计算机的名称或`IP`地址;`port`指定与主机连接的端口号;`path`指定资源再服务器上的位置;`query`用于传递额外的信息参数,通常用来标识资源的某个特定子集,查询字符串以`?`开头,并由`&`分隔多个键值对;`fragment`指定资源的一部分,通常用来跳转到某个页面或文档中的特定位置,以`#`开头
   ![](markdown图像集/2024-12-17-21-40-13.png)
2. 对应的在`GeneriUri`中:`uri_`存储完整的`uri`字符串,包含了`uri`所有组成部分;`base_`存储`uri`去掉`fragment`部分的内容;`scheme_`存储`uri`中的`scheme`部分;`auth_`存储`uri`中用户信息部分;`path_`存储`uri`中的路径部分;`query_`存储`uri`中查询字符串部分;`frag_`存储`uri`中的片段标识符`fragment`部分
   ![](markdown图像集/2024-12-17-21-40-34.png)
3. `Resolve()`:解析一个相对`URI`(当前`URI`对象)相对于一个基准`URI`(`baseuri`)的绝对`URI`.它会根据`URI`组成部分(`shceme、auth、path、query、frag`)来拼接、修改并返回解析后的`URI`.如:
   ```C++
   1. 
   baseuri = "http://example.com/path/to/resource"
   relativeuri = "d/e/f"// 此时没有指定scheme、auth、query、frag部分,只有path且为相对路径部分"d/e/f"
   =>
   http://example.com/path/to/resource/d/e/f
   2. 
   baseuri = "http://example.com/path/to/resource"
   relativeuri = "https://anotherexample.com/other/path"// 此时指定了scheme部分("https"),auth部分("anotherexample.com"),path部分("other/path").无query、frag部分
   =>
   // 此时因为当前URI就有scheme部分,所以直接使用当前URI对象的uri各个部分,不会拼接baseuri,此时根本就不会使用baseuri的各个部分
   https://anotherexample.com/other/path
   // 此时会执行Resolve()函数的第一个if()然后直接调到276行resuri.base_ = CopyPart(resuri.frag_, frag_, GetFragStringLength());
   ```
   <span style="color:red;">总的来说,`Resolve()`处理的拼接是只有`path`部分,其余部分不会拼接.具体来说:
   * `cururi`有`scheme`部分,此时`Resolve()`返回的`URI`对象就是`cururi`
   * `cururi`无`scheme`部分有`auth`部分,此时`Resolve()`返回的`URI`对象就是`baseuri`的`scheme`部分加`cururi`的所有部分
   * `cururi`无`scheme`、`auth`部分有`path`,此时`Resolve()`返回的`URI`对象就是`baseuri`的`scheme`、`auth`部分加`baseuri`的`path`和`cururi`的`path`拼接后的部分
   * `cururi`无`scheme`、`auth`、`path`部分有/无`query`,此时`Resolve()`返回的`URI`对象就是`baseuri`的`scheme`、`auth`、`path`部分加`cururi`的`query`/`baseuri`的`query`部分
   * `frag`部分只有`cururi`中有,`baseuri`不考虑</span>
4. <mark>`Allocate()`中为什么分配内存`size_t total = (3*len+7)*sizeof(Ch);`?</mark>
   理论上,`URI`的每个部分的长度可能接近整个`URI`的长度,因此你可能需要分配大约3倍的空间,以保证可以容纳所有的部分.不乘以6是因为可能会导致过度分配内存,乘3是一个合理的估算.加7的目的:这是为了保证内存对齐,并为不同的`URI`部分之间留出足够的空间.具体来说,7是为了避免由于内存对齐规则(通常是8字节对齐)而产生的浪费或错位
5. `Parse()`:将输入的`URI`字符串拆解成不同的组成部分,如`scheme_`、`auth_`、`path_`、`query_`和`frag_`,并存储到相应的成员变量中
6. `RemoveDotSegments()`:用于处理和规范化`URI`对象的路径部分中的`'.'`(当前目录)和`'..'`(上一目录);如果路径段是`'..'`,则表示上一目录,需要回退到上一个目录;如果路径段是`'.'`,表示当前目录,不需要做任何处理,直接跳过该段   
## 测试结果
   ![](markdown图像集/2024-12-22-11-22-16.png)
# 面试项目介绍
1. Rapidjson是一个由腾讯开源的高性能C++ JSON解析器及生成器
2. 技术特点:
   * 高性能：Rapidjson的性能表现非常出色，其解析速度可与strlen()相比。它通过使用模版及内联函数来降低函数调用开销，并且支持SSE2/SSE4.2加速
   * 丰富的API风格：Rapidjson同时支持SAX和DOM风格的AP
   * Unicode支持：Rapidjson对Unicode有良好的支持，包括UTF-8、UTF-16、UTF-32编码
   * 内存优化：Rapidjson在内存使用上进行了优化，自定义内存分配器
   

