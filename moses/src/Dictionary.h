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

#include <vector>

class FactorTypeSet;

class Dictionary
{
protected:
	const size_t m_noScoreComponent;
	std::vector< FactorTypeSet* > m_factorsUsed;

	Dictionary(size_t noScoreComponent)
		:m_noScoreComponent(noScoreComponent)
		,m_factorsUsed(2)
	{
	}

public:
	const FactorTypeSet &GetFactorsUsed(FactorDirection direction) const
	{
		return *m_factorsUsed[direction];
	}
	virtual ~Dictionary()
	{
	}
	size_t GetNoScoreComponent() const
	{
		return m_noScoreComponent;
	}
	
	virtual DecodeType GetDecodeType() const = 0;
};
