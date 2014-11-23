#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <unistr.h>
#include <string.h>
#include <glib.h>
#include "doc2text.h"
#include "linebreak.h"
#include "clang.h"
#include "dbmalloc.h"


typedef struct {

	DOC2TEXT_HANDLE handle; 

	int total_size;

	int paragraph_count;
	
	char* content;

} DocxFd;


static const uint8_t accept[] = {
	0x3F, 					//英文?
	0x21, 					//英文!
	0x2E, 					//英文.
	0x0D, 0x0A, 			//换行符号
	0xEF, 0xBC, 0x9F, 		//中文?
	0xEF, 0xBC, 0x81,		//中文!
	0xE3, 0x80, 0x82,		//中文.
	0x00,
};


static int utf8_bytes (const utf8_t* p)
{
	int bytes = 0;

	if (*p < 0x80) 
		bytes = 1;
	else if ((*p & 0xE0) == 0xC0)
		bytes = 2;
	else if ((*p & 0xF0) == 0xE0)
		bytes = 3;
	else if ((*p & 0xF8) == 0xF0)
		bytes = 4;
	else 
		printf("ERROR!!!, invaild utf8 args!\n");

	return bytes;
}


static int sentence_symbol_utf8 (const utf8_t* p)
{	
	int bytes = utf8_bytes(p);
	int accept_size = sizeof(accept)/sizeof(accept[0]) - 1;		//0x00 is the end of accept
	const utf8_t *sptr = accept;
	const utf8_t *eptr = sptr;
	
	while (eptr < accept + accept_size) {

		if (!u8_strncmp(p, eptr, bytes)) {
			//printf("sentence symbol utf8 find!\n");
			printf("offset = %d\n", eptr - accept);
			return 1;
		}
		eptr = g_utf8_next_char(sptr);	
		sptr = eptr;
	}

	return 0;
}


void* docx_open (const char *fpath)
{
	DocxFd* fd = app_malloc(sizeof(DocxFd));
	if (!fd) {
		printf("Malloc DocxFd error!\n");
		return NULL;
	}

	fd->handle= docx2text_create(fpath);
	if (!fd->handle) {
		printf("Doc2text create error!\n");
		app_free(fd);
		return NULL;
	} 

	docx2txt_getText(fd->handle, &(fd->content), &(fd->total_size));

	fd->paragraph_count = docx2txt_getParagraphCount(fd->handle);
	printf("total_paragraph = %d\n", fd->paragraph_count);

	return (void*)fd;
}


void docx_close (void* id)
{
	DocxFd* fd = (DocxFd*)id;

	if (fd) {
		docx2text_destroy(fd->handle);
		app_free(fd);
	}
}


/*	func: get_text form docx text buffer
 *	
 *  @return :	1, this buffer will be the last of file
 *				0, get_text normal
 *				-1, offset is error!
 */

int docx_get_text(void* id, int offset, char *buf,  int *len)
{
	//utf8
	DocxFd* fd = (DocxFd*)id;

	int cp_len;
	int ret, i;
	char* brks;
	int brk_index;

	int max_offset = fd->total_size;

	if (offset >= max_offset) {
		*len = 0;
		return -1;
	} else {
	
		if ( ( offset + *len) > max_offset) {
			printf("Max_offset littler than offset + *len!\n");
			cp_len = max_offset - offset;
			memcpy(buf, fd->content + offset, cp_len);
			*len = cp_len;
			ret = 1;

		} else {
			const char* text_lang = get_text_lang(UTF8, (const char*)(fd->content + offset), *len);
			brks = app_malloc(*len);
			set_linebreaks_utf8((const utf8_t*)(fd->content + offset), *len, text_lang, brks);

			for (i = *len - 1; i >= 0; i--) {
				if ((brks[i] == LINEBREAK_ALLOWBREAK) 
					|| (brks[i] == LINEBREAK_MUSTBREAK)) {
					brk_index = i;
					break;
				}
			}

			if (i <= 0) {			// breakable place not found
				printf("WARNING: not found breakable place ");
				*len = 0;
				ret = 0;
			} else {
				memcpy(buf, fd->content + offset, brk_index + 1);
				*len = brk_index + 1;
				ret = 0;
			}

			app_free(brks);
		}
	}

	return ret;
}


int docx_get_byte_offset(void* id, int offset, int nChar)
{
	return offset + nChar;
}


//return -1 means to the end
static int 
docx_next_sentence (void* id, int offset)
{
	DocxFd* fd = (DocxFd*)id;

	char* eptr;
	char* nptr;
	char* sptr = fd->content + offset;
	int pos = offset;

	while (pos < fd->total_size - 1) {

		eptr =  g_utf8_next_char(sptr);

		if (sentence_symbol_utf8((const utf8_t*)eptr)) {
			
			do {
				nptr = g_utf8_next_char(eptr);
				eptr = nptr;
			} while (((nptr - fd->content) < fd->total_size - 1)
					&& sentence_symbol_utf8((const utf8_t*)nptr));	//skip continuous

			if ((nptr - fd->content) >= (fd->total_size - 1))
				pos = fd->total_size - 1;
			else
				pos = nptr - fd->content;

			printf("find next sentence, pos = %d\n", pos);

			return pos;
		}

		pos = eptr - fd->content;
		sptr = eptr;
	}

	printf("next sentence already to the end, return fd->total_size");

	return -1;
}


//return 0, means has been to the begning
static int
docx_prev_sentence (void* id, int offset)
{
	DocxFd* fd = (DocxFd*)id;
	int pos;
	char* nptr;

	char* sptr = fd->content + offset;
	char* eptr = sptr;

	int scount = 0;

	while (eptr) {
		eptr = g_utf8_find_prev_char(fd->content, sptr);
		if (!eptr) {
			printf("g_utf8 find prev char has been to the begining!\n");
			return 0;
		}
	
		if (sentence_symbol_utf8((const utf8_t*)eptr)) {

			if (scount) {
				nptr = g_utf8_next_char(eptr);	
				pos = nptr - fd->content;	
				
				return pos;
			} else {
				do {								//skip contious
					nptr = g_utf8_find_prev_char(fd->content, eptr);
					eptr = nptr;
				} while (nptr && sentence_symbol_utf8((const utf8_t*)nptr));

				if (!nptr)
					return 0;	

				scount = 1;
			}
		}

		sptr = eptr;
	}

	printf("prev sentence, already to the begining!\n");
	return 0;
}


int docx_skip_sentences(void* id, int offset, int n)
{
	DocxFd* fd = (DocxFd*)id;

	int i;
	int ret_offset = offset;
	if (n > 0) {
		for (i = 0; i < n; i++) {
			ret_offset = docx_next_sentence(id, ret_offset);
			if (ret_offset == -1) {
				printf("Docx skip n sentences, to the file end!\n");
				return  -1;
			}
		}
	} else {
		for (i = 0; i < -n; i++) {
			ret_offset = docx_prev_sentence(id, ret_offset);
			if (ret_offset == 0) {
				printf("Docx skip n sentences, to the file beginning!\n");
				return -1;
			}
		}
	}

	return ret_offset;
}


int docx_skip_heading(void* id, int offset, int orient)
{
	DocxFd* fd = (DocxFd*)id;
	
	if(0 == orient){
		return offset;
	}
	
	int cur_paragraph = docx2txt_offsetToParagraph(fd->handle, offset);
	while (1) {
		orient > 0 ? cur_paragraph++ : cur_paragraph--;
		if ((cur_paragraph < 0) || (cur_paragraph > fd->paragraph_count - 1)){
			printf("paragraph limit\n");
			return -1;
		}

		if (docx2txt_getParagraphLevel(fd->handle, cur_paragraph) != 0) { //level 0 heading
			printf("find heading!, paragraph = %d\n", cur_paragraph);
			break;
		}
	}

	return docx2txt_getParagraphOffset(fd->handle, cur_paragraph);
}


int docx_skip_paragraph(void* id, int offset, int orient)
{
	DocxFd* fd = (DocxFd*)id;

	if(0 == orient){
		return offset;
	}
	
	int cur_paragraph = docx2txt_offsetToParagraph(fd->handle, offset);

	orient > 0 ? cur_paragraph++ : cur_paragraph--;

	if ((cur_paragraph < 0) || (cur_paragraph > fd->paragraph_count - 1)){
		printf("paragraph limit\n");
		return -1;
	}
	return docx2txt_getParagraphOffset(fd->handle, cur_paragraph);
}

unsigned long docx_getTotalSize (void* id)
{
	DocxFd* fd = (DocxFd*)id;
	if (fd) {
		return fd->total_size;
	}
	return 0;
}

