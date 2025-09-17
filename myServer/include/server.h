#pragma once
#include "winsock2.h"
#include <vector>
#include <string>
#include <map>
#include "handleHttp.h"

#pragma comment(lib,"ws2_32.lib")

class Server {
private:
    SOCKET srvSocket;
    sockaddr_in addr;
    std::vector<SOCKET> clientSockets{};
	std::map<SOCKET, HttpRequest> clientBuffers; // 存储每个客户端的缓冲区
    SOCKET maxFd;

    fd_set masterReadFds;
    fd_set masterWriteFds;

    bool isRunning;

    std::string path;

    // 私有方法
    bool loadConfig();
    bool initializeSocket();
    void updateMaxFd();

public:
    Server();
    ~Server();

    // 公共接口
    bool initialize();
    void run();
    void stop();

    // 连接管理
    void acceptNewConnection();
    void handleClientRequest(SOCKET clientSocket);
    void handleClientResponse(SOCKET clientSocket);
    void cleanupClient(SOCKET clientSocket);

	////回调函数注册
	//void onRequestProcessed(SOCKET clientSocket, const std::string& response);

    // 状态获取
    size_t getClientCount() const { return clientSockets.size(); }
    bool isServerRunning() const { return isRunning; }
    SOCKET getServerSocket() const { return srvSocket; }
};