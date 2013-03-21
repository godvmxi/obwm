/*****************************************************************************
 * Copyright: 
 * File name: 
 * Description: 
 * Author: dandan
 * Version: 0.1
 * Date: 
 * History: 
 * *****************************************************************************/


#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h> 
#include "socket_xml.h"                   

/* global var*/
int sockfd = 0;
OB_SOCKET obSocket ;
int main_init(int argc, char **argv);
int main(int argc, char **argv){
	printf("system->%d\n",system("ls"));
	printf("system->%d\n",system("a"));
	printf("system->%d\n",system("./a.out"));
	printf("system->%d\n",system("app"));
//	start_socket_server(3333);
//	socket_xml_exec();
	//main_init(argc,argv);
}
int main_init(int argc, char **argv)
{
       xmlDocPtr doc = NULL,doc2=NULL;       /* document pointer */
	xmlChar *buf;
	int recvBufSize,sendBufSize;
       xmlNodePtr root_node = NULL, node1 = NULL, node2 = NULL,node3 = NULL; /* node pointers */                                                      
       //Creates a new document, a node and set it as a root node
       doc = xmlNewDoc(BAD_CAST "1.0");
       root_node = xmlNewNode(NULL, BAD_CAST "root");
       xmlDocSetRootElement(doc, root_node);    
       //creates a new node, which is "attached" as child node of root_node node. 
       // xmlNewProp() creates attributes, which is "attached" to an node.
       node1=xmlNewChild(root_node, NULL, BAD_CAST "method", BAD_CAST"raise");
       node1 =xmlNewChild(root_node, NULL, BAD_CAST "src",BAD_CAST "remote_dev");
       node1 =xmlNewChild(root_node, NULL, BAD_CAST "id",BAD_CAST "0");
       //xmlNewProp(node1, BAD_CAST "attribute", BAD_CAST "yes"); 
	node1=xmlNewChild(root_node, NULL, BAD_CAST "app",NULL);
	node2 = xmlNewChild(node1,NULL,BAD_CAST "winid",BAD_CAST "111");	
	node2 = xmlNewChild(node1,NULL,BAD_CAST "pid",BAD_CAST "222");	
	node2 = xmlNewChild(node1,NULL,BAD_CAST "title",BAD_CAST "333");	
	node2 = xmlNewChild(node1,NULL,BAD_CAST "cmd",BAD_CAST "444");	
	node2 = xmlNewChild(node1,NULL,BAD_CAST "name",BAD_CAST "555");	
	node1 = xmlNewChild(root_node,NULL,BAD_CAST "data",NULL);
	node2 = xmlNewChild(node1,NULL,BAD_CAST "x",BAD_CAST "100");	
	node2 = xmlNewChild(node1,NULL,BAD_CAST "y",BAD_CAST "100");	
	node2 = xmlNewChild(node1,NULL,BAD_CAST "width",BAD_CAST "200");	
	node2 = xmlNewChild(node1,NULL,BAD_CAST "height",BAD_CAST "200");	
	node2 = xmlNewChild(node1,NULL,BAD_CAST "extend",BAD_CAST "0");	

       xmlSaveFormatFileEnc(argc > 1 ? argv[1] : "-", doc, "UTF-8", 1); 
	xmlDocDumpFormatMemory(doc,&buf,&recvBufSize,1);
	printf("char ->%d\n%s",recvBufSize,buf);
       xmlFreeDoc(doc);
	//read from memory
	//doc2 = xmlReadMemory(buf,bufferSize,NULL,NULL,0);
	int type = 0;
		
	start_socket_server(3333);
	socket_xml_exec();
	//printf("read from char *-->\n");
	//xmlSaveFormatFileEnc(argc > 1 ? argv[1] : "-", doc2, "UTF-8", 1); 
       xmlCleanupParser();
       xmlMemoryDump();      //debug memory for regression tests

       return(0);
}
void *get_client_from_app_info(OB_SOCKET *ob)
{
        return NULL;
}
int ob_start_app(OB_SOCKET *ob){
        return system(ob->appInfo.cmd);
}
int ob_kill_app(OB_SOCKET *ob){
        return 1;
}
int ob_exit_app(OB_SOCKET *ob){
        return 1;
}
int ob_set_full_app(OB_SOCKET *ob){
        return 1;
}
int ob_set_max_app(OB_SOCKET *ob){
        return 1;
}
int ob_set_min_app(OB_SOCKET *ob){
        return 1;
}
int ob_set_layer_app(OB_SOCKET *ob){
        return 1;
}
int ob_get_list_app(OB_SOCKET *ob){
        return 1;
}
int ob_get_state_app(OB_SOCKET *ob){
        return 1;
}
int ob_exit(OB_SOCKET *ob){
        return 1;
}
int ob_restart(OB_SOCKET *ob){
        return 1;
}
int ob_refresh(OB_SOCKET *ob){
        return 1;
}
int ob_resize(OB_SOCKET *ob){
        return 1;
}
int ob_move(OB_SOCKET *ob){
        return 1;
}
int ob_resize_move(OB_SOCKET *ob){
        return 1;
}
void trint_element_names(xmlNode * a_node)
{
    xmlNode *cur_node = NULL;
	return ;
}
/*

    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            printf("node->  %d\t%s\t-->%s\n",cur_node->type, cur_node->name,xmlNodeGetContent(cur_node));
        }
        print_element_names(cur_node->children);
    }
}
*/
static xmlXPathObjectPtr
getXPathObjectPtr(xmlDocPtr doc, xmlChar* xpath_exp) {
        xmlXPathObjectPtr result;
        xmlXPathContextPtr context;

        context = xmlXPathNewContext(doc);
        result = xmlXPathEvalExpression((const xmlChar*)xpath_exp, context);
        xmlXPathFreeContext(context);

        if(NULL == result) {
                fprintf(stderr, "eval expression error!\n");
                return NULL;
        }   

        if(xmlXPathNodeSetIsEmpty(result->nodesetval)) {
                fprintf(stderr, "empty node set!\n");
                xmlXPathFreeObject(result);
                return NULL;
        }   
        return result;
}
int query_value_from_xpath(xmlDoc *doc,char *xpath,char **buf,int bufferSize)
{
	memset((void *)(*buf),0,bufferSize);
//	printf("query value");
	xmlXPathObjectPtr xpath_obj = NULL; 
	xmlNodeSetPtr nodeset = NULL;
	xpath_obj = getXPathObjectPtr(doc,BAD_CAST xpath);
     	if(NULL != xpath_obj) {
        	nodeset = xpath_obj->nodesetval;
                int i = 0;
                for(i = 0; i < nodeset->nodeNr; i ++) {
        //                uri = xmlGetProp(nodeset->nodeTab[i],(const xmlChar*)"href");
                        //sprintf(*buf,"link address:%s->%s->%s\n",uri,nodeset->nodeTab[i]->name,
                        //sprintf(*buf,"%s%s",*buf,xmlNodeGetContent(nodeset->nodeTab[i]));
			strcat(*buf,(const char *)xmlNodeGetContent(nodeset->nodeTab[i]));
                        //xmlFree(uri);
                }   
                xmlXPathFreeObject(xpath_obj);
		return 1;
        }   
	else
		return -1;
}
int parse_xml_from_buf(char *buf,int len,OB_SOCKET *ob){
	char fuck[XML_BUF_SIZE];
	char *tmp = (char *)fuck;
	
//	printf("parese xml from buf ->%d->%s",len,buf);
	memset((void *)ob,0,sizeof(OB_SOCKET));
	xmlDoc  *doc = xmlReadMemory((xmlChar *)buf,len,NULL,NULL,0);
	if(doc == NULL){
		printf("buf can't conver to xml\n");
		return -1;
	}
	printf("app comtrol data +++++++++++++++++++++\n");
	query_value_from_xpath(doc,"//method",&tmp,XML_BUF_SIZE);
	printf("xpath method -> %s\n",tmp);
	ob->method = atoi(tmp);
	query_value_from_xpath(doc,"//src",&tmp,XML_BUF_SIZE);
	printf("xpath src    -> %s\n",tmp);
	strcat(ob->src,tmp); 
	query_value_from_xpath(doc,"//id",&tmp,XML_BUF_SIZE);
	printf("xpath id     -> %s\n",tmp);
	ob->id = atoi(tmp);
	query_value_from_xpath(doc,"//winid",&tmp,XML_BUF_SIZE);
	printf("app basic data +++++++++++++++++++++\n");
	printf("xpath winid  -> %s\n",tmp);
	ob->appInfo.winid = atoi(tmp);
	query_value_from_xpath(doc,"//pid",&tmp,XML_BUF_SIZE);
	printf("xpath pid    -> %s\n",tmp);
	ob->appInfo.pid = atoi(tmp);
	query_value_from_xpath(doc,"//title",&tmp,XML_BUF_SIZE);
	printf("xpath title  -> %s\n",tmp);
	strcat(ob->appInfo.title,tmp);
	query_value_from_xpath(doc,"//cmd",&tmp,XML_BUF_SIZE);
	printf("xpath cmd    -> %s\n",tmp);
	ob->appInfo.pid = atoi(tmp);
	query_value_from_xpath(doc,"//name",&tmp,XML_BUF_SIZE);
	printf("xpath name   -> %s\n",tmp);
	ob->appInfo.pid = atoi(tmp);
	printf("app extra data +++++++++++++++++++++\n");
	query_value_from_xpath(doc,"//x",&tmp,XML_BUF_SIZE);
	printf("xpath x      -> %s\n",tmp);
	ob->appData.x = atoi(tmp);
	query_value_from_xpath(doc,"//y",&tmp,XML_BUF_SIZE);
	printf("xpath y      -> %s\n",tmp);
	ob->appData.y = atoi(tmp);
	query_value_from_xpath(doc,"//width",&tmp,XML_BUF_SIZE);
	printf("xpath width  -> %s\n",tmp);
	ob->appData.width = atoi(tmp);
	query_value_from_xpath(doc,"//height",&tmp,XML_BUF_SIZE);
	printf("xpath height -> %s\n",tmp);
	ob->appData.height = atoi(tmp);
	query_value_from_xpath(doc,"//extend",&tmp,XML_BUF_SIZE);
	printf("xpath extend -> %s\n",tmp);
	ob->appData.extend = atoi(tmp);
	return 1;
}
int exec_socket_cmd(OB_SOCKET *ob,char **ack,int *ack_len,int ackBufSize)
{
	xmlNodePtr dataNode  = xmlNewNode(NULL,BAD_CAST"data");
	int state = 0;
	switch(ob->method)
	{
		case OB_START_APP :
			//start app
			break;
		case OB_KILL_APP :
			//kill app
			break;
		case OB_EXIT_APP :
			//exit
			break;
		case OB_SET_FULLSCREEN :
			//
			break;
		case OB_SET_MAX :
			//
			break;
		case OB_SET_MIN :
			//
			break;
		case OB_SET_BOTTOM :
			//
			break;
		case OB_SET_TOP :
			break;
		case OB_SET_NORMAL :
			break;
		case OB_GET_APPS_LIST :
			break;
		case OB_GET_APP_STATE :
			break;
		case OB_EXIT :
			break;
		case OB_RESTART :
			break;
		case OB_REFRESH :
			break;
		case OB_RESIZE :
			break;
		case OB_MOVE :
			break;
		case OB_RESIZE_MOVE :
			break;
		default :
			break;
	}
	char tmp[20];
	xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
       	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST "root");
       	xmlDocSetRootElement(doc, root_node); 
	memset(tmp,0,20);
	sprintf(tmp,"%d",ob->id);
	xmlNewChild(root_node, NULL, BAD_CAST "id",BAD_CAST tmp); 
	memset(tmp,0,20);
	sprintf(tmp,"%d",ob->id);
	xmlNewChild(root_node, NULL, BAD_CAST "state",BAD_CAST tmp); 
	xmlAddChild(root_node,dataNode); 
	
	xmlDocDumpFormatMemory(doc,(xmlChar **)ack,ack_len,1);
	
}
int  start_socket_server(int port)
{
        socklen_t len;
        socklen_t src_len;
        struct sockaddr_in servaddr, cliaddr;
        sockfd = socket(AF_INET, SOCK_DGRAM, 0); /* create a socket */

        /* init s//ervaddr */
        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(port);

        /* bind address and port to socket */
        if(bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
        {
            perror("bind error");
		return -1;
	}
	printf("bind socket ok\n");
}
int  add_data_to_xml_node(xmlNode *node,char *key,char *value)
{
	//add child data
	if(node == NULL)
	{
		printf("xml node can't be null\n");
		return -1;
	}
	if(xmlNewChild(node,NULL,BAD_CAST key ,BAD_CAST value) < 0)
	{
		printf("can,t add -> %s -->%s\n",key,value);
		return -1;
	}
	return 1;
}
int  socket_xml_exec(void)
{
	char rev[SOCKET_BUF_SIZE];
	char send[SOCKET_BUF_SIZE];
	char *sendPtr = send;
	int revBufSize = 0;
	int sendBufSize = 0,sendResult = 0;
	int state = 0;
	static int counter = 0;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t src_len;
	
	memset(rev,0,SOCKET_BUF_SIZE);
	memset(send,0,SOCKET_BUF_SIZE);
	memset(&cliaddr,0,sizeof(struct sockaddr_in));
	printf("wait for net socket->%d\n",counter++);
	revBufSize = recvfrom(sockfd, rev, SOCKET_BUF_SIZE, 0, (struct sockaddr *)&cliaddr, &src_len);
	if(revBufSize < 0)
        {
		printf("receive error!!\n");
		return -1;
        }
	
	printf("receive data ->%d\n",revBufSize);
	if(parse_xml_from_buf(rev,revBufSize,&obSocket) < 0){
		printf("parse buf to xml error");
		return -1;
	}
	exec_socket_cmd(&obSocket,&sendPtr,&sendBufSize,SOCKET_BUF_SIZE);
	sendResult=sendto(sockfd, send, sendBufSize, 0, (struct sockaddr *)&cliaddr, sizeof(struct sockaddr));	
	if(revBufSize < 0)
        {
		printf("send ack data error!\n");
		return -1;
        }
	else if(sendResult != sendBufSize){
		printf("send ack data lost some -> %d - %d = %d\n",sendBufSize,sendResult, sendBufSize-sendResult);
		return -1;
	}
	return 1;
}
	
