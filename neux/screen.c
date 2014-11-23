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
 * Neuros X screen support routines.
 *
 * REVISION:
 *
 *
 * 1) Initial creation. ----------------------------------- 2006-05-06 MG
 *
 */

//#define DBGMSG //
#include "nc-err.h"
#include "widgets.h"
#include "nx-widgets.h"


/**
 * Function returns screen height.
 *
 * @return
 *       Screen height in pixel.
 *
 */
int NeuxScreenHeight(void)
{
    return NxAppGetNeuxScreenHeight();
}


/**
 * Function returns screen width.
 *
 * @return
 *       Screen width in pixel.
 *
 */
int NeuxScreenWidth(void)
{
    return NxAppGetNeuxScreenWidth();
}

/**
 * Function snap screen to image file.
 *
 * @param path
 *      full name of .jpg file
 * @param x
 *      x coordinate on screen.
 * @param y
 *      y coordinate on screen.
 * @param w
 *      width in pixel.
 * @param h
 *      height in pixel.
 *
 */
void NeuxSnapScreen(const char *path, int x, int y, int w, int h)
{
	GrSnapScreenToImage(path, x, y, w, h);
}


