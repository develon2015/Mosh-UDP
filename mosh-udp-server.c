/*####################################
# Mosh-UDP daemon
# Version 3.0
######################################*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <termios.h>
#include <unistd.h>
#include <sys/param.h>
#include <fcntl.h>
#include <wait.h>

char **Argv = NULL;

void 
init_daemon() {
	int logfd = open("/tmp/moshd.log", O_CREAT | O_WRONLY, 0666);
	if (logfd < 0)
		perror("log");
	ftruncate(logfd, 0); // 删除旧的日志
	if (dup2(logfd, STDOUT_FILENO) != STDOUT_FILENO) {
		perror("重定向失败");
	}
	close(logfd);
	printf("新的日志: \n");
	// 子进程不再继承终端IO

	int pid = fork();
	if(pid < 0) {
		perror("fork");
		exit(1);  //创建错误，退出
	}
	if(pid > 0) //父进程退出, 子进程成为孤儿继承, 被init进程收养
		exit(0);

	setsid(); //使子进程成为组长, 它不是终端进程直接创建的, 因此得以脱离终端
}

int
startMosh(int session_port, int *port, char *key) {
	int fd[2] = { 0 };
	if (pipe(fd) != EXIT_SUCCESS) {
		perror("pipe");
		return -1;
	}
	char cmd[1024] = { 0 };
	char extra_port[20] = { 0 };
	sprintf(extra_port, "-p %u", session_port);
	sprintf(cmd, "mosh-server new %s 2>& 1 | %s --LOGIN %d %d", session_port == 0 ? "" : extra_port, Argv[0], fd[0], fd[1]);
	printf("cmd -> %s\n", cmd);
	system(cmd);
	close(fd[1]);
	char buf[1024] = { 0 };
	read(fd[0], buf, 1024);
	close(fd[0]);
	printf("[I] buf -> %s\n", buf);
	if (sscanf(buf, "GO! %d %s", port, key) != 2) {
		printf("请检查LOGIN模块\n");
		return -1;
	}
	printf("通过网络提交: %s\n", buf);
	return 0;
}

void
info() {
	printf("Usage:\n");
	printf("moshd <port> <passwd> [-f]\n");
	printf("\t运行moshd在UDP端口port上, -f使其运行在前台, 仅用于调试\n");
	printf("moshd -h\n\t打印本帮助文档\n");
	puts("");
	printf("版本:\tversion 3.0\n");
	printf("\t" __DATE__ " " __TIME__ "\n");
}

int
main(int argc, char *argv[]) {
	if (argc < 2 || strcmp("-h", argv[1]) == 0) {
		info();
		return 0;
	}
	Argv = argv;
GET_INFO:
	if (strcmp("--LOGIN", argv[1]) == 0) {
		int fd0, fd1;
		sscanf(argv[2], "%d", &fd0);
		sscanf(argv[3], "%d", &fd1);
		close(fd0);
		printf("fd0, 1 -> %d %d\n", fd0, fd1);
		char buf[1024] = { 0 };
		while (strlen(buf) < 16) // mosh-server返回空行数量因版本而异, 故修复bug
			fgets(buf, 1024, stdin); //huh
		printf("buf -> %s\n", buf);
		char check[1024] = { 0 };
		sscanf(buf, "%s", check);
		if (strcmp("Failed", check) == 0) {
			printf("[E] 启动失败, 可能是端口被占用\n");
			close(fd1);
			return 0;
		}
		if (strcmp("MOSH", check) != 0) {
			printf("未获取到登录信息 -> %s\n", buf);
			goto GET_INFO;
		}
		int port;
		char key[1024];
		if (sscanf(buf, "%*s %*s %d %s", &port, key) != 2) {
			printf("[E] 启动失败, 未知的错误\n");
			close(fd1);
			return 0;
		}
		char output[1024] = { 0 };
		sprintf(output, "GO! %d %s", port, key);
		printf("output -> %s\n", output);
		printf("[I] write %ld\n", write(fd1, output, strlen(output)) );
		close(fd1);
		return 0;
	}
	printf("欢迎使用Mosh登录器-server, 访问主页: https://github.com/develon2015/Mosh-UDP\n");
	int port = 6666; /* 监听端口 */
	if (sscanf(argv[1], "%d", &port) != 1) {
		printf("使用默认端口 %d\n", port);
	}

	if (argc != 3 && argc != 4) {
		info();
		return 1;
	}
	char passwd[1024] = { 0 }; /* 密码 */
	if (sscanf(argv[2], "%s", passwd) != 1) {
		puts("密码异常");
		return 1;
	}

	int sfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sfd == 0) {
		perror("UDP");
		return 1;
	}
	struct sockaddr_in addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("0.0.0.0"); // 公网IP
	addr.sin_port = htons(port);
	if (bind(sfd, (struct sockaddr *)&addr, (socklen_t)sizeof addr) != EXIT_SUCCESS) {
		perror("bind");
		return 0;
	}

	printf("启动成功!\n");
	// -f 参数将 moshd 置于前台运行, 此参数仅用于调试
	if (!(argc == 4 && strcmp("-f", argv[3]) == 0))
		init_daemon();
	else
		puts("您当前处于调试模式");

L_while:
	{
		char buf[1024 + 8] = { 0 };
		struct sockaddr_in client = { 0 };
		socklen_t len = sizeof client;

		printf("等待客户端连接...\n");
		recvfrom(sfd, buf, 1024, 0, (struct sockaddr *)&client, &len);

		printf("[%s : %d]\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
		printf("客户端: %s\n", buf);

		char buf_login_magic[1024] = { 0 },
		     buf_passwd[1024] = { 0 },
		     buf_check[1024] = { 0 };

		int session_port = 0;

		if (sscanf(buf, "%s %s %d %s", buf_login_magic, buf_passwd, &session_port, buf_check) != 4) {
			const char *RS = "GET OUT!";
			sendto(sfd, RS, strlen(RS) + 1, 0, (void *)&client, len);
			goto L_while;
		}

		if (strcmp("LOGIN", buf_login_magic) != 0 || strcmp("0xArimuraKasumi", buf_check) != 0 || strcmp(passwd, buf_passwd) != 0) {
			const char * RS = "拒绝登录, 可能是您的密码错误";
			sendto(sfd, RS, strlen(RS) + 1, 0, (void *)&client, len);
			goto L_while;
		}

		// 验证成功, 启动moser-server
		switch (fork()) {
			case 0:
				switch (fork()) {
					case 0:
						break;
					case -1:
					default:
						return 0;
				}
				break;
			case -1:
			default:
				wait(NULL);
				goto L_while;
		}

		int mosh_port = 0;
		char mosh_key[1024] = { 0 };
		char mosh_buf[1024] = { 0 };
		if (startMosh(session_port, &mosh_port, mosh_key) != 0) {
			const char *RS = "服务器无法启动mosh-server, 换个端口试试?";
			sendto(sfd, RS, strlen(RS) + 1, 0, (void *)&client, len);
			return 0;
		}
		sprintf(mosh_buf, "GO! %d %s #", mosh_port, mosh_key);
		// 返回"GO! 端口号 KEY #"
		// 重复发包以提高成功率
		for (int i = 0; i < 20; i++) {
			sendto(sfd, mosh_buf, strlen(mosh_buf) + 1, 0, (void *)&client, len);
			sleep(0.2);
		}
	}

	return 0;
}
