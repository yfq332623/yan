高性能 C++ 开发实战
本项目是基于 C++14 编写的轻量级 Web 服务器实现登录和访问静态页面功能，采用 Reactor 模型，旨在解决高并发场景下的资源调度与连接管理问题 。

🌟 核心特性 (Key Features)

高并发：Epoll (ET模式) + 非阻塞 I/O + 线程池 。

资源池：单例 MySQL 连接池（RAII 管理）+ LRU 缓存优化文件读取 。

健壮性：升序双向链表定时器清理死连接 + 异步日志系统 。

🛠️ 快速上手 (Quick Start)

```bash
# 1. 编译（需已安装 CMake、g++、MySQL 开发库如 libmysqlclient-dev）
mkdir -p build && cd build && cmake .. && make

# 2. 运行（必须在项目根目录执行，否则无法找到 www/、count.txt 等资源）
cd /path/to/myweb
./build/server

# 3. 测试
# 浏览器访问 http://127.0.0.1:8888
# 例如：http://127.0.0.1:8888/ 、 http://127.0.0.1:8888/fudan、http://127.0.0.1:8888/login(实现登录)
```

📂 核心代码导读

- `include/webServer.h`：事件监听与分发中心
- `include/connection_pool.h`：数据库连接池（RAII 管理）
- `include/log.h`：异步日志生产/消费者模型
- `include/util_timer.h`：升序链表实现的 15s 定时器
- `src/`：所有 .cpp 实现
- `www/`：静态资源（如 index.html）