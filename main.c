#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ctype.h> // 用于isxdigit函数
#include "top.h"

#pragma comment(lib, "ws2_32.lib")

// 生成包含输入表单和命令执行结果的HTML页面
// 参数说明：
//   command - 用户输入的命令字符串，NULL或空字符串表示无命令输入
//   param   - 用户输入的参数字符串，NULL或空字符串表示无参数
//   result  - 命令执行结果字符串，NULL或空字符串表示无结果显示
// 返回值：
//   返回动态分配的字符串，使用后需free释放
char *generate_page_with_result(const char *command, const char *param, const char *result)
{
    char cmd_line[512] = {0};
    if (command && strlen(command) > 0)
    {
        if (param && strlen(param) > 0)
        {
            snprintf(cmd_line, sizeof(cmd_line), "%s %s", command, param);
        }
        else
        {
            snprintf(cmd_line, sizeof(cmd_line), "%s", command);
        }
    }

    const char *html_template_start =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n\r\n"
        "<!DOCTYPE html>"
        "<html lang=\"zh-CN\">"
        "<head>"
        "<meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
        "<title>命令执行服务器</title>"
        "<style>"
        "body {"
        "  font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;"
        "  background: linear-gradient(135deg, #667eea, #764ba2);"
        "  color: #fff;"
        "  margin: 0; padding: 20px;"
        "  display: flex;"
        "  justify-content: center;"
        "  align-items: flex-start;"
        "  min-height: 100vh;"
        "}"
        ".container {"
        "  background: rgba(255, 255, 255, 0.1);"
        "  padding: 30px;"
        "  border-radius: 12px;"
        "  box-shadow: 0 8px 32px 0 rgba(31, 38, 135, 0.37);"
        "  width: 400px;"
        "  max-width: 90vw;"
        "}"
        "h2 {"
        "  text-align: center;"
        "  margin-bottom: 20px;"
        "  font-weight: 700;"
        "}"
        "form {"
        "  display: flex;"
        "  flex-direction: column;"
        "}"
        "input[type=text] {"
        "  padding: 10px;"
        "  margin-bottom: 15px;"
        "  border: none;"
        "  border-radius: 6px;"
        "  font-size: 16px;"
        "}"
        "input[type=submit] {"
        "  background-color: #5a67d8;"
        "  color: white;"
        "  padding: 12px;"
        "  border: none;"
        "  border-radius: 6px;"
        "  font-size: 16px;"
        "  cursor: pointer;"
        "  transition: background-color 0.3s ease;"
        "}"
        "input[type=submit]:hover {"
        "  background-color: #434190;"
        "}"
        "pre {"
        "  background: rgba(0, 0, 0, 0.3);"
        "  padding: 15px;"
        "  border-radius: 8px;"
        "  overflow-x: auto;"
        "  white-space: pre-wrap;"
        "  word-wrap: break-word;"
        "  font-size: 14px;"
        "  margin-top: 20px;"
        "}"
        "</style>"
        "</head>"
        "<body>"
        "<div class=\"container\">"
        "<h2>请输入命令和参数</h2>"
        "<form method=\"POST\" action=\"/\">"
        "命令: <input type=\"text\" name=\"command\" placeholder=\"例如: dir\" value=\"%s\" required><br>"
        "参数: <input type=\"text\" name=\"param\" placeholder=\"例如: C:\\\\\" value=\"%s\"><br>"
        "<input type=\"submit\" value=\"执行\">"
        "</form>";

    const char *html_template_end =
        "</div>"
        "</body>"
        "</html>";

    // 估算页面大小，预留足够空间
    size_t page_size = strlen(html_template_start) + strlen(html_template_end) + 1024;
    if (result && strlen(result) > 0)
    {
        page_size += strlen(result) + strlen(cmd_line) + 64;
    }

    char *page = (char *)malloc(page_size);
    if (!page)
        return NULL;

    // 写入页面头部和表单，填充命令和参数默认值
    snprintf(page, page_size, html_template_start,
             command ? command : "",
             param ? param : "");

    // 如果有执行结果，追加显示
    if (result && strlen(result) > 0)
    {
        char result_section[2048];
        snprintf(result_section, sizeof(result_section),
                 "<h2>执行命令: %s</h2><pre>%s</pre>",
                 cmd_line, result);
        strncat(page, result_section, page_size - strlen(page) - 1);
    }

    // 追加页面尾部
    strncat(page, html_template_end, page_size - strlen(page) - 1);

    return page;
}

// HTTP响应头
const char *head_fmt =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html; charset=utf-8\r\n"
    "Content-Length: %ld\r\n"
    "Connection: close\r\n"
    "\r\n"
    "%s";

const char *css_style =
    "<style>"
    "body {"
    "  font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;"
    "  background: linear-gradient(135deg, #667eea, #764ba2);"
    "  color: #fff;"
    "  margin: 0; padding: 0;"
    "  display: flex;"
    "  justify-content: center;"
    "  align-items: center;"
    "  height: 100vh;"
    "}"
    ".container {"
    "  background: rgba(255, 255, 255, 0.1);"
    "  padding: 30px;"
    "  border-radius: 12px;"
    "  box-shadow: 0 8px 32px 0 rgba(31, 38, 135, 0.37);"
    "  width: 320px;"
    "}"
    "h2 {"
    "  text-align: center;"
    "  margin-bottom: 20px;"
    "  font-weight: 700;"
    "}"
    "form {"
    "  display: flex;"
    "  flex-direction: column;"
    "}"
    "input[type=text] {"
    "  padding: 10px;"
    "  margin-bottom: 15px;"
    "  border: none;"
    "  border-radius: 6px;"
    "  font-size: 16px;"
    "}"
    "input[type=submit] {"
    "  background-color: #5a67d8;"
    "  color: white;"
    "  padding: 12px;"
    "  border: none;"
    "  border-radius: 6px;"
    "  font-size: 16px;"
    "  cursor: pointer;"
    "  transition: background-color 0.3s ease;"
    "}"
    "input[type=submit]:hover {"
    "  background-color: #434190;"
    "}"
    "</style>";

const char *css_style_inputandview =
    "<style>"
    "body { background: #222; color: #0f0; font-family: monospace; padding: 20px; }"
    ".container { max-width: 600px; margin: auto; background: #111; padding: 20px; border-radius: 8px; }"
    "input[type=text] { width: 100%; padding: 8px; margin: 6px 0; box-sizing: border-box; border-radius: 4px; border: 1px solid #555; background: #333; color: #0f0; }"
    "input[type=submit] { background-color: #0f0; color: #000; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; }"
    "input[type=submit]:hover { background-color: #6f6; }"
    "pre#output { background: rgba(0,0,0,0.7); padding: 15px; border-radius: 8px; height: 300px; overflow-y: auto; white-space: pre-wrap; word-wrap: break-word; font-size: 14px; margin-top: 20px; }"
    "</style>";

const char *html_page_inputandview =
    "<!DOCTYPE html>"
    "<html lang=\"zh-CN\">"
    "<head>"
    "<meta charset=\"UTF-8\">"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<title>命令执行服务器</title>"
    "%.*s" // CSS插入位置
    "</head>"
    "<body>"
    "<div class=\"container\">"
    "<h2>请输入命令和参数</h2>"
    "<form method=\"POST\" action=\"/\">"
    "命令: <input type=\"text\" name=\"command\" placeholder=\"例如: dir\" required><br>"
    "参数: <input type=\"text\" name=\"param\" placeholder=\"例如: C:\\\\\"><br>"
    "<input type=\"submit\" value=\"执行\">"
    "</form>"
    "<h2>命令执行输出</h2>"
    "<pre id=\"output\"></pre>"
    "</div>"
    "<script>"
    "var ws = new WebSocket('ws://' + location.host + '/ws');"
    "var output = document.getElementById('output');"
    "ws.onmessage = function(evt) {"
    "  output.textContent += evt.data + '\\n';"
    "  output.scrollTop = output.scrollHeight;"
    "};"
    "ws.onopen = function() { console.log('WebSocket连接已建立'); };"
    "ws.onclose = function() { console.log('WebSocket连接已关闭'); };"
    "</script>"
    "</body>"
    "</html>";

const char *html_frame =
    "<!DOCTYPE html>"
    "<html lang=\"zh-CN\">"
    "<head>"
    "<meta charset=\"UTF-8\">"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<title>命令执行服务器</title>"
    "%.*s" // 预留CSS插入位置
    "</head>"
    "<body>"
    "<div class=\"container\">"
    "<h2>请输入命令和参数</h2>"
    "<form method=\"POST\" action=\"/\">"
    "命令: <input type=\"text\" name=\"command\" placeholder=\"例如: dir\" required><br>"
    "参数: <input type=\"text\" name=\"param\" placeholder=\"例如: C:\\\\\"><br>"
    "<input type=\"submit\" value=\"执行\">"
    "</form>"
    "</div>"
    "</body>"
    "</html>";

static struct mg_connection *ws_conn = NULL; // 保存WebSocket连接指针
static struct mg_timer timer;                // 定时器对象

// Connection event handler function
static void fn(struct mg_connection *c, int ev, void *ev_data)
{
    switch (ev)
    {
    case MG_EV_HTTP_MSG:
    { // New HTTP request received
        // struct mg_http_message *hm = (struct mg_http_message *)ev_data; // Parsed HTTP request
        // if (mg_match(hm->uri, mg_str("/api/hello"), NULL))
        // {
        //     // REST API call
        //     char response[4096];
        //     char response1[4096];
        //     int len = snprintf(response, sizeof(response), html_page_inputandview, strlen(css_style_inputandview), css_style_inputandview);
        //     len = snprintf(response1, sizeof(response1), head_fmt, len, response);
        //     // 发送完整响应
        //     mg_send(c, response1, len);
        //     // 发送完毕后关闭连接
        //     c->is_draining = 1;
        // }
        // else
        // {
        //     struct mg_http_serve_opts opts = {.root_dir = "."}; // For all other URLs,
        //     mg_http_serve_dir(c, hm, &opts);                    // Serve static files
        // }
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;
        if (mg_match(hm->uri, mg_str("/"), NULL))
        {
            char response[4096];
            char response1[4096];
            int len = snprintf(response, sizeof(response), html_page_inputandview, strlen(css_style_inputandview), css_style_inputandview);
            len = snprintf(response1, sizeof(response1), head_fmt, len, response);
            mg_send(c, response1, len);
        }
        else if (mg_match(hm->uri, mg_str("/ws"), NULL))
        {
            mg_ws_upgrade(c, hm, NULL); // 关键：升级为WebSocket连接
        }
        else
        {
            mg_http_reply(c, 404, "", "Not Found\n");
        }
        break;
    }
    case MG_EV_WS_OPEN:
    {
        // 保存WebSocket连接指针
        ws_conn = c;
        printf("WebSocket连接已建立\n");
        break;
    }
    case MG_EV_WS_MSG:
    {
        // 可处理客户端消息，示例不做处理
        break;
    }
    case MG_EV_CLOSE:
    {
        // 连接关闭时清理指针
        if (ws_conn == c)
        {
            ws_conn = NULL;
            printf("WebSocket连接已关闭\n");
        }
        break;
    }
    }
}

// 定时器回调函数，每2秒调用一次
static void timer_cb(void *arg)
{
    (void)arg;
    if (ws_conn != NULL)
    {
        char buf[64];
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        strftime(buf, sizeof(buf), "服务器时间: %Y-%m-%d %H:%M:%S", tm_info);
        mg_ws_send(ws_conn, buf, strlen(buf), WEBSOCKET_OP_TEXT);
    }
}

struct mg_connection *pcConn_send;
int main()
{
    struct mg_mgr mgr;                                     // Mongoose event manager. Holds all connections
    mg_mgr_init(&mgr);                                     // Initialise event manager
    mg_http_listen(&mgr, "http://0.0.0.0:8000", fn, NULL); // Setup listener
    // 初始化定时器，2秒周期
    mg_timer_add(&mgr, 2000, MG_TIMER_REPEAT, timer_cb, NULL);
    for (;;)
    {
        mg_mgr_poll(&mgr, 1000); // Infinite event loop
    }
    return 0;
}