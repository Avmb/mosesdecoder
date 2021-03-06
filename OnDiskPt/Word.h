#pragma once
// $Id$
/***********************************************************************
 Moses - factored phrase-based, hierarchical and syntactic language decoder
 Copyright (C) 2009 Hieu Hoang

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***********************************************************************/
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <boost/shared_ptr.hpp>
#include "Vocab.h"

namespace Moses
{
class Word;
}

namespace OnDiskPt
{
class Vocab;

/* A wrapper around a vocab id, and a boolean indicating whther it is a term or non-term.
 * Factors can be represented by using a vocab string with | character, eg go|VB
 */
class Word
{
  friend std::ostream& operator<<(std::ostream&, const Word&);

protected:
  bool m_isNonTerminal;
  UINT64 m_vocabId;

public:
  explicit Word()
  {}

  explicit Word(bool isNonTerminal)
  :m_isNonTerminal(isNonTerminal)
  ,m_vocabId(0)
  {}

  Word(const Word &copy);
  ~Word();


  void CreateFromString(const std::string &inString, Vocab &vocab);
  bool IsNonTerminal() const {
    return m_isNonTerminal;
  }

  size_t WriteToMemory(char *mem) const;
  size_t ReadFromMemory(const char *mem);
  size_t ReadFromFile(std::fstream &file);

  void SetVocabId(UINT32 vocabId) {
    m_vocabId = vocabId;
  }

  void ConvertToMoses(
    const std::vector<Moses::FactorType> &outputFactorsVec,
    const Vocab &vocab,
    Moses::Word &overwrite) const;

	virtual void DebugPrint(std::ostream &out, const Vocab &vocab) const;

  int Compare(const Word &compare) const;
  bool operator<(const Word &compare) const;
  bool operator==(const Word &compare) const;

};

typedef boost::shared_ptr<Word> WordPtr;
}

