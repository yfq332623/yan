#include"log.h"

bool Log::init(const char *file_name, int log_buf_size, int split_lines, int max_queue_size) {
    
    if (max_queue_size >= 1) {
        m_is_async = true; 
        m_log_queue = new block_queue<string>(max_queue_size);
        std::thread tid(flush_log_thread);
        tid.detach();
    }

 
    m_log_buf_size = log_buf_size; 
    m_buf = new char[m_log_buf_size]; 
    memset(m_buf, '\0', m_log_buf_size); 
    m_split_lines = split_lines; 

    time_t t = time(NULL); 
    struct tm *sys_tm = localtime(&t); 
    struct tm my_tm = *sys_tm; 
    const char *p = strrchr(file_name, '/'); 
    char log_full_name[512] = {0}; 

    if (p == NULL) {
        snprintf(log_full_name, 511, "%d_%02d_%02d_%s", 
                 my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
    } else {

        strcpy(log_name, p + 1); 
        strncpy(dir_name, file_name, p - file_name + 1);
        snprintf(log_full_name, 511, "%s%d_%02d_%02d_%s", 
                 dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name);
    }

    m_today = my_tm.tm_mday; 

    m_fp = fopen(log_full_name, "a");
    if (m_fp == NULL) {
        fprintf(stderr, "Log Init Error: %s\n", strerror(errno));
        return false; 
    }
    return true; 
}

void Log::write_log(int level, const char *format, ...) {
    // 1. 获取微秒级时间
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    // 2. 确定日志级别标签
    char s[16] = {0};
    switch (level) {
        case 0: strcpy(s, "[debug]:"); break;
        case 1: strcpy(s, "[info]:");  break;
        case 2: strcpy(s, "[warn]:");  break;
        case 3: strcpy(s, "[error]:"); break;
        default: strcpy(s, "[info]:"); break;
    }

    // 3. 开始写草稿（这里要锁一下，因为大家共用 m_buf）
    m_mutex.lock();
    m_count++; // 记录又写了一行

    // 4. 先在缓冲区写时间戳和标签
    int n = snprintf(m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                     my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);
    
    // 5. 处理变长参数 ... (将你传入的内容拼接到时间后面)
    va_list arg_list;
    va_start(arg_list, format);
    // 从 m_buf+n 的位置开始写，剩余空间要扣除已写的 n 和最后的换行符
    int m = vsnprintf(m_buf + n, m_log_buf_size - n - 1, format, arg_list);
    va_end(arg_list);

    // 6. 封口：换行并结束
    m_buf[n + m] = '\n';
    m_buf[n + m + 1] = '\0';
    string log_str = m_buf;
    m_mutex.unlock();

    // 7. 投递决策
    if (m_is_async && !m_log_queue->full()) {
        // 异步模式：扔进队列，秒回业务逻辑
        m_log_queue->push(log_str);
    } else {
        // 同步模式：主线程亲自拿锁写磁盘（保底逻辑）
        m_mutex.lock();
        fputs(log_str.c_str(), m_fp);
        printf("Synchronous write!\n");
        m_mutex.unlock();
    }
}

void Log::async_write_log() {
    string single_log;
    while (m_log_queue->pop(single_log)) {
        m_mutex.lock();
        fputs(single_log.c_str(), m_fp);
        m_mutex.unlock();
        usleep(1000);
        fflush(m_fp);
    }
}

void Log::flush(void) {
    m_mutex.lock();
    fflush(m_fp);
    m_mutex.unlock();
}

Log::~Log() {
    if (m_fp != NULL) {
        fclose(m_fp);
    }
    if (m_buf != NULL) {
        delete [] m_buf;
    }
}

Log::Log() { m_count = 0; m_is_async = false; m_fp = nullptr; m_buf = nullptr; }