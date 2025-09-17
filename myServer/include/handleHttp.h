#pragma once
#include "winsock2.h"
#include <string>
#include <unordered_map>
#include <functional>

using string = std::string;

// HTTP����ṹ��
struct HttpRequest {
    string method;
    string path;
    string version;
    std::unordered_map<string, string> headers;
    string body;

    string getHeader(const string& key) const;
};

// HTTP��Ӧ�ṹ��
struct HttpResponse {
    int statusCode;
    string statusText;
    std::unordered_map<string, string> headers;
    string body;

    string toString() const;
};

// ·�ɴ���������
using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;

class HttpHandler {
private:
    std::unordered_map<string, RouteHandler> routes;
    string webRoot; // ��ҳ��Ŀ¼

    // ������ط���
    bool parseRequestLine(const string& line, HttpRequest& request);
    bool parseHeaders(const string& headersStr, HttpRequest& request);
    string getMimeType(const string& filePath) const;

    // Ĭ��·�ɴ���
    HttpResponse handleStaticFile(const HttpRequest& request);
    HttpResponse handleNotFound(const HttpRequest& request);

public:
    HttpHandler();
    explicit HttpHandler(const string& rootPath);

    // ·�ɹ���
    void addRoute(const string& path, RouteHandler handler);
    void setWebRoot(const string& rootPath);

    // ���Ĵ�����
    HttpRequest parseHttpRequest(const string& request);
    HttpResponse createHttpResponse(const HttpRequest& request);

    string createHttpResponse(const string& file_path); // ���ذ汾

    // ���ߺ���
    static string urlDecode(const string& str);
    static string readFile(const string& filePath);
    static bool fileExists(const string& filePath);
};