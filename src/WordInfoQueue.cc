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
 * The WordInfo class handles information about a recognized "word"
 * (see the definition of "word" in the above vocabulary).
 */

#include "sys.h"
#include "WordInfoQueue.h"

WordInfoQueue::WordInfoQueue() {
  // FIXME make user-settable
  _maxSize = 50;
}

WordInfoQueue::~WordInfoQueue() {
  delete &_wordInfos;
}

int WordInfoQueue::deleteBefore(int pos) {
  _wordInfos.resize(pos);
  return pos;
}

int WordInfoQueue::maxSize() {
  return (_maxSize);
}

WordInfoQueue::infoList::iterator WordInfoQueue::begin() {
  return _wordInfos.begin();
}

int WordInfoQueue::doGC () {
  int deleted = 0;

  if (size() > maxSize()) {
    deleted = size() - maxSize();
    deleteBefore(maxSize());
  }

  return deleted;
}

