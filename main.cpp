#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<cstring>
#include<fstream>
#include<sstream>
#include<thread>
#include<mutex>
#include<unistd.h>
#include<vector>
#include<queue>
#include<condition_variable>
#include<sys/epoll.h>
#include<fcntl.h>
#include<memory>
#include<fcntl.h>
#include<errno.h>
#include<cstdio>
#include<unordered_map>
#include"log.h"
#include"util_timer.h"
using namespace std;

class WebServer{
       struct ThreadArgs {
    int socket;
    WebServer* self;
    ThreadArgs(int s, WebServer* srv) : socket(s), self(srv) {}
};

    //变成非阻塞的工具函数
    void SetNonBlocking(int fd){
        int flags = fcntl(fd,F_GETFL,0); // 1. 获取文件描述符当前的状态标志
        if(flags == -1) perror("fcntl F_GETFL");
        if(fcntl(fd,F_SETFL,flags|O_NONBLOCK)==-1) perror("fcntl F_SETFL");
    }
public:
   WebServer(int port,int thread_num = 4):_port(port),_server_fd(-1){
    for(int i=0;i < thread_num;++i){
            _workers.emplace_back([this]{               //创建一个线程,形成线程池模式
                while(true){
                    unique_ptr<ThreadArgs> arg = nullptr;
                    {
                    unique_lock<mutex> lock(this->queue_mtx);
                    this->_cv.wait(lock,[this]{
                        return this->stop || !this->_tasks.empty();//若没有stop和任务为空则一直休眠线程卡在这里
                    });
                    if(this->_tasks.empty() && this->stop) return;//当stop后被叫醒,若任务为空则返回结束线程
                    arg = move(this->_tasks.front());
                    this->_tasks.pop();
                }
                 if(arg){
                    handle_client(move(arg));
                }
            }
            });
        }
    }

    ~WebServer(){
        {
            lock_guard<mutex> lock(queue_mtx);
            stop = true;
        }
        _cv.notify_all();
        for(auto& t : _workers){
            if(t.joinable())
            t.join();
        }
    }
   

   bool init(){
    //创建异步日志系统
    if(!Log::get_instance()->init("server.log",2000,800000,800)){
        return false;
    }
    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(_server_fd < 0){
        return false;
    }
    int opt =1;

    //允许服务器立刻重启时,不用等操作系统回收端口

    if(setsockopt(_server_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt)) < 0){
        perror("setsockopt error");
        return false;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(_port);

    if(bind(_server_fd,(struct sockaddr*)&address,sizeof(address))<0){return false;}
    if(listen(_server_fd , 5 )<0){
        return false;
    }

    //I/O复用epoll定义
    _epoll_fd = epoll_create(1);
    if(_epoll_fd ==-1){
        return false;
    }
    struct epoll_event ev;
    ev.data.fd = _server_fd;
    ev.events = EPOLLIN;
    if(epoll_ctl(_epoll_fd,EPOLL_CTL_ADD,_server_fd,&ev)==-1) return false;

    ifstream is("count.txt");
        if(is.is_open()){
            is >> visit_count;
            is.close();
        }
        Log::get_instance()->write_log(0, "Epoll Server Init Success. Port: %d", _port);
        cout<<"Epoll 服务器初始化成功，监听端口：127.0.0.1:"<< _port << endl;
        return true;
    }


   void start(){
        struct epoll_event events[1024];  
        while(true){                   //开始epoll监控循环
            int n = epoll_wait(_epoll_fd,events,1024,5000); //存放请求个数,参数"-1"表示永久阻塞,就是一直会等着,来数据了就响应
                                                    //改成5000ms就会醒来一次
            
            timer_lst.tick();//先检测定时器有没有超时

            for(int i= 0;i< n;++i){
                int active_fd = events[i].data.fd;//定义一个active来收取监控里的编号
                if(active_fd == _server_fd){      //如果收取到的编号是新的
                    int new_socket = accept(_server_fd,NULL,NULL);
                    if(new_socket >= 0){
                        SetNonBlocking(new_socket);

                    util_timer* timer = new util_timer();
                    timer->user_data = new client_data(); 
                    timer->user_data->socket_fd = new_socket;
                    timer->expire = time(NULL) + 15; // 15秒不说话就踢掉
                    timer->cb_fun = cb_func;

                    timer_lst.add_timer(timer); // 进名单
                    fd_to_timer[new_socket] = timer; // 进索引

                    }else{
                        continue;
                    }


                    struct epoll_event client_ev;   //建立新的监控来记录new_socket
                    client_ev.data.fd = new_socket;
                    client_ev.events = EPOLLIN|EPOLLET;//ET模式下需要的方式也就是非阻塞边缘触发
                    epoll_ctl(_epoll_fd,EPOLL_CTL_ADD,new_socket,&client_ev);
                    cout << "新客进店(ET模式): " << new_socket << endl;
                }
                
                else{                           //如果收取到的编号是老的
                    if (fd_to_timer.count(active_fd)) {
                    util_timer* timer = fd_to_timer[active_fd];
                    timer->expire = time(NULL) + 15; // 重新给 15 秒
                    timer_lst.adjust_timer(timer);
                    Log::get_instance()->write_log(0, "Timer adjusted for fd %d", active_fd);
                }


                     epoll_ctl(_epoll_fd,EPOLL_CTL_DEL,active_fd,NULL);
                    auto arg =make_unique<ThreadArgs>(active_fd,this);
                    {
                        lock_guard<mutex> lock(queue_mtx);
                        _tasks.push(move(arg));
                    }
                       _cv.notify_one();
                }
            }
        }
    }

static void cb_func(client_data* user_data){
    Log::get_instance()->write_log(1, "Timer expired: close fd %d", user_data->socket_fd);
    epoll_ctl(user_data->timer->prev ? -1 : -1, EPOLL_CTL_DEL, user_data->socket_fd, 0);
    close(user_data->socket_fd);
}
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
   unordered_map<int , string> client_buffers;
   sort_timer_lst timer_lst;
   unordered_map<int, util_timer*> fd_to_timer;
   static int pipefd[2];

//删除货架的函数,删map,删监控,关闭连接

   void close_connection(int fd){
    lock_guard<mutex> lock(map_mtx);
    client_buffers.erase(fd);
    epoll_ctl(_epoll_fd,EPOLL_CTL_DEL,fd,NULL);
    close(fd);
   }

   static void handle_client(unique_ptr<ThreadArgs> arg){
    int client_socket = arg->socket;
    WebServer* server = arg->self;
    char buffer[1024]={0};
    string &data = server->client_buffers[client_socket];
    //string request_data;
    while(true){

        //非阻塞I/O下的读取逻辑

        int bytes_read =read(client_socket,buffer,sizeof(buffer)-1);
        if(bytes_read > 0){

            //只要能读取到数据就 > 0,然后接在request_data后面

            buffer[bytes_read] = '\0';       //一种增强性能的方式,再要拷贝的字符数组最后加'\0'
            data.append(buffer);
        }

        if(bytes_read == 0) {
            server->close_connection(client_socket);
             return;
        }

        if(bytes_read == -1){
            if(errno==EAGAIN || errno == EWOULDBLOCK)

            //内核缓冲区空了,数据被读完了
                break;
            if(errno ==EINTR)
            //被系统打断，再试一次
            continue;
            else{
                server->close_connection(client_socket);
                return;
            }
        }
        }
        //如果啥也没有发送
        if(data.empty()){
            server->close_connection(client_socket);
            return;
        }
    
    
    int state = 0; //定义状态
    string path = "index.html";
    while(true){
        size_t pos = data.find("\r\n");   //先判定数据是否存在第一行
        if(pos ==string::npos) return;    //不存在直接return
        string line = data.substr(0,pos); //取第一行
        data.erase(0,pos+2);              //取完之后消除第一行更新第一行内容
        if(state == 0){
            size_t p1 = line.find(" ");
            size_t p2 = line.find(" ",p1+1);
            if(p1!= string::npos && p2!= string::npos){
                path = line.substr(p1+1,p2-p1-1);
                Log::get_instance()->write_log(0, "Extract Path: %s", path.c_str());
                if(path=="/"||path==""){
                    path = "index.html";
                }
                state = 1;
            }
        }else if(state == 1){
            if(line.empty()) break;
        }
    }
{
    lock_guard<mutex> lock(server->map_mtx);
    server->client_buffers.erase(client_socket);
}

    if(path.find("favicon.ico") != string::npos){
        server->close_connection(client_socket);
        return;
    }

    if(path!="index.html"){
        path += ".html";
    }
    string filename ="./www/" + path;

    string status = "200 OK";
    string content;
    cout << "Debug: 正在尝试打开文件 -> " << filename << endl;
    ifstream f(filename);
    if(!f.is_open()){
        status = "404 Not Found";
        f.clear();
        f.open("./www/404.html");
    }
    
    if(f.is_open()){
        stringstream ss;
        ss << f.rdbuf();
        content = ss.str();
        if(path=="index.html"){
            string targe ="{{COUNT}}";
            size_t pos =content.find(targe);
            if(pos!=string ::npos){
                content.replace(pos,targe.length(),to_string(server->visit_count + 1));
            }
        }
    }else{
        content = "<html><body><h1>404 Page Missing</h1></body></html>";
    }

    string header = "HTTP/1.1 " + status + "\r\nContent-Type: text/html; Charset=UTF-8\r\n";
    header += "Content-Length: " + to_string(content.size()) + "\r\n\r\n";
    string response = header + content;

    if(send(client_socket,response.c_str(),response.size(),0) > 0){
        server->visit_mtx.lock();
        server->visit_count++;
        cout << "访客+1，总数: " << server->visit_count << endl;
        ofstream out("count.txt");
        out << server->visit_count;
        server->visit_mtx.unlock();

       Log::get_instance()->write_log(0, "Path: %s | Total Visits: %d", path.c_str(), server->visit_count);
        
    }

    server->close_connection(client_socket);
    }
};

int main(){
    WebServer my_server(8888);
    if(my_server.init()){
        my_server.start();
    }else{
        perror("服务器启动失败");
    }
    return 0;
}