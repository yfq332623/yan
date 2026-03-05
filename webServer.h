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
#include "log.h"
#include "util_timer.h"

using namespace std;

class WebServer {
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
    mutex visit_mtx;
    mutex queue_mtx;
    mutex map_mtx;
    vector<thread> _workers;
    queue<unique_ptr<ThreadArgs>> _tasks;
    condition_variable _cv;
    bool stop = false;
    unordered_map<int, string> client_buffers;
    sort_timer_lst timer_lst;
    unordered_map<int, util_timer*> fd_to_timer;
    static int pipefd[2];

    // 删除货架的函数,删map,删监控,关闭连接
    void close_connection(int fd);

    static void handle_client(unique_ptr<ThreadArgs> arg);
};

#endif
