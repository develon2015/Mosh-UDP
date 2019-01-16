/*####################################
# Mosh server
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

char **Argv = NULL;

void 
init_daemon() {
	int pid = fork();
	if(pid < 0) {
		perror("fock");
		exit(1);  //创建错误，退出
	}
	if(pid > 0) //父进程退出
		exit(0);

	setsid(); //使子进程成为组长

	close(STDOUT_FILENO);
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
	printf("mosh-udp-server:\tversion 2.0\n");
	printf("编译时间:\t\t" __DATE__ " " __TIME__ "\n");
}

int
main(int argc, char *argv[]) {
	if (argc == 2 && strcmp("-h", argv[1]) == 0) {
		info();
		return 0;
	}
	Argv = argv;
	if (argc != 1) {
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
			int port;
			char key[1024];
			if (sscanf(buf, "%*s %*s %d %s", &port, key) != 2) {
				// 启动失败, 可能是端口被占用
				printf("[E] 启动失败, 可能是端口被占用\n");
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
	}
	printf("欢迎使用Mosh登录器-server, 访问主页: https://www.cnblogs.com/develon/p/10279405.html\n");
	printf("请选择端口号: ");
	setbuf(stdout, NULL);
	int port = 6666;
	if (scanf("%d", &port) != 1) {
		printf("使用默认端口 %d\n", port);
	}

//******输入密码
	struct termios save, current;
	tcgetattr(0, &save);// 得到原来的终端属性
	current = save;
	current.c_lflag &= ~ECHO;// 关闭回显
	tcsetattr(0, TCSANOW, &current);// 设置新的终端属性

        // 为了适应开机启动, 可以选择性关闭这条代码
	// setbuf(stdin, NULL);
	// 在/etc/rc.local中添加以下代码, 以便开机自启, 有时你需要使用runuser选择非root用户来启动本程序
        // echo "6666passwd" | moshd -safe
        if (!(argc == 2 && strcmp("-safe", argv[1]) == 0))
		setbuf(stdin, NULL);
	printf("请输入密码: ");
	setbuf(stdout, NULL);
	char passwd[1024] = { 0 };
	scanf("%s", passwd);

	tcsetattr(0, TCSANOW, &save);// 恢复原来的终端属性
	puts("");
//******

	int sfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sfd == 0) {
		perror("UDP");
		return 0;
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
	if (!(argc > 1 && strcmp("-f", argv[1]) == 0))
		init_daemon();
	else
		puts("您当前处于调式模式");

	while (1) {
		char buf[1024 + 8] = { 0 };
		struct sockaddr_in client = { 0 };
		socklen_t len = sizeof client;
		printf("等待客户端连接...\n");
		recvfrom(sfd, buf, 1024, 0, (struct sockaddr *)&client, &len);
		printf("[%s : %d]\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
		char buf_login_magic[1024] = { 0 },
			buf_passwd[1024] = { 0 },
			buf_check[1024] = { 0 };
		int session_port = 0;
		if (sscanf(buf, "%s %s %d %s", buf_login_magic, buf_passwd, &session_port, buf_check) != 4) {
GETOUT:
			sendto(sfd, "GET OUT!", 9, 0, (void *)&client, len);
			continue;
		}
		if (strcmp("LOGIN", buf_login_magic) != 0 || strcmp("0xArimuraKasumi", buf_check) != 0 || strcmp(passwd, buf_passwd) != 0)
			goto GETOUT;
		
		int mosh_port = 0;
		char mosh_key[1024] = { 0 };
		char mosh_buf[1024] = { 0 };
		if (startMosh(session_port, &mosh_port, mosh_key) != 0) {
			printf("start mosh failed\n");
			goto GETOUT;
		}
		sprintf(mosh_buf, "GO! %d %s #", mosh_port, mosh_key);
		// 返回"GO! 端口号 KEY #"
		sendto(sfd, mosh_buf, strlen(mosh_buf)+1, 0, (void *)&client, len);
	}
	
	return 0;
}
