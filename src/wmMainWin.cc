/*
 * wmMainWin.cc
 *
 * Description: dock app implementation of GUI main window
 *    Derived from wmcb, by Jani Frilander.
 *
 * Copyright (c) 2000, Brian Craft.
 * See the LICENSE file. All rights not granted therein are reserved.
 *
 * @author Brian Craft
 * $Revision: 1.15 $
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
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <stdarg.h>

#include <X11/xpm.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "wmgeneral.h"
#include "wmvoice-master.xpm"

using namespace std;

#include "Voice.h"
#include "MainWin.h"
#include "Error.h"
#include "App.h"
#include "EventStream.h"
#include "ParseEventStream.h"
#include "xvoice.h"
#include "mouseGrid.h"

#include <list>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <popt.h>

#define MASK_SIZE 64
#define WMVOICE_VERSION "0.1"

struct xpm_position
{
  int x, y;
};

struct xpm_surface
{
  int x, y, w, h;
};

struct xpm_position pos[]=
{
  {5,48}
};

struct xpm_surface rel[]=
{
  {12,64,12,11}
};

struct xpm_surface prs[]=
{
  {0,64,12,11}
};

struct xpm_surface cb={5,5,54,39}; /* reco text */
struct xpm_surface bn={21,48,38,11}; /* grammar */
extern XpmIcon wmgen;

GC norm_gc,inv_gc,weak_gc;
Time wmvoice_stamp;
XFontStruct *font_info;
XFontStruct *bn_font;
char wmvoice_mask_bits[64*64];
int region = -1;
XColor color1,color2,color3;

typedef int (*tofn)(void*);

class wmMainWindow : public MainWindow
{
  public:

    wmMainWindow();
    ~wmMainWindow(){};

    void mic(int);
    void reco(const char *text, bool firm);
    void errorMsg(int sev, const char* fn, const char* fmt, ...);
    void vocab(const char *name, bool active);
    void initVocabs();
    void setTarget(char const*) { };

    void wmMainWindow::main(bool save, bool mic_on);
    void installTimeout(tofn, int, void*);

    friend MainWindow* getMainWindow(int argc, char **argv,
        const struct poptOption*);

  protected:

    class timeout {
      public:
        void *data;
        tofn fn;
        int len;
        struct timeval last;

        timeout::timeout(tofn, int, void *);
        timeout::~timeout(){};
    };

    typedef list<timeout> timeoutList;

    timeoutList timeouts;

    void checkTimeouts();

    void button_press(int b);
    void draw_vocab();
    bool mic_on;
};

wmMainWindow::wmMainWindow():
  mic_on(false)
{
}

static void* vv_data;
static int (*vv_fn)(void *);
static int vv_socket;

void notify (int socket_handle, int (*recv_fn)(void*),
    void* recv_data, void* UNUSED(client_data))
{
  vv_data = recv_data;
  vv_fn = (int (*)(void*))recv_fn;
  vv_socket = socket_handle;
  return;
}

void wmMainWindow::initVocabs()
{
  initGrid(display, Root, this);
}

void wmMainWindow::mic(int state)
{
  mic_on = state;
  draw_vocab();
}

static void init(int argc, char *argv[], const struct poptOption* xvopt)
{
  XGCValues my_gcv;
  unsigned long my_gcm;
  XWindowAttributes attributes;
  Font f;
  char const* font_name="6x13";
  static const struct poptOption options[] = {
    {"font", 'f',  POPT_ARG_STRING,        &font_name, 0, "Set font", "FONT" },
    {NULL,   '\0', POPT_ARG_INCLUDE_TABLE, (void*)xvopt,      0, NULL,       NULL   },
    {NULL, '\0', 0, NULL, 0, NULL, NULL}
  };

  createXBMfromXPM(wmvoice_mask_bits,wmvoice_master_xpm,MASK_SIZE,
      MASK_SIZE);
  openXwindow(argc,argv,options,wmvoice_master_xpm,wmvoice_mask_bits,MASK_SIZE,
      MASK_SIZE);

  XSelectInput(display, Root, StructureNotifyMask);

  /* this uses an array in case we want more buttons later */
  AddMouseRegion(0,pos[0].x,pos[0].y,pos[0].x+prs[0].w,pos[0].y+prs[0].h);
  /*AddMouseRegion(1, cb.x, cb.y, cb.w, cb.h); text area */

  XGetWindowAttributes(display, Root, &attributes);

  my_gcm = GCForeground | GCBackground | GCFont; 

  color1.red = 0x2000;
  color1.green = 0x2000;
  color1.blue = 0x2000;
  color1.pixel = 0;
  if (!XAllocColor(display, attributes.colormap, &color1)) 
  {  
    fprintf(stderr, "wmvoice.app: can't allocate color.\n");
  }

  color2.red = 0x2000;
  color2.green = 0xb200;
  color2.blue = 0xae00;
  color2.pixel = 0;
  if (!XAllocColor(display, attributes.colormap, &color2)) 
  {  
    fprintf(stderr, "wmvoice.app: can't allocate color.\n");
  }

  color3.red = 0xaf00;
  color3.green = 0x2400;
  color3.blue = 0x0500;
  color3.pixel = 0;
  if (!XAllocColor(display, attributes.colormap, &color3)) 
  {  
    fprintf(stderr, "wmvoice.app: can't allocate color.\n");
  }

  f=XLoadFont(display, font_name);
  font_info = XQueryFont(display,f);

  my_gcv.background = color1.pixel;
  my_gcv.foreground = color2.pixel;
  my_gcv.font = font_info->fid;

  norm_gc = XCreateGC(display, Root, my_gcm, &my_gcv);

  my_gcv.background = color2.pixel;
  my_gcv.foreground = color1.pixel;
  my_gcv.font = font_info->fid;

  inv_gc = XCreateGC(display, Root, my_gcm, &my_gcv);

  my_gcv.background = color1.pixel;
  my_gcv.foreground = color3.pixel;
  my_gcv.font = font_info->fid;

  weak_gc = XCreateGC(display, Root, my_gcm, &my_gcv);    
}

int num_chars(const char* pos, int width, char* out)
{
  int i = 0,j = 0;
  while (pos[i]) {
    out[i] = pos[i];
    out[i+1] = '\0';
    if ((j=XTextWidth(font_info, out, i+1)) > width) {
      dbgprintf(("width %d cols %d\n", j, width));
      out[i] = '\0';
      return i;
    }
    ++i;
  }
  return i;
}

static void draw_string(const char* str, GC fg, GC bg)
{
  const char *pos,*ppos;
  int n_bytes = strlen(str);
  int y=0;
  int len;
  int rows,height;
  char out[100]; /* should be big enough for any font */

  height = font_info->ascent + font_info->descent;
  rows = cb.h/height;
  pos=ppos=str;

  XFillRectangle(display,wmgen.pixmap,bg,cb.x,cb.y,cb.w,cb.h);

  y = 0;
  for (pos = str; y<rows && pos < str + n_bytes; pos += len) {
    len = num_chars(pos, cb.w, out);

    XDrawString(display,wmgen.pixmap,fg,cb.x,
        cb.y+(height*y)+font_info->ascent,pos,len);
    ++y;
  }
  RedrawWindow();
}

void wmMainWindow::reco(const char* str, bool firm)
{
  draw_string(str, firm?norm_gc:weak_gc, inv_gc);
}

void wmMainWindow::errorMsg(int UNUSED(sev), char const* UNUSED(fn), const char* fmt, ...)
{
  fstring msg;
  va_list ap;
  va_start(ap, fmt);
  msg.vappendf(fmt, ap);
  draw_string(msg.c_str(), inv_gc, norm_gc);
  va_end(ap);
}

static list<string> vocabs;

void wmMainWindow::draw_vocab()
{
  const char* str;

  if (!mic_on) {
    str = "-off-";
  } else if (vocabs.size() > 0) {
    str = vocabs.back().c_str();
  } else {
    str = "-on-";
  }

  XFillRectangle(display,wmgen.pixmap,inv_gc,bn.x,bn.y,bn.w,bn.h);
  if (vocabs.size() > 0) {
    dbgprintf(("%s\n", vocabs.back().c_str()));
    XDrawString(display, wmgen.pixmap, norm_gc, bn.x,
        bn.y+font_info->ascent-1, str, strlen(str));
  }
  RedrawWindow();
}

void wmMainWindow::vocab(const char *name, bool active)
{
  string str(name);
  if (active) {
    vocabs.push_back(str);
  } else {
    vocabs.remove(str);
  }
  draw_vocab();
}

void wmMainWindow::button_press(int b)
{
  if(!(b<0)) {
    copyXPMArea(prs[b].x,prs[b].y,prs[b].w,prs[b].h,pos[b].x,pos[b].y);
    if (mic_on) {
      vCommand(CMICOFF);
    } else {
      vCommand(CMICON);
    }
  }
}


MainWindow* getMainWindow(int argc, char **argv, const struct poptOption* opt)
{
  init(argc,argv,opt);
  wmMainWindow* mw = new wmMainWindow();

  XWindowAttributes attr;
  XGetWindowAttributes(display, DefaultRootWindow(display), &attr);
  XSelectInput(display, DefaultRootWindow(display), 
      attr.your_event_mask | PropertyChangeMask);

  return mw;
}

void wmMainWindow::checkTimeouts()
{
  struct timeval now;
  timeoutList::iterator i;  /* current focused target in _actvWin */

  for (i = timeouts.begin(); i != timeouts.end(); ++i) {
    gettimeofday(&now, NULL);
    if ((now.tv_sec-i->last.tv_sec)*1000+
        now.tv_usec-i->last.tv_usec > i->len) {
      i->fn(i->data);
      i->last.tv_sec = now.tv_sec;
      i->last.tv_usec = now.tv_usec;
    }
  }
}

wmMainWindow::timeout::timeout(tofn f, int l, void *d) :
    data(d), fn(f), len(l)
{
  gettimeofday(&this->last, NULL);
}

void wmMainWindow::installTimeout(tofn fn, int time, void *data)
{
  timeouts.push_back(timeout(fn, time, data));
}

static void mic(int state, void *data)
{
  wmMainWindow *gmw = (wmMainWindow*)data;
  gmw->mic(state);
}

static void reco(const char *text, int firm, void *data)
{
  wmMainWindow *gmw = (wmMainWindow*)data;
  gmw->reco(text, firm);
}

static void error(int UNUSED(err), const char *msg, void *data)
{
  wmMainWindow *gmw = (wmMainWindow*)data;
  gmw->errorMsg(E_CONFIG, "", msg);
}

static void vocab(const char *name, int active, void *data)
{
  wmMainWindow *gmw = (wmMainWindow*)data;
  gmw->vocab(name, active);
}

static VClient wmClient = {
  "wm",
  mic,
  0, /* state */
  reco,
  error,
  vocab,
  notify,
  initialState,
};

void wmMainWindow::main(bool save, bool turn_mic_on)
{
  XEvent Event;

  button_press(-1);

  bool init = false;
  void xvoice_init(VClient*, bool);

  while(1) {
    fd_set readfds;
    struct timeval tv;

    tv.tv_sec = 0;
    tv.tv_usec = 100;
    FD_ZERO(&readfds);
    FD_SET(vv_socket, &readfds);

    /* check data from the voice engine */
    if (select(vv_socket+1, &readfds, NULL, NULL, &tv) == 1) {
      vv_fn(vv_data);
    }

    checkTimeouts();

    /* check for X events */
    while (XPending(display) > 0) {
      XNextEvent(display, &Event);
      switch (Event.type)
      {
        case Expose:
          RedrawWindow();
          if (init == false) {
            xvoice_init(&wmClient, save);
            if (turn_mic_on) {
              vCommand(CMICON);
            }
            init = true;
          }
          break;
        case DestroyNotify:
          XCloseDisplay(display);
          exit(0);
          break;
        case ButtonPress:
          dbgprintf(("button press; "));
          region = CheckMouseRegion(Event.xbutton.x, Event.xbutton.y);
          dbgprintf(("region %d\n", region));
          switch(region)
          {
            case 0:
              button_press(0);
              break;
            case 1: /* click in text area, not implemented */
              break;
          }
          /* draw_buffer(cur_buffer,selected_buffer); */
          RedrawWindow();
          break;
        case ButtonRelease:
          if ((region == 0) || (region == 1))
            copyXPMArea(rel[region].x,rel[region].y,rel[region].w,
                rel[region].h,pos[region].x,pos[region].y);
          RedrawWindow();
          break;
      }
    }
  }
  return;
}
