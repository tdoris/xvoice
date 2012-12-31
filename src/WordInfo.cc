// -*- C++ -*-

/**
 * WordInfo.cc
 *
 * Description: declaration of a class which preserves information about
 * words recognized by the speech engine.
 *
 * Copyright (c) 2001, Deborah Kaplan and Jessica Perry Hekman
 * See the LICENSE file. All rights not granted therein are reserved.
 *
 * @author Deborah Kaplan
 * @author Jessica Perry Hekman
 * $Revision: 1.4 $
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

/*
 * The WordInfo class handles information about a recognized "word"
 * (see the definition of "word" in the above vocabulary).
 */

#include "sys.h"
#include "WordInfo.h"

// constructors
WordInfo::WordInfo() {
  _wordLength = 0;
  _letters = NULL;
  _trainable = false;
  _command = false;
  _editable = false;
}

WordInfo::WordInfo(const char *letters) {
  _wordLength = strlen(letters);
  _letters = new char[_wordLength + 1];
  strcpy((char *)_letters, letters);
  _trainable = false;
  _command = false;
  _editable = false;
}

WordInfo::WordInfo(const char *letters, bool command) {
  _wordLength = strlen(letters);
  _letters = new char[_wordLength + 1];
  strcpy((char *)_letters, letters);
  _trainable = false;
  _command = command;
  _editable = false;
}

// destructor
WordInfo::~WordInfo() {
  delete[] _letters;
}

/*
 * return a list of strings of alternate possibilities for this word.
 * Return NULL by default.
 */
const char **WordInfo::getAlternates() {
  return (NULL);
}

/* return the characters in this word: its letters (spelling). */
const char *WordInfo::getLetters() {
  return (_letters);
}

/* set the letters (spelling) of this word */
void WordInfo::setLetters(const char *letters) {
  _letters = letters;
}

/*
 * can this particular WordInfo be trained? Reasons
 * it might not include: implemented with an engine which does not
 * train; entered by hand and not associated with any pronunciation
 */
bool WordInfo::trainable() {
  return (_trainable);
}

bool WordInfo::command() {
  return (_command);
}

/* return the number of characters in this word */
int WordInfo::length(void) {
  return(_wordLength);
}

void WordInfo::edit_letters(char *letters) {
  // FIXME: should we copy (using strcpy) to get our own copy of this string?
  _letters = letters;
}

/*
 * TODO: add bool command here in the future?
 * add vocab_info getter here in the future?
 */
