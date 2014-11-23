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

#include "oshared.h"

#include <iostream>
#include "metadata.h"
#include "wv2/olestream.h"

using namespace wvWare;

static bool read_vt_string(OLEStreamReader* reader, std::ostream& log_stream, std::string& s)
{
	U16 string_type = reader->readU16();
	if (string_type != 0x1E)
	{
		log_stream << "Incorrect string type.\n";
		return false;
	}
	reader->readU16(); // padding
	U32 len = reader->readU32();
	s = "";
	for (int j = 0; j < len - 1; j++)
	{
		char ch = (char)reader->readS8();
		if (ch == '\0')
			break;
		s += ch;
	}
	return true;
}

static bool read_vt_i4(OLEStreamReader* reader, std::ostream& log_stream, int& i)
{
	U16 string_type = reader->readU16();
	if (string_type != 0x0003)
	{
		log_stream << "Incorrect value type.\n";
		return false;
	}
	reader->readU16(); // padding
	i = reader->readS32();
	return true;
}

static bool read_vt_filetime(OLEStreamReader* reader, std::ostream& log_stream, tm& time)
{
	U16 type = reader->readU16();
	if (type != 0x0040)
	{
		log_stream << "Incorrect variable type.\n";
		return false;
	}
	reader->readU16(); // padding
	U32 file_time_low = reader->readU32();
	U32 file_time_high = reader->readU32();
	if (file_time_low == 0 && file_time_high == 0)
	{
		// Sometimes field exists but date is zero.
		// Last modification time saved by LibreOffice 3.5 when document is created is an example.
		time = (tm){0};
		return true;
	}
	unsigned long long int file_time = ((unsigned long long int)file_time_high << 32) | (unsigned long long int)file_time_low;
	time_t t = (time_t)(file_time / 10000000 - 11644473600LL);
	tm* res = gmtime(&t);
	if (res == NULL)
	{
		log_stream << "Incorrect time value.\n";
		return false;
	}
	time = *res;
	return true;
}

bool parse_oshared_summary_info(const std::string& file_name, std::ostream& log_stream, Metadata& meta)
{
	log_stream << "Extracting metadata.\n";
	OLEStorage storage(file_name);
	if (!storage.open(OLEStorage::ReadOnly))
	{
		log_stream << "Error opening " << file_name << " as OLE file.\n";
		return false;
	}
	OLEStreamReader* reader = storage.createStreamReader("\005SummaryInformation");
	if (reader == NULL)
	{
		log_stream << "Error opening SummaryInformation stream.\n";
		return false;
	}
	size_t field_set_stream_start = reader->tell();
	if (reader->readU16() != 0xFFFE)
	{
		log_stream << "Incorrect ByteOrder value.\n";
		return false;
	}
	U16 version = reader->readU16();
	if (version != 0x00)
	{
		log_stream << "Incorrect Version value.\n";
		return false;
	}
	U16 system_identifier = reader->readU32();
	for (int i = 0; i < 4; i++)
	{
		U32 clsid_part = reader->readU32();
		if (clsid_part != 0x00)
		{
			log_stream << "Incorrect CLSID value.\n";
			return false;
		}
	}
	U32 num_property_sets = reader->readU32();
	if (num_property_sets != 0x01 && num_property_sets != 0x02)
	{
		log_stream << "Incorrect number of property sets.\n";
		return false;
	}
	for (int i = 0; i < 4; i++)
	{
		U32 fmtid0_part = reader->readU32();
	}
	U32 offset = reader->readU32();
	int property_set_pos = field_set_stream_start + offset;
	reader->seek(property_set_pos);
	U32 size = reader->readU32();
	U32 num_props = reader->readU32();
	for (int i = 0; i < num_props; i++)
	{
		U32 prop_id = reader->readU32();
		U32 offset = reader->readU32();
		int p = reader->tell();
		switch (prop_id)
		{
			case 0x00000004:
			{
				reader->seek(property_set_pos + offset);
				std::string author;
				if (read_vt_string(reader, log_stream, author))
					meta.setAuthor(author);
				break;
			}
			case 0x00000008:
			{
				reader->seek(property_set_pos + offset);
				std::string last_modified_by;
				if (read_vt_string(reader, log_stream, last_modified_by))
					meta.setLastModifiedBy(last_modified_by);
				break;
			}
			case 0x0000000C:
			{
				reader->seek(property_set_pos + offset);
				tm creation_date;
				if (read_vt_filetime(reader, log_stream, creation_date))
					meta.setCreationDate(creation_date);
				break;
			}
			case 0x0000000D:
			{
				reader->seek(property_set_pos + offset);
				tm last_modification_date;
				if (read_vt_filetime(reader, log_stream, last_modification_date))
					meta.setLastModificationDate(last_modification_date);
				break;
			}
			case 0x0000000E:
			{
				reader->seek(property_set_pos + offset);
				int page_count;
				if (read_vt_i4(reader, log_stream, page_count))
					meta.setPageCount(page_count);
				break;
			}
			case 0x0000000F:
			{
				reader->seek(property_set_pos + offset);
				int word_count;
				if (read_vt_i4(reader, log_stream, word_count))
					meta.setWordCount(word_count);
				break;
			}
		}
		reader->seek(p);
	}
	delete reader;
	return true;
}

bool parse_oshared_document_summary_info(const std::string& file_name, std::ostream& log_stream, int& slide_count)
{
	log_stream << "Extracting additional metadata.\n";
	OLEStorage storage(file_name);
	if (!storage.open(OLEStorage::ReadOnly))
	{
		log_stream << "Error opening " << file_name << " as OLE file.\n";
		return false;
	}
	OLEStreamReader* reader = storage.createStreamReader("\005DocumentSummaryInformation");
	if (reader == NULL)
	{
		log_stream << "Error opening DocumentSummaryInformation stream.\n";
		return false;
	}
	size_t field_set_stream_start = reader->tell();
	if (reader->readU16() != 0xFFFE)
	{
		log_stream << "Incorrect ByteOrder value.\n";
		return false;
	}
	U16 version = reader->readU16();
	if (version != 0x00)
	{
		log_stream << "Incorrect Version value.\n";
		return false;
	}
	U16 system_identifier = reader->readU32();
	for (int i = 0; i < 4; i++)
	{
		U32 clsid_part = reader->readU32();
		if (clsid_part != 0x00)
		{
			log_stream << "Incorrect CLSID value.\n";
			return false;
		}
	}
	U32 num_property_sets = reader->readU32();
	if (num_property_sets != 0x01 && num_property_sets != 0x02)
	{
		log_stream << "Incorrect number of property sets.\n";
		return false;
	}
	for (int i = 0; i < 4; i++)
	{
		U32 fmtid0_part = reader->readU32();
	}
	U32 offset = reader->readU32();
	int property_set_pos = field_set_stream_start + offset;
	reader->seek(property_set_pos);
	U32 size = reader->readU32();
	U32 num_props = reader->readU32();
	bool slide_count_found = false;
	for (int i = 0; i < num_props; i++)
	{
		U32 prop_id = reader->readU32();
		U32 offset = reader->readU32();
		int p = reader->tell();
		switch (prop_id)
		{
			case 0x00000007:
				reader->seek(property_set_pos + offset);
				if (!read_vt_i4(reader, log_stream, slide_count))
					return false;
				slide_count_found = true;
				break;
		}
		reader->seek(p);
	}
	if (!slide_count_found)
		slide_count = -1;
	delete reader;
	return true;
}
