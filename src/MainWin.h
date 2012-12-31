// -*- C++ -*-

/**
 * MainWin.h
 *
 * Description: declaration of MainWindow object
 *
 * Copyright (c) 1999, David Z. Creemer.
 * Copyright (c) 2000, Brian Craft.
 * See the LICENSE file. All rights not granted therein are reserved.
 *
 * @author David Z. Creemer
 * @author Brian Craft

 * $Revision: 1.16 $
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

#ifndef _MAINWIN_H
#define _MAINWIN_H

#include "Target.h"

class MainWindow
{
  public:

    virtual ~MainWindow(){};

    // events
    virtual void errorMsg(int sev, const char* fn, const char* fmt, ...) = 0;

    // utility
    virtual void initVocabs() = 0;

    // targets
    virtual void setTarget(const char*) = 0;

    // the main loop
    virtual void main(bool, bool) = 0;
    virtual void installTimeout(int (*)(void*), int, void *) = 0;

    // status bar
    virtual void set_status(const char *status_info) { };
};

extern MainWindow* gMainWin;

#endif // _MAINWIN_H
