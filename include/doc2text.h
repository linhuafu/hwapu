#ifndef __DOC2TEXT_H
#define __DOC2TEXT_H

#include <stdint.h>

enum DOCSeekWhence { DOC_SEEK_SET, DOC_SEEK_CUR, DOC_SEEK_END };

typedef void* DOC2TEXT_HANDLE;

#ifdef __cplusplus
extern "C" {
#endif

DOC2TEXT_HANDLE doc2text_create(const char* filename);
void doc2text_destroy(DOC2TEXT_HANDLE handle);
void doc2text_gettext(DOC2TEXT_HANDLE handle, uint16_t** addr, int* length);
/* Navigation in paragraph */
int doc2txt_getParagraphCount(DOC2TEXT_HANDLE handle);
int doc2txt_getParagraphOffset(DOC2TEXT_HANDLE handle, int para);
int doc2txt_getParagraphLevel(DOC2TEXT_HANDLE handle, int para);
int doc2txt_offsetToParagraph(DOC2TEXT_HANDLE handle, int offset);

DOC2TEXT_HANDLE docx2text_create(const char* filename);
void docx2text_destroy(DOC2TEXT_HANDLE handle);
void docx2txt_getText(DOC2TEXT_HANDLE handle, char** addr, int* size);
/* Navigation in paragraph */
int docx2txt_getParagraphCount(DOC2TEXT_HANDLE handle);
int docx2txt_getParagraphOffset(DOC2TEXT_HANDLE handle, int para);
int docx2txt_getParagraphLevel(DOC2TEXT_HANDLE handle, int para);
int docx2txt_offsetToParagraph(DOC2TEXT_HANDLE handle, int offset);

#ifdef __cplusplus
}
#endif

#endif
