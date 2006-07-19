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

#include <limits>
#include <cmath>
#include "Manager.h"
#include "TypeDef.h"
#include "Util.h"
#include "TargetPhrase.h"
#include "HypothesisCollectionIntermediate.h"
#include "LatticePath.h"
#include "LatticePathCollection.h"
#include "TranslationOption.h"
#include "LMList.h"

using namespace std;

Manager::Manager(const Sentence &sentence, StaticData &staticData)
:m_source(sentence)
,m_hypoStack(sentence.GetSize() + 1)
,m_staticData(staticData)
,m_possibleTranslations(sentence)
{
	std::vector < HypothesisCollection >::iterator iterStack;
	for (iterStack = m_hypoStack.begin() ; iterStack != m_hypoStack.end() ; ++iterStack)
	{
		HypothesisCollection &sourceHypoColl = *iterStack;
		sourceHypoColl.SetMaxHypoStackSize(m_staticData.GetMaxHypoStackSize());
		sourceHypoColl.SetBeamThreshold(m_staticData.GetBeamThreshold());
	}
}

Manager::~Manager()
{
}

void Manager::ProcessSentence()
{
	
	list < DecodeStep > &decodeStepList = m_staticData.GetDecodeStepList();

	PhraseDictionary &phraseDictionary = decodeStepList.front().GetPhraseDictionary();
	const LMList &lmListInitial = m_staticData.GetLanguageModel(Initial);
	// create list of all possible translations
	// this is only valid if:
	//		1. generation of source sentence is not done 1st
	//		2. initial hypothesis factors are given in the sentence
	//CreateTranslationOptions(m_source, phraseDictionary, lmListInitial);
	m_possibleTranslations.CreateTranslationOptions(decodeStepList
  														, lmListInitial
  														, m_staticData.GetAllLM()
  														, m_staticData.GetFactorCollection()
  														, m_staticData.GetWeightWordPenalty()
  														, m_staticData.GetDropUnknown()
  														, m_staticData.GetVerboseLevel());

	// output
	//TRACE_ERR (m_possibleTranslations << endl);


	// seed hypothesis
	{
	Hypothesis *hypo = new Hypothesis(m_source, m_possibleTranslations.GetInitialCoverage());
	TRACE_ERR(m_possibleTranslations.GetInitialCoverage().GetWordsCount() << endl);
#ifdef N_BEST
	LMList allLM = m_staticData.GetAllLM();
	hypo->ResizeComponentScore(allLM, decodeStepList);
#endif
	m_hypoStack[m_possibleTranslations.GetInitialCoverage().GetWordsCount()].AddPrune(hypo);
	}
	
	// go thru each stack
	std::vector < HypothesisCollection >::iterator iterStack;
	for (iterStack = m_hypoStack.begin() ; iterStack != m_hypoStack.end() ; ++iterStack)
	{
		HypothesisCollection &sourceHypoColl = *iterStack;
		sourceHypoColl.PruneToSize(m_staticData.GetMaxHypoStackSize());
		sourceHypoColl.InitializeArcs();
		//sourceHypoColl.Prune();
		ProcessOneStack(sourceHypoColl);
		//ProcessOneStack(decodeStepList, sourceHypoColl);

		//OutputHypoStack();
		OutputHypoStackSize();
	}

	// output
	//OutputHypoStack();
	//OutputHypoStackSize();
}

const Hypothesis *Manager::GetBestHypothesis() const
{
	// best
	const HypothesisCollection &hypoColl = m_hypoStack.back();
	return hypoColl.GetBestHypothesis();
}

void Manager::ProcessOneStack(HypothesisCollection &sourceHypoColl)
{
	// go thru each hypothesis in the stack
	HypothesisCollection::iterator iterHypo;
	for (iterHypo = sourceHypoColl.begin() ; iterHypo != sourceHypoColl.end() ; ++iterHypo)
	{
		Hypothesis &hypothesis = **iterHypo;
		ProcessOneHypothesis(hypothesis);
	}
}
void Manager::ProcessOneHypothesis(const Hypothesis &hypothesis)
{
	HypothesisCollectionIntermediate outputHypoColl;
	CreateNextHypothesis(hypothesis, outputHypoColl);

	// add to real hypothesis stack
	HypothesisCollectionIntermediate::iterator iterHypo;
	for (iterHypo = outputHypoColl.begin() ; iterHypo != outputHypoColl.end() ; )
	{
		Hypothesis *hypo = *iterHypo;

		hypo->CalcScore(m_staticData.GetLanguageModel(Initial)
									, m_staticData.GetLanguageModel(Other)
									, m_staticData.GetWeightDistortion()
									, m_staticData.GetWeightWordPenalty()
									, m_possibleTranslations.GetFutureScore(), m_source);
//		if(m_staticData.GetVerboseLevel() > 2) 
//		{			
//			hypo->PrintHypothesis(m_source, m_staticData.GetWeightDistortion(), m_staticData.GetWeightWordPenalty());
//		}
		size_t wordsTranslated = hypo->GetWordsBitmap().GetWordsCount();

		if (m_hypoStack[wordsTranslated].AddPrune(hypo))
		{
			HypothesisCollectionIntermediate::iterator iterCurr = iterHypo++;
			outputHypoColl.Detach(iterCurr);
			if(m_staticData.GetVerboseLevel() > 2) 
				{
					if(m_hypoStack[wordsTranslated].getBestScore() == hypo->GetScore(ScoreType::Total))
						{
							cout<<"new best estimate for this stack"<<endl;
						}
					cout<<"added hypothesis on stack "<<wordsTranslated<<" now size "<<m_hypoStack[wordsTranslated].size()<<endl<<endl;
				}

		}
		else
		{
			++iterHypo;
		}
	}

}
void Manager::CreateNextHypothesis(const Hypothesis &hypothesis, HypothesisCollectionIntermediate outputHypoColl)
{
	int maxDistortion = m_staticData.GetMaxDistortion();
	if (maxDistortion < 0)
	{	// no limit on distortion
		TranslationOptionCollection::const_iterator iterTransOpt;
		for (iterTransOpt = m_possibleTranslations.begin(); iterTransOpt != m_possibleTranslations.end(); ++iterTransOpt)
		{
			const TranslationOption &transOpt = *iterTransOpt;

			if ( !transOpt.Overlap(hypothesis)) 
			{
				Hypothesis *newHypo = hypothesis.CreateNext(transOpt);
				//newHypo->PrintHypothesis(m_source);
				outputHypoColl.AddNoPrune( newHypo );			
			}
		}
	}
	else
	{
		const WordsBitmap hypoBitmap = hypothesis.GetWordsBitmap();
		size_t hypoWordCount		= hypoBitmap.GetWordsCount()
			,hypoFirstGapPos	= hypoBitmap.GetFirstGapPos();

		// MAIN LOOP. go through each possible hypo
		TranslationOptionCollection::const_iterator iterTransOpt;
		for (iterTransOpt = m_possibleTranslations.begin(); iterTransOpt != m_possibleTranslations.end(); ++iterTransOpt)
		{
			const TranslationOption &transOpt = *iterTransOpt;
			// calc distortion if using this poss trans

			size_t transOptStartPos = transOpt.GetStartPos();

			if (hypoFirstGapPos == hypoWordCount)
			{
				if (transOptStartPos == hypoWordCount
					|| (transOptStartPos > hypoWordCount 
					&& transOpt.GetEndPos() <= hypoWordCount + m_staticData.GetMaxDistortion())
					)
				{
					Hypothesis *newHypo = hypothesis.CreateNext(transOpt);
					//newHypo->PrintHypothesis(m_source);
					outputHypoColl.AddNoPrune( newHypo );			
				}
			}
			else
			{
				if (transOptStartPos < hypoWordCount)
				{
					if (transOptStartPos >= hypoFirstGapPos
						&& !transOpt.Overlap(hypothesis))
					{
						Hypothesis *newHypo = hypothesis.CreateNext(transOpt);
						//newHypo->PrintHypothesis(m_source);
						outputHypoColl.AddNoPrune( newHypo );			
					}
				}
				else
				{
					if (transOpt.GetEndPos() <= hypoFirstGapPos + m_staticData.GetMaxDistortion()
						&& !transOpt.Overlap(hypothesis))
					{
						Hypothesis *newHypo = hypothesis.CreateNext(transOpt);
						//newHypo->PrintHypothesis(m_source);
						outputHypoColl.AddNoPrune( newHypo );			
					}
				}
			}
		}
	}
}

// OLD FUNCTIONS
void Manager::ProcessOneStack(const list < DecodeStep > &decodeStepList, HypothesisCollection &sourceHypoColl)
{
	// go thru each hypothesis in the stack
	HypothesisCollection::iterator iterHypo;
	for (iterHypo = sourceHypoColl.begin() ; iterHypo != sourceHypoColl.end() ; ++iterHypo)
	{
		Hypothesis &hypothesis = **iterHypo;
		ProcessOneHypothesis(decodeStepList, hypothesis);
	}
}

void Manager::ProcessOneHypothesis(const list < DecodeStep > &decodeStepList, const Hypothesis &hypothesis)
{
	vector < HypothesisCollectionIntermediate > outputHypoCollVec( decodeStepList.size() );
	HypothesisCollectionIntermediate::iterator iterHypo;

	// initial translation step
	list < DecodeStep >::const_iterator iterStep = decodeStepList.begin();
	const DecodeStep &decodeStep = *iterStep;
	ProcessInitialTranslation(hypothesis, decodeStep, outputHypoCollVec[0]);

	// do rest of decode steps
	int indexStep = 0;
	for (++iterStep ; iterStep != decodeStepList.end() ; ++iterStep) 
	{
		const DecodeStep &decodeStep = *iterStep;
		HypothesisCollectionIntermediate 
								&inputHypoColl		= outputHypoCollVec[indexStep]
								,&outputHypoColl	= outputHypoCollVec[indexStep + 1];

		// is it translation or generation
		switch (decodeStep.GetDecodeType()) 
		{
		case Translate:
			{
				// go thru each hypothesis just created
				for (iterHypo = inputHypoColl.begin() ; iterHypo != inputHypoColl.end() ; ++iterHypo)
				{
					Hypothesis &inputHypo = **iterHypo;
					ProcessTranslation(inputHypo, decodeStep, outputHypoColl);
				}
				break;
			}
		case Generate:
			{
				// go thru each hypothesis just created
				for (iterHypo = inputHypoColl.begin() ; iterHypo != inputHypoColl.end() ; ++iterHypo)
				{
					Hypothesis &inputHypo = **iterHypo;
					ProcessGeneration(inputHypo, decodeStep, outputHypoColl);
				}
				break;
			}
		}

		indexStep++;
	} // for (++iterStep 

	// add to real hypothesis stack
	HypothesisCollectionIntermediate &lastHypoColl	= outputHypoCollVec[decodeStepList.size() - 1];
	for (iterHypo = lastHypoColl.begin() ; iterHypo != lastHypoColl.end() ; )
	{
		Hypothesis *hypo = *iterHypo;

		hypo->CalcScore(m_staticData.GetLanguageModel(Initial)
									, m_staticData.GetLanguageModel(Other)
									, m_staticData.GetWeightDistortion()
									, m_staticData.GetWeightWordPenalty()
									, m_possibleTranslations.GetFutureScore(), m_source);
//		if(m_staticData.GetVerboseLevel() > 2) 
//		{			
//			hypo->PrintHypothesis(m_source, m_staticData.GetWeightDistortion(), m_staticData.GetWeightWordPenalty());
//		}
		size_t wordsTranslated = hypo->GetWordsBitmap().GetWordsCount();

		if (m_hypoStack[wordsTranslated].AddPrune(hypo))
		{
			HypothesisCollectionIntermediate::iterator iterCurr = iterHypo++;
			lastHypoColl.Detach(iterCurr);
			if(m_staticData.GetVerboseLevel() > 2) 
				{
					if(m_hypoStack[wordsTranslated].getBestScore() == hypo->GetScore(ScoreType::Total))
						{
							cout<<"new best estimate for this stack"<<endl;
							
						}
					cout<<"added hypothesis on stack "<<wordsTranslated<<" now size "<<m_hypoStack[wordsTranslated].size()<<endl<<endl;
				}

		}
		else
		{
			++iterHypo;
		}
	}

}

void Manager::ProcessInitialTranslation(const Hypothesis &hypothesis, const DecodeStep &decodeStep, HypothesisCollectionIntermediate &outputHypoColl)
{
	int maxDistortion = m_staticData.GetMaxDistortion();
	if (maxDistortion < 0)
	{	// no limit on distortion
		TranslationOptionCollection::const_iterator iterTransOpt;
		for (iterTransOpt = m_possibleTranslations.begin(); iterTransOpt != m_possibleTranslations.end(); ++iterTransOpt)
		{
			const TranslationOption &transOpt = *iterTransOpt;

			if ( !transOpt.Overlap(hypothesis)) 
			{
				Hypothesis *newHypo = hypothesis.CreateNext(transOpt);
				//newHypo->PrintHypothesis(m_source);
				outputHypoColl.AddNoPrune( newHypo );			
			}
		}
	}
	else
	{
		const WordsBitmap hypoBitmap = hypothesis.GetWordsBitmap();
		size_t hypoWordCount		= hypoBitmap.GetWordsCount()
			,hypoFirstGapPos	= hypoBitmap.GetFirstGapPos();

		// MAIN LOOP. go through each possible hypo
		TranslationOptionCollection::const_iterator iterTransOpt;
		for (iterTransOpt = m_possibleTranslations.begin(); iterTransOpt != m_possibleTranslations.end(); ++iterTransOpt)
		{
			const TranslationOption &transOpt = *iterTransOpt;
			// calc distortion if using this poss trans

			size_t transOptStartPos = transOpt.GetStartPos();

			if (hypoFirstGapPos == hypoWordCount)
			{
				if (transOptStartPos == hypoWordCount
					|| (transOptStartPos > hypoWordCount 
					&& transOpt.GetEndPos() <= hypoWordCount + m_staticData.GetMaxDistortion())
					)
				{
					Hypothesis *newHypo = hypothesis.CreateNext(transOpt);
					//newHypo->PrintHypothesis(m_source);
					outputHypoColl.AddNoPrune( newHypo );			
				}
			}
			else
			{
				if (transOptStartPos < hypoWordCount)
				{
					if (transOptStartPos >= hypoFirstGapPos
						&& !transOpt.Overlap(hypothesis))
					{
						Hypothesis *newHypo = hypothesis.CreateNext(transOpt);
						//newHypo->PrintHypothesis(m_source);
						outputHypoColl.AddNoPrune( newHypo );			
					}
				}
				else
				{
					if (transOpt.GetEndPos() <= hypoFirstGapPos + m_staticData.GetMaxDistortion()
						&& !transOpt.Overlap(hypothesis))
					{
						Hypothesis *newHypo = hypothesis.CreateNext(transOpt);
						//newHypo->PrintHypothesis(m_source);
						outputHypoColl.AddNoPrune( newHypo );			
					}
				}
			}
		}
	}
}

void Manager::ProcessTranslation(const Hypothesis &hypothesis, const DecodeStep &decodeStep, HypothesisCollectionIntermediate &outputHypoColl)
{
	const WordsRange &sourceWordsRange				= hypothesis.GetCurrSourceWordsRange();
	const Phrase sourcePhrase 								= m_source.GetSubString(sourceWordsRange);
	const PhraseDictionary &phraseDictionary	= decodeStep.GetPhraseDictionary();
	const TargetPhraseCollection *phraseColl	=	phraseDictionary.FindEquivPhrase(sourcePhrase);
	size_t currTargetLength										= hypothesis.GetCurrTargetLength();
	Hypothesis *newHypo;
	if (phraseColl != NULL)
	{
		TargetPhraseCollection::const_iterator iterTargetPhrase;

		for (iterTargetPhrase = phraseColl->begin(); iterTargetPhrase != phraseColl->end(); ++iterTargetPhrase)
		{
			const TargetPhrase& targetPhrase	= *iterTargetPhrase;
			
			TranslationOption transOpt(sourceWordsRange
																	, targetPhrase);
	
			Hypothesis *newHypo = hypothesis.MergeNext(transOpt);
			
			newHypo = hypothesis.MergeNext(transOpt);
			
			if (newHypo != NULL)
			{
				outputHypoColl.AddNoPrune( newHypo );
			}
		}
	}
	else if (sourceWordsRange.GetWordsCount() == 1 && currTargetLength == 1)
	{ // unknown handler here
		const FactorTypeSet &targetFactors 		= phraseDictionary.GetFactorsUsed(Output);
		newHypo = new Hypothesis(hypothesis);
		
		for (unsigned int currFactor = 0 ; currFactor < NUM_FACTORS ; currFactor++)
		{
			if (targetFactors.Contains(currFactor))
			{
				FactorType factorType = static_cast<FactorType>(currFactor);
				const Factor *targetFactor = newHypo->GetFactor(newHypo->GetSize() - 1, factorType);

				if (targetFactor == NULL)
				{
					const Factor *sourceFactor = sourcePhrase.GetFactor(0, factorType)
											,*unkownfactor;
					switch (factorType)
					{
					case POS:
						unkownfactor = m_staticData.GetFactorCollection().AddFactor(Output, factorType, UNKNOWN_FACTOR);
						newHypo->SetFactor(0, factorType, unkownfactor);
						break;
					default:
						unkownfactor = m_staticData.GetFactorCollection().AddFactor(Output, factorType, sourceFactor->GetString());
						newHypo->SetFactor(0, factorType, unkownfactor);
						break;
					}
				}
			}
		}
		outputHypoColl.AddNoPrune( newHypo );
	}
	

}


// helpers
typedef pair<Word, float> WordPair;
typedef list< WordPair > WordList;	
	// 1st = word 
	// 2nd = score
typedef list< WordPair >::const_iterator WordListIterator;

inline void IncrementIterators(vector< WordListIterator > &wordListIterVector
												, const vector< WordList > &wordListVector)
{
	for (size_t currPos = 0 ; currPos < wordListVector.size() ; currPos++)
	{
		WordListIterator &iter = wordListIterVector[currPos];
		iter++;
		if (iter != wordListVector[currPos].end())
		{ // eg. 4 -> 5
			return;
		}
		else
		{ //  eg 9 -> 10
			iter = wordListVector[currPos].begin();
		}
	}
}

void Manager::ProcessGeneration(const Hypothesis &hypothesis
																, const DecodeStep &decodeStep
																, HypothesisCollectionIntermediate &outputHypoColl)
{
	const GenerationDictionary &generationDictionary = decodeStep.GetGenerationDictionary();
	const float weight = generationDictionary.GetWeight();

	size_t hypoSize	= hypothesis.GetSize()
		, targetLength	= hypothesis.GetCurrTargetLength();

	// generation list for each word in hypothesis
	vector< WordList > wordListVector(targetLength);

	// create generation list
	int wordListVectorPos = 0;
	for (size_t currPos = hypoSize - targetLength ; currPos < hypoSize ; currPos++)
	{
		WordList &wordList = wordListVector[wordListVectorPos];
		const FactorArray &factorArray = hypothesis.GetFactorArray(currPos);

		const OutputWordCollection *wordColl = generationDictionary.FindWord(factorArray);

		if (wordColl == NULL)
		{	// word not found in generation dictionary
			// go no further
			return;
		}

		OutputWordCollection::const_iterator iterWordColl;
		for (iterWordColl = wordColl->begin() ; iterWordColl != wordColl->end(); ++iterWordColl)
		{
			const Word &outputWord = (*iterWordColl).first;
			float score = (*iterWordColl).second;
			wordList.push_back(WordPair(outputWord, score));
		}
		
		wordListVectorPos++;
	}

	// use generation list (wordList)
	// set up iterators
	size_t numIteration = 1;
	vector< WordListIterator >	wordListIterVector(targetLength);
	vector< const Word* >				mergeWords(targetLength);
	for (size_t currPos = 0 ; currPos < targetLength ; currPos++)
	{
		wordListIterVector[currPos] = wordListVector[currPos].begin();
		numIteration *= wordListVector[currPos].size();
	}

	// go thru each possible factor for each word & create hypothesis
	for (size_t currIter = 0 ; currIter < numIteration ; currIter++)
	{
		float generationScore = 0; // total score for this string of words

		// create vector of words with new factors for last phrase
		for (size_t currPos = 0 ; currPos < targetLength ; currPos++)
		{
			const WordPair &wordPair = *wordListIterVector[currPos];
			mergeWords[currPos] = &(wordPair.first);
			generationScore += wordPair.second;
		}

		// merge with existing hypothesis
		Hypothesis *mergeHypo = hypothesis.Clone();
		mergeHypo->MergeFactors(mergeWords, generationDictionary, generationScore, weight);
		outputHypoColl.AddNoPrune(mergeHypo);

		// increment iterators
		IncrementIterators(wordListIterVector, wordListVector);
	}
}

void Manager::OutputHypoStackSize()
{
	std::vector < HypothesisCollection >::const_iterator iterStack;
	for (iterStack = m_hypoStack.begin() ; iterStack != m_hypoStack.end() ; ++iterStack)
	{
		TRACE_ERR ( (int)(*iterStack).size() << ", ");
	}
	TRACE_ERR (endl);
}

void Manager::OutputHypoStack(int stack)
{
	if (stack >= 0)
	{
		TRACE_ERR ( "Stack " << stack << ": " << endl << m_hypoStack[stack] << endl);
	}
	else
	{ // all stacks
		int i = 0;
		vector < HypothesisCollection >::iterator iterStack;
		for (iterStack = m_hypoStack.begin() ; iterStack != m_hypoStack.end() ; ++iterStack)
		{
			HypothesisCollection &hypoColl = *iterStack;
			TRACE_ERR ( "Stack " << i++ << ": " << endl << hypoColl << endl);
		}
	}
}

void Manager::CalcNBest(size_t count, LatticePathList &ret) const
{
#ifdef N_BEST
	if (count <= 0)
		return;

	list<const Hypothesis*> sortedPureHypo = m_hypoStack.back().GetSortedList();

	if (sortedPureHypo.size() == 0)
		return;

	LatticePathCollection contenders;

	// path of the best
	contenders.insert(new LatticePath(*sortedPureHypo.begin()));
	
	// used to add next pure hypo path
	list<const Hypothesis*>::const_iterator iterBestHypo = ++sortedPureHypo.begin();

	for (size_t currBest = 0 ; currBest <= count && contenders.size() > 0 ; currBest++)
	{
		// get next best from list of contenders
		LatticePath *path = *contenders.begin();
		ret.push_back(path);
		contenders.erase(contenders.begin());

		// create deviations from current best
		path->CreateDeviantPaths(contenders);
		
		// if necessary, add next pure path
		if (path->IsPurePath() && iterBestHypo != sortedPureHypo.end())
		{
			contenders.insert(new LatticePath(*iterBestHypo));
			++iterBestHypo;
		}
	}
#endif
}
