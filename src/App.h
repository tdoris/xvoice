/*
 * App.h
 *
 * Description: state machine for commanding/dictating to applications.
 *
 * Copyright (c) 1999, 2000, 2001, David Z. Creemer, Tom Doris, Brian Craft.
 * Deborah Kaplan, Jessica Hekman.
 * See the LICENSE file. All rights not granted therein are reserved.
 *
 * @author David Z. Creemer
 * @author Tom Doris
 * @author Brian Craft
 * @author Deborah Kaplan
 * @author Jessica Hekman

 * $Revision: 1.11 $
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

#ifndef _APP_H
#define _APP_H
#include "EventStream.h"
#include <string>

enum vCommandType {
  CCOMMAND = 0,
  CSTOPCOMMAND,
  CDICTATE,
  CSTOPDICTATE,
  CIDLE,
  CMICOFF,
  CCORRECTION,
  CREBUILD,
  CLASTITEM, /* last item with a voice command */
  CFOCUS,
  CMICON
};

int loadGrammars();
void vCommand(vCommandType t);
void appSetSystemState(char *, int);
EventRecord* appRecordMouse();
int appSendEventStream( EventStream* evs );
void initialState(void*);
void add_auto_grammar(const char *autog_expr, const char *gram_name);
void add_alwayson(std::string grammar_name, bool globallyOrJustWindows = true);
void remove_alwayson(std::string grammar_name);
void clear_alwayson();

#endif /* _APP_H */
