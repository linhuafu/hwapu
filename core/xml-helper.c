#include <stdlib.h>
#include <string.h>

#include "xml-helper.h"
#include "plextalk-config.h"

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

void xml_close(xmlDocPtr doc)
{
	if (doc != NULL)
		xmlFreeDoc(doc);
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

int xml_get_integer(xmlDocPtr doc, const xmlChar* xpath, int32_t *value)
{
	xmlXPathObjectPtr xpathObj;
	xmlChar* content;

	xpathObj = get_xpath_object(doc, xpath);
	if (xpathObj == NULL)
		return -1;

	content = xmlNodeGetContent(xpathObj->nodesetval->nodeTab[0]);
	*value = atoi(content);
	xmlFree(content);

	xmlXPathFreeObject(xpathObj);

	return 0;
}

int xml_get_float(xmlDocPtr doc, const xmlChar* xpath, double *value)
{
	xmlXPathObjectPtr xpathObj;
	xmlChar* content;

	xpathObj = get_xpath_object(doc, xpath);
	if (xpathObj == NULL)
		return -1;

	content = xmlNodeGetContent(xpathObj->nodesetval->nodeTab[0]);
	*value = atof(content);
	xmlFree(content);

	xmlXPathFreeObject(xpathObj);

	return 0;
}

int xml_read_equalizer_10bands(const char *preset, struct equalizer_10bands *equ)
{
	char path[PATH_MAX];
	xmlDocPtr doc;
	xmlXPathObjectPtr xpathObj;
	xmlNodeSetPtr nodes;
	int i;

	strlcpy(path, PLEXTALK_EQU_PRESETS_DIR, PATH_MAX);
	strlcat(path, preset, PATH_MAX);

	/* Load XML document */
	doc = xmlParseFile(path);
	if (doc == NULL) {
		fprintf(stderr, "Error: unable to parse file \"%s\"\n", path);
		return -1;
	}

	xpathObj = get_xpath_object(doc, (xmlChar*) "/equalizer/band");
	if (xpathObj == NULL) {
		fprintf(stderr,"Error: unable to evaluate xpath expression \"%s\"\n", path);
		xmlFreeDoc(doc);
		return -1;
	}

	nodes = xpathObj->nodesetval;
	for (i = 0; i < nodes->nodeNr; i++) {
		xmlChar* gain = xmlNodeGetContent(nodes->nodeTab[i]);
		equ->gains[i] = atof(gain);
		xmlFree(gain);
	}

	/* Cleanup of XPath data */
	xmlXPathFreeObject(xpathObj);

	/* free the document */
	xmlFreeDoc(doc);

	return 0;
}

int plextalk_time_system_init(void)
{
//wgp 
#if 0
	xmlDocPtr doc;
	int ret;

	doc = xml_open(PLEXTALK_CONFIG_DIR "plextalk.xml");
	if (doc == NULL)
		return -1;
	//km 24hoursystem->hoursystem
	ret = xml_get_integer(doc, (xmlChar*) "/plextalk/timesystem/hoursystem",
							&g_config->hour_system);
	ret |= xml_get_integer(doc, (xmlChar*) "/plextalk/timesystem/format",
							&g_config->time_format);

	xml_close(doc);

	return ret;
#else 


	 Get_Device_Setting_Xml();
	

#endif
//wgp 
}
