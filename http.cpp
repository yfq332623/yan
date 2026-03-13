#include "http.h"
#include "webServer.h"
#include "log.h"
#include <mutex>
#include <iostream>

using namespace std;

bool Parse(string& data, HttpRequest& req, int client_socket, WebServer* server) {
    //如果啥也没有发送
    if (data.empty()) {
        server->close_connection(client_socket);
        return false;
    }

    string raw_request = data;  // 保存完整报文，用于终端打印

    int state = 0; //定义状态：0 解析请求行，1 解析/跳过剩余头部
    req.method.clear();
    req.path = "index.html";
    req.query.clear();
    req.headers.clear();
    req.body.clear();   // 目前不解析 body，仅占位
    while (true) {
        size_t pos = data.find("\r\n");   //先判定数据是否存在第一行
        if (pos == string::npos) return false;    //不存在直接return
        string line = data.substr(0, pos); //取第一行
        data.erase(0, pos + 2);              //取完之后消除第一行更新第一行内容
        if (state == 0) {
            // 解析请求行：METHOD SP URL SP VERSION
            size_t p1 = line.find(" ");
            size_t p2 = line.find(" ", p1 + 1);
            if (p1 != string::npos && p2 != string::npos) {
                req.method = line.substr(0, p1);
                string url = line.substr(p1 + 1, p2 - p1 - 1);

                // 拆分 path 和 query
                size_t qpos = url.find("?");
                if (qpos == string::npos) {
                    req.path = url;
                    req.query.clear();
                } else {
                    req.path = url.substr(0, qpos);
                    req.query = url.substr(qpos + 1);
                }

                if (req.path.find("favicon.ico") != string::npos) {
                    server->close_connection(client_socket);
                    return false;
                }
                Log::get_instance()->write_log(0, "Extract Path: %s", req.path.c_str());
                Log::get_instance()->write_log(0, "Normal request fd: %d", client_socket);
                cout << "---------- HTTP Request ----------\n" << raw_request << "---------- End ----------\n";
                if (req.path == "/" || req.path == "") {
                    req.path = "index.html";
                }
                state = 1;
            }
        } else if (state == 1) {
            // 解析请求头：形如 "Key: Value"
            if (line.empty()) {
                // 空行：头部结束
                break;
            }

            size_t colon = line.find(':');
            if (colon != string::npos) {
                string key = line.substr(0, colon);

                // 去掉冒号后面开头的空格和制表符
                size_t start = colon + 1;
                while (start < line.size() && (line[start] == ' ' || line[start] == '\t')) {
                    ++start;
                }
                string value = line.substr(start);
                req.headers[key] = value;
            }
        }
    }

    // 解析请求体（body）：仅在存在 Content-Length 头部时尝试
    auto it = req.headers.find("Content-Length");
    if (it != req.headers.end()) {
        int content_length = 0;
        try {
            content_length = stoi(it->second);
        } catch (...) {
            content_length = 0;
        }

        if (content_length > 0 && data.size() >= static_cast<size_t>(content_length)) {
            req.body = data.substr(0, content_length);
            data.erase(0, content_length);
        }
    }
    {
        lock_guard<mutex> lock(server->conn_mtx);
        server->client_buffers.erase(client_socket);
    }

    if (req.path != "index.html") {
        req.path += ".html";
    }
    return true;
}
