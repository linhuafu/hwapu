#ifndef DOCTOTEXT_MISC_H
#define DOCTOTEXT_MISC_H

#include "formatting_style.h"
#include <string>
#include <vector>
#include <ustring.h>
using namespace wvWare;

struct tm;

typedef std::vector<std::string> svector;

std::string formatTable(std::vector<svector>& mcols, const FormattingStyle& options);
std::string formatUrl(const std::string& mlink, const FormattingStyle& options);
std::string formatList(std::vector<std::string>& mlist, const FormattingStyle& options);

std::string ustring_to_string(const UString& s);

/**
	Parses date and time in %Y-%m-%dT%H:%M:%S or %Y%m%d;%H%M%S format
**/
bool string_to_date(const std::string& s, tm& date);

#endif
