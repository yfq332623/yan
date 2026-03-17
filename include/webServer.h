#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <unistd.h>
#include <vector>
#include <queue>
#include <condition_variable>
#include <sys/epoll.h>
#include <fcntl.h>
#include <memory>
#include <errno.h>
#include <cstdio>
#include <unordered_map>
#include"connection_pool.h"
#include "log.h"
#include "util_timer.h"
#include "http.h"

using namespace std;

class WebServer {
    friend bool Parse(std::string& data, HttpRequest& req, int client_socket, WebServer* server);
    struct ThreadArgs {
        int socket;
        WebServer* self;
        ThreadArgs(int s, WebServer* srv) : socket(s), self(srv) {}
    };

    // 变成非阻塞的工具函数
    void SetNonBlocking(int fd);

public:
    WebServer(int port, int thread_num = 4);
    ~WebServer();

    bool init();
    void start();

    static void cb_func(client_data* user_data);

private:
    int _port;
    int _server_fd;
    int visit_count = 0;
    int _epoll_fd = -1;
    mutex visit_mtx; //计数器互斥锁
    mutex queue_mtx; //任务队列互斥锁
    mutex conn_mtx; //连接相关 map 互斥锁
    vector<thread> _workers; //线程池
    queue<unique_ptr<ThreadArgs>> _tasks;  //任务队列
    condition_variable _cv; 
    bool stop = false; //线程池停止标志
    unordered_map<int, string> client_buffers;  //缓存读取http数据的缓冲区
    sort_timer_lst timer_lst; //处理定时超时的升序链表
    unordered_map<int, util_timer*> fd_to_timer;//索引判断fd这个链接闹钟在不在
    static int pipefd[2]; 

    // 关闭连接：清 map、摘 epoll、关 fd；可选是否由定时器回调调用（避免重复 delete 定时器节点）
    void close_connection(int fd, bool from_timer = false);

    static void handle_client(unique_ptr<ThreadArgs> arg);
};

#endif
