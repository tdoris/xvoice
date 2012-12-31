/**
 * xvoice.h
 *
 * Description: program globals
 *
 * Copyright (c) 1999, David Z. Creemer.
 * See the LICENSE file. All rights not granted therein are reserved.
 *
 * @author David Z. Creemer
 * $Revision: 1.10 $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef _XVOICE_H
#define _XVOICE_H

#include "Voice.h"

extern char xvoicexml[];
extern void xvoice_init(VClient *cl, bool save);
extern void xvoice_status(const char *status_info);

#endif // _XVOICE_H
