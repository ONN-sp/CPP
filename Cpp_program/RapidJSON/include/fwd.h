// 这是一个为了不同头文件之间的前向声明的头文件.主要目的是提前声明一些类和模板
#ifndef RAPIDJSON_FWD_H_ 
#define RAPIDJSON_FWD_H_
#include "rapidjson.h" 

namespace RAPIDJSON{
// 定义各种字符编码结构体
template<typename CharType> struct UTF8;// UTF-8 编码
template<typename CharType> struct UTF16;// UTF-16 编码
template<typename CharType> struct UTF16BE;// UTF-16 大端编码
template<typename CharType> struct UTF16LE;// UTF-16 小端编码
template<typename CharType> struct UTF32;// UTF-32 编码
template<typename CharType> struct UTF32BE;// UTF-32 大端编码
template<typename CharType> struct UTF32LE;// UTF-32 小端编码
template<typename CharType> struct ASCII;// ASCII 编码
template<typename CharType> struct AutoUTF;// 自动 UTF 编码

template<typename SourceEncoding, typename TargetEncoding>
struct Transcoder;// 定义编码转换器结构体

// allocators.h

class CrtAllocator;// 定义 CRT 分配器

template <typename BaseAllocator>
class MemoryPoolAllocator;// 定义内存池分配器模板类

// stream.h

template <typename Encoding>
struct GenericStringStream;// 定义通用字符串流结构体

typedef GenericStringStream<UTF8<char> > StringStream;  // 将 UTF-8 字符串流定义为 StringStream

template <typename Encoding>
struct GenericInsituStringStream;// 定义内存内字符串流结构体

typedef GenericInsituStringStream<UTF8<char> > InsituStringStream;// 将 UTF-8 内存内字符串流定义为 InsituStringStream

// stringbuffer.h

template <typename Encoding, typename Allocator>
class GenericStringBuffer;// 定义通用字符串缓冲区模板类

typedef GenericStringBuffer<UTF8<char>, CrtAllocator> StringBuffer;  // 将 UTF-8 字符串缓冲区定义为 StringBuffer

// filereadstream.h

class FileReadStream;// 定义文件读取流类

// filewritestream.h

class FileWriteStream;// 定义文件写入流类

// memorybuffer.h

template <typename Allocator>
struct GenericMemoryBuffer;// 定义通用内存缓冲区结构体

typedef GenericMemoryBuffer<CrtAllocator> MemoryBuffer;// 将 CRT 分配器的通用内存缓冲区定义为 MemoryBuffer

// memorystream.h

struct MemoryStream;// 定义内存流结构体

// reader.h

template<typename Encoding, typename Derived>
struct BaseReaderHandler;// 定义基本读取处理器模板类

template <typename SourceEncoding, typename TargetEncoding, typename StackAllocator>
class GenericReader;// 定义通用读取器模板类

typedef GenericReader<UTF8<char>, UTF8<char>, CrtAllocator> Reader;// 将 UTF-8 源编码和目标编码的通用读取器定义为 Reader

// writer.h

template<typename OutputStream, typename SourceEncoding, typename TargetEncoding, typename StackAllocator, unsigned writeFlags>
class Writer;// 定义通用写入器模板类

// prettywriter.h

template<typename OutputStream, typename SourceEncoding, typename TargetEncoding, typename StackAllocator, unsigned writeFlags>
class PrettyWriter;// 定义美化写入器模板类

// document.h

template <typename Encoding, typename Allocator> 
class GenericMember;  // 定义通用成员模板类

template <bool Const, typename Encoding, typename Allocator>
class GenericMemberIterator;// 定义通用成员迭代器模板类

template<typename CharType>
struct GenericStringRef;// 定义通用字符串引用结构体

template <typename Encoding, typename Allocator> 
class GenericValue;// 定义通用值模板类

typedef GenericValue<UTF8<char>, MemoryPoolAllocator<CrtAllocator> > Value;  // 将 UTF-8 编码和内存池分配器的通用值定义为 Value

template <typename Encoding, typename Allocator, typename StackAllocator>
class GenericDocument;// 定义通用文档模板类

typedef GenericDocument<UTF8<char>, MemoryPoolAllocator<CrtAllocator>, CrtAllocator> Document;  // 将 UTF-8 编码、内存池分配器和 CRT 分配器的通用文档定义为 Document

// pointer.h

template <typename ValueType, typename Allocator>
class GenericPointer;// 定义通用指针模板类

typedef GenericPointer<Value, CrtAllocator> Pointer;// 将 Value 和 CRT 分配器的通用指针定义为 Pointer

// schema.h

template <typename SchemaDocumentType>
class IGenericRemoteSchemaDocumentProvider;// 定义通用远程模式文档提供者接口

template <typename ValueT, typename Allocator>
class GenericSchemaDocument;// 定义通用模式文档模板类

typedef GenericSchemaDocument<Value, CrtAllocator> SchemaDocument;  // 将 Value 和 CRT 分配器的通用模式文档定义为 SchemaDocument
typedef IGenericRemoteSchemaDocumentProvider<SchemaDocument> IRemoteSchemaDocumentProvider;  // 将通用模式文档提供者接口定义为 IRemoteSchemaDocumentProvider

template <
    typename SchemaDocumentType,
    typename OutputHandler,
    typename StateAllocator>
class GenericSchemaValidator;// 定义通用模式验证器模板类

typedef GenericSchemaValidator<SchemaDocument, BaseReaderHandler<UTF8<char>, void>, CrtAllocator> SchemaValidator;  // 将通用模式验证器定义为 SchemaValidator
}
#endif 
