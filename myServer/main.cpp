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
	//�˹���
	WSADATA wsaData;
	int nRc = WSAStartup(0x0202, &wsaData);
	if (nRc) cout<<"Winsock  startup failed with error!"<<endl;
	if (wsaData.wVersion != 0x0202) cout<<"Winsock version is not correct!"<<endl;

	cout<<"Winsock  startup Ok!"<<endl;

	//�����׽���
	fd_set readfds, writefds;
	fd_set master_readfds, master_writefds;

	//��ʼ��������
	FD_ZERO(&master_readfds);
	FD_ZERO(&master_writefds);

	
	SOCKET srvSocket;					//����socket
	sockaddr_in addr;					//��������ַ
	vector<sockaddr_in> clientAddrs;	//�ͻ��˵�ַ
	vector<SOCKET>clientSockets;		//�Ựsocket�������client����ͨ��
	SOCKET max_fd = 0;					//�����ֵ
	int addrLen = 0;					//ip��ַ����

	//��������socket
	srvSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (srvSocket != INVALID_SOCKET)
		printf("Socket create Ok!\n");


	//�������÷������Ķ˿ں͵�ַ
	


	//binding
	int rtn = ::bind(srvSocket, (LPSOCKADDR)&addr, sizeof(addr));
	if (rtn != SOCKET_ERROR)
		printf("Socket bind Ok!\n");

	//����
	rtn = listen(srvSocket, 5);
	if (rtn != SOCKET_ERROR)
		printf("Socket listen Ok!\n");

	u_long blockMode = 1;//��srvSock��Ϊ������ģʽ�Լ����ͻ���������

	//����ioctlsocket����srvSocket��Ϊ������ģʽ���ĳɷ������fd_setԪ�ص�״̬����ÿ��Ԫ�ض�Ӧ�ľ���Ƿ�ɶ����д
	if ((rtn = ioctlsocket(srvSocket, FIONBIO, &blockMode) == SOCKET_ERROR)) { //FIONBIO��������ֹ�׽ӿ�s�ķ�����ģʽ��
		cout << "ioctlsocket() failed with error!\n";
		return;
	}
	cout << "ioctlsocket() for server socket ok!	Waiting for client connection and data\n";

	//���÷���������socket��������
	FD_SET(srvSocket, &master_readfds);
	max_fd = srvSocket;

	while (true) {
		//�������Ͽ���һ�ݵ���������
		readfds = master_readfds;
		writefds = master_writefds;

		//�����ܹ����Զ���д�ľ������
		int activity = select(max_fd + 1, &readfds, &writefds, NULL, NULL);

		if (activity == SOCKET_ERROR) {
			cerr << "select error: " << WSAGetLastError() << endl;
			break;
		}

		//���srvSock�յ��������󣬽��ܿͻ���������
		for (int i = 0; i <=max_fd; i++) {
			if (FD_ISSET(i, &readfds)) {
				if(i==srvSocket) {
					//���µĿͻ�����������
					acceptNewConnection(i ,readfds);
				}
				else {
					//�������еĿͻ�����
					handleClientRequest(i, readfds);
				}
			}
		}

		//�����д��socket
		for (int i = 0; i < activity; i++) {
			if (FD_ISSET(i, &writefds)) {
				//�������еĿͻ�����
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
