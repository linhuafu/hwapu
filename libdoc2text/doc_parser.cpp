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

#include "doc_parser.h"

#include "misc.h"
#include <glib.h>
#include "fields.h"
#include "handlers.h"
#include "olestream.h"
#include "paragraphproperties.h"
#include "parser.h"
#include "parserfactory.h"
#include <stdio.h>
#include <unistd.h>
#include "ustring.h"
#include <vector>
#include <map>
#ifdef WIN32
	#include <windows.h>
#endif
#include "wvlog.h"
#include "metadata.h"
#include "oshared.h"

#include "wv2/lists.h"

using namespace wvWare;
using namespace std;

static UString string_to_ustring(const std::string s)
{
	UString r;
	for (int i = 0; i < s.length(); i++)
	{
		if (s[i] & 128)
		{
			gunichar* out = g_utf8_to_ucs4_fast(&s.c_str()[i], 2, NULL);
			r += UString(UChar(out[0]));
			g_free(out);
			i++;
		}
		else
			r += UString(s[i]);
	}
	return r;
}

struct CurrentTable
{
	bool in_table;
	std::string curr_cell_text;
	svector curr_row_cells;
	std::vector<svector> rows;
	CurrentTable() : in_table(false) {};
};

struct CurrentHeaderFooter
{
	bool in_header;
	bool in_footer;
	UString header;
	UString footer;
	CurrentHeaderFooter() : in_header(false), in_footer(false) {};
};

class DocToText_TextHandler : public TextHandler
{
	private:
		UString* Text;
		std::vector<pair<int /*offset*/, int /*heading_level*/> >* m_paragraphs;

		CurrentHeaderFooter* m_curr_header_footer;
		CurrentTable* m_curr_table;
		FormattingStyle m_formatting;
		bool m_verbose_logging;
		std::ostream* m_log_stream;
		// constants are from MS Word Binary File Format Specification
		enum FieldType
		{
			FLT_NONE = 0,
			FLT_EMBEDDED_OBJECT = 58,
			FLT_HYPERLINK = 88
		};
		FieldType m_curr_field_type;
		UString m_curr_hyperlink_url;
		UString m_curr_hyperlink_text;

	public:
		DocToText_TextHandler(UString* text, std::vector<pair<int /*offset*/, int /*heading_level*/> >* paragraphs,
				CurrentHeaderFooter* curr_header_footer, CurrentTable* curr_table,
				const FormattingStyle& formatting, bool verbose_logging, std::ostream& log_stream)
			: m_curr_header_footer(curr_header_footer)
		{
			Text = text;
			m_paragraphs = paragraphs;
			m_curr_table = curr_table;
			m_formatting = formatting;
			m_curr_field_type = FLT_NONE;
			m_verbose_logging = verbose_logging;
			m_log_stream = &log_stream;
		}

		void sectionStart(SharedPtr<const Word97::SEP> sep)
		{
		}

		void sectionEnd()
		{
		}

		void pageBreak()
		{
		}

		void paragraphStart(SharedPtr<const ParagraphProperties>
			paragraphProperties)
		{
			if (m_curr_table->in_table && (!paragraphProperties->pap().fInTable))
			{
				m_curr_table->in_table = false;
				(*Text) += string_to_ustring(formatTable(m_curr_table->rows, m_formatting));
				m_curr_table->rows.clear();
			}
			if (!paragraphProperties->pap().fInTable &&
				!m_curr_header_footer->in_header &&
				!m_curr_header_footer->in_footer) {
				int heading_level = 0;
				if (paragraphProperties->listInfo())
					heading_level = 1 + paragraphProperties->listInfo()->startAt();
				m_paragraphs->push_back(pair<int, int>(Text->length(), heading_level));
			}
		}

		void paragraphEnd()
		{
			if (m_curr_table->in_table)
				m_curr_table->curr_cell_text += "\n";
			else if(m_curr_header_footer->in_header)
				m_curr_header_footer->header += UString("\n");
			else if (m_curr_header_footer->in_footer)
				m_curr_header_footer->footer += UString("\n");
			else
				(*Text) += UString("\n");
		}

		void runOfText (const UString &text, SharedPtr< const Word97::CHP > chp)
		{
			if (m_curr_field_type == FLT_HYPERLINK)
			{
				if (m_curr_hyperlink_url.isEmpty())
				{
					m_curr_hyperlink_url = text;
					if (m_curr_hyperlink_url.substr(0, 12) == " HYPERLINK \"")
						m_curr_hyperlink_url = m_curr_hyperlink_url.substr(12);
					if (m_curr_hyperlink_url[m_curr_hyperlink_url.length() - 1] == '"')
						m_curr_hyperlink_url = m_curr_hyperlink_url.substr(0, m_curr_hyperlink_url.length() - 1);
				}
				else
					m_curr_hyperlink_text += text;
			}
			else if (m_curr_field_type == FLT_EMBEDDED_OBJECT)
			{
				// do nothing
			}
			else if (m_curr_table->in_table)
				m_curr_table->curr_cell_text += ustring_to_string(text);
			else if (m_curr_header_footer->in_header)
				m_curr_header_footer->header += text;
			else if (m_curr_header_footer->in_footer)
				m_curr_header_footer->footer += text;
			else
				(*Text) += text;
		}

		void specialCharacter(SpecialCharacter character,
			SharedPtr<const Word97::CHP> chp)
		{
		}

		void footnoteFound (FootnoteData::Type type, UChar character,
			SharedPtr<const Word97::CHP> chp,
			const FootnoteFunctor &parseFootnote)
		{
		}

		void footnoteAutoNumber(SharedPtr<const Word97::CHP> chp)
		{
		}

		void fieldStart(const FLD *fld,
			SharedPtr<const Word97::CHP> chp)
		{
			if (fld->flt == FLT_HYPERLINK)
				m_curr_field_type = FLT_HYPERLINK;
			else if (fld->flt == FLT_EMBEDDED_OBJECT)
			{
				if (m_verbose_logging)
					*m_log_stream << "Embedded OLE object reference found.\n";
				m_curr_field_type = FLT_EMBEDDED_OBJECT;
				/*
				 * todo: handle embedded ole object
				 */
			}
			else if (m_verbose_logging)
				*m_log_stream << "Field with unknown type " << std::hex << (int)fld->flt << ".\n";
		}

		void fieldSeparator(const FLD* fld,
			SharedPtr<const Word97::CHP> chp)
		{
		}

		void fieldEnd(const FLD* fld,
			SharedPtr<const Word97::CHP> chp)
		{
			if (m_curr_field_type == FLT_HYPERLINK)
			{
				m_curr_field_type = FLT_NONE;
				UString link_str = UString(formatUrl(ustring_to_string(m_curr_hyperlink_url), m_formatting).c_str()) + m_curr_hyperlink_text;
				m_curr_hyperlink_url = "";
				m_curr_hyperlink_text = "";
				if (m_curr_table->in_table)
					m_curr_table->curr_cell_text += ustring_to_string(link_str);
				else if (m_curr_header_footer->in_header)
					m_curr_header_footer->header += link_str;
				else if (m_curr_header_footer->in_footer)
					m_curr_header_footer->footer += link_str;
				else
					(*Text) += link_str;
			}
			else if (m_curr_field_type == FLT_EMBEDDED_OBJECT)
				m_curr_field_type = FLT_NONE;
		}
};

class DocToText_TableHandler : public TableHandler
{
	private:
		UString* Text;
		CurrentTable* m_curr_table;

	public:
		DocToText_TableHandler(UString* text, CurrentTable* curr_table)
		{
			Text = text;
			m_curr_table = curr_table;
		}

		void tableRowStart(SharedPtr<const Word97::TAP> tap)
		{
			m_curr_table->in_table = true;
		}

		void tableRowEnd()
		{
			m_curr_table->rows.push_back(m_curr_table->curr_row_cells);
			m_curr_table->curr_row_cells.clear();
		}

		void tableCellStart()
		{
		}

		void tableCellEnd()
		{
			m_curr_table->curr_row_cells.push_back(m_curr_table->curr_cell_text);
			m_curr_table->curr_cell_text = "";
		}
};

class DocToText_SubDocumentHandler : public SubDocumentHandler
{
	private:
		CurrentHeaderFooter* m_curr_header_footer;

	public:
		DocToText_SubDocumentHandler(CurrentHeaderFooter* curr_header_footer)
			: m_curr_header_footer(curr_header_footer)
		{
		}

		virtual void headerStart(HeaderData::Type type)
		{
			switch (type)
			{
				case HeaderData::HeaderOdd:
				case HeaderData::HeaderEven:
				case HeaderData::HeaderFirst:
					m_curr_header_footer->in_header = true;
					break;
				case HeaderData::FooterOdd:
				case HeaderData::FooterEven:
				case HeaderData::FooterFirst:
					m_curr_header_footer->in_footer = true;
					break;
			}
		}

		virtual void headerEnd()
		{
			m_curr_header_footer->in_header = false;
			m_curr_header_footer->in_footer = false;
		}
};

struct DOCParser::Implementation
{
	bool m_error;
	std::string m_file_name;
	bool m_verbose_logging;
	std::ostream* m_log_stream;
	std::streambuf* m_cerr_buf_backup;

	SharedPtr<Parser> m_parser;
	UString m_text;
	CurrentHeaderFooter m_curr_header_footer;
	CurrentTable m_curr_table;
	DocToText_TextHandler* m_text_handler;
	DocToText_TableHandler* m_table_handler;
	DocToText_SubDocumentHandler* m_sub_document_handler;
	U32 m_piece_count, m_curr_piece;

	vector<pair<int /*offset*/, int /*heading_level*/> > m_paragraphs;

	void modifyCerr()
	{
		if (m_verbose_logging)
		{
			if (m_log_stream != std::cerr)
				m_cerr_buf_backup = std::cerr.rdbuf(m_log_stream->rdbuf());
		}
		else
			std::cerr.setstate(std::ios::failbit);
	}

	void restoreCerr()
	{
		if (m_verbose_logging)
		{
			if (m_log_stream != std::cerr)
				std::cerr.rdbuf(m_cerr_buf_backup);
		}
		else
			std::cerr.clear();
	}
};

DOCParser::DOCParser(const std::string& file_name)
{
	impl = new Implementation();
	impl->m_error = false;
	impl->m_file_name = file_name;
	impl->m_verbose_logging = false;
	impl->m_log_stream = &std::cerr;
	impl->m_text_handler = NULL;
	impl->m_table_handler = NULL;
	impl->m_sub_document_handler = NULL;
}

DOCParser::~DOCParser()
{
	if (impl->m_text_handler)
		delete impl->m_text_handler;
	if (impl->m_table_handler)
		delete impl->m_table_handler;
	if (impl->m_sub_document_handler)
		delete impl->m_sub_document_handler;
	delete impl;
}

void DOCParser::setVerboseLogging(bool verbose)
{
	impl->m_verbose_logging = verbose;
}

void DOCParser::setLogStream(std::ostream& log_stream)
{
	impl->m_log_stream = &log_stream;
}

bool DOCParser::parseStart(const FormattingStyle& formatting)
{
	impl->m_error = false;
	impl->modifyCerr();

	impl->m_parser = ParserFactory::createParser(impl->m_file_name.c_str());
	if (!impl->m_parser || !impl->m_parser->isOk())
	{
		*impl->m_log_stream << "Creating parser failed.\n";
		goto err0;
	}

	impl->m_text_handler = new DocToText_TextHandler(&impl->m_text, &impl->m_paragraphs,
		&impl->m_curr_header_footer, &impl->m_curr_table, formatting,
		impl->m_verbose_logging, *impl->m_log_stream);
	if (!impl->m_text_handler) {
		*impl->m_log_stream << "Creating TextHandler failed.\n";
		goto err0;
	}
	impl->m_parser->setTextHandler(impl->m_text_handler);

	impl->m_table_handler = new DocToText_TableHandler(&impl->m_text, &impl->m_curr_table);
	if (!impl->m_table_handler) {
		*impl->m_log_stream << "Creating TableHandler failed.\n";
		goto err1;
	}
	impl->m_parser->setTableHandler(impl->m_table_handler);

	impl->m_sub_document_handler = new DocToText_SubDocumentHandler(&impl->m_curr_header_footer);
	if (!impl->m_sub_document_handler) {
		*impl->m_log_stream << "Creating SubDocumentHandler failed.\n";
		goto err2;
	}
	impl->m_parser->setSubDocumentHandler(impl->m_sub_document_handler);

	if (!impl->m_parser->parseStart(&impl->m_piece_count)) {
		*impl->m_log_stream << "Parse Start failed.\n";
		goto err3;
	}
	impl->m_curr_piece = 0;

	impl->restoreCerr();
	return true;

err3:
	delete impl->m_sub_document_handler;
	impl->m_sub_document_handler = NULL;
err2:
	delete impl->m_table_handler;
	impl->m_table_handler = NULL;
err1:
	delete impl->m_text_handler;
	impl->m_text_handler = NULL;
err0:
	impl->m_error = true;
	impl->restoreCerr();
	return false;
}

unsigned long DOCParser::pieceCount()
{
	return impl->m_piece_count;
}

bool DOCParser::pieceSeek(unsigned long offset, enum DOCSeekWhence whence)
{
	switch (whence) {
	case DOC_SEEK_CUR:
		if (impl->m_curr_piece + offset <= impl->m_piece_count) {
			impl->m_curr_piece += offset;
			return true;
		}
		break;
	case DOC_SEEK_SET:
		if (offset <= impl->m_piece_count) {
			impl->m_curr_piece = offset;
			return true;
		}
		break;
	case DOC_SEEK_END:
		if (offset <= impl->m_piece_count) {
			impl->m_curr_piece = impl->m_piece_count - offset;
			return true;
		}
		break;
	}

	return false;
}

unsigned long DOCParser::pieceTell()
{
	return impl->m_curr_piece;
}

UString& DOCParser::pieceText(const FormattingStyle& formatting)
{
	impl->m_text = "";
	impl->m_paragraphs.clear();

	if (impl->m_curr_piece < impl->m_piece_count) {
		impl->modifyCerr();
		impl->m_parser->parsePiece(impl->m_curr_piece);
		impl->restoreCerr();
		impl->m_curr_piece++;
	}

	return impl->m_text;
}

int DOCParser::getParagraphCount()
{
	return impl->m_paragraphs.size();
}

int DOCParser::getParagraphOffset(int para)
{
	return impl->m_paragraphs[para].first;
}

int DOCParser::getParagraphLevel(int para)
{
	return impl->m_paragraphs[para].second;
}

int DOCParser::offsetToParagraph(int offset)
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

Metadata DOCParser::metaData()
{
	impl->m_error = false;
	Metadata meta;
	impl->m_error = !parse_oshared_summary_info(impl->m_file_name, *impl->m_log_stream, meta);
	return meta;
}

bool DOCParser::error()
{
	return impl->m_error;
}
