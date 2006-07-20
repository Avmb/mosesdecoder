// $Id$

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

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

#pragma once

#include <iostream>
#include <vector>
#include <list>
#include <string>
#include "Word.h"
#include "WordsBitmap.h"
#include "TypeDef.h"
#include "Util.h"

class Phrase
{
	friend std::ostream& operator<<(std::ostream&, const Phrase&);
protected:
	FactorDirection				m_direction;
	size_t								m_phraseSize, m_arraySize;
	FactorArray						*m_factorArray;

public:
	inline Phrase()
		: m_phraseSize(0)
		, m_arraySize(0)
		, m_factorArray(NULL)
	{	// shouldn't be used. only for map
	}
	Phrase(const Phrase &copy);

	inline Phrase(FactorDirection direction)
		:m_direction(direction)
		, m_phraseSize(0)
		, m_arraySize(0)
		, m_factorArray(NULL)
	{
	}
	Phrase(FactorDirection direction, const Phrase &copy, const std::vector< const Word* > &mergeWords);

	virtual ~Phrase();

	static std::vector< std::vector<std::string> > Parse(const std::string &phraseString);
	void CreateFromString(const std::vector<FactorType> &factorOrder
											, const std::string &phraseString
											, FactorCollection &factorCollection);
	void CreateFromString(const std::vector<FactorType> &factorOrder
											, const std::vector< std::vector<std::string> > &phraseVector
											, FactorCollection &factorCollection);

	bool IsCompatible(const Phrase &inputPhrase) const;

	inline FactorDirection GetDirection() const
	{
		return m_direction;
	}

	inline size_t GetSize() const
	{
		return m_phraseSize;
	}
	inline const FactorArray &GetFactorArray(size_t pos) const
	{
		return m_factorArray[pos];
	}
	inline FactorArray &GetFactorArray(size_t pos)
	{
		return m_factorArray[pos];
	}
	inline const Factor *GetFactor(size_t pos, FactorType factorType) const
	{
		FactorArray &ptr = m_factorArray[pos];
		return ptr[factorType];
	}
	inline void SetFactor(size_t pos, FactorType factorType, const Factor *factor)
	{
		FactorArray &ptr = m_factorArray[pos];
		ptr[factorType] = factor;
	}

	bool Contains(const std::vector< std::vector<std::string> > &subPhraseVector
							, const std::vector<FactorType> &inputFactor) const;

	FactorArray &AddWord();
	void AddWords(const Phrase &copy);

	Phrase GetSubString(const WordsRange &wordsRange) const;

	TO_STRING

	// used to insert & find phrase in dictionary
	bool operator< (const Phrase &compare) const;
};

