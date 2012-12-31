// -*- C++ -*-

/**
 * EventStream.h
 *
 * Description: declaration of an ordered list of X events
 *
 * Copyright (c) 1999, David Z. Creemer.
 * Copyright (c) 2000, Brian Craft.
 * Copyright (c) 2001, Jessica Perry Hekman.
 * See the LICENSE file. All rights not granted therein are reserved.
 *
 * @author David Z. Creemer
 * @author Brian Craft
 * @author Jessica Hekman
 * $Revision: 1.17 $
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

#ifndef _EVENTSTREAM_H_
#define _EVENTSTREAM_H_

#include <list>

enum EventType { EVKEY, EVMODIFIER, EVMOUSE, EVGRAM, EVCASE, EVSPACE, EVPAUSE, EVREPEAT, EVCALL, EVSTATUSBAR };

enum ButtonEvent {
  BUTTON_UP,
  BUTTON_DOWN,
  BUTTON_CLICK,
  BUTTON_DOUBLE_CLICK,
  BUTTON_TRIPLE_CLICK
};

enum KeyEvent {
  KEY_UP,
  KEY_DOWN,
  KEY_CLICK
};

enum CoordOrigin {
  ORG_WIN,
  ORG_ROOT,
  ORG_REL,
  ORG_GRID,
  ORG_WIDGET
};

class Event
{
  public:

    EventType type;
    union {
      struct {
        int action;
        unsigned short c;
      } key;
      struct {
        bool alt, ctrl, shift;
        int action;
      } modifier;
      struct {
        bool alt, ctrl, shift, motion;
        int x, y, origin, button, action;
        char *wid;
      } mouse;
      struct {
        bool enable;
        char *name;
      } grammar;
      struct {
        bool continuous, setUserCase;
        char *name;
      } setUserCase;
      struct {
        bool spacingEvent, continuous, setUserSpace;
        char *name;
      } setUserSpace;
      struct {
        bool pauseEvent;
	int duration;
      } pause;
      int repeat;
      struct {
	char *expr;
	char *command;
      } call;
      struct {
        char *value;
      } singleString;
    } val;

    /*
     * constructor
     */
    Event(int action0, unsigned short c0)
    {
      type = EVKEY;
      val.key.action = action0;
      val.key.c = c0;
    }
    Event(bool alt0, bool ctrl0, bool shift0, int action0)
    {
      type = EVMODIFIER;
      val.modifier.alt = alt0;
      val.modifier.ctrl = ctrl0;
      val.modifier.shift = shift0;
      val.modifier.action = action0;
    }
    Event(bool alt0, bool ctrl0, bool shift0, int x0, int y0, const char *wid0,
        int origin0, int button0, int action0, bool motion0)
    {
      type = EVMOUSE;
      val.mouse.alt = alt0;
      val.mouse.ctrl = ctrl0;
      val.mouse.shift = shift0;
      val.mouse.x = x0;
      val.mouse.y = y0;
      if (wid0 == NULL) val.mouse.wid = NULL;
      else {
        val.mouse.wid = new char[strlen(wid0)+1];
        strcpy(val.mouse.wid, wid0);
      }
      val.mouse.origin = origin0;
      val.mouse.button = button0;
      val.mouse.action = action0;
      val.mouse.motion = motion0;
    }
    Event(bool enable0, const char* name0)
    {
      type = EVGRAM;
      val.grammar.enable = enable0;
      val.grammar.name = new char[strlen(name0)+1];
      strcpy(val.grammar.name,name0);
    }
    Event(bool continuous0, bool UNUSED(setUserCase0), const char* name0)
    {
      type = EVCASE;
      val.setUserCase.continuous = continuous0;
      val.setUserCase.name = new char[strlen(name0)+1];
      strcpy(val.setUserCase.name,name0);
    }
    Event(bool UNUSED(spacingEvent0), bool continuous0, bool UNUSED(setUserCase0),
        const char* name0)
    {
      type = EVSPACE;
      val.setUserSpace.continuous = continuous0;
      val.setUserSpace.name = new char[strlen(name0)+1];
      strcpy(val.setUserSpace.name,name0);
    }
    Event(bool UNUSED(pause), int duration)
    {
      type = EVPAUSE;
      val.pause.duration = duration;
    }
    Event(int count)
    {
      type = EVREPEAT;
      val.repeat = count;
    }
    Event(const char *expr0, const char *command0)
    {
      type = EVCALL;
      val.call.expr = NULL;
      val.call.command = NULL;
      if (expr0) {
	val.call.expr = new char[strlen(expr0)+1];
	strcpy(val.call.expr, expr0); }
      if (command0) {
	val.call.command = new char[strlen(command0)+1];
	strcpy(val.call.command, command0); }
    }
    Event (EventType t, const char *statusValue) {
        type = t;
        val.singleString.value = strdup(statusValue);
    }
    Event(const Event& ev)
    {
      type = ev.type;
      if (type == EVGRAM) {
        val.grammar.name = new char[strlen(ev.val.grammar.name)+1];
        strcpy(val.grammar.name,ev.val.grammar.name);
        val.grammar.enable = ev.val.grammar.enable;
      } else if (type == EVCASE) {
        val.setUserCase.name = new char[strlen(ev.val.setUserCase.name)+1];
        strcpy(val.setUserCase.name,ev.val.setUserCase.name);
        val.setUserCase.continuous = ev.val.setUserCase.continuous;
      } else if (type == EVSPACE) {
        val.setUserSpace.name = new char[strlen(ev.val.setUserSpace.name)+1];
        strcpy(val.setUserSpace.name,ev.val.setUserSpace.name);
        val.setUserSpace.continuous = ev.val.setUserSpace.continuous;
      } else if (type == EVMOUSE) {
        val = ev.val;
        if (val.mouse.wid != NULL) {
          val.mouse.wid = new char[strlen(ev.val.mouse.wid)+1];
          strcpy(val.mouse.wid,ev.val.mouse.wid);
        }
      } else if (type == EVCALL) {
	val.call.expr = NULL;
	val.call.command = NULL;
	if (ev.val.call.expr) {
	  val.call.expr = new char[strlen(ev.val.call.expr)+1];
	  strcpy(val.call.expr, ev.val.call.expr); }
	if (ev.val.call.command) {
	  val.call.command = new char[strlen(ev.val.call.command)+1];
	  strcpy(val.call.command, ev.val.call.command); }
      } else if (type == EVSTATUSBAR) {
          val.singleString.value = strdup(ev.val.singleString.value);
      } else {
        val = ev.val;
      }
    }

    ~Event()
    {
      if (type == EVGRAM) delete val.grammar.name;
      else if (type == EVMOUSE && val.mouse.wid != NULL) delete val.mouse.wid;
    } 
};

class EventStream : public std::list<Event>
{
  public:
    /*
     * parse an XML attribute list for key events, and add the result
     * to the event stream
     *
     * @param attrs an attr=value pair array
     *
     * @return true if parse OK
     */
    bool parseKeyEvent( const char** attrs );

    /*
     * parse an XML attribute list for mouse button events, and add
     * the result to the event stream
     *
     * @param attrs an attr=value pair array
     *
     * @return true if parse OK
     */
    bool parseMouseEvent( const char** attrs );
    bool parseGrammarEvent( const char** attrs );
    bool parseSetCaseEvent( const char** attrs );
    bool parseSetSpacingEvent( const char** attrs );
    bool parsePauseEvent( const char** attrs );
    bool parseRepeatEvent( const char** attrs );
    bool parseCallEvent( const char** attrs );
    bool parseSetStatusEvent( const char** attrs );
};

#endif // _EVENTSTREAM_H_
