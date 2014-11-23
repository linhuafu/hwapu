#ifndef __XML_HELPER_H__
#define __XML_HELPER_H__

#include <stdint.h>
#include <limits.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>

xmlDocPtr xml_open(const xmlChar* filename);
void xml_close(xmlDocPtr doc);

/* caller must call xmlXPathFreeObject */
xmlXPathObjectPtr get_xpath_object(xmlDocPtr doc, const xmlChar *xpath);

/* caller must call xmlFree */
xmlChar* xml_get(xmlDocPtr doc, const xmlChar* xpath);

int xml_get_integer(xmlDocPtr doc, const xmlChar* xpath, int32_t *value);
int xml_get_float(xmlDocPtr doc, const xmlChar* xpath, double *value);


struct equalizer_10bands {
	double gains[10];
};

int xml_read_equalizer_10bands(const char* filename, struct equalizer_10bands *equ);

int plextalk_time_system_init(void);

#endif
