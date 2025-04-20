# 轻量级 KV 存储系统

一个简单但功能完整的键值（KV）存储系统。

## 特性

- 轻量级设计，容易理解和扩展
- 多种存储引擎选择（内存、文件）
- 简单的命令行客户端接口
- 支持数据持久化
- 支持 gRPC 服务端和客户端
- 支持批量操作（批量获取、设置、删除）
- 线程安全实现
- 性能基准测试

## 系统架构

### 存储引擎

系统支持以下存储引擎类型：

1. **内存存储（MemoryStorage）**:
   - 数据仅保存在内存中，重启后数据丢失
   - 适用于临时数据或高性能场景

2. **文件存储（FileStorage）**:
   - 数据持久化到文件
   - 系统重启后数据保留
   - 支持自动加载和保存

### 通信方式

系统支持两种通信方式：

1. **基本 TCP 套接字**:
   - 服务器和客户端使用简单的 TCP 套接字通信
   - 实现基本的请求 - 响应模型
   - 客户端发送文本格式的命令（如 "GET key"）
   - 服务器处理请求并返回响应（如 "SUCCESS 值" 或 "ERROR 键不存在"）

2. **gRPC 接口**:
   - 基于 Protobuf 的接口定义
   - 支持异步 gRPC 服务
   - 提供高性能的二进制通信

### 客户端接口

系统提供两种客户端接口：

1. **命令行客户端**:
   - 交互式操作界面
   - 支持的命令：get、put、del、mget、mput、mdel

2. **库客户端**:
   - C++ API 接口
   - 可集成到其他应用程序中

## 基本命令

命令行客户端支持以下命令：

- `get <key>` - 获取键值
- `put <key> <value>` - 设置键值对
- `del <key>` - 删除键值对
- `mget <key1> <key2> ...` - 批量获取多个键值
- `mput <key1> <value1> <key2> <value2> ...` - 批量设置多个键值对
- `mdel <key1> <key2> ...` - 批量删除多个键
- `exit` - 退出客户端

## 构建和运行

项目使用 Bazel 构建系统。

### 构建项目

```bash
# 构建所有组件
./run.sh
```

### 运行服务端

```bash
# 使用内存存储（默认）
./bin/kv_server_main --ip=127.0.0.1 --port=8080 --storage_type=memory

# 使用文件存储
./bin/server/kv_server_main --ip=127.0.0.1 --port=8080 --storage_type=file --storage_path=data.db
```

### 运行客户端

```bash
# 连接到服务端
./bin/kv_client_main --server_ip=127.0.0.1 --server_port=8080
```

### 运行 gRPC 服务端和客户端

```bash
# 启动 gRPC 服务端
./bin/grpc_kv_server_main

# 启动 gRPC 客户端
./bin/grpc_kv_client_main
```

## 项目结构

```
tiny_kv_storage/
├── src/
│   ├── server/         - 基本 TCP 服务端实现
│   ├── client/         - 基本 TCP 客户端实现
│   ├── grpc_server/    - gRPC 服务端实现
│   ├── grpc_client/    - gRPC 客户端实现
│   ├── common/         - 公共组件（存储引擎、缓存等）
│   ├── proto/          - Protobuf 定义
│   └── benchmark/      - 性能测试
├── third_party/        - 第三方依赖
└── WORKSPACE           - Bazel 工作空间定义
```

## 作者

- lutongjia (<tobijah@163.com>)
