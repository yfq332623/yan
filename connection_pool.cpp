#include"connection_pool.h"
#include"log.h"

connection_pool* connection_pool::GetInstance() {
    static connection_pool connPool; // 静态局部变量，C++11 以后是线程安全的
    return &connPool;
}

void connection_pool::init(string url, string User, string Password, string DBName, int Port, int MaxConn) {
    m_MaxConn = MaxConn;

    for (int i = 0; i < m_MaxConn; i++) {
        MYSQL *con = nullptr;
        con = mysql_init(con); // 1. 初始化

        if (con == nullptr) {
            Log::get_instance()->write_log(3, "MySQL Error");
            exit(1);
        }

        // 2. 真正拨号连接
        con = mysql_real_connect(con, url.c_str(), User.c_str(), Password.c_str(), DBName.c_str(), Port, NULL, 0);

        if (con == nullptr) {
            Log::get_instance()->write_log(3, "MySQL Connect Error");
            exit(1);
        }

        // 3. 塞进池子
        connList.push_back(con);
        ++m_FreeConn;
    }

    // 4. 交警上岗：初始化信号量
    sem_init(&reserve, 0, m_FreeConn);  //带锁的一个计数器
}

MYSQL *connection_pool::GetConnection() {
    MYSQL *con = NULL;

    if (0 == connList.size()) return NULL;

    // A. 关键：等待信号量（如果没有卡了，线程会在这里排队）
    sem_wait(&reserve);

    // B. 加锁取连接，防止两个线程同时抢走同一个 MYSQL* 指针
    m_lock.lock();

    con = connList.front();
    connList.pop_front();

    --m_FreeConn;
    ++m_CurConn;

    m_lock.unlock();
    return con;
}

bool connection_pool::ReleaseConnection(MYSQL *con) {
    if (NULL == con) return false;

    m_lock.lock();

    connList.push_back(con); // 放回池子
    ++m_FreeConn;
    --m_CurConn;

    m_lock.unlock();

    // 关键：释放信号量（卡片放回，通知排队的人）
    sem_post(&reserve);
    return true;
}


void connection_pool::DestroyPool() {
    m_lock.lock();
    if (connList.size() > 0) {
        for (auto it = connList.begin(); it != connList.end(); ++it) {
            MYSQL *con = *it;
            mysql_close(con); // 关闭每一个 MySQL 连接
        }
        m_CurConn = 0;
        m_FreeConn = 0;
        connList.clear();
    }
    m_lock.unlock();
}