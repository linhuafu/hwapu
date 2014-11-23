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
 * Form Main header.
 *
 * REVISION:
 * 
 *
 * 1) Initial creation. ----------------------------------- 2006-02-06 EY
 *
 */

#ifndef _FILE_DIALOG_H_
#define _FILE_DIALOG_H_

#include "widgets.h"
#include "vprompt-defines.h"
#include "libvprompt.h"
#include "plextalk-config.h"


int FileDialogOpen(const char *path, int _filtertype, GR_BOOL multisel, char ***selectedfiles, int *selectedcount,int* tts_running);
void FileDialogFreeSelections(char ***selectedfiles, int selectedcount);

#endif //_NWMAIN_H_
