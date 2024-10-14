1. `net start mysql80`:这条命令在`Windows`命令提示符下运行,用来启动名为`mysql80`的`MySQL`服务(`mysql80`:这是`MySQL 8.0`的服务名称.不同的`MySQL`版本或者实例可以有不同的服务名,具体取决于安装时的配置.默认情况下,`MySQL 8.0`的服务名通常是 `mysql80`)
2. `mysql -u root -p`:按根用户进入`mysql`,需要输入密码
3. `SQL`中的字符串类型:`varchar()`<=>`char[]`
4. `SQL`语言分为四种:`DDL DML DQL DCL`
   * `DDL`:数据定义语言,用来定义数据库对象(数据库、表、字段)
   * `DML`:数据操作语言,用来对数据库表中的数据进行增删改
   * `DQL`:数据查询语言,用来查询数据库中表的记录
   * `DCL`:数据控制语言,用来创建数据库用户、控制数据库的访问权限
5. `DDL`-数据库操作:
   * `SHOW DATABASES;`:查询所有数据库
   * `SELECT DATABASE();`:查询当前正在使用的数据库
   * `CREATE DATABASE [IF NOT EXISTS] 数据库名 [DEFAULT CHARSET 字符集] [COLLATE 排序规则];`:创建数据库(`create database if not exists test`:不存在`test`数据库就创建)
   * `DROP DATABASE [IF EXISTS] 数据库名`:删除数据库
   * `USE 数据库名`:使用指定数据库  这个指令会切换到指定的数据库中去
6. `DDL`-表操作
   * 查询
     * `SHOW TABLES;`:查询当前数据库下的所有表
     * `DESC 表名;`:查询指定表的表结构
     * `SHOW CREATE TABLE 表名;`:查询指定表的建表语句
   * 创建
     * `CREATE TABLE 表名(字段1 字段1类型 [COMMENT 字段1注释,...]) [COMMENT 表注释]`:创建表