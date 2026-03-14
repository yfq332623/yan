#ifndef HTTP_H
#define HTTP_H

#include <string>
#include <unordered_map>
using namespace std;
class WebServer;

// 表示一次 HTTP 请求中目前关心的基础信息
struct HttpRequest {
    string method;  // GET / POST ...
    string path;    // 不带 query 的路径部分，例如 /index.html
    string query;   // ? 之后的查询字符串，例如 a=1&b=2，没有则为空

    // 简单保存本次请求的所有请求头：Key -> Value
    unordered_map<string, string> headers;

    // 请求体内容（目前不解析，只占位）
    string body;
};

// 解析 HTTP 请求：从 data 中解析出 HttpRequest，并完成日志、打印、清理 client_buffers。
// 返回 true 表示解析成功；返回 false 表示连接已关闭或解析失败，调用方应直接 return。
bool Parse(std::string& data, HttpRequest& req, int client_socket, WebServer* server);

#endif
