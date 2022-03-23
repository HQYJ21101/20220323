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

#define IP "192.168.1.58"
#define PORT 9999

//账户　密码
char buf0[10]="",buf1[10]="";
//信息结构体
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

int look_up(int sfd,char *buf,char *buf0,char *rdata);
int add_user(int sfd);
int del_user(int sfd);
int send_recv(int sfd,char *sdata);
int change_user(int sfd,char *buf0,char flag);
int login_crc(int sfd,char *buf0);

int main(int argc, const char *argv[])
{
	if(argc<2)
	{
		fprintf(stderr, "请输入要连接的服务器IP地址\n");
		return -1;
	}

	//套接字
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

	//连接服务器
	if(connect(sfd, (struct sockaddr*)&sin, sizeof(sin)) < 0)
	{
		perror("connect");
		return -1;
	}
	printf("connect success...\n");

	char buf[256]="";
	while(1)
	{
		//登录界面
LOGIN:
		printf("-----------------\n");
		printf("  1登录\t0退出\n");
		printf("-----------------\n");

		char login=getchar();
		while(getchar()!='\n');
		if('0'==login)
		{
			return 0;
		}
		else if('1'==login)
		{
			//发送登录信息
			bzero(buf, sizeof(buf));
			bzero(buf0,sizeof(buf0));
			bzero(buf1,sizeof(buf1));

			printf("请输入账户(xxxx)>>>");
			fgets(buf0, sizeof(buf0), stdin);
			buf0[strlen(buf0)-1] = '\0';
			printf("请输入密码(xxxx)>>>");
			fgets(buf1, sizeof(buf1), stdin);
			buf1[strlen(buf1)-1] = 0;

			sprintf(buf,"2%s%s",buf0,buf1);
			//printf("%s\n",buf);
			if(send(sfd, buf, sizeof(buf), 0) < 0)
			{
				perror("send");
				return -1;
			}
			//接收登录数据          
			char rdata[256]="";
			bzero(rdata,sizeof(rdata));                                                               
			int res=recv(sfd,rdata,sizeof(rdata),0);                                                      
			if(res<0)                                                                               
			{                                                                                       
				perror("recv");                                                                    
				return -1;                                                                              
			}                                                                                       
			else if(0==res)                                                                         
			{                                                                                       
				printf("[%s--%d]=%d 服务端链接关闭\n",inet_ntoa(sin.sin_addr),ntohs(sin.sin_port),sfd); 
				return -1;                                                                              
			}
			printf("%s\n",rdata);

			//判断接收数据　普通用户user　超级用户root
			if('u'==rdata[1])
			{
				//user 界面---------------------------------------
				char flag='u';
				login_crc(sfd,buf0);
UI:
				printf(">1查询\t2修改\t0退出账户\n");
				char userkey=getchar();
				while(getchar()!='\n');
				if('1'==userkey)
				{
					//查询函数
					look_up(sfd,buf,buf0,rdata);
					goto UI;
				}
				else if('2'==userkey)
				{
					//改名字　性别　部门　电话　薪资　地址
					change_user(sfd,buf0,flag);
					goto UI;
				}
				else if('0'==userkey)
				{
					//退出账户
					goto LOGIN;
				}
				else{printf("输入错误\n");}
			}
			else if('r'==rdata[1])
			{
				//root 界面---------------------------------------
				while(1)
				{
					char flag='r';
					//增删改查
					printf("\t5增加\t6删除\t7修改\t8查找\t0退出账户\n");
					char rootkey=getchar();
					while(getchar()!='\n');

					if('5'==rootkey)
					{
						//增加员工
						add_user(sfd);
					}
					else if('6'==rootkey)
					{
						//删除员工
						del_user(sfd);
					}
					else if('7'==rootkey)
					{
						//改名字　性别　部门　电话　薪资　地址
						change_user(sfd,buf0,flag);
					}
					else if('8'==rootkey)
					{
						//查　工号　名字　部门
						look_up(sfd,buf,buf0,rdata);
					}
					else if('0'==rootkey)
					{
						//退出账户
						break;
					}
					else{continue;}
				}
			}
			else
			{
				//登录失败
				goto LOGIN;
			}

		}
		else
		{
			printf("输入错误　重新输入\n");
			goto LOGIN;
		}
	}
	close(sfd);

	return 0;
}

int login_crc(int sfd,char *buf0)
{
	//初次登录修改密码
	if(strcmp(buf1,"0000")==0)
	{
		char newmima[10]="";
		char sdata[256]="";
		printf("初次登录修改密码\n");
		printf("新密码:");
		fgets(newmima,sizeof(newmima),stdin);
		newmima[strlen(newmima)-1]=0;
		sprintf(sdata,"77%s%s",buf0,newmima);
		printf("%s\n",sdata);
		send_recv(sfd,sdata);
	}
	return 0;
}

//修改函数
int change_user(int sfd,char *buf0,char flag)
{
	char idbuf[10]="";
	char chbuf[16]="";
	char option0[64]="";
	char option1[64]="";
	char chdata[256]="";
	bzero(idbuf,sizeof(idbuf));
	//判断使用者权限　普通用户只能修改自己部分信息　超级用户不限制
	if('r'==flag)
	{
		printf(">修改id:");
		fgets(idbuf,sizeof(idbuf),stdin);
		idbuf[strlen(idbuf)-1]=0;
	}
	else
	{
		strcpy(idbuf,buf0);
	}
CHANGEKEY:
	//改名字　性别　部门　电话　薪资　地址
	printf("修改选项\t1)姓名\t2)性别\t3)部门\t4)电话\t5)薪资\t6)地址\t7)密码\t0)返回\n");
	char changekey=getchar();
	while(getchar()!='\n');
	if('u'==flag&&changekey=='5')
	{printf("权限不够\n");goto CHANGEKEY;}

	switch(changekey)
	{
	case '0':return 0;
	case '1':
			 bzero(option0,sizeof(option0));strcpy(option0,"姓名");break;
	case '2':
			 bzero(option0,sizeof(option0));strcpy(option0,"性别(m/w)");break;
	case '3':
			 bzero(option0,sizeof(option0));strcpy(option0,"部门(a/b)");break;
	case '4':
			 bzero(option0,sizeof(option0));strcpy(option0,"电话");break;
	case '5':
			 bzero(option0,sizeof(option0));strcpy(option0,"薪资");break;
	case '6':
			 bzero(option0,sizeof(option0));strcpy(option0,"地址");break;
	case '7':
			 bzero(option0,sizeof(option0));strcpy(option0,"密码");break;
	default:
			 printf("输入错误　重新输入\n");goto CHANGEKEY;

	}
	printf("%s修改为:",option0);
	fgets(chbuf,sizeof(chbuf),stdin);
	chbuf[strlen(chbuf)-1]=0;

//发送修改信息
	bzero(chdata,sizeof(chdata));
	sprintf(chdata,"7%c%s%s",changekey,idbuf,chbuf);
	printf("发送%s\n",chdata);
	send_recv(sfd,chdata);
}
//发送接收函数
int send_recv(int sfd,char *sdata)
{
	char sbuf[256]="";
	strcpy(sbuf,sdata);
	if(send(sfd,sbuf, sizeof(sbuf), 0) < 0)  
	{
		perror("send");
		return -1;
	}
	printf("%s\n",sbuf);
	char rdata[256]="";
	int res=recv(sfd,rdata,sizeof(rdata),0);                                                      
	if(res<0)                                                                               
	{                                                                                       
		perror("recv");                                                                    
		return -1;                                                                              
	}                                                                                       
	else if(0==res)                                                                         
	{                                                                                       
		//printf("[%s--%d]=%d 服务端链接关闭\n",inet_ntoa(cin.sin_addr),ntohs(cin.sin_port),sfd); 
		return -1; 
	}
	printf("接收%s\n",rdata);

}

//超级用户删除函数
int del_user(int sfd)
{
	char sdata[256]="";
	char buf00[10]="";
	printf("删除id:");
	fgets(buf00,sizeof(buf00),stdin);
	buf00[strlen(buf00)-1]=0;
	bzero(sdata,sizeof(sdata));
	sprintf(sdata,"6%s",buf00);
	if(send(sfd,sdata, sizeof(sdata), 0) < 0)  
	{
		perror("send");
		return -1;
	}

	//删除账户密码　个人信息　接收两个返回数据
	int i=2;
	while(i--)
	{
		bzero(sdata,sizeof(sdata));
		int res=recv(sfd,sdata,sizeof(sdata),0);                                                      
		if(res<0)                                                                               
		{                                                                                       
			perror("recv");                                                                    
			return -1;                                                                              
		}                                                                                       
		else if(0==res)                                                                         
		{                                                                                       
			//printf("[%s--%d]=%d 服务端链接关闭\n",inet_ntoa(cin.sin_addr),ntohs(cin.sin_port),sfd); 
			return -1;                                                                              
		}
		printf("%s\n",sdata);
	}
}

//超级用户增加员工　
int add_user(int sfd)
{
	struct personage useradd;
	char dataadd[256]="";
	printf("工号\t姓名\t性别\t部门\t电话\t薪资\t地址\n");
	scanf("%s %s %s %s %s %s %s",useradd.id,useradd.name,useradd.sex,\
			useradd.department,useradd.phone,useradd.salary,useradd.address);
	while(getchar()!='\n');
	//printf("%s",useradd.address);
	
	sprintf(dataadd,"5useradd");
	if(send(sfd,dataadd, sizeof(dataadd), 0) < 0)  
	{
		perror("send");
		return -1;
	}
	if(send(sfd,&useradd, sizeof(useradd), 0) < 0)  
	{
		perror("send");
		return -1;
	}
	int i=2,res=0;
	while(i--)
	{
		bzero(dataadd,sizeof(dataadd));
		res=recv(sfd,dataadd,sizeof(dataadd),0);                                                      
		if(res<0)                                                                               
		{                                                                                       
			perror("recv");                                                                    
			return -1;                                                                              
		}                                                                                       
		else if(0==res)                                                                         
		{                                                                                       
			//printf("[%s--%d]=%d 服务端链接关闭\n",inet_ntoa(cin.sin_addr),ntohs(cin.sin_port),sfd); 
			return -1;                                                                              
		}
		printf("%s\n",dataadd);
	}
}

//用户查找
int look_up(int sfd,char *buf,char *buf0,char *rdata)
{

INQUIREKEY:
	//查询
	printf(">>1个人信息\t2姓名\t3部门\t0返回\n");
	char inquirekey=getchar();
	while(getchar()!='\n');
	if(inquirekey=='0')
		return 0;
	else if(inquirekey=='1'||inquirekey=='2'||inquirekey=='3')
	{
		char name[16]="";

		bzero(buf,sizeof(buf));
		printf("查询:");
		fgets(name,sizeof(name),stdin);
		name[strlen(name)-1]=0;
		sprintf(buf,"3%c%s",inquirekey,name);
		//发送注册数据                            
		int res=send(sfd,buf,sizeof(buf),0);    
		if(res==-1)                           
		{                                     
			perror("send");                  
			return -1;                            
		}                                     
		//	printf(">  %s\n",buf);

		struct personage colleague;
		//接收数据                                                                              
		do{
			res=recv(sfd,&colleague,sizeof(colleague),0);                                                      
			if(res<0)                                                                               
			{                                                                                       
				perror("recv");                                                                    
				return -1;                                                                              
			}                                                                                       
			else if(0==res)                                                                         
			{                                                                                       
				//printf("[%s--%d]=%d 服务端链接关闭\n",inet_ntoa(cin.sin_addr),ntohs(cin.sin_port),sfd); 
				return -1;                                                                              
			}

			//权限设置　超级用户和自己查看全部个人信息　普通用户查看他人部分数据
			if(('r'==rdata[1]) || ((strcmp(colleague.id,buf0)==0)&&('1'==inquirekey)))
			{
				printf(">  %s %s %s %s %s %s %s\n",colleague.id,colleague.name, \
						colleague.sex,colleague.department,colleague.phone,colleague.salary,colleague.address);
			}	
			else
			{
				printf(">  %s %s %s %s %s\n",colleague.id,colleague.name, \
						colleague.sex,colleague.department,colleague.phone);
			}
		}while(strcmp(colleague.id,"end")!=0);
		printf("-------------------------\n");
		goto INQUIREKEY;
	}
	else{
		printf("输入错误　重新输入\n");
		goto INQUIREKEY;
	}
}
