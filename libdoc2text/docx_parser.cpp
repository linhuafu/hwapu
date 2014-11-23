/****************************************************************************
**
** DocToText - Converts DOC, XLS, PPT, RTF, ODF (ODT, ODS, ODP),
**             OOXML (DOCX, XLSX, PPTX) and HTML documents to plain text.
**             Extracts metadata and annotations.
**
** Copyright (c) 2006-2013, SILVERCODERS(R)
** http://silvercoders.com
**
** Project homepage: http://silvercoders.com/en/products/doctotext
**
** This program may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file COPYING.GPL included in the
** packaging of this file.
**
** Please remember that any attempt to workaround the GNU General Public
** License using wrappers, pipes, client/server protocols, and so on
** is considered as license violation. If your program, published on license
** other than GNU General Public License version 2, calls some part of this
** code directly or indirectly, you have to buy commercial license.
** If you do not like our point of view, simply do not use the product.
**
** Licensees holding valid commercial license for this product
** may use this file in accordance with the license published by
** SILVERCODERS and appearing in the file COPYING.COM
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
**
*****************************************************************************/

#include <fstream>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <iostream>
#include <map>
#include "metadata.h"
#include "misc.h"
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include "docx_parser.h"
#include "doctotext_unzip.h"
#include "ut_xml.h"

using namespace std;

const int CASESENSITIVITY = 1;

static string int_to_str(int i)
{
	ostringstream s;
	s << i;
	return s.str();
}

static int str_to_int(const string& s)
{
	istringstream ss(s);
	int i;
	ss >> i;
	return i;
}

class DocxListener : public UT_XML::Listener
{
public:
	DocxListener() : m_error(false), m_verbose_logging(false), m_log_stream(&std::cerr), in_table(false) {}

	virtual void startElement (const gchar * name, const gchar ** atts);
	virtual void endElement (const gchar * name);
	virtual void charData (const gchar * buffer, int length);

	void setVerboseLogging(bool verbose);
	void setLogStream(std::ostream& log_stream);

	bool readDocx(const DocToTextUnzip& zipfile, std::string* content, std::vector<pair<int, int> >* paragraphs);

	bool error();

private:
	bool m_verbose_logging;
	std::ostream* m_log_stream;

	bool space_preserve;
	bool contex_text;
	bool in_table;
	bool table_col0;
	bool table_col_newline;
	int para_begin;
	int heading_level;

	std::string* m_content;
	std::vector<pair<int /*offset*/, int /*heading_level*/> >* m_paragraphs;

	bool m_error;
};

void DocxListener::setVerboseLogging(bool verbose)
{
	m_verbose_logging = verbose;
}

void DocxListener::setLogStream(std::ostream& log_stream)
{
	m_log_stream = &log_stream;
}

static const gchar *get_attr_val(const gchar ** atts, const gchar *attr)
{
	while (*atts) {
		if (!xmlStrcmp(*atts, attr))
			return *(atts + 1);
		atts++;
	}

	return NULL;
}

void DocxListener::startElement (const gchar * name, const gchar ** atts)
{
	if (m_verbose_logging)
		*m_log_stream << "startElement: " << name << endl;

	contex_text = false;
	if (!xmlStrcmp(name, "w:tbl")) {		// Table
		in_table = true;
	}
	else if (!xmlStrcmp(name, "w:p")) {		// Paragraph
		if (!in_table) {
			para_begin = m_content->length();
			heading_level = 0;
		}
	}
	else if (!xmlStrcmp(name, "w:ilvl")) {	// Paragraph Heading Level
		heading_level = 1 + atoi(get_attr_val(atts, "w:val"));
	}
	else if (!xmlStrcmp(name, "w:t")) {		// Text
		const gchar *xml_space = get_attr_val(atts, "xml:space");
		space_preserve = xml_space != NULL && !xmlStrcmp(xml_space, "preserve");
		contex_text = true;
	}
	else if (!xmlStrcmp(name, "w:cr") ||	// Carriage return
	         !xmlStrcmp(name, "w:br")) {	// Page break
		try {
			m_content->append("\n");
		}
		catch(std::bad_alloc) {
			*m_log_stream << "out of memory" << endl;
			m_error = true;
		}
		table_col_newline = in_table;
	}
	else if (!xmlStrcmp(name, "w:tab")) {	// Tab
		try {
			m_content->append("\t");
		}
		catch(std::bad_alloc) {
			*m_log_stream << "out of memory" << endl;
			m_error = true;
		}
	}
	else if (!xmlStrcmp(name, "w:tr")) {	// Table Row
		table_col0 = true;
		table_col_newline = false;
	}
	else if (!xmlStrcmp(name, "w:tc")) {	// Table Column
		if (!table_col0) {
			try {
				m_content->append("\t");
			}
			catch(std::bad_alloc) {
				*m_log_stream << "out of memory" << endl;
				m_error = true;
			}
		}
		table_col0 = false;
	}
}

void DocxListener::endElement (const gchar * name)
{
	if (m_verbose_logging)
		*m_log_stream << "endElement: " << name << endl;

	if (!xmlStrcmp(name, "w:tbl")) {		// Table
		in_table = false;
	}
	else if (!xmlStrcmp(name, "w:p")) {		// Paragraph
		if (!in_table || table_col_newline) {
			try {
				m_content->append("\n");
				if (!in_table) m_paragraphs->push_back(pair<int, int>(para_begin, heading_level));
			}
			catch(std::bad_alloc) {
				*m_log_stream << "out of memory" << endl;
				m_error = true;
			}
		}
	}
	else if (!xmlStrcmp(name, "w:tr")) {	// Table Row
		try {
			m_content->append("\n");
		}
		catch(std::bad_alloc) {
			*m_log_stream << "out of memory" << endl;
			m_error = true;
		}
	}
}

void DocxListener::charData (const gchar * buffer, int length)
{
	if (contex_text) {
		if (space_preserve) {
			try {
				m_content->append(buffer, 0, length);
			}
			catch(std::bad_alloc) {
				*m_log_stream << "out of memory" << endl;
				m_error = true;
			}
		} else {
			int i, j;
			for (i = 0; i < length; i++)
				if (!isspace(buffer[i]))
					break;
			for (j = length - 1; j >= i; j--)
				if (!isspace(buffer[j]))
					break;
			if (i <= j) {
				try {
					m_content->append(buffer, i, j - i + 1);
				}
				catch(std::bad_alloc) {
					*m_log_stream << "out of memory" << endl;
					m_error = true;
				}
			}
		}
	}
}

bool DocxListener::readDocx(const DocToTextUnzip& zipfile,
			std::string* content, std::vector<pair<int, int> >* paragraphs)
{
	bool ret;

	string xml_content;
	ret = zipfile.read("word/document.xml", &xml_content);
	if (!ret)
	{
		*m_log_stream << "Error reading word/document.xml" << endl;
		return false;
	}

	m_content = content;
	m_paragraphs = paragraphs;

	UT_XML ut_xml;
	ut_xml.setListener((UT_XML::Listener*)this);
	UT_Error error = ut_xml.parse (xml_content.c_str(), xml_content.length());
	if (error != UT_OK) {
		*m_log_stream << "Error parsing word/document.xml" << endl;
		return false;
	}

	return true;
}

bool DocxListener::error(void)
{
	return m_error;
}

struct DOCXParser::Implementation
{
	bool m_error;

	std::string m_file_name;

	bool m_verbose_logging;
	std::ostream* m_log_stream;

	string m_content;

	vector<pair<int /*offset*/, int /*heading_level*/> > m_paragraphs;
};

DOCXParser::DOCXParser(const string& file_name)
{
	impl = new Implementation();
	impl->m_error = false;
	impl->m_file_name = file_name;
	impl->m_verbose_logging = false;
	impl->m_log_stream = &std::cerr;
}

DOCXParser::~DOCXParser()
{
	delete impl;
}

void DOCXParser::setVerboseLogging(bool verbose)
{
	impl->m_verbose_logging = verbose;
}

void DOCXParser::setLogStream(std::ostream& log_stream)
{
	impl->m_log_stream = &log_stream;
}

bool DOCXParser::plainText()
{
	bool ret;

	DocToTextUnzip zipfile(impl->m_file_name.c_str());
	if (impl->m_log_stream != &std::cerr)
		zipfile.setLogStream(*impl->m_log_stream);
	if (!zipfile.open())
	{
		*impl->m_log_stream << "Error opening file " << impl->m_file_name.c_str() << endl;
		impl->m_error = true;
		return false;
	}

	if (!zipfile.exists("word/document.xml"))
	{
		zipfile.close();
		*impl->m_log_stream << "Error reading word/document.xml" << endl;
		impl->m_error = true;
		return false;
	}

	DocxListener listener;
	listener.setVerboseLogging(impl->m_verbose_logging);
	if (impl->m_log_stream != &std::cerr)
		listener.setLogStream(*impl->m_log_stream);
	ret = listener.readDocx(zipfile, &impl->m_content, &impl->m_paragraphs) && !listener.error();
	if (!ret)
		impl->m_error = true;

	zipfile.close();
	return ret;
}

std::string& DOCXParser::getTextContent()
{
	return impl->m_content;
}

int DOCXParser::getParagraphCount()
{
	return impl->m_paragraphs.size();
}

int DOCXParser::getParagraphOffset(int para)
{
	return impl->m_paragraphs[para].first;
}

int DOCXParser::getParagraphLevel(int para)
{
	return impl->m_paragraphs[para].second;
}

int DOCXParser::offsetToParagraph(int offset)
{
	int len = impl->m_paragraphs.size();
	int first = 0;

	while (len > 1) {
		int half = len / 2;
		if (offset < impl->m_paragraphs[first + half].first)
			len = half;
		else {
			first += half;
			len -= half;
		}
	}

	return first;
}

static std::string get_element_string_value(xmlDocPtr doc, xmlNodePtr elem)
{
	xmlChar* val = xmlNodeListGetString(doc, elem->xmlChildrenNode, 1);
	std::string s((char*)val);
	xmlFree(val);
	return s;
}

Metadata DOCXParser::metaData()
{
	impl->m_error = false;

	Metadata meta;
	DocToTextUnzip zipfile(impl->m_file_name.c_str());
	if (impl->m_log_stream != &std::cerr)
		zipfile.setLogStream(*impl->m_log_stream);
	if (!zipfile.open())
	{
		*impl->m_log_stream << "Error opening file " << impl->m_file_name.c_str() << endl;
		impl->m_error = true;
		return meta;
	}

	if (zipfile.exists("docProps/core.xml"))
	{
		std::string core_xml;
		if (!zipfile.read("docProps/core.xml", &core_xml))
		{
			*impl->m_log_stream << "Error reading docProps/core.xml" << endl;
			zipfile.close();
			impl->m_error = true;
			return meta;
		}

		xmlDocPtr core_doc = xmlParseMemory(core_xml.c_str(), core_xml.length());
		if (core_doc == NULL)
		{
			impl->m_error = true;
			return meta;
		}
		xmlNodePtr core_root_elem = xmlDocGetRootElement(core_doc);
		xmlNodePtr core_elem2 = core_root_elem->children;
		while (core_elem2 != NULL)
		{
			if (string((char*)core_elem2->name) == "creator")
				meta.setAuthor(get_element_string_value(core_doc, core_elem2));
			if (string((char*)core_elem2->name) == "created")
			{
				tm creation_date;
				string_to_date(get_element_string_value(core_doc, core_elem2), creation_date);
				meta.setCreationDate(creation_date);
			}
			if (string((char*)core_elem2->name) == "lastModifiedBy")
				meta.setLastModifiedBy(get_element_string_value(core_doc, core_elem2));
			if (string((char*)core_elem2->name) == "modified")
			{
				tm last_modification_date;
				string_to_date(get_element_string_value(core_doc, core_elem2), last_modification_date);
				meta.setLastModificationDate(last_modification_date);
			}
			core_elem2 = core_elem2->next;
		}
		xmlFreeDoc(core_doc);
		xmlCleanupParser();
		xmlMemoryDump();

		std::string app_xml;
		if (!zipfile.read("docProps/app.xml", &app_xml))
		{
			*impl->m_log_stream << "Error reading docProps/app.xml" << endl;
			zipfile.close();
			impl->m_error = true;
			return meta;
		}
		xmlDocPtr app_doc = xmlParseMemory(app_xml.c_str(), app_xml.length());
		if (app_doc == NULL)
		{
			impl->m_error = true;
			return meta;
		}
		xmlNodePtr app_root_elem = xmlDocGetRootElement(app_doc);
		xmlNodePtr app_elem2 = app_root_elem->children;
		while (app_elem2 != NULL)
		{
			if (string((char*)app_elem2->name) == "Pages")
				meta.setPageCount(str_to_int(get_element_string_value(app_doc, app_elem2)));
			if (string((char*)app_elem2->name) == "Words")
				meta.setWordCount(str_to_int(get_element_string_value(app_doc, app_elem2)));
			app_elem2 = app_elem2->next;
		}
		xmlFreeDoc(app_doc);
		xmlCleanupParser();
		xmlMemoryDump();
	}

	zipfile.close();
	return meta;
}

bool DOCXParser::error(void)
{
	return impl->m_error;
}
