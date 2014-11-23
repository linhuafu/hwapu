#ifndef __DOCX_H__
#define __DOCX_H__


void* docx_open(const char *fpath);
void docx_close(void* id);

int docx_get_text(void* id, int offset, char *buf,  int *len);
int docx_get_byte_offset(void* id, int offset, int nChar);

int docx_skip_sentences(void* id, int offset, int n);
int docx_skip_heading(void* id, int offset, int n);
int docx_skip_paragraph(void* id, int offset, int n);
unsigned long docx_getTotalSize (void* id);

#endif
