#ifndef RAPIDJSON_STREAM_H_
#define RAPIDJSON_STREAM_H_

#include "rapidjson.h"
#include "encodings.h"
namespace RAPIDJSON{
    ///////////////////////////////////////////////////////////////////////////////
    //  Stream

    /*! \class rapidjson::Stream
        \brief Concept for reading and writing characters.

        For read-only stream, no need to implement PutBegin(), Put(), Flush() and PutEnd().

        For write-only stream, only need to implement Put() and Flush().

    \code
    concept Stream {
        typename Ch;    //!< Character type of the stream.

        //! Read the current character from stream without moving the read cursor.
        Ch Peek() const;

        //! Read the current character from stream and moving the read cursor to next character.
        Ch Take();

        //! Get the current read cursor.
        //! \return Number of characters read from start.
        size_t Tell();

        //! Begin writing operation at the current read pointer.
        //! \return The begin writer pointer.
        Ch* PutBegin();

        //! Write a character.
        void Put(Ch c);

        //! Flush the buffer.
        void Flush();

        //! End the writing operation.
        //! \param begin The begin write pointer returned by PutBegin().
        //! \return Number of characters written.
        size_t PutEnd(Ch* begin);
    }
    \endcode
    */
    template<typename Stream>
    struct StreamTraits{
        enum {copyOptimization = 0;}// 不启用复制优化  不进行本地副本的创建
    };
    /**
     * @brief 如果需要,则为写操作保留count个字符的空间
     * 默认情况不分配  要分配的话需要修改函数的实现
     * @tparam Stream 
     * @param stream 
     * @param count 
     */
    template<typename Stream>
    inline void PutReserve(Stream& stream, size_t count){
        (void) stream;
        (void) count;
    }
    /**
     * @brief 无需额外检查  直接将字符写入流
     * 
     * @tparam Stream 
     * @param stream 
     * @param c 
     */
    template<typename Stream>
    inline void PutUnsafe(Stream& stream, typename Stream::Ch c){
        stream.Put(c);
    }
    /**
     * @brief 将n个字符c写入流
     * 
     * @tparam Stream 
     * @tparam Ch 
     * @param stream 
     * @param c 
     * @param n 
     */
    template<typename Stream, typename Ch>
    inline void PutN(Stream& stream, Ch c, size_t n){
        PutReserve(stream, n);
        for(size_t i=0;i<n;i++)
            PutUnsafe(stream, c);
    }
    /**
     * @brief 用于包装任意输入流的包装类   这是一个完全抽象的上层接口层
     * 
     * @tparam InputStream 
     * @tparam Encoding 
     */
    template<typename InputStream, typename Encoding=UTF8<>>
    class GenericStreamWrapper{
        public:
            typedef typename Encoding::Ch Ch;
            GenericStreamWrapper(InputStream& is) : is_(is) {}
            Ch Peek() const {return is_.Peen();}
            Ch Take() {return is_.Take();}
            size_t Tell() {return is_.Tell();}
            Ch PutBegin() {return is_.PutBegin();}
            void Put(Ch ch) {is_.Put(ch);}
            void Flush() {is_.Flush();}
            size_t PutEnd(Ch* ch) {return is_.PutEnd(ch);}
            const Ch* Peek4() const {return is_.Peek4();}
            UTFType GetType() const {return is_.GetType();}
            bool HasBOM() const {return is_.HasBOM();}// AutoUTFInputStream的包装函数 - 检查是否存在 BOM (字节顺序标记)
        protected:
            InputStream& is_;
    };
    /**
     * @brief 用于内存中字符串的只读字符串流  从内存缓冲区读取数据  memorystream.h的不同字符编码扩展实现
     * 
     * @tparam Encoding 
     */
    template<typename Encoding>
    struct GenericStringStream{
        typedef typename Encoding::Ch Ch;
        GenericStringStream(const Ch* src) 
            : src_(src),
              head_(src)
            {}
        Ch Peek() const {return *src_;}
        Ch Take() {return *src_++;}
        size_t Tell() const {return static_cast<size_t>(src_ - head_);}
        Ch* PutBegin() {RAPIDJSON_ASSERT(false); return 0;}
        void Put(Ch) {RAPIDJSON_ASSERT(false);}
        void Flush() {RAPIDJSON_ASSERT(false);}
        size_t PutEnd(Ch*) {RAPIDJSON_ASSERT(false); return 0;}
        const Ch* src_;
        const Ch* head_;
    };
    /**
     * @brief 专门化StreamTraits  以启用GenericStringStream的复制优化
     * 
     * @tparam Encoding 
     */
    template<typename Encoding>
    struct StreamTraits<GenericStringStream<Encoding>>{
        enum {copyOptimization = 1;}// 启用复制优化
    };
    typedef GenericStringStream<UTF8<>> StringStream;
    /**
     * @brief 就地解析设计的读写字符串流
     * 该流允许从字符串中读取和写入相同的字符串.它特别适用于就地解析器,在解析过程中可以修改输入数据
     * 
     * @tparam Encoding 
     */
    template<typename Encoding>
    struct GenericInsituStringStream{
        typedef typename Encoding::Ch Ch;
        /**
         * @brief 接受一个字符串缓冲区用于就地读写操作
         * 
         * @param src 
         */
        GenericInsituStringStream(Ch* src)
            : src_(src),
              dst_(0),
              head_(src)
            {}
        Ch Peek() {return *src_;}
        Ch Take() {return *src_++;}
        size_t Tell() {return static_cast<size_t>(src_ - head_);}
        void Put(Ch c) {RAPIDJSON_ASSERT(dst_!=0); *dst_++=c;}// 需要先PutBegin(),将写位置移到当前位置,而不是0  这样就能在当前这个src_读取位置也开始写了,即实现了就地写 
        Ch* PutBegin() {return dst_ = src_;}
        size_t PutEnd(Ch* begin) {return static_cast<size_t>(dst_ - begin);}
        void Flush() {}
        /**
         * @brief 推入count个字符,返回指向开头的指针
         * 
         * @param count 
         * @return Ch* 
         */
        Ch* Push(size_t count) {
            Ch* begin = dst_;
            dst_ += count;
            return begin;
        }
        /**
         * @brief 弹出count个字节  即撤销push操作
         * 
         * @param count 
         */
        void Pop(size_t count) {dst_ -= count;}
        Ch* src_;// 读取位置
        Ch* dst_;// 写入位置
        Ch* head_;// 起始位置
    };
    template<typename Encoding>
    struct StreamTraits<GenericInsituStringStream<Encoding>>{
        enum {copyOptimization = 1};
    };
    typedef GenericInsituStringStream<UTF8<>> InsituStringStream;
}
#endif