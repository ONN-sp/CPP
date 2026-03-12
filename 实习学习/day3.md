1. 如果一个.go程序第一行写了package a，那么意思是这个.go程序就属于包a。其他.go代码可以通过引入包a来调用这个.go程序中的方法
2. import库时user github.com/yourusername/trpc-user-system/proto/user，前面的user表示这个包的别名
3. go中没有类，所以它的方法不是绑定到类对象的，而是绑定到一个类型上的（或一个类型指针）的，即利用接收者参数。接收者参数（Receiver）是定义方法时的一个特殊参数，它绑定了一个方法到特定的类型上。通过接收者参数，我们可以在结构体、自定义类型或其他类型上定义方法，从而为这些类型添加特定的行为
4. go没有继承机制，它的继承思想是用嵌套实现的，比如结构体a里面嵌套了结构体b，那么a就可以使用b的方法，也可以对b的方法进行新的定义，即覆盖，定义为表示结构体a类型的含义的方法，只是名字没变
5. go的多态是用接口实现的。允许同一个接口被不同的类型实现，这样在调用方法时，具体执行哪个方法取决于运行时对象的实际类型
    ```go
    // 定义一个 Animal 类型
    type Animal struct {
        name string
    }
    // 实现 Mover 接口的 move 方法
    func (a *Animal) move() {
        fmt.Printf("%s is moving.\n", a.name)
    }
    // 定义一个 Car 类型
    type Car struct {
        brand string
    }
    // 实现 Mover 接口的 move 方法
    func (c *Car) move() {
        fmt.Printf("%s car is moving.\n", c.brand)
    }
    ```
6. go的接口interface是一种类型
7. 如果想要将消息类型用在rpc系统中，可以在.proto文件中定义一个rpc服务接口，protocol buffer编译器会根据所选择的不同语言生成服务接口代码和存根（如.pb.go、.trpc.go文件）。比如a.protoc将生成 a.pb.go ：包含消息类型的序列化和反序列化代码。 a.trpc.go ：包含服务接口代码和客户端存根代码
8. 客户端存根代码的主要作用是：封装网络通信：隐藏底层的网络通信细节，如建立连接、发送请求、接收响应等。序列化和反序列化（序列化常称为编码，反序列化常称为解码）：将客户端的请求数据序列化为字节流发送给服务端，并将服务端返回的字节流反序列化为客户端可以理解的数据结构。提供接口：为客户端提供与服务端方法一致的接口，使得客户端可以像调用本地方法一样调用远程方法，此时客户端就可以通过客户端代理（如:proxy := pb.NewGreeterClientProxy(client.WithTarget("ip://127.0.0.1:8000"))    proxy.SayHello(context.Background(), &pb.HelloRequest{Msg: "Hello"}))直接调用服务端定义的服务了
9. protoc文件的import时的路径是从运行.protoc文件的命令的当前目录开始算起的
10. go的new()和make()函数是用于创建的初始化变量的重要工具
    * new()用于创建指定类型的零值变量,并返回该变量的指针,适用于创建引用类型以外的其他类型变量
        ```go
        numPtr := new(int)
        ```
    * make()用于创建并初始化引用类型变量,如切片、映射和通道
        ```go
        slice := make([]int, 3)
        ```
11. 短变量声明不需要指定类型
12. 在go中，当一个结构体嵌套了另一个结构体时，外层结构体会继承嵌套结构体的所有字段和方法。为了使外层解耦他有具有自己的行为，就可以重新定义它的方法，即覆盖
13. go的值接收参数和指针接收参数
    * 值接收参数:值接收者会创建接收者的一个副本，方法操作的是这个副本，而不是原始值
    ```go
    type Animal struct {
        name string
    }
    // 值接收者方法
    func (a Animal) rename(newName string) {
        a.name = newName
    }
    func main() {
        animal := Animal{name: "Dog"}
        animal.rename("Cat")
        fmt.Println(animal.name) // 输出: Dog，因为 rename 方法操作的是副本
    } 
    ```
    * 指针接收参数:操作的是原始值的内存地址，因此可以修改原始值的状态
    ```go
    type Animal struct {
        name string
    }
    // 指针接收者方法
    func (a *Animal) rename(newName string) {
        a.name = newName
    }
    func main() {
        animal := Animal{name: "Dog"}
        animal.rename("Cat")
        fmt.Println(animal.name) // 输出: Cat，因为 rename 方法修改了原始值
    }
    ```
14. 指针接收者表示的是这个类型的指针上定义了方法
15. 因为go没有类，所以方法是绑定到类型上，而不是一个class上
16. 如果是接口的实现的话，那么其实现的指针接收参数就不能用值类型调用，只能用指针调用；而如果是值接收，那么指针调用和值调用都可以，这和普通的go语言方法不同。普通的go语言方法，对于指针接收和值接收都可以分别用指针和值进行调用
    ```go
    // 定义一个接口
    type Mover interface {
        move()
    }
    // 定义一个 Animal 类型
    type Animal struct {
        name string
    }
    // 指针接收者方法实现 Mover 接口
    func (a *Animal) move() {
        fmt.Printf("%s is moving.\n", a.name)
    }
    // Animal类型是接口Mover类型的实现
    func main() {
        // 创建 Animal 的值和指针
        animal := Animal{name: "Dog"}
        animalPtr := &Animal{name: "Cat"}
        // 指针可以作为接口类型
        var mover Mover = animalPtr
        mover.move() // 输出: Cat is moving.
        // 值不能直接作为接口类型，因为方法是用指针接收者实现的
        // var moverValue Mover = animal // 编译错误：cannot use animal (type Animal) as type Mover in assignment: Animal does not implement Mover (move method has pointer receiver)
    }
    ```
17. ...在函数参数中表示不定参数，如`func printNumbers(nums ...int) `:...允许在调用函数时传递零个或多个int类型的函数
18. `切片...`表示展开切片，如:`mySlice := []int{1, 2, 3, 4, 5}   printNumbers(mySlice...)`:展开切片，传递每个元素作为单独的参数
19. go的结构体初始化
    ```go
    1. 使用字面量
    p := Person{
        Name:    "Alice",
        Age:     30,
        Address: "123 Main St",
    }
    2. 使用空结构体
    p := Empty{}
    ```
20. 在go中，
    * 在相同包内访问时：如果两个.go文件属于同一个包(package指定)，那么可以直接访问对方的变量(无论是小写字母还是大写字母开头的变量)，如
    ```go
    // file1.go
    package mypackage
    var myVar = "Hello from file1"
    // file2.go
    package mypackage
    import "fmt"
    func printVar() {
        fmt.Println(myVar) // 直接访问 file1.go 中的变量
    }
    ```
    * 不同包间的访问时:如果两个 .go 文件属于不同的包，那么只能访问对方大写字母开头的变量
    ```go
    // file1.go
    package mypackage
    var MyVar = "Hello from file1" 
    package main
    // file2.go
    package main
    import (
        "fmt"
        "mypackage"
    )
    func main() {
        fmt.Println(mypackage.MyVar) // 访问 mypackage 包中的变量
    }
    ```
21. go中接口的实现指的是对这个接口包含的一组方法进行了实现的某个类型
22. go中接口变量可以被它的某个实现的类型对应的变量进行赋值，如果这个类型实现时用的指针接收，那么接口变量就只能被指针赋值；如果是用的值接收，那么指针赋值和值赋值都可以，如
    ```go
    type Mover interface {
        move()
    }
    // 指针接收者实现
    type Animal struct{ name string }
    func (a *Animal) move() { 
        fmt.Println(a.name, "is moving.") 
    }
    // 值接收者实现
    type Car struct{ brand string }
    func (c Car) move() {
        fmt.Println(c.brand, "car is moving.")
    }
    func main() {
        var mover Mover
        // 仅指针可用（因指针接收者）
        mover = &Animal{name: "Dog"}  // ✅
        mover.move()
        // 值或指针都可用（因值接收者）
        mover = Car{brand: "Toyota"}   // ✅ 值
        mover.move()
        mover = &Car{brand: "BMW"}     // ✅ 指针
        mover.move()
    }
    ```
23. go的并发
    * go协程：go程是由go运行时管理的轻量级线程:`go someFunction()`会启动一个新的go协程并执行， someFunction的执行发生在新的 Go 协程中，协程允许程序同时执行多个任务，而这些任务可以并发执行，共享同一个进程的内存空间
    * 通道是协程之间通信的方法。通道总是遵循先进先出，保证收发数据的顺序。通过是一种引用类型，类似切片，其定义可以为:
    ```go
    var ch1 chan int // 声明一个传递整型的通道
    ch2 := make(chan int) // 不会给通道赋予任何初始值
    ch3 := make(chan int, 2) // 通过可以带缓冲。带缓冲的话仅当缓冲区填满后，向其通道再方式数据时才会阻塞；当缓冲区为空时，从通过读取时会阻塞
    ```
    * 通道操作:发送、接收、关闭；发送和接收都是使用`<-`符号
    ```go
    ch := make(chan int)
    ch <- 10 // 把10发送到ch中
    x := <- ch // 从ch中接收值并赋值给变量x
    <-ch       // 从ch中接收值，忽略结果，从通道中接收后，那么通道就会少一个元素
    close(ch) // 关闭通道
    ```
    * 关于关闭通道需要注意的事情是，只有在通知接收方协程所有的数据都发送完毕的时候才需要关闭通道。通道是可以被垃圾回收机制回收的，它和关闭文件是不一样的，在结束操作之后关闭文件是必须要做的，但关闭通道不是必须的。 只应由发送者关闭信道，而不应由接收者关闭。向一个已经关闭的信道发送数据会引发程序 panic
    * 对于已经关闭的通道，如果再读取`v, ok := <-ch`,那么此时的ok=false
    * 优雅的关闭通道:利用range，`for i := range c`会不断从通道中接收值，直到它被关闭
    ```go
    func fibonacci(n int, c chan int) {
        x, y := 0, 1
        for i := 0; i < n; i++ {
            c <- x
            x, y = y, x+y
        }
        close(c)
    }
    c := make(chan int, 10)
	go fibonacci(cap(c), c)
	for i := range c {
		fmt.Println(i)
	} 
    ```
    * 在 Go 语言中，select 语句用于同时监听多个通道（channel）的通信操作,它允许程序在多个通道操作之间进行选择，当某个通道准备好时执行相应的操作.`select case`中的每个case分支都可以监听一个通道的读或写操作,如
    ```go
    select {
        case <-channel1:
            // 当 channel1 有值可读时执行
        case channel2 <- value:
            // 当 channel2 可写时执行
        default:
            // 如果没有通道准备好，则执行 default 分支（可选）
            // default实现了非阻塞操作，即没有通过可操作时不会阻塞，然后直接执行default分支
    }
    ``` 
24. `c := make(chan int)`创建一个通道后，此时这个通道是没有东西可读的，即`case <- c`这个分支是不会执行的
    ```go
    package main
    import "fmt"
    func fibonacci(c, quit chan int) {
        x, y := 0, 1
        for {
            select {
            case c <- x:
                x, y = y, x+y
            case <-quit:
                fmt.Println("quit")
                return
            }
        }
    }
    func main() {
        c := make(chan int)
        quit := make(chan int)
        go func() {
            for i := 0; i < 10; i++ {
                fmt.Println(<-c)
            }
            quit <- 0
        }()
        fibonacci(c, quit)
    }
    =>
    0
    1
    1
    2
    3
    5
    8
    13
    21
    34
    quit
    ```
25. 上述代码中出现了匿名函数
    ```go
    go func() {
		for i := 0; i < 10; i++ {
			fmt.Println(<-c)
		}
		quit <- 0
	}()
    // 花括号内为函数体,这个匿名函数的输入参数为空，返回参数也是空
    ```
26. chan的阻塞情况:
* 向无缓冲通道发送数据
```go
ch := make(chan int)  // 无缓冲通道
ch <- 42  // 阻塞！直到有其他 goroutine 接收这个值
// 当前协程会阻塞到ch <- 42后，即需要其他协程读取后，当前协程才不会阻塞，如：
func main() {
    ch := make(chan int) // 创建一个无缓冲通道
    go func() { <-ch }() // 启动一个新协程，从通道中读取数据
        ch <- 42             // 向通道发送数据
    fmt.Println(66)      // 主协程继续执行
}
```
* 从空的无缓冲同都接收数据
    ```go
    ch := make(chan int)
    val := <-ch  // 阻塞！直到有其他 goroutine 发送数据
    // 当前协程会阻塞在<-ch后，直到另一个协程向该通道发送数据
    ```
    * 向已满的有缓冲通道发送数据
    ```go
    ch := make(chan int, 2)  // 缓冲大小为 2
    ch <- 1
    ch <- 2
    ch <- 3  // 阻塞！因为缓冲区已满（除非有接收方取走数据）
    // 阻塞在ch <- 3后(放入成功了的)，直到有接收方取走数据
    ```
    * 从空的有缓冲通道接收数据
    ```go
    ch := make(chan int, 3)
    val := <-ch  // 阻塞！因为缓冲区为空（除非有发送方放入数据）
    // 阻塞在<-ch这一步，直到其他协程写入通道，val就能读出然后不阻塞了
    ```
    * nil通道上的任何操作
    ```go
    var ch chan int  // 未初始化的 nil 通道
    ch <- 42        // 报错
    val := <-ch     // 报错
    // nil通道会报错，是常见的bug，需要用make来创建通道，因为它会初始化
    // make创建的通道会初始化，不是nil通道
    ```
1.  分析下面bug
    ```go
    func main() {
        ch := make(chan int,1) // 创建一个无缓冲通道
        val := <-ch            // 向通道发送数据
        go func() { ch<-42 }() // 启动一个新协程，从通道中读取数据
        fmt.Println(val)      // 主协程继续执行
    }
    // 上述程序会报错，因为所有协程都死锁了
    // 主协程会阻塞在val := <-ch，导致另一个协程都开不了，因此死锁
    ```
2.  只要有一个协程没有死锁，程序就不会报错

