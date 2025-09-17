#include "winsock2.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>

#include "server.h"
#include "handleHttp.h"

using json = nlohmann::json;
using string = std::string;

// 私有方法
bool Server::loadConfig() {
	std::ifstream config_file("./config/config.json");
	if (config_file.is_open()) {
		json config = json::parse(config_file);
		string host = config.value("host", "0.0.0.0");
		u_short port = config.value("port", 5050);
		string path = config.value("path", "./www");
		std::cout << "Web root path: " << path << std::endl;
		this->path = path;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);

		if (host == "0.0.0.0") addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
		else if (host == "localhost") addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
		else addr.sin_addr.S_un.S_addr = inet_addr(host.c_str());

		std::cout << "Configuration loaded: " << host << ":" << port << std::endl;
	}
	else {
		std::cout << "open config.json failed, use default config" << std::endl;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(5050);
		addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY); //主机上任意一块网卡的IP地址
	}
	config_file.close();
	return true;
}

bool Server::initializeSocket() {
	//创建监听socket
	srvSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (srvSocket == INVALID_SOCKET) {
		std::cerr << "Socket create failed with error!" << std::endl;
		return false;
	}
	else std::cout << "Socket create Ok!" << std::endl;

	//绑定
	int rtn = ::bind(srvSocket, (LPSOCKADDR)&addr, sizeof(addr));
	if (rtn == SOCKET_ERROR) {
		std::cerr << "Socket bind failed with error!" << std::endl;
		return false;
	}
	else std::cout << "Socket bind Ok!" << std::endl;

	//监听
	rtn = listen(srvSocket, 5);
	if (rtn == SOCKET_ERROR) {
		std::cerr << "Socket listen failed with error!" << std::endl;
		return false;
	}
	else std::cout << "Socket listen Ok!" << std::endl;
		

	u_long blockMode = 1;//将srvSock设为非阻塞模式以监听客户连接请求

	//调用ioctlsocket，将srvSocket改为非阻塞模式，改成反复检查fd_set元素的状态，看每个元素对应的句柄是否可读或可写
	if ((rtn = ioctlsocket(srvSocket, FIONBIO, &blockMode) == SOCKET_ERROR)) { //FIONBIO：允许或禁止套接口s的非阻塞模式。
		std::cerr << "ioctlsocket() failed with error!\n";
		return false;
	}
	std::cout << "ioctlsocket() for server socket ok!	Waiting for client connection and data\n";

	//设置服务器监听socket到主集合
	FD_SET(srvSocket, &masterReadFds);
	maxFd = srvSocket;

	return true;
}
void Server::updateMaxFd() {
	maxFd = srvSocket;
	for (SOCKET client : clientSockets) {
		if (client > maxFd) {
			maxFd = client;
		}
	}
}

Server::Server() : srvSocket(INVALID_SOCKET), maxFd(0), isRunning(false) {
	FD_ZERO(&masterReadFds);
	FD_ZERO(&masterWriteFds);

	// 初始化服务器地址
	addr.sin_family = AF_INET;
	addr.sin_port = htons(5050);
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
}
Server::~Server() {
	stop();
}

// 公共接口
bool Server::initialize() {
	// 初始化Winsock
	WSADATA wsaData;
	if (WSAStartup(0x0202, &wsaData) != 0) {
		std::cerr << "Winsock startup failed with error!" << std::endl;
		return false;
	}

	if (wsaData.wVersion != 0x0202) {
		std::cerr << "Winsock version is not correct!" << std::endl;
		WSACleanup();
		return false;
	}

	// 加载配置
	if (!loadConfig()) {
		std::cout << "Using default configuration" << std::endl;
	}

	// 初始化socket
	if (!initializeSocket()) {
		std::cerr << " Socket initialization failed!" << std::endl;
		WSACleanup();
		return false;
	}

	std::cout << "Server initialized successfully!" << std::endl;
	return true;
}
void Server::run() {
	if (srvSocket == INVALID_SOCKET) {
		std::cerr << "Server not initialized!" << std::endl;
		return;
	}

	isRunning = true;
	std::cout << "Server started, waiting for connections..." << std::endl;
	// 主循环
	while (isRunning) {
		// 复制主集合到工作集合
		fd_set readFds = masterReadFds;
		fd_set writeFds = masterWriteFds;
		// 设置超时时间
		timeval timeout;
		timeout.tv_sec = 1;  // 1秒
		timeout.tv_usec = 0;
		// 使用select监听多个socket
		int activity = select(maxFd + 1, &readFds, &writeFds, nullptr, &timeout);
		if (activity == SOCKET_ERROR) {
			std::cerr << "select() failed with error!" << std::endl;
			break;
		}
		if (activity == 0) {
			continue;
		}
		// 处理可读socket
		for(SOCKET i = 0; i <= maxFd; ++i) {
			if (FD_ISSET(i, &readFds)) {
				if (i == srvSocket) {
					acceptNewConnection();
				} else {
					handleClientRequest(i);
				}
			}
		}
		// 处理可写socket
		for(SOCKET i = 0; i <= maxFd; ++i) {
			if (FD_ISSET(i, &writeFds)) {
				handleClientResponse(i);
			}
		}
				
	}
}
void Server::stop() {
	isRunning = false;

	// 关闭所有客户端连接
	for (SOCKET client : clientSockets) {
		cleanupClient(client);
	}
	clientSockets.clear();

	// 关闭服务器socket
	if (srvSocket != INVALID_SOCKET) {
		closesocket(srvSocket);
		srvSocket = INVALID_SOCKET;
	}

	FD_ZERO(&masterReadFds);
	FD_ZERO(&masterWriteFds);

	WSACleanup();
	std::cout << "Server stopped" << std::endl;
}

// 连接管理
void Server::acceptNewConnection() {
	sockaddr_in clientAddr;
	int addrLen = sizeof(clientAddr);
	SOCKET clientSocket = accept(srvSocket, (sockaddr*)&clientAddr, &addrLen);
	if (clientSocket == INVALID_SOCKET) {
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK) {
			std::cerr << "accept() failed with error: " << err << std::endl;
		}
		return;
	}

	std::cout << "New connection accepted, socket: " << clientSocket << std::endl;
	// 设置客户端socket为非阻塞模式
	u_long blockMode = 1;
	if (ioctlsocket(clientSocket, FIONBIO, &blockMode) == SOCKET_ERROR) {
		std::cerr << "ioctlsocket() failed with error for client socket!" << std::endl;
		closesocket(clientSocket);
		return;
	}

	// 添加到客户端列表和主集合
	clientSockets.push_back(clientSocket);
	FD_SET(clientSocket, &masterReadFds);
	updateMaxFd();

	//Todo:打印客户端信息
}
void Server::handleClientRequest(SOCKET clientSocket) {
	char buffer[4096];
	int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

	if (bytesReceived > 0) {
		buffer[bytesReceived] = '\0';
		std::cout << "Received " << bytesReceived << " bytes from client " << clientSocket << std::endl;
		std::string requestData(buffer, bytesReceived);
		// 解析HTTP请求
		HttpHandler httpHandler(path);
		HttpRequest reques = httpHandler.parseHttpRequest(requestData);

		clientBuffers[clientSocket] = reques; // 存储请求数据
		// 设置客户端socket为可写，准备发送响应
		FD_SET(clientSocket, &masterWriteFds);
	}
	else if (bytesReceived == 0) {
		std::cout << "Client " << clientSocket << " disconnected" << std::endl;
		cleanupClient(clientSocket);
	}
	else {
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK) {
			std::cerr << "recv() failed with error: " << err << std::endl;
			cleanupClient(clientSocket);
		}
	}
}
void Server::handleClientResponse(SOCKET clientSocket) {
	auto it = clientBuffers.find(clientSocket);
	if (it == clientBuffers.end()) {
		std::cerr << "No request data for client " << clientSocket << std::endl;
		cleanupClient(clientSocket);
		return;
	}

	HttpHandler httpHandler(path);
	std::string responseData = httpHandler.createHttpResponse(it->second).toString();
	int bytesSent = send(clientSocket, responseData.c_str(), responseData.size(), 0);
	if (bytesSent == SOCKET_ERROR) {
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK) {
			std::cerr << "send() failed with error: " << err << std::endl;
			cleanupClient(clientSocket);
			return;
		}
	}
	else {
		std::cout << "Sent " << bytesSent << " bytes to client " << clientSocket << std::endl;
	}
	// 清理请求数据
	FD_CLR(clientSocket, &masterWriteFds);
	clientBuffers.erase(clientSocket);
}
void Server::cleanupClient(SOCKET clientSocket) {
	closesocket(clientSocket);
	FD_CLR(clientSocket, &masterReadFds);
	FD_CLR(clientSocket, &masterWriteFds);

	// 从客户端列表中移除
	auto it = std::find(clientSockets.begin(), clientSockets.end(), clientSocket);
	if (it != clientSockets.end()) {
		clientSockets.erase(it);
	}

	updateMaxFd();
	std::cout << "Client " << clientSocket << " cleaned up" << std::endl;
}

//void Server::onRequestProcessed(SOCKET clientSocket, const std::string& response) {
//	clientBuffers[clientSocket] = response;
//	FD_SET(clientSocket, &masterWriteFds); // 只在Server类内部操作fd_set
//}