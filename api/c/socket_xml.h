/*****************************************************************************
 * Copyright: 
 * File name: 
 * Description: 
 * Author: dandan
 * Version: 0.1
 * Date: 
 * History: 
 * *****************************************************************************/

#ifndef LIBXML2_PARSE
#define LIBXML2_PARSE
#include <syslog.h>
#include <stdio.h>
#include <libxml/parser.h> 
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <string.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

/* macro define*/
#define SOCKET_BUF_SIZE 1024
#define XML_BUF_SIZE 40
#define SERV_PORT 3333
/* global var*/
//extern OB_SOCKET obSocket;
/*function define*/

struct RAISE{
	char src[50];
	int winid;

};
typedef enum {
	OB_START_APP,
	OB_KILL_APP,
	OB_EXIT_APP,
	OB_SET_FULLSCREEN,
	OB_SET_MAX,
	OB_SET_MIN,
	OB_SET_BOTTOM,
	OB_SET_TOP,
	OB_SET_NORMAL,
	OB_GET_APPS_LIST,
	OB_GET_APP_STATE,
	OB_EXIT,
	OB_RESTART,
	OB_REFRESH,
	OB_RESIZE,
	OB_MOVE,
	OB_RESIZE_MOVE
}WM_METHOD;
typedef struct{
	int winid;
	int pid;
	char title[XML_BUF_SIZE];
	char cmd[XML_BUF_SIZE];
	char name[XML_BUF_SIZE];
}APP_INFO;
typedef struct {
	int x;
	int y;
	int width;
	int height;
	int extend;
	int desktop;
}APP_DATA;
typedef struct {
	//here may add sender socket msg
	WM_METHOD method;
	char src[XML_BUF_SIZE];
	int id;
	APP_INFO appInfo;
	APP_DATA appData;
	struct sockaddr_in addr;
	socklen_t len; 
}OB_SOCKET;








/*************************************************
 * Function: // 函数名称
 * Description: // 函数功能、性能等的描述
 * Calls: // 被本函数调用的函数清单
 * Called By: // 调用本函数的函数清单
 * Table Accessed: // 被访问的表（此项仅对于牵扯到数据库操作的程序）
 * Table Updated: // 被修改的表（此项仅对于牵扯到数据库操作的程序）
 * Input: // 输入参数说明，包括每个参数的作// 用、取值说明及参数间关系。
 * Output: // 对输出参数的说明。
 * Return: // 函数返回值的说明
 * Others: // 其它说明
 * *************************************************/
int parse_xml_from_buf(char *buf,int len,OB_SOCKET *ob); 
int  start_socket_server(int port);
int exec_socket_cmd(OB_SOCKET *ob,char **ack,int *ack_len,int ackSize);
int socket_xml_exec(void);
int  add_data_to_xml_node(xmlNode *node,char *key,char *value);
int main_test(int argc, char **argv);
#endif
