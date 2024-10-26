#ifndef RAPIDJSON_ALLOCATORS_H
#define RAPIDJSON_ALLOCATORS_H


#include "rapidjson.h"
#include "internal/meta.h"
#include <memory>// STL
#include <limits>// STL
#include <type_traits>// C++11标准下包含此头文件,它提供了一些模板元编程工具

namespace RAPIDJSON{
    #ifndef RAPIDJSON_ALLOCATOR_DEFAULT_CHUNK_CAPACITY
    #define RAPIDJSON_ALLOCATOR_DEFAULT_CHUNK_CAPACITY (64 * 1024)// 默认定义的内存块容量为64KB
    #endif
    /**
     * @brief 基于C标准库malloc和free函数实现的一个内存块分配器
     * 
     */
    class CrtAllocator {
    public:
        static const bool kNeedFree = true;
        /**
         * @brief Malloc方法用于分配内存.如果size不为0,则使用RAPIDJSON_MALLOC来分配指定大小的内存
         * 
         * @param size 
         * @return void* 
         */
        void* Malloc(size_t size){ 
            if (size)
                return RAPIDJSON_MALLOC(size);
            else
                return nullptr; // standardize to returning nullptr.
        }
        /**
         * @brief 重新分配内存.如果新分配的内存大小为0,则只需要释放原内存即可
         * 
         * @param originalPtr 
         * @param originalSize 
         * @param newSize 
         * @return void* 
         */
        void* Realloc(void* originalPtr, size_t originalSize, size_t newSize) {
            (void)originalSize;// 解除编译器警告
            if (newSize == 0) {
                RAPIDJSON_FREE(originalPtr);
                return nullptr;
            }
            return RAPIDJSON_REALLOC(originalPtr, newSize);
        }
        /**
         * @brief 调用RAPIDJSON_FREE()释放指定内存
         * 
         * @param ptr 
         */
        static void Free(void *ptr) RAPIDJSON_NOEXCEPT {RAPIDJSON_FREE(ptr);}// RAPIDJSON_NOEXCEPT<=>C++11的noexcept  标记该函数不抛异常
        // 重载==和!=
        bool operator==(const CrtAllocator&) const RAPIDJSON_NOEXCEPT {
            return true;
        }
        bool operator!=(const CrtAllocator&) const RAPIDJSON_NOEXCEPT {
            return false;
        }
    };
    /**
     * @brief 实现了一个内存池分配器,说白了就是为了更高效的分配内存块.默认使用CrtAllocator作为底层内存块分配器
     * 此内存池分配器用于高效地进行小块内存分配(预先分配较大的内存块,然后在这些内存块中按需分配小对象),避免频繁调用操作系统的内存分配函数,并且可以提升分配和释放操作的性能
     * 
     * @tparam BaseAllocator 
     */
    template <typename BaseAllocator = CrtAllocator>
    class MemoryPoolAllocator {// 内存块的头部结构
        struct ChunkHeader {
            size_t capacity;    // 内存块容量
            size_t size;        // 内存块已经用的大小
            ChunkHeader *next;  // 指向下一个内存块的指针
        };
        struct SharedData {// 共享数据结果,用于在多个内存池分配器实例之间共享内存块信息
            ChunkHeader *chunkHead;  // 指向当前内存块链表的头部(节点) 通过这个指针,分配器可用轻松地遍历所有的内存块,以便于分配和释放内存
            BaseAllocator* ownBaseAllocator; // 如果当前对象传入了新的BaseAllocator,那么该指针指向它;如果使用默认的C内存分配器,就指向nullptr
            size_t refcount;// 引用计数,用于记录有多少内存池分配器实例共享这个内存池
            bool ownBuffer;// 指示当前内存块是否由用户自己提供,或者是由MemoryPoolAllocator自己分配的
        };
        // RAPIDJSON_ALIGN将结构体的大小调整为系统要求的对齐单位(通常是4字节或8字节),这意味着SharedData和ChunkHeader的大小可能会比它们实际的结构体大小略大
        static const size_t SIZEOF_SHARED_DATA = RAPIDJSON_ALIGN(sizeof(SharedData));// SharedData结构体的对齐后大小     SIZEOF_SHARED_DATA表示SharedData的对齐大小,指针加上这个偏移量后,可以跳过SharedData的空间,指向SharedData之后的内存区域
        static const size_t SIZEOF_CHUNK_HEADER = RAPIDJSON_ALIGN(sizeof(ChunkHeader));// ChunkHeader结构体的对齐后大小
        // 获取SharedData中的chunkHead的起始地址
        static inline ChunkHeader *GetChunkHead(SharedData *shared)
        {
            return reinterpret_cast<ChunkHeader*>(reinterpret_cast<uint8_t*>(shared) + SIZEOF_SHARED_DATA);
        }
        // 获取SharedData中的内存块数据区起始地址
        static inline uint8_t *GetChunkBuffer(SharedData *shared)
        {
            return reinterpret_cast<uint8_t*>(shared->chunkHead) + SIZEOF_CHUNK_HEADER;
        }
        static const size_t kDefaultChunkCapacity = RAPIDJSON_ALLOCATOR_DEFAULT_CHUNK_CAPACITY; // 默认的内存块容量,64KB
    public:
        static const bool kNeedFree = false;    // 指示用户是否需要显示调用Free()来是否内存.对于MemoryPoolAllocator对象来说,kNeedFree=false,表示不需要用户手动释放内存,此分配器会自动管理内存释放
        static const bool kRefCounted = true;   // 指示内存池分配器是否支持引用计数
        /**
         * @brief 默认的构造函数.
         * 分配一个新的SharedData,并为其分配第一个内存块
         * 
         * @param chunkSize 
         * @param baseAllocator 
         */
        explicit
        MemoryPoolAllocator(size_t chunkSize = kDefaultChunkCapacity, BaseAllocator* baseAllocator = nullptr) : 
            chunk_capacity_(chunkSize),
            baseAllocator_(baseAllocator ? baseAllocator : RAPIDJSON_NEW(BaseAllocator)()),
            shared_(static_cast<SharedData*>(baseAllocator_ ? baseAllocator_->Malloc(SIZEOF_SHARED_DATA + SIZEOF_CHUNK_HEADER) : 0))
        {
            RAPIDJSON_ASSERT(baseAllocator_ != 0);
            RAPIDJSON_ASSERT(shared_ != 0);
            if (baseAllocator) {
                shared_->ownBaseAllocator = 0;
            }
            else {
                shared_->ownBaseAllocator = baseAllocator_;
            }
            shared_->chunkHead = GetChunkHead(shared_);
            shared_->chunkHead->capacity = 0;
            shared_->chunkHead->size = 0;
            shared_->chunkHead->next = 0;
            shared_->ownBuffer = true;
            shared_->refcount = 1;
        }
        /**
         * @brief 用户提供缓冲区的构造函数
         * 使用用户提供的内存缓冲区来初始化内存块,当该缓冲区用完时,内存池将使用指定大小分配新的内存块
         * @param buffer 用户提供的内存缓冲区
         * @param size 缓冲区大小
         * @param chunkSize 指定新的内存块大小,当用户提供的缓冲区用完时,使用该大小创建新的内存块
         * @param baseAllocator 基础分配器
         */
        MemoryPoolAllocator(void *buffer, size_t size, size_t chunkSize = kDefaultChunkCapacity, BaseAllocator* baseAllocator = 0) :
            chunk_capacity_(chunkSize),
            baseAllocator_(baseAllocator),
            shared_(static_cast<SharedData*>(AlignBuffer(buffer, size)))
        {
            RAPIDJSON_ASSERT(size >= SIZEOF_SHARED_DATA + SIZEOF_CHUNK_HEADER);
            shared_->chunkHead = GetChunkHead(shared_);
            shared_->chunkHead->capacity = size - SIZEOF_SHARED_DATA - SIZEOF_CHUNK_HEADER;
            shared_->chunkHead->size = 0;
            shared_->chunkHead->next = 0;
            shared_->ownBaseAllocator = 0;
            shared_->ownBuffer = false;
            shared_->refcount = 1;
        }
        // 使用已有的MemoryPoolAllocator实例rhs进行拷贝构造
        MemoryPoolAllocator(const MemoryPoolAllocator& rhs) RAPIDJSON_NOEXCEPT :
            chunk_capacity_(rhs.chunk_capacity_),
            baseAllocator_(rhs.baseAllocator_),
            shared_(rhs.shared_)
        {
            RAPIDJSON_NOEXCEPT_ASSERT(shared_->refcount > 0);
            ++shared_->refcount;// 拷贝构造 引用计数+1
        }
        MemoryPoolAllocator& operator=(const MemoryPoolAllocator& rhs) RAPIDJSON_NOEXCEPT
        {
            RAPIDJSON_NOEXCEPT_ASSERT(rhs.shared_->refcount > 0);
            ++rhs.shared_->refcount;
            this->~MemoryPoolAllocator();// 赋值就会把旧实例给释放
            baseAllocator_ = rhs.baseAllocator_;
            chunk_capacity_ = rhs.chunk_capacity_;
            shared_ = rhs.shared_;
            return *this;
        }
        // C++11允许的移动构造
        MemoryPoolAllocator(MemoryPoolAllocator&& rhs) RAPIDJSON_NOEXCEPT :
            chunk_capacity_(rhs.chunk_capacity_),
            baseAllocator_(rhs.baseAllocator_),
            shared_(rhs.shared_)
        {
            RAPIDJSON_NOEXCEPT_ASSERT(rhs.shared_->refcount > 0);
            rhs.shared_ = 0;
        }
        MemoryPoolAllocator& operator=(MemoryPoolAllocator&& rhs) RAPIDJSON_NOEXCEPT
        {
            RAPIDJSON_NOEXCEPT_ASSERT(rhs.shared_->refcount > 0);
            this->~MemoryPoolAllocator();
            baseAllocator_ = rhs.baseAllocator_;
            chunk_capacity_ = rhs.chunk_capacity_;
            shared_ = rhs.shared_;
            rhs.shared_ = 0;
            return *this;
        }
        /**
         * @brief 释放MemoryPoolAllocator实例在使用过程中分配的资源
         * 
         */
        ~MemoryPoolAllocator() RAPIDJSON_NOEXCEPT {
            if (!shared_) 
                return;
            if (shared_->refcount > 1) {// 引用计数>1,则表示该内存池分配器实例被多个对象共享,因此只需将引用计数-1即可
                --shared_->refcount;
                return;
            }
            Clear();// 引用计数为1,说明没有其它对象共享内存池,则情况内存块并释放资源
            BaseAllocator *a = shared_->ownBaseAllocator;
            if (shared_->ownBuffer)// 如果内存块由分配自己管理,则释放shared_就行
                baseAllocator_->Free(shared_);
            RAPIDJSON_DELETE(a);
        }
        // 清理处理第一个或用户提供的内存块的其余所有内存块
        void Clear() RAPIDJSON_NOEXCEPT {
            RAPIDJSON_NOEXCEPT_ASSERT(shared_->refcount > 0);
            for (;;) {// 以链表的形式释放所有内存块(除了第一个)
                ChunkHeader* c = shared_->chunkHead;
                if (!c->next) 
                    break;
                shared_->chunkHead = c->next;
                baseAllocator_->Free(c);
            }
            shared_->chunkHead->size = 0;
        }
        /**
         * @brief 计算所有内存块中已分配的总容量
         * 
         * @return size_t 
         */
        size_t Capacity() const RAPIDJSON_NOEXCEPT {
            RAPIDJSON_NOEXCEPT_ASSERT(shared_->refcount > 0);
            size_t capacity = 0;
            for (ChunkHeader* c = shared_->chunkHead; c != 0; c = c->next)
                capacity += c->capacity;
            return capacity;
        }
        /**
         * @brief 计算所有内存块中已分配的内存大小 
         * 
         * @return size_t 
         */
        size_t Size() const RAPIDJSON_NOEXCEPT {
            RAPIDJSON_NOEXCEPT_ASSERT(shared_->refcount > 0);
            size_t size = 0;
            for (ChunkHeader* c = shared_->chunkHead; c != 0; c = c->next)
                size += c->size;
            return size;
        }
        /**
         * @brief 返回当前内存池是否被多个对象共享
         * 
         * @return true 
         * @return false 
         */
        bool Shared() const RAPIDJSON_NOEXCEPT {
            RAPIDJSON_NOEXCEPT_ASSERT(shared_->refcount > 0);
            return shared_->refcount > 1;
        }
        /**
         * @brief 分配指定大小的内存块
         * 
         * @param size 
         * @return void* 
         */
        void* Malloc(size_t size) {
            RAPIDJSON_NOEXCEPT_ASSERT(shared_->refcount > 0);
            if (!size)
                return nullptr;
            size = RAPIDJSON_ALIGN(size);// 对齐分配大小  内存分配前先对齐size,保证内存分配的边界正确
            if (RAPIDJSON_UNLIKELY(shared_->chunkHead->size + size > shared_->chunkHead->capacity))
                if (!AddChunk(chunk_capacity_ > size ? chunk_capacity_ : size))
                    return nullptr;
            void *buffer = GetChunkBuffer(shared_) + shared_->chunkHead->size;
            shared_->chunkHead->size += size;
            return buffer;
        }
        /**
         * @brief 重新分配已分配的内存块,调整其大小
         * 
         * @param originalPtr 
         * @param originalSize 
         * @param newSize 
         * @return void* 
         */
        void* Realloc(void* originalPtr, size_t originalSize, size_t newSize) {
            if (originalPtr == 0)// 原始内存没有分配,则直接分配新的内存块
                return Malloc(newSize);
            RAPIDJSON_NOEXCEPT_ASSERT(shared_->refcount > 0);
            if (newSize == 0)// 用户不需再分配内存
                return nullptr;
            // 对originalSize和newSize进行内存对齐.对齐操作是为了保证内存块地址和大小符合硬件对齐要求(如按 8 字节对齐).这有助于提升 CPU 的访问效率
            originalSize = RAPIDJSON_ALIGN(originalSize);
            newSize = RAPIDJSON_ALIGN(newSize);
            if (originalSize >= newSize)// 新的大小<=原始大小,则无需分配新内存,直接返回原指针即可
                return originalPtr;
            // 判断是否是内存池中当前最后一个分配的内存块
            if (originalPtr == GetChunkBuffer(shared_) + shared_->chunkHead->size - originalSize) {
                size_t increment = static_cast<size_t>(newSize - originalSize);
                if (shared_->chunkHead->size + increment <= shared_->chunkHead->capacity) {// 如果新增后的大小仍小于原容量 则直接增加当前内存块的大小,更新size即可
                    shared_->chunkHead->size += increment;
                    return originalPtr;
                }
            }
            // 如果不能按上面的扩展,那么只能分配新的内存块
            if (void* newBuffer = Malloc(newSize)) {
                if (originalSize)// 如果分配成功,会将原始内存中的内容拷贝到新分配的内存中
                    std::memcpy(newBuffer, originalPtr, originalSize);
                return newBuffer;
            }
            else
                return nullptr;
        }
        // MemoryPoolAllocator自动管理内存,因此此方法不执行任何操作
        static void Free(void *ptr) RAPIDJSON_NOEXCEPT { (void)ptr; }
        // 两个内存分配器相同的条件是它们共享相同的SharedData
        bool operator==(const MemoryPoolAllocator& rhs) const RAPIDJSON_NOEXCEPT {
            RAPIDJSON_NOEXCEPT_ASSERT(shared_->refcount > 0);
            RAPIDJSON_NOEXCEPT_ASSERT(rhs.shared_->refcount > 0);
            return shared_ == rhs.shared_;
        }
        bool operator!=(const MemoryPoolAllocator& rhs) const RAPIDJSON_NOEXCEPT {
            return !operator==(rhs);
        }
    private:
        /**
         * @brief 创建新的内存块,并将其添加到内存块链表中
         * 
         * @param capacity 
         * @return true 
         * @return false 
         */
        bool AddChunk(size_t capacity) {
            if (!baseAllocator_)
                shared_->ownBaseAllocator = baseAllocator_ = RAPIDJSON_NEW(BaseAllocator)();
            if (ChunkHeader* chunk = static_cast<ChunkHeader*>(baseAllocator_->Malloc(SIZEOF_CHUNK_HEADER + capacity))) {
                chunk->capacity = capacity;
                chunk->size = 0;
                chunk->next = shared_->chunkHead;
                shared_->chunkHead = chunk;
                return true;
            }
            else
                return false;
        }
        /**
         * @brief 对齐缓冲区的起始地址,并更新剩余可用大小,从而确保内存块的地址满足对齐要求
         * 
         * @param buf 
         * @param size 
         * @return void* 
         */
        static inline void* AlignBuffer(void* buf, size_t &size)
        {
            RAPIDJSON_NOEXCEPT_ASSERT(buf != 0);
            const uintptr_t mask = sizeof(void*) - 1;// 计算出对齐掩码,确保地址对齐到指针大小的倍数
            const uintptr_t ubuf = reinterpret_cast<uintptr_t>(buf);// 将 buf 转换为 uintptr_t 类型,便于计算
            if (RAPIDJSON_UNLIKELY(ubuf & mask)) {// 如果地址未对齐(即与mask进行与操作后结果不为0)
                const uintptr_t abuf = (ubuf + mask) & ~mask;// !!! 将地址向上(地址增大)对齐到最近的指针大小的倍数
                RAPIDJSON_ASSERT(size >= abuf - ubuf);
                buf = reinterpret_cast<void*>(abuf);// 返回对齐后的地址
                size -= abuf - ubuf;// 更新剩余的可用内存大小  原始剩余的可用大小-因为对齐而损失的内存大小
            }
            return buf;
        }
        size_t chunk_capacity_;// 指定内存块的大小,默认为16KB
        BaseAllocator* baseAllocator_;// 用于分配内存块的基础分配器,为空就是使用默认的BaseAllocator->CrtAllocator
        SharedData *shared_;// 指向共享的SharedData,包含内存块的链表头等信息
    };
    namespace internal{
        template<typename, typename = void>
        struct IsRefCounted : public FalseType// 通用模板   IsRefCounted默认继承自FalseType,表示类型T不支持引用计数
        { };
        template<typename T>
        struct IsRefCounted<T, typename internal::EnableIfCond<T::kRefCounted>::Type> : public TrueType//部分特化模板 这段代码通过模板特化和SFINAE机制
        { };
    }
    // 下面的Realloc Malloc Free是为了在直接指定内存分配器时提供统一的内存分配、重新分配和释放的接口,而不是作为类的方法出现   A& a是指定的基础分配器的实例,如A=MeoryAllocator
    template<typename T, typename A>
    inline T* Realloc(A& a, T* old_p, size_t old_n, size_t new_n)
    {
        RAPIDJSON_NOEXCEPT_ASSERT(old_n <= (std::numeric_limits<size_t>::max)() / sizeof(T) && new_n <= (std::numeric_limits<size_t>::max)() / sizeof(T));
        return static_cast<T*>(a.Realloc(old_p, old_n * sizeof(T), new_n * sizeof(T)));
    }
    template<typename T, typename A>
    inline T *Malloc(A& a, size_t n = 1)
    {
        return Realloc<T, A>(a, NULL, 0, n);// old_n=0->malloc
    }
    template<typename T, typename A>
    inline void Free(A& a, T *p, size_t n = 1)
    {
        static_cast<void>(Realloc<T, A>(a, p, n, 0));// new_n=0->free
    }
    // 自定义一个内存分配器 继承自std::allocator<T> 它通过组合一个基础分配器 BaseAllocator(默认为CrtAllocator),实现了灵活且高效的内存管理机制
    template <typename T, typename BaseAllocator = CrtAllocator>// T:内存分配所管理的对象类型
    class StdAllocator : public std::allocator<T>// StdAllocator继承自std::allocator<T>,这使得它可以无缝地与STL容器配合使用
    {
        typedef std::allocator<T> allocator_type;
        #if RAPIDJSON_HAS_CXX11
            typedef std::allocator_traits<allocator_type> traits_type;// 这是C++11引入的特性,用于提供对分配器的通用接口  std::allocator<T>作为这个工具类的底层内存分配器
        #else
            typedef allocator_type traits_type;
        #endif
    public:
        typedef BaseAllocator BaseAllocatorType;
        StdAllocator() RAPIDJSON_NOEXCEPT :// 默认构造函数
            allocator_type(),// 调用std::allocator<T>默认构造函数
            baseAllocator_()
        { }
        StdAllocator(const StdAllocator& rhs) RAPIDJSON_NOEXCEPT :// 拷贝构造函数
            allocator_type(rhs),
            baseAllocator_(rhs.baseAllocator_)
        { }
        template<typename U>
        StdAllocator(const StdAllocator<U, BaseAllocator>& rhs) RAPIDJSON_NOEXCEPT :// 模板拷贝构造函数,允许从不同类型的拷贝
            allocator_type(rhs),
            baseAllocator_(rhs.baseAllocator_)
        { }
        #if RAPIDJSON_HAS_CXX11_RVALUE_REFS
            StdAllocator(StdAllocator&& rhs) RAPIDJSON_NOEXCEPT :// 移动构造函数
                allocator_type(std::move(rhs)),
                baseAllocator_(std::move(rhs.baseAllocator_))
            { }
        #endif
        #if RAPIDJSON_HAS_CXX11
        // 面两个类型特性指示分配器在容器移动赋值和交换时是否需要传播其状态,std::true_type表示需要传播
        using propagate_on_container_move_assignment = std::true_type;
        using propagate_on_container_swap = std::true_type;
        #endif
        StdAllocator(const BaseAllocator& baseAllocator) RAPIDJSON_NOEXCEPT :// 允许使用BaseAllocator实例来构造StdAllocator,即可以自定义基础分配器
            allocator_type(),
            baseAllocator_(baseAllocator)
        { }
        ~StdAllocator() RAPIDJSON_NOEXCEPT
        { }
        template<typename U>
        struct rebind {// 允许分配器重新绑定到不同类型
            typedef StdAllocator<U, BaseAllocator> other;
        };
        // std::allocator_traits的常用成员
        typedef typename traits_type::size_type         size_type;// 分配器的大小类型
        typedef typename traits_type::difference_type   difference_type;// 指针差类型
        typedef typename traits_type::value_type        value_type;// 分配器管理的对象类型
        typedef typename traits_type::pointer           pointer;// 对象的指针类型
        typedef typename traits_type::const_pointer     const_pointer;// 对象的引用类型
        #if RAPIDJSON_HAS_CXX11
        typedef typename std::add_lvalue_reference<value_type>::type &reference;
        typedef typename std::add_lvalue_reference<typename std::add_const<value_type>::type>::type &const_reference;
        pointer address(reference r) const RAPIDJSON_NOEXCEPT// 获取对象的地址
        {
            return std::addressof(r);
        }
        const_pointer address(const_reference r) const RAPIDJSON_NOEXCEPT
        {
            return std::addressof(r);
        }
        // 返回分配器能够分配的最大对象数量 通过traits_type的max_size来计算
        size_type max_size() const RAPIDJSON_NOEXCEPT
        {
            return traits_type::max_size(*this);
        }
        // 在分配好的内存空间p上构造对象,使用传入的参数args
        template <typename ...Args>
        void construct(pointer p, Args&&... args)
        {
            traits_type::construct(*this, p, std::forward<Args>(args)...);
        }
        // 调用对象p的析构函数,销毁对象
        void destroy(pointer p)
        {
            traits_type::destroy(*this, p);
        }
        #endif
        // 使用BaseAllocator的Malloc方法为类型U分配n个对象的内存  而不是使用traits_type::allocate  traits_type::deallocate  
        template <typename U>
        U* allocate(size_type n = 1, const void* = 0)
        {
            return RAPIDJSON::Malloc<U>(baseAllocator_, n);
        }
        // 使用BaseAllocator的Free方法释放类型U的n个对象的内存
        template <typename U>
        void deallocate(U* p, size_type n = 1)
        {
            RAPIDJSON::Free<U>(baseAllocator_, p, n);
        }
        // 为当前类型T分配内存,等效于前面的allocate<value_type>(n)
        pointer allocate(size_type n = 1, const void* = 0)
        {
            return allocate<value_type>(n);
        }
        // 等效前面的deallocate<value_type>(n)
        void deallocate(pointer p, size_type n = 1)
        {
            deallocate<value_type>(p, n);
        }
        #if RAPIDJSON_HAS_CXX11
            using is_always_equal = std::is_empty<BaseAllocator>;// 指示分配是否总是相等. BaseAllocator是空类(即不包含任何成员变量),则StdAllocator的所有实例总是相等
        #endif
        // 根据判断BaseAllocator是否相等来判断StdAllocator实例是否相等
        template<typename U>
        bool operator==(const StdAllocator<U, BaseAllocator>& rhs) const RAPIDJSON_NOEXCEPT
        {
            return baseAllocator_ == rhs.baseAllocator_;
        }
        template<typename U>
        bool operator!=(const StdAllocator<U, BaseAllocator>& rhs) const RAPIDJSON_NOEXCEPT
        {
            return !operator==(rhs);
        }
        static const bool kNeedFree = BaseAllocator::kNeedFree;// 指示分配器是否需要调用Free方法释放内存
        static const bool kRefCounted = internal::IsRefCounted<BaseAllocator>::Value;// 指示分配器是否支持引用计数  这个Value是IsRefCounted的父类BoolType中的value,这一点就是模仿std::is_integral中的::value
        // 定义StdAllocator中的Malloc Realloc Free
        void* Malloc(size_t size)
        {
            return baseAllocator_.Malloc(size);
        }
        void* Realloc(void* originalPtr, size_t originalSize, size_t newSize)
        {
            return baseAllocator_.Realloc(originalPtr, originalSize, newSize);
        }
        static void Free(void *ptr) RAPIDJSON_NOEXCEPT
        {
            BaseAllocator::Free(ptr);
        }
        private:
            template <typename, typename>
            friend class StdAllocator; // 友元.允许不同类型参数的StdAllocator实例访问当前实例的私有成员
            BaseAllocator baseAllocator_;// 基础分配器
    };
    // 由于在C++17之前std::allocator<void>是合法的(尽管void不能直接分配内存),但这里为了保持某些项目中可能使用了这个旧标准,所以定义了一个支持std::allocator<void>的StdAllocator
    #if !RAPIDJSON_HAS_CXX17
    template <typename BaseAllocator>
    class StdAllocator<void, BaseAllocator> : public std::allocator<void>
    {
        typedef std::allocator<void> allocator_type;

    public:
        typedef BaseAllocator BaseAllocatorType;
        StdAllocator() RAPIDJSON_NOEXCEPT :
            allocator_type(),
            baseAllocator_()
        { }
        StdAllocator(const StdAllocator& rhs) RAPIDJSON_NOEXCEPT :
            allocator_type(rhs),
            baseAllocator_(rhs.baseAllocator_)
        { }
        template<typename U>
        StdAllocator(const StdAllocator<U, BaseAllocator>& rhs) RAPIDJSON_NOEXCEPT :
            allocator_type(rhs),
            baseAllocator_(rhs.baseAllocator_)
        { }
        StdAllocator(const BaseAllocator& baseAllocator) RAPIDJSON_NOEXCEPT :
            allocator_type(),
            baseAllocator_(baseAllocator)
        { }
        ~StdAllocator() RAPIDJSON_NOEXCEPT
        { }
        // 为什么这个StdAllocator用于void类型也要定义rebind?
        // STL容器和算法期望所有分配器都实现rebind,以便可以灵活地为不同类型分配内存.这是C++分配器接口的一部分,即使是特化为void类型的分配器,也必须遵循这一接口要求
        // 在很多实际场景中,虽然分配器可能一开始被定义为void类型,但在运行时,我们需要为具体类型的对象(如 int 或 double)分配内存.通过rebind, StdAllocator<void, BaseAllocator>可以动态生成适用于其他类型的分配器
        template<typename U>
        struct rebind {//  
            typedef StdAllocator<U, BaseAllocator> other;
        };
        typedef typename allocator_type::value_type value_type;
    private:
        template <typename, typename>
        friend class StdAllocator;
        BaseAllocator baseAllocator_;
    };
    #endif
}
#endif
