#ifndef DOCTOTEXT_OSHARED_H
#define DOCTOTEXT_OSHARED_H

#include <iosfwd>

class Metadata;

bool parse_oshared_summary_info(const std::string& file_name, std::ostream& log_stream, Metadata& meta);
bool parse_oshared_document_summary_info(const std::string& file_name, std::ostream& log_stream, int& slide_count);

#endif
