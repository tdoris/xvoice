/*
 * Voice.h
 *
 * Description: interface to voice glue
 *
 * Copyright (c) 1999, David Z. Creemer.
 * Copyright (c) 2000, 2002 Brian Craft.
 * See the LICENSE file. All rights not granted therein are reserved.
 *
 * @author David Z. Creemer and Tom Doris
 * @author Brian Craft
 * $Revision: 1.16 $
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

#ifndef _VOICE_H
#define _VOICE_H

#include "WordInfoQueue.h"

#ifdef __cplusplus
extern "C" {
#endif

#define testbit(i,b) (1<<(b) & (i))
#define clearbit(i,b) (~(1<<(b) & (i)))

/*
 * A voice client.
 *
 * No elements are required.
 */

#define V_STATE_RUNNING 0x01
#define V_STATE_COMMAND 0x02
#define V_STATE_DICTATE 0x04

struct VClient {
  char const* name;
  void (*mic)(int state, void *data);
  void (*state)(int state, void *data);
  void (*reco)(const char *text, int firm, void *data);
  void (*error)(int err, const char* msg, void *data);
  void (*vocab)(const char *name, int active, void *data);
  void (*notifier)(int fd, int (*fn)(void*), void *fndata, void *data);
  void (*init)(void *data);
};
typedef struct VClient VClient;

enum VFlags {
  V_NAV,    /* We are the navigator */
  V_SAVE    /* Save audio */
};
typedef enum VFlags VFlags;

enum VAudioType { 
  AUDOSS, 
  AUDESD, 
  AUDARTS 
};
typedef enum VAudioType VAudioType;

int VOpen(const VClient *client, VAudioType lib, int flags, 
    void *data);
void VClose(int vh);
int VRun();

int VGetState();

enum VAppType {
  V_APP_CONSOLE,
  V_APP_X
};
typedef enum VAppType VAppType;

void VRegister(int vh, VAppType type, int id, /* Register application window */
    int add);
void VFocus(int vh, VAppType type, int id);   /* Focus a window (nav only) */ 
void VMicOn(int vh, int state);

/*
 * Result codes for vocab/grammar/dictation handlers.
 */

enum VVocabResult {
  V_VOCAB_SUCCESS,
  V_VOCAB_FAIL,
  V_VOCAB_RECO
};
typedef enum VVocabResult VVocabResult;

/*
 * Command grammar functions.
 */

typedef void (*VGramHdlr)(int vh, VVocabResult msgtype, const char *phrase, 
    const char *trans, void *user_data);

void VGrammarDir(const char *dir);
int VCompileGrammar(const char *filename, int trans);
int VCompileGrammarString(const char *name, const char *grammar, int trans);
void VEnableGrammar(int vh, const char *name, VGramHdlr callback, 
    void *data);
void VDisableGrammar(int vh, const char *name);
bool VGrammarExists(int vh, const char *name, VGramHdlr callback,
    void *data);

void xv_disable_compile();
void xv_enable_compile();
bool xv_get_disable_compile();

/*
 * Simple vocabulary functions.
 */

struct VVocab {
  int val;
  char const* phrase;
};
typedef struct VVocab VVocab;

typedef void (*VVocHdlr)(int vh, VVocabResult msgtype, const char *phrase, 
    int val, void *data);

void VDefineVocab(int vh, const char *name, VVocHdlr callback, 
    const VVocab *cmds, void *data);
void VEnableVocab(int vh, const char *name);
void VDisableVocab(int vh, const char *name);

/*
 * Dictation functions.
 */

typedef void (*VTextHdlr)(int vh, VVocabResult msgtype, int count, char const** text, 
    void *data);

void VStartDictation(int vh, VTextHdlr callback, void *data);
void VStopDictation(int vh);


/*
 * Engine parameter functions.
 */

/*
 * These are SMAPI specific. We need some sort of capabilities
 * API so we can determine these things dynamically.
 */

enum VEnginePerformance {
  V_PERF_FAST,
  V_PERF_BALANCED,
  V_PERF_ACCURATE
};
typedef enum VEnginePerformance VEnginePerformance;

struct VEngineParam {
  unsigned long text_to;
  unsigned long command_to;
  unsigned long unambiguous_to;
  unsigned long threshold;
  unsigned long threshold_min;
  unsigned long threshold_max;
  VEnginePerformance performance;
};
typedef struct VEngineParam VEngineParam;

void VGetEngineParam(int vh, VEngineParam *ep);
void VSetEngineParam(int vh, VEngineParam *ep);

  /* Word history */
// TODO remove from MainWin, wmMainWin, gnomeMainWin
// TODO make per target
extern WordInfoQueue *_mainWIQ;

#ifdef __cplusplus
}
#endif

#endif // _VOICE_H
