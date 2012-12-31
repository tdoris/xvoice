#ifndef WMGENERAL_H_INCLUDED
#define WMGENERAL_H_INCLUDED
#include <popt.h>

/* Defines */

#define MAX_MOUSE_REGION (16)

/* Typedefs */

typedef struct _rckeys rckeys;

struct _rckeys {
  const char *label;
  char **var;
};

typedef struct _rckeys2 rckeys2;

struct _rckeys2 {
  const char *family;
  const char *label;
  char **var;
};

typedef struct {
  Pixmap pixmap;
  Pixmap mask;
  XpmAttributes attributes;
} XpmIcon;

/* Global variable */

extern Display *display;
extern Window Root, iconwin, win;


/* Function Prototypes */

void AddMouseRegion(int index, int left, int top, int right, int bottom);
int CheckMouseRegion(int x, int y);

void openXwindow(int argc, char* argv[], poptOption const* opts, char const* const pixmap_bytes[], char const* pixmask_bits, int pixmask_width, int pixmask_height);
void RedrawWindow(void);
void RedrawWindowXY(int x, int y);

void createXBMfromXPM(char*, char const* const*, int, int);
void copyXPMArea(int, int, int, int, int, int);
void copyXBMArea(int, int, int, int, int, int);
void setMaskXY(int, int);

void parse_rcfile(const char *, rckeys *);

#endif
