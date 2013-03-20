/*******************************************
* compile: gcc -I/usr/include/libxml2/ -lxml2 tree1.c
* usage: create a xml tree
*
*******************************************/
#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h> 
#include "parse_xml.h"                   
void *parse_xml_from_buf(char *buf,int len,int *state);
int main(int argc, char **argv)
{
       xmlDocPtr doc = NULL,doc2=NULL;       /* document pointer */
	xmlChar *buf;
	int bufferSize;
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
	xmlDocDumpFormatMemory(doc,&buf,&bufferSize,1);
	printf("char ->%d\n%s",bufferSize,buf);
       xmlFreeDoc(doc);
	//read from memory
	//doc2 = xmlReadMemory(buf,bufferSize,NULL,NULL,0);
	int type = 0;
	parse_xml_from_buf(buf,bufferSize,&type);
	//printf("read from char *-->\n");
	//xmlSaveFormatFileEnc(argc > 1 ? argv[1] : "-", doc2, "UTF-8", 1); 
       xmlCleanupParser();
       xmlMemoryDump();      //debug memory for regression tests

       return(0);
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
#define XML_BUF_SIZE 40
void *parse_xml_from_buf(char *buf,int len,int *type){
	OB_SOCKET obSocket,*ob;
	ob = &obSocket;
	char *tmp;
	char fuck[XML_BUF_SIZE];
	tmp = fuck;
	
//	printf("parese xml from buf ->%d->%s",len,buf);
	memset((void *)ob,0,sizeof(OB_SOCKET));
	xmlDoc  *doc = xmlReadMemory((xmlChar *)buf,len,NULL,NULL,0);
	if(doc == NULL){
		*type = 0;
		printf("can convert\n");
		return NULL;
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
	
}
int exec_socket_cmd(OB_SOCKET *ob){
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
		case OB_SET_HIDE :
			//
			break;
		case OB_SET_MIN_RUN :
			//
			break;
		case OB_SET_BOTTOM :
			//
			break;
		case OB_SET_TOP :
			break;
		case OB_SET_MAXIMIZE :
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
		default :
			break;
	}
}
int start_socket_server(int port){

}
