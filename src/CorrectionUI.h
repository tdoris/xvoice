// -*- C++ -*-

/*
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

/* CorrectionUI. A UI for interacting with the Corrector. Note that
 * objects in this class will create a Corrector object and pass
 * significant amounts of processing to it.
 */

#ifndef CorrectionUI
#define CorrectionUI

class Corrector {
  public:

    /* constructor */
    CorrectionUI (WordInfoQueueHead *wordList, WordInfoQueue
        *newest, WordInfoQueue *oldest,
        Window *current_application);

    /* destructor */
    ~CorrectionUI();

  protected:

    /* 
     * display a list of alternates for the indexth newest word in the
     * list 
     */
    void display_alternates(int index);

    /*
     * request that the Corrector replace the specified word in the
     * specified application with a new spelling, presumably by sending
     * the appropriate set of backspaces and new keystrokes. 
     */

    void edit_word(int index, char *correct_spelling, Window *win);

    /*
     * request that the Corrector "train" the engine that the word it
     * misheard at the indexth newest point in this list of words was
     * actually spelled as specified by new_spelling.
     */
    void train_word(int index, char *new_spelling);

    /*
     * request that the Corrector add word at the indexth newest point
     * in this word list. 
     */
    void add_word(WordInfo *word, int index);

    /*
     * if from_index and to_index are legal, request that the Corrector
     * move the from_indexth newest word in the list to the to_indexth
     * newest spot. 
     */
    void move_word(int from_index, int to_index);

  private:
    Corrector *_corrector;

}

#endif /* CorrectionUI */
