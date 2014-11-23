#ifndef __RADIO_REC_DRAW_H__
#define __RADIO_REC_DRAW_H__

#include <stdio.h>
#include "radio_type.h"


void radio_rec_draw_image(struct radio_nano* nano);
void radio_rec_draw_source(struct radio_nano* nano);
void radio_rec_draw_title(struct radio_nano* nano, struct _rec_data* data);
void radio_rec_draw_time(struct radio_nano* nano, struct _rec_data* data);
void radio_rec_draw_copy(struct radio_nano* nano);


#endif
