# Mosh-UDP
> 使用UDP协议, 无TCP连接, 可快速突破防火墙控制您的Linux计算机 <br/>
> 推荐自行编译, 仓库中已编译为Windows(基于cygwin), Linux, Android(基于termux) <br>
> 安装服务器moshd (Linux x86-64): <br/>
> 	`wget https://github.com/develon2015/Mosh-UDP/blob/master/server?raw=true -O moshd && chmod +x moshd` <br/>
> 安装客户端moshc (Linux x86-64): <br/>
	`wget https://github.com/develon2015/Mosh-UDP/blob/master/client?raw=true -O moshc && chmod +x moshc`  <br/>

# Usage
* 启动moshd, 设置一个用于监听的UDP端口和密码, 它将作为守护程序运行<br/>
* 启动moshc, 在命令行参数中依次提供IP和moshd监听的端口, 然后在终端中输入密码<br/>
* 连接成功<br/>
* 本程序完全是为mosh服务, 假定用户熟悉mosh基本操作, 安装mosh请前往http://mosh.org<br/>
* 2.0船新版本, 客户端moshc已支持域名解析, 自定义mosh-server会话端口<br/>
* 低质网络环境下会有卡死, 失效等正常现象, ^C中断, 然后自行检测服务器上的失效残留mosh-server进程

```
$ moshc
Usage:
        moshc   <IP> <Port> [session-port]
	mosh-udp-server:        version 2.0
	编译时间:               Apr 18 2019 23:04:31
```

```
$ moshd
欢迎使用Mosh登录器-server, 访问主页: https://www.cnblogs.com/develon/p/10279405.html
请选择端口号: a
使用默认端口 6666
请输入密码:
启动成功!
```

# 开机自启动
> echo 端口 密码 | moshd -safe <br/>
> 指定用户: <br/>
> runuser \<user\> -c 'echo "端口 密码" | /.../moshd -safe'
