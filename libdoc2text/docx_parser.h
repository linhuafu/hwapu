#ifndef DOCTOTEXT_ODFOOXML_PARSER_H
#define DOCTOTEXT_ODFOOXML_PARSER_H

#include "formatting_style.h"
#include "doc2text.h"
#include <string>

class Metadata;

class DOCXParser
{
	private:
		struct Implementation;
		Implementation* impl;

	public:
		DOCXParser(const std::string &file_name);
		~DOCXParser();

		void setVerboseLogging(bool verbose);
		void setLogStream(std::ostream& log_stream);

		bool plainText();

		std::string& getTextContent();

		/* Navigation in piece */
		int getParagraphCount();
		int getParagraphOffset(int para);
		int getParagraphLevel(int para);
		int offsetToParagraph(int offset);

		Metadata metaData();

		bool error();
};

#endif
