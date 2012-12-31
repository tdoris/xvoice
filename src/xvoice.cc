/*
 * xvoice.cc
 *
 * Description: main program code for XVoice
 *
 * Copyright (c) 1999, David Z. Creemer.
 * Copyright (c) 2000, 2002, Brian Craft.
 * See the LICENSE file. All rights not granted therein are reserved.
 *
 * @author David Z. Creemer
 * @author Brian Craft
 * $Revision: 1.22 $
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
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <popt.h>

using namespace std;

#include "Error.h"
#include "MainWin.h"
#include "App.h"

#include "xvoice.h"
#if defined( HAVE_SMAPI )
#include "Voice.h"
#endif

MainWindow *gMainWin;
char grammarDir [ 512 ];
char modulesDir [ 512 ];
char xvoicexml[1024];
VAudioType audiolib;
int navigator = false;

void usage()
{
  fprintf( stderr, "%s %s usage:\n-m turn mic on\n-h show this help\n", 
      PACKAGE, VERSION );
}

#define LOOPTRAP "XVOICELOOPING"
void bootstrap_environment(char** argv)
{
  char *bindir = getenv("SPCH_BIN");
  char *loop = getenv(LOOPTRAP);

#ifdef VVINSTALLED_SYSTEM
  char *path, *newpath;
  char *addpath = VVINSTALLED_SYSTEM ":";

  path = getenv("PATH");
  newpath = (char *) malloc(strlen(path) + strlen(addpath) + 1);
  strcpy(newpath,path);
  strcat(newpath,addpath);
  
  setenv("LD_ASSUME_KERNEL","2.4.1",1);
  setenv("PATH", newpath, 1);
  setenv("LD_LIBRARY_PATH", "/usr/local/viavoice/redhat-6.2/lib:/usr/local/viavoice/redhat-6.2/usr/lib:/usr/lib", 1);
#endif

  if (bindir == 0) {
    printf("Environment not set. Looking for vvsetenv....");
    if (loop != 0) {
      printf("\nxvoice is looping. vvsetenv is probably not installed correctly.\n");
      exit(-1);
    }
    /*
     * try running vvsetenv. if it works,
     * then restart after sourcing vvsetenv.
     */
    if (system("vvsetenv") == 0) {
      char** arg;
      fstring cmdline;
      cmdline.appendf(". vvsetenv; exec ");
      for (arg = argv; *arg != NULL; ++arg)
        cmdline.appendf("%s ", *arg);

      printf("found vvsetenv.\n");
      printf("Restarting with new environment.\n");
      printf("command line:\n%s\n", cmdline.c_str());
      setenv(LOOPTRAP, "YES", 1);
      /* 
       * We checked for USE_SHELL in configure; it must be bash2
       * or zsh. "Why not bash1?" I hear you cry. Well,
       * because bash 1.14.7 has a bug which is invoked by
       * vvsetenv. Amazing, but true.
       */
      execlp(USE_SHELL, USE_SHELL,"-c", cmdline.c_str(), NULL);
      printf("exec failed with %s shell.\n Bailing out.\n", USE_SHELL);
      exit(-1);
    } else {
      printf("\nDidn't find vvsetenv. Bailing out.\n");
      exit(-1);
    }
  } else {
    printf("Environment set.\n");
  }
}

void ChildWaiter(int UNUSED(dummy)) {
       int status;
       while (wait3(&status, WNOHANG, NULL) > 0);
}

void xvoice_status(const char *status_info) 
{
    gMainWin->set_status(status_info);
}

void xvoice_init(VClient *cl, bool UNUSED(save))
{
  fstring filename;
  struct stat s;
  char *home=getenv("HOME");
  int vhdl;
  struct sigaction act;

  VGrammarDir(grammarDir);
  vhdl = VOpen(cl, audiolib, navigator? 1 << V_NAV : 0, gMainWin);
  if (vhdl < 0) {
    exit(1);
  }

  filename.appendf("%s/.%s", home, PACKAGE);
  if (stat(filename.c_str(), &s) != 0) {
    if (mkdir(filename.c_str(), 0755) < 0) {
      LogMessage(E_FATAL, "Could not create data directory %s!\n",
          filename.c_str());
    }
  }

  filename.resize(0);
  filename.appendf("%s/.%s/grammars", home, PACKAGE);
  if (stat(filename.c_str(), &s) != 0) {
    if (mkdir(filename.c_str(), 0755) < 0) {
      LogMessage(E_FATAL, "Could not create data directory %s!\n",
          filename.c_str());
    }
  }

  loadGrammars();

  /*
   * Reap dead children
   */
  act.sa_handler = ChildWaiter;
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_RESTART;
  sigaction(SIGCHLD, &act, NULL);

}

int main( int argc, char* argv[] )
{
  bootstrap_environment(argv);
  int save = false;
  int mic_on = false;
  char* config = NULL;
  static const struct poptOption options[] = {
    {"save",      's', POPT_ARG_NONE,   &save,           0, "Save audio files",      NULL   },
    {"mic",       'm', POPT_ARG_NONE,   &mic_on,         0, "Start with mic on",     NULL   },
    {"config",    'c', POPT_ARG_STRING, &config,         0, "Config file",           "FILE" },
    {"navigator", 'n', POPT_ARG_NONE,   &navigator,      0, "Run in navigator mode", NULL   },
#ifdef HAVE_ESD
    {"esd",       'e', POPT_ARG_VAL,    &audiolib,  AUDESD, "Use esound",            NULL   },
#endif
#ifdef HAVE_ARTS
    {"arts",      'a', POPT_ARG_VAL,    &audiolib, AUDARTS,  "Use aRts",             NULL   },
#endif
    {NULL, '\0', 0, NULL, 0, NULL, NULL}
  };

  audiolib = AUDOSS;

  // Create main window global; this is the UI.
  extern MainWindow* getMainWindow(int argc, char **argv,
      const struct poptOption* opt);
  gMainWin = getMainWindow(argc, argv, options);
  char* home;
  home=getenv("HOME");

  // Create word history list global
  _mainWIQ = new WordInfoQueue();

  // Get the location of the grammar file (xvoice.xml) to use
  if (config == NULL)
    sprintf(xvoicexml, "%s/.xvoice/xvoice.xml", home);
  else
    sprintf(xvoicexml, "%s", config);

  /* 
   * Specify the location where we will place the grammars once we
   * build them from the grammar file.
   */
  sprintf(grammarDir, "%s/.xvoice/grammars", home);
  sprintf(modulesDir, "%s/.xvoice/modules", home);
  dbgprintf(("home %s dir %s modules %s \n", home, grammarDir, modulesDir));

  printf(PACKAGE " version " VERSION "\n");

  gMainWin->main(save, mic_on);

  delete gMainWin;
  return 0;
}
