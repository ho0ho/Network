#include <WinSock2.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		printf("������ ���� �ʱ�ȭ ����!\n");
		return -1;
	}
	/*printf("������ ���� �ʱ�ȭ ����!\n"); */

	//const char *ipaddr = "230.200.12.5";
	//printf("IP ���ڿ� �ּ�: %s\n", ipaddr);
	//printf("IP ���ڿ� �ּ� => ���� = 0x%08X\n", inet_addr(ipaddr));

	//IN_ADDR in_addr;
	//in_addr.s_addr = inet_addr(ipaddr);
	//printf("IP ���� => ���ڿ� �ּ� = %s\n", inet_ntoa(in_addr));

	unsigned short us = 0x1234;
	unsigned long ul = 0x12345678;

	printf("0x%08X => 0x%08X\n", us, htons(us));
	printf("0x%08X => 0x%08X\n", ul, htonl(ul));

	unsigned short n_us = htons(us);
	unsigned long n_ul = htonl(ul);

	printf("0x%08X => 0x%08X\n", n_us, ntohs(n_us));
	printf("0x%08X => 0x%08X\n", n_ul, ntohl(n_ul));

	WSACleanup(); 
	return 0;
}
