1. 在 Protocol Buffers（protobuf）中，go_package 是一个重要的选项，用于指定生成的 Go 代码应该属于哪个模块（module）和包（package）。它通常出现在 .proto 文件中，用于控制生成代码的导入路径和包结构
2. `option go_package="trpc.group/trpc-go/trpc-go/examples/helloworld/pb";`:表明生成的.go包属于`trpc.group/trpc-go/trpc-go`模块（这是一个已经有的完整模块名），`examples/helloworld/pb`就算这个模块下的包，那么最后pb文件生成的包就会在这个路径下面。对于其它的项目使用这个pb文件生成的包，它的go.mod中导入的是模块`trpc.group/trpc-go/trpc-go`，具体的程序中是`import trpc.group/trpc-go/trpc-go/examples/helloworld/pb`，因为这个pb文件生成的包就在这个模块下的这个路径中，这个`import`路径其实就是`go_package`指向的路径，其实也可以不看这个`go_package`，单纯从这个pb文件生成的包的地址来看，即模块名+包的相对地址
3. 一个包怎么被导入到另一个程序中：那个包的module名+那个包相对这个module的相对路径
4. pb中没有定义service就不会生成.trpc.go文件，只有.go程序
5. 通常一个go项目（需求）的开始都是从pb文件开始的，pb文件定义了这个项目（需求）中所用的输入和输出格式，一般对于一个要被调用的服务，它的输入和输出就是这个pb文件定义的数据格式（比如：一个包含id和品质的结构体），这个被调服务（就是客户端要调用的那个后端服务，往往是服务端注册上去的）的最上层接口往往就是以pb文件为输入和输出格式设定的。pb文件定义的message最终都会转换为对应编译生成的.go包的结构体，这个结构体就是这个服务被在线调用时相互传输用的（比如作为一个底层接口的输入输出被调用），pb中的request和response对应的结构体往往就是这个服务最上层的main.go调用的主处理逻辑(process()或handle()等)的输入和输出，比如：利用version+svn+name来提取出redis中存的相应表（这个表是以redis中键值对的值进行存储的），表的结构是id+品质，因此这个pb文件定义的request就是包含了version、svn、name三个元素的message，response就是包含了id、品质两个元素的message
6. `trpc`采用`protobuf`来描述一个服务,用`protobuf`定义服务方法,请求参数和相应参数
7. protocol buffers是google用于序列化结构化数据的语言中立、平台中立、可扩展的机制-类似XML，但更小、更快、更简单
8. 第一行为`syntax = "proto3"`
9. ```go
    syntax = "proto3";
    message SearchRequest {
        string query = 1;
        int32 page_number = 2;
        int32 results_per_page = 3;
    }
    ```
    SearchRequest消息定义了三个字段。每个字段由类型、名称和编号组成。字段编号在此消息中是唯一的
10. .proto文件注释和cpp一样
11. 导入另一个文件中的定义`import`,如:`import "myproject/other_protos.proto";`
12. `.proto`支持嵌套类型
    ```go
    message SearchResponse {
        message Result {
            string url = 1;
            string title = 2;
            repeated string snippets = 3;
        }
        repeated Result results = 1;
    }
    ```
13. 如果要关联映射作为数据定义的一部分，protocal buffers提供了`map<key_type, value_type> map_field = N;`,如:`map<string, Project> projects = 3;`
14. trpc采用protobuf来描述一个服务，我们用protobuf定义服务方法，请求参数和响应参数。写好protobuf后可以通过trpc命令行生成完整服务代码`trpc create -p helloworld.proto`,通过protobuf可以生成桩代码，protobuf生成的桩代码在给定的`option`中的地址下
15. protobuf是一种高效、跨平台、语言无关的序列号框架，用于结构化数据的存储和传输，它比JSON/XML更小、更快、更简单，广泛应用于RPC通信
16. protobuf定义服务,用于trpc通信
    ```go
    service Greeter {
        rpc SayHello (HelloRequest) returns (HelloReply);  // 定义 RPC 方法
    }
    message HelloRequest {
        string name = 1;
    }
    message HelloReply {
        string message = 1;
    }
    ```
17. pb文件中类型在变量左边
18. go mod中导入的是每个其他库的go mod中的module名，然后程序在基于这个module去import到具体的包下面
19. protobuf文件中的go_package指定的是利用编译工具（trpc）生成的包的目标地址
20. proto文件其实就是一个约束（也可以叫做协议），通过proto生成的.go代码会将rpc方法变成func，可能把message变成结构体。proto定义的rpc方法其实是一个方法接口，它定义了接收什么参数，返回什么参数这些，接收参数长什么样，返回参数长什么样
21. 在基于 Protocol Buffers（Protobuf）和 tRPC 的系统中，客户端调用的是基于.proto 文件定义的服务接口，而底层实际执行的是服务端用具体编程语言实现的代码逻辑（底层调用是通过trpc实现的）


