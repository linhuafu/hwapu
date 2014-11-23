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
 * Neuros X events support routines.
 *
 * REVISION:
 *
 *
 * 1) Initial creation. ----------------------------------- 2006-05-19 EY
 *
 */

void
OnKeyDown(NX_WIDGET *, DATA_POINTER);
void
OnKeyUp(NX_WIDGET *, DATA_POINTER);
void
OnFocusIn(NX_WIDGET *, DATA_POINTER);
void
OnFocusOut(NX_WIDGET *, DATA_POINTER);
void
OnExposure(NX_WIDGET *, DATA_POINTER);
void
OnTimer(NX_WIDGET *, DATA_POINTER);
void
OnDestroy(NX_WIDGET *, DATA_POINTER);
void
OnChange(NX_WIDGET *, DATA_POINTER);
void
OnHotplug(NX_WIDGET *, DATA_POINTER);
void
OnSWOn(NX_WIDGET *, DATA_POINTER);
void
OnSWOff(NX_WIDGET *, DATA_POINTER);
