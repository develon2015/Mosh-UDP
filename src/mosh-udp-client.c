/**
 * 由于2.0版本出现问题, 现推出3.0版本, 不向后兼容
 * 
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>



int
resolveIPv4(const char *domain, struct in_addr *sin_addr) {
	struct hostent *ent = gethostbyname2(domain, AF_INET);
	if (ent == NULL) {
		printf("[E] can't resolve the IP: %s\n", hstrerror(h_errno));
		return -1;
	}
	*sin_addr = *(struct in_addr *)ent->h_addr;
	return 0;
}

void
info(const char *pname) {
	printf("Usage:\n\t%s\t<IP> <Port> [session-port] [public-port]\n", pname);
	printf("\tsession-port指定服务器监听端口，默认是60001以上自增的。public-port用于NAT指定公网映射的端口\n");
	printf("版本:\tversion 3.0\n");
	printf("\t" __DATE__ " " __TIME__ "\n");
}

int
main(int argc, char *argv[]) {
        if (argc < 3) {
L:
		info(argv[0]);
		return 0;
	}
	int session_port = 0; /* 会话端口 */
	if (argc >= 4 && sscanf(argv[3], "%d", &session_port) != 1) {
L_unknown_port:
		printf("%s 表示您想使用哪个UDP端口? 请提供正确的数字, 0代表默认\n", argv[3]);
		return 1;
	}
	if (session_port > 65535 || session_port < 0)
		goto L_unknown_port;
	int public_port = 0; /* 公网端口映射 */
	if (argc == 5 && sscanf(argv[4], "%d", &public_port) != 1) {
L_unknown_public_port:
		printf("%s 表示您想连接到哪个UDP端口? 请提供正确的数字, 默认是服务器返回的随机端口\n", argv[4]);
		return 1;
	}
	if (public_port > 65535 || public_port < 0)
		goto L_unknown_public_port;
	int port = 0; /* 服务端监听端口 */
	if (sscanf(argv[2], "%d", &port) != 1)
		goto L;
	int sfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sfd < 0) {
		perror("socket");
		close(sfd);
		return 0;
	}

	struct sockaddr_in addr = { 0 };
	addr.sin_family = AF_INET;
	if (resolveIPv4(argv[1], &addr.sin_addr) != 0) {
		puts("连接失败");
		close(sfd);
		return 1;
	}
	printf("Connecting %s:%d...\n", inet_ntoa(addr.sin_addr), port);
	addr.sin_port = htons(port); 

	char buf[1024] = { 0 };
	char pwd[1024] = { 0 };
	sprintf(buf, "TEST");
	sendto(sfd, buf, strlen(buf), 0, (struct sockaddr *)&addr, (socklen_t)sizeof addr);
	memset(buf, 0, 1024);
	printf("测试连接...\n");
	recvfrom(sfd, buf, 1024, 0, 0, 0);
	printf("请输入密码: ");
	fgets(pwd, 1024, stdin);
	sprintf(buf, "LOGIN %s %d 0xArimuraKasumi", pwd, session_port);
	sendto(sfd, buf, strlen(buf), 0, (struct sockaddr *)&addr, (socklen_t)sizeof addr);
	memset(buf, 0, 1024);
	printf("等待响应, 超时请中断命令...\n");
	recvfrom(sfd, buf, 1024, 0, 0, 0);
	printf("服务器回应: %s\n", buf);

	int buf_port;
	char buf_key[1024] = { 0 };
	char cmd[1024] = { 0 };
	if (sscanf(buf, "GO! %d %s #", &buf_port, buf_key) != 2) {
		printf("登录失败, 请重试!\n");
		close(sfd);
		return 0;
	}
	if (public_port == 0)
		sprintf(cmd, "MOSH_KEY=%s mosh-client %s %d", buf_key, inet_ntoa(addr.sin_addr), buf_port);
	else
		sprintf(cmd, "MOSH_KEY=%s mosh-client %s %d", buf_key, inet_ntoa(addr.sin_addr), public_port);

	printf("%s\n", cmd);
	close(sfd);
	system(cmd);
	return 0;
}

