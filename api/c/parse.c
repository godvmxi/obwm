#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>

static xmlDocPtr
getDocPtr(char* docname) {
	xmlDocPtr doc = NULL;
	xmlKeepBlanksDefault(0);

	doc = xmlParseFile(docname);
	if(NULL == doc) {
		fprintf(stderr, "document cannot be parsed!\n");
		exit(1);
	}
	return doc;
}

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

int main() {
	char* docname = "web.html";
	xmlDocPtr doc = NULL;
	xmlXPathObjectPtr xpath_obj = NULL;
	xmlNodeSetPtr nodeset = NULL;
	xmlChar* xpath_exp = (xmlChar*)"//pid";
	xmlChar* uri;

	doc = getDocPtr(docname);

	xpath_obj = getXPathObjectPtr(doc, xpath_exp);
	//printf("pid ->%s ->%s\n",xpath_obj->name,NULL);
	if(NULL != xpath_obj) {
		nodeset = xpath_obj->nodesetval;
		int i = 0;
		for(i = 0; i < nodeset->nodeNr; i ++) {
			uri = xmlGetProp(nodeset->nodeTab[i],(const xmlChar*)"href");
			printf("link address:%s->%s->%s\n",uri,nodeset->nodeTab[i]->name,
			xmlNodeGetContent(nodeset->nodeTab[i]));
			xmlFree(uri);
		}
		xmlXPathFreeObject(xpath_obj);
	}
	xmlFreeDoc(doc);
	xmlCleanupParser();
	
	return 1;
}

