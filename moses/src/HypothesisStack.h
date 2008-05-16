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

#include <limits>
#include <map>
#include <set>

#include "Hypothesis.h"
#include "BitmapContainer.h"

class BitmapContainer;

typedef std::map<WordsBitmap, BitmapContainer*> _BMType;

/** defines less-than relation on hypotheses.
* The particular order is not important for us, we need just to figure out
* which hypothesis are equal based on:
*   the last n-1 target words are the same
*   and the covers (source words translated) are the same
*/
class HypothesisRecombinationOrderer
{
public:
	bool operator()(const Hypothesis* hypoA, const Hypothesis* hypoB) const
	{
		// Are the last (n-1) words the same on the target side (n for n-gram LM)?
		int ret = hypoA->NGramCompare(*hypoB);
//		int ret = hypoA->FastNGramCompare(*hypoB, m_NGramMaxOrder - 1);
		if (ret != 0)
		{
			return (ret < 0);
		}

		// same last n-grams. compare source words translated
		const WordsBitmap &bitmapA		= hypoA->GetWordsBitmap()
			, &bitmapB	= hypoB->GetWordsBitmap();
		ret = bitmapA.Compare(bitmapB);

		return (ret < 0);
	}
};


/** Stack for instances of Hypothesis, includes functions for pruning. */ 
class HypothesisStack 
{
private:
	typedef std::set< Hypothesis*, HypothesisRecombinationOrderer > _HCType;
public:
	typedef _HCType::iterator iterator;
	typedef _HCType::const_iterator const_iterator;
	friend std::ostream& operator<<(std::ostream&, const HypothesisStack&);

protected:
	_BMType m_bitmapAccessor;

	float m_bestScore; /**< score of the best hypothesis in collection */
	float m_worstScore; /**< score of the worse hypthesis in collection */
	float m_beamWidth; /**< minimum score due to threashold pruning */
	int m_kbestCubePruning;
	size_t m_maxHypoStackSize; /**< maximum number of hypothesis allowed in this stack */
	_HCType m_hypos; /**< contains hypotheses */
	bool m_nBestIsEnabled; /**< flag to determine whether to keep track of old arcs */
	
	/** add hypothesis to stack. Prune if necessary. 
	 * Returns false if equiv hypo exists in collection, otherwise returns true
	 */
	std::pair<HypothesisStack::iterator, bool> Add(Hypothesis *hypothesis);

	//! remove hypothesis pointed to by iterator but don't delete the object
	void Detach(const HypothesisStack::iterator &iter);
		
	/** destroy all instances of Hypothesis in this collection */
	void RemoveAll();
	/** destroy Hypothesis pointed to by iterator (object pool version) */
	inline void Remove(const HypothesisStack::iterator &iter);

public:
	//! iterators
	const_iterator begin() const { return m_hypos.begin(); }
	const_iterator end() const { return m_hypos.end(); }
	size_t size() const { return m_hypos.size(); }

	HypothesisStack();
	~HypothesisStack()
	{
		RemoveAll();
		m_bitmapAccessor.clear();
	}

	/** adds the hypo, but only if within thresholds (beamThr, stackSize).
	*	This function will recombine hypotheses silently!  There is no record
	* (could affect n-best list generation...TODO)
	* Call stack for adding hypothesis is
			AddPrune()
				Add()
					AddNoPrune()
	*/
	bool AddPrune(Hypothesis *hypothesis);

	void AddInitial(Hypothesis *hypo);

	/** set maximum number of hypotheses in the collection
   * \param maxHypoStackSize maximum number (typical number: 100)
   */
	inline void SetMaxHypoStackSize(size_t maxHypoStackSize)
	{
		m_maxHypoStackSize = maxHypoStackSize;
	}

	inline size_t GetMaxHypoStackSize() const
	{
		return m_maxHypoStackSize;
	}

	/** set beam threshold, hypotheses in the stack must not be worse than 
    * this factor times the best score to be allowed in the stack
	 * \param beamThreshold minimum factor (typical number: 0.03)
	 */
	inline void SetBeamWidth(float beamWidth)
	{
		m_beamWidth = beamWidth;
	}

	inline void SetKBestCubePruning(int kbestCubePruning)
	{
		m_kbestCubePruning = kbestCubePruning;
	}

	inline int GetKBestCubePruning() const
	{
		return m_kbestCubePruning;
	}

	/** return score of the best hypothesis in the stack */
	inline float GetBestScore() const
	{
		return m_bestScore;
	}
	
	void AddHypothesesToBitmapContainers();
	
	const _BMType& GetBitmapAccessor() const
	{
		return m_bitmapAccessor;
	}

	void SetBitmapAccessor(const WordsBitmap &newBitmap
						   , HypothesisStack &stack
						   , const WordsRange &range
						   , BitmapContainer &bitmapContainer
						   , const SquareMatrix &futureScore
						   , const TranslationOptionList &transOptList);

	/** pruning, if too large.
	 * Pruning algorithm: find a threshold and delete all hypothesis below it.
	 * The threshold is chosen so that exactly newSize top items remain on the 
	 * stack in fact, in situations where some of the hypothesis fell below 
	 * m_beamWidth, the stack will contain less items.
	 * \param newSize maximum size */
	void PruneToSize(size_t newSize);

	//! return the hypothesis with best score. Used to get the translated at end of decoding
	const Hypothesis *GetBestHypothesis() const;
	//! return all hypothesis, sorted by descending score. Used in creation of N best list
	std::vector<const Hypothesis*> GetSortedList() const;
	
	/** make all arcs in point to the equiv hypothesis that contains them. 
	* Ie update doubly linked list be hypo & arcs
	*/
	void CleanupArcList();
	
	TO_STRING();
};
