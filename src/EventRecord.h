/*
 * This file is part of xvoice, a voice control app for the linux
 * desktop.
 * Copyright (C) 2002 Brian Craft (bcboy@thecraftstudio.com)
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _EVENTRECORD_H
#define _EVENTRECORD_H
#include "Error.h"

/*
 * Struct for recording input macros
 *
 * Note that this is unrelated to EvenStream.
 */
struct EventRecord {
  bool window;    /* true if there is an active window */
  int x, y;       /* only valid if window == true */
  int x_root, y_root;
  unsigned int button;
  bool owner;     /* true if event is inside this target */
  fstring *widget;
};
typedef struct EventRecord EventRecord;

#endif /* _EVENTRECORD_H */
