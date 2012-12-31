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
 * WordInfoQueue is class for maintaining a linked list of WordInfo
 * objects and metadata about them.
 */

#ifndef _WORDINFOQUEUE_H
#define _WORDINFOQUEUE_H

#include <list>
#include "WordInfo.h"

class WordInfoQueue {
  public:

    typedef std::list<WordInfo> infoList;

  // constructor
    WordInfoQueue();

  // destructor
  ~WordInfoQueue();

  WordInfo *getNewest() {
    return &(_wordInfos.front());
  }

  /* Adds a new item. The "beginning" of the stl list (returned by
     list.begin()) contains the newest word.*/
  void append(WordInfo *wi) {
    _wordInfos.push_front(*wi);
  }

  /* Remove all objects farther back than, and including the element
     at, pos. */
  int deleteBefore(int pos);

  /* Return the number of objects in this list. */
  int size() {
    return (_wordInfos.size());
  }

  /* return the maximum length of this queue before it
     should be reaped (garbage-collected). */
  int maxSize();

  /* return an iterator pointing to the beginning of the list */
  infoList::iterator begin();

  /* remove the appropriate number of WordInfoQueues from the
   queue in order to remain at or maxLength. Return the number
   removed. */
  int doGC();

private:
  infoList _wordInfos; /* list of WordInfo objects */
  infoList::iterator _curWordInfo;
  int _maxSize;
};

#endif /* _WORDINFOQUEUE_H */
