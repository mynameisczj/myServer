#include "winsock2.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>

#include "server.h"

using json = nlohmann::json;
// 私有方法
bool Server::loadConfig() {
	std::ifstream config_file("config.json");
	if (config_file.is_open()) {
		json config = json::parse(config_file);
		std::string host = config.value("host", "0.0.0.0");
		u_short port = config.value("port", 5050);
		std::string document_root = config.value("document_root", "./www");

		std::cout << "host: " << host << std::endl;
		std::cout << "port: " << port << std::endl;
		std::cout << "document_root: " << document_root << std::endl;

		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);

		if (host == "0.0.0.0") addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
		else if (host == "localhost") addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
		else addr.sin_addr.S_un.S_addr = inet_addr(host.c_str());
	}
	else {
		std::cout << "open config.json failed, use default config" << std::endl;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(5050);
		addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY); //主机上任意一块网卡的IP地址
	}
	config_file.close();
}
bool Server::initializeSocket() {

}
void Server::updateMaxFd() {

}

Server::Server() {

}
Server::~Server() {

}

// 公共接口
bool Server::initialize() {

}
void Server::run() {

}
void Server::stop() {

}

// 连接管理
void Server::acceptNewConnection() {

}
void Server::handleClientRequest(SOCKET clientSocket) {

}
void Server::handleClientResponse(SOCKET clientSocket) {

}
void Server::cleanupClient(SOCKET clientSocket) {

}
