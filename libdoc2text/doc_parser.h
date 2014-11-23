#ifndef DOCTOTEXT_DOC_PARSER_H
#define DOCTOTEXT_DOC_PARSER_H

#include <wv2/ustring.h>
#include <string>
#include "doc2text.h"

class FormattingStyle;
class Metadata;

class DOCParser
{
	private:
		struct Implementation;
		Implementation* impl;

	public:
		DOCParser(const std::string& file_name);
		~DOCParser();

		void setVerboseLogging(bool verbose);
		void setLogStream(std::ostream& log_stream);

		bool parseStart(const FormattingStyle& formatting);
		unsigned long pieceCount();

		wvWare::UString& pieceText(const FormattingStyle& formatting);
		bool pieceSeek(unsigned long offset, enum DOCSeekWhence whence);
		unsigned long pieceTell();

		/* Navigation in piece */
		int getParagraphCount();
		int getParagraphOffset(int para);
		int getParagraphLevel(int para);
		int offsetToParagraph(int offset);

		Metadata metaData();

		bool error();
};

#endif
