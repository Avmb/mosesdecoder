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
#include "Sentence.h"
#include <boost/algorithm/string.hpp>
#include "PhraseDictionary.h"
#include "TranslationOptionCollectionText.h"

int Sentence::Read(std::istream& in,const std::vector<FactorType>& factorOrder,
									 FactorCollection &factorCollection) 
{
	std::string line;
	do 
		{
			if (getline(in, line, '\n').eof())	return 0;
			boost::trim(line);
		} while (line == "");
	
	CreateFromString(factorOrder, line, factorCollection);
	return 1;
}

TargetPhraseCollection const* Sentence::
CreateTargetPhraseCollection(PhraseDictionaryBase const& d,
														 const WordsRange& r) const 
{
	Phrase src=GetSubString(r);
	return d.GetTargetPhraseCollection(src);
}

TranslationOptionCollection* 
Sentence::CreateTranslationOptionCollection() const 
{
	return new TranslationOptionCollectionText(*this);
}
