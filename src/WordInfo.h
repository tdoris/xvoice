// -*- C++ -*-

/**
 * WordInfo.h
 *
 * Description: declaration of a class which preserves information about
 * words recognized by the speech engine.
 *
 * Copyright (c) 2001, Deborah Kaplan and Jessica Perry Hekman
 * See the LICENSE file. All rights not granted therein are reserved.
 *
 * @author Deborah Kaplan
 * @author Jessica Perry Hekman
 * $Revision: 1.5 $
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

#ifndef _WORDINFO_H
#define _WORDINFO_H

#include <stddef.h>
#include <string.h>

class WordInfo {
  public:
    // constructor
    WordInfo();

    // constructor
    WordInfo(const char *letters);

    // constructor
    WordInfo(const char *letters, bool command);

    // destructor
    ~WordInfo();

    /*
     * return a list of strings of alternate possibilities for this word.
     * Return NULL by default.
     */
    const char **getAlternates();

    /* return the characters in this word: its letters (spelling). */
    const char *getLetters();

    /* set the letters (spelling) of this word */
    void setLetters(const char *letters);

    /*
     * can this particular WordInfo be trained? Reasons
     * it might not include: implemented with an engine which does not
     * train; entered by hand and not associated with any pronunciation
     */
    bool trainable();

    /* is this word a command? */
    bool command();

    /* return the number of characters in this word */
    int length();

    /* if this WordInfo is modified, can the application
       with which it is associated be notified of that change? */
    bool editable();

    /* set the edited spelling of this word. See the vocabulary for the
       definition of "edit." */
    void edit_letters(char *letters);

    /*
      indicates order in which this word was created relative to all
      WordInfos which exist. This will be used for when the user is
      switching between targets, and needs to get a list of recent
      words. */
    long index;

  private:
    const char *_letters;
    int _wordLength;
    bool _trainable;
    bool _command;
    bool _editable;

    // add bool command here in the future?
};

#endif /* _WORDINFO_H */
