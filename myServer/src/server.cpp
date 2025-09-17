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

// ˽�з���
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
		addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY); //����������һ��������IP��ַ
	}
	config_file.close();
	return true;
}

bool Server::initializeSocket() {
	//��������socket
	srvSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (srvSocket == INVALID_SOCKET) {
		std::cerr << "Socket create failed with error!" << std::endl;
		return false;
	}
	else std::cout << "Socket create Ok!" << std::endl;

	//��
	int rtn = ::bind(srvSocket, (LPSOCKADDR)&addr, sizeof(addr));
	if (rtn == SOCKET_ERROR) {
		std::cerr << "Socket bind failed with error!" << std::endl;
		return false;
	}
	else std::cout << "Socket bind Ok!" << std::endl;

	//����
	rtn = listen(srvSocket, 5);
	if (rtn == SOCKET_ERROR) {
		std::cerr << "Socket listen failed with error!" << std::endl;
		return false;
	}
	else std::cout << "Socket listen Ok!" << std::endl;
		

	u_long blockMode = 1;//��srvSock��Ϊ������ģʽ�Լ����ͻ���������

	//����ioctlsocket����srvSocket��Ϊ������ģʽ���ĳɷ������fd_setԪ�ص�״̬����ÿ��Ԫ�ض�Ӧ�ľ���Ƿ�ɶ����д
	if ((rtn = ioctlsocket(srvSocket, FIONBIO, &blockMode) == SOCKET_ERROR)) { //FIONBIO��������ֹ�׽ӿ�s�ķ�����ģʽ��
		std::cerr << "ioctlsocket() failed with error!\n";
		return false;
	}
	std::cout << "ioctlsocket() for server socket ok!	Waiting for client connection and data\n";

	//���÷���������socket��������
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

	// ��ʼ����������ַ
	addr.sin_family = AF_INET;
	addr.sin_port = htons(5050);
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
}
Server::~Server() {
	stop();
}

// �����ӿ�
bool Server::initialize() {
	// ��ʼ��Winsock
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

	// ��������
	if (!loadConfig()) {
		std::cout << "Using default configuration" << std::endl;
	}

	// ��ʼ��socket
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
	// ��ѭ��
	while (isRunning) {
		// ���������ϵ���������
		fd_set readFds = masterReadFds;
		fd_set writeFds = masterWriteFds;
		// ���ó�ʱʱ��
		timeval timeout;
		timeout.tv_sec = 1;  // 1��
		timeout.tv_usec = 0;
		// ʹ��select�������socket
		int activity = select(maxFd + 1, &readFds, &writeFds, nullptr, &timeout);
		if (activity == SOCKET_ERROR) {
			std::cerr << "select() failed with error!" << std::endl;
			break;
		}
		if (activity == 0) {
			continue;
		}
		// ����ɶ�socket
		for(SOCKET i = 0; i <= maxFd; ++i) {
			if (FD_ISSET(i, &readFds)) {
				if (i == srvSocket) {
					acceptNewConnection();
				} else {
					handleClientRequest(i);
				}
			}
		}
		// �����дsocket
		for(SOCKET i = 0; i <= maxFd; ++i) {
			if (FD_ISSET(i, &writeFds)) {
				handleClientResponse(i);
			}
		}
				
	}
}
void Server::stop() {
	isRunning = false;

	// �ر����пͻ�������
	for (SOCKET client : clientSockets) {
		cleanupClient(client);
	}
	clientSockets.clear();

	// �رշ�����socket
	if (srvSocket != INVALID_SOCKET) {
		closesocket(srvSocket);
		srvSocket = INVALID_SOCKET;
	}

	FD_ZERO(&masterReadFds);
	FD_ZERO(&masterWriteFds);

	WSACleanup();
	std::cout << "Server stopped" << std::endl;
}

// ���ӹ���
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
	// ���ÿͻ���socketΪ������ģʽ
	u_long blockMode = 1;
	if (ioctlsocket(clientSocket, FIONBIO, &blockMode) == SOCKET_ERROR) {
		std::cerr << "ioctlsocket() failed with error for client socket!" << std::endl;
		closesocket(clientSocket);
		return;
	}

	// ��ӵ��ͻ����б��������
	clientSockets.push_back(clientSocket);
	FD_SET(clientSocket, &masterReadFds);
	updateMaxFd();

	//Todo:��ӡ�ͻ�����Ϣ
}
void Server::handleClientRequest(SOCKET clientSocket) {
	char buffer[4096];
	int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

	if (bytesReceived > 0) {
		buffer[bytesReceived] = '\0';
		std::cout << "Received " << bytesReceived << " bytes from client " << clientSocket << std::endl;
		std::string requestData(buffer, bytesReceived);
		// ����HTTP����
		HttpHandler httpHandler(path);
		HttpRequest reques = httpHandler.parseHttpRequest(requestData);

		clientBuffers[clientSocket] = reques; // �洢��������
		// ���ÿͻ���socketΪ��д��׼��������Ӧ
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
	// ������������
	FD_CLR(clientSocket, &masterWriteFds);
	clientBuffers.erase(clientSocket);
}
void Server::cleanupClient(SOCKET clientSocket) {
	closesocket(clientSocket);
	FD_CLR(clientSocket, &masterReadFds);
	FD_CLR(clientSocket, &masterWriteFds);

	// �ӿͻ����б����Ƴ�
	auto it = std::find(clientSockets.begin(), clientSockets.end(), clientSocket);
	if (it != clientSockets.end()) {
		clientSockets.erase(it);
	}

	updateMaxFd();
	std::cout << "Client " << clientSocket << " cleaned up" << std::endl;
}

//void Server::onRequestProcessed(SOCKET clientSocket, const std::string& response) {
//	clientBuffers[clientSocket] = response;
//	FD_SET(clientSocket, &masterWriteFds); // ֻ��Server���ڲ�����fd_set
//}