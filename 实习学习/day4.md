1. io包指定了io.Reader接口，它表示数据流的读取端。io.Reader接口有一个Read方法:`func (T) Read(b []byte) (n int, err error)`:Read用数据填充给定的字节切片并返回填充的字节数和错误值。在遇到数据流的结尾时，它会返回一个io.EOF错误
    ```go
    func main() {
        r := strings.NewReader("Hello, Reader!") // 创建一个可以按需读取字符串内容的流式读取器，即字符串放在这个流式读取器里的
        b := make([]byte, 8)
        for {
            n, err := r.Read(b)
            fmt.Printf("n = %v err = %v b = %v\n", n, err, b)
            fmt.Printf("b[:n] = %q\n", b[:n])
            if err == io.EOF {
                break
            }
        }
    }
    =>
    n = 8 err = <nil> b = [72 101 108 108 111 44 32 82]
    b[:n] = "Hello, R"
    n = 6 err = <nil> b = [101 97 100 101 114 33 32 82]
    b[:n] = "eader!"
    n = 0 err = EOF b = [101 97 100 101 114 33 32 82]
    b[:n] = ""
    ```
2. go中的for range:for range 是一种非常强大且常用的循环结构，用于遍历数组、切片、字符串、映射（map）和通道（channel）,如
    ```go
    1. 
    slice := []int{1, 2, 3, 4, 5}
    for index, value := range slice{}
    2.
    str := "Hello, 世界"
    for index, runeValue := range str{}
    3.
    m := map[string]int{
            "apple":  1,
            "banana": 2,
            "cherry": 3,
        }
    for key, value := range m{}
    4.  
    ch := make(chan int)
    go func() {
            for i := 0; i < 5; i++ {
                ch <- i
            }
            close(ch)
        }()
    for value := range ch {} // 用于从通道中接收值，直到通道关闭
    ```
3. 匿名函数可以在定义时直接调用
   ```go
   	go func() {
        ...
	}()
    ```
4. 在go中，init()函数会自动执行，它是在包被加载时自动执行的，它是在包的初始化阶段被调用
5. 对于一个go程序来说，包的初始化顺序是从依赖关系最浅的包开始，逐步向依赖关系更深的包进行。如：如果包A依赖包B（即包A中import了包B），那么包B会先于包A进行初始化，包括执行包B中的init()函数
6. go中，[]Item{a}表示一个切片字面量，其类型是 []Item，即一个元素类型为Item的切片，该切片包含了一个元素a
    ```go
    type Item struct {
        name string
        value int
    }
    func main() {
        a := Item{name: "item1", value: 10} // 初始化一个Item结构体变量a
        slice := []Item{a} // 创建了一个包含一个元素a的切片，其中的元素的类型为Item
        fmt.Println(slice)
    }
    ```