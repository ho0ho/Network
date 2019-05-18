#include <windows.h>
#include <process.h>
#include <stdio.h>

#define BUFFERSIZE 1024

SOCKET listenSock;

void DisplayMessage() {
	LPVOID pMsg;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)& pMsg, 0, NULL);
	printf("%s\n", (char*)pMsg);
	LocalFree(pMsg);
}

bool CreateListenSocket() {
	int retval;
	listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSock == INVALID_SOCKET) {
		DisplayMessage();
		return false;
	}

	u_long on = 1;
	retval = ioctlsocket(listenSock, FIONBIO, &on);
	if (retval == SOCKET_ERROR) {
		DisplayMessage();
		return false;
	}
		 
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(40100);
	
	retval = bind(listenSock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) {
		DisplayMessage();
		return false;
	}

	retval = listen(listenSock, SOMAXCONN);
	if (retval == SOCKET_ERROR) {
		DisplayMessage();
		return false;
	}

	return true;
}

unsigned int WINAPI ComThread(void* pParam) {
	SOCKET clientSock = (SOCKET)pParam;
	int recvByte;
	char buf[BUFFERSIZE];
	SOCKADDR_IN clientaddr;

	while (1) {
		recvByte = recv(clientSock, buf, BUFFERSIZE, 0);
		if (recvByte == SOCKET_ERROR) {
			if (WSAGetLastError() != WSAEWOULDBLOCK) {
				DisplayMessage();
				break;
			}
		}
		else if (recvByte == 0) {
			DisplayMessage();
			break;				
		}
		else {
			int addrlen = sizeof(clientaddr);
			int retval = getpeername(clientSock, (SOCKADDR*)& clientaddr, &addrlen);
			if (retval == SOCKET_ERROR) {
				DisplayMessage();
				return;
			}
			buf[recvByte] = '\0';
			printf("[TCP 서버] IP = %s, Port = %d의 메시지: %s",	inet_ntoa(clientaddr.sin_addr),	ntohs(clientaddr.sin_port));

			retval = send(clientSock, buf, recvByte, 0);
			if (retval == SOCKET_ERROR) {
				DisplayMessage();
				break;
			}
		}
	}
	closesocket(clientSock);
	printf("\n[TCP 서버] 클라이언트 종료: IP 주소 = %s, Port 번호 = %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
	return 0;
}

unsigned int WINAIP ListenThread(void* pParam) {
	while (1) {

	}
}