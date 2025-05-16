#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ctype.h> // 用于isxdigit函数
#include "top.h"
#include "mongoose.h"
#include <time.h>
#include <stdio.h>
#include <stdarg.h>

#pragma comment(lib, "ws2_32.lib")
// 连接事件处理函数，处理HTTP请求、WebSocket连接及消息等
static void fn(struct mg_connection *c, int ev, void *ev_data)
{
    switch (ev)
    {
    case MG_EV_HTTP_MSG:
    {
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;
        // 处理预检请求（OPTIONS）
        if (mg_strcmp(hm->method, mg_str("OPTIONS")) == 0)
        {
            mg_printf(c,
                      "HTTP/1.1 204 No Content\r\n"
                      "Access-Control-Allow-Origin: *\r\n"
                      "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
                      "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
                      "Access-Control-Max-Age: 3600\r\n" /* 指定浏览器预检请求（preflight request，HTTP OPTIONS 请求）的结果可以缓存多长时间（秒） */
                      "\r\n");
            c->is_draining = 1;
        }
        else if (mg_match(hm->uri, mg_str("/ws"), NULL))
        {
            // WebSocket升级请求
            mg_ws_upgrade(c, hm, NULL); // 关键：升级为WebSocket连接
        }
        else if (mg_match(hm->uri, mg_str("/api/echo"), NULL))
        {
            mg_http_reply(c, 200, "Access-Control-Allow-Origin: *\r\n", "%.*s", hm->body.len, hm->body.buf);
        }
        else if (mg_match(hm->uri, mg_str("/api/echo1"), NULL))
        {
            mg_http_reply(c, 200, "Access-Control-Allow-Origin: *\r\n", "%.*s", hm->body.len, hm->body.buf);
        }
        else if (mg_match(hm->uri, mg_str("/api/echo2"), NULL))
        {
            mg_http_reply(c, 200, "Access-Control-Allow-Origin: *\r\n", "%.*s", hm->body.len, hm->body.buf);
        }
        else
        {
            // 其他路径返回404
            mg_http_reply(c, 404, "", "Not Found\n");
        }
        break;
    }
    default:
        break;
    }
}

int main()
{
    struct mg_fs *fs = &mg_fs_posix;
    struct mg_mgr mgr;                                   // Mongoose事件管理器，管理所有连接
    mg_mgr_init(&mgr);                                   // 初始化事件管理器
    mg_http_listen(&mgr, "http://0.0.0.0:80", fn, NULL); // 监听0.0.0.0:8000端口，绑定事件处理函数fn
    for (;;)
    {
        mg_mgr_poll(&mgr, 1000); // 事件循环，等待并处理事件，超时1000ms
    }
    return 0;
}