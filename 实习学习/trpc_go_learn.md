# 微服务
1. 微服务是一种将大型单体应用拆分成多个独立、小型、松耦合服务（微服务之间尽可能解耦）的软件架构风格。每个微服务专注于单一业务功能、独立开发、部署、运行和扩展，并通过轻量级通信机制（grpc、http、trpc等）协同工作。微服务核心思想就是用多个协作的小服务代替一个庞大的整体应用，小服务之间用轻量级的api接口（grpc、trpc等）进行调用，但是微服务之间怎么知道对方的接口的地址呢？这里是利用服务注册与发现中心，如：服务1要找服务2的接口地址是通过服务的注册和发现中心实现的（比如北极星），服务2会先将自己注册到服务的注册和发现中心，然后服务1会从这个中心发现服务1的接口地址，然后再对这个服务发起调用。服务1对部署在不同服务器上的服务2的接口进行调用时会利用rpc框架，即实现调用远程服务就跟调用本地一样
# 客户端代理
1. 客户端代理是一个抽象层，由trpc、grpc框架生成，它封装了底层网络通信细节（如序列化、传输协议等），并暴露与服务端方法签名一致的接口（如：可以通过客户端代理直接调用服务端的方法SayHello()），此时使得客户端可以像调用本地方法一样调用远程的服务端服务了
2. 客户端要通过rpc进行调用服务端的服务时就要在pb中写对应的服务端的服务，此时编译pb文件才会生成.trpc.go文件，否则只有.go文件
3. 对于实习过程中基于trpc-go框架构建客户端的话，得到的一般都是客户端代理，后缀都是`proxy`，然后以客户端代理进行后续处理
4. pb文件编译后生成的.trpc.go中会生成客户端代理构建函数，和与服务端方法签名一致的可以由客户端之间调用的接口
5. pb文件编译后生成的.trpc.go中会生成服务端的服务的注册函数(如`RegisterGreeterService`)，也会生成一个服务端的服务的描述变量(如`GreeterServer_ServiceDesc`，描述了ServiceName等)
# Server模块(trpc-go根目录下的server文件夹)
1. 一个服务进程可能会监听多个端口，在不同端口上提供不同的业务服务。因此 server 模块提出了服务实例(Server)、逻辑服务(Service)和协议服务(proto service)的概念。通常一个进程包含一个服务实例，每个服务实例可以包含一个或者多个逻辑服务。逻辑服务用于名字注册，客户端会使用逻辑服务名进行路由寻址发起网络请求，服务端接收到请求后根据指定的协议服务执行服务端的业务逻辑。
   - `Server`：代表一个服务实例，即一个进程
   - `Service`：代表一个逻辑服务，即一个真正监听端口的对外服务，与配置文件yaml中的 service 一一对应，一个 server 可能包含多个 service，一个端口一个 service
   - `Proto service`：代表一个协议服务，protobuf 协议文件里面定义的 service就是协议服务，通常 service 与 proto service 是一一对应的，也可由用户自己通过 Register 任意组合
   ```golang
   // Server is a tRPC server. One process, one server.
   // A server may offer one or more services.
   type Server struct {
       MaxCloseWaitTime time.Duration
   }
   // Service is the interface that provides services.
   type Service interface {
       // Register registers a proto service.
       Register(serviceDesc interface{}, serviceImpl interface{}) error
       // Serve starts serving.
       Serve() error
       // Close stops serving.
       Close(chan struct{}) error
   }
   ```
2. 一个服务进程可能会监听多个端口，在不同端口上提供不同的业务服务。因此server模块提出了服务实例、逻辑服务，server代表一个服务实例，即一个进程；service代表一个逻辑服务，即一个真正监听端口的对外服务，与配置文件中的service一一对应，一个server可能包含对各service，一个端口一个service。如:
  ```yaml
  server: # 服务端配置
    app: test # 业务的应用名
    server: helloworld # 进程服务名
    close_wait_time: 5000 # 关闭服务时的最小等待时间，用于等待服务反注册完成，单位 ms
    max_close_wait_time: 60000 # 关闭服务时的最大等待时间，用于等待请求处理完成，单位 ms
    service: # 业务服务提供两个 service，监听不同的端口提供不同协议的服务
      - name: trpc.test.helloworld.HelloTrpc # 第一个 service 的路由名称
        ip: 127.0.0.1 # 服务监听 ip 地址
        port: 8000 # 服务监听端口 8000
        protocol: trpc # 提供 trpc 协议服务
      - name: trpc.test.helloworld.HelloHttp # 另一个 service 的路由名称
        ip: 127.0.0.1 # 服务监听 ip 地址
        port: 8080 # 监听端口 8080
        protocol: http # 提供 http 协议服务
  ```
  ```go
  type helloImpl struct{}
  func (s *helloImpl) SayHello(ctx context.Context, req *pb.HelloRequest) (*pb.HelloReply, error) {
      rsp := &pb.HelloReply{}
      // implement business logic here ...
      return rsp, nil
  }

  func main() {
      s := trpc.NewServer()
      // 为每个 service 单独注册 proto service
      pb.RegisterHiServer(s.Service("trpc.test.helloworld.HelloTrpc"), helloImpl)
      pb.RegisterHiServer(s.Service("trpc.test.helloworld.HelloHttp"), helloImpl)
  }
  ```
  因此一般只会调用一次NewServer()
3. `pb.RegisterHiServer(s.Service("trpc.test.helloworld.HelloTrpc"), helloImpl)`的调用流程为:
`s.Service("service_name")`利用yaml中的服务端的服务名构建一个`Service`->pb文件(`.trpc.go`)的`func RegisterGreeterService(s server.Service, svr GreeterService)`,此pb文件中会基于proto构建一个`ServiceDesc`结构体->`func (s *service) Register(serviceDesc interface{}, serviceImpl interface{}) error`,第一个参数要是ServiceDesc指针类型
4. 解释服务端注册服务，开启服务，然后客户端调用的整个过程
    ```go
    // server
    func main() {
      s := trpc.NewServer() // // 初始化配置文件里的信息(如yaml配置文件)，按照配置文件信息构建一个服务端实例
      pb.RegisterGreeterService(s, &Greeter{}) // 将Greeter结构体实现的服务注册到服务器上
      if err := s.Serve(); err != nil { // 开启服务端的服务，等待客户端的调用
        log.Error(err)
      }
    }
    type Greeter struct{}
    func (g Greeter) Hello(ctx context.Context, req *pb.HelloRequest) (*pb.HelloReply, error) { // Hello 方法处理客户端的 Hello 请求
      log.Infof("got hello request: %s", req.Msg)
      return &pb.HelloReply{Msg: "Hello " + req.Msg + "!"}, nil
    }
    // client
    func main() {
      c := pb.NewGreeterClientProxy(client.WithTarget("ip://127.0.0.1:8000")) // 按照指定的服务端地址创建一个客户端代理
      rsp, err := c.Hello(context.Background(), &pb.HelloRequest{Msg: "world"}) // 调用服务端注册的Hello()服务，输入参数是按照pb文件规定的Hello()这个服务接口的输入类型进行构建的
      if err != nil {
        log.Error(err)
      }
      log.Info(rsp.Msg)
    }
    // 在基于 trpc 的代码中，客户端通过客户端代理调用 SayHello 就可以直接执行到服务器端的 SayHello 服务，是因为 trpc 框架实现了底层的远程过程调用（RPC）机制，因此客户端代理调用SayHello就跟调用本地的一样
    ```
## service 映射关系
假如协议文件中提供 hello service，如：
  ```protobuf
  service hello { // 协议服务
      rpc SayHello(HelloRequest) returns (HelloReply) {};
  }
  ```
  配置文件写了多个 service，分别提供 trpc 和 http 协议服务如：
  ```yaml
  server: # 服务端配置
    app: test # 业务的应用名
    server: helloworld # 进程服务名
    close_wait_time: 5000 # 关闭服务时的最小等待时间，用于等待服务反注册完成，单位 ms
    max_close_wait_time: 60000 # 关闭服务时的最大等待时间，用于等待请求处理完成，单位 ms
    service: # 业务服务提供两个 service，监听不同的端口提供不同协议的服务
      - name: trpc.test.helloworld.HelloTrpc # 第一个 service 的路由名称
        ip: 127.0.0.1 # 服务监听 ip 地址
        port: 8000 # 服务监听端口 8000
        protocol: trpc # 提供 trpc 协议服务
      - name: trpc.test.helloworld.HelloHttp # 另一个 service 的路由名称
        ip: 127.0.0.1 # 服务监听 ip 地址
        port: 8080 # 监听端口 8080
        protocol: http # 提供 http 协议服务
  ```
  为不同的逻辑服务注册协议服务
  ```go
  type helloImpl struct{}
  func (s *helloImpl) SayHello(ctx context.Context, req *pb.HelloRequest) (*pb.HelloReply, error) {
      rsp := &pb.HelloReply{}
      // implement business logic here ...
      return rsp, nil
  }
  func main() {
      s := trpc.NewServer()
      // 推荐：为每个 service 单独注册 proto service
      pb.RegisterHiServer(s.Service("trpc.test.helloworld.HelloTrpc"), helloImpl)
      pb.RegisterHiServer(s.Service("trpc.test.helloworld.HelloHttp"), helloImpl)
      // 第二种方式，为 server 中的所有 service 注册同一个 proto service
      pb.RegisterHelloServer(s, helloImpl)
  }
  ```
## 服务端执行流程
1. 网络层 Accept 到一个新连接启动一个协程处理该连接的数据
2. 收到一个完整数据包，解包整个请求
3. 查询根据具体的 proto service 名，定位到具体处理函数
4. 解码请求体
5. 设置消息整体超时
6. 解压缩，反序列化请求体
7. 调用前置拦截器
8. 进入业务处理函数
9. 退出业务处理函数
10. 调用后置拦截器
11. 序列化，压缩响应体
12. 打包整个响应
13. 回包给上游客户端
## server.go(server包)
1. `func (s *Server) AddService(serviceName string, service Service)`:给这个服务器实例添加一个逻辑服务，也就是yaml中对应的一个service
2. `func (s *Server) Service(serviceName string) Service`:通过服务名返回一个具体对应的service
3. `func (s *Server) Register(serviceDesc interface{}, serviceImpl interface{}) error`:第一参数为通过pb文件构建的serviceDesc结构体指针类型，即一个服务描述。此方法就是在服务器实例中注册指定的服务描述的所有服务实例（一个服务描述可能有多个服务实例，比如在yaml中同一个service下可能多个方法（GET、GetUser等），导致有多个服务实例，通常一个服务描述就是一个服务实例）
4. `func (s *Server) Close(ch chan struct{}) error`:关闭服务器，并通知相关的协程和调用者，传入的ch是用于通知调用者关闭操作已完成
5. `func (s *Server) tryClose()`:执行实际的服务器的关闭逻辑，包括执行关闭钩子函数（这些函数通常用于执行关闭前的清理操作（如关闭数据库连接、保存状态等），钩子函数也就是关闭前执行的回调函数）和关闭所有服务
6. `func (s *Server) RegisterOnShutdown(fn func())`:注册一个关闭钩子函数，该函数将在服务器关闭时被调用
## service,go(server包)
1. `func (s *service) Serve() error`:开启服务端的服务，包括监听端口、注册服务到服务注册中心`s.opts.Registry`(如果yaml配置指定了服务注册中心(如北极星),则进行服务注册),并在服务运行期间保持监听状态
2. `func (s *service) Handle(ctx context.Context, reqBuf []byte) (rspBuf []byte, err error)`:接收客户端的请求，解码请求数据，调用具体的业务逻辑处理请求，并将处理结果编码后返回
3. `func (s *service) HandleClose(ctx context.Context) error`:用于在服务关闭时清理资源、执行必要的关闭操作
4. `func (s *service) encode(ctx context.Context, msg codec.Msg, rspBodyBuf []byte, e error) (rspBuf []byte, err error)`:将服务器端的服务的响应数据和可能的错误编码为可以发送给客户端的字节流
5. `func (s *service) decode(ctx context.Context, msg codec.Msg, reqBuf []byte) ([]byte, error)`:将客户端的请求数据解码为服务端的服务可以处理的格式
6. `func (s *service) setOpt(msg codec.Msg)`:设置消息对象的一些基本属性，这些属性是服务的元数据，其实就是从yaml中解析后的属性
7. `func (s *service) handle(ctx context.Context, msg codec.Msg, reqBodyBuf []byte) (interface{}, error)`:根据客户端的请求类型(普通RPC,流式RPC)，调用相应的处理函数来处理请求，并返回响应结果
8. `func (s *service) handleResponse(ctx context.Context, msg codec.Msg, rspBody interface{}) ([]byte, error)`:将服务处理后的响应数据进行序列化（二进制化）、压缩和编码，最终生成可以发送给客户端的字节流，它会调用encode
9. `func (s *service) filterFunc`:构造一个过滤函数，用于在请求处理中执行解压缩、反序列化(反二进制化)
等预处理操作10. `func (s *service) Register(serviceDesc interface{}, serviceImpl interface{}) error`:按照服务描述将服务注册到服务端实例
1.  `func (s *service) Close(ch chan struct{}) error`:关闭服务端的指定服务，包括取消所有子上下文、注销服务到注册中心，并等待当前活跃的请求处理完成。这个函数主要用于服务的优雅关闭
# trpc-go源码根目录下的trpc包
## trpc.go
1. 这个trpc包主要负责从yaml配置文件加载服务器配置、初始化服务器实例以及设置相关差距和服务
2. `func NewServer(opt ...server.Option) *server.Server`:加载服务器配置文件（默认为 trpc_go.yaml），并将其设置为全局配置,初始化插件和客户端，创建一个新的trpc服务器实例
3. `func NewServerWithConfig(cfg *Config, opt ...server.Option) *server.Server`:根据给定的配置（cfg）初始化服务器，即不是读取的yaml文件配置
4. `func GetAdminService(s *server.Server) (*admin.Server, error)`:从服务器获取管理员服务实例，通过服务名从服务器获取管理员服务实例
5. `func setupAdmin(s *server.Server, cfg *Config)`:初始化管理员服务，也就是Admin配置信息
6. `func newServiceWithConfig(cfg *Config, serviceCfg *ServiceConfig, opt ...server.Option) server.Service`:根据配置信息创建服务实例，cfg会传入配置信息，也就是一个结构体，这个结构体可以从yaml文件中解析，也可以手动给定
7. <mark>需要注意：`NewServer()`不会注册插件，它只是根据trpc_go.yaml文件中的相应插件对应的方法进行执行，插件的注册是在包导入时就完成了，即插件的注册是在相应包的`init()`函数中完成的，在`init()`时会注册插件，该插件会在该包中实现`Type()、Setup()`等方法，这些方法就会在`NerServer()`中的`SetPlugins()`下的`setup()`中按照插件名(如`config-rainbow`)而被调用</mark>
## config.go(trpc包)
1. `Config`结构体，包含了yaml文件中的所有配置项
   ```go
   type Config struct {
    Global struct {
      Namespace     string `yaml:"namespace"`      // Namespace for the configuration.
      EnvName       string `yaml:"env_name"`       // Environment name.
      ContainerName string `yaml:"container_name"` // Container name.
      LocalIP       string `yaml:"local_ip"`       // Local IP address.
      EnableSet     string `yaml:"enable_set"`     // Y/N. Whether to enable Set. Default is N.
      // Full set name with the format: [set name].[set region].[set group name].
      FullSetName string `yaml:"full_set_name"`
      // Size of the read buffer in bytes. <=0 means read buffer disabled. Default value will be used if not set.
      ReadBufferSize *int `yaml:"read_buffer_size,omitempty"`
    }
    Server struct {
      App      string `yaml:"app"`       // Application name.
      Server   string `yaml:"server"`    // Server name.
      BinPath  string `yaml:"bin_path"`  // Binary file path.
      DataPath string `yaml:"data_path"` // Data file path.
      ConfPath string `yaml:"conf_path"` // Configuration file path.
      Admin    struct {
        IP           string      `yaml:"ip"`            // NIC IP to bind, e.g., 127.0.0.1.
        Nic          string      `yaml:"nic"`           // NIC to bind.
        Port         uint16      `yaml:"port"`          // Port to bind, e.g., 80. Default is 9028.
        ReadTimeout  int         `yaml:"read_timeout"`  // Read timeout in milliseconds for admin HTTP server.
        WriteTimeout int         `yaml:"write_timeout"` // Write timeout in milliseconds for admin HTTP server.
        EnableTLS    bool        `yaml:"enable_tls"`    // Whether to enable TLS.
        RPCZ         *RPCZConfig `yaml:"rpcz"`          // RPCZ configuration.
      }
      Transport    string           `yaml:"transport"`     // Transport type.
      Network      string           `yaml:"network"`       // Network type for all services. Default is tcp.
      Protocol     string           `yaml:"protocol"`      // Protocol type for all services. Default is trpc.
      Filter       []string         `yaml:"filter"`        // Filters for all services.
      StreamFilter []string         `yaml:"stream_filter"` // Stream filters for all services.
      Service      []*ServiceConfig `yaml:"service"`       // Configuration for each individual service.
      // Minimum waiting time in milliseconds when closing the server to wait for deregister finish.
      CloseWaitTime int `yaml:"close_wait_time"`
      // Maximum waiting time in milliseconds when closing the server to wait for requests to finish.
      MaxCloseWaitTime int `yaml:"max_close_wait_time"`
      Timeout          int `yaml:"timeout"` // Timeout in milliseconds.
    }
    Client  ClientConfig  `yaml:"client"`  // Client configuration.
    Plugins plugin.Config `yaml:"plugins"` // Plugins configuration.
  } 
  type ClientConfig struct {
    Network        string                  `yaml:"network"`        // Network for all backends. Default is tcp.
    Protocol       string                  `yaml:"protocol"`       // Protocol for all backends. Default is trpc.
    Filter         []string                `yaml:"filter"`         // Filters for all backends.
    StreamFilter   []string                `yaml:"stream_filter"`  // Stream filters for all backends.
    Namespace      string                  `yaml:"namespace"`      // Namespace for all backends.
    Transport      string                  `yaml:"transport"`      // Transport type.
    Timeout        int                     `yaml:"timeout"`        // Timeout in milliseconds.
    Discovery      string                  `yaml:"discovery"`      // Discovery mechanism.
    ServiceRouter  string                  `yaml:"servicerouter"`  // Service router.
    Loadbalance    string                  `yaml:"loadbalance"`    // Load balancing algorithm.
    Circuitbreaker string                  `yaml:"circuitbreaker"` // Circuit breaker configuration.
    Service        []*client.BackendConfig `yaml:"service"`        // Configuration for each individual backend.
  }
  ```
1. 这种对应yaml文件的结构体一般会自己根据自己的项目需求定义一个config.go包含这些结构体
2. `func LoadConfig(configPath string) (*Config, error)`:
3. `LoadConfig()`会调用`func parseConfigFromFile(configPath string) (*Config, error)`:
4. `parseConfigFromFile()`函数会调用`gopkg.in/yaml.v3`包的`Unmarshal()`对yaml文件进行解析
# config模块
## config.go(config包)
1. 配置管理是微服务治理体系中非常重要的一环，tRPC 框架为业务程序开发提供了一套支持从多种数据源(yaml、json等)获取配置，解析配置和感知配置变化的标准接口，框架屏蔽了和数据源对接细节，简化了开发
2. 业务配置是供业务使用的配置(比如yaml中的配置)，它由业务程序定义配置的格式，含义和参数范围，tRPC 框架并不使用业务配置，也不关心配置的含义。框架仅仅关心如何获取配置内容，解析配置，发现配置变化并告知业务程序；
3. 业务配置和框架配置的区别在于使用配置的主体和管理方式不一样。框架配置是供 tRPC 框架使用的，由框架定义配置的格式和含义。框架配置仅支持从本地文件读取方式，在程序启动是读取配置，用于初始化框架。框架配置不支持动态更新配置，如果需要更新框架配置，则需要重启程序；而业务配置则不同，业务配置支持从多种数据源获取配置，比如：本地文件，配置中心，数据库等。如果数据源支持配置项事件监听功能，tRPC 框架则提供了机制以实现配置的动态更新。config包就是用来读取业务配置、监听业务配置（如七彩石配置可以使业务监听配置指向的七彩石服务的配置项的变更等（Watch()））
4. 数据源是获取配置的来源，配置存储的地方。常见的数据源包括：file、etcd、configma、env等，默认是file，所以config.Load()的时候没有用WithProvider()
5. 业务配置中的 Codec 是指从配置源获取到的配置的格式，常见的配置文件格式为：yaml，json，toml 等，默认是yaml，所以config.Load()的时候没有用WithCodec()
6. 配置接口模块实现的示意图
  ![](2025-06-12-22-49-43.png)
1. 默认编解码器和数据源分别是yaml和file,如果不是则要：
  ```go
  // 加载配置文件：path 为配置文件路径
  func Load(path string, opts ...LoadOption) (Config, error)
  // 更改编解码类型，默认为“yaml”格式
  func WithCodec(name string) LoadOption
  // 更改数据源，默认为“file”
  func WithProvider(name string) LoadOption
  // 加载 etcd 配置文件：config.WithProvider("etcd")
  // 例子
  c, _ := config.Load("./trpc_go.yaml", config.WithCodec("yaml"), config.WithProvider("etcd"))
  // 加载本地配置文件，codec 为 json，数据源为 file
  c, _ := config.Load("./trpc_go.yaml", config.WithCodec("json"), config.WithProvider("file"))
  // 加载本地配置文件，默认codec 为 yaml，数据源为 file
  c, _ := config.Load("./trpc_go.yaml")
  ```
1. config包读取yaml配置的通用接口
  ```go
  // Config 配置通用接口
  type Config interface {
      Load() error
      Reload()
      Get(string, interface{}) interface{}
      Unmarshal(interface{}) error
      IsSet(string) bool
      GetInt(string, int) int
      GetInt32(string, int32) int32
      GetInt64(string, int64) int64
      GetUint(string, uint) uint
      GetUint32(string, uint32) uint32
      GetUint64(string, uint64) uint64
      GetFloat32(string, float32) float32
      GetFloat64(string, float64) float64
      GetString(string, string) string
      GetBool(string, bool) bool
      Bytes() []byte
  }
  // 这些通用接口可以在外部被我们用于自己的业务时进行实现
  ```
1. `func GetString(key string) (string, error)`:根据给定的键key从kv存储中获取对应的字符串值(此函数是用于从键值（KV）存储中获取值的，所以在调用这个函数之前，相关的键值对应该已经被存储到 KV 存储中了)
2.  `func GetInt(key string) (int, error)`:根据给定的键key从kv存储中获取对应的int值
3.  `func GetWithUnmarshal(key string, val interface{}, unmarshalName string) error`:根据给定的键key从kv存储中获取值并根据指定的解码器（json、pb、xml等）名称将其解码（反序列化）到目标变量中，这里使用的是默认的Provider数据源"file"
4.  `func GetWithUnmarshalProvider(key string, val interface{}, unmarshalName string, provider string) error`:用于从指定的配置提供者（数据源）中获取配置值，并使用指定的解码器将其解码到目标变量中
5.  `func GetYAML(key string, val interface{}) error`:根据给定的键key从kv存储中获取值并根据指定的解码器yaml反序列化到val中
6.  `func GetYAMLWithProvider(key string, val interface{}, provider string) error`:用于从指定的配置提供者（数据源）中获取配置值，并使用指定的解码器yaml将其解码到目标变量中 
7.  `func GetJSON(key string, val interface{}) error `:根据给定的键key从kv存储中获取值并根据指定的解码器json反序列化到val中
8.  `func GetJSONWithProvider(key string, val interface{}, provider string) error`:用于从指定的配置提供者（数据源）中获取配置值，并使用指定的解码器json将其解码到目标变量中   
9.  `func (yu *YamlUnmarshaler) Unmarshal(data []byte, val interface{}) error`:将输入的字节切片中的 YAML 文档进行解析，并将解析后的值填充到输出参数所指向的值中，如:`var t T    yaml.Unmarshal([]byte("a: 1\nb: 2"), &t)`
10. `func (ju *JSONUnmarshaler) Unmarshal(data []byte, val interface{}) error`:将输入的字节切片中的 json 文档进行解析，并将解析后的值填充到输出参数所指向的值中
11. 定义一个kv型的配置接口
    ```go
    type KVConfig interface {
      KV
      Watcher
      Name() string
    }
    ```
12. `func Get(name string) KVConfig`:根据给定键key返回对应的KV值
13. `func GetCodec(name string) Codec`:根据编码源codec名来得到具体的编码源（yaml、json等）
14. `func GetProvider(name string) DataProvider`:根据provider数据源名来得到具体的数据源（file、etcd等）
15. `func Load(path string, opts ...LoadOption) (Config, error)`:通过指定的路径和可选的加载选项来加载配置文件（yaml、json等文件）,如`config.Load("./trpc_go.yaml")`
16. 反序列化(Unmarshal())是将数据从一种存储或传输格式（如字节流、字符串、JSON、XML、YAML 等）转换回原始编程语言中的对象或数据结构的过程。反序列化是序列化的逆过程，序列化是将对象或数据结构转换为便于存储或传输的格式，如
    ```go
    // 反序列化
    type Person struct {
      Name string
      Age  int
      City string
    }
    var p Person
    json.Unmarshal([]byte(`{"name": "John", "age": 30, "city": "New York"}`), &p)
    =>
    p:{
      Name : "John",
      Age  : 30,
      City : "New York"
    }
    ```
# naming名字服务模块
## 北极星插件
1. 其服务发现的寻址方式有两种
   ![](2025-06-15-20-43-10.png)
2. 源码见:`https://github.com/trpc-ecosystem/go-naming-polarismesh/blob/main/README.zh_CN.md`