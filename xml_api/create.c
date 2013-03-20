/*******************************************
* compile: gcc -I/usr/include/libxml2/ -lxml2 tree1.c
* usage: create a xml tree
*
*******************************************/
#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h> 
#include "parse_xml.h"                   
void *parse_xml_from_buf(char *buf,int len,int *state);
int main(int argc, char **argv)
{
       xmlDocPtr doc = NULL,doc2=NULL;       /* document pointer */
	xmlChar *buf;
	int buffer;
       xmlNodePtr root_node = NULL, node1 = NULL, node2 = NULL,node3 = NULL; /* node pointers */                                                      
       //Creates a new document, a node and set it as a root node
       doc = xmlNewDoc(BAD_CAST "1.0");
       root_node = xmlNewNode(NULL, BAD_CAST "root");
       xmlDocSetRootElement(doc, root_node);    
       //creates a new node, which is "attached" as child node of root_node node. 
       // xmlNewProp() creates attributes, which is "attached" to an node.
       node1=xmlNewChild(root_node, NULL, BAD_CAST "method", BAD_CAST"raise");
       node1 =xmlNewChild(root_node, NULL, BAD_CAST "src",BAD_CAST "remote_dev");
       //xmlNewProp(node1, BAD_CAST "attribute", BAD_CAST "yes"); 
	node1=xmlNewChild(root_node, NULL, BAD_CAST "app",NULL);
	node2 = xmlNewChild(node1,NULL,BAD_CAST "winid",BAD_CAST "winid");	
	node2 = xmlNewChild(node1,NULL,BAD_CAST "pid",BAD_CAST "pid");	
	node2 = xmlNewChild(node1,NULL,BAD_CAST "title",BAD_CAST "title");	
	node2 = xmlNewChild(node1,NULL,BAD_CAST "cmd",BAD_CAST "cmd");	
	node2 = xmlNewChild(node1,NULL,BAD_CAST "name",BAD_CAST "name");	
	node1 = xmlNewChild(root_node,NULL,BAD_CAST "data",NULL);
	node2 = xmlNewChild(node1,NULL,BAD_CAST "x",BAD_CAST "100");	
	node2 = xmlNewChild(node1,NULL,BAD_CAST "y",BAD_CAST "100");	
	node2 = xmlNewChild(node1,NULL,BAD_CAST "width",BAD_CAST "200");	
	node2 = xmlNewChild(node1,NULL,BAD_CAST "height",BAD_CAST "200");	
	node2 = xmlNewChild(node1,NULL,BAD_CAST "extend",BAD_CAST "0");	

	//Here goes another way to create nodes.
       //node = xmlNewNode(NULL, BAD_CAST "app");
      // node1 = xmlNewText(BAD_CAST"other way to create content");
       //xmlAddChild(node, node1);
       //xmlAddChild(root_node, node);                            
//Dumping document to stdio or file
       //xmlSaveFormatFileEnc(argc > 1 ? argv[1] : "-", doc, "UTF-8", 1); 
/*free the document */
	//printf("save file ok\n");
	xmlDocDumpFormatMemory(doc,&buf,&buffer,1);
	printf("char ->%d\n%s",buffer,buf);
       xmlFreeDoc(doc);
	//read from memory
	doc2 = xmlReadMemory(buf,buffer,NULL,NULL,0);
	int type = 0;
	parse_xml_from_buf(buf,buffer,&type);
	//printf("read from char *-->\n");
	//xmlSaveFormatFileEnc(argc > 1 ? argv[1] : "-", doc2, "UTF-8", 1); 
       xmlCleanupParser();
       xmlMemoryDump();      //debug memory for regression tests

       return(0);
}
void print_element_names(xmlNode * a_node)
{
    xmlNode *cur_node = NULL;

    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            printf("node->  %d\t%s\t-->%s\n",cur_node->type, cur_node->name,
	xmlNodeGetContent(cur_node));
        }
        print_element_names(cur_node->children);
    }
}
void *parse_xml_from_buf(char *buf,int len,int *type){
	xmlDoc  *doc = xmlReadMemory((xmlChar *)buf,len,NULL,NULL,0);
	if(doc == NULL){
		*type = 0;
		return NULL;
	}
	xmlNode *root =NULL, *node1 = NULL, *node2 = NULL;
	root = xmlDocGetRootElement(doc);
	print_element_names(root);
	node1 = root->children;
	if(node1 == NULL)
	{
		*type = 0;
		return NULL;
	}
	char src[40];
	sprintf(src,"%s",xmlNodeGetContent(node1));
	
		
}
