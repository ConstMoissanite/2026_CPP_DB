# MiniDB — 微型关系型数据库管理系统

《现代 C++ 程序设计》课程项目。对标 MySQL，采用 C/S 架构，使用 C++20/23 从零实现 SQL 解析、存储引擎、执行引擎和 TCP/IP 网络层。**不依赖任何 STL 容器。**

## 快速开始

### 环境要求

- **编译器**：clang++ 或 g++（支持 C++20）
- **构建工具**：CMake ≥ 3.16
- **目标平台**：Linux（评分环境）/ Windows（开发环境）

### 构建

```bash
# 配置
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# 编译
cmake --build build
```

### 运行测试

```bash
# 运行全部测试
cd build && ctest --output-on-failure

# 或单独运行
./build/test_lexer      # 词法分析器测试
./build/test_parser     # 语法分析器测试
./build/test_c_api      # C API 测试
./build/test_storage    # 存储引擎测试
./build/test_executor   # 执行引擎测试
```

### 启动服务

```bash
# 启动服务器（默认端口 3307）
./build/minidb_server

# 启动客户端
./build/minidb_client
```

## 使用示例

```
minidb> create database mydb
OK
  Statement: CREATE DATABASE

minidb> use mydb
OK

minidb> create table person (id int primary, name string)
OK
  Statement: CREATE TABLE

minidb> insert into person values (1001, "peter")
OK

minidb> insert into person values (1002, "mary")
OK

minidb> select * from person
OK
  name: id       type: int
  name: name     type: string

minidb> select name from person where id = 1001
OK
  name: name     type: string

minidb> update person set name = "alice" where id = 1001
OK

minidb> delete from person where id = 1002
OK

minidb> exit
```

## 支持的 SQL 语法

### DDL（数据定义语言）

| 语句 | 语法 |
|------|------|
| 创建数据库 | `CREATE DATABASE <dbname>` |
| 删除数据库 | `DROP DATABASE <dbname>` |
| 切换数据库 | `USE <dbname>` |
| 创建表 | `CREATE TABLE <name> (<col> <type> [PRIMARY], ...)` |
| 删除表 | `DROP TABLE <name>` |

### DML（数据操作语言）

| 语句 | 语法 |
|------|------|
| 查询 | `SELECT <col>\|* FROM <table> [WHERE <col> <op> <value>]` |
| 插入 | `INSERT INTO <table> VALUES (<val>, ...)` |
| 更新 | `UPDATE <table> SET <col> = <val> [WHERE <col> <op> <val>]` |
| 删除 | `DELETE FROM <table> [WHERE <col> <op> <val>]` |

### 限制

- **数据类型**：仅支持 `int`（C++ 默认整型）和 `string`（最长 256 字符，UTF-8）
- **WHERE 运算符**：仅支持 `=`、`<`、`>`
- **字符串字面量**：使用双引号（`"peter"`），而非单引号
- **标识符**：全小写英文字母，不含下划线和特殊字符
- **分号**：语句末尾分号可选

## 架构

```
┌──────────────┐     TCP/IP (JSON)     ┌──────────────┐
│  客户端 CLI  │ ◄──────────────────► │  服务器      │
│  (REPL界面)  │                       │  (解析+执行) │
└──────────────┘                       └──────┬───────┘
                                              │
                    ┌─────────────────────────┼─────────────────────────┐
                    │                         ▼                         │
                    │  ┌──────────┐  ┌──────────────┐  ┌────────────┐  │
                    │  │  词法分析 │─►│  语法分析     │─►│  执行引擎  │  │
                    │  │  Lexer   │  │  Parser      │  │  Executor  │  │
                    │  └──────────┘  └──────────────┘  └─────┬──────┘  │
                    │                                        │         │
                    │    ┌───────────────────────────────────┘         │
                    │    ▼                                               │
                    │  ┌──────────────┐                                │
                    │  │   存储引擎    │                                │
                    │  │  Database    │                                │
                    │  │  ├─ Table    │  ← 文件持久化 (.dat)           │
                    │  │  ├─ Index    │  ← 有序数组 + 二分查找          │
                    │  │  ├─ Row      │                                │
                    │  │  └─ Value    │  ← int | string 判别联合        │
                    │  └──────────────┘                                │
                    └──────────────────────────────────────────────────┘
```

### 库依赖关系

```
minidb_parser    词法/语法分析 + C ABI 封装
       │
minidb_storage   文件持久化、表/行/列/索引管理
       │
minidb_execution SQL 执行引擎（DDL + DML）
       │
minidb_network   TCP 服务器/客户端、JSON 协议序列化
```

### 存储格式

```
data/                          # 数据根目录
├── mydb/                      # 数据库目录
│   ├── person.dat             # 表文件（纯文本）
│   └── person.idx             # 索引文件（预留）
└── other/
```

表文件格式（纯文本）：
```
2                              # 列数
id 0 1                         # name type(0=int,1=string) is_primary
name 1 0
2                              # 行数
1001|peter                     # 值用 '|' 分隔
1002|mary
```

### 通信协议

客户端与服务器通过 TCP 以 **换行分隔的 JSON** 进行通信：

- **请求**：`{"type":"sql","sql":"<SQL 语句>"}`
- **退出**：`{"type":"exit"}`
- **成功响应**：`{"status":"ok","message":"...","columns":[...],"rows":[...]}`
- **错误响应**：`{"status":"error","message":"..."}`

## 项目结构

```
include/minidb/
  parser.h              # 公有 C ABI 头文件
src/
  core/
    string.hpp           # 自定义 String 类（替代 std::string）
    vector.hpp           # 自定义 Vector<T> 模板（替代 std::vector）
  parser/
    token.hpp/.cpp       # Token 定义（关键字、标识符、字面量、运算符）
    lexer.hpp/.cpp       # 手写词法分析器（大小写不敏感）
    ast.hpp/.cpp         # AST 节点定义（9 种语句类型）
    parser.hpp/.cpp      # 递归下降语法分析器
    parser_c.cpp         # C ABI 实现层
  storage/
    column.hpp/.cpp      # 列（名称、类型、是否主键）
    row.hpp/.cpp         # 行 + Value 判别联合（int | string）
    table.hpp/.cpp       # 表（Schema + 数据 + 文件读写）
    index.hpp/.cpp       # 索引（有序数组 + 二分查找）
    database.hpp/.cpp    # 数据库（表集合 + 全局注册表）
  execution/
    executor.hpp/.cpp    # 执行引擎（DDL/DML 全部语句）
  network/
    server.hpp/.cpp      # TCP 服务器
    client.hpp/.cpp      # TCP 客户端
    protocol.hpp/.cpp    # JSON 协议序列化/反序列化
  cli/
    server_main.cpp      # 服务器入口
    client_main.cpp      # 客户端入口（REPL）
tests/
  test_lexer.cpp         # 词法分析器测试（13 项）
  test_parser.cpp        # 语法分析器测试（25 项）
  test_c_api.cpp         # C API 测试（18 项）
  test_storage.cpp       # 存储引擎测试
  test_executor.cpp      # 执行引擎测试
```

## 编码规范

- **禁止使用 STL 容器**（课程硬性要求），使用 `minidb::core::Vector<T>` 和 `minidb::core::String` 替代
- **C++ 标准**：C++20，尽量到 C++23
- **命名空间**：`minidb::core` → `minidb::parser` → `minidb::storage` → `minidb::execution` → `minidb::network`
- **私有成员**：以下划线前缀命名（`_data`, `_size`, `_pos`）
- **头文件保护**：`MINIDB_MODULE_FILE_HPP` 风格
- **测试框架**：手写宏，无外部依赖
- **编译器警告**：`-Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wcast-align -Wconversion -Wsign-conversion -Wnull-dereference`

## 课程信息

- **课程**：《现代 C++ 程序设计》（2025–2026 第二学期）
- **截止日期**：2026 年 6 月 21 日
- **提交方式**：将源码、报告、评分表打包为 `.zip` 发送至课程邮箱
- **详细需求**：见 `req export.md`
