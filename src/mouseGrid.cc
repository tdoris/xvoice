#include "sys.h"

using namespace std;

#include "x.h"
#include "Voice.h"
#include "MainWin.h"
#include "ParseEventStream.h"
#include "App.h"

static int vhdl;

class grid
{
  public:
    grid(Display* d, Window w, MainWindow* mw1) :
      display(d), root(w), mw(mw1), active(false) {};
    ~grid();

    void windows_new();
    void windows_destroy();
    void draw_grid(int x, int y, unsigned int w, unsigned int h);

    Display* display;
    Window root;
    unsigned int rw, rh, rd;
    MainWindow* mw;
    Window gwin[8];
    XColor black, white;
    bool active;
};

void grid::windows_new()
{
  int i;
  XSetWindowAttributes attr;
  static char bitmap[] = { 
    0x0c, 0x0c, 0x03, 0x03,
  };
  Pixmap bm, stip;

  bm = XCreateBitmapFromData(display, root, bitmap, 4, 4);

  stip = XCreatePixmapFromBitmapData(display, root, bitmap,
      4, 4, white.pixel, black.pixel, rd);

  attr.background_pixmap = stip;
  attr.override_redirect = True;

  for (i = 0; i < 8; ++i) {
    gwin[i] = XCreateWindow(display, root, 4, 4, 4, 4, 0,
        CopyFromParent, InputOutput, CopyFromParent, 
        CWOverrideRedirect|CWBackPixmap, &attr);
  }
}

void grid::windows_destroy()
{
  int i;

  for (i = 0; i < 8; ++i) {
    XDestroyWindow(display, gwin[i]);
  }
}

void grid::draw_grid(int x, int y, unsigned int w, unsigned int h)
{
  int i;

  dbgprintf(("x %d y %d w %d h %d\n", x, y, w, h));
  for (i = 0; i < 4; ++i) {
    XUnmapWindow(display, gwin[i]);
    XResizeWindow(display, gwin[i], 1, h);
    XMoveWindow(display, gwin[i], x + w*i/3, y);
    XMapWindow(display, gwin[i]);

    XUnmapWindow(display, gwin[i+4]);
    XResizeWindow(display, gwin[i+4], w, 1);
    XMoveWindow(display, gwin[i+4], x, y + h*i/3);
    XMapWindow(display, gwin[i+4]);
  }
}

enum GridCmd {
  GRID_STOP = 9,
  GRID_WARP
};

static VVocab gridVocab[] = { 
  { 0, "grid one" },
  { 1, "grid two" },
  { 2, "grid three" },
  { 3, "grid four" },
  { 4, "grid five" },
  { 5, "grid six" },
  { 6, "grid seven" },
  { 7, "grid eight" },
  { 8, "grid nine" },
  { GRID_STOP, "stop grid" },
  { GRID_WARP, "warp" },
  { -1, NULL }
};

static void grid_voc(int UNUSED(vh), VVocabResult ev, char const* UNUSED(ph), int val, void *gp)
{
  grid* g = (grid*)gp;
  int x, y, nx, ny;
  int square;
  Window z;
  unsigned int w, h, nw, nh, du;

  dbgprintf(("val %d\n", val));
  if (ev == V_VOCAB_FAIL) {
    /* 
     * FIXME leave this as FATAL? If nothing else depends on it,
     * make it CONFIG
     */
    g->mw->errorMsg(E_FATAL, __FUNCTION__,
        "Couldn't install mouse grid vocabulary.");
    g->active = false;
    return;
  }
  if (ev == V_VOCAB_SUCCESS) {
    return;
  }
  XGetGeometry(g->display, g->gwin[0], &z,
      &x, &y, &du, &h,
      &du, &du);

  XGetGeometry(g->display, g->gwin[4], &z, 
      &x, &y, &w, &du,
      &du, &du); 

  if (val == GRID_STOP) {
    g->active = false;
    g->windows_destroy();
    VDisableVocab(vhdl, "gridVocab");
    return;
  }

  if (val < GRID_STOP) square = val;
  else square = 4;

  nw = w/3;
  nh = h/3;
  nx = x + nw*(square%3);
  ny = y + nh*(square/3);
  if (w > 10 && h > 10 && val < GRID_STOP) {
    g->draw_grid(nx, ny, nw, nh);
  } else {
    fstring str;
    EventStream *es;
    nx += nw/2;
    ny += nh/2;
    str.appendf("<mouse origin='root' x='%d' y='%d' />", nx, ny);
    es = parseBuff(str.c_str());
    appSendEventStream(es);
    g->active = false;
    g->windows_destroy();
    VDisableVocab(vhdl, "gridVocab");
  }
}



static VVocab guiVocab[] = { 
  { 0, "mouse grid" },
  { -1, NULL }
};

static void gui_vocab_cb(int UNUSED(vh), VVocabResult ev, char const* UNUSED(ph), int UNUSED(val), 
    void *gp)
{
  grid* g = (grid*)gp;

  dbgprintf((""));
  if (ev == V_VOCAB_FAIL) {
    /*
     * FIXME leave this as FATAL? If nothing else depends on it,
     * make it CONFIG
     */
    g->mw->errorMsg(E_FATAL, __FUNCTION__,
        "Couldn't install grid vocabulary");
    g->active = false;
    return;
  }
  if (ev == V_VOCAB_SUCCESS) {
    VEnableVocab(vhdl, "guiVocab");
    return;
  }
  if (g->active) return;

  g->active = true;
  VEnableVocab(vhdl, "gridVocab");

  g->windows_new();
  g->draw_grid(0, 0, g->rw, g->rh);
}

void initGrid(Display* display, Window root, MainWindow* mw)
{
  grid* g = new grid(display, root, mw);
  XWindowAttributes attr;

  XGetWindowAttributes(display, root, &attr);

  g->rw = attr.width;
  g->rh = attr.height;
  g->rd = attr.depth;

  g->white.red = 0xFFFF;
  g->white.green = 0xFFFF;
  g->white.blue = 0xFFFF;
  g->white.pixel = 0;

  if (!XAllocColor(display, attr.colormap, &g->white)) {  
    fprintf(stderr, "wmvoice.app: can't allocate color.\n");
  }

  g->black.red = 0x0;
  g->black.green = 0x0;
  g->black.blue = 0x0;
  g->black.pixel = 0;

  if (!XAllocColor(display, attr.colormap, &g->black)) {  
    fprintf(stderr, "wmvoice.app: can't allocate color.\n");
  }

  VDefineVocab(vhdl, "gridVocab", grid_voc, gridVocab, (void *)g);
  VDefineVocab(vhdl, "guiVocab", gui_vocab_cb, guiVocab, (void *)g);
}
