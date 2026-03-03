高性能 C++ 开发实战
本项目是基于 C++14 编写的轻量级 Web 服务器，采用 Reactor 模型，旨在解决高并发场景下的资源调度与连接管理问题 。

🌟 核心特性 (Key Features)

高并发：Epoll (ET模式) + 非阻塞 I/O + 线程池 。

资源池：单例 MySQL 连接池（RAII 管理）+ LRU 缓存优化文件读取 。

健壮性：升序双向链表定时器清理死连接 + 异步日志系统 。

🛠️ 快速上手 (Quick Start)
Bash
# 1. 编译 (Makefile 已配置 C++14 与库链接)
make 

# 2. 运行
./server

# 3. 测试
访问 http://127.0.0.1:8888 
    也可加访问127.0.0.1:8888/fudan 可以添加/...

📂 核心代码导读
main.cpp: 事件监听与分发中心。

connection_pool.h: 数据库车队调度逻辑。

log.h: 异步日志生产/消费者模型。

util_timer.h: 升序链表实现的15s定时器

www/: 存放静态资源（如 index.html）。