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

#pragma comment(lib, "ws2_32.lib")

static struct mg_connection *ws_conn = NULL; // 保存WebSocket连接指针
static struct mg_timer timer;                // 定时器对象

#define UPLOAD_DIR "uploads"
#define MAX_UPLOAD_SIZE (10 * 1024 * 1024) // 最大10MB

// HTTP响应头格式字符串
const char *head_fmt =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html; charset=utf-8\r\n"
    "Content-Length: %ld\r\n"
    "Connection: close\r\n"
    "\r\n"
    "%s";

// 页面CSS样式，定义了页面整体风格和控件样式
const char *css_style_inputandview =
    "<style>"
    /* 页面主体样式 */
    "body {"
    "background: linear-gradient(135deg, #9a7fff, #bfa0ff);"        /* 渐变背景色 */
    "color: #5a3dbf;"                                               /* 文字颜色 */
    "font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;" /* 字体 */
    "padding: 30px;"
    "margin: 0;"
    "height: 100vh;" /* 视口高度 */
    "display: flex;"
    "justify-content: center;"
    "align-items: center;"
    "}"
    /* 容器样式 */
    ".container {"
    "max-width: 1300px;"
    "width: 100%;"
    "background: #e6dbff;" /* 容器背景色 */
    "padding: 30px 35px;"
    "border-radius: 16px;"                             /* 圆角 */
    "box-shadow: 0 8px 24px rgba(154, 127, 255, 0.3);" /* 阴影 */
    "border: 1px solid #b3a1ff;"                       /* 边框 */
    "color: #5a3dbf;"
    "display: flex;"
    "flex-direction: row;"
    "gap: 30px;" /* 子元素间距 */
    "height: 800px;"
    "box-sizing: border-box;"
    "min-width: 0;" /* 防止溢出 */
    "}"
    /* 左侧面板 */
    ".left-panel {"
    "flex: 0 0 40%;" /* 固定宽度40% */
    "display: flex;"
    "flex-direction: column;"
    "}"
    /* 右侧面板 */
    ".right-panel {"
    "flex: 1;" /* 剩余空间 */
    "display: flex;"
    "flex-direction: column;"
    "min-width: 0;" /* 允许缩小 */
    "}"
    /* 文本输入框 */
    "input[type=text] {"
    "width: 100%;"
    "padding: 14px 18px;"
    "margin: 12px 0;"
    "box-sizing: border-box;"
    "border-radius: 10px;"
    "border: 1.8px solid #b3a1ff;"
    "background: #f2eaff;"
    "color: #5a3dbf;"
    "font-size: 16px;"
    "transition: border-color 0.3s ease, background-color 0.3s ease;"
    "}"
    /* 输入框聚焦样式 */
    "input[type=text]:focus {"
    "outline: none;"
    "border-color: #7f66ff;"
    "background-color: #d9ccff;"
    "box-shadow: 0 0 10px #7f66ff;"
    "}"
    /* 按钮行容器 */
    ".button-row {"
    "display: flex;"               /* 水平排列 */
    "flex-wrap: wrap;"             /* 允许换行 */
    "gap: 12px;"                   /* 按钮间距 */
    "margin-top: 12px;"            /* 顶部间距 */
    "justify-content: flex-start;" /* 左对齐 */
    "}"
    /* 按钮和提交按钮通用样式 */
    ".button-row input[type=submit],"
    ".button-row button {"
    "flex: none;"                                                   /* 固定大小 */
    "padding: 14px 28px;"                                           /* 内边距 */
    "border-radius: 10px;"                                          /* 圆角 */
    "font-weight: 700;"                                             /* 加粗 */
    "font-size: 16px;"                                              /* 字体大小 */
    "cursor: pointer;"                                              /* 鼠标手型 */
    "box-shadow: 0 5px 15px rgba(127, 102, 255, 0.5);"              /* 阴影 */
    "transition: background-color 0.3s ease, box-shadow 0.3s ease;" /* 平滑过渡 */
    "}"
    /* 提交按钮特有样式 */
    ".button-row input[type=submit] {"
    "background-color: #7f66ff;" /* 紫色背景 */
    "color: #fff;"               /* 白色文字 */
    "border: none;"              /* 无边框 */
    "}"
    /* 提交按钮悬停 */
    ".button-row input[type=submit]:hover {"
    "background-color: #a18cff;"                       /* 浅紫色 */
    "box-shadow: 0 7px 20px rgba(161, 140, 255, 0.7);" /* 加深阴影 */
    "}"
    /* 普通按钮特有样式 */
    ".button-row button {"
    "background-color: #7f66ff;" /* 紫色背景 */
    "color: #fff;"
    "border: none;"
    "box-shadow: 0 5px 15px rgba(127, 102, 255, 0.5);" /* 阴影 */
    "}"
    /* 普通按钮悬停 */
    ".button-row button:hover {"
    "background-color: #a18cff;"                       /* 浅紫色 */
    "box-shadow: 0 7px 20px rgba(161, 140, 255, 0.7);" /* 加深阴影 */
    "}"
    /* 命令输出文本框 */
    "pre#output {"
    "flex: 1;"                            /* 填满剩余空间 */
    "background: #d9ccff;"                /* 背景色 */
    "padding: 22px;"                      /* 内边距 */
    "border-radius: 16px;"                /* 圆角 */
    "overflow-y: auto;"                   /* 纵向滚动 */
    "overflow-x: auto;"                   /* 横向滚动 */
    "white-space: pre-wrap;"              /* 保留空白符，自动换行 */
    "word-wrap: break-word;"              /* 长单词换行 */
    "word-break: break-word;"             /* 兼容换行 */
    "font-size: 15px;"                    /* 字体大小 */
    "margin-top: 12px;"                   /* 顶部间距 */
    "color: #6b4fcf;"                     /* 文字颜色 */
    "box-shadow: inset 0 0 12px #b3a1ff;" /* 内阴影 */
    "font-family: Consolas, monospace;"   /* 等宽字体 */
    "min-width: 0;"                       /* 允许缩小 */
    "max-width: 100%;"                    /* 最大宽度 */
    "}"
    /* 模态框背景 */
    ".modal {"
    "display: none;"
    "position: fixed;"
    "z-index: 1000;"
    "left: 0; top: 0;"
    "width: 100%; height: 100%;"
    "overflow: auto;"
    "background-color: rgba(90, 61, 191, 0.25);" /* 半透明背景 */
    "}"
    /* 模态框内容 */
    ".modal-content {"
    "background-color: #e6dbff;"
    "margin: 10% auto;"
    "padding: 20px 30px;"
    "border-radius: 16px;"
    "max-width: 400px;"
    "box-shadow: 0 8px 24px rgba(154, 127, 255, 0.3);"
    "color: #5a3dbf;"
    "position: relative;"
    "box-sizing: border-box;"
    "}"
    /* 关闭按钮 */
    ".close {"
    "color: #5a3dbf;"
    "position: absolute;"
    "top: 12px;"
    "right: 20px;"
    "font-size: 28px;"
    "font-weight: bold;"
    "cursor: pointer;"
    "}"
    /* 关闭按钮悬停 */
    ".close:hover {"
    "color: #7f66ff;"
    "}"
    /* 上传表单文件输入 */
    "#uploadForm input[type=file] {"
    "width: 100%;"
    "padding: 6px 0;"
    "margin-top: 10px;"
    "cursor: pointer;"
    "}"
    /* 上传表单提交按钮 */
    "#uploadForm input[type=submit] {"
    "margin-top: 20px;"
    "width: 100%;"
    "background-color: #7f66ff;"
    "color: #fff;"
    "padding: 14px 0;"
    "border: none;"
    "border-radius: 10px;"
    "cursor: pointer;"
    "font-weight: 700;"
    "font-size: 16px;"
    "transition: background-color 0.3s ease, box-shadow 0.3s ease;"
    "box-shadow: 0 5px 15px rgba(127, 102, 255, 0.5);"
    "}"
    /* 上传按钮悬停 */
    "#uploadForm input[type=submit]:hover {"
    "background-color: #a18cff;"
    "box-shadow: 0 7px 20px rgba(161, 140, 255, 0.7);"
    "}"
    /* 上传状态文本 */
    "#uploadStatus {"
    "margin-top: 15px;"
    "font-weight: 600;"
    "color: #5a3dbf;"
    "}"
    /* 上传目录标签 */
    "#uploadForm label[for=saveDir] {"
    "display: block;"
    "margin-top: 12px;"
    "font-weight: 600;"
    "color: #5a3dbf;"
    "}"
    /* 上传目录输入框 */
    "#uploadForm input#saveDir {"
    "width: 100%;"
    "padding: 10px 14px;"
    "margin-top: 6px;"
    "border-radius: 10px;"
    "border: 1.8px solid #b3a1ff;"
    "background: #f2eaff;"
    "color: #5a3dbf;"
    "font-size: 15px;"
    "box-sizing: border-box;"
    "transition: border-color 0.3s ease, background-color 0.3s ease;"
    "}"
    /* 上传目录输入框聚焦 */
    "#uploadForm input#saveDir:focus {"
    "outline: none;"
    "border-color: #7f66ff;"
    "background-color: #d9ccff;"
    "box-shadow: 0 0 10px #7f66ff;"
    "}"
    "</style>";

const char *html_page_inputandview =
    "<!DOCTYPE html>"
    "<html lang=\"zh-CN\">"
    "<head>"
    "<meta charset=\"UTF-8\">"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<title>命令执行服务器</title>"
    "%.*s"
    "</head>"
    "<body>"
    "<div class=\"container\">"
    "  <div class=\"left-panel\">"
    "    <h2>请输入命令和参数</h2>"
    "    <form id=\"cmdForm\">"
    "      命令: <input type=\"text\" id=\"command\" placeholder=\"例如: dir\" required><br>"
    "      参数: <input type=\"text\" id=\"param\" placeholder=\"例如: C:\\\\\"><br>"
    "      <div class=\"button-row\">"
    "        <input type=\"submit\" value=\"执行\">"
    "        <button id=\"openUploadBtn\" type=\"button\">上传文件</button>"
    "        <button id=\"openFtpBtn\" type=\"button\">FTP</button>"
    "        <button id=\"clearOutputBtn\" type=\"button\">Clear</button>"
    "      </div>"
    "    </form>"
    "  </div>"
    "  <div class=\"right-panel\">"
    "    <h2>命令执行输出</h2>"
    "    <pre id=\"output\"></pre>"
    "  </div>"
    "</div>"

    "<div id=\"uploadModal\" class=\"modal\">"
    "  <div class=\"modal-content\">"
    "    <span id=\"closeModal\" class=\"close\">&times;</span>"
    "    <h2>上传文件到服务器（分块上传）</h2>"
    "    <form id=\"uploadForm\">"
    "      <label for=\"saveDir\">保存目录:</label>"
    "      <input type=\"text\" id=\"saveDir\" name=\"saveDir\" placeholder=\"例如: upload\" required>"
    "      <input type=\"file\" id=\"fileInput\" name=\"file\" required>"
    "      <input type=\"button\" id=\"uploadBtn\" value=\"开始上传\">"
    "    </form>"
    "    <div id=\"uploadStatus\"></div>"
    "    <progress id=\"uploadProgress\" value=\"0\" max=\"100\" style=\"width:100%; margin-top:10px; display:none;\"></progress>"
    "  </div>"
    "</div>"

    "<script>"
    "document.addEventListener('DOMContentLoaded', function() {"
    "  var ws = new WebSocket('ws://' + location.host + '/ws');"
    "  var output = document.getElementById('output');"

    "  ws.onmessage = function(evt) {"
    "    output.textContent += evt.data + '\\n';"
    "    output.scrollTop = output.scrollHeight;"
    "  };"

    "  ws.onopen = function() { console.log('WebSocket连接已建立'); };"
    "  ws.onclose = function() { console.log('WebSocket连接已关闭'); };"

    "  document.getElementById('cmdForm').onsubmit = function(e) {"
    "    e.preventDefault();"
    "    var cmd = document.getElementById('command').value.trim();"
    "    var param = document.getElementById('param').value.trim();"
    "    var msg = cmd + ';' + param;"
    "    if (ws.readyState === WebSocket.OPEN) {"
    "      ws.send(msg);"
    "      output.textContent += '> ' + msg + '\\n';"
    "    } else {"
    "      alert('WebSocket未连接');"
    "    }"
    "    return false;"
    "  };"

    "  var openBtn = document.getElementById('openUploadBtn');"
    "  var openFtpBtn = document.getElementById('openFtpBtn');"
    "  var modal = document.getElementById('uploadModal');"
    "  var closeBtn = document.getElementById('closeModal');"
    "  var uploadForm = document.getElementById('uploadForm');"
    "  var uploadStatus = document.getElementById('uploadStatus');"
    "  var uploadProgress = document.getElementById('uploadProgress');"
    "  var uploadBtn = document.getElementById('uploadBtn');"

    "  openBtn.onclick = function() {"
    "    modal.style.display = 'block';"
    "    uploadStatus.textContent = '';"
    "    uploadProgress.style.display = 'none';"
    "    uploadProgress.value = 0;"
    "    uploadForm.reset();"
    "  };"

    "  openFtpBtn.onclick = function() {"
    "  window.open('/ftp', '_blank');"
    "  };"

    "document.getElementById('clearOutputBtn').onclick = function() {"
    "var output = document.getElementById('output');"
    "output.textContent = '';"
    "};"

    "  closeBtn.onclick = function() {"
    "    modal.style.display = 'none';"
    "  };"

    "  window.onclick = function(event) {"
    "    if (event.target === modal) {"
    "      modal.style.display = 'none';"
    "    }"
    "  };"

    "  uploadBtn.onclick = async function() {"
    "    var fileInput = document.getElementById('fileInput');"
    "    var saveDir = document.getElementById('saveDir').value.trim();"

    "    if (!fileInput.files.length) {"
    "      alert('请选择文件');"
    "      return;"
    "    }"
    "    if (!saveDir) {"
    "      alert('请输入保存目录');"
    "      return;"
    "    }"

    "    var file = fileInput.files[0];"
    "    var chunkSize = 1024 * 1024;"
    "    var totalChunks = Math.ceil(file.size / chunkSize);"
    "    var uploadId = Date.now() + '-' + Math.random().toString(36).substr(2, 9);"

    "    uploadStatus.style.color = '#5a3e36';"
    "    uploadStatus.textContent = '上传中...';"
    "    uploadProgress.style.display = 'block';"
    "    uploadProgress.value = 0;"

    "    for (var chunkIndex = 0; chunkIndex < totalChunks; chunkIndex++) {"
    "      var start = chunkIndex * chunkSize;"
    "      var end = Math.min(start + chunkSize, file.size);"
    "      var chunk = file.slice(start, end);"
    "      var formData = new FormData();"
    "      formData.append('saveDir', saveDir);"
    "      formData.append('fileName', file.name);"
    "      formData.append('uploadId', uploadId);"
    "      formData.append('chunkIndex', chunkIndex);"
    "      formData.append('totalChunks', totalChunks);"
    "      formData.append('chunk', chunk);"

    "      try {"
    "        var response = await fetch('/upload', {"
    "          method: 'POST',"
    "          body: formData"
    "        });"
    "        if (!response.ok) {"
    "          throw new Error('服务器返回错误: ' + response.status);"
    "        }"
    "        var text = await response.text();"
    "        if (text.indexOf('错误') !== -1 || text.indexOf('失败') !== -1) {"
    "          throw new Error(text);"
    "        }"
    "      } catch (err) {"
    "        uploadStatus.style.color = 'red';"
    "        uploadStatus.textContent = '上传失败: ' + err.message;"
    "        uploadProgress.style.display = 'none';"
    "        return;"
    "      }"
    "      uploadProgress.value = ((chunkIndex + 1) / totalChunks) * 100;"
    "    }"

    "    uploadStatus.style.color = 'green';"
    "    uploadStatus.textContent = '上传成功！';"
    "    uploadProgress.style.display = 'none';"
    "  };"
    "});"
    "</script>"
    "</body>"
    "</html>";

// 1. 新增FTP页面HTML
const char *html_page_ftp =
    "<!DOCTYPE html>"
    "<html lang=\"zh-CN\">"
    "<head>"
    "<meta charset=\"UTF-8\">"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<title>简单FTP页面</title>"
    "<style>"
    "body { font-family: Arial, sans-serif; padding: 20px; background: #f0f0f0; }"
    "h1 { color: #5a3dbf; }"
    "</style>"
    "</head>"
    "<body>"
    "<h1>欢迎使用简单FTP页面</h1>"
    "<p>这里可以放置FTP相关功能或说明。</p>"
    "</body>"
    "</html>";

// 判断目录是否存在，返回0表示存在，-1表示不存在或错误
static int mg_fs_dir_exists(struct mg_fs *fs, const char *path)
{
    size_t size;
    time_t mtime;
    int r = fs->st(path, &size, &mtime);
    if (r < 0)
        return -1;
    if ((r & MG_FS_DIR) != 0)
        return 0;
    return -1;
}

// 如果目录不存在则创建目录，成功返回0，失败返回-1
static int mg_fs_mkdir_if_not_exists(struct mg_fs *fs, const char *path)
{
    if (mg_fs_dir_exists(fs, path) == 0)
    {
        return 0; // 目录已存在
    }
    if (fs->mkd == NULL)
    {
        return -1; // 不支持创建目录
    }
    if (fs->mkd(path))
    {
        return 0; // 创建成功
    }
    return -1; // 创建失败
}

// 保存文件到指定路径，成功返回0，失败返回-1
static int mg_fs_save_file(struct mg_fs *fs, const char *filepath, const char *data, size_t len)
{
    struct mg_fd *fd = mg_fs_open(fs, filepath, MG_FS_WRITE);
    if (fd == NULL)
    {
        return -1; // 打开文件失败
    }
    size_t written = fs->wr(fd->fd, data, len);
    mg_fs_close(fd);
    return (written == len) ? 0 : -1; // 判断写入是否完整
}

// 校验文件名是否合法，禁止包含路径分隔符和 ".."
static int is_valid_filename(const char *filename)
{
    // 禁止包含路径分隔符和 ".."
    if (strstr(filename, "..") != NULL)
        return 0;
    if (strchr(filename, '/') != NULL)
        return 0;
    if (strchr(filename, '\\') != NULL)
        return 0;
    return 1;
}

static int mg_vcmp(struct mg_str *a, const char *b)
{
    size_t len = strlen(b);
    if (a->len != len)
        return -1;
    return memcmp(a->buf, b, len);
}

static int mg_fs_append_file(struct mg_fs *fs, const char *filepath, const char *data, size_t len)
{
    // 先打开文件，读取已有内容长度
    size_t size;
    time_t mtime;
    int r = fs->st(filepath, &size, &mtime);
    if (r <= 0)
    {
        // 文件不存在，直接写入新文件
        return mg_fs_save_file(fs, filepath, data, len);
    }

    // 文件存在，打开文件写入（覆盖）
    struct mg_fd *fd = mg_fs_open(fs, filepath, MG_FS_WRITE);
    if (fd == NULL)
    {
        return -1;
    }

    // 先读取已有内容
    char *buf = malloc(size + len);
    if (!buf)
    {
        mg_fs_close(fd);
        return -1;
    }

    size_t read_len = fs->rd(fd->fd, buf, size);
    if (read_len != size)
    {
        free(buf);
        mg_fs_close(fd);
        return -1;
    }

    // 追加新数据
    memcpy(buf + size, data, len);

    // 重新写入全部数据
    fs->wr(fd->fd, buf, size + len);

    free(buf);
    mg_fs_close(fd);

    return 0;
}

// 处理文件上传请求
static void handle_upload(struct mg_connection *c, struct mg_http_message *hm)
{
    chdir("D:\\workspace\\vscode_workspace\\vscode_normalcode\\");
    struct mg_fs *fs = &mg_fs_posix;

    // 解析multipart表单，提取字段
    struct mg_http_part part;
    size_t off = 0;

    char saveDir[256] = {0};
    char fileName[256] = {0};
    char uploadId[256] = {0};
    int chunkIndex = -1;
    int totalChunks = -1;
    const char *chunkData = NULL;
    size_t chunkLen = 0;

    // 遍历所有part
    while ((off = mg_http_next_multipart(hm->body, off, &part)) != 0)
    {
        if (mg_vcmp(&part.name, "saveDir") == 0)
        {
            snprintf(saveDir, sizeof(saveDir), "%.*s", (int)part.body.len, part.body.buf);
        }
        else if (mg_vcmp(&part.name, "fileName") == 0)
        {
            snprintf(fileName, sizeof(fileName), "%.*s", (int)part.body.len, part.body.buf);
        }
        else if (mg_vcmp(&part.name, "uploadId") == 0)
        {
            snprintf(uploadId, sizeof(uploadId), "%.*s", (int)part.body.len, part.body.buf);
        }
        else if (mg_vcmp(&part.name, "chunkIndex") == 0)
        {
            char tmp[32] = {0};
            snprintf(tmp, sizeof(tmp), "%.*s", (int)part.body.len, part.body.buf);
            chunkIndex = atoi(tmp);
        }
        else if (mg_vcmp(&part.name, "totalChunks") == 0)
        {
            char tmp[32] = {0};
            snprintf(tmp, sizeof(tmp), "%.*s", (int)part.body.len, part.body.buf);
            totalChunks = atoi(tmp);
        }
        else if (mg_vcmp(&part.name, "chunk") == 0)
        {
            chunkData = part.body.buf;
            chunkLen = part.body.len;
        }
    }

    if (saveDir[0] == 0 || fileName[0] == 0 || uploadId[0] == 0 || chunkIndex < 0 || totalChunks < 0 || chunkData == NULL)
    {
        mg_http_reply(c, 400, "", "上传参数不完整\n");
        return;
    }

    // 校验文件名
    if (!is_valid_filename(fileName))
    {
        mg_http_reply(c, 400, "", "非法文件名\n");
        return;
    }

    // 创建保存目录
    if (mg_fs_mkdir_if_not_exists(fs, saveDir) != 0)
    {
        mg_http_reply(c, 500, "", "无法创建保存目录\n");
        return;
    }

    // 临时文件名，避免多个上传冲突
    char tmpFilePath[512];
    snprintf(tmpFilePath, sizeof(tmpFilePath), "%s/%s_%s.tmp", saveDir, uploadId, fileName);

    // 以追加方式打开临时文件
    if (mg_fs_save_file(fs, tmpFilePath, chunkData, chunkLen))
    {
        mg_http_reply(c, 500, "", "写入文件失败\n");
        return;
    }

    // 如果是最后一个块，重命名临时文件为正式文件名
    if (chunkIndex == totalChunks - 1)
    {
        char finalFilePath[512];
        snprintf(finalFilePath, sizeof(finalFilePath), "%s/%s", saveDir, fileName);

        // 删除已有文件（如果存在）
        fs->rm(finalFilePath);

        if (!fs->mv(tmpFilePath, finalFilePath))
        {
            mg_http_reply(c, 500, "", "重命名文件失败\n");
            return;
        }

        mg_http_reply(c, 200, "", "上传成功！\n");
    }
    else
    {
        // 还未完成所有块，返回继续上传
        mg_http_reply(c, 200, "", "块上传成功\n");
    }
}

static struct mg_str mg_http_get_mime(const char *path)
{
    const char *ext = strrchr(path, '.');
    if (ext == NULL)
    {
        return mg_str("application/octet-stream");
    }
    ext++; // 跳过点号 '.'

    if (strcasecmp(ext, "html") == 0 || strcasecmp(ext, "htm") == 0)
    {
        return mg_str("text/html");
    }
    else if (strcasecmp(ext, "css") == 0)
    {
        return mg_str("text/css");
    }
    else if (strcasecmp(ext, "js") == 0)
    {
        return mg_str("application/javascript");
    }
    else if (strcasecmp(ext, "json") == 0)
    {
        return mg_str("application/json");
    }
    else if (strcasecmp(ext, "png") == 0)
    {
        return mg_str("image/png");
    }
    else if (strcasecmp(ext, "jpg") == 0 || strcasecmp(ext, "jpeg") == 0)
    {
        return mg_str("image/jpeg");
    }
    else if (strcasecmp(ext, "gif") == 0)
    {
        return mg_str("image/gif");
    }
    else if (strcasecmp(ext, "svg") == 0)
    {
        return mg_str("image/svg+xml");
    }
    else if (strcasecmp(ext, "txt") == 0)
    {
        return mg_str("text/plain");
    }
    else if (strcasecmp(ext, "pdf") == 0)
    {
        return mg_str("application/pdf");
    }
    else if (strcasecmp(ext, "zip") == 0)
    {
        return mg_str("application/zip");
    }
    else
    {
        return mg_str("application/octet-stream");
    }
}

static void handle_static_file(struct mg_connection *c, struct mg_http_message *hm)
{
    struct mg_fs *fs = &mg_fs_posix;
    const char *base_dir = "frontend"; // 静态文件根目录

    // 先把 hm->uri 复制到缓冲区，添加 \0
    char uri_buf[512];
    if (hm->uri.len >= sizeof(uri_buf))
    {
        mg_http_reply(c, 414, "", "URI Too Long\n");
        return;
    }
    memcpy(uri_buf, hm->uri.buf, hm->uri.len);
    uri_buf[hm->uri.len] = '\0';

    // 确认以 "/static/" 开头
    const char *prefix = "/static/";
    size_t prefix_len = strlen(prefix);
    if (strncmp(uri_buf, prefix, prefix_len) != 0)
    {
        mg_http_reply(c, 400, "", "Bad Request\n");
        return;
    }

    // 取出相对路径部分
    const char *rel_path = uri_buf + prefix_len;
    if (strlen(rel_path) == 0)
    {
        mg_http_reply(c, 400, "", "文件路径不能为空\n");
        return;
    }

    // 路径安全检查，禁止目录穿越
    if (strstr(rel_path, "..") != NULL)
    {
        mg_http_reply(c, 400, "", "非法文件路径\n");
        return;
    }

    // 构造完整文件路径
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s/%s", base_dir, rel_path);

    // 打开文件
    struct mg_fd *fd = mg_fs_open(fs, file_path, MG_FS_READ);
    if (fd == NULL)
    {
        mg_http_reply(c, 404, "", "文件未找到\n");
        return;
    }

    // 获取文件大小和修改时间
    size_t size;
    time_t mtime;
    if (fs->st(file_path, &size, &mtime) < 0)
    {
        mg_fs_close(fd);
        mg_http_reply(c, 500, "", "无法获取文件信息\n");
        return;
    }

    if (size > MAX_UPLOAD_SIZE)
    {
        mg_fs_close(fd);
        mg_http_reply(c, 413, "", "文件过大\n");
        return;
    }

    // 读取文件内容
    char *buf = malloc(size);
    if (!buf)
    {
        mg_fs_close(fd);
        mg_http_reply(c, 500, "", "内存不足\n");
        return;
    }

    size_t n = fs->rd(fd->fd, buf, size);
    mg_fs_close(fd);
    if (n != size)
    {
        free(buf);
        mg_http_reply(c, 500, "", "读取文件失败\n");
        return;
    }

    // 根据文件扩展名返回Content-Type
    struct mg_str mime = mg_http_get_mime(file_path);

    // 返回文件内容
    mg_printf(c,
              "HTTP/1.1 200 OK\r\n"
              "Content-Type: %.*s\r\n"
              "Content-Length: %zu\r\n"
              "Connection: close\r\n"
              "\r\n",
              (int)mime.len, mime.buf, size);

    mg_send(c, buf, size);

    free(buf);
}

// 连接事件处理函数，处理HTTP请求、WebSocket连接及消息等
static void fn(struct mg_connection *c, int ev, void *ev_data)
{
    switch (ev)
    {
    case MG_EV_HTTP_MSG:
    {
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;
        if (mg_match(hm->uri, mg_str("/"), NULL))
        {
            // 访问根路径，返回HTML页面
            char response[10240];
            char response1[10240];
            int len = snprintf(response, sizeof(response), html_page_inputandview, strlen(css_style_inputandview), css_style_inputandview);
            len = snprintf(response1, sizeof(response1), head_fmt, len, response);
            mg_send(c, response1, len);
        }
        else if (mg_match(hm->uri, mg_str("/ws"), NULL))
        {
            // WebSocket升级请求
            mg_ws_upgrade(c, hm, NULL); // 关键：升级为WebSocket连接
        }
        else if (mg_match(hm->uri, mg_str("/upload"), NULL))
        {
            // 只接受POST请求上传文件
            if (0 == mg_strcmp(hm->method, mg_str("POST")))
            {
                handle_upload(c, hm);
            }
            else
            {
                mg_http_reply(c, 405, "", "Method Not Allowed\n");
            }
        }
        else if (mg_match(hm->uri, mg_str("/ftp"), NULL))
        {
            char response[4096];
            int len = snprintf(response, sizeof(response), head_fmt, strlen(html_page_ftp), html_page_ftp);
            mg_send(c, response, len);
        }
        else if (mg_match(hm->uri, mg_str("/static/"), NULL))
        {
            handle_static_file(c, hm);
        }
        else
        {
            // 其他路径返回404
            mg_http_reply(c, 404, "", "Not Found\n");
        }
        break;
    }
    case MG_EV_WS_OPEN:
    {
        // WebSocket连接建立，保存连接指针
        ws_conn = c;
        printf("WebSocket连接已建立\n");
        break;
    }
    case MG_EV_WS_MSG:
    {
        // 收到WebSocket消息，处理客户端发送的命令
        struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;
        // 申请缓冲区，拷贝消息并添加字符串结束符
        char *msg = malloc(wm->data.len + 1);
        if (msg == NULL)
        {
            // 内存不足，发送错误消息
            const char *err = "服务器内存不足\n";
            struct mg_str err_str = {(char *)err, strlen(err)};
            mg_ws_send(c, err_str.buf, err_str.len, WEBSOCKET_OP_TEXT);
            break;
        }
        memcpy(msg, wm->data.buf, wm->data.len);
        msg[wm->data.len] = '\0'; // 转成C字符串

        printf("收到客户端命令: %s\n", msg);

        // TODO: 这里可以添加命令执行逻辑，并通过WebSocket发送结果给客户端

        free(msg);
        break;
    }
    case MG_EV_CLOSE:
    {
        // 连接关闭时清理WebSocket连接指针
        if (ws_conn == c)
        {
            ws_conn = NULL;
            printf("WebSocket连接已关闭\n");
        }
        break;
    }
    }
}

void send_tagged_data(struct mg_connection *ws_conn)
{
    /*
        {
            "type": "data",           // 消息类型，固定为 "data"
            "label": "temperature",   // 数据标签，标识数据类别
            "timestamp": 1687000000,  // UNIX 时间戳（秒）
            "value": 23.5             // 数据值，数值或数组
        }
    */
    if (ws_conn == NULL)
        return;

    time_t now = time(NULL);
    char buf[256];

    // 示例：发送温度数据
    float temperature = 23.5; // 这里用实际采集数据替代
    int len = snprintf(buf, sizeof(buf),
                       "{\"type\":\"data\",\"label\":\"temperature\",\"timestamp\":%ld,\"value\":%.2f}",
                       now, temperature);
    mg_ws_send(ws_conn, buf, len, WEBSOCKET_OP_TEXT);

    // 示例：发送湿度数据
    float humidity = 60.2;
    len = snprintf(buf, sizeof(buf),
                   "{\"type\":\"data\",\"label\":\"humidity\",\"timestamp\":%ld,\"value\":%.2f}",
                   now, humidity);
    mg_ws_send(ws_conn, buf, len, WEBSOCKET_OP_TEXT);
}

// 定时器回调函数，每2秒调用一次，向客户端发送服务器当前时间
static void timer_cb(void *arg)
{
    (void)arg;
    if (ws_conn != NULL)
    {
        send_tagged_data(ws_conn);
    }
}

struct mg_connection *pcConn_send; // 未使用的连接指针变量，保留

int main()
{
    struct mg_fs *fs = &mg_fs_posix;
    struct mg_mgr mgr;                                     // Mongoose事件管理器，管理所有连接
    mg_mgr_init(&mgr);                                     // 初始化事件管理器
    mg_http_listen(&mgr, "http://0.0.0.0:8000", fn, NULL); // 监听0.0.0.0:8000端口，绑定事件处理函数fn
    // 初始化定时器，周期2秒，重复调用timer_cb函数
    mg_timer_add(&mgr, 2000, MG_TIMER_REPEAT, timer_cb, NULL);
    for (;;)
    {
        mg_mgr_poll(&mgr, 1000); // 事件循环，等待并处理事件，超时1000ms
    }
    return 0;
}
