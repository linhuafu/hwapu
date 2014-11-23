#include <linebreak.h>
#include "utf8_utils.h"

/*
 * utf is a string of 1, 2, 3 or 4 bytes.  The valid strings
 * are as follows (in "bit format"):
 *    0xxxxxxx                                      valid 1-byte
 *    110xxxxx 10xxxxxx                             valid 2-byte
 *    1110xxxx 10xxxxxx 10xxxxxx                    valid 3-byte
 *    11110xxx 10xxxxxx 10xxxxxx 10xxxxxx           valid 4-byte
 */
int
utf8_next_nchar(const char *str, int offset, int n)
{
	int inc;

	inc = n > 0 ? 1 : -1;

	for (n = (n > 0) ? n : -n; n > 0; n--) {
		do {
			offset += inc;
		} while ((str[offset] & 0xc0) == 0x80);
	}

	return offset;
}

int
utf8_text_to_fit(GR_GC_ID gc, GR_SIZE max_width,
					const char *text, size_t len, GR_SIZE* width)
{
	GR_SIZE w, h, b;
	int first = 0;
	GR_SIZE first_width = 0;

	while (len > 0) {
		int half = len / 2;
		while ((text[first + half] & 0xc0) == 0x80)
			half--;
		if (half == 0)
			break;

		GrGetGCTextSize(gc, (char*)text + first, half, MWTF_UTF8, &w, &h, &b);
		if (first_width + w > max_width)
			len = half;
		else {
			first_width += w;
			first += half;
			len -= half;
		}
	}

	if (width)
		*width = first_width;
	return first;
}

int
utf8_fit_width(const char *lang, GR_GC_ID gc, GR_SIZE width,
		const char *text, size_t len, slist_t *head)
{
	GR_SIZE w, h, b, last_w;
	char *brks;
	text_line_t *lt;
	int i, line_start, last_word;
	int line_count = 0;

	GrGetGCTextSize(gc, text, len, MWTF_UTF8, &w, &h, &b);
	if (w <= width) {
		lt = malloc(sizeof(*lt));
		if (lt == NULL)
			return -1;
		lt->text = text;
		lt->len = len;
		lt->width = w;
		nxSlistAdd(head, &lt->list);
		return 1;
	}

	brks = malloc(len);
	if (brks == NULL)
		return -1;

	/* Show the breaking points */
    set_linebreaks_utf8(text, len, lang, brks);

	line_start = 0;
	last_word = 0;
	for (i = 1; i <= len; i++) {
		switch (brks[i-1]) {
		case LINEBREAK_MUSTBREAK:
		case LINEBREAK_ALLOWBREAK:
			GrGetGCTextSize(gc, text + line_start, i - line_start, MWTF_UTF8, &w, &h, &b);
			if (w > width) {
				if (last_word > line_start) {
					lt = malloc(sizeof(*lt));
					if (lt == NULL) {
						free(brks);
						return line_count ? line_count : -1;
					}
					if (w - last_w > width) {
						int partial_len, partial_w;
						partial_len = utf8_text_to_fit(gc, width - last_w,
									text + last_word, i - last_word, &partial_w);
						last_word += partial_len;
						last_w += partial_w;
					}
					lt->text = text + line_start;
					lt->len = last_word - line_start;
					lt->width = last_w;
					nxSlistAdd(head, &lt->list);
					head = &lt->list;
					line_count++;

					line_start = last_word;
					w = w - last_w;
				}

				/* too long word */
				while (w > width) {
					lt = malloc(sizeof(*lt));
					if (lt == NULL) {
						free(brks);
						return line_count ? line_count : -1;
					}
					lt->text = text + line_start;
					lt->len = utf8_text_to_fit(gc, width, text + line_start, i - line_start, &last_w);
					lt->width = last_w;
					nxSlistAdd(head, &lt->list);
					head = &lt->list;
					line_count++;

					line_start += lt->len;
					w = w - last_w;
				}
			}
			if (brks[i-1] == LINEBREAK_MUSTBREAK || w == width) {
				lt = malloc(sizeof(*lt));
				if (lt == NULL) {
					free(brks);
					return line_count ? line_count : -1;
				}
				lt->text = text + line_start;
				lt->len = i - line_start;
				lt->width = w;
				nxSlistAdd(head, &lt->list);
				head = &lt->list;
				line_count++;

				line_start = i;
				last_word = line_start;
				last_w = 0;
			} else {
				last_word = i;
				last_w = w;
			}
			break;
		}
	}

	if (line_start < len) {
		lt = malloc(sizeof(*lt));
		if (lt == NULL) {
			free(brks);
			return line_count ? line_count : -1;
		}
		lt->text = text + line_start;
		lt->len = len - line_start;
		lt->width = w;
		nxSlistAdd(head, &lt->list);
		line_count++;
	}

	free(brks);
	return line_count;
}

int
utf8_fit_area(const char *lang, GR_GC_ID gc, GR_SIZE width, GR_SIZE height, int line_space,
		const char **ptext, size_t len, slist_t *head)
{
	GR_SIZE w, h, b, last_w;
	char *brks;
	text_line_t *lt;
	int i, line_start, last_word;
	int line_count = 0, max_line_count;
	const char *text = *ptext;

	GrGetGCTextSize(gc, text, len, MWTF_UTF8, &w, &h, &b);
	if (w <= width) {
		lt = malloc(sizeof(*lt));
		if (lt == NULL)
			return -1;
		lt->text = text;
		lt->len = len;
		lt->width = w;
		nxSlistAdd(head, &lt->list);
		*ptext = text + len;
		return 1;
	}

	brks = malloc(len);
	if (brks == NULL)
		return -1;

	/* Show the breaking points */
    set_linebreaks_utf8(text, len, lang, brks);

	max_line_count = height / (h + line_space);
	line_start = 0;
	last_word = 0;
	for (i = 1; i <= len; i++) {
		switch (brks[i-1]) {
		case LINEBREAK_MUSTBREAK:
		case LINEBREAK_ALLOWBREAK:
			GrGetGCTextSize(gc, text + line_start, i - line_start, MWTF_UTF8, &w, &h, &b);
			if (w > width) {
				if (last_word > line_start) {
					lt = malloc(sizeof(*lt));
					if (lt == NULL) {
						free(brks);
						if (line_count == 0)
							return -1;
						*ptext = text + line_start;
						return line_count;
					}
					lt->text = text + line_start;
					lt->len = last_word - line_start;
					lt->width = last_w;
					nxSlistAdd(head, &lt->list);
					line_count++;
					if (line_count == max_line_count) {
						*ptext = text + last_word;
						return line_count;
					}
					head = &lt->list;

					line_start = last_word;
					w = w - last_w;
				}

				/* too long word */
				while (w > width) {
					lt = malloc(sizeof(*lt));
					if (lt == NULL) {
						free(brks);
						if (line_count == 0)
							return -1;
						*ptext = text + line_start;
						return line_count;
					}
					lt->text = text + line_start;
					lt->len = utf8_text_to_fit(gc, width, text + line_start, i - line_start, &last_w);
					lt->width = last_w;
					nxSlistAdd(head, &lt->list);
					line_count++;
					if (line_count == max_line_count) {
						*ptext = lt->text + lt->len;
						return line_count;
					}
					head = &lt->list;

					line_start += lt->len;
					w = w - last_w;
				}
			}
			if (brks[i-1] == LINEBREAK_MUSTBREAK || w == width) {
				lt = malloc(sizeof(*lt));
				if (lt == NULL) {
					free(brks);
					if (line_count == 0)
						return -1;
					*ptext = text + line_start;
					return line_count;
				}
				lt->text = text + line_start;
				lt->len = i - line_start;
				lt->width = w;
				nxSlistAdd(head, &lt->list);
				line_count++;
				if (line_count == max_line_count) {
					*ptext = text + i;
					return line_count;
				}
				head = &lt->list;

				line_start = i;
				last_word = line_start;
				last_w = 0;
			} else {
				last_word = i;
				last_w = w;
			}
			break;
		}
	}

	if (line_start < len && line_count < max_line_count) {
		lt = malloc(sizeof(*lt));
		if (lt == NULL) {
			free(brks);
			if (line_count == 0)
				return -1;
			*ptext = text + line_start;
			return line_count;
		}
		lt->text = text + line_start;
		lt->len = len - line_start;
		lt->width = w;
		nxSlistAdd(head, &lt->list);
		line_count++;
	}

	free(brks);
	return line_count;
}
