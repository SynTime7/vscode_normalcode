#define _WIN32_WINNT 0x0601  // 定义Windows版本，0x0601代表Windows 7及以上
#include <winsock2.h>        // Windows套接字API头文件
#include <ws2tcpip.h>        // 支持IPv6和其他扩展
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#pragma comment(lib, "ws2_32.lib")  // 链接Winsock库

#define PORT 8080           // 监听端口号
#define BACKLOG 10          // 监听队列长度
#define BUF_SIZE 8192       // 缓冲区大小

// URL解码函数，将%XX编码转换为对应字符，+转换为空格
void url_decode(const char* src, char* dst, int dst_size) {
    char a, b;
    int i = 0;
    while (*src && i < dst_size - 1) {
        if (*src == '%') {
            // 判断后面两个字符是否为十六进制数字
            if (isxdigit((a = *(src + 1))) && isxdigit((b = *(src + 2)))) {
                // 将十六进制字符转换为数字
                a = (a >= 'a') ? a - 'a' + 10 : (a >= 'A') ? a - 'A' + 10 : a - '0';
                b = (b >= 'a') ? b - 'a' + 10 : (b >= 'A') ? b - 'A' + 10 : b - '0';
                dst[i++] = (char)(a * 16 + b);  // 合成一个字符
                src += 3;  // 跳过%XX
            } else {
                dst[i++] = *src++;
            }
        } else if (*src == '+') {
            dst[i++] = ' ';  // +号转换为空格
            src++;
        } else {
            dst[i++] = *src++;  // 直接复制字符
        }
    }
    dst[i] = '\0';  // 字符串结束符
}

// 生成HTML表单页面，返回字符串指针，调用者负责释放
char* generate_html_form() {
    const char* form_html =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n\r\n"
        "<!DOCTYPE html>"
        "<html lang=\"zh-CN\">"
        "<head>"
        "<meta charset=\"UTF-8\">"
        "<title>命令交互界面</title>"
        "<style>"
        "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: #f0f2f5; margin: 0; padding: 0; }"
        ".container { max-width: 400px; margin: 80px auto; background: white; padding: 30px; border-radius: 8px; box-shadow: 0 4px 12px rgba(0,0,0,0.1); }"
        "h1 { text-align: center; color: #333; }"
        "form { display: flex; flex-direction: column; }"
        "label { margin-top: 15px; font-weight: bold; color: #555; }"
        "input[type=text] { padding: 10px; margin-top: 5px; border: 1px solid #ccc; border-radius: 4px; font-size: 16px; }"
        "input[type=submit] { margin-top: 25px; padding: 12px; background-color: #4CAF50; color: white; border: none; border-radius: 4px; font-size: 18px; cursor: pointer; transition: background-color 0.3s ease; }"
        "input[type=submit]:hover { background-color: #45a049; }"
        "</style>"
        "</head>"
        "<body>"
        "<div class=\"container\">"
        "<h1>请输入命令与参数</h1>"
        "<form method=\"POST\" action=\"/\">"
        "<label for=\"command\">命令：</label>"
        "<input type=\"text\" id=\"command\" name=\"command\" placeholder=\"例如: dir\">"
        "<label for=\"param\">参数：</label>"
        "<input type=\"text\" id=\"param\" name=\"param\" placeholder=\"例如: C:\\\\Users\">"
        "<input type=\"submit\" value=\"发送\">"
        "</form>"
        "</div>"
        "</body>"
        "</html>";

    // 分配内存并复制HTML字符串
    char* html = (char*)malloc(strlen(form_html) + 1);
    if (!html) return NULL;
    strcpy(html, form_html);
    return html;
}

// 生成POST请求响应页面，显示提交的参数，调用者负责释放
char* generate_html_response_post(const char* body) {
    // HTML头部和样式
    const char* html_header =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n\r\n"
        "<!DOCTYPE html>"
        "<html lang=\"zh-CN\">"
        "<head>"
        "<meta charset=\"UTF-8\">"
        "<title>POST参数展示</title>"
        "<style>"
        "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: #f0f2f5; margin: 0; padding: 0; }"
        ".container { max-width: 500px; margin: 80px auto; background: white; padding: 30px; border-radius: 8px; box-shadow: 0 4px 12px rgba(0,0,0,0.1); }"
        "h1 { text-align: center; color: #333; }"
        "ul { list-style: none; padding: 0; }"
        "li { background: #e8f0fe; margin: 8px 0; padding: 10px 15px; border-radius: 4px; font-size: 16px; color: #202124; }"
        "a { display: inline-block; margin-top: 20px; text-decoration: none; color: #4CAF50; font-weight: bold; }"
        "a:hover { text-decoration: underline; }"
        "</style>"
        "</head>"
        "<body>"
        "<div class=\"container\">"
        "<h1>接收到的POST参数</h1>"
        "<ul>";

    const char* html_footer = "</ul><a href=\"/\">返回输入页面</a></div></body></html>";

    char params_html[4096] = {0};  // 用于拼接参数列表HTML

    // 复制body，避免修改原字符串
    char* body_copy = _strdup(body);
    if (!body_copy) return NULL;

    // 按&分割参数对
    char* token = strtok(body_copy, "&");
    while (token) {
        char* equal_sign = strchr(token, '=');
        if (equal_sign) {
            *equal_sign = '\0';  // 分割key和value
            const char* key = token;
            const char* value = equal_sign + 1;

            char decoded_key[256] = {0};
            char decoded_value[256] = {0};
            url_decode(key, decoded_key, sizeof(decoded_key));       // 解码key
            url_decode(value, decoded_value, sizeof(decoded_value)); // 解码value

            // 拼接HTML列表项
            strcat(params_html, "<li><strong>");
            strcat(params_html, decoded_key);
            strcat(params_html, "：</strong> ");
            strcat(params_html, decoded_value);
            strcat(params_html, "</li>");
        } else {
            // 没有=号，只有key
            strcat(params_html, "<li><strong>");
            strcat(params_html, token);
            strcat(params_html, "：</strong> (无值)</li>");
        }
        token = strtok(NULL, "&");
    }
    free(body_copy);

    // 计算总长度，分配内存
    size_t total_len = strlen(html_header) + strlen(params_html) + strlen(html_footer) + 1;
    char* html = (char*)malloc(total_len);
    if (!html) return NULL;

    // 拼接完整HTML
    strcpy(html, html_header);
    strcat(html, params_html);
    strcat(html, html_footer);

    return html;
}

// 读取一行HTTP请求头，直到遇到换行符，返回读取的字符数
int read_line(SOCKET sock, char* buf, int size) {
    int i = 0;
    char c = '\0';
    int n;
    while ((i < size - 1) && (c != '\n')) {
        n = recv(sock, &c, 1, 0);
        if (n > 0) {
            if (c == '\r') {
                // 预读下一个字符是否是\n
                n = recv(sock, &c, 1, MSG_PEEK);
                if ((n > 0) && (c == '\n')) {
                    recv(sock, &c, 1, 0);  // 读取\n
                } else {
                    c = '\n';  // 只遇到\r，视为换行
                }
            }
            buf[i++] = c;
        } else {
            c = '\n';  // 连接关闭或错误，结束读取
        }
    }
    buf[i] = '\0';  // 字符串结束符
    return i;
}

int main() {
    WSADATA wsaData;
    int iResult;

    // 初始化Winsock库，版本2.2
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    // 创建TCP套接字
    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd == INVALID_SOCKET) {
        printf("socket failed: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // 设置套接字选项，允许地址重用
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    // 绑定地址和端口
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);      // 网络字节序端口号
    server_addr.sin_addr.s_addr = INADDR_ANY; // 监听所有网卡
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("bind failed: %ld\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // 监听端口，等待客户端连接
    if (listen(server_fd, BACKLOG) == SOCKET_ERROR) {
        printf("listen failed: %ld\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    printf("服务器启动，监听端口 %d\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);

        // 接受客户端连接，返回新的套接字
        SOCKET client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_fd == INVALID_SOCKET) {
            printf("accept failed: %ld\n", WSAGetLastError());
            continue;
        }

        char line[BUF_SIZE];
        // 读取请求行（例如：GET / HTTP/1.1）
        if (read_line(client_fd, line, sizeof(line)) <= 0) {
            closesocket(client_fd);
            continue;
        }

        char method[8] = {0};
        char uri[1024] = {0};
        sscanf(line, "%7s %1023s", method, uri);  // 解析请求方法和URI

        int content_length = 0;
        // 读取请求头，查找Content-Length字段
        while (1) {
            if (read_line(client_fd, line, sizeof(line)) <= 0) break;
            if (strcmp(line, "\n") == 0 || strcmp(line, "\r\n") == 0) break; // 空行表示头结束

            if (_strnicmp(line, "Content-Length:", 15) == 0) {
                content_length = atoi(line + 15);  // 获取请求体长度
            }
        }

        // 处理GET请求，返回HTML表单页面
        if (_stricmp(method, "GET") == 0) {
            char* response = generate_html_form();
            if (response) {
                send(client_fd, response, (int)strlen(response), 0);
                free(response);
            }
        }
        // 处理POST请求，读取请求体并显示参数
        else if (_stricmp(method, "POST") == 0) {
            if (content_length > 0 && content_length < BUF_SIZE) {
                char* body = (char*)malloc(content_length + 1);
                if (!body) {
                    closesocket(client_fd);
                    continue;
                }
                int received = 0;
                // 循环接收请求体数据
                while (received < content_length) {
                    int r = recv(client_fd, body + received, content_length - received, 0);
                    if (r <= 0) break;
                    received += r;
                }
                body[received] = '\0';  // 字符串结束符

                // 生成并发送响应页面
                char* response = generate_html_response_post(body);
                if (response) {
                    send(client_fd, response, (int)strlen(response), 0);
                    free(response);
                }
                free(body);
            } else {
                // Content-Length异常，返回表单页面
                char* response = generate_html_form();
                if (response) {
                    send(client_fd, response, (int)strlen(response), 0);
                    free(response);
                }
            }
        }
        // 不支持的请求方法，返回405错误
        else {
            const char* resp405 =
                "HTTP/1.1 405 Method Not Allowed\r\n"
                "Content-Length: 0\r\n"
                "Connection: close\r\n\r\n";
            send(client_fd, resp405, (int)strlen(resp405), 0);
        }

        // 关闭客户端连接
        closesocket(client_fd);
    }

    // 关闭服务器套接字，清理Winsock
    closesocket(server_fd);
    WSACleanup();
    return 0;
}
