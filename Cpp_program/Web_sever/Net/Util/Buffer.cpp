#include "Buffer.h"
#include "../../Base/Logging/Logging.h"
#include <sys/uio.h>
#include <cassert>
#include <algorithm>
#include <cstring>

using namespace tiny_muduo;

Buffer::Buffer(int initialSize)
    : buffer_(kCheapPrepend+kInitialSize),// 此时会直接填充kCheapPrepend+kInitialSize个'\0'
      readIndex_(kCheapPrepend),
      writeIndex_(kCheapPrepend){
        assert(readablebytes()==0);
        assert(writeablebytes()==initialSize);
        assert(prependablebytes()==kCheapPrepend);
      }
// 从文件描述符读取数据到缓冲区(其实就是往Buffer中写)
int Buffer::ReadFd(int fd){
    char extrabuf[65536] = {0};// 临时缓冲区
    struct iovec iv[2];// 用于分散/聚集IO操作的结构体
    const int writeable = writeablebytes();// 获取可写字节数
    // 设置iovec数组,用于readv操作   iv其实有两个缓冲区   对于iv[0],其实它就指向了Buffer中可写缓冲区块,而iv[1]是为了数据太长写不下,而设计的
    iv[0].iov_base = beginWrite();// 第一个iovec指向缓冲区的可写部分
    iv[0].iov_len = writeable;
    iv[1].iov_base = extrabuf;// 第二个iovec指向临时缓冲区
    iv[1].iov_len = sizeof(extrabuf);
    // 如果可写入字节数小于临时缓冲区的大小,则使用两个iovec(两个iovec就是同时包括上面的iv[0]  iv[1]),一个指向Bufer的可写部分,一个指向extrabuf;否则使用一个iovec(只有iv[0]),指向Buffer可写部分
    const int iovcnt = (writeable < static_cast<int>(sizeof(extrabuf)) ? 2:1);
    // 指向分散读的操作,将数据从fd读入iv中的缓冲区
    int readn = static_cast<int>(::readv(fd, iv, iovcnt));// readv系统调用是一个分散读取操作,它允许将数据从一个文件描述符一次性地读取到多个缓冲区中
    if(readn < 0)
        LOG_ERROR << "Buffer::ReadFd readv failed";
    else if(readn <= writeable)// 读取的数据量<=缓冲区Buffer可写入的数据量,此时就可以直接写入
        writeIndex_ += readn; 
    else{// 读取的数据量>缓冲区可写入的字节数
        writeIndex_ = static_cast<int>(buffer_.size());// 将写入索引指到缓冲区末尾
        Append(extrabuf, readn-writeable);// 将剩余的数据追加到临时缓冲区extrabuf中
    }
}
// 获取缓冲区起始位置的指针
char* Buffer::begin(){
    return buffer_.data();
}
// 获取缓冲区起始位置的指针
const char* Buffer::begin() const {
    return buffer_.data();
}
// 获取当前读索引位置的指针 readIndex_
char* Buffer::beginRead(){
    return begin() + readIndex_;
}
// 获取当前读索引位置的指针 readIndex_
const char* Buffer::beginRead() const {
    return begin() + readIndex_;
}
// 获取当前写索引位置的指针 writeIndex_
char* Buffer::beginWrite(){
    return begin() + writeIndex_;
}
// 获取当前写索引位置的指针 writeIndex_
const char* Buffer::beginWrite() const { 
    return begin() + writeIndex_;
}
// 在缓冲区中查找换行符,因为HTTP协议中是
const char* Buffer::FindCRLF() const {
    const char* find = std::search(Peek(), beginWrite(), kCRLF, kCRLF+2);// 在readable缓冲区中查找换行符
    return find==beginWrite() ? nullptr:find;
}
// 追加字符串到缓冲区
void Buffer::Append(const char* message){
    Append(message, static_cast<int>(std::strlen(message)));// std::strlen()不能统计到'\0',因此下面的std::copy()没有复制'\0'到新位置
}
// 追加指定长度的字符串到缓冲区
void Buffer::Append(const char* message, int len){
    bool ResizeOrUnresize = MakeSureEnoughStorage(len);
    if(ResizeOrUnresize){// // 确保缓冲区有足够的存储空间存储len长的char
        std::copy(message, message+len, beginWrite());// 将message复制到beginWrite()位置
        writeIndex_ += len;
    }
    else{
        writeIndex_ += len;// 1000+408
        writeIndex_ -= readIndex_;// 1408-58
        // muduo中此种情况未把readIndex回到初始位置8,以下为自己修正的功能
        readIndex_ = kCheapPrepend;// 将readIndex_返回到前面,以保持prependable等于kCheapPrepend
        writeIndex_ += readIndex_;// 1350+8=1358
    }
}
// 追加std::string格式到缓冲区
void Buffer::Append(const std::string& message){
    Append(message.data(), static_cast<int>(message.size()));
}
// 从缓冲区中提取指定长度的数据  实际上就是一个移动readIndex_的过程(因为从缓冲区提取了数据,直接改变的就是readIndex_),不会关心具体的内容
void Buffer::Retrieve(int len){
    assert(readablebytes() >= len);
    if(len+readIndex_ < writeIndex_)
        readIndex_ += len;
    else
        RetrieveAll();
}
// 提取直到指定位置的数据
void Buffer::RetrieveUnitilIndex(const char* index){
    assert(index <= beginWrite());
    readIndex_ += static_cast<int>(index-beginRead());
}
// 复位操作
void Buffer::RetrieveAll(){
    // 提取完数据后,就把两个索引writeIndex_ readIndex_复原到最初的kCheapPrepend位置
    writeIndex_ = kCheapPrepend;
    readIndex_ = kCheapPrepend;
}
// 从缓冲区中提取指定长度的数据,并返回一个std::string
std::string Buffer::RetrieveAsString(int len){
    std::string ret = std::move(PeekAsString(len));// 将读缓冲区的数据移到ret中
    Retrieve(len);
    return ret;
}
// 从缓冲区中提取所有的数据,并返回一个std::string
std::string Buffer::RetrieveAllAsString(){
    std::string ret = std::move(PeekAllAsString());// 将读缓冲区的数据移到ret中
    RetrieveAll();
    return ret;
}
// 获取当前读位置的指针(这是beginRead()的为了上层公开调用的封装)  readIndex_
const char* Buffer::Peek() const {
    return beginRead();
}
// 获取当前读位置的指针(这是beginRead()的为了上层公开调用的封装)  readIndex_
char* Buffer::Peek() {
    return beginRead();
}
// 将读缓冲区指定长度中的数据转换为std::string
std::string Buffer::PeekAsString(int len){
    return std::string(beginRead(), beginRead()+len);
}
// 将读缓冲区中所有数据转换为std::string
std::string Buffer::PeekAllAsString(){
    return std::string(beginRead(), beginRead());
}
// 获取可读字节数 readable
int Buffer::readablebytes() const {
    return writeIndex_-readIndex_;
}
// 获取可写字节数 writable
int Buffer::writeablebytes() const {
    return static_cast<int>(buffer_.size())-writeIndex_;
}
// 获取可预留字节数  prependable
int Buffer::prependablebytes() const {
    return readIndex_;
}
// 确保缓冲区有足够的存储空间  可写区域不够的话就自动调整大小   返回true表示此时不用自动调整大小,返回false表示此时需要自动调整大小
bool Buffer::MakeSureEnoughStorage(int len){
    if(writeablebytes() >= len)
        return true;
    if(writeablebytes()+prependablebytes() >= kCheapPrepend+len){// 内部腾挪
        std::copy(beginRead(), beginWrite(), begin()+kCheapPrepend);
        writeIndex_ = kCheapPrepend + readablebytes();
        readIndex_ = kCheapPrepend;
        return true;
    }
    else{
        buffer_.resize(writeIndex_+len);// resize()直接调整的是vector的size,但是当size>capacity时,就会自动按指数增长此时的capacity了
        return false;
    }
}



