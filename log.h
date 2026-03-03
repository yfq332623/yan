#ifndef LOG_H
#define LOG_H

#include<deque>
#include<mutex>
#include<condition_variable>
#include <cstdio>       // fopen, fclose, fputs, snprintf 等 C 风格 IO
#include <iostream>     // 虽然我们主用 fputs，但有时调试会用到
#include <string>       // std::string
#include <cstring>      // memset, strcpy, strrchr
#include <stdarg.h>     // 核心！处理 write_log 里的 ... 变长参数 (va_list)
#include <ctime>        // time, localtime 等时间处理
#include <thread>       
#include <sys/time.h>
#include <unistd.h>

using namespace std;
template <typename T>

class block_queue{
public:
   block_queue(int max_size =1000):m_max_size(max_size){};

   bool full() {
        m_mutex.lock();
        if (m_queue.size() >= m_max_size) {
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }


   bool push(const T& item){
        unique_lock<mutex> lock(m_mutex);
    if(m_queue.size()>=m_max_size){
        m_cond.notify_all();
        return false;
    }
    m_queue.push_back(item);
    m_cond.notify_one();
    return true;
   };
   bool pop(T& item){
    unique_lock<mutex> lock(m_mutex);
    while(m_queue.empty()){
        m_cond.wait(lock);
    }
    item = m_queue.front();
    m_queue.pop_front();
    return true;
   }
private:
   int m_max_size;
   mutex m_mutex;
   condition_variable m_cond;
   deque<T> m_queue;
};

class Log {
public:
    // 单例模式：获取唯一实例的接口
    static Log* get_instance() {
        static Log instance;
        return &instance;
    }

    // 异步写日志的辅助函数（传给线程的入口）
    static void flush_log_thread() {
        Log::get_instance()->async_write_log();
    }

    // 初始化函数（参数：文件名，缓冲区大小，最大行数，队列容量）
    bool init(const char *file_name, int log_buf_size = 2000, int split_lines = 5000000, int max_queue_size = 0);

    // 前台写日志入口（那个 ... 就是变长参数语法）
    void write_log(int level, const char *format, ...);

    // 强制刷新缓冲区
    void flush(void);

private:
    Log();           // 构造函数私有
    virtual ~Log();  // 析构函数私有
    
    // 后台线程真正执行的函数
    void async_write_log();

private:
    char dir_name[128];     // 日志文件所在的路径目录
    char log_name[128];     // 日志文件名
    int m_split_lines;      // 日志最大行数
    int m_log_buf_size;     // 日志缓冲区的大小
    long long m_count;      // 已写入的日志行数记录
    int m_today;            // 记录当前是哪一天（按天分类日志）
    FILE *m_fp;             // 打开的日志文件指针
    char *m_buf;            // 排版用的草稿纸（缓冲区）
    block_queue<string> *m_log_queue; // 指向传送带（阻塞队列）的指针
    bool m_is_async;        // 异步/同步的切换开关
    mutex m_mutex;          // 保护文件操作或成员变量的互斥锁
};

#endif