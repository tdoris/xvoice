// -*- C++ -*-

/**
 * Target.h
 *
 * Description: declaration of an event Target class
 *
 * Copyright (c) 1999, David Z. Creemer.
 * Copyright (c) 2000, Brian Craft.
 * See the LICENSE file. All rights not granted therein are reserved.
 *
 * @author David Z. Creemer
 * @author Brian Craft
 * $Revision: 1.19 $
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

/*
 * The Target class handles
 *      - sending events to targets
 *      - recording events on targets
 *      - keeping a list of running targets
 *      - finding the currently focused target
 */
#ifndef _TARGET_H_
#define _TARGET_H_

#include "x.h"
#include "Error.h"
#include <list>
#include <string>
#include "EventRecord.h"

class EventStream;
class Event;

/* 
 * this is basically a void pointer with
 * the ability to delete objects.
 * subclass & declare a destructor, and it will be
 * called when Target.cc destroys a Target pointing
 * to your data.
 */

class TargetData
{
  public:
    TargetData();
    virtual ~TargetData();
};

enum FocusEvent { TNONE, TFOCUS, TNAME };

class Target
{
  public:

    /*
     * constructor
     */
    Target();

    /*
     * destructor
     */
    virtual ~Target();

    /*
     * get the name of the target; return null if the target is invalid
     *
     * @return the name of the target window
     */
    const char *name() { return _curWin->name.c_str(); }

    /*
     * send the given C string to the target
     *
     * @param the string to send
     */

    int Target::sendText( const char* text );

    /*
     * send the given event stream to the target
     *
     * @param the stream to send
     */

    int sendEventStream( EventStream* evs );

    TargetData* getData() { return _curWin->data; }
    void setData(TargetData* d) { _curWin->data = d; }

    FocusEvent focusedTarget();
    void gc();

    EventRecord* recordMouse();

#ifdef DEBUG
    void dump_targs();
#endif

    class winInfo {             /* for tracking active targets */
      public:
        Window window;
        std::string name;
        TargetData* data;

        winInfo::winInfo(Window w, char const* n);
        winInfo::~winInfo();
        bool operator==(winInfo w) {
          return (w.window==window);
        }
    };
    typedef std::list<winInfo> winList;

    winList _actvWin;           /* active target list */
    winList::iterator _curWin;  /* current focused target in _actvWin */

    /* for maintaining _actvWin */
  protected:

    void sendEvent(XEvent* evt);
    void do_motion(int x, int y, char* wid, int origin);
    void do_button(int button, int action, bool alt, bool ctrl, bool shift);
    void do_modifiers(Event *ev);
    void do_modifiers(bool alt, bool ctrl, bool shift, bool press);
    void do_key(Event *ev);
    void do_setUserCase(Event *ev);
    void do_setUserSpace(Event *ev);
    bool do_find_and_focus(const char *expr);
    void do_command(const char *command);
};

bool root_prop_event(XEvent* ev, Target* targ);

#endif // _TARGET_H
