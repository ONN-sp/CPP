1. yaml中定义的client的服务指的是这个客户端代理要调用的后端服务的配置，而不是这个客户端自己的服务
2. 框架配置文件主要分为四部分
    * global
    * server
    * client
    * plugins
3. 只有将yaml文件定义在当前运行目录下，才不需要-conf来指定路径
4. trpc-go框架下NewServer()会加载yaml配置信息，但是其返回的server结构体不能直接获取对应的每一条配置信息，它只是将配置信息加载好了，初始化了一个服务器实例
5. yaml文件中的配置信息对应到代码中有两种表示形式，一种是无切片的嵌套结构体，一种是有切片的嵌套结构体
    ```yaml
     # 底层配置用-开头，表示列表项，对应到结构体中就要是结构体切片
    plugins:
        config:
        rainbow: # 新增七彩石配置
        providers:
            - name: rainbow
              appid: v1_rb5_e9941919-cff4-4e7e-9e84-bd700
              group: hello_test
              env_name: Default
    ```
    对应的嵌套结构体为
    ```go
    type PluginsRainbowConfig struct {
        Plugins struct {
            Config struct {
                System struct {
                    Providers []struct { // 这里要用结构体切片，因为
                        Name     string `yaml:"name"`     // 服务名称
                        Appid    string `yaml:"appid"`    // 服务id
                        Group    string `yaml:"group"`    // 服务分组
                        Env_name string `yaml:"env_name"` // 环境名称
                    } `yaml:"providers"`
                } `yaml:"rainbow"` // 七彩石配置
            } `yaml:"config"`
        } `yaml:"plugins"` // 全局配置
    }
    ```
    ```yaml
    global:  # 全局配置
        namespace: Development  # 环境类型，分正式 production 和非正式 development 两种类型
        env_name: test
    ```
    对应的嵌套结构体为：
    ```go
    type Global struct {
        type System struct {
            namespace string `yaml:"Development"`
            env_name string `yaml:"test"`
        } `yaml:"Development"`
    } `yaml:"global"`
    ```
6. 只有将yaml文件定义在当前运行目录下，才不需要-conf来指定路径
7. 对于上述结构体后的`yaml:"Development"`这种的解释：这是结构体标签，对于这里，就是用于在解析YAML时明确字段与数据源的映射关系，即比如：指定后，那么字段namespace在yaml中的键名必须是name
8. 尝试增加客户端的yaml文件中的service的过程中遇到问题：pb文件修改后无法利用编译工具将生成的包后在程序中导入？
    问题解决：pb文件中的go_package的路径应该是git.woa.com/工蜂名/项目根目录/pb生成的.go的路径
9. 所遇问题：客户端配置读取不到？
    具体原因：因为客户端我是用pb.NewGreeterClientProxy()创建一个代理，而我的pb生成的包名是hello，NewGreeterClientProxy()中会调用WithClientRPCName("/hello.Greeter/SayHello")，但是我的yaml文件中客户端的name=trpc.test.hello.Greeter，因此就是错的，客户端的RPC名字要和yaml文件中保持一致，即可以将yaml文件客户端的name改为hello.Greeter
10. 在 gRPC 和 tRPC 的 YAML 配置文件中，client 部分的 service 配置通常指的是客户端需要调用的后端服务的连接信息，而不是客户端自身提供的服务配置
