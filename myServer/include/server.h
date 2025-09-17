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
	std::map<SOCKET, HttpRequest> clientBuffers; // �洢ÿ���ͻ��˵Ļ�����
    SOCKET maxFd;

    fd_set masterReadFds;
    fd_set masterWriteFds;

    bool isRunning;

    std::string path;

    // ˽�з���
    bool loadConfig();
    bool initializeSocket();
    void updateMaxFd();

public:
    Server();
    ~Server();

    // �����ӿ�
    bool initialize();
    void run();
    void stop();

    // ���ӹ���
    void acceptNewConnection();
    void handleClientRequest(SOCKET clientSocket);
    void handleClientResponse(SOCKET clientSocket);
    void cleanupClient(SOCKET clientSocket);

	////�ص�����ע��
	//void onRequestProcessed(SOCKET clientSocket, const std::string& response);

    // ״̬��ȡ
    size_t getClientCount() const { return clientSockets.size(); }
    bool isServerRunning() const { return isRunning; }
    SOCKET getServerSocket() const { return srvSocket; }
};