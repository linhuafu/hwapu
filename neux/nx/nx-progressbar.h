/*
 *  Copyright(C) 2006 Neuros Technology International LLC.
 *               <www.neurostechnology.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 2 of the License.
 *
 *
 *  This program is distributed in the hope that, in addition to its
 *  original purpose to support Neuros hardware, it will be useful
 *  otherwise, but WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************
 *
 * Widget ProgressBar header.
 *
 * REVISION:
 *
 *
 * 1) Initial creation. ----------------------------------- 2006-03-02 EY
 *
 */

#ifndef _NXPROGRESSBAR_H_
#define _NXPROGRESSBAR_H_

#include "nx-base.h"

#define PROGRESSBAR_WIDTH 		100
#define PROGRESSBAR_HEIGHT 		20

#define PROGRESSBAR_BACKGROUND_COLOR	MWRGB(243, 112, 22)
#define PROGRESSBAR_BORDER_SIZE			1

enum EM_PROGRESS_TYPE
{
	ptNormal,
	ptVolume
};

typedef struct
{
	int orientation;
	int type;

	int min;
	int max;
	int value;


	GR_COLOR blockcolor;
	int borderwidth;

	CallBackStruct cb_exposure;

}NX_PROGRESSBAR;

/*member  function */
void
NxProgressBarCreate (NX_WIDGET *);
void
NxProgressBarDestroy(NX_WIDGET *);
void
NxProgressBarDraw(NX_WIDGET *);

/*event handler */
void
NxProgressBarExposureHandler (GR_EVENT *, NX_WIDGET *);




/*public function*/
int
NxSetProgressBarValue(NX_WIDGET *, int);
int
NxGetProgressBarValue(NX_WIDGET *, int *);

#endif //_NXPROGRESSBAR_H_

