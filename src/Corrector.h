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
 * $Revision: 1.3 $
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
 * Corrector edits (interacting with applications) and/or trains
 * (interacting with the engine). Interacts with WordInfo and
 * WordInfoQueue and WordInfoQueueHead.
 */

#ifndef _CORRECTOR_H
#define _CORRECTOR_H

class Corrector {
  public:

    /* 
     * constructor. Takes a WIQH, and two WIQs which delineate points within it
     * which will be treated as ends of the list of words to be handled. No
     * WordInfos newer than newest or older than oldest will be modified in any
     * way by this Corrector. Restriction: the WordInfoQueues must be in the
     * list pointed to by the WordInfoQueueHead (wordList).
     */
    Corrector (WordInfoQueueHead *wordList, WordInfoQueue *newest, WordInfoQueue
        *oldest);

    // destructor
    ~Corrector();

    /*
     * get the alternates list for the indexth newest word in the list
     * of words handled by this object.
     */
    char **get_alternate(int index);

    /*
     * replace the specified word in the specified application with a
     * new spelling, presumably by sending the appropriate set of
     * backspaces and new keystrokes; saves this change in xvoice's
     * representation of the voice buffer (see the definition of "voice
     * buffer" in the vocabulary). [Note: make less GUI-specific by not
     * using Window.]  
     */
    void edit_word(int index, char *correct_spelling, Window *win);

    /*
     * interact with the engine to "train" it that the word it misheard
     * at the indexth newest point in this list of words was actually
     * spelled as specified by new_spelling. 
     */
    void train_word(int index, char *new_spelling);

    /*
     * add word at the indexth newest point in this
     * WordInfoQueueHead.
     */
    void add_word(int index, WordInfo *word);

    /* create a new WordInfo */
    WordInfo *create_word(char *spelling);

    /*
     * if from_index and to_index are legal, move the from_indexth
     * newest word in the list to the to_indexth newest spot.
     */
    void move_word(int from_index, int to_index);

}

#endif /* _CORRECTOR_H */
