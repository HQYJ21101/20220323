#include<stdio.h>
#include <sqlite3.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
int main(int argc, const char *argv[])
{
	sqlite3 *ppDb=NULL;
	if(sqlite3_open("./my_staff.db",&ppDb)<0)
	{
		printf("%s\n",sqlite3_errmsg(ppDb));
		return -1;
	}
	printf("sqlite3_open\n");

	//创建员工表
	char buf[128]="create table if not exists staff (id char primary key,mima char,power char)";
	char *errmsg=NULL;
	if(sqlite3_exec(ppDb,buf,NULL,NULL,&errmsg)!=0)
	{
		printf("%s\n",errmsg);
		return -1;
	}
	printf("创建员工信息表成功\n");

	//创建员工信息表
	char stafflist[256]="create table if not exists information (id char primary key,name char,sex char,department char,phone char,salary char,address char)";
	if(sqlite3_exec(ppDb,stafflist,NULL,NULL,&errmsg)!=0)
	{
		printf("%s\n",errmsg);
		return -1;
	}
	printf("创建个人信息表成功\n");

	char new0[10],new1[10];
	printf("输入root用户　密码:(例:boss 1234)\n");
	fscanf(stdin,"%s %s",new0,new1);
	char new[128];
	sprintf(new,"insert into staff values (\"%s\",\"%s\",\"root\")",new0,new1);
	if(sqlite3_exec(ppDb,new,NULL,NULL,&errmsg)!=0)
	{
		printf("%s\n",errmsg);
		return -1;
	}
	printf("创建root user成功\n");
	sqlite3_close(ppDb);
	return 0;
}
