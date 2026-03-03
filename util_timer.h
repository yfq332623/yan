#ifndef UTIL_TIMER_H
#define UTIL_TIMER_H

#include<sys/socket.h>
#include<netinet/in.h>
#include <ctime>

using namespace std;
class util_timer;

struct client_data{
    sockaddr_in address;
    int socket_fd;
    util_timer* timer;
};

class util_timer{
public:
   time_t expire;
   void (*cb_fun)(client_data*);
   client_data* user_data;
   util_timer* next;
    util_timer*  prev;
   util_timer():next(nullptr),prev(nullptr){}
};

class sort_timer_lst{
public:
   sort_timer_lst():head(nullptr),tail(nullptr){}
   ~sort_timer_lst();

   void add_timer(util_timer* timer);
    
    // 调整定时器（当 client 发了数据，时间往后挪，需重新调整位置）
    void adjust_timer(util_timer* timer);
    
    // 删除定时器（手动断开连接时调用）
    void del_timer(util_timer* timer);
    
    // 核心
    void tick();
private:
    void add_timer(util_timer* timer, util_timer* lst_head);
    util_timer*  head;
    util_timer*  tail;
};

#endif