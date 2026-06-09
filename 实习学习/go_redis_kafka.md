1. redis与其他kv缓存有以下三个特点
    * redis支持数据的持久化，可以将内存中的数据保存在磁盘中，重启时可以再次加载进行使用
    * redis不仅仅支持简单的kv类型的数据，同时还提供string、list、set、hash、zset
    * redis支持数据备份，即master-slave模式的数据备份
2. github.com/garyburd/redigo/redis库操作redis
    ```go
    1. c, err := redis.Dial("tcp", "localhost:6379")
    2. _, err = c.Do("Set", "abc", 100)
    3. val, err := redis.Int(c.Do("Get", "abc"))
    4.  _, err = c.Do("MSet", "abc", 100, "efg", 300)
    5.  vals, err := redis.Ints(c.Do("MGet", "abc", "efg"))
    6.  _, err = c.Do("expire", "abc", 10)
    7. _, err = c.Do("lpush", "book_list", "abc", "ceg", 300)
    8. val, err := redis.String(c.Do("lpop", "book_list"))
    9.  _, err = c.Do("HSet", "books", "abc", 100)
    10. val, err := redis.Int(c.Do("HGet", "books", "abc"))
    ```
3. go-redis库操作redis
    ```go
    1. rdb := redis.NewClient(&redis.Options{
        Addr:     "localhost:6379", // Redis 服务器地址
        Password: "",              // 密码
        DB:       0,               // 数据库编号
    })
    2. _, err := rdb.Ping(context.Background()).Result() // 检查连接
    3. err := rdb.Set(ctx, "key", "value", 0).Err() // 0表示不过期
    4. val, err := rdb.Get(ctx, "key").Result()
    5. rdb.SetEx(ctx, "expire_key", "value", time.Hour) // 1小时后过期
    6. rdb.HSet(ctx, "user:1", "name", "Alice", "age", 30)
    7. name, err := rdb.HGet(ctx, "user:1", "name").Result()
    8. user, err := rdb.HGetAll(ctx, "user:1").Result()
    9. rdb.HIncrBy(ctx, "user:1", "age", 1) // 递增数据字段
    10. rdb.LPush(ctx, "messages", "msg1") // 从左侧推入
    11. rdb.RPush(ctx, "messages", "msg2") // 从右侧推入
    12. messages, err := rdb.LRange(ctx, "messages", 0, -1).Result() // 获取范围
    13. msg, err := rdb.LPop(ctx, "messages").Result() // 弹出元素
    14. rdb.SAdd(ctx, "tags", "golang", "redis")
    15. tags, err := rdb.SMembers(ctx, "tags").Result()
    16. isMember, err := rdb.SIsMember(ctx, "tags", "golang").Result()
    17. rdb.SUnion(ctx, "tags1", "tags2") // 并集
    18. rdb.SInter(ctx, "tags1", "tags2") // 交集
    19. rdb.ZAdd(ctx, "rank", redis.Z{Score: 90, Member: "Alice"})
    20. rank, err := rdb.ZRank(ctx, "rank", "Alice").Result() //获取由score得到的排名
    21. results, err := rdb.ZRangeWithScores(ctx, "rank", 0, -1).Result() // 范围查询
    ```
4. kafka中的基础概念：
    ```txt
    1.Topic(话题)：Kafka中用于区分不同类别信息的类别名称。由producer指定
    2.Producer(生产者)：将消息发布到Kafka特定的Topic的对象(过程)
    3.Consumers(消费者)：订阅并处理特定的Topic中的消息的对象(过程)
    4.Broker(Kafka服务集群)：已发布的消息保存在一组服务器中，称之为Kafka集群。集群中的每一个服务器都是一个代理(Broker). 消费者可以订阅一个或多个话题，并从Broker拉数据，从而消费这些已发布的消息。
    5.Partition(分区)：Topic物理上的分组，一个topic可以分为多个partition，每个partition是一个有序的队列。partition中的每条消息都会被分配一个有序的id（offset）
    6.Replication:每一个分区都有多个副本，副本的作用是做备胎。当主分区（Leader）故障的时候会选择一个备胎（Follower）上位，成为Leader
    7.Consumer Group
    ```
5. kafka原理:
    * Producer：Producer即生产者，消息的产生者，是消息的⼊口。
    * kafka cluster：kafka集群，一台或多台服务器组成
        - Broker：Broker是指部署了Kafka实例的服务器节点。每个服务器上有一个或多个kafka的实例，我们姑且认为每个broker对应一台服务器。每个kafka集群内的broker都有一个不重复的 编号，如图中的broker-0、broker-1等……
        - Topic：消息的主题，可以理解为消息的分类，kafka的数据就保存在topic。在每个broker上 都可以创建多个topic。实际应用中通常是一个业务线建一个topic。
        - Partition：Topic的分区，每个topic可以有多个分区，分区的作用是做负载，提高kafka的吞 吐量。同一个topic在不同的分区的数据是不重复的，partition的表现形式就是一个一个的⽂件夹！
        - Replication:每一个分区都有多个副本，副本的作用是做备胎。当主分区（Leader）故障的 时候会选择一个备胎（Follower）上位，成为Leader。在kafka中默认副本的最大数量是10 个，且副本的数量不能大于Broker的数量，follower和leader绝对是在不同的机器，同一机 器对同一个分区也只可能存放一个副本（包括自己）。
    * Consumer：消费者，即消息的消费方，是消息的出口。
    * Consumer Group：我们可以将多个消费组组成一个消费者组，在kafka的设计中同一个分 区的数据只能被消费者组中的某一个消费者消费。同一个消费者组的消费者可以消费同一个 topic的不同分区的数据，这也是为了提高kafka的吞吐量
    ![](2025-06-22-20-52-13.png)
    ![](2025-06-22-21-25-48.png)
    * kafka工作流程：
        - 生产者从kafka集群获取分区leader信息
        - 生产者将消息发送给leader
        - leader将消息写入本地磁盘
        - follower从leader拉取消息数据
        - follwer将消息写入本地磁盘后向leader发送ack
        - leader收到所有的follwer的ack之后向生产者发送ack
    ![](2025-06-22-20-52-44.png)
    * 选择partition分区的原则
        - partition在被写入的时候(即生产者发送消息时)可以指定需要写入的partition(通过指定分区编号实现)，如果有指定，则写入对应的partition
        - 如果没有指定partition，但是设置了数据的key，则会根据key的值hash出一个partition（类似redis的哈希槽）,相同的key会在一个partition里面。这是kafka默认分区策略
        - 如果既没指定partition，又没有设置key，则会采用轮询方式，即每次取一小段时间的数据写入某个partition，下一小段的时间写入下一个partition
    * ack应答机制：producer在向kafka写入消息的时候，可以设置参数来确定是否确认kafka接收到数据，这个参数可设置 的值为 0,1,all
        - 0代表producer往集群发送数据不需要等到集群的返回，不确保消息发送成功。安全性最低但是效 率最高
        - 1代表producer往集群发送数据只要leader应答就可以发送下一条，只确保leader发送成功。
        - all代表producer往集群发送数据需要所有的follower都完成从leader的同步才会发送下一条，确保 leader发送成功和所有的副本都完成备份。安全性最高，但是效率最低
    * topic和数据日志
        - topic 是同⼀类别的消息记录（record）的集合，一个topic可以分为多个分区，每个partition都是⼀个有序并且不可变的消息记录集合。当新的数据写⼊时，就被追加到partition的末尾。在每个partition中，每条消息都会被分配⼀个顺序的唯⼀标识，这个标识被称为offset，即偏移量（Offset 就是 Kafka 分区里每条消息的唯一“下标”，消费者用它来精确记录自己读到哪了）。注意，Kafka只保证在同⼀个partition内部消息是有序的，在不同partition之间，并不能保证消息有序
        ![](2025-06-22-20-57-37.png)
        - Kafka可以配置⼀个保留期限，⽤来标识⽇志会在Kafka集群内保留多⻓时间。Kafka集群会保留在保留 期限内所有被发布的消息，不管这些消息是否被消费过。⽐如保留期限设置为两天，那么数据被发布到 Kafka集群的两天以内，所有的这些数据都可以被消费。当超过两天，这些数据将会被清空，以便为后 续的数据腾出空间
    * partition结构：Partition在服务器上的表现形式就是⼀个⼀个的⽂件夹，每个partition的⽂件夹下⾯会有多组segment⽂件，每组segment⽂件⼜包含.index⽂件、.log ⽂件、.timeindex⽂件三个⽂件，其中.log ⽂件就是实际存储message的地⽅，⽽.index和.timeindex⽂件为索引⽂件，⽤于检索消息
    ![](2025-06-22-21-03-15.png) 
    * 消费数据：多个消费者实例可以组成⼀个消费者组cosumer group，并⽤⼀个标签来标识这个消费者组。⼀个消费者组中的不同消费者实例可以运⾏在不同的进程甚⾄不同的服务器上。如果所有的消费者实例都在同⼀个消费者组中，那么消息记录会被很好的均衡的发送到每个消费者实例。如果所有的消费者实例都在不同的消费组，即一个消费者为一组，那么每一条消息记录会被广播到每一个消费组实例
    ![](2025-06-22-20-58-25.png)
6. 自己关于kafka的总结：kafka实例（一个实例通常就是一个broker，多个broker组成集群）作为中间的消息队列，让上下游服务完全解耦，并且对于下游消费者可以根据不同的消费者组进行负载均衡。对于生产者发送的消息是有类别的，每一个消息类对应一个topic，而每一个topic对应多个分区，每个分区是有序的，分区可以在不同的broker上，即实现分布式存储。每一个分区可以有多个副本，也就是follwer，实现高可用性
7. sarama库操作kafka:
    ```go
    1. // 创建同步/异步生产者
    config := sarama.NewConfig()
    config.Producer.RequiredAcks = sarama.WaitForAll // 等待所有副本确认
    config.Producer.Retry.Max = 5                   // 发送失败重试次数
    config.Producer.Return.Successes = true         // 接收发送成功通知
    2. // 生产消息
    producer, err := sarama.NewSyncProducer([]string{"localhost:9092"}, config)
    msg := &sarama.ProducerMessage{
        Topic: "test-topic",
        Value: sarama.StringEncoder("Hello Kafka!"),
    }
    partition, offset, err := producer.SendMessage(msg) // 返回分区和偏移量
    3. // 异步生产消息
    asyncProducer, _ := sarama.NewAsyncProducer(brokers, config)
    asyncProducer.Input() <- &sarama.ProducerMessage{Topic: "test", Value: sarama.StringEncoder("Async Msg")}
    select {
        case <-asyncProducer.Successes(): // 发送成功
        case err := <-asyncProducer.Errors(): // 发送失败
    }
    4. // 直连消费者，非消费者组的模式
    consumer, err := sarama.NewConsumer([]string{"127.0.0.1:9092"}, nil)
    partitionList, err := consumer.Partitions("web_log") // 根据topic取到所有的分区
    for partition := range partitionList { // 遍历所有的分区
        // 针对每个分区创建一个对应的分区消费者
        pc, err := consumer.ConsumePartition("web_log", int32(partition), sarama.OffsetNewest)
        defer pc.AsyncClose()
        // 异步从每个分区消费信息
        go func(sarama.PartitionConsumer) {
            for msg := range pc.Messages() {
                fmt.Printf("Partition:%d Offset:%d Key:%v Value:%v", msg.Partition, msg.Offset, msg.Key, msg.Value)
            }
        }(pc)
    }
    ```
8. kafka只能按照topic和分区进行过滤，对于通过用户id进行过滤就只能在生产端或消费端，生产端可以让每个分区对应一个用户id；要么只能在消费端，消费者订阅topic（其实就是消费topic）后，对消费到的消息进行判断，只处理符合指定用户id的消息
9. 一个broker就是一个独立的kafka服务实例（broker就是指部署了kafka实例的服务器节点），在大多数情况下，一个服务器上运行一个kafka broker实例
10. 消费kafka就是订阅kafka服务中的某个topic
11. kafka是基于磁盘的