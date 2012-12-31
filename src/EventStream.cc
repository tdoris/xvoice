/*
 * EventStream.cc
 *
 * Description: implementation of an ordered list of X events
 *
 * Copyright (c) 1999, David Z. Creemer.
 * Copyright (c) 2000, Brian Craft.
 * Copyright (c) 2001, Jessica Perry Hekman.
 * See the LICENSE file. All rights not granted therein are reserved.
 *
 * @author David Z. Creemer
 * @author Brian Craft
 * @author Jessica Perry Hekman
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

#include "sys.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cctype>

#include "x.h"

using namespace std;

#include "MainWin.h"
#include "Error.h"
#include "EventStream.h"

struct attrTable {
  char const* name;
  int value;
};

static attrTable* findAttr(attrTable *t, const char *string)
{
  int i = 0;
  while (t[i].name != NULL) 
  {
    if (!strcasecmp(string, t[i].name)) return &t[i];
    ++i;
  }
  return NULL;
}

static struct attrTable kaction[] = {
  { "up",          KEY_UP },
  { "down",        KEY_DOWN },
  { "click",       KEY_CLICK },
  { NULL, -1 }
};

//Parse a single key in an event stream
bool EventStream::parseKeyEvent( const char** a )
{
  const char** attrs;
  int action = KEY_CLICK;
  bool alt = false, ctrl = false, shift = false;
  attrTable *at;

  /*
   * Check to see if this keyEvent has a modifier; it will
   * continue later to check the actual key.
   */
  attrs = a;
  while ( *attrs ) {
    const char* key = *attrs++;
    const char* value = *attrs++;

    if ( strcasecmp( key, "alt" ) == 0 ) {
      alt = ( strcasecmp( value, "true" ) == 0 );
    } else if ( strcasecmp( key, "control" ) == 0 ) {
      ctrl = ( strcasecmp( value, "true" ) == 0 );
    } else if ( strcasecmp( key, "shift" ) == 0 ) {
      shift = ( strcasecmp( value, "true" ) == 0 );
    } else if ( strcasecmp( key, "action" ) == 0 ) {
      at = findAttr(kaction , value);
      if (at != NULL) action = at->value;
    }
  }

  // Create an event to handle the modifiers: push those keys down
  // before sending the events for the character keys
  Event evModDown(alt, ctrl, shift, KEY_DOWN);
  push_back(evModDown);

  // Look for special (backslashed) characters
  attrs = a;
  unsigned short c = '\0';
  while ( *attrs ) {
    const char* key = *attrs++;
    const char* value = *attrs++;

    if ( strcasecmp( key, "char" ) == 0 ) {
      while (*value) {
        c = *value;

        /*
         * Translate escape sequences into hex codes.
         * TODO: can we handle \xxx here as well?
         * TODO: handle \n --> xFF0A + xFF0D?
         */
        switch (c) {
          case '\b':
            c = 0xFF08;
            break;
          case '\t':
            c = 0xFF09;
            break;
          case '\r':
            c = 0xFF0D;
            break;
          default:
            break;
        }

        /*
         * need to handle \t, \b, \r explicitly.
         * NOTE: this does not seem to generally be called;
         * backslashed characters show up as one character
         * at this point, not two. This should stay in here
         * for now in case there are circumstances when
         * backslashed chars do show up as two chars, and
         * should eventually be removed if possible.
         */
        if (c=='\\') {
          if (value[1] == 't') {
            c = 0xFF09;
          } else if (value[1]=='r') {
            c = 0xFF0D;
          } else if (value[1]=='b') {
            c = 0xFF08;
          } else if (isxdigit(value[1])) {
            char sym[5];
            strncpy(sym, value+1, 4);
            sym[4] = '\0';
            dbgprintf(("keysym %s\n", sym));
            /* keysym */
            c = strtoul(sym, NULL, 16);
            value += 3;
          } else {
            c=value[1];
          }
          value+=2;
        } else {
          ++value;
        }
        Event evKey(action, c);
        push_back(evKey);
      }
    }
  }

  // Release the modifier keys which we had previously pressed
  Event evModUp(alt, ctrl, shift, KEY_UP);
  push_back(evModUp);

  return true;
}

static struct attrTable baction[] = {
  { "up",          BUTTON_UP },
  { "down",        BUTTON_DOWN },
  { "click",       BUTTON_CLICK },
  { "double click",BUTTON_DOUBLE_CLICK },
  { "triple click",BUTTON_TRIPLE_CLICK },
  { NULL, -1 }
};

static struct attrTable borigin[] = { 
  { "window",   ORG_WIN },
  { "root",     ORG_ROOT },
  { "relative", ORG_REL },
  { "grid",     ORG_GRID },
  { "widget",   ORG_WIDGET },
  { NULL, -1 }
};

bool EventStream::parseMouseEvent( const char** attrs )
{
  int action = BUTTON_CLICK;
  int origin = ORG_WIN;
  int button = 0 ;
  int x = 0, y = 0;
  const char *wid = NULL;
  bool alt = false, ctrl = false, shift = false, motion = false;
  attrTable *at;

  while ( *attrs )
  {
    const char* key = *attrs++;
    const char* value = *attrs++;

    if ( strcasecmp( key, "button" ) == 0 ) {
      button = atoi( value );
    } else if ( strcasecmp( key, "x" ) == 0 ) {
      motion = true;
      x = atoi( value );
    } else if ( strcasecmp( key, "y" ) == 0 ) {
      motion = true;
      y = atoi( value );
    } else if ( strcasecmp( key, "wid" ) == 0 ) {
      motion = true;
      wid = value;
    } else if ( strcasecmp( key, "origin" ) == 0 ) {
      at = findAttr(borigin, value);
      if (at != NULL) origin = at->value;
    } else if ( strcasecmp( key, "action" ) == 0 ) {
      at = findAttr(baction, value);
      if (at != NULL) action = at->value;
    } else if ( strcasecmp( key, "alt" ) == 0 ) {
      alt = ( strcasecmp( value, "true" ) == 0 );
    } else if ( strcasecmp( key, "control" ) == 0 ) {
      ctrl = ( strcasecmp( value, "true" ) == 0 );
    } else if ( strcasecmp( key, "shift" ) == 0 ) {
      shift = ( strcasecmp( value, "true" ) == 0 );
    } else {
      LogMessage(E_CONFIG, "Unknown attribute: %s\n", key);
      return false;
    }
  }

  Event ev(alt, ctrl, shift, x, y, wid, origin, button, action, motion);
  push_back(ev);
  return true;
}


bool EventStream::parseGrammarEvent( const char** attrs )
{
  const char *name = NULL;
  bool enable = true;

  while ( *attrs ) {
    const char* key = *attrs++;
    const char* value = *attrs++;

    if ( strcasecmp( key, "name" ) == 0 ) {
      name = value;
    } else if ( strcasecmp( key, "action" ) == 0 ) {
      enable = ( strcasecmp( value, "on" ) == 0 );
      dbgprintf(("action %d\n", enable));
    }
  }
  if (name == NULL) return false;
  Event ev( enable, name );
  push_back(ev);
  return true;
}

bool EventStream::parseSetCaseEvent( const char** attrs )
{
  const char *name = NULL;
  bool continuous = false;

  while ( *attrs ) {
    const char* key = *attrs++;
    const char* value = *attrs++;

    if ( strcasecmp( key, "name" ) == 0 ) {
      name = value;
    } else if ( strcasecmp( key, "duration" ) == 0 ) {
      continuous = ( strcasecmp( value, "continuous" ) == 0 );
    }
  }
  if (name == NULL) { return false; }
  if (( strcasecmp( name, "Lowercase" )  != 0 ) &&
      ( strcasecmp( name, "Uppercase" )  != 0 ) &&
      ( strcasecmp( name, "TitleCase" )  != 0 ) &&
      ( strcasecmp( name, "None" )  != 0 ) &&
      ( strcasecmp( name, "Normal" ) != 0 )) { return false; }

  Event ev( continuous, true, name );
  push_back(ev);
  return true;
}

bool EventStream::parseSetSpacingEvent( const char** attrs )
{
  const char *name = NULL;
  bool continuous = false;

  while ( *attrs ) {
    const char* key = *attrs++;
    const char* value = *attrs++;

    /*
     * FIXME eventually don't set by name (a string) but by
     * an int which tells us how many spaces. Note that this
     * will screw up some algorithms in App.cc/dictationHandler.
     */

    if ( strcasecmp( key, "name" ) == 0 ) {
      name = value;
    } else if ( strcasecmp( key, "duration" ) == 0 ) {
      continuous = ( strcasecmp( value, "continuous" ) == 0 );
    }
  }
  if (name == NULL) { return false; }
  if (( strcasecmp( name, "no-space" )  != 0 ) &&
      ( strcasecmp( name, "normal" ) != 0 )) { return false; }


  Event ev( true, continuous, true, name );
  push_back(ev);
  return true;
}

bool EventStream::parsePauseEvent( const char** attrs )
{
  int duration = 1;

  while ( *attrs ) {
    const char* key = *attrs++;
    const char* value = *attrs++;

    if ( strcasecmp( key, "duration" ) == 0 ) {
      // if value isn't an integer, will default to 0
      duration = atoi( value );
    }
  }

  Event ev( true, duration );
  push_back(ev);
  return true;
}

bool EventStream::parseRepeatEvent( const char** attrs )
{
  int count = 0;

  while ( *attrs ) {
    const char* key = *attrs++;
    const char* value = *attrs++;

    if ( strcasecmp( key, "count" ) == 0 ) {
      count = atoi(value);
    }
  }
  Event ev(count);
  push_back(ev);
  return true;
}

bool EventStream::parseCallEvent( const char** attrs )
{
  const char *expr = NULL;
  const char *command = NULL;

  while ( *attrs ) {
    const char* key = *attrs++;
    const char* value = *attrs++;

    if ( strcasecmp( key, "expr" ) == 0 ) {
      expr = value;
    } else if ( strcasecmp( key, "command" ) == 0 ) {
      command = value;
    }
  }
  if (!expr && !command) return false;
  Event ev( expr, command );
  push_back(ev);
  return true;
}

bool EventStream::parseSetStatusEvent( const char** attrs )
{
  const char *statusValue = NULL;

  while ( *attrs ) {
    const char* key = *attrs++;
    const char* value = *attrs++;

    if ( strcasecmp( key, "value" ) == 0 ) {
        statusValue = value;
    }
  }

  if (statusValue == NULL) { return false; }

  Event ev( EVSTATUSBAR, statusValue );
  push_back(ev);
  return true;
}
