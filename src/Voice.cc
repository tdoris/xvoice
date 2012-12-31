/*
 * Voice.cc
 *
 * Description: Interface to IBM's ViaVoice SMAPI lib
 *
 * Copyright (c) 1999, David Z. Creemer, Tom Doris.
 * Copyright (c) 2000, 2001, 2002, Brian Craft.
 * See the LICENSE file. All rights not granted therein are reserved.
 *
 * @author David Z. Creemer
 * @author Tom Doris
 * @author Brian Craft
 * $Revision: 1.38 $
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
#include <cctype>
#include <cerrno>
#include <map>
#include <list>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <smapi.h>
#include <vtbnfc.h>

using namespace std;

#include "config.h"
#include "Error.h"
#include "Voice.h"
#include "xvoice.h"

// LogMessage defined in Error.h
#undef LogMessage
#define LogMessage(args...)

/* defines */
#define SMHANDLER_CAST SmHandler (*) (SM_MSG, PTRTYPE, PTRTYPE)

/* local variables */

#define MAX_ENGINE_ERROR 28
static char const* EngineErrors[] = {
  "Ok",
  "Api version",
  "No msg",
  "No conn",
  "No server",
  "Inval",
  "Unexp",
  "Timeout",
  "Unkmsg",
  "Msg size",
  "No handles",
  "Bad handle",
  "No mem",
  "Alloc",
  "Bad app name",
  "No accept",
  "All busy",
  "Create mbox failed",
  "Create event sem failed",
  "Assoc event sem failed",
  "Deallocating sh mem",
  "Queue close error",
  "Free mem error",
  "Close event sem failed",
  "Sub unset error",
  "Open queue failed",
  "Create mutex sem failed",
  "Attach mutex sem failed"
};

#define MAX_SMAPI_ERROR 178
static char const* SmapiErrors[] = {
  "Ok",
  "Not valid request",
  "Bad mode",
  "Not while mic on",
  "Mic already on",
  "Mic already off",
  "Mic on pending",
  "Mic off pending",
  "Not while playing",
  NULL,
  "Bad audio",              /* 10 */
  "Record open error",
  "Play open error",
  "Audio in use",
  "Bad audio protocol",
  "Audio timeout",
  "Audio disconnected",
  "Audio overrun",
  "Audio forced mic off",
  "No more audio files",
  "Bad ap",                 /* 20 */
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  "Bad deco",               /* 30 */
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  "Bad addword",            /* 40 */
  "Added",
  "Not added",
  "Multiple spellings",
  "Illegal spelling",
  "Illegal soundslike",
  "Mismatched acoustics",
  "Bad acoustics",
  "Spelling too long",
  "Acoustics too long",
  "Addword limit exceeded", /* 50 */
  "Extended vocab add",
  "Recording required",
  NULL, NULL, NULL, NULL, NULL, NULL,
  "Server error",           /* 60 */
  "Server malloc error",
  "Server file open error",
  "Server file write error",
  "Server file read error",
  "Server file close error",
  "Server process error",
  "Server terminated",
  NULL, NULL,
  "Bad tag",                /* 70 */
  "Bad uttno",
  "Bad message",
  NULL, NULL, NULL, NULL, NULL, NULL,
  "Not deleted",            /* 80 */
  "Not invocab",
  "Invocab",
  "Bad vocab",
  "Missing extern",
  NULL, NULL, NULL, NULL, NULL,
  "Bad userid",             /* 90 */
  "Bad enrollid",
  "Bad password",
  "Bad taskid",
  "Bad client",
  "Userid exists",
  "Enrollid exists",
  "Userid busy",
  "Enrollid busy",
  "Bad script",
  "Bad description",        /* 100 */
  "Enrollid running",
  "Enrollment not complete",
  "Mismatched language",
  "Mismatched alphabet",
  "Mismatched script",
  "Bad language",
  "Bad name",
  "Invalid window handle",
  NULL,
  "Bad item",               /* 110 */
  "Bad value",
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  "Busy last utterance",    /* 120 */
  "Busy word correction",
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  "No space",               /* 130 */
  "No space init reco",
  "No space init enroll",
  "No space term enroll",
  "No space mic on",
  NULL, NULL, NULL, NULL, NULL,
  "Invalid parm max len",   /* 140 */
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL,                     /* 150 */
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL,                     /* 160 */
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  "No focus app",           /* 170 */
  "Focus granted",
  "Focus request pending",
  "Focus denied",
  "Nav already defined",
  "Not in notify",
  "Exists in notify",
  "Document cached	",
  "Incompatible enrollment"
};

WordInfoQueue *_mainWIQ;

enum VError {
  V_ERROR_ENGINE = 1, 
  V_ERROR_CONNECT,
  V_ERROR_PARAM, 
  V_ERROR_DEF_VOCAB, 
  V_ERROR_UNDEF_VOCAB, 
  V_ERROR_EN_VOCAB, 
  V_ERROR_DIS_VOCAB, 
  V_ERROR_RECO, 
  V_ERROR_MISSING,
  V_ERROR_MIC_ON,
  V_ERROR_MIC_OFF,
  V_ERROR_VOICE,
  V_ERROR_COMPILE,
  V_ERROR_SYS,
  V_ERROR_TRANS
};

static char const* VoiceErrors[] = {
  "",
  "Voice engine error",
  "Error connecting to engine",
  "Invalid parameter",
  "Unable to define vocabulary or grammar",
  "Unable to undefine vocabulary or grammar",
  "Unable to enable vocabulary or grammar",
  "Unable to disable vocabulary or grammar",
  "Unable to continue recognition",
  "Unknown words in grammar",
  "Unable to turn on mic",
  "Unable to turn off mic",
  "Internal Voice lib error",
  "Grammar compilation failed",
  "System call error",
  "Error compiling translations"
};



//  map of current grammars
class grammar
{
  public:
    void *trans;
    VGramHdlr handler;
    void *user;
    grammar(VGramHdlr h, void *t, void *u) {
      handler = h;
      trans = t;
      user = u;
    }
    grammar() { };
};

typedef map<string, grammar> gramMap;
static gramMap actvGram;

class vocab
{
  public:
    const VVocab *trans;
    VVocHdlr handler;
    void *user;
    vocab(VVocHdlr h, const VVocab *t, void *u) {
      handler = h;
      trans = t;
      user = u;
    }
    vocab() { };
};

typedef map<string, vocab> vocList;
static vocList actvVoc;

static VTextHdlr curTextHdlr = NULL;
static void *curTextData = NULL;

static const VClient *vclient; /* XXX part of session struct */
static const char *grammarDir = "";
static void *initdata; /* XXX make part of session struct */
static unsigned int estate = 0;   /* engine state */
static int smapifd;
static int (*smapifn)(void*);
static void *smapifndata;


//  ViaVoice information
static char   userid   [80] = SM_USE_CURRENT;
static char   enrollid [80] = SM_USE_CURRENT;
static char   taskid   [80] = SM_USE_CURRENT;

/*  functions */

/*
 * Error handling
 */
static inline void verror(int err, const char* data);

static inline void LookupSmapiError(fstring& msg, int rc)
{
  if (rc > MAX_SMAPI_ERROR || SmapiErrors[rc] == NULL) {
    msg.appendf("Unknown smapi error: %d\n", rc);
  } else {
    msg.appendf(SmapiErrors[rc]);
  }
}

static inline void LookupVoiceError(fstring& msg, int err)
{
  msg.appendf("%s: ",VoiceErrors[err]);
}

static inline int _CheckCallError(int rc, int err, const char *context, 
    const char *fn)
{
  if (rc > 0) {
    fstring msg;

    msg.appendf("%s: ", fn);
    LookupVoiceError(msg, err);
    msg.appendf(" ");
    LookupSmapiError(msg, rc);
    if (context) {
        msg.appendf("\n  Context: \"");
        msg.appendf(context);
        msg.appendf("\"");
    }
    verror(err, msg.c_str());
    LogMessage(err, msg.c_str());
  }
  return rc;
}

/*
 * What are the error codes for unsolicited VV messages???
 */

/*
 * The CheckEngine*() routines extract error codes (if necessary) and check for
 * engine errors. If an engine error occurs, a client error() is dispatched.
 */

/*
 * For Async calls.
 */

static inline int _CheckEngineRC(int rc, const char *fn)
{
  if (rc < 0) {
    fstring msg;
    msg.appendf("%s: ", fn);
    if (-rc > MAX_ENGINE_ERROR || EngineErrors[-rc] == NULL) {
      msg.appendf(" Unknown engine error: %d\n", rc);
    } else {
      msg.appendf(" %s\n", EngineErrors[-rc]);
    }
    verror(V_ERROR_ENGINE, msg.c_str());
    LogMessage(V_ERROR_ENGINE, msg.c_str());
  }

  return rc;
}

/*
 * For Async replies
 */

static inline int _CheckEngineReply(SM_MSG reply, const char *fn)
{
  int rc;
  SmGetRc(reply, &rc);

  return _CheckEngineRC(rc,fn);
}

/*
 * For synchronous calls
 */

static inline int _CheckEngineSync(int rc, SM_MSG reply, const char *fn)
{
  if (rc != 0) {
    return _CheckEngineRC(rc,fn);
  }

  return _CheckEngineReply(reply,fn);
}

#define CheckEngineSync(rc,reply) _CheckEngineSync(rc,reply,__FUNCTION__)
#define CheckEngineReply(reply) _CheckEngineReply(reply,__FUNCTION__)
#define CheckEngineRC(rc) _CheckEngineRC(rc, __FUNCTION__)
#define CheckCallError(rc,err,ct) _CheckCallError(rc,err,ct,__FUNCTION__);

/*
 * Default client methods
 */

static inline void vmic(int state) {
  if (vclient->mic) vclient->mic(state, initdata);
};

static inline void vstate(int UNUSED(state)) {
  if (vclient->state) vclient->state(estate, initdata);
};

static inline void vreco(const char *text, int firm) {
  if (vclient->reco) vclient->reco(text, firm, initdata);
};

static inline void verror(int err, const char* data) {
  if (vclient->error) vclient->error(err, data, initdata);
};

static inline void vvocab(const char *name, int active) {
  if (vclient->vocab) vclient->vocab(name, active, initdata);
}

static inline int vnotifier(int fd, int (*fn)(void*), void *fndata, 
    void *data) 
{
  if (vclient->notifier) {
    vclient->notifier(fd, fn, fndata, data);
  } else {
    smapifd = fd;
    smapifn = fn;
    smapifndata = fndata;
  }
  return SM_RC_OK;
};

static inline void vinit()
{
  if (vclient->init) vclient->init(initdata);
}

void VSetEngineParam(int UNUSED(vh), VEngineParam *ep)
{
  SM_MSG reply;
  int rc;

  rc = SmSet(SM_TEXT_PHRASE_TIMEOUT, ep->text_to, &reply);
  rc = CheckEngineSync(rc, reply);
  if (rc != SM_RC_OK) {
    CheckCallError(rc, V_ERROR_PARAM, "");
    return;
  }

  rc = SmSet(SM_COMMAND_PHRASE_TIMEOUT, ep->command_to, &reply);
  rc = CheckEngineSync(rc, reply);
  if (rc != SM_RC_OK) {
    CheckCallError(rc, V_ERROR_PARAM, "");
    return;
  }

  rc = SmSet(SM_UNAMBIGUOUS_COMMAND_PHRASE_TIMEOUT, ep->unambiguous_to, &reply);
  rc = CheckEngineSync(rc, reply);
  if (rc != SM_RC_OK) {
    CheckCallError(rc, V_ERROR_PARAM, "");
    return;
  }

  rc = SmSet(SM_REJECTION_THRESHOLD, ep->threshold, &reply);
  rc = CheckEngineSync(rc, reply);
  if (rc != SM_RC_OK) {
    CheckCallError(rc, V_ERROR_PARAM, "");
    return;
  }

  dbgprintf(("setting %d\n",ep->performance));
  switch (ep->performance) {
    case V_PERF_FAST:
      rc = SmSet(SM_OPTIMIZE_PERFORMANCE, SM_OPTIMIZE_SPEED, &reply);
      break;
    case V_PERF_BALANCED:
      rc = SmSet(SM_OPTIMIZE_PERFORMANCE, SM_OPTIMIZE_DEFAULT, &reply);
      break;
    case V_PERF_ACCURATE:
      rc = SmSet(SM_OPTIMIZE_PERFORMANCE, SM_OPTIMIZE_ACCURACY, &reply);
      break;
  }

  rc = CheckEngineSync(rc, reply);
  if (rc != SM_RC_OK) {
    CheckCallError(rc, V_ERROR_PARAM, "");
  }

  return;
}

void VGetEngineParam(int UNUSED(vh), VEngineParam *ep)
{
  SM_MSG reply;
  unsigned long name;
  int rc;
  unsigned long perf;

  rc = SmQuery(SM_TEXT_PHRASE_TIMEOUT, &reply);
  rc = CheckEngineSync(rc, reply);
  if (rc != SM_RC_OK) {
    CheckCallError(rc, V_ERROR_PARAM, "text phrase timeout");
    return;
  }
  SmGetItemValue(reply, &name, &ep->text_to);

  rc = SmQuery(SM_COMMAND_PHRASE_TIMEOUT, &reply);
  rc = CheckEngineSync(rc, reply);
  if (rc != SM_RC_OK) {
    CheckCallError(rc, V_ERROR_PARAM, "command phrase timeout");
    return;
  }
  SmGetItemValue(reply, &name, &ep->command_to);

  rc = SmQuery(SM_UNAMBIGUOUS_COMMAND_PHRASE_TIMEOUT, &reply);
  rc = CheckEngineSync(rc, reply);
  if (rc != SM_RC_OK) {
    CheckCallError(rc, V_ERROR_PARAM, "unambiguous command phrase timeout");
    return;
  }
  SmGetItemValue(reply, &name, &ep->unambiguous_to);

  rc = SmQuery(SM_REJECTION_THRESHOLD, &reply);
  rc = CheckEngineSync(rc, reply);
  if (rc != SM_RC_OK) {
    CheckCallError(rc, V_ERROR_PARAM, "rejection threshold");
    return;
  }
  SmGetItemValue(reply, &name, &ep->threshold);

  ep->threshold_min = SM_MIN_REJECTION_THRESHOLD;
  ep->threshold_max = SM_MAX_REJECTION_THRESHOLD;

  rc = SmQuery(SM_OPTIMIZE_PERFORMANCE, &reply);
  rc = CheckEngineSync(rc, reply);
  if (rc != SM_RC_OK) {
    CheckCallError(rc, V_ERROR_PARAM, "optimize performance");
    return;
  }
  SmGetItemValue(reply, (unsigned long *)&name, &perf);

  switch (perf) {
    case SM_OPTIMIZE_ACCURACY:
      ep->performance = V_PERF_ACCURATE;
      break;
    case SM_OPTIMIZE_DEFAULT:
      ep->performance = V_PERF_BALANCED;
      break;
    case SM_OPTIMIZE_SPEED:
      ep->performance = V_PERF_FAST;
      break;
  }

  return;
}

int VGetState()
{
  return estate;
}

void VMicOn(int UNUSED(vh), int state)
{
  int rc;

  if (state) {
    rc = SmMicOn(SmAsynchronous);
  } else {
    rc = SmMicOff(SmAsynchronous);
  }
  CheckEngineRC(rc);
}

void VStartDictation(int vh, VTextHdlr callback, void *data)
{
  curTextHdlr = callback;
  curTextData = data;
  VEnableVocab(vh, "text");
  return;
}

void VStopDictation(int vh)
{
  VDisableVocab(vh, "text");
  return;
}

void VDisableVocab(int UNUSED(vh), const char *name)
{
  int rc;
  /* cast because VV fails to use const correctly */
  rc = SmDisableVocab((char*)name, SmAsynchronous);
  CheckEngineRC(rc);
  return;
}

void VEnableVocab(int UNUSED(vh), const char *name)
{
  int rc;
  /* cast because VV fails to use const correctly */
  rc = SmEnableVocab((char*)name, SmAsynchronous);
  CheckEngineRC(rc);
  return;
}

static SmHandler DefineVocabCB(SM_MSG reply, void *client,
    void *call_data);

void VDefineVocab(int UNUSED(vh), const char *name, VVocHdlr callback, 
    const VVocab *cmds, void *user)
{
  SM_MSG reply;
  SM_VOCWORD *voc_words;
  SM_VOCWORD **voc_ptrs;
  int i, count;
  int rc;

  for (count = 0; cmds[count].phrase != NULL; ++count);

  voc_words = (SM_VOCWORD *)malloc(count*sizeof(SM_VOCWORD));
  voc_ptrs = (SM_VOCWORD **)malloc(count*sizeof(SM_VOCWORD));
  /*
   * The SmDefineVocab call expects an array of pointers to SM_VOCWORD
   * structures rather than the SM_VOCWORDs themself
   */
  for (i = 0; i < count; i++) {
    dbgprintf(("vocab %s, cmd %s\n", name, cmds[i].phrase));
    voc_ptrs[i] = &(voc_words[i]);
    voc_words[i].spelling = const_cast<char*>(cmds[i].phrase);
    voc_words[i].spelling_size = strlen(voc_words[i].spelling);
    voc_words[i].flags = 0;
  }

  dbgprintf(("%d commands\n",i));
  /*
   * using synchronous mode here to avoid a race condition.
   * i'm not sure this is necessary
   */
  rc = SmDefineVocab((char*)name, i, voc_ptrs, &reply);
  rc = CheckEngineSync(rc, reply);
  if (rc != SM_RC_OK) {
    CheckCallError(rc, V_ERROR_DEF_VOCAB, name);
  } else {
    actvVoc[name] = vocab(callback, cmds, user);
    dbgprintf(("vocab count %d\n", actvVoc.size()));
    DefineVocabCB (reply, NULL, NULL);
  }

  free(voc_words);
  free(voc_ptrs);
  return;
}

static SmHandler SetCB(SM_MSG UNUSED(reply), void* UNUSED(client), void* UNUSED(call_data))
{
  return SM_RC_OK;
}

static SmHandler MicOnCB(SM_MSG reply, void* UNUSED(client), void* UNUSED(call_data))
{
  int rc;
  rc = CheckEngineReply(reply);
  switch (rc) {
    case SM_RC_MIC_OFF_PENDING: /* MicStateCB should handle these */
    case SM_RC_MIC_ON_PENDING:
    case SM_RC_MIC_ALREADY_ON:
    case SM_RC_OK:
      break;
    default:
      vmic(false);
      CheckCallError(rc, V_ERROR_MIC_ON, "");
      return SM_RC_OK;
  }

  /*
   * VERY IMPORTANT - this tells the recognizer to 'go' (ie. start
   * capturing the audio and processing it)
   */
  rc = SmRecognizeNextWord(SmAsynchronous);
  CheckEngineRC(rc);
  return SM_RC_OK;
}

static SmHandler MicOffCB(SM_MSG reply, void* UNUSED(client), void* UNUSED(call_data))
{
  int rc;
  rc = CheckEngineReply(reply);
  switch (rc) {
    case SM_RC_MIC_OFF_PENDING: /* MicStateCB should handle these */
    case SM_RC_MIC_ON_PENDING:
    case SM_RC_MIC_ALREADY_OFF:
    case SM_RC_OK:
      break;
    default:
      vmic(true);
      CheckCallError(rc, V_ERROR_MIC_OFF, "");
      break;
  }
  return SM_RC_OK;
}

static SmHandler MicStateCB(SM_MSG reply, void* UNUSED(client), void* UNUSED(call_data))
{
  unsigned long state;
  int rc;

  rc = CheckEngineReply(reply);
  CheckCallError(rc, V_ERROR_VOICE, "unexpected mic state error");
  if (rc != SM_RC_OK) return SM_RC_OK;
  SmGetMicState(reply, &state);
  if (state == SM_NOTIFY_MIC_ON) 
    vmic(true);
  else 
    vmic(false);

  return SM_RC_OK;
}

static SmHandler EnableVocabCB(SM_MSG reply, void* UNUSED(client), void* UNUSED(call_data))
{
  char *vocab;
  int rc;
  unsigned int newstate = estate;

  SmGetVocabName(reply, &vocab);
  dbgprintf(("%s\n", vocab));

  rc = CheckEngineReply(reply);
  if (rc == 0) {
    vvocab(vocab, true);

    if (!strcmp(vocab, "text")) {
      newstate = estate | V_STATE_DICTATE;
    } else {
      newstate = estate | V_STATE_COMMAND;
    }

    if (newstate != estate) {
      estate = newstate;
      vstate(estate);
    }
  } else {
    CheckCallError(rc, V_ERROR_EN_VOCAB, vocab);

    /* engine errors (rc < 0) already handled */
    /* XXX -- notify vocab handler??? */
  } 
  
  return SM_RC_OK;
}

static SmHandler DisableVocabCB(SM_MSG reply, void* UNUSED(client), void* UNUSED(call_data))
{
  char *vocab;
  int rc;
  unsigned int newstate = estate;

  SmGetVocabName(reply, &vocab);
  dbgprintf(("%s\n", vocab));

  rc = CheckEngineReply(reply);

  if (rc == 0) {
    vvocab(vocab, false);

    if (!strcmp(vocab, "text")) {
      newstate = estate^(estate&V_STATE_DICTATE);

    } else {
      if (actvGram.size() == 0) 
        newstate = estate^(estate&V_STATE_COMMAND);
    }

    if (newstate != estate) {
      estate = newstate;
      vstate(estate);
    }
  } else {
    CheckCallError(rc, V_ERROR_DIS_VOCAB, vocab);

    /* engine errors (rc < 0) already handled */
    /* XXX -- notify vocab handler??? */
  } 
  return SM_RC_OK;
}

static SmHandler DefineGrammarCB(SM_MSG reply, void* UNUSED(client), void* UNUSED(call_data))
{
  int rc;
  char *vocab;
  fstring msg, filename;
  SM_VOCWORD *missing;
  unsigned long i, num_missing;

  rc = CheckEngineReply(reply);
  SmGetVocabName(reply, &vocab);

  if (rc != SM_RC_OK) {
    actvGram[vocab].handler(0, V_VOCAB_FAIL, NULL, NULL,
        actvGram[vocab].user);
    actvGram.erase(vocab);

    switch (rc) { 
      case SM_RC_NOT_INVOCAB: 
      case SM_RC_MISSING_EXTERN:
        SmGetVocWords(reply, &num_missing, &missing);
        msg.appendf("Missing %d word(s) from '%s': %s\n",
            num_missing, vocab, msg.c_str());
        for (i = 0; i < num_missing; i++)
          msg.appendf("%s ", missing[i].spelling);

        /* CB return codes are not documented. wtf is this -1 for? */
        /*return (-1);*/
        break;
      default:
        msg.appendf(vocab);
        break;
    }

    CheckCallError(rc, V_ERROR_DEF_VOCAB, msg.c_str());
    return SM_RC_OK;
  }

  dbgprintf(( "Defined '%s'\n", vocab));

  /* translations */
  void *tr;
  struct stat st;
  filename.appendf("%s/%s.fst", grammarDir, vocab);
  if (stat(filename.c_str(), &st) == 0) {
    dbgprintf(( "translation is %s\n", filename.c_str()));
    if (VtLoadFSG((char*)filename.c_str(), &tr)) {
      msg.appendf(VoiceErrors[V_ERROR_TRANS]);
      msg.appendf(": %s", filename.c_str());
      verror(V_ERROR_TRANS, msg.c_str());
      LogMessage(V_ERROR_TRANS, msg.c_str());
      actvGram[vocab].handler(0, V_VOCAB_FAIL, NULL, NULL,
          actvGram[vocab].user);
      actvGram.erase(vocab);
      rc = SmUndefineVocab((char*)vocab, SmAsynchronous);
      CheckEngineRC(rc);
      return SM_RC_OK;
    } else {
      actvGram[vocab].trans = tr;
    }
  }

  /*
   * XXX -- Note that we send the success message prematurely here.
   */
  actvGram[vocab].handler(0, V_VOCAB_SUCCESS, NULL, NULL, actvGram[vocab].user);
  rc = SmEnableVocab(vocab, SmAsynchronous);
  CheckEngineRC(rc);
  if (rc != SM_RC_OK) {
    actvGram[vocab].handler(0, V_VOCAB_FAIL, NULL, NULL,
        actvGram[vocab].user); /* XXX clean up all these duplicate failures */
    actvGram.erase(vocab);
  }
  return SM_RC_OK;
}

static SmHandler FocusCB(SM_MSG reply, void* UNUSED(client), void* UNUSED(call_data))
{
  int rc;

  CheckEngineReply(reply);
  /* XXX -- implement me */
  rc = SmRecognizeNextWord(SmAsynchronous);
  CheckEngineRC(rc);
  return (SM_RC_OK);
}


static int addToWordHistory (char const** words, unsigned long arrayLength, 
    void *firm, bool isCommand);

/*
 * This method is a callback which is invoked when speech has been recognised
 * when speaking while a _grammar_ (not dynamic vocab nor dictation) is active.
 * Note: the WordInfo API considers a grammar recognition to be a command.
 *
 * TODO: find a way to tell if a grammar recognition is actually plain text and
 * therefore not actually a command.
 */

static SmHandler RecoPhraseCB(SM_MSG reply, void* UNUSED(client), void* UNUSED(call_data))
{
  int rc;
  SM_WORD *firm;
  unsigned long num_firm;
  unsigned int i;
  char *vocab ;
  unsigned long flags;
  char const** words;
  char* translation;
  fstring phrase;

  SmGetVocabName(reply, &vocab);
  rc = CheckEngineReply(reply);
  if (rc != SM_RC_OK) {
    CheckCallError(rc, V_ERROR_RECO, vocab);
    goto phrasecont;
  }

  SmGetPhraseState(reply, &flags);
  SmGetFirmWords(reply, &num_firm, &firm);

  /*
   * Concatenate recognised words
   */

  phrase = firm [0].spelling;
  for (i = 1 ; i < num_firm ; i++)
    phrase.appendf( " %s", firm [i].spelling );

  dbgprintf(("%s\n",phrase.c_str()));
  if (!(flags & SM_PHRASE_ACCEPTED)) {    /* not recognised */
    vreco(phrase.c_str(), false);
    goto phrasecont;
  }

  vreco(phrase.c_str(), true);

  words = (char const**)malloc(sizeof(char const*)*(num_firm+1));
  addToWordHistory(words, num_firm, firm, true);

  /* 
   * translate the array of strings "words" into the output for the grammar
   * macro it represents 
   */

  if (actvGram[vocab].trans != NULL) {
    if (VtGetTranslation(actvGram[vocab].trans, const_cast<char**>(words), &translation)) {
      char *msg;
      VtGetMessage(&msg);
      verror(V_ERROR_TRANS, msg);
      LogMessage(V_ERROR_TRANS, msg);
    } 
  }
  free(words);
  actvGram[vocab].handler(0, V_VOCAB_RECO, phrase.c_str(),
      translation, actvGram[vocab].user);

phrasecont:

  rc = SmRecognizeNextWord(SmAsynchronous);
  CheckEngineRC(rc);
  return SM_RC_OK;
}

static SmHandler DefineVocabCB(SM_MSG reply, void* UNUSED(client), void* UNUSED(call_data))
{
  char *vocab;
  int rc;
  SM_VOCWORD *missing;
  unsigned long i, num_missing;
  fstring msg;

  SmGetVocabName(reply, &vocab);

  rc = CheckEngineReply(reply);
  if (rc != SM_RC_OK) {
    actvVoc[vocab].handler(0, V_VOCAB_FAIL, msg.c_str(), 0, 
        actvVoc[vocab].user);
    actvVoc.erase(vocab);

    CheckCallError(rc, V_ERROR_DEF_VOCAB, vocab);
    return SM_RC_OK;
  }

  /*
   * Check to see if any of the words from the vocabulary are missing
   * from the recognizers pool(s)
   */

  SmGetVocWords(reply, & num_missing, & missing );

  if (num_missing) {
    msg.appendf("Missing %ld word(s) from '%s': ", num_missing, vocab);
    for (i = 0; i < num_missing; i++)
      msg.appendf("%s ", missing[i].spelling);

    verror(V_ERROR_MISSING, msg.c_str());
    LogMessage(V_ERROR_MISSING, msg.c_str());
  }

  actvVoc[vocab].handler(0, V_VOCAB_SUCCESS, NULL, 0, actvVoc[vocab].user);
  return SM_RC_OK;
}

/*
 * This callback handler is called when a vocabulary is undefined
 * It only prints an error message (or debugging information)
 */
static SmHandler UndefineVocabCB(SM_MSG reply, void* UNUSED(client),
    void* UNUSED(call_data))
{
  char *vocab;
  int rc;
  fstring msg;

  rc = CheckEngineReply(reply);
  SmGetVocabName(reply, &vocab);

  dbgprintf(("Undefining vocabulary %s\n", vocab));

  CheckCallError(rc, V_ERROR_UNDEF_VOCAB, vocab);
  return SM_RC_OK;
}

/*
 * This loads the .fsg grammar file
 */

void VEnableGrammar(int UNUSED(vh), const char *name, VGramHdlr callback, 
    void *user)
{
  int rc;
  fstring filename;

  if (name == NULL || actvGram.find(name) != actvGram.end()) return;
  filename.appendf( "%s/%s.fsg", grammarDir, name );
  dbgprintf (( "grammar is %s\n", filename.c_str() ));

  /* 
   * XXX - There is a potential problem here if someone clears the grammars
   * before this is enabled. 
   */

  actvGram[name]=grammar(callback, (void *)NULL, user);
  /*
   * IMPORTANT: the define grammar callback enables the new grammar 
   * for us to ensure everything's ok
   */
  rc = SmDefineGrammar ((char*)name, (char*)filename.c_str(), 0, 
      SmAsynchronous);
  CheckEngineRC(rc);

  return;
}

bool VGrammarExists(int UNUSED(vh), const char *name, VGramHdlr UNUSED(callback),
    void* UNUSED(user))
{

  fstring filename;
  filename.appendf( "%s/%s.fsg", grammarDir, name );
  FILE *fp;

  fp = fopen(filename.c_str(), "r");

  if (fp == NULL)
    return false;

  fclose(fp);
  return true;
}

void VDisableGrammar(int UNUSED(vh), const char *name)
{
  int rc;

  if (actvGram.find(name) == actvGram.end()) return; /* XXX throw error */
  dbgprintf(("removing %s\n", name));
  VtUnloadFSG(actvGram[name].trans);
  actvGram.erase(name);
  rc = SmDisableVocab((char*)name, SmAsynchronous );  
  CheckEngineRC(rc);
  rc = SmUndefineVocab((char*)name, SmAsynchronous);
  CheckEngineRC(rc);
  return;
}

/*
 * Dunno what this is about. Perhaps it's called on error after
 * SmRecognizeNextWord(), instead of the three Reco callbacks.
 */
static SmHandler GetNextWordCB(SM_MSG reply, void* UNUSED(client), void* UNUSED(call_data))
{
  int rc;
  rc = CheckEngineReply(reply);
  CheckCallError(rc, V_ERROR_RECO, "");

  return SM_RC_OK;
}

/*
 * FIXME: If this is too slow we should use a hash to look up
 * commands.
 * Walk through all the entries in all the currently active
 * vocabularies; when you match an entry to "word," call the
 * vocabulary's handler on the matched phrase, and return true.
 */

static bool matchCommand(char *word)
{
  const VVocab *t;

  vocList::iterator voc;
  for (voc=actvVoc.begin(); voc!=actvVoc.end(); voc++) {
    for (t = voc->second.trans; t->phrase != NULL; ++t) {
      if (!strcmp(t->phrase, word)) {
        voc->second.handler(0, V_VOCAB_RECO, t->phrase, t->val, 
            voc->second.user);
        return true;
      }
    }
  }
  return false;
}

/*
 * This method is called when a word from a dynamic vocab has been recognised.
 * A dynamic vocabulary is one which is loaded/defined at runtime.
 * Note: this is called, for example, when "start dictation" or "stop
 * dictation" are recognized; in those cases, the two words are
 * recognized as one word with a space in it by the engine.
 */

static SmHandler RecoWordCB(SM_MSG reply, void* UNUSED(client), void* UNUSED(call_data))
{
  int rc;
  SM_WORD *firm;
  unsigned long i, num_firm;
  char *vocab;
  fstring msg;
  char const** words;

  rc = CheckEngineReply(reply);
  SmGetVocabName(reply, &vocab);
  if (rc != SM_RC_OK) {
    CheckCallError(rc, V_ERROR_RECO, vocab);
    goto wordcont;
  }

  rc = SmGetFirmWords(reply, &num_firm, &firm);

  for (i = 0 ; i < num_firm; i++) {
    if (firm[i].spelling[0] == '\0') {
      vreco("(unknown)", true);
    } else {
      vreco(firm[i].spelling, true);
      if (!matchCommand(firm[i].spelling)) {
        msg.appendf("Recognized word we don't have a record of (%s).\n"
            "This should never happen.\n", firm[i].spelling);
        verror(V_ERROR_VOICE, msg.c_str());
        LogMessage(V_ERROR_VOICE, msg.c_str());
      }
    }
  }

  // Save the recognized word(s) in the word history
  words = (char const**)malloc(sizeof(char const*)*(num_firm+1));
  addToWordHistory(words, num_firm, firm, true);
  free(words);

wordcont:
  rc = SmRecognizeNextWord(SmAsynchronous);
  CheckEngineRC(rc);
  return (SM_RC_OK);
}

/*
 * This method is called when a chunk of text has been recognised in dictation
 * mode. The method sends this text to the target application
 */

static SmHandler RecoTextCB(SM_MSG reply, void* UNUSED(client), void* UNUSED(call_data))
{
  int rc;
  unsigned long num_firm;
  SM_WORD *firm;

  rc = CheckEngineReply(reply);
  CheckCallError(rc, V_ERROR_RECO, "text");
  if (rc == SM_RC_OK) {
    if (curTextHdlr == NULL) {
      char const* msg = "Attempt to dictate with no handler set!";
      verror(V_ERROR_VOICE, msg);
      LogMessage(V_ERROR_VOICE, msg);
    } else {
      rc = SmGetFirmWords(reply, &num_firm, &firm);
      char const** words = (char const**)malloc(sizeof(char*)*(num_firm+1));
      addToWordHistory(words, num_firm, firm, false);
      for (unsigned i = 0 ; i < num_firm; i++) {
        vreco(words[i], true);
      }
      curTextHdlr(0, V_VOCAB_RECO, num_firm, words, curTextData); 
      free(words);
    }
  }
  return (SM_RC_OK);
}

static SmHandler UtteranceCB(SM_MSG reply, void* UNUSED(client), void* UNUSED(call_data))
{
  int rc;
  rc = CheckEngineReply(reply);
  return SM_RC_OK;
}

static SmHandler DisconnectCB(SM_MSG reply, void* UNUSED(client), void* UNUSED(call_data))
{
  int rc;
  rc = CheckEngineReply(reply);
  return SM_RC_OK;
}

static SmHandler EngineErrorCB(SM_MSG reply, void* UNUSED(client), void* UNUSED(call_data))
{
  int rc;
  rc = CheckEngineReply(reply);
  return SM_RC_OK;
}

void VGrammarDir(const char *dir)
{
  grammarDir = dir; /* XXX -- copy this value */
}

void VRegister(int UNUSED(vh), VAppType UNUSED(type), int UNUSED(id), int UNUSED(add))
{
  /* XXX write me */
}

void VFocus(int UNUSED(vh), VAppType UNUSED(type), int UNUSED(id))
{
  /* XXX write me */
}

int VRun()
{
  fd_set readfs;
  int i = 0;

  while (i >= 0 && smapifn != NULL) {
    FD_ZERO(&readfs);
    FD_SET(smapifd, &readfs);
    i = select(smapifd+1, &readfs, NULL, NULL, NULL);
    smapifn(smapifndata);
  }

  return 0;
}

void VClose(int UNUSED(vh))
{
  SM_MSG reply;
  int rc;
  /* 
   * This better be synchronous, because we're about to lose
   * our event loop & our connect to the voice engine goes with it.
   */
  rc = SmDisconnect(0, NULL, &reply);
  CheckEngineRC(rc);
}

static SmHandler ConnectCB(SM_MSG reply, void* UNUSED(client), void* UNUSED(call_data))
{
  int rc;
  rc = CheckEngineReply(reply);
  CheckCallError(rc, V_ERROR_CONNECT, "");
  if (rc == SM_RC_OK) {
    vinit();
  }
  return SM_RC_OK;
}

/*
 * This is called almost immediately after the app launches
 * it initialises the speech engine
 */
int VOpen(const VClient *cl, VAudioType lib, int flags, void *data)
{
  static int first = true;
  int        rc;
  int        smc;
  SmArg      smargs [30];
  static     int input_id; /* XXX this fails for > 1 file */
  static char const* audiolibs[] = {
    "pcm;audoss.so;;foo",
    "pcm;libxvesd.so;;foo",
    "pcm;libxvarts.so;;foo",
  };

  vclient = cl;
  initdata = data; /* XXX session struct */

  if (first)
  {

    smc = 0;
    SmSetArg(smargs [smc], const_cast<char*>(SmNapplicationName), vclient->name);   smc++;
    SmSetArg(smargs [smc], const_cast<char*>(SmNexternalNotifier), vnotifier); smc++;
    SmSetArg(smargs [smc], const_cast<char*>(SmNexternalNotifierData), &input_id);   smc++;

    /*
     * The call to SmOpen initializes any data that's inside of libSm
     */
    rc = SmOpen(smc, smargs);
    CheckEngineRC(rc);

    switch (rc) {
      case SM_RC_OK:
        break;
        /* XXX -- handle NAVIGATOR_ALREADY_DEFINED */
      case SM_RC_OPEN_SYNCH_QUEUE_FAILED:
        LogMessage(V_ERROR_VOICE, "Creating default user");
        system("$SPCH_BIN/vvuser -adduser $USER");
        rc = SmOpen(smc, smargs);
        CheckEngineRC(rc);
        if (rc == SM_RC_OK) {
          break;
        }
        /* fall through */
      default:
        CheckCallError(rc, V_ERROR_ENGINE, "");
        return -1;
    }

    /*
     * Add the callbacks to catch the messages coming back from the
     * reco engine
     */
    SmAddCallback(const_cast<char*>(SmNconnectCallback),
        (SMHANDLER_CAST)ConnectCB,       NULL);
    SmAddCallback(const_cast<char*>(SmNdisconnectCallback),
        (SMHANDLER_CAST)DisconnectCB,    NULL);
    SmAddCallback(const_cast<char*>(SmNsetCallback),
        (SMHANDLER_CAST)SetCB,           NULL);
    SmAddCallback(const_cast<char*>(SmNmicOnCallback),
        (SMHANDLER_CAST)MicOnCB,         NULL);
    SmAddCallback(const_cast<char*>(SmNmicOffCallback),
        (SMHANDLER_CAST)MicOffCB,        NULL);
    SmAddCallback(const_cast<char*>(SmNmicStateCallback),
        (SMHANDLER_CAST)MicStateCB,      NULL);
    SmAddCallback(const_cast<char*>(SmNenableVocabCallback),
        (SMHANDLER_CAST)EnableVocabCB,   NULL);
    SmAddCallback(const_cast<char*>(SmNdisableVocabCallback),
        (SMHANDLER_CAST)DisableVocabCB,  NULL);
    SmAddCallback(const_cast<char*>(SmNdefineVocabCallback),
        (SMHANDLER_CAST)DefineVocabCB,   NULL);    
    SmAddCallback(const_cast<char*>(SmNdefineGrammarCallback),
        (SMHANDLER_CAST)DefineGrammarCB, NULL);
    SmAddCallback(const_cast<char*>(SmNrecognizeNextWordCallback),
        (SMHANDLER_CAST)GetNextWordCB,   NULL);
    SmAddCallback(const_cast<char*>(SmNrecognizedWordCallback),
        (SMHANDLER_CAST)RecoWordCB,      NULL);  
    SmAddCallback(const_cast<char*>(SmNrecognizedPhraseCallback),
        (SMHANDLER_CAST)RecoPhraseCB,    NULL);
    SmAddCallback(const_cast<char*>(SmNrecognizedTextCallback),
        (SMHANDLER_CAST)RecoTextCB,      NULL);
    SmAddCallback(const_cast<char*>(SmNutteranceCompletedCallback),
        (SMHANDLER_CAST)UtteranceCB,  NULL);
    SmAddCallback(const_cast<char*>(SmNfocusGrantedCallback),
        (SMHANDLER_CAST)FocusCB,    NULL);
    SmAddCallback(const_cast<char*>(SmNundefineVocabCallback),
        (SMHANDLER_CAST)UndefineVocabCB,  NULL);
    SmAddCallback(const_cast<char*>(SmNreportEngineErrorCallback),
        (SMHANDLER_CAST)EngineErrorCB,  NULL);

    
    first = false;
  }

  smc = 0;
  SmSetArg(smargs [smc], const_cast<char*>(SmNuserId),       userid   );  smc++;
  SmSetArg(smargs [smc], const_cast<char*>(SmNenrollId),     enrollid );  smc++;
  SmSetArg(smargs [smc], const_cast<char*>(SmNtask),         taskid   );  smc++;
  SmSetArg(smargs [smc], const_cast<char*>(SmNrecognize),    true     );  smc++;
  if (testbit(flags,V_NAV)) {
    dbgprintf(("Navigator mode\n"));
    SmSetArg(smargs [smc], const_cast<char*>(SmNnavigator),    true     );  smc++;
  }
  SmSetArg(smargs [smc], const_cast<char*>(SmNoverrideLock), true     );  smc++;
  SmSetArg(smargs [smc], const_cast<char*>(SmNaudioHost), audiolibs[lib]);  smc++;

  rc = SmConnect(smc, smargs, SmAsynchronous);
  CheckEngineRC(rc);
  CheckCallError(rc, V_ERROR_PARAM, "SmConnect");
  if (rc != SM_RC_OK) {
    return -1;
  }

  SM_MSG reply;

  if (testbit(flags,V_SAVE)) {
    rc = SmSet(SM_SAVE_AUDIO, true, &reply);
    rc = CheckEngineSync(rc, reply);
    CheckCallError(rc, V_ERROR_PARAM, "");
    if (rc != SM_RC_OK) {
      return -1;
    }
  }

  if (vclient->mic) {
    rc = SmSet(SM_NOTIFY_MIC_STATE, true, &reply);
    rc = CheckEngineSync(rc, reply);
    CheckCallError(rc, V_ERROR_PARAM, "");
    if (rc != SM_RC_OK) {
      return -1;
    }
  }

  estate = estate | V_STATE_RUNNING;
  return 0; /* XXX voicehandle */
}

bool disable_vvcompile = false;

void
xv_disable_compile() 
{
    disable_vvcompile = true;
}

void
xv_enable_compile() 
{
    disable_vvcompile = false;
}

bool
xv_get_disable_compile()
{
    return disable_vvcompile;
}

int VCompileGrammar(const char *filename, int trans)
{
  fstring ifilename;
  fstring ofilename;
  const char *dir;
  VtArg vtargs[5];
  int vtc;
  int rc;

  if (disable_vvcompile)
      return 0;

  if (strchr(filename, '/') == NULL) { /* no path */
    dir = grammarDir;
  } else {
    dir = ".";
  }

  ifilename.appendf("%s/%s.bnf", dir, filename);
  ofilename.appendf("%s/%s.fsg", dir, filename);

  vtc = 0;
  VtSetArg(vtargs[vtc], const_cast<char*>(VtNbnfFile),       ifilename.c_str());  vtc++;
  VtSetArg(vtargs[vtc], const_cast<char*>(VtNfsgFile),       ofilename.c_str());  vtc++;
  VtSetArg(vtargs[vtc], const_cast<char*>(VtNenMode),        1);  vtc++;
  VtSetArg(vtargs[vtc], const_cast<char*>(VtNfsgFlags), SM_PHRASE_ALLOW_SILENCES);  vtc++;

  
  string status_strip = "compiling " + ifilename;  
  xvoice_status(status_strip.c_str());

  rc = VtCompileGrammar(vtc, vtargs);
  
  if (rc != 0) {
    char *msg;
    VtGetMessage(&msg);
    verror(V_ERROR_COMPILE, msg);
    LogMessage(V_ERROR_COMPILE, msg);
    return -1;
  }

  if (trans) {
#define VTCOMPILE_BUG
#ifdef VTCOMPILE_BUG
    /*
     * We have to suspend handling stopped children so we don't have
     * a race with somebody closing an app we spawned while we're
     * in waitpid here.
     */
    struct sigaction act, oldact;
    act.sa_handler = SIG_DFL;
    sigemptyset(&act.sa_mask);
    sigaction(SIGCHLD, &act, &oldact);
    /*
     * this code consistantly segfaults on grammars that
     * reference constituent expressions, e.g. (from the docs):
     *    my {<pet>} has fleas -> {1} scratches.
     * However, the output file is written before the segfault.
     */

    int child = fork();
    if (child < 0) {
      verror(V_ERROR_SYS, strerror(errno));
      LogMessage(V_ERROR_SYS, strerror(errno));
      return -1;
    }
    if (child) {
      int status;
      int count=6;
      while(!waitpid(child, &status, WNOHANG)) {
          count--;
          if (count == 1) {
              fprintf(stderr, "ViaVoice compilation hung (unfortuantely normal).  Trying to recover...\n");
              kill(child, SIGTERM);
              sleep(1);
              continue;
          }
          sleep(1);
          if (! count) {
              fprintf(stderr, "Boy it was really hung.\n");
              kill(child, SIGKILL);
              break;
          }
      }
    } else {
        signal(SIGSEGV, SIG_IGN);
#endif
      ofilename.resize(0);
      ofilename.appendf("%s/%s.fst", dir, filename);

      vtc = 0;
      VtSetArg(vtargs[vtc], const_cast<char*>(VtNbnfFile),       ifilename.c_str());  vtc++;
      VtSetArg(vtargs[vtc], const_cast<char*>(VtNfsgFile),       ofilename.c_str());  vtc++;
      VtSetArg(vtargs[vtc], const_cast<char*>(VtNtrMode),        1);  vtc++;
      VtSetArg(vtargs[vtc], const_cast<char*>(VtNfsgFlags), SM_PHRASE_ALLOW_SILENCES);  vtc++;
      VtCompileGrammar(vtc, vtargs);
#ifdef VTCOMPILE_BUG
      _exit(0);
    }
    /*
     * Restore handling of stopped children.
     */
    sigaction(SIGCHLD, &oldact, NULL);
#endif
  }

  status_strip = "compiled " + ifilename;  
  xvoice_status(status_strip.c_str());

  return 0;
}

int VCompileGrammarString(const char *name, const char *grammar, int trans)
{
  FILE *fp;
  fstring filename = name;

  filename.appendf(".bnf");
  fp = fopen(filename.c_str(), "w");
  if (fp == NULL) {
      verror(V_ERROR_SYS, strerror(errno));
      LogMessage(V_ERROR_SYS, strerror(errno));
      return -1;
  }
  fprintf(fp, grammar);
  fclose(fp);
  return VCompileGrammar(filename.c_str(), trans);
}

/* 
 * Given an empty string array, and an array length, and an SM_WORD which
 * contains recognized words, copy the words from the SM_WORD into the
 * string array and also add them to the end of the word history
 * (a WordInfoQueue).
 */
int addToWordHistory (char const** words, unsigned long arrayLength,
    void *firm, bool isCommand) {

  /*
   * TODO: find a way of dynamically dealing with words of more than
   * 512 characters.
   */
  int wordSize = 511;
  char *word = (char*)malloc(sizeof(char*)*(wordSize));
  word[0] = '\0';

  const char *space = " ";

  unsigned int i;
  WordInfo *wi;
  SM_WORD *firmSmWord = (SM_WORD *)firm;

  /*
   * Garbage collect
   * FIXME: no, do it every _mainWIQH->length() mod some number
   */
  /*int collected =*/ _mainWIQ->doGC();

  /*
   * FIXME: debugging code
   */
#if 0
  printf ("Collected %d\n", collected);
#endif

  // now append the firm words to the end of the queue
  for (i = 0 ; i < arrayLength ; i++) {
    if (strcmp(firmSmWord[i].spelling,"\\") == 0) {
      /* Quote '\' coming from recognizer */
      words[i] = "\\\\";
    } else {
      words[i]=firmSmWord[i].spelling;
    }

    if (isCommand) {
      /*
       * Create a single string with spaces in it out of the
       * newly-recognized phrase
       */
      strcat(word, words[i]);
      if (i != arrayLength - 1)
	strcat(word, space);
    } else {
      // create a WordInfo for each word and append it to the queue
      wi = new WordInfo(words[i], false);
      _mainWIQ->append(wi);
    }
  }

  /*
   * if it was a phrase, we have a string; create a WordInfo and
   * append it to the queue here.
   */
  if (isCommand) {
    wi = new WordInfo(word, false);
    _mainWIQ->append(wi);

    // TODO: note that this WI is a command
  }

  words[i]=0;

  // FIXME debugging code
  /*
  WordInfoQueue::infoList::iterator iter = _mainWIQ->begin();

  for (int i = 0; i < _mainWIQ->size(); i++) {
    const char *letters = (*iter).getLetters();
    printf("%s - ", letters);
    iter++;
  }
  printf ("\n");
  */
  return (0);
}
