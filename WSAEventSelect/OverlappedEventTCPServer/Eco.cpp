#include <WinSock2.h>
#include <process.h>
#include <stdio.h>

#define BUFSIZE 1024

struct SOCKETINFO {
	WSAOVERLAPPED overlapped;
	WSABUF wsabuf;		// WSASend()�� buf���� ������ WSABUFŸ������ �ޱ⶧���� �ʿ���
						// wsabuf.buf�� char *Ÿ������, ���뿡 ���� �ּҸ� ���� �� ���� ��, ���� ������ �ٷ� ������ ����
	SOCKET sock;
	char buf[BUFSIZE];	// �� buf������ char[]���̶� ���� �����͸� �ٷ� ���� �� ����
	int recvbytes;
	int sendbytes;
};

CRITICAL_SECTION cs;

SOCKET listenSock;
int nTotalSockets = 0;

SOCKETINFO* SocketInfoArray[WSA_MAXIMUM_WAIT_EVENTS];
WSAEVENT EventArray[WSA_MAXIMUM_WAIT_EVENTS];

void DisplayMessage() {
	LPVOID pMsg;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)& pMsg, 0, NULL);
	printf("%s\n", pMsg);
	LocalFree(pMsg);
}

BOOL AddSocketInfo(SOCKET sock) {
	EnterCriticalSection(&cs);

	if (nTotalSockets >= WSA_MAXIMUM_WAIT_EVENTS) {
		printf("[����] ���� ������ �߰��� �� �����ϴ�!\n");
		return false;
	}

	SOCKETINFO* ptr = new SOCKETINFO;
	if (ptr == NULL) {
		printf("[����] �޸𸮰� �����մϴ�!\n");
		return false;
	}

	WSAEVENT hEvent = WSACreateEvent();
	if (hEvent == WSA_INVALID_EVENT) {
		DisplayMessage();
		return false;
	}

	ZeroMemory(&(ptr->overlapped), sizeof(ptr->overlapped));
	ptr->overlapped.hEvent = hEvent;
	ptr->wsabuf.buf = ptr->buf;
	ptr->wsabuf.len = BUFSIZE;
	ptr->sock = sock;
	ptr->recvbytes = 0;
	ptr->sendbytes = 0;

	SocketInfoArray[nTotalSockets] = ptr;
	EventArray[nTotalSockets] = hEvent;
	nTotalSockets++;

	LeaveCriticalSection(&cs);
	return TRUE;
}

void RemoveSocketInfo(int nIndex) {
	EnterCriticalSection(&cs);

	SOCKETINFO* ptr = SocketInfoArray[nIndex];
	//SOCKADDR_IN clientaddr;
	//int addrlen = sizeof(clientaddr);
	//getpeername(ptr->sock, (SOCKADDR*)& clientaddr, &addrlen);
	//printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ� = %s, port ��ȣ = %d\n",
	//	inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
	closesocket(ptr->sock);
	delete ptr;
	WSACloseEvent(EventArray[nIndex]);
	for (int i = nIndex; i < nTotalSockets; i++) {
		SocketInfoArray[i] = SocketInfoArray[i + 1];
		EventArray[i] = EventArray[i + 1];
	}
	nTotalSockets--;

	LeaveCriticalSection(&cs);
}

bool CreateListenSocket() {
	listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSock == INVALID_SOCKET) {
		DisplayMessage();
		return FALSE;
	}

	if (AddSocketInfo(listenSock) == FALSE) return false;

	//WSAEVENT hEvent = WSACreateEvent();
	//if (hEvent == WSA_INVALID_EVENT) {
	//	DisplayMessage();
	//	return FALSE;
	//}
	//EventArray[nTotalSockets++] = hEvent;

	//int retval = WSAEventSelect(listenSock, EventArray[nTotalSockets - 1], FD_ACCEPT | FD_CLOSE);
	//if (retval == SOCKET_ERROR) {
	//	DisplayMessage();
	//	return FALSE;
	//}

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(40100);
	int retval = bind(listenSock, (SOCKADDR*)& serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) {
		DisplayMessage();
		return FALSE;
	}

	retval = listen(listenSock, SOMAXCONN);
	if (retval == SOCKET_ERROR) {
		DisplayMessage();
		return FALSE;
	}

	return TRUE;
}

unsigned int WINAPI WorkerThread(void* pParam) {
	while (1) {
		DWORD index = WSAWaitForMultipleEvents(nTotalSockets, EventArray, FALSE, WSA_INFINITE, FALSE);
		if (index == WSA_WAIT_FAILED) {
			DisplayMessage();
			continue;
		}
		index -= WSA_WAIT_EVENT_0;
		WSAResetEvent(EventArray[index]);
		if (index == 0) continue;

		SOCKETINFO * ptr = SocketInfoArray[index];
		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(ptr->sock, (SOCKADDR*)& clientaddr, &addrlen);

		DWORD cbTransferred, flags;
		int retval = WSAGetOverlappedResult(ptr->sock, &(ptr->overlapped), &cbTransferred, FALSE, &flags);
		if (retval == FALSE || cbTransferred == 0) {
			if (retval == FALSE) DisplayMessage();
			RemoveSocketInfo(index);
			printf("[TCP/ %s:%d]\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
			continue;
		}

		if (ptr->recvbytes == 0) {
			ptr->recvbytes = cbTransferred;
			ptr->sendbytes = 0;
			ptr->buf[ptr->recvbytes] = '\0';
			printf("[TCP/ %s:%d] %s\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), ptr->buf);
		}
		else
			ptr->sendbytes += cbTransferred;

		if (ptr->recvbytes > ptr->sendbytes) {
			ZeroMemory(&(ptr->overlapped), sizeof(ptr->overlapped));
			ptr->overlapped.hEvent = EventArray[index];
			ptr->wsabuf.buf = ptr->buf + ptr->sendbytes;
			ptr->wsabuf.len = ptr->recvbytes - ptr->sendbytes;

			DWORD sendbytes;
			retval = WSASend(ptr->sock, &(ptr->wsabuf), 1, &sendbytes, 0, &(ptr->overlapped), NULL);
			if (retval == SOCKET_ERROR) {
				if (WSAGetLastError() != WSA_IO_PENDING)
					DisplayMessage();
				continue;
			}
		}
		else {
			ptr->recvbytes = 0;

			ZeroMemory(&(ptr->overlapped), sizeof(ptr->overlapped));
			ptr->overlapped.hEvent = EventArray[index];
			ptr->wsabuf.buf = ptr->buf/* + ptr->sendbytes*/;
			ptr->wsabuf.len = BUFSIZE;

			DWORD recvbytes;
			flags = 0;
			retval = WSARecv(ptr->sock, &(ptr->wsabuf), 1, &recvbytes, &flags, &(ptr->overlapped), NULL);
			if (retval == SOCKET_ERROR) {
				if (WSAGetLastError() != WSA_IO_PENDING)
					DisplayMessage();
				continue;
			}
		}
	}
	return 0;
}

unsigned int WINAPI ListenThread(void* pParam) {
	while (1) {
		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		SOCKET client_sock = accept(listenSock, (SOCKADDR*)& clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			DisplayMessage();
			continue;
		}
		printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ� = %s, port ��ȣ = %d\n",
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
		
		if (AddSocketInfo(client_sock) == FALSE) {
			closesocket(client_sock);
			printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ� = %s, port ��ȣ = %d\n",
				inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
			continue;
		}

		SOCKETINFO* ptr = SocketInfoArray[nTotalSockets - 1];
		DWORD recvbytes;
		DWORD flags = 0;
		int retval = WSARecv(ptr->sock, &(ptr->wsabuf), 1, &recvbytes, &flags, &(ptr->overlapped), NULL);
		if (retval == SOCKET_ERROR) {
			if (WSAGetLastError() != WSA_IO_PENDING) {
				DisplayMessage();
				RemoveSocketInfo(nTotalSockets - 1);
				continue;
			}
		}

		if (WSASetEvent(EventArray[0]) == FALSE) {
			DisplayMessage();
			break;
		}
	}
	closesocket(listenSock);
	return 0;
}

int main(int argc, char* argv[]) {
	WSADATA wsa;
	InitializeCriticalSection(&cs);
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		printf("������ ���� �ʱ�ȭ ����!\n");
		return -1;
	}
	if (!CreateListenSocket()) {
		printf("��� ���� ���� ����! \n");
		return -1;
	}

	unsigned int threadID;
	HANDLE threadArray[2];
	threadArray[0] = (HANDLE)_beginthreadex(0, 0, ListenThread, 0, 0, &threadID);
	threadArray[1] = (HANDLE)_beginthreadex(0, 0, WorkerThread, 0, 0, &threadID);
	WaitForMultipleObjects(2, threadArray, TRUE, INFINITE);

	WSACleanup();
	DeleteCriticalSection(&cs);
	return 0;
}