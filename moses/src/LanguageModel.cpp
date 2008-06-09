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

#include <cassert>
#include <limits>
#include <iostream>
#include <sstream>

#include "LanguageModel.h"
#include "TypeDef.h"
#include "Util.h"
#include "FactorCollection.h"
#include "Phrase.h"
#include "StaticData.h"

using namespace std;

LanguageModel::LanguageModel(bool registerScore, ScoreIndexManager &scoreIndexManager) 
{
	if (registerScore)
		scoreIndexManager.AddScoreProducer(this);
}
LanguageModel::~LanguageModel() {}

// don't inline virtual funcs...
size_t LanguageModel::GetNumScoreComponents() const
{
	return 1;
}

void LanguageModel::CalcScore(const Phrase &phrase
														, float &fullScore
														, float &ngramScore) const
{
	fullScore	= 0;
	ngramScore	= 0;

	size_t phraseSize = phrase.GetSize();
	vector<const Word*> contextFactor;
	contextFactor.reserve(m_nGramOrder);

	// start of sentence
	for (size_t currPos = 0 ; currPos < m_nGramOrder - 1 && currPos < phraseSize ; currPos++)
	{
		contextFactor.push_back(&phrase.GetWord(currPos));		
		fullScore += GetValue(contextFactor);
	}
	
	if (phraseSize >= m_nGramOrder)
	{
		contextFactor.push_back(&phrase.GetWord(m_nGramOrder - 1));
		ngramScore = GetValue(contextFactor);
	}
	
	// main loop
	for (size_t currPos = m_nGramOrder; currPos < phraseSize ; currPos++)
	{ // used by hypo to speed up lm score calc
		for (size_t currNGramOrder = 0 ; currNGramOrder < m_nGramOrder - 1 ; currNGramOrder++)
		{
			contextFactor[currNGramOrder] = contextFactor[currNGramOrder + 1];
		}
		contextFactor[m_nGramOrder - 1] = &phrase.GetWord(currPos);
		float partScore = GetValue(contextFactor);		
		ngramScore += partScore;		
	}
	fullScore += ngramScore;	
}

LanguageModel::State LanguageModel::GetState(const std::vector<const Word*> &contextFactor, unsigned int* len) const
{
  State state;
	unsigned int dummy;
  if (!len) len = &dummy;
  GetValue(contextFactor,&state,len);
  return state;
}

