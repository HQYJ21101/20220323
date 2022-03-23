#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sqlite3.h>

#define IP "192.168.1.58"
#define PORT 9999

struct personage
{
	char id[16];
	char name[16];
	char sex[2];
	char department[16];
	char phone[16];
	char salary[16];
	char address[16];
};
int fun_all(int newfd,sqlite3 *ppDb);
int log_in(int newfd,sqlite3 *ppDb,char *data);
int look_up(int newfd,sqlite3 *ppDb,char *buf);
int add_user_root(int newfd,sqlite3 *ppDb);
int my_sqlite3_exec(int newfd,sqlite3 *ppDb,char *buff);
int del_user_root(int newfd,sqlite3 *ppDb,char *buf);
int change_user_root(int newfd,sqlite3 *ppDb,char *buf);
void hander(int sig)
{
	while(waitpid(-1,NULL,WNOHANG)>0);
	return;
}


int main(int argc, const char *argv[])
{
	if(argc<2)
	{
		fprintf(stderr, "请输入服务器IP地址\n");
		return -1;
	}
	//打开库
	sqlite3 *ppDb=NULL;
	if(sqlite3_open("./my_staff.db",&ppDb)<0)
	{
		printf("%s\n",sqlite3_errmsg(ppDb));
		return -1;
	}
	printf("sqlite3_open\n");

	int sfd;
	sfd=socket(AF_INET,SOCK_STREAM,0);
	if(sfd<0)
	{
		perror("sfd");
		return -1;
	}

	//填充地址信息结构体 , man 7 ip
	struct sockaddr_in sin;
	sin.sin_family  	= AF_INET; 			//地址族，固定填AF_INET
	sin.sin_port 		= htons(PORT); 		//端口号的网络字节序
	sin.sin_addr.s_addr = inet_addr(argv[1]); 	//IP的网络字节序;
	//允许端口快速重用
	int reuse = 1; 	//允许
	if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
	{
		perror("setsockopt");
		return -1;
	}
	__sighandler_t s=signal(17,hander);
	if(s==SIG_ERR)
	{
		perror("signal");
		return -1;
	}


	if(bind(sfd,(struct sockaddr*) &sin,sizeof(sin))<0)
	{
		perror("bind");
		return -1;
	}
	printf("bind 111\n");
	if(listen(sfd,10)<0)
	{
		perror("listen");
		return -1;
	}
	printf("listen 111\n");

	struct sockaddr_in cin;
	int addrlen=sizeof(cin);
	while(1)
	{
		int newfd=accept(sfd,(struct sockaddr *)&cin,&addrlen);
		if(newfd<0)
		{
			perror("accept");
			return -1;
		}
		printf("[%s | %d] newfd=%d\n", inet_ntoa(cin.sin_addr), ntohs(cin.sin_port), newfd);

		if(fork()==0)
		{
			close(sfd);
			int a=0;
			while(a!=(-1))
			{
				printf("<--------------------------->\n");
				a=fun_all(newfd,ppDb);

			}
			exit(0);
		}
		close(newfd);
	}
	close(sfd);

	return 0;
}


int fun_all(int newfd0,sqlite3 *ppDb)
{
	int newfd=newfd0;
	char buf[256]="";
	ssize_t res = 0;
	//接收
	bzero(buf, sizeof(buf));
	res = recv(newfd, buf, sizeof(buf), 0);
	if(res < 0)
	{
		perror("recv");
		return -1;
	}
	else if(0 == res)
	{
		printf("kehu关闭\n");
		return -1;
	}


	//数据判断
	//２登录　３普通用户查看　４普通用户修改
	printf("%ld %ld:%s\n",strlen(buf), sizeof(buf),buf);
	if('2'==buf[0])
	{
		//登录
		log_in(newfd,ppDb,buf);
	}
	else if('3'==buf[0]||'8'==buf[0])
	{
		//普通用户查看
		look_up(newfd,ppDb,buf);

	}
	else if('4'==buf[0])
	{
		//修改函数
		change_user_root(newfd,ppDb,buf);	

	}
	else if('5'==buf[0])
	{
		//增加普通用户　由超级用户执行　默认密码００００
		add_user_root(newfd,ppDb);

	}
	else if('6'==buf[0])
	{
		//删除函数
		del_user_root(newfd,ppDb,buf);
	}
	else if('7'==buf[0])
	{
		//修改函数
		change_user_root(newfd,ppDb,buf);	
	}
	else
	{return -1;}


	printf("=============================\n\n");
	return 0;
	//	printf("::%s\n",buf);
}

//修改函数
int change_user_root(int newfd,sqlite3 *ppDb,char *buf)
{
	char chdata[256]="";
	char iddatd[10]="";
	char option0[32]="";
	bzero(chdata,sizeof(chdata));
	bzero(iddatd,sizeof(iddatd));
	strncpy(iddatd,buf+2,4);
	//判断要修改的信息
	switch(buf[1])                                                      
	{
	case '1':
		bzero(option0,sizeof(option0));strcpy(option0,"name");break;
	case '2':
		bzero(option0,sizeof(option0));strcpy(option0,"sex");break;
	case '3':
		bzero(option0,sizeof(option0));strcpy(option0,"department");break;
	case '4':
		bzero(option0,sizeof(option0));strcpy(option0,"phone");break;
	case '5':
		bzero(option0,sizeof(option0));strcpy(option0,"salary");break;
	case '6':                                                          
		bzero(option0,sizeof(option0));strcpy(option0,"address");break;
	case '7':
		bzero(option0,sizeof(option0));strcpy(option0,"mima");break;
	default:;
	}

	//修改密码和个人信息表名不一样
	printf("%s\n",buf+6);
	if(buf[1]=='7')
		sprintf(chdata,"update staff set %s='%s' where id='%s'",option0,buf+6,iddatd);
	else
		sprintf(chdata,"update information set %s='%s' where id='%s'",option0,buf+6,iddatd);

	//操作库
	printf("%s\n",chdata);
	if(my_sqlite3_exec(newfd,ppDb,chdata)==-1)
		return -1;
	return 0;

}

//删除函数 删除账户　个人信息
int del_user_root(int newfd,sqlite3 *ppDb,char *buf)
{
	char delid0[256]="";
	char delid1[256]="";
	sprintf(delid0,"delete from information where id='%s';",buf+1);
	sprintf(delid1,"delete from staff where id='%s';",buf+1);
	my_sqlite3_exec(newfd,ppDb,delid0);
	my_sqlite3_exec(newfd,ppDb,delid1);
	return 1;
}

//普通用户增加　默认密码0000
int add_user_root(int newfd,sqlite3 *ppDb)
{
	//接收结构体
	struct personage adduser;
	int res = recv(newfd, &adduser, sizeof(adduser), 0);
	if(res < 0)
	{
		perror("recv");
		return -1;
	}
	else if(0 == res)
	{
		printf("kehu关闭\n");
		return -1;
	}
	char buff[256]="";
	//插入账户密码
	sprintf(buff,"insert into staff values('%s','0000','user')",adduser.id);
	my_sqlite3_exec(newfd,ppDb,buff);
	//插入个人信息
	bzero(buff,sizeof(buff));
	sprintf(buff,"insert into information values('%s','%s','%s','%s','%s','%s','%s');",\
			adduser.id,adduser.name,adduser.sex,adduser.department,adduser.phone,\
			adduser.salary,adduser.address);
	my_sqlite3_exec(newfd,ppDb,buff);
}

//库操作函数
int my_sqlite3_exec(int newfd,sqlite3 *ppDb,char *buff)
{
	char sdata[256]="";	
	char *errmsg=NULL;
	if(sqlite3_exec(ppDb,buff,NULL,NULL,&errmsg)!=0)
	{
		printf("%s\n",errmsg);
		bzero(sdata,sizeof(sdata));
		sprintf(sdata,"nosuccess-%s",errmsg);
		if(send(newfd,sdata, sizeof(sdata), 0) < 0)
		{
			perror("send");
			return -1;
		}
		return -1;
	}
	printf("adduser成功\n");
	bzero(sdata,sizeof(sdata));
	sprintf(sdata,"success");
	if(send(newfd,sdata, sizeof(sdata), 0) < 0)
	{
		perror("send");
		return -1;
	}
	return 1;
}

//查找函数
int look_up(int newfd,sqlite3 *ppDb,char *buf)
{
	printf("look_up\n");
	char buf0[10]="";
	strcpy(buf0,buf+2);
	//判断查找　工号a=0　姓名1　部门3
	int a=0;
	if(buf[1]=='2')
		a=1;
	else if(buf[1]=='3')
		a=3;
	else
		a=0;

	char *errmsg=NULL;
	char* buff="select * from information";
	char **pres=NULL;
	int hang,lie;

	if(sqlite3_get_table(ppDb,buff,&pres,&hang,&lie,&errmsg)!=0)
	{
		printf("sqlite3_get_table   -1");
		return -1;
	}
	int i,flag=0;
	struct personage myself;
	for(i=a;i<(hang+1)*lie;i=i+lie)
	{                                            
		if(strcmp(buf0,pres[i])==0)
		{
			//拷贝查询到的信息
			strcpy(myself.id,pres[i-a]);
			strcpy(myself.name,pres[i-a+1]);
			strcpy(myself.sex,pres[i-a+2]);
			strcpy(myself.department,pres[i-a+3]);
			strcpy(myself.phone,pres[i-a+4]);
			strcpy(myself.salary,pres[i-a+5]);
			strcpy(myself.address,pres[i-a+6]);
			flag=1;
			if(send(newfd,&myself, sizeof(myself), 0) < 0)
			{
				perror("send");
				return -1;
			}
		}

		if(a==0&&flag==1)
			break;
	}
	//发送完成的结构体信息
	struct personage crc={"end"};
	if(send(newfd,&crc, sizeof(crc), 0) < 0)
	{
		perror("send");
		return -1;
	}
	//释放获取到的结果
	sqlite3_free_table(pres);
	printf("--------look up---------\n");
	return flag;

}

//登录函数
int log_in(int newfd,sqlite3 *ppDb,char *data)
{
	char buf0[10],buf1[10];
	//确认用户账户和密码
	//接收账户和密码
	bzero(buf0,sizeof(buf0));
	bzero(buf1,sizeof(buf1));
	for(int i=1;i<=4;i++)
		buf0[i-1]=data[i];
	for(int i=5;i<=8;i++)
		buf1[i-5]=data[i];
	//查找用户库账户密码 
	char *errmsg=NULL;                     
	char* buff="select * from staff";
	char **pres=NULL;
	int hang,lie;
	char sdata[256]="";
	bzero(sdata,sizeof(sdata));

	if(sqlite3_get_table(ppDb,buff,&pres,&hang,&lie,&errmsg)!=0)
	{
		printf("sqlite3_get_table   -1");
		return -1;
	}
	int i,flag=0;
	for(i=0;i<(hang+1)*lie;i=i+lie)
	{
		if(strcmp(buf0,pres[i])==0)
		{
			if(strcmp(buf1,pres[i+1])==0)
			{
				sprintf(sdata,"2%s success!",pres[i+2]);
				flag=1;
			}
		}
	}
	if(0==flag)
	{
		sprintf(sdata,"2no success!");
	}
	//发送登录的返回信息
	if(send(newfd,sdata, sizeof(sdata), 0) < 0)
	{
		perror("send");
		return -1;
	}
	printf("登录%s\n",sdata);
	//释放获取到的结果
	sqlite3_free_table(pres);
	return flag;

}
