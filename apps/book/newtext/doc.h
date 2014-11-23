#ifndef __DOC_H__
#define __DOC_H__

void* doc_open(const char *fpath);
void doc_close(void* id);

int doc_get_total_piece(void* id);
unsigned long doc_get_piece_size(void* id, unsigned long long offset);

int doc_get_text(void* id, int offset, char *buf,  int *len);
int doc_get_byte_offset(void* id, int offset, unsigned long nchar);

int doc_skip_sentences(void* id, int offset, int n);
int doc_skip_heading(void* id, int offset, int orient);
int doc_skip_paragraph(void* id, int offset, int orient);

#endif
