#ifndef __UTF8_UTILS_H
#define __UTF8_UTILS_H

#include <nano-X.h>
#include "nxlist.h"

typedef struct {
	slist_t list;
	const char *text;
	size_t len;
	size_t width;
} text_line_t;

int
utf8_next_nchar(const char *str, int offset, int n);

int
utf8_text_to_fit(GR_GC_ID gc, GR_SIZE max_width,
		const char *text, size_t len, GR_SIZE* width);

int
utf8_fit_width(const char *lang, GR_GC_ID gc, GR_SIZE width,
		const char *text, size_t len, slist_t *head);

int
utf8_fit_area(const char *lang, GR_GC_ID gc, GR_SIZE width, GR_SIZE height, int line_space,
		const char **ptext, size_t len, slist_t *head);

#endif
