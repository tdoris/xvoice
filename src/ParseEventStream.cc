/*
 * ParseEventStream.cc
 *
 * Description: parse an event stream XML, and create an EventStream object
 *
 * Copyright (c) 1999, David Z. Creemer, Tom Doris.
 * Copyright (c) 2000, Brian Craft.
 * Copyright (c) 2001, Jessica Perry Hekman
 * See the LICENSE file. All rights not granted therein are reserved.
 *
 * @author David Z. Creemer
 * @author Tom Doris
 * @author Brian Craft
 * @author Jessica Perry Hekman
 * $Revision: 1.27 $
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
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "MainWin.h"
#include "expat/xmlparse.h"
#include "EventStream.h"
#include "ParseEventStream.h"
#include "Error.h"
#include "keys.h"
#include "Voice.h"
#include "App.h"

using std::stack;

enum appType { APPAPP, APPDEF, APPVOC };
struct configCtx {
  application* app;
  appType type;
  applicationList* applist;
  bool inCdata;
  const char *gDir;
  int gFd;
};

typedef struct configCtx configCtx;

/* userData is the EventStream object created in ParseFile */

static void startElement( void *userData, const char *name,
    const char **attrs )
{
  configCtx *ctx = (configCtx *)userData;
  const char *aname = NULL, *expr = NULL, *dictation = NULL, *before = NULL;
  const char *enableOn = NULL, *alwaysOn = NULL;

  if (strcasecmp(name, "xvoice") == 0) return;

  dbgprintf(("name %s\n", name));
  while  (*attrs != NULL) {
    dbgprintf(("attribute %s\n", *attrs));
    if (strcasecmp(*attrs, "name") == 0)
      aname = *++attrs;
    else if (strcasecmp(*attrs, "expr") == 0)
      expr = *++attrs;
    else if (strcasecmp(*attrs, "dictation") == 0)
      dictation = *++attrs;
    else if (strcasecmp(*attrs, "before") == 0)
      before = *++attrs;
    else if (strcasecmp(*attrs, "enableOn") == 0)
      enableOn = *++attrs;
    else if (strcasecmp(*attrs, "alwaysOn") == 0)
        alwaysOn = *++attrs;
    else
      attrs++;
    attrs++;
  }

  if (aname == NULL) {
    LogMessage (E_FATAL,
        "No application name given in '%s' in grammar!\n",
        name);
    exit (-1);
    // FIXME fix LogMessage so it exits even if no gnomeMainWin exists
  }

  if (strcasecmp(name, "application") == 0) {
    if (expr == NULL) expr = aname;
    if (dictation == NULL) dictation = "";
    if (before == NULL) before = "";
    ctx->app = new application(aname,expr,dictation,before);
    ctx->type = APPAPP;
  } else if (strcasecmp(name, "vocab") == 0) {
      ctx->app = new application(aname,"","", "");
      ctx->type = APPVOC;
  } else if (strcasecmp(name, "define") == 0) {
    ctx->app = new application(aname,"","");
    ctx->type = APPDEF;
  }
  if (enableOn) {
      add_auto_grammar(enableOn, aname);
  }
  if (alwaysOn) {
      add_alwayson(aname, ((strcmp(alwaysOn,"true") == 0 ? true : false)));
  }
}

static void endElement(void *userData, const char *name)
{
  configCtx *ctx = (configCtx *)userData;
  if(strcasecmp(name, "application") == 0 ) {
    for(applicationList::iterator li = ctx->applist->begin();
        li != ctx->applist->end(); li++) {
        if (strcmp(li->name, ctx->app->name) == 0) {
            // override the previous entry in the list with the new
            // one.  Delete this one.
            ctx->applist->erase(li);  // XXX: mem leak?
            break;
        }
    }
    if (ctx->app->before) {
        for(applicationList::iterator li = ctx->applist->begin();
            li != ctx->applist->end(); li++) {
            if (strcmp(li->name, ctx->app->before) == 0) {
                // Put the application *before* the one listed.
                ctx->applist->insert(li, *ctx->app);
                return;
            }
        }
    }
    ctx->applist->push_back(*ctx->app);
  } else if (strcasecmp(name, "vocab") == 0 ) {
    delete ctx->app;
  } else if (strcasecmp(name, "define") == 0 ) {
    delete ctx->app;
  }
}

static void endCdataSection(void *userData)
{
  configCtx *ctx = (configCtx *)userData;
  char *name = ctx->app->name;
  close(ctx->gFd);
  dbgprintf(("wrote bnf\n"));
  if (ctx->type != APPDEF) {
    VCompileGrammar(name, 1);
  }
  ctx->inCdata = false;
}

static void startCdataSection(void *userData) 
{
  configCtx *ctx = (configCtx *)userData;
  close(ctx->gFd);
  fstring buff;
  buff.appendf("%s/%s.bnf", ctx->gDir, ctx->app->name);
  dbgprintf(("opening bnf %s\n",buff.c_str()));
  ctx->gFd = open( buff.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0644);
  if ( ctx->gFd == -1 ) {
    LogMessage(E_FATAL, "Could not open bnf file %s\n", buff.c_str());
    exit (-1);
    // FIXME fix LogMessage so it exits even if gnomeMainWin doesn't exit
  }

  ctx->inCdata = true;
}

static void handleCharData(void *userData, const XML_Char *s, int len)
{
  configCtx *ctx = (configCtx *)userData;
  if (ctx->inCdata == true) {
    dbgprintf(("writing %d chars\n",len));
    if (write(ctx->gFd, s, len) < 0) {
      // FIXME figure out name of BNF and include in error message
      LogMessage(E_FATAL, "Error writing to bnf\n");
      // FIXME make LogMessage exit even if no gnomeMainWin
      exit (-1);
    }
  }
}


/* 
 * Parse the grammar file (filename) and place the results in the
 * grammar directory (gdir).
 */
applicationList* parseFile( const char* filename, const char* gdir,
                            applicationList* es)
{
  configCtx ctx;
  ctx.gDir = gdir;
  ctx.gFd = -1;
  ctx.inCdata = false;
  int fd = open(filename, O_RDONLY);
  bool createdhere = false;

  if (!es) {
      es = new applicationList();
      createdhere = true;
  }
  
  ctx.applist = es;

  if (fd == -1) {
    if (createdhere)
      delete es;
    return NULL;
  }

  off_t fsize = lseek(fd, 0, SEEK_END);

  char* bytes = (char*) mmap(NULL, fsize, PROT_READ, MAP_SHARED, fd, 0);

  if (bytes != MAP_FAILED) {
    int done;
    XML_Parser parser = XML_ParserCreate(NULL);
    // es becomes the "userData" arg to the handlers:
    XML_SetUserData(parser, &ctx);
    XML_SetElementHandler(parser, startElement, endElement);
    XML_SetCharacterDataHandler(parser, handleCharData);
    XML_SetCdataSectionHandler(parser, startCdataSection, endCdataSection);
    do {
      if (!XML_Parse(parser, bytes, fsize, done)) {
        LogMessage(E_CONFIG, "%s at line %d in %s\n",
                   XML_ErrorString(XML_GetErrorCode(parser)),
                   XML_GetCurrentLineNumber(parser),
                   filename);

        if (createdhere)
          delete es;
        return NULL;
      }
    } while (!done);

    XML_ParserFree(parser);
    munmap(bytes, fsize);
  } else {
    LogMessage(E_FATAL, "MMAP error");
    if (createdhere)
      delete es;
    return NULL;
  }

  close(fd);

  return es;
}


static void startElementB(void *userData, const char *name, const char **attrs)
{
  dbgprintf(("name %s attr %s\n", name, attrs[0]));
  stack<EventStream*>* ess = (stack<EventStream*>*)userData;
  EventStream *es = ess->top();
  if (strcasecmp(name, "key") == 0) {
    es->parseKeyEvent(attrs);
  } else if (strcasecmp(name, "mouse") == 0) {
    es->parseMouseEvent(attrs);
  } else if (strcasecmp(name, "grammar") == 0) {
    es->parseGrammarEvent(attrs);
  } else if (strcasecmp(name, "setCase") == 0) {
    es->parseSetCaseEvent(attrs);
  } else if (strcasecmp(name, "setSpace") == 0) {
    es->parseSetSpacingEvent(attrs);
  } else if (strcasecmp(name, "pause") == 0) {
    es->parsePauseEvent(attrs);
  } else if (strcasecmp(name, "repeat") == 0) {
    ess->push(new EventStream());
    es = ess->top();
    es->parseRepeatEvent(attrs);
  } else if (strcasecmp(name, "call") == 0) {
    es->parseCallEvent(attrs);
  } else if (strcasecmp(name, "setStatus") == 0) {
    es->parseSetStatusEvent(attrs);
  }
}

static void endElementB(void *userData, const char *name)
{
  stack<EventStream*>* ess = (stack<EventStream*>*)userData;
  EventStream *es = ess->top();
  dbgprintf(("name %s\n", name));
  if(strcasecmp(name, "repeat") == 0 ) {
    if (es->front().type != EVREPEAT) {
      LogMessage(E_CONFIG,
          "Unmatched \"repeat\" element in xvoice.xml\n");
      return;
    }
    int count = es->front().val.repeat;
    es->pop_front();
    ess->pop();
    EventStream *les = ess->top();
    while (count--) {
      EventStream ces(*es);
      les->splice(les->end(), ces);
    }
    delete es;
  }
}


EventStream* parseBuff(const char* bytes)
{
  stack<EventStream*> ess;
  EventStream *es;
  off_t fsize = strlen(bytes);
  XML_Parser parser = XML_ParserCreate(NULL);
  ess.push(new EventStream());
  // es becomes the "userData" arg to the handlers:
  XML_SetUserData(parser, &ess);
  XML_SetElementHandler(parser, startElementB, endElementB);
#define PARSE_INIT "<!DOCTYPE EXAMPLE  [\n" KEYS "]>\n <s>"
  XML_Parse(parser, PARSE_INIT, strlen(PARSE_INIT), 0); /* keeps the parser happy */
  if (!XML_Parse(parser, bytes, fsize, 0))
  {
    LogMessage(E_CONFIG, "%s at line %d\n",
        XML_ErrorString(XML_GetErrorCode(parser)),
        XML_GetCurrentLineNumber(parser));

    while (ess.empty() == false) {
      es = ess.top();
      ess.pop();
      delete es;
    }
    return NULL;
  }
  XML_Parse(parser, "</s>", 3, 1); /* keeps the parser happy */

  XML_ParserFree(parser);

  while (ess.empty() == false) {
    es = ess.top();
    ess.pop();
    if (ess.empty() == false) delete es;
  }
  return es;
}
