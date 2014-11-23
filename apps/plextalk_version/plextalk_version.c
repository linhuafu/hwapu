#include <stdio.h>
#include <libintl.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>

#define DEFAULT_VERSION_STR	"0x0000"
#define VERSION_TMP_FILE	"/tmp/plextalk_version"
#define PLEXTALK_CONFIG_DIR	"/opt/plextalk/data/config/"

xmlDocPtr xml_open(const xmlChar* filename)
{
	xmlDocPtr doc;

	xmlKeepBlanksDefault(0);

	/* Load XML document */
	doc = xmlParseFile(filename);
	if (doc == NULL)
		fprintf(stderr, "Error: unable to parse file \"%s\"\n", filename);

	return doc;
}

xmlXPathObjectPtr get_xpath_object(xmlDocPtr doc, const xmlChar *xpath)
{
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;

	/* Create xpath evaluation context */
	xpathCtx = xmlXPathNewContext(doc);
	if (xpathCtx == NULL) {
		fprintf(stderr,"Error: unable to create new XPath context\n");
		return NULL;
	}

	/* Evaluate xpath expression */
	xpathObj = xmlXPathEvalExpression(xpath, xpathCtx);
	xmlXPathFreeContext(xpathCtx);

	if (xpathObj == NULL) {
		fprintf(stderr,"Error: unable to evaluate xpath expression\n");
		return NULL;
	}

	if (xmlXPathNodeSetIsEmpty(xpathObj->nodesetval)) {
		fprintf(stderr, "Error: empty node set\n");
		xmlXPathFreeObject(xpathObj);
		return NULL;
	}

	return xpathObj;
}

xmlChar* xml_get(xmlDocPtr doc, const xmlChar* xpath)
{
	xmlXPathObjectPtr xpathObj;
	xmlChar* ret;

	xpathObj = get_xpath_object(doc, xpath);
	if (xpathObj == NULL)
		return NULL;

	ret = xmlNodeGetContent(xpathObj->nodesetval->nodeTab[0]);
	xmlXPathFreeObject(xpathObj);

	return ret;
}

void xml_close(xmlDocPtr doc)
{
	if (doc != NULL)
		xmlFreeDoc(doc);
}

static int GetVerInfo(char *str, int len)
{
	int ret = 0;
	xmlDocPtr doc;
	xmlChar *buf = NULL;
		
	doc = xml_open((const xmlChar*)PLEXTALK_CONFIG_DIR "version.xml");
	if(doc){
		buf = xml_get(doc, (const xmlChar*)"/version/software");
		if(buf){
			ret = 1;
			strncpy(str, (const char*)buf,len);
			xmlFree(buf);
		}
		xml_close(doc);
	}
	else{
		fprintf(stderr, "%s", "read version.xml fail\n");	
	}
	return ret;
}


static void
get_rid_of_dot (char* buf, int buf_size)
{
	char *ret_buf;
	int i, j;

	ret_buf = malloc(buf_size);
	memset(ret_buf, 0, buf_size);

	ret_buf[0] = '0';
	ret_buf[1] = 'x';
	j = 2;
	for (i = 0; i < buf_size; i++) {
		if (buf[i] != '.') {
			ret_buf[j] = buf[i];
			j++;
		}
	}

	memcpy(buf, ret_buf, buf_size);
	free(ret_buf);
}

int main (void)
{
	char version_str[32];
	char buffer[32];
	char version_str_x[32];		//16进制方式
	
	int fd;
	
	memset(version_str, 0, 32);
	memset(buffer, 0, 32);
	
	if (!GetVerInfo(buffer, 32)) {
		fprintf(stderr, "%s", "GetVerInfo Error!!\n");
		snprintf(version_str, 32, "%s", DEFAULT_VERSION_STR);
	} else {
		fprintf(stderr, "%s", "GetVerInfo Success!!\n");
		get_rid_of_dot(buffer, 32);
		snprintf(version_str, 32, "%s", buffer);
	}

	snprintf(version_str_x, 32, "%d", strtol(version_str, NULL, 16));		//以16进制方式写入

	fd = open(VERSION_TMP_FILE, O_RDWR|O_CREAT|O_APPEND, 777);

	fprintf(stderr, "%s%s!!!\n", "GetVerion:", version_str_x);
	
	write(fd, version_str_x, strlen(version_str_x));

	close(fd);

	return 0;
}

