#ifndef CONNECTION_POOL_H
#define CONNECTION_POOL_H

#include <mysql/mysql.h>
#include <list>
#include <string>
#include <semaphore.h>
#include <mutex>
using namespace std;

class connection_pool {
public:
    // --- API 按钮 1：获取唯一实例（单例模式） ---
    static connection_pool *GetInstance();

    // --- API 按钮 2：初始化池子 ---
    void init(std::string url, std::string User, std::string Password, 
              std::string DataBaseName, int Port, int MaxConn); 

    // --- API 按钮 3：借出一个连接 ---
    MYSQL *GetConnection();				 

    // --- API 按钮 4：归还一个连接 ---
    bool ReleaseConnection(MYSQL *conn); 

    // --- API 按钮 5：销毁池子 ---
    void DestroyPool();					 

private:
    connection_pool(){}
    ~connection_pool(){}

    int m_MaxConn;				// 最大连接数
    int m_CurConn;				// 当前已使用的连接数
    int m_FreeConn;				// 当前空闲的连接数
    
    list<MYSQL *> connList; // 连接池（容器），用来放 MYSQL 指针
    mutex m_lock;           // 互斥锁，保证多个线程
    sem_t reserve;          // 信号量
};

class connectionRAII {
public:
    connectionRAII(MYSQL **con, connection_pool *connPool) {
        *con = connPool->GetConnection();
        conRAII = *con;
        poolRAII = connPool;
    }
    // 析构时（函数结束时）自动放回池子
    ~connectionRAII() {
        poolRAII->ReleaseConnection(conRAII);
    }
private:
    MYSQL *conRAII;
    connection_pool *poolRAII;
};
#endif