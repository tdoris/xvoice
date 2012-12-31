/*
 * App.cc
 *
 * Description: state machine for commanding/dictating to applications.
 *
 * Copyright (c) 1999, 2000, 2001, David Z. Creemer, Tom Doris, Brian Craft,
 * Deborah Kaplan, Jessica Perry Hekman.
 * See the LICENSE file. All rights not granted therein are reserved.
 *
 * @author David Z. Creemer
 * @author Tom Doris
 * @author Brian Craft
 * @author Deborah Kaplan
 * @author Jessica Perry Hekman
 *
 * $Revision: 1.33 $
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
#include <stack>
#include <list>
#include <map>
#include <gnome.h>
#include <cctype>
#include <sys/dir.h>
#include <sys/param.h>
#include <sys/types.h>
#include <regex.h>

#include "glib.h"
#include "Error.h"
#include "Target.h"
#include "MainWin.h"
#include "Voice.h"
#include "ParseEventStream.h"
#include "xvoice.h"
#include "App.h"

using namespace std;

/* local variables */
static  applicationList *gAppList = NULL;

static char const* vIdleVocabName = "IdleVocab";
static VVocab vIdleVocab[] = {
  { CCOMMAND, "command mode" },
  { CDICTATE, "dictate mode" },
  { CMICOFF, "microphone off" },
  { CREBUILD, "build grammar files" },
  { -1, NULL }
};

static char const* vCommandVocabName = "CommandVocab";
static VVocab vCommandVocab[] = {
  { CSTOPCOMMAND, "stop command" },
  { CIDLE, "idle mode" },
  { -1, NULL }
};

static char const* vDictateVocabName = "DictateVocab";
static VVocab vDictateVocab[] = {
  { CSTOPDICTATE, "stop dictation" },
  { CCORRECTION, "correction" },
  { CIDLE, "idle mode" },
  { -1, NULL }
};

typedef list<string> grList;

typedef map<string, bool> defaultAppList;

class State : public TargetData {
  public:
    string app;
    bool commanding;
    grList appGram;
    bool dictating;
    char const* userCase;
    char const* lastUserCase;
    bool resetUserCase;
    int userSpace;
    bool resetUserSpace;
    fstring systemCase;
    bool resetSystemCase;
    int systemSpace;
    bool resetSystemSpace;
    State();
    ~State() {};
};


static stack<int> wordLengths;

static defaultAppList alwaysOnList;

static Target target;
static State state;
static int vhdl;
extern char grammarDir[];
extern char modulesDir[];

void disableAlwaysOn(void);
void enableAlwaysOn(void);

State::State() :
    commanding(false),
    dictating(false)
{
  gboolean def;
  userCase = "None";
  resetUserCase = false;
  userSpace = gnome_config_get_int_with_default
    ("xvoice/Dictation/DefaultSpacing=1", &def);
  resetUserSpace = false;

  /* 
   * When these variables are first initialized, we are about to dictate into
   * an app for the first time ever. Hence we are at the beginning
   * of a new paragraph, so set system casing/spacing accordingly.
   */
  char *s = gnome_config_get_string_with_default
    ("xvoice/Dictation/InitialCase=Capitalize", &def);
  systemCase = s;
  g_free(s);
  resetSystemCase = true;
  systemSpace = gnome_config_get_int_with_default
    ("xvoice/Dictation/InitialSpacing=0", &def);
  resetSystemSpace = true;
}

void appSetSystemState(char *systemCase, int systemSpace)
{
  // update active window list targets with new defaults
  Target::winList::iterator win;

  for (win = target._actvWin.begin(); win != target._actvWin.end(); ++win) {
    TargetData *td = win->data;
    State *s = (State *)td;
    if (s != NULL) {
      s->systemCase = systemCase;
      s->systemSpace = systemSpace;
    }
  }
}

FILE *autof = NULL;
bool did_auto = false;

fstring
getAutoGrammarFile()
{
    fstring pathname;
    pathname.appendf("%s/%s",modulesDir,".autoGrammar.xml");
    return pathname;
}

int cleanOldGrammars()
{
  /* Clean old grammar files. */
  struct direct **files;
  int count, i;
  count = scandir(grammarDir, &files, NULL, NULL);

  for (i = 0; i < count; i++) {
    if (strcmp (files[i]->d_name, ".") != 0
	&& strcmp (files[i]->d_name, "..") != 0) {
      fstring pathname;
      pathname.appendf("%s/%s", grammarDir, files[i]->d_name);
      unlink (pathname.c_str());
    }
  }
  unlink (getAutoGrammarFile().c_str());
}

int loadMainGrammar()
{
  // Wes: I don't get it.  Delete this variable and things fail
  // (run-time), even though its not used.
  struct direct **files;

  if (gAppList != NULL) delete gAppList;
  gAppList=parseFile(xvoicexml, grammarDir);

  // look for a global config
  if (gAppList != NULL) {
    printf ("Loaded grammar in %s\n", xvoicexml);
    return true;
  } else {
    gAppList=parseFile(DATADIR "/" PACKAGE "/xvoice.xml", grammarDir);
  }

  /*
   * Some desperate measures in case xvoice.xml is not in the
   * expected location
   */
  if (gAppList != NULL) {
    printf("Loaded grammar in " DATADIR "/" PACKAGE "/xvoice.xml\n");
    return true;
  } else {
    gAppList=parseFile("xvoice.xml", grammarDir);
  }

  if (gAppList != NULL) {
    printf("Loaded grammar in ./xvoice.xml\n");
    return true;
  } else {
    gAppList=parseFile("../xvoice.xml", grammarDir);
  }

  if (gAppList == NULL)
    LogMessage(E_CONFIG,
        "Could not find xvoice.xml; no grammars will be "
        "available.\nLooked in:\n\n%s\n"
        DATADIR "/" PACKAGE
        "/xvoice.xml\n./xvoice.xml\n../xvoice.xml\n\n",
        xvoicexml);

  // We shouldn't get this far.
  return false;
}

void loadModuleGrammars(applicationList* intolist)
{
    /* we do this in 2 passes.  First we load everything with a
       define- prefix to allow at least minimal pre loading of define
       statements.  Then, we load the rest of the files, which can
       make use of all of the previously defined prerequisites. */

    struct direct **files;
    int count, i, pathnum, prefixes;

    string search_paths[] = 
    {
        DATADIR "/" PACKAGE "/modules",
        modulesDir
    };

    /* warning: kind of hacky to achieve code reduction */
    for(prefixes = 0; prefixes < 2; prefixes++) {
        for(pathnum=0; pathnum < 2; pathnum++) {
            files = NULL;
            count = scandir(search_paths[pathnum].c_str(), &files, NULL,
                            alphasort);

            for (i = 0; i < count; i++) {
                // ignore other files (backups, dirs, etc)
                if (strcmp(files[i]->d_name + strlen(files[i]->d_name)-4, 
                            ".xml") == 0 &&
                    strncmp(files[i]->d_name, ".", 1) != 0 &&
                    (strncmp(files[i]->d_name, "define-", 7) != 0) == prefixes) {
                    fstring pathname;
                    pathname.appendf("%s/%s", search_paths[pathnum].c_str(),
                                     files[i]->d_name);
                    if (!parseFile(pathname.c_str(), grammarDir, intolist)) {
                        LogMessage(E_CONFIG,
                                   "Failedto parse the %s file\n",
                                   pathname.c_str());
                    } else {
                        fprintf(stderr, "Loaded grammer in %s\n",
                                pathname.c_str());
                    }
                }
            }
            free(files);
        }
    }
}

#define CHECKORDIE(ff) \
    if (!stat(ff, &statbuf)) {                   \
        if (statbuf.st_mtime > dirTime) {       \
            if (files) free(files);              \
            return true;                         \
        }                                        \
    }
  

/* check most recently modified files.  if nothing newer than the
   grammerDir save directory, then we're safe */
bool needGrammarCompile()
{
  struct direct **files = NULL;
  int count, i;
  struct stat statbuf;
  time_t dirTime;
  time_t greatestModuleTime = 0;
  int pathnum;

  /* check the compilation dir */
  if (stat(grammarDir, &statbuf) || statbuf.st_mtime == 0)
      return true; // no such directory yet.
  dirTime = statbuf.st_mtime;
  
  CHECKORDIE(xvoicexml);
  CHECKORDIE(DATADIR "/" PACKAGE "/xvoice.xml");
  CHECKORDIE("./xvoice.xml");
  CHECKORDIE("../xvoice.xml");

  string search_paths[] = 
      {
          DATADIR "/" PACKAGE "/modules",
          modulesDir
      };

  for(pathnum=0; pathnum < 2; pathnum++) {
      files = NULL;
      count = scandir(search_paths[pathnum].c_str(), &files, NULL,
                      alphasort);
      for (i = 0; i < count; i++) {
          fstring pathname;
          pathname.appendf("%s/%s", search_paths[pathnum].c_str(),
                           files[i]->d_name);
          CHECKORDIE(pathname.c_str());
      }
  }
  xvoice_status("Grammer files already pre-compiled");
  return false;
}

void
add_auto_grammar(const char *autog_expr, const char *gram_name) 
{
    // don't do it if we've already compiled.
    if (xv_get_disable_compile())
        return;
    
    if (!autof) {
        if ((autof = fopen(getAutoGrammarFile().c_str(),"a")) == NULL) {
            fprintf(stderr,
                    "couldn't write to path %s.  Auto-grammer not built\n",
                    getAutoGrammarFile().c_str());
            return;
        }
    }
    if (!did_auto) {
//        fprintf(autof,"");
        fprintf(autof,"<xvoice>\n\n  <!-- automatically generated by xvoice.  Do not edit -->\n\n  <application name='autogrammar' expression='autogrammar' alwaysOn='true'><![CDATA[\n    <<root>> = \n");
        fprintf(autof,"      enable %s grammar -> <grammar name='%s' action='on' />\n                <setStatus value='enabled %s grammar\'/>\n", autog_expr, gram_name, gram_name);
        did_auto = true;
    } else {
        fprintf(autof,"    | enable %s grammar -> <grammar name='%s' action='on' />\n                <setStatus value='enabled %s grammar\'/>\n", autog_expr, gram_name, gram_name);
    }
    fprintf(autof,"    | disable %s grammar -> <grammar name='%s' action='off' />\n                <setStatus value='disabled %s grammar\'/>\n\n", autog_expr, gram_name, gram_name);
    fclose(autof);
    autof = NULL;
}

bool
end_auto_grammar() 
{

    if (did_auto) {
        if (!autof) {
            if ((autof = fopen(getAutoGrammarFile().c_str(),"a")) == NULL) {
                fprintf(stderr,
                        "couldn't write to path %s.  Auto-grammer not built\n",
                        getAutoGrammarFile().c_str());
                return false;
            }
        }
        fprintf(autof,"    .\n  ]]>\n  </application>\n</xvoice>\n");
        fclose(autof);
        autof = NULL;
        did_auto = false;
        return true;
    }
    return false;
}

int loadGrammars()
{
    clear_alwayson();
    if (!needGrammarCompile())
        xv_disable_compile();
    else {
        xv_enable_compile();
        cleanOldGrammars();
    }

    // load a primary xvoice.xml file.
    if (!loadMainGrammar()) {
        xv_enable_compile();
        return false;
    }
    
    // load all modules
    loadModuleGrammars(gAppList);

    // load the auto grammar set (and ignore failures if it doesn't exist)
    end_auto_grammar();
    if (parseFile(getAutoGrammarFile().c_str(), grammarDir, gAppList)) {
        printf ("Loaded grammar in %s\n", getAutoGrammarFile().c_str());
    }
    xv_enable_compile(); // just to be sure, we always turn it back on.
    return true;
}

static application* getApp(const char *name)
{
  regex_t preg;

  applicationList::iterator app;

  // Only do the for loop if there are grammars to loop through
  if (gAppList != NULL) {
    for(app=gAppList->begin(); app!=gAppList->end(); app++) {
      dbgprintf(("(%s) looking at %s\n",name, app->name));
      if (regcomp(&preg, app->expr, REG_EXTENDED|REG_NOSUB) == 0) {
        if (regexec(&preg, name, 0, NULL, 0) == 0) {
          dbgprintf(("matched\n"));
          regfree(&preg);
          return &*app;
        }
        regfree(&preg);
      }
    }
  }

  return NULL;
}

static void appHandler(int vh, VVocabResult ev, const char *ph, const char *tr, 
    void *name);
static void grammarEvent(Event& ev)
{
  if (ev.val.grammar.enable) {
    state.appGram.push_back(ev.val.grammar.name);
    VEnableGrammar(vhdl, ev.val.grammar.name, appHandler,
       (void *)state.appGram.back().c_str());
  } else {
    VDisableGrammar(vhdl,ev.val.grammar.name);
    grList::iterator gr;
    for (gr=state.appGram.begin(); gr!=state.appGram.end(); gr++) {
      if (!strcmp(gr->c_str(), ev.val.grammar.name)) {
        state.appGram.erase(gr);
        break;
      }
    }
  }
}

void do_setUserCase(Event *ev)
{
  state.lastUserCase = state.userCase;
  state.userCase = ev->val.setUserCase.name;
  state.resetUserCase = ! ev->val.setUserCase.continuous;
}

void do_setUserSpace(Event *ev)
{
  if (strcmp(ev->val.setUserSpace.name, "no-space") == 0) {
    state.userSpace = 0;
  } else {
    state.userSpace = 1;
  }
  state.resetUserSpace = ! ev->val.setUserSpace.continuous;
}

void do_pause(Event *ev)
{
  sleep (ev->val.pause.duration);
}

void do_setStatus(Event *ev)
{
    xvoice_status(ev->val.singleString.value);
}

void do_add_alwaysOn(Event *ev)
{
    add_alwayson(ev->val.singleString.value);
}

void do_remove_alwaysOn(Event *ev)
{
    remove_alwayson(ev->val.singleString.value);
}

/*
 * Returns true if we lost the target
 */

static bool dispatch(const char *translation)
{
  EventStream* es;
  bool ret = false;

  es = parseBuff(translation);
  dbgprintf(("The translation is : \"%s\"\n",translation));
  dbgprintf(("Length of event stream %d\n", es->size()));
  if (!es) {
    LogMessage(E_CONFIG, "XML error parsing \"%s\"\n",translation);
    delete es;
    return false;
  }

  while (!es->empty()) {
    Event ev = es->front();
    switch (ev.type) {
      case EVGRAM:
        dbgprintf(("grammar event %d %s\n",
              ev.val.grammar.enable,
              ev.val.grammar.name));
        grammarEvent(ev);
        es->pop_front();
        break;
      case EVCASE:
        dbgprintf(("%s %d\n", ev.val.setUserCase.name,
              ev.val.setUserCase.continuous));
        do_setUserCase(&ev);
        es->pop_front();
        break;
      case EVSPACE:
        dbgprintf(("%s %d\n", ev.val.setUserSpace.name,
              ev.val.setUserSpace.continuous));
        do_setUserSpace(&ev);
        es->pop_front();
        break;
      case EVPAUSE:
        dbgprintf(("%d\n", ev.val.pause.duration));
        do_pause(&ev);
        es->pop_front();
        break;
      case EVSTATUSBAR:
        dbgprintf(("%s\n", ev.val.singleString.value));
        do_setStatus(&ev);
        es->pop_front();
        break;
      default:
        if (target.sendEventStream(es)) { // lost target
          ret = true;
        }
        break;
    }
    if (ret == true) break;
  }
  delete es;
  return ret;
}

static void navHandler(int UNUSED(vh), VVocabResult ev, const char *ph,
    const char *tr, void* UNUSED(name))
{
  switch (ev) {
    case V_VOCAB_FAIL:
      LogMessage(E_CONFIG,
          "Couldn't activate Window Manager commands\n%s", ph);
      break;
    case V_VOCAB_SUCCESS:
      break;
      // FIXME write more helpful error message
    case V_VOCAB_RECO:
      if (dispatch(tr))
        LogMessage(E_SEVERE, "Lost nav target");
      break;
    default:
      break;
  }
}

static void appHandler(int UNUSED(vh), VVocabResult ev, const char *ph, const char *tr, 
    void *name)
{
  grList::iterator gr;
  switch (ev) {
    case V_VOCAB_FAIL:
      LogMessage(E_CONFIG, "Couldn't activate grammar %s\n%s\n",
          (const char *)name, ph);
      for (gr=state.appGram.begin(); gr!=state.appGram.end(); gr++) {
        if (!strcmp(gr->c_str(), (const char *)name)) {
          state.appGram.erase(gr);
          break;
        }
      }
      break;
    case V_VOCAB_SUCCESS:
      break;
    case V_VOCAB_RECO:
      if (dispatch(tr)) vCommand(CIDLE);
      break;
    default:
      break;
  }
}

// Based on the caseReq string, modify the word string.
static void setWordCase(char *word, const char *caseReq)
{
  if(strcasecmp(caseReq, "Uppercase") == 0) {
    g_strup(word);
  } else if(strcasecmp(caseReq, "Lowercase") == 0) {
    g_strdown(word);
  } else if(strcasecmp(caseReq, "TitleCase") == 0) {
    // FIXME: don't modify "a", "the", etc.
    word[0]=toupper(word[0]);
  } else if(strcasecmp(caseReq, "Capitalize") == 0) {
    word[0]=toupper(word[0]);
  }
}

static void dictationHandler(int UNUSED(vh), VVocabResult UNUSED(msgtype), int num_firm, 
    char const** text, void* UNUSED(data))
{
  int i, spacing;
  fstring buffer;
  gboolean def;

  /*
   * iterate through each word in a phrase. a phrase is determined
   * by the engine.
   */
  for (i = 0 ; i < num_firm; i++)
  {
    char wordbuf[512];
    char* word = wordbuf;
    bool dynamic_allocated_word = false;
    size_t word_len = strlen(text[i]);

    if (word_len > 511)
    {
      word = new char [word_len + 1];
      dynamic_allocated_word = true;
    }

    buffer.resize(0); // no clear() function?
    strcpy(word, text[i]);

    /*
     * Deal with special requests and punctuation.
     */

    /*
     * if the engine says it's a new paragraph, set some state for the
     * next time through the loop or the next time through the
     * method.
     */
    if (strcmp(word,"NEW-PARAGRAPH")==0) {
      wordLengths.push( 2 );
      if (target.sendText("\\r\\r")) { /* lost target */
        vCommand(CIDLE);
        return;
      }
      state.systemCase = "Capitalize";
      state.resetSystemCase = true;
      state.systemSpace = gnome_config_get_int_with_default
        ("xvoice/Dictation/AfterParaSpacing=0", &def);
      state.resetSystemSpace = true;
      continue;
    }

    /*
     * if the engine says it's a new line, set some state for the
     * next time through the loop or the next time through the
     * method.
     */
    if (strcmp(word,"NEW-LINE")==0) {
      wordLengths.push( 1 );
      if (target.sendText("\\r")) { /* lost target */
        vCommand(CIDLE);
        return;
      }
      if (gnome_config_get_bool_with_default
          ("xvoice/Dictation/AfterNewLineCap=true", &def)) {
        state.systemCase = "Capitalize";
        state.resetSystemCase = true;
      }
      state.systemSpace = gnome_config_get_int_with_default
        ("xvoice/Dictation/AfterNewLineSpacing=0", &def);
      state.resetSystemSpace = true;
      continue;
    }

    // Other special cases
    if (strcmp(word,"OPEN-\"")==0 || strcmp(word,"(")==0 ||
        strcmp(word,"{")==0 || strcmp(word,"[")==0) {

      // These need normal spacing before them, but no space after

      for (spacing = state.userSpace == 1
          ? state.systemSpace : state.userSpace;
          spacing > 0; spacing--) {
        wordLengths.push( 1 );
        if (target.sendText(" ")) { /* lost target */
          vCommand(CIDLE);
          return;
        }
      }

      if (strcmp(word,"OPEN-\"")==0) {
        word[0] = '"';
	word[1] = 0;
      }
      wordLengths.push( 1 );
      if (target.sendText(word)) { /* lost target */
        vCommand(CIDLE);
        return;
      }
      state.systemSpace = 0;
      state.resetSystemSpace = true;
      continue;
    } /* if OPEN-quote or paren, bracket, brace */

    /*
     * this needs no spacing before or after it. (Eventually, a
     * user should be able to specify spacing for this in
     * xvoice.xml.)
     */
    else if (strcmp(word,"SPACE-BAR")==0) {
      wordLengths.push( 1 );
      if (target.sendText(" ")) { /* lost target */
        vCommand(CIDLE);
        return;
      }
      state.systemSpace = 0;
      state.resetSystemSpace = true;
      continue;
    }

    /*
     * this needs no spacing before it and normal spacing after
     * it, so continue on to the rest of this control structure
     */
    else if (strcmp(word,"CLOSE-\"")==0) {
      word[0] = '"';
      word[1] = 0;
    }

    /*
     * If the word is (other) punctuation, print the word to the buffer
     * and loop before we have a chance to screw with case/spacing.
     * In some cases (eg, end of sentences), set later spacing state.
     */
    if (( strlen( word ) == 1) &&
        ispunct( word[0] ) ) {
      buffer = word;

      char p = word[0];
      /*
       * FIXME: check for all punctuation cases, not just 3, from
       * array.
       */
      if (p == '.' || p == '?' || p == '!') {
        state.systemCase = "Capitalize";
        state.resetSystemCase = true;

        gboolean def;
        state.systemSpace =
          gnome_config_get_int_with_default
          ("xvoice/Dictation/AfterSentenceSpacing=2", &def);
        state.resetSystemSpace = true;
      }

      /*
       * These lines are normally done at the end of a loop through
       * the current phrase, but we need to reloop early so as to
       * save the bits of state that we just set,  so do them now
       * and then loop.
       * FIXME write method taking buffer as input?
       */
      wordLengths.push( buffer.size());
      if (target.sendText(buffer.c_str())) {
        vCommand(CIDLE);
      }
      continue;
    }

    /*
     * We only get here if all we're looking at is a normal
     * word, not a special request or punctuation.
     */

    /*
     * deal with user and system spacing requests.
     * always have special requests override normal
     * requests; if both are special, user trumps system.
     * assume "1" is "normal" and "not 1" is "special."
     */
    for (spacing = (state.userSpace == 1
          ? state.systemSpace : state.userSpace);
        spacing > 0; spacing--) {
      buffer.appendf(" ");
    }

    /*
     * If the user has not specified any casing requests, check
     * the system casing requests. Otherwise, do what the user
     * asked. By default, do not modify the word at all.
     */
    if(strcasecmp(state.userCase, "None") == 0) {
      setWordCase(word, state.systemCase.c_str());
    } else { // user has made a request
      setWordCase(word, state.userCase);
    }

    /*
     * Print word to buffer no matter what; its casing
     * may or may not have been modified.
     */
    buffer.appendf("%s", word);

    /*
     * reset all state here: if we were in one of those special cases
     * where it got set and we need it for the next time through,
     * we will already have hit a "continue" statement and won't
     * get to this particular hunk of code. Do check whether each
     * state *should* be reset; some of them need to continue
     * indefinitely.
     */

    /*
     * FIXME if user says "set uppercase; lower next;
     * title next; word1; word2"
     * then word2 *should* be uppercase but *will* be lower.
     */
    if(state.resetUserCase) {
      state.userCase = state.lastUserCase;
      state.lastUserCase = gnome_config_get_string_with_default
        ("xvoice/Dictation/DefaultCase=None", &def);
      state.resetUserCase = false;
    }

    if(state.resetUserSpace) {
      state.userSpace = gnome_config_get_int_with_default
        ("xvoice/Dictation/DefaultSpacing=1", &def);
      state.resetUserSpace = false;
    }

    if(state.resetSystemCase) {
      state.systemCase = gnome_config_get_string_with_default
        ("xvoice/Dictation/DefaultCase=Normal", &def);
      state.resetSystemCase = false;
    }

    if(state.resetSystemSpace) {
      state.systemSpace = gnome_config_get_int_with_default
        ("xvoice/Dictation/DefaultSpacing=1", &def);
      state.resetSystemSpace = false;
    }

    /*
     * Do some maintenance before looping:
     */

    wordLengths.push( buffer.size());

    // If you have lost the target, send a CIDLE command.
    if (target.sendText(buffer.c_str())) {
      vCommand(CIDLE);
    }

    if (dynamic_allocated_word)
      delete [] word;

  } // end iteration through words in phrase
}

static void CommandState(bool enter)
{
  char *name = NULL;

  if (enter) {
    dbgprintf(("enter\n"));
    while (state.appGram.size() > 0) {
      VDisableGrammar(vhdl,state.appGram.front().c_str());
      state.appGram.pop_front();
    }

    application *app = getApp(target.name());
    if (app != NULL)
      name = app->name;
    if (name != NULL) {
      state.app = name;
      state.appGram.push_back(name);
      VEnableGrammar(vhdl, name, appHandler,
          (void*)state.appGram.back().c_str());
      if (!state.commanding)
        VEnableVocab(vhdl, vCommandVocabName);
      enableAlwaysOn();
      state.commanding = true;
    } else {
      enter = false; /* fall through to disable */
    }
  }

  if (!enter && state.commanding) {
    VDisableVocab(vhdl,vCommandVocabName);
    while (state.appGram.size() > 0) {
      VDisableGrammar(vhdl,state.appGram.front().c_str());
      state.appGram.pop_front();
    }
    disableAlwaysOn();
    state.commanding = false;
  }
  return;
}

static void DictateState(bool enter)
{
  if (enter && !state.dictating) {
    dbgprintf(("enter\n"));
    VStartDictation(vhdl,dictationHandler, NULL);
    VEnableVocab(vhdl, vDictateVocabName);
    state.dictating = true;
  } else if (!enter && state.dictating) {
    VDisableVocab(vhdl,vDictateVocabName);
    VStopDictation(vhdl);
    state.dictating = false;
  }
}

/*
 * Called periodically. Check to see if we have changed focus; if we have, save
 * current state (if we have a target), and restore the state from the last
 * time this target had focus.
 */

static int Focus(void*)
{
  FocusEvent ev;
  State *save_state = (State*) target.getData();
  application *app;

  ev = target.focusedTarget();
  switch (ev) {
    case TNONE:
      return 1;                    /* same window, same name */

    case TNAME:
      app = getApp(target.name());
      if (app == NULL || app->name == state.app) return 1;

      gMainWin->setTarget(target.name());
      if (app->dictation != NULL && strcmp (app->dictation, "on") == 0) {
	DictateState(true);
      } else {
	DictateState(false);
      }
      CommandState(false);
      CommandState(true);
      return 1;

    default:

      gMainWin->setTarget(target.name());

      *save_state = state;

      DictateState(false);          /* reset state */
      CommandState(false);

      save_state = (State*)target.getData();
      if (save_state == NULL) {     /* new window */
        save_state = new State;
        save_state->commanding = true;

	app = getApp(target.name());
	if (app != NULL &&
	    app->dictation != NULL &&
	    strcmp (app->dictation, "on") == 0)
	  save_state->dictating = true;

        target.setData((TargetData*)save_state);
      }

      if (save_state->dictating)
        DictateState(true);
      if (save_state->commanding) {
        CommandState(true);
        /* We can get away with enabling the base grammar twice. */
        grList::iterator gr;
        for (gr = save_state->appGram.begin();
            gr != save_state->appGram.end(); ++gr) {
          state.appGram.push_back(*gr);
          VEnableGrammar(vhdl, gr->c_str(), appHandler, (void*)gr->c_str());
        }
      }
      return 1;
  } /* switch */
}

void vCommand(vCommandType t)
{
  switch (t) {
    case CMICON:
      VMicOn(vhdl, 1);
      return;
    case CMICOFF:
      VMicOn(vhdl, 0);
      return;
    case CREBUILD:
      loadGrammars();
      return;
    case CIDLE:
      DictateState(false);
      CommandState(false);
      break;
    case CCOMMAND:
      CommandState(true);
      break;
    case CSTOPCOMMAND:
      CommandState(false);
      break;
    case CFOCUS:
      Focus(NULL);
      break;
    case CDICTATE:
      DictateState(true);
      break;
    case CSTOPDICTATE:
      DictateState(false);
      break;
    case CCORRECTION:
      {
        dbgprintf(("correction\n"));
        if (!wordLengths.empty())
        {
          dbgprintf(("doing correction\n"));
          int lastWordLen = wordLengths.top();
          wordLengths.pop();
          dbgprintf(("lastWordLen %d\n", lastWordLen));
          fstring buffer(lastWordLen, '\b');
          if (target.sendText( buffer.c_str() )) {
            /* lost target */
	    printf ("lost target\n");
            dbgprintf(("lost target\n"));
            vCommand(CIDLE);
          }
        }
      }
      break;
    default:
      break;
  }
}

static void vocHandler(int UNUSED(vh), VVocabResult ev, char const* UNUSED(ph), int val, 
    void* UNUSED(user))
{
  switch (ev) {
    case V_VOCAB_FAIL:
      break;
    case V_VOCAB_SUCCESS:
      break;
    case V_VOCAB_RECO:
      vCommand((enum vCommandType)val);
      break;
  }
}

EventRecord* appRecordMouse()
{
  return target.recordMouse();
}

int appSendEventStream( EventStream* evs )
{
  return target.sendEventStream(evs);
}

static int cleanTarget(void* UNUSED(data))
{
  target.gc();
  return TRUE;
}

void add_alwayson(string grammar_name, bool apporglobal)
{
    if (alwaysOnList.find(grammar_name) == alwaysOnList.end())
        alwaysOnList.insert(pair<string,int>(grammar_name, apporglobal));
}

void remove_alwayson(string grammar_name)
{
    alwaysOnList.erase(grammar_name);
}

void clear_alwayson()
{
    alwaysOnList.erase(alwaysOnList.begin(), alwaysOnList.end());
}

void initialState(void* UNUSED(data))
{
  VDefineVocab(vhdl, vCommandVocabName, vocHandler, vCommandVocab, NULL);
  VDefineVocab(vhdl, vDictateVocabName, vocHandler, vDictateVocab, NULL);
  VDefineVocab(vhdl, vIdleVocabName, vocHandler, vIdleVocab, NULL);

  target.focusedTarget();
  target.setData((TargetData*)new State);
  CommandState(true);

  VEnableVocab(vhdl, vIdleVocabName);

  gMainWin->installTimeout(Focus, 800, NULL);
  gMainWin->installTimeout(cleanTarget, 8000, NULL);
  gMainWin->setTarget(target.name());
  gMainWin->initVocabs();

  if (VGrammarExists(vhdl, "windowmanagershortcuts", navHandler,  (void*)NULL))
    VEnableGrammar(vhdl, "windowmanagershortcuts", navHandler, (void*)NULL);

  return;
}

void enableAlwaysOn(void)
{
    defaultAppList::iterator iter;
    for(iter = alwaysOnList.begin(); iter != alwaysOnList.end(); iter++) {
        if (VGrammarExists(vhdl, iter->first.c_str(), appHandler,  (void*)NULL))
            VEnableGrammar(vhdl, iter->first.c_str(), appHandler, (void*)NULL);
    }
}

void disableAlwaysOn(void)
{
    defaultAppList::iterator iter;
    for(iter = alwaysOnList.begin(); iter != alwaysOnList.end(); iter++) {
        if (! iter->second &&
            VGrammarExists(vhdl, iter->first.c_str(), appHandler,  (void*)NULL))
            VDisableGrammar(vhdl, iter->first.c_str());
    }
}
