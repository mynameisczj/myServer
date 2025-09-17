#pragma once
#include "winsock2.h"
#include <string>
#include <unordered_map>
#include <functional>

using string = std::string;

// HTTP请求结构体
struct HttpRequest {
    string method;
    string path;
    string version;
    std::unordered_map<string, string> headers;
    string body;

    string getHeader(const string& key) const;
};

// HTTP响应结构体
struct HttpResponse {
    int statusCode;
    string statusText;
    std::unordered_map<string, string> headers;
    string body;

    string toString() const;
};

// 路由处理函数类型
using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;

class HttpHandler {
private:
    std::unordered_map<string, RouteHandler> routes;
    string webRoot; // 网页根目录

    // 解析相关方法
    bool parseRequestLine(const string& line, HttpRequest& request);
    bool parseHeaders(const string& headersStr, HttpRequest& request);
    string getMimeType(const string& filePath) const;

    // 默认路由处理
    HttpResponse handleStaticFile(const HttpRequest& request);
    HttpResponse handleNotFound(const HttpRequest& request);

public:
    HttpHandler();
    explicit HttpHandler(const string& rootPath);

    // 路由管理
    void addRoute(const string& path, RouteHandler handler);
    void setWebRoot(const string& rootPath);

    // 核心处理函数
    HttpRequest parseHttpRequest(const string& request);
    HttpResponse createHttpResponse(const HttpRequest& request);

    string createHttpResponse(const string& file_path); // 重载版本

    // 工具函数
    static string urlDecode(const string& str);
    static string readFile(const string& filePath);
    static bool fileExists(const string& filePath);
};