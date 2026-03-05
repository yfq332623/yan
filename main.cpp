#include "webServer.h"
int main(){
    WebServer my_server(8888);
    if(my_server.init()){
        my_server.start();
    }else{
        perror("服务器启动失败");
    }
    return 0;
}