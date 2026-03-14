#include "webServer.h"
int main(){
    WebServer my_server(8888);
    if(my_server.init()){
        connection_pool::GetInstance()->init("localhost", "root", "332623yfq", "webserver_db", 3306, 5);
        my_server.start();
    }else{
        perror("服务器启动失败");
    }
    connection_pool::GetInstance()->DestroyPool();
    return 0;
}
