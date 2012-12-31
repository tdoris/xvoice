// -*- C++ -*-

/**
 * ParseEventStream.h
 *
 * Description: parse an event stream from XML
 *
 * Copyright (c) 1999, David Z. Creemer.
 * Copyright (c) 2000, Brian Craft.
 * Copyright (c) 2001, Jessica Perry Hekman.
 * See the LICENSE file. All rights not granted therein are reserved.
 *
 * @author David Z. Creemer
 * @author Brian Craft
 * @author Jessica Perry Hekman
 * $Revision: 1.12 $
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

#ifndef _PARSEEVENTSTREAM_H_
#define _PARSEEVENTSTREAM_H_

#include "EventStream.h"

class application{
  public:
    char* name;
    char* expr;
    char *dictation;
    char *before;
    application(const char* n, const char* e, const char* d, const char *b = "",
                const char *eo = ""){
      name=new char[strlen(n)+1];
      strcpy(name,n);
      expr=new char[strlen(e)+1];
      strcpy(expr,e);
      dictation=new char[strlen(d)+1];
      strcpy(dictation,d);
      before=new char[strlen(b)+1];
      strcpy(before,b);
    }
    ~application() {
      delete name;
      delete expr;
      delete dictation;
      delete before;
    }

};
typedef std::list<application> applicationList;

applicationList* parseFile( const char* filename, const char* grammardir,
                            applicationList* es = NULL );
EventStream* parseBuff( const char* bytes);

#endif // _PARSEEVENTSTREAM_H_
