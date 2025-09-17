#pragma once
#include "winsock2.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>
#include <vector>
#include "handleHttp.h"
#include "server.h"


using namespace std;
using json = nlohmann::json;

#pragma comment(lib,"ws2_32.lib")

int main() {
	//八股文
	WSADATA wsaData;
	int nRc = WSAStartup(0x0202, &wsaData);
	if (nRc) cout<<"Winsock  startup failed with error!"<<endl;
	if (wsaData.wVersion != 0x0202) cout<<"Winsock version is not correct!"<<endl;

	cout<<"Winsock  startup Ok!"<<endl;

	//主从套接字
	fd_set readfds, writefds;
	fd_set master_readfds, master_writefds;

	//初始化主集合
	FD_ZERO(&master_readfds);
	FD_ZERO(&master_writefds);

	
	SOCKET srvSocket;					//监听socket
	sockaddr_in addr;					//服务器地址
	vector<sockaddr_in> clientAddrs;	//客户端地址
	vector<SOCKET>clientSockets;		//会话socket，负责和client进程通信
	SOCKET max_fd = 0;					//最大句柄值
	int addrLen = 0;					//ip地址长度

	//创建监听socket
	srvSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (srvSocket != INVALID_SOCKET)
		printf("Socket create Ok!\n");


	//加载配置服务器的端口和地址
	


	//binding
	int rtn = ::bind(srvSocket, (LPSOCKADDR)&addr, sizeof(addr));
	if (rtn != SOCKET_ERROR)
		printf("Socket bind Ok!\n");

	//监听
	rtn = listen(srvSocket, 5);
	if (rtn != SOCKET_ERROR)
		printf("Socket listen Ok!\n");

	u_long blockMode = 1;//将srvSock设为非阻塞模式以监听客户连接请求

	//调用ioctlsocket，将srvSocket改为非阻塞模式，改成反复检查fd_set元素的状态，看每个元素对应的句柄是否可读或可写
	if ((rtn = ioctlsocket(srvSocket, FIONBIO, &blockMode) == SOCKET_ERROR)) { //FIONBIO：允许或禁止套接口s的非阻塞模式。
		cout << "ioctlsocket() failed with error!\n";
		return;
	}
	cout << "ioctlsocket() for server socket ok!	Waiting for client connection and data\n";

	//设置服务器监听socket到主集合
	FD_SET(srvSocket, &master_readfds);
	max_fd = srvSocket;

	while (true) {
		//从主集合拷贝一份到工作集合
		readfds = master_readfds;
		writefds = master_writefds;

		//返回总共可以读或写的句柄个数
		int activity = select(max_fd + 1, &readfds, &writefds, NULL, NULL);

		if (activity == SOCKET_ERROR) {
			cerr << "select error: " << WSAGetLastError() << endl;
			break;
		}

		//如果srvSock收到连接请求，接受客户连接请求
		for (int i = 0; i <=max_fd; i++) {
			if (FD_ISSET(i, &readfds)) {
				if(i==srvSocket) {
					//有新的客户端连接请求
					acceptNewConnection(i ,readfds);
				}
				else {
					//处理已有的客户连接
					handleClientRequest(i, readfds);
				}
			}
		}

		//处理可写的socket
		for (int i = 0; i < activity; i++) {
			if (FD_ISSET(i, &writefds)) {
				//处理已有的客户连接
				handleClientResponse(i, writefds);
			}
		}
		
	}

	for (SOCKET client : clientSockets) {
		closesocket(client);
	}
	closesocket(srvSocket);
	WSACleanup();
	return 0;
}
