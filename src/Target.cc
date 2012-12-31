/*
 * Target.cc
 *
 * Description: implementation of an event Target class
 *
 * Copyright (c) 1999, David Z. Creemer.
 * Copyright (c) 2000, Brian Craft.
 * Copyright (c) 2001, Jessica Perry Hekman.
 * See the LICENSE file. All rights not granted therein are reserved.
 *
 * @author David Z. Creemer
 * @author Brian Craft
 * @author Jessica Perry Hekman
 * $Revision: 1.32 $
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
#include <cstdio>
#include "x.h"
#include "xvoice.h"
#include <X11/extensions/XTest.h>
#include <sys/types.h>
#include <regex.h>
#include <gnome.h>

using namespace std;

#include "EventStream.h"
#include "Target.h"
#include "Error.h"

extern Display *display;

static unsigned char xerror;
static int (*old_handler)(Display *, XErrorEvent *);

int xerror_handler(Display* UNUSED(d), XErrorEvent *ev)
{
  xerror = ev->error_code;
  return True;
}

static void set_trap()
{
  xerror = 0;
  old_handler = XSetErrorHandler(xerror_handler);
}

static int remove_trap()
{
  XSetErrorHandler(old_handler);
  return xerror;
}

static KeyCode ShiftCode = 0;
static KeyCode LockCode;
static KeyCode CtrlCode;
static KeyCode AltCode;

static void find_mods(Display* d)
{
  XModifierKeymap* map;

  map = XGetModifierMapping(d);
  ShiftCode = map->modifiermap[0];
  LockCode = map->modifiermap[map->max_keypermod];
  CtrlCode = map->modifiermap[map->max_keypermod*2];
  AltCode = map->modifiermap[map->max_keypermod*3];
}

TargetData::TargetData()
{
}

TargetData::~TargetData()
{
}

/*
 * constructor
 */
Target::Target()
{
}

/*
 * destructor
 */
Target::~Target()
{
}

Target::winInfo::winInfo(Window w, char const* n)
    : window(w), name(n), data(0)
{
  dbgprintf(("win %x name %s\n", window, name.c_str()));
}

Target::winInfo::~winInfo()
{
  if (data != NULL)
    delete data;
}

void Target::sendEvent(XEvent* evt)
{
  evt->xany.window = _curWin->window;
  // send the event
  XSendEvent( display, _curWin->window, True, 0xfff, evt );
}

/*
 * send the given C string to the target
 *
 * @param the string to send
 */

int Target::sendText(const char* text)
{
  const char* ev[3];

  ev[0] = "char";
  ev[1] = text;
  ev[2] = NULL;

  EventStream es;
  es.parseKeyEvent(ev);
  sendEventStream(&es);

  return 0;
}


#if 0
static void dumpXerror(int code)
{
  char buff[100];
  XGetErrorText(display, code, buff, 100);
  dbgprintf(("X error: %s\n", buff));
}
#endif

static bool find_window(Display *d, Window w, char* wid, int* xp, int* yp)
{
  Window parent, root, *children, child;
  unsigned int i, num, dx, dy, border, bpp;
  int x, y, inc;
  char *c = wid;

  dbgprintf(("window %x\n", w));
  while (*c == ':') {
    ++c;
    sscanf(c, "%d%n", &i, &inc);
    dbgprintf(("c %s i %d\n", c, i));
    c += inc;
    if (XQueryTree(d, w, &root, &parent, &children, &num)
        && children != NULL && i < num) {
      w = children[i];
      dbgprintf(("\t%x\n", w));
      XFree(children);
    } else {
      return false;
    }
  }
  if (!XGetGeometry(d, w, &root, &x, &y, &dx, &dy, &border, &bpp))
    return false;
  XTranslateCoordinates(d, w, root, dx/2, dy/2, xp, yp, &child);
  dbgprintf(("x %d y %d\n", *xp, *yp));

  return true;
}

void Target::do_motion(int x, int y, char* wid, int origin)
{
  int newx, newy;
  unsigned int dx, dy, border, bpp;
  Window root, child;
  switch (origin) {
    case ORG_WIN:
      /* We don't really need this since we only use root */
      XGetGeometry(display, _curWin->window, &root, &newx, &newy, &dx, &dy,
          &border, &bpp);
      XTranslateCoordinates(display, _curWin->window, root,
          x, y, &newx, &newy, &child);
      dbgprintf(("evx %d evy %d x %d y %d\n", x,
            y, newx, newy));
      XTestFakeMotionEvent(display, 0, newx, newy, 0);
      break;
    case ORG_ROOT:
      newx = x;
      newy = y;
      XTestFakeMotionEvent(display, 0, newx, newy, 0);
      break;
    case ORG_REL:
      XWarpPointer(display, None, None, 0, 0, 0, 0, x, y);
      break;
    case ORG_GRID:
      dbgprintf(("grid not implemented\n"));
      break;
    case ORG_WIDGET:
      dbgprintf(("widget\n"));
      if (find_window(display, _curWin->window, wid, &newx, &newy)) {
        XTestFakeMotionEvent(display, 0, newx, newy, 0);
      }
      break;
    default:
      dbgprintf(("bad motion origin\n"));
      break;
  }

  XSync( display, false );
}

void Target::do_modifiers(Event *ev)
{
  if (ShiftCode == 0) find_mods(display);

  if (ev->val.modifier.alt) {
    if (ev->val.modifier.action != KEY_UP)
      XTestFakeKeyEvent(display, AltCode, true, 0);
    if (ev->val.modifier.action != KEY_DOWN)
      XTestFakeKeyEvent(display, AltCode, false, 0);
  }

  if (ev->val.modifier.ctrl) {
    if (ev->val.modifier.action != KEY_UP)
      XTestFakeKeyEvent(display, CtrlCode, true, 0);
    if (ev->val.modifier.action != KEY_DOWN)
      XTestFakeKeyEvent(display, CtrlCode, false, 0);
  }

  if (ev->val.modifier.shift) {
    if (ev->val.modifier.action != KEY_UP)
      XTestFakeKeyEvent(display, ShiftCode, true, 0);
    if (ev->val.modifier.action != KEY_DOWN)
      XTestFakeKeyEvent(display, ShiftCode, false, 0);
  }

  XSync( display, false );
}

void Target::do_modifiers(bool alt, bool ctrl, bool shift, bool press)
{
  if (ShiftCode == 0) find_mods(display);

  if (alt) {
    XTestFakeKeyEvent(display, AltCode, press, 0);
  }

  if (ctrl) {
    XTestFakeKeyEvent(display, CtrlCode, press, 0);
  }

  if (shift) {
    XTestFakeKeyEvent(display, ShiftCode, press, 0);
  }
}

void Target::do_key(Event *ev)
{
  gboolean  def;
  if (gnome_config_get_bool_with_default
      ("xvoice/Dictation/SlowKeys=false", &def)) {
    usleep(1);
  }
  KeyCode c = XKeysymToKeycode(display, ev->val.key.c);
  bool shift = false;

  if (XKeycodeToKeysym(display, c, 0) == ev->val.key.c) {
    /* check usual case first & do nothing */
  } else if (XKeycodeToKeysym(display, c, 1) == ev->val.key.c) {
    shift = true;
  } else {
    dbgprintf(("bad keysym %x\n, ev->val.key.c"));
    return;
  }

  if (shift)
    XTestFakeKeyEvent(display, ShiftCode, true, 0);

  if (ev->val.key.action != KEY_UP)
    XTestFakeKeyEvent(display, c, true, 0);
  if (ev->val.key.action != KEY_DOWN)
    XTestFakeKeyEvent(display, c, false, 0);

  if (shift)
    XTestFakeKeyEvent(display, ShiftCode, false, 0);

  XSync( display, false );
}

void Target::do_button(int button, int action, bool alt, bool ctrl, bool shift)
{
  do_modifiers(alt, ctrl, shift, true);
  if (action != BUTTON_UP)
    XTestFakeButtonEvent(display, button, true, 0);
  if (action != BUTTON_DOWN)
    XTestFakeButtonEvent(display, button, false, 0);
  if (action == BUTTON_DOUBLE_CLICK || action == BUTTON_TRIPLE_CLICK) {
    XTestFakeButtonEvent(display, button, true, 0);
    XTestFakeButtonEvent(display, button, false, 0);
  }
  if (action == BUTTON_TRIPLE_CLICK) {
    XTestFakeButtonEvent(display, button, true, 0);
    XTestFakeButtonEvent(display, button, false, 0);
  }
  do_modifiers(alt, ctrl, shift, false);
}

/* Search recursively through the window tree
   for a window whose name matches the regular expression */

static Window find_window_by_name(Display* display, Window w, regex_t* reg) {
  char *name;
  Window *children;
  unsigned int n_children;
  Window tw;

  if (XQueryTree(display, w, &tw, &tw, &children, &n_children) == 0) {
    dbgprintf(("XQueryTree failed"));
    return 0; }

  while (n_children-- > 0) {
    w = children[n_children];

    if (   XFetchName(display, w, &name) != 0
	&& regexec(reg, name, 0, NULL, 0) == 0) {
      XFree(children);
      return w; }

    if ((w = find_window_by_name(display, w, reg)) != 0) {
      XFree(children);
      return w; }
  }

  XFree(children);
  return 0;
}


bool Target::do_find_and_focus(const char *expr) {
  regex_t reg;
  Window w;

  if (expr == NULL)
    return false;

  if (regcomp(&reg, expr, REG_EXTENDED|REG_NOSUB) != 0) {
    dbgprintf(("regcomp failed"));
    return false;
  }

  // FIXME: only finds first with matching name (is this a potential problem?)
  if ((w = find_window_by_name(display, DefaultRootWindow(display), &reg))
      != 0) {
    if (XMapRaised(display, w) != 0)
      dbgprintf(("XMapRaised failed\n"));
    if (XSetInputFocus(display, w, RevertToPointerRoot, CurrentTime) != 0)
      dbgprintf(("XSetInputFocus failed\n"));

    regfree(&reg);
    return true; }

  regfree(&reg);
  return false;
}


void Target::do_command(const char *command) {
  if (command != NULL && fork() == 0) {
    // BUG: drop privileges.  Is this correct or necessary??
    setuid(getuid());
    setgid(getgid());

    dbgprintf((": %s", command));
    execl("/bin/sh", "sh", "-c", command, NULL);
    exit(0); }

  XSync( display, false );
}


int Target::sendEventStream(EventStream* es)
{
  XSync(display, false);
  set_trap();
  dbgprintf(("target %s\n",_curWin->name.c_str()));
  bool more = true;
  while (!es->empty() && more) {
    Event ev = es->front();
    dbgprintf(("len %d\n", es->size()));
    switch (ev.type) {
      case EVKEY:
        dbgprintf(("key event: %x\n", ev.val.key.c));
        do_key(&ev);
        es->pop_front();
        break;
      case EVMODIFIER:
        dbgprintf(("modifier event: %d %d %d\n", ev.val.modifier.alt,
		   ev.val.modifier.ctrl, ev.val.modifier.shift));
        do_modifiers(&ev);
        es->pop_front();
        break;
      case EVMOUSE:
        dbgprintf(("mouse event\n"));
        if (ev.val.mouse.motion) {
          dbgprintf(("doing motion\n"));
          do_motion(ev.val.mouse.x, ev.val.mouse.y, ev.val.mouse.wid,
              ev.val.mouse.origin);
        }
        if (ev.val.mouse.button) {
          dbgprintf(("doing button\n"));
          do_button(ev.val.mouse.button, ev.val.mouse.action,
              ev.val.mouse.alt, ev.val.mouse.ctrl, ev.val.mouse.shift);
        }
        es->pop_front();
        break;
      case EVCALL:
	dbgprintf(("call expr %s command %s\n",
		   ev.val.call.expr? ev.val.call.expr : "NULL",
		   ev.val.call.command? ev.val.call.command : "NULL"));
	if (!do_find_and_focus(ev.val.call.expr))
	  do_command(ev.val.call.command);
	es->pop_front();
	break;
      default:
        more = false;// no more target events
        break;
    }
  }

#if 0
dumpXerror(xerror);
#endif

  if (remove_trap() == BadWindow ) {
    dbgprintf(("BadWindow\n",_curWin->name.c_str()));
    return -1;
  }
  return 0;
}

/*
 * Garbage collect various bits of state on this Target object --
 * currently, just the active windows list.
 */
void Target::gc()
{
  /*
   * Walk through the list of active windows. For each one, attempt to
   * call an X function on it (we use XFetchName, but any would do). If
   * it throws a BadWindow error, that window no longer exists, so
   * remove it from the list of active windows.
   */

  winList::iterator win;
  int j;
  char *n;

  for (win = _actvWin.begin(); win != _actvWin.end(); ++win) {
    if (win == _curWin) continue; /* otherwise we segfault our clients */
    set_trap();
    j = XFetchName( display, win->window, &n );
    /*
     * This next condition used to be remove_trap() == BadWindow || !j,
     * but some focused windows don't have a name themselves
     * (although their parents do) and so return j=0.  If this change
     * ends up being a problem, one can simply swap in a different
     * X function and revert to the old conditional.
     */
    if (remove_trap() == BadWindow) {
      dbgprintf(("removing window from active window list\n"));
      win = _actvWin.erase(win);
    } else {
        if (n != NULL) XFree(n);
    }
  }
#ifdef DEBUG
  dump_targs();
#endif
}

typedef list<Window> path;
static path* find_pointer_window(Display* d, Window wr)
{
  int x, y, wx, wy;
  unsigned int mask;
  Window window, root, child = wr;
  path *wp = new path;

  set_trap();
  while (child != None) {
    window = child;
    wp->push_back(window);
    dbgprintf(("window %x\n", window));
    XQueryPointer(d, window, &root, &child, &x, &y, &wx, &wy, &mask);
  }
  if (remove_trap() != 0) {
    delete wp;
    return NULL;
  }

  return wp;
}

static fstring* window_path(Display* d, path* wp)
{
  fstring *str = new fstring;
  Window parent, root, *children;
  unsigned int i, num;

  while (wp->size() > 1) {
    if (XQueryTree(d, wp->front(), &root, &parent, &children, &num)
        && children != NULL) {
      wp->pop_front();
      for (i = 0; i < num; ++i) {
        dbgprintf(("\t%x\n", children[i]));
        if (children[i] == wp->front()) {
          str->appendf(":%d", i);
          break;
        }
      }
      XFree(children);
      if (i == num) {
        delete str;
        return NULL;
      }
    } else {
      delete str;
      return NULL;
    }
  }
  return str;
}

EventRecord* Target::recordMouse()
{
  Window window, root;
  XEvent ev;
  EventRecord *er;
  int x, y;
  unsigned int w, h, border, bpp;
  dbgprintf(("grabbing pointer...\n"));

  set_trap();
  window = _curWin->window;
  XGetGeometry(display, window, &root, &x, &y, &w, &h, &border, &bpp);
  XGrabPointer(display, window, False, ButtonPressMask | ButtonReleaseMask,
      GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
  if (remove_trap() != 0) {
    set_trap();
    window = DefaultRootWindow(display);
    XGrabPointer(display, window, False, ButtonPressMask | ButtonReleaseMask,
        GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
    if (remove_trap() != 0) return NULL;
  }

  er = new EventRecord;
  if (window == _curWin->window) {
    er->window = true;
  } else {
    er->window = false;
  }


  bool waiting = true;
  dbgprintf(("entering loop...\n"));
  while (waiting) {
    XMaskEvent(display, ButtonPressMask | ButtonReleaseMask, &ev);
    switch (ev.type) {
      case ButtonPress:
        dbgprintf(("got press...\n"));
        er->x = ev.xbutton.x;
        er->y = ev.xbutton.y;
        er->x_root = ev.xbutton.x_root;
        er->y_root = ev.xbutton.y_root;
        er->button = ev.xbutton.button;
        break;
      case ButtonRelease:
        dbgprintf(("got release...\n"));
        XUngrabPointer(display, CurrentTime);
        XFlush(display);
        waiting = false;
        break;
      default:
        dbgprintf(("got something i don't know...\n"));
        break;
    }
  }
  path *wp = find_pointer_window(display, window);
  if (wp != NULL) {
    dbgprintf(("valid widget\n"));
    er->widget = window_path(display, wp);
    delete wp;
  } else er->widget = NULL;

  if (er->x > x && er->x < x + (int)w
      && er->y > y && er->y < y + (int)h) er->owner = true;
  else er->owner = false;
  return er;
}

#ifdef DEBUG
void Target::dump_targs()
{
  winList::iterator win;
  for (win = _actvWin.begin(); win != _actvWin.end(); win++)
    dbgprintf(("win %x name %s\n", win->window, win->name.c_str()));
}
#endif

/*
 * Check focus window id and name. Update _curWin and _actvWin if appropriate.
 * Return TFOCUS if the focus window has changed, TNAME if the name has
 * changed, TNONE if neither has changed.
 *
 * XGetInputFocus() may or may not give a window with which XFetchName works;
 * see below.
 *
 * XFetchName can return NULL, if the WM_NAME property is not set. We map this
 * to "".
 *
 * If the window dies between these two calls, we pretend it didn't happen and
 * wait for the next timeout to get the new focus window.
 */

FocusEvent Target::focusedTarget()
{
  Window wx;
  winList::iterator win;
  FocusEvent ret;
  int i;
  Window parent_win;
  Window root_win;
  Window *dummy_child_list;
  Window w;
  unsigned int dummy_n_children;
  string win_name;
  char *n;

  ret = TNONE;

  XGetInputFocus(display,&wx,&i); /* wx == the current focused window
				     in the following */
  /* XGetInputFocus returns RevertToPointerRoot for most windows
   * but RevertToParent for GTK+ 2 windows.  In the latter case
   * the window name is given by a parent window.  We look up the
   * tree to find a window title below.
   */

  for (win = _actvWin.begin(); win != _actvWin.end(); ++win)
    if (win->window == wx) break;

  if (win == _actvWin.end()) {  /* new window, add to list */
    _actvWin.push_back(winInfo(wx, ""));
    win = _actvWin.end();
    --win;
  }

  if (_curWin != win)
    ret = TFOCUS;

  /* When we first start up, under some window managers _curWin ends
     up being null, and under others it doesn't. I don't fully
     understand what is going on, but explicitly setting _curWin here
     does seem to fix things. --jphekman */
  if (_curWin == NULL) {
    _curWin = win;
  }

  // proceed to get first valid window name of nearest parent
  root_win = 0;
  win_name = "";
  for (w = wx; root_win == 0 || w != root_win; w = parent_win)
    {
      dbgprintf(("window=%u\n", w));
      set_trap();
      i = XFetchName(display, w, &n);
      dbgprintf(("i=%d\n", i));
      if (remove_trap() == BadWindow)
	{
	  dbgprintf(("BadWindow; ignoring\n"));
	  /*
	   * Wait for next timeout. Let bad window be gc'd.
	   */
	  ret = TNONE;
	  break;
	}
      else
	{
	  if (i) // XFetchName succeeded
	    {
	      if (n) // yeah! actual name
		{
		  win_name=n;
		  XFree(n);
		}
	      break;
	    }
	}
      XQueryTree(display, w, &root_win, &parent_win, &dummy_child_list,
		 &dummy_n_children);
    }
  // win_name is now set

  if (i) // last XFetchName succeeded
    {
      if (win->name != win_name) // update name, as it has changed
	{
	  if (ret != TFOCUS)
	    {
	      ret = TNAME;
	    }
	  win->name = win_name;
	}
      // we got some sort of good window, so we can change "focus"
      _curWin = win;
    }
  else // bad window
    {
      ret = TNONE;
    }
  return ret;
}
