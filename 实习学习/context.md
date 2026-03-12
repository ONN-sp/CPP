1. 此包是用于不同的协程直接传递请求的相关数据，并且可以用来控制协程的生命周期和取消操作，主要有四个功能：数据传递（WithValue()）、取消协程（WithCancel()）、截止时间（WithDeadline()）、超时时间（WithTimeout()）
2. context包的学习
    * Context 的主要作用就是在不同的 Goroutine 之间同步请求特定的数据、取消信号以及处理请求的截止日期
    * 每一个 Context 都会从最顶层的 Goroutine 一层一层传递到最下层，这也是 Golang 中上下文最常见的使用方式，如果没有 Context，当上层执行的操作出现错误时，下层其实不会收到错误而是会继续执行下去。有了context后，上层协程出错，此时下层的协程由于没有收到context也会结束（比如上层出错，那么ctx就会结束而传不下来），如：
        ```go
        func main() {
            ctx, cancel := context.WithTimeout(context.Background(), 1*time.Second) // 过期时间
            defer cancel()
            go HelloHandle(ctx, 500*time.Millisecond) // 处理时间
            select {
            case <-ctx.Done():
                fmt.Println("Hello Handle ", ctx.Err())
            }
        }
        func HelloHandle(ctx context.Context, duration time.Duration) {
            select {
            case <-ctx.Done():
                fmt.Println(ctx.Err())
            case <-time.After(duration):
                fmt.Println("process request with", duration)
            }
        }
        // 此程序会因为上层协程由于ctx过期时间到了，则这个ctx传不下去了，因此下层的HelloHandle协程会在case <-ctx.Done()分支直接结束
        ```
        ![alt text](image-6.png)
    * context.Context结构体定义了四个方法
        ```go
        type Context interface {
         Deadline() (deadline time.Time, ok bool)
         Done() <-chan struct{}
         Err() error
         Value(key interface{}) interface{}
        }
        ```
    * context.Background()会返回一个空的上下文，通常用于main中或顶层中
    * context.TODO()也是创建一个空的上下文，也只能用于高等级或当您不确定使用什么 context，或函数以后会更新以便接收一个 context 
    * context的顶层是一个父ctx，也就是这个context树的根。通过WithCancel()、WithDeadline()、WithTimeout()、WithValue()可以基于这个父ctx创建子ctx，也就是继承父ctx
    * WithCancel函数，传递一个父Context作为参数，返回子Context，以及一个取消函数用来取消Context
    * WithDeadline函数，和WithCancel差不多，它会多传递一个截止时间参数，意味着到了这个时间点，即过期，会自动取消Context，当然我们也可以不等到这个时候，可以提前通过取消函数进行取消
    * WithValue函数和取消Context无关，它是为了生成一个绑定了一个键值对数据的Context，这个绑定的数据可以通过Context.Value方法访问到。context 包中的 context.WithValue 函数能从父上下文中创建一个子上下文，传值的子上下文使用 context.valueCtx 类型，valueCtx是一个将ctx绑定到键值对的结构体
        ```go
        func main() {
            ctx := context.Background()
            ctx = context.WithValue(ctx, "name", 123)
            GetUser(ctx)
        }
        func GetUser(ctx context.Context) {
            fmt.Println(ctx.Value("name").(int))
        }
        ```
    * WithCancel()：取消协程:考虑这样一种情景:我有一个获取ip的协程，但是这是一个耗时操作，用户可能随时会取消，如果用户取消了，那么之前那个正在获取ip的协程就应该要停止，因此出现了context的取消协程功能
        ```go
        1. 无协程取消的情况
        var wait = sync.WaitGroup{} // 开启一个同步等待，类似线程的t.join()
        func GetIP(ctx context.Context) {
            time.Sleep(5 * time.Second)
            fmt.Println("longRunningTask running")
            wait.Done()
        }
        func main() {
            t1 := time.Now()
            wait.Add(1)
            go GetIP(context.Background())
            wait.Wait()
            fmt.Println(time.Since((t1)))
        }
        =>
        longRunningTask running
        5.0007454s
        // 此时是没有协程取消，那么就算用户取消了，GetIP这个协程还是会执行完
        2. 再另一个协程中加入协程取消
        var wait = sync.WaitGroup{}
        func GetIP(ctx context.Context) {
            defer wait.Done()
            select {
            case <-ctx.Done():
                fmt.Println(ctx.Err())
                return
            case <-time.After(5 * time.Second):
                fmt.Println("longRunningTask running")
            }
        }
        func main() {
            t1 := time.Now()
            wait.Add(1)
            ctx, cancel := context.WithCancel(context.Background())
            go GetIP(ctx)
            go func() {// 模拟用户正在进行取消操作的那个协程
                time.Sleep(2 * time.Second)
                cancel() // 当前协程执行取消上下文操作从而达到协程取消
            }()
            wait.Wait()
            fmt.Println(time.Since((t1)))
        }
        =>
        context canceled
        2.0011113s
        ``` 
    * WithDeadline():设置截止时间
        ```go
        var wait = sync.WaitGroup{}
        func GetIP(ctx context.Context) {
            defer wait.Done()
            select {
            case <-ctx.Done():
                fmt.Println("longRunningTask exit")
                return
            case <-time.After(5 * time.Second):
                fmt.Println("longRunningTask running")
            }
        }
        func main() {
            t1 := time.Now()
            wait.Add(1)
            ctx, _ := context.WithDeadline(context.Background(), time.Now().Add(2*time.Second))
            go GetIP(ctx)
            wait.Wait()
            fmt.Println(time.Since((t1)))
        }
        =>
        context deadline exceeded
        2.0011967s
        // 此时不需要手动设定协程调用cancel()
        ```
    * WithTimeout:设置超时时间，和截止时间一样，本身就是调用WithDeadline去执行的
        ```go
                var wait = sync.WaitGroup{}
        func GetIP(ctx context.Context) {
            defer wait.Done()
            select {
            case <-ctx.Done():
                fmt.Println("longRunningTask exit")
                return
            case <-time.After(5 * time.Second):
                fmt.Println("longRunningTask running")
            }
        }
        func main() {
            t1 := time.Now()
            wait.Add(1)
            ctx, _ := context.WithDeadline(context.Background(), time.Now().Add(2*time.Second))
            go GetIP(ctx)
            wait.Wait()
            fmt.Println(time.Since((t1)))
        }
        =>
        context deadline exceeded
        2.0008966s
        ```
    * 以Context作为参数的函数方法，应该把Context作为第一个参数，放在第一位
    * Context是线程安全的，可以放心的在多个goroutine中传递。同一个Context可以传给使用其的多个goroutine，且Context可被多个goroutine同时安全访问
 
