#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "quickjs.h"  // 引入 QuickJS 標頭檔


#define PORT 3303

// Path: quickjs_handler.js
void executeQuickJS(const char *url, int socket_fd) {
    JSRuntime *rt;
    JSContext *ctx;

    rt = JS_NewRuntime();
    if (!rt) {
        printf("QuickJS runtime creation failed\n");
        return;
    }

    ctx = JS_NewContext(rt);
    if (!ctx) {
        printf("QuickJS context creation failed\n");
        JS_FreeRuntime(rt);
        return;
    }

    // 加載並執行 quickjs_handler.js
    FILE *f = fopen("quickjs_handler.js", "r");
    if (f) {
        fseek(f, 0, SEEK_END);
        size_t len = ftell(f);
        fseek(f, 0, SEEK_SET);

        char *buf = malloc(len + 1);
        if (!buf) {
            printf("Failed to allocate memory for JavaScript file\n");
            fclose(f);
            JS_FreeContext(ctx);
            JS_FreeRuntime(rt);
            return;
        }

        fread(buf, 1, len, f);
        buf[len] = '\0';
        fclose(f);

        JSValue eval_ret = JS_Eval(ctx, buf, len, "quickjs_handler.js", JS_EVAL_TYPE_GLOBAL);

        if (JS_IsException(eval_ret)) {
            printf("JavaScript evaluation failed\n");
            JS_FreeValue(ctx, eval_ret);
            JS_FreeContext(ctx);
            JS_FreeRuntime(rt);
            free(buf);
            return;
        }

        JS_FreeValue(ctx, eval_ret);
        free(buf);
    } else {
        printf("Failed to open quickjs_handler.js\n");
    }

    // 呼叫 JavaScript 中的 handleRequest 函數
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue func = JS_GetPropertyStr(ctx, global_obj, "handleRequest");
    JSValue js_url = JS_NewString(ctx, url);
    JSValue result = JS_Call(ctx, func, global_obj, 1, &js_url);

    if (JS_IsException(result)) {
        printf("JavaScript function call failed\n");
    } else {
        const char *str = JS_ToCString(ctx, result);
        printf("JavaScript function returned: %s\n", str);

        // 準備 HTTP 響應
        char response[1024];
        snprintf(response, sizeof(response), 
                 "HTTP/1.1 200 OK\nContent-Type: text/plain\n\n%s", str);
        
        // 發送響應
        send(socket_fd, response, strlen(response), 0);

        JS_FreeCString(ctx, str);
    }

    JS_FreeValue(ctx, result);
    JS_FreeValue(ctx, func);
    JS_FreeValue(ctx, js_url);
    JS_FreeValue(ctx, global_obj);
    JS_RunGC(rt);  // 執行垃圾收集
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
}



int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    //char *hello = "HTTP/1.1 200 OK\nContent-Type: text/plain\n\nHello I am QuickJs Server!\n";

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        return -1;
    }

    // Forcefully attaching socket to the port 3303
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the network address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        return -1;
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        return -1;
    }

    printf("Server is running on http://localhost:3303\n"); 

    while(1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
            perror("accept");
            return -1;
        }

        read(new_socket, buffer, 1024);
        printf("Received request: %s\n", buffer);

        // 解析 HTTP 請求中的 URL
        char *method, *url, *protocol;
        method = strtok(buffer, " ");
        url = strtok(NULL, " ");
        protocol = strtok(NULL, "\r\n");

        if (method && url && protocol) {
            printf("Parsed URL: %s\n", url);
            executeQuickJS(url, new_socket);  // 使用 URL 調用 QuickJS
        }
        printf("Hello message sent\n");
        close(new_socket);
    }

    return 0;
}

