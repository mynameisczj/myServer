#pragma once
#include "winsock2.h"
#include "handleHttp.h"

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cctype>

using string = std::string;

std::vector<string> split(const string& str, char delimiter) {
    std::vector<string> tokens;
    std::stringstream ss(str);
    string token;
    while (std::getline(ss, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

// 辅助函数：字符串修剪
string trim(const string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

HttpHandler::HttpHandler() : webRoot(".") {}
HttpHandler::HttpHandler(const string& rootPath) : webRoot(rootPath) {}

void HttpHandler::setWebRoot(const string& rootPath) {
	webRoot = rootPath;
}

string HttpRequest::getHeader(const string& key) const {
    auto it = headers.find(key);
    return it != headers.end() ? it->second : "";
}

void HttpHandler::addRoute(const string& path, RouteHandler handler) {
    routes[path] = handler;
}

string HttpResponse::toString() const {
    std::stringstream ss;
    ss << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";

    for (const auto& header : headers) {
        ss << header.first << ": " << header.second << "\r\n";
    }

    if (!body.empty()) {
        ss << "Content-Length: " << body.length() << "\r\n";
    }

    ss << "\r\n" << body;
    return ss.str();
}

HttpRequest HttpHandler::parseHttpRequest(const string& request) {
    HttpRequest httpRequest;

    size_t pos = request.find("\r\n\r\n");
    string headersPart = request.substr(0, pos);
    if (pos != string::npos) {
        httpRequest.body = request.substr(pos + 4);
    }

    // 分割请求行和头部
    size_t lineEnd = headersPart.find("\r\n");
    if (lineEnd != string::npos) {
        string requestLine = headersPart.substr(0, lineEnd);
        parseRequestLine(requestLine, httpRequest);

        string headersStr = headersPart.substr(lineEnd + 2);
        parseHeaders(headersStr, httpRequest);
    }

    return httpRequest;
}

bool HttpHandler::parseRequestLine(const string& line, HttpRequest& request) {
    std::vector<string> parts = split(line, ' ');
    if (parts.size() >= 3) {
        request.method = parts[0];
        request.path = parts[1];
        request.version = parts[2];
        std::cout << request.path << std::endl;
        return true;
    }
    return false;
}

bool HttpHandler::parseHeaders(const string& headersStr, HttpRequest& request) {
    std::vector<string> lines = split(headersStr, '\n');
    for (const auto& line : lines) {
        size_t colonPos = line.find(':');
        if (colonPos != string::npos) {
            string key = trim(line.substr(0, colonPos));
            string value = trim(line.substr(colonPos + 1));
            request.headers[key] = value;
        }
    }
    return true;
}

HttpResponse HttpHandler::createHttpResponse(const HttpRequest& request) {
    // 检查是否有自定义路由
    auto it = routes.find(request.path);
    if (it != routes.end()) {
        return it->second(request);
    }

    // 默认处理静态文件
    return handleStaticFile(request);
}

HttpResponse HttpHandler::handleStaticFile(const HttpRequest& request) {
    HttpResponse response;
    string filePath = webRoot + request.path;

    // 默认文件
    if (request.path == "/") {
        filePath = webRoot + "/index.html";
    }
    std::cout << filePath << std::endl;
    if (fileExists(filePath)) {
        response.statusCode = 200;
        response.statusText = "OK";
        response.headers["Content-Type"] = getMimeType(filePath);
        response.body = readFile(filePath);
    }
    else {
        response = handleNotFound(request);
    }

    return response;
}

HttpResponse HttpHandler::handleNotFound(const HttpRequest& request) {
    HttpResponse response;
    response.statusCode = 404;
    response.statusText = "Not Found";
    response.headers["Content-Type"] = "text/html";
    response.body = R"(
        <html>
            <head><title>404 Not Found</title></head>
            <body>
                <h1>404 Not Found</h1>
                <p>The requested URL )" + request.path + R"( was not found on this server.</p>
            </body>
        </html>
    )";
    return response;
}

string HttpHandler::getMimeType(const string& filePath) const {
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos != string::npos) {
        string ext = filePath.substr(dotPos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == "html" || ext == "htm") return "text/html";
        if (ext == "css") return "text/css";
        if (ext == "js") return "application/javascript";
        if (ext == "json") return "application/json";
        if (ext == "png") return "image/png";
        if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
        if (ext == "gif") return "image/gif";
        if (ext == "ico") return "image/x-icon";
    }
    return "text/plain";
}

string HttpHandler::urlDecode(const string& str) {
    string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            int value;
            std::istringstream iss(str.substr(i + 1, 2));
            if (iss >> std::hex >> value) {
                result += static_cast<char>(value);
                i += 2;
            }
        }
        else if (str[i] == '+') {
            result += ' ';
        }
        else {
            result += str[i];
        }
    }
    return result;
}

string HttpHandler::readFile(const string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) return "";

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool HttpHandler::fileExists(const string& filePath) {
    std::ifstream file(filePath);
    std::cout<< file.good()<<std::endl;
    return file.good();
}