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
 * Modal widget support routines.
 *
 * REVISION:
 *
 * 2) Pull in WM/Desktop support. ------------------------- 2007-03-31 MG
 * 1) Initial creation. ----------------------------------- 2006-12-23 MG
 *
 */
#include "nx-widgets.h"
#include "widgets.h"
#include "application.h"

//#define OSD_DBG_MSG
#include "nc-err.h"

static int ModalRegister = 0;
static int ExitFlag = 0;

/**
 * Function puts widget layer into modal state.
 *
 * @param endOfModal
 *        flag to end modal state.
 * @para msgReturn
 *        pointer to modal state return value.
 * @return
 *        modal state return value.
 *
 */
int NeuxDoModal(int * endOfModal, int *msgReturn)
{
	GR_EVENT event;

	ModalRegister++;
	for (;;) {
		GrGetNextEvent(&event);
		NxEventResolveRoutine(&event);
		if (*endOfModal || ExitFlag)
			break;
	}
	ModalRegister--;

	ExitFlag = 0;
	if (msgReturn)
		return *msgReturn;
	return 0;
}

int NeuxIsModalActive()
{
	return ModalRegister ? 1 : 0;
}

void NeuxClearModalActive()
{
	ExitFlag = 1;
}
