
#include "doc_parser.h"
#include <fstream>
#include "docx_parser.h"
#include "misc.h"
#include "doctotext_unzip.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "doc2text.h"

using namespace std;
using namespace wvWare;

const struct FormattingStyle formatting = {
	TABLE_STYLE_TABLE_LOOK,
	URL_STYLE_TEXT_ONLY,
	LIST_STYLE_DOT,
};

DOC2TEXT_HANDLE doc2text_create(const char* filename)
{
	std::string path = filename;
	DOCParser *parser = new DOCParser(path);
	if (!parser->parseStart(formatting)) {
		delete parser;
		return NULL;
	}
	return (DOC2TEXT_HANDLE)parser;
}

void doc2text_destroy(DOC2TEXT_HANDLE handle)
{
	DOCParser *parser = (DOCParser *)handle;
	delete parser;
}

unsigned long doc2text_total_pieces(DOC2TEXT_HANDLE handle)
{
	DOCParser *parser = (DOCParser *)handle;
	return parser->pieceCount();
}

int doc2text_seek(DOC2TEXT_HANDLE handle, unsigned long offset, enum DOCSeekWhence whence)
{
	DOCParser *parser = (DOCParser *)handle;
	return parser->pieceSeek(offset, whence) ? 0 : -1;
}

unsigned long doc2text_tell(DOC2TEXT_HANDLE handle)
{
	DOCParser *parser = (DOCParser *)handle;
	return parser->pieceTell();
}

uint16_t* doc2text_gettext(DOC2TEXT_HANDLE handle, unsigned long *length)
{
	DOCParser *parser = (DOCParser *)handle;
	UString* string = &parser->pieceText(formatting);
	*length = string->length();
	return (uint16_t*) string->data();
}

int doc2txt_getParagraphCount(DOC2TEXT_HANDLE handle)
{
	DOCParser *parser = (DOCParser *)handle;
	return parser->getParagraphCount();
}

int doc2txt_getParagraphOffset(DOC2TEXT_HANDLE handle, int para)
{
	DOCParser *parser = (DOCParser *)handle;
	return parser->getParagraphOffset(para);
}

int doc2txt_getParagraphLevel(DOC2TEXT_HANDLE handle, int para)
{
	DOCParser *parser = (DOCParser *)handle;
	return parser->getParagraphLevel(para);
}

int doc2txt_offsetToParagraph(DOC2TEXT_HANDLE handle, int offset)
{
	DOCParser *parser = (DOCParser *)handle;
	return parser->offsetToParagraph(offset);
}


DOC2TEXT_HANDLE docx2text_create(const char* filename)
{
	std::string path = filename;
	DOCXParser *parser = new DOCXParser(path);
	if (!parser->plainText()) {
		delete parser;
		return NULL;
	}
	return (DOC2TEXT_HANDLE)parser;
}

void docx2text_destroy(DOC2TEXT_HANDLE handle)
{
	DOCXParser *parser = (DOCXParser *)handle;
	delete parser;
}

void docx2txt_getText(DOC2TEXT_HANDLE handle, char** addr, int* size)
{
	DOCXParser *parser = (DOCXParser *)handle;
	*addr = parser->getTextContent().c_str();
	*size = parser->getTextContent().length();
}

int docx2txt_getParagraphCount(DOC2TEXT_HANDLE handle)
{
	DOCXParser *parser = (DOCXParser *)handle;
	return parser->getParagraphCount();
}

int docx2txt_getParagraphOffset(DOC2TEXT_HANDLE handle, int para)
{
	DOCXParser *parser = (DOCXParser *)handle;
	return parser->getParagraphOffset(para);
}

int docx2txt_getParagraphLevel(DOC2TEXT_HANDLE handle, int para)
{
	DOCXParser *parser = (DOCXParser *)handle;
	return parser->getParagraphLevel(para);
}

int docx2txt_offsetToParagraph(DOC2TEXT_HANDLE handle, int offset)
{
	DOCXParser *parser = (DOCXParser *)handle;
	return parser->offsetToParagraph(offset);
}
