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

#include <list>
#include <vector>
#include <map>
#include <memory>
#include "TypeDef.h"
#include "ScoreIndexManager.h"
#include "FactorCollection.h"
#include "Parameter.h"
#include "LanguageModel.h"
#include "LMList.h"
#include "SentenceStats.h"
#include "DecodeGraph.h"
//#include "UnknownWordHandler.h"

class InputType;
class LexicalReordering;
class PhraseDictionary;
class GenerationDictionary;
class DistortionScoreProducer;
class WordPenaltyProducer;
class DecodeStep;
class UnknownWordPenaltyProducer;

/** Contains global variables and contants */
class StaticData
{
private:
	static StaticData									s_instance;
protected:	
	std::vector<PhraseDictionary*>	m_phraseDictionary;
	std::vector<GenerationDictionary*>	m_generationDictionary;
	std::vector <DecodeGraph*>		m_decodeStepVL;
	Parameter			*m_parameter;
	std::vector<FactorType>			m_inputFactorOrder, m_outputFactorOrder;
	LMList									m_languageModel;
	ScoreIndexManager				m_scoreIndexManager;
	std::vector<float>			m_allWeights;
	std::vector<LexicalReordering*>                   m_reorderModels;
		// Initial	= 0 = can be used when creating poss trans
		// Other		= 1 = used to calculate LM score once all steps have been processed
	float
		m_beamWidth,
		m_kbestCubePruning,
		m_weightDistortion, 
		m_weightWordPenalty, 
		m_wordDeletionWeight,
		m_weightUnknownWord;
									// PhraseTrans, Generation & LanguageModelScore has multiple weights.
	int																	m_maxDistortion;
									// do it differently from old pharaoh
									// -ve	= no limit on distortion
									// 0		= no disortion (monotone in old pharaoh)
	size_t                              
			m_maxHypoStackSize //hypothesis-stack size that triggers pruning
			, m_nBestSize
			, m_nBestFactor
			, m_maxNoTransOptPerCoverage
			, m_maxNoPartTransOpt
			, m_maxPhraseLength;
	
	std::string									m_nBestFilePath;
	bool                        m_fLMsLoaded, m_labeledNBestList,m_nBestIncludesAlignment;
	/***
	 * false = treat unknown words as unknowns, and translate them as themselves;
	 * true = drop (ignore) them
	 */
	bool m_dropUnknown;
	bool m_wordDeletionEnabled;

	bool m_sourceStartPosMattersForRecombination;
	bool m_recoverPath;

	InputTypeEnum m_inputType;
	size_t m_numInputScores;

	mutable size_t m_verboseLevel;
	DistortionScoreProducer *m_distortionScoreProducer;
	WordPenaltyProducer *m_wpProducer;
	UnknownWordPenaltyProducer *m_unknownWordPenaltyProducer;
	bool m_reportSegmentation;
	bool m_reportAllFactors;
	bool m_useDistortionFutureCosts;
	bool m_isDetailedTranslationReportingEnabled;
	bool m_onlyDistinctNBest;
	bool m_computeLMBackoffStats;

	mutable std::auto_ptr<SentenceStats> m_sentenceStats;
	std::string m_factorDelimiter; //! by default, |, but it can be changed
	size_t m_maxFactorIdx[2];  //! number of factors on source and target side
	size_t m_maxNumFactors;  //! max number of factors on both source and target sides
	
	XmlInputType m_xmlInputType; //! method for handling sentence XML input

	bool m_mbr; //! use MBR decoder
	size_t m_mbrSize; //! number of translation candidates considered
	float m_mbrScale; //! scaling factor for computing marginal probability of candidate translation

	bool m_timeout; //! use timeout
	size_t m_timeout_threshold; //! seconds after which time out is activated

	bool m_useTransOptCache;
	mutable std::map<Phrase, TranslationOptionList> m_transOptCache;

	mutable const InputType* m_input;  //! holds reference to current sentence
	bool m_isAlwaysCreateDirectTranslationOption;
	//! constructor. only the 1 static variable can be created

	bool m_outputWordGraph; //! whether to output word graph
        bool m_outputSearchGraph; //! whether to output search graph

	StaticData();

	//! helper fn to set bool param from ini file/command line
	void SetBooleanParameter(bool *paramter, string parameterName, bool defaultValue);

	/***
	 * load all language models as specified in ini file
	 */
	bool LoadLanguageModels();
	/***
	 * load not only the main phrase table but also any auxiliary tables that depend on which features are being used
	 * (eg word-deletion, word-insertion tables)
	 */
	bool LoadPhraseTables();
	//! load all generation tables as specified in ini file
	bool LoadGenerationTables();
	//! load decoding steps
	bool LoadMapping();
	bool LoadLexicalReorderingModel();
	
public:

	bool IsAlwaysCreateDirectTranslationOption() const {
		return m_isAlwaysCreateDirectTranslationOption;
	}
	//! destructor
	~StaticData();
	//! return static instance for use like global variable
	static const StaticData& Instance() { return s_instance; }
	
	/** delete current static instance and replace with another. 
		* Used by gui front end
		*/
	#ifdef WIN32
	static void Reset() { s_instance = StaticData(); }
	#endif
	
	/** load data into static instance. This function is required
		* as LoadData() is not const
		*/
	static bool LoadDataStatic(Parameter *parameter)
	{
		return s_instance.LoadData(parameter);
	}

	/** Main function to load everything.
	 * Also initialize the Parameter object
	 */
	bool LoadData(Parameter *parameter);

	const PARAM_VEC &GetParam(const std::string &paramName) const
	{
		return m_parameter->GetParam(paramName);
	}

	bool IsComputeLMBackoffStats() const
	{
		return m_computeLMBackoffStats;
	}
	const std::vector<FactorType> &GetInputFactorOrder() const
	{
		return m_inputFactorOrder;
	}
	const std::vector<FactorType> &GetOutputFactorOrder() const
	{
		return m_outputFactorOrder;
	}

	const std::vector<DecodeGraph*> &GetDecodeStepVL() const
	{
		return m_decodeStepVL;
	}
	
	inline bool GetSourceStartPosMattersForRecombination() const
	{ 
		return m_sourceStartPosMattersForRecombination; 
	}
	inline bool GetDropUnknown() const 
	{ 
		return m_dropUnknown; 
	}
	inline size_t GetMaxNoTransOptPerCoverage() const 
	{ 
		return m_maxNoTransOptPerCoverage;
	}
	inline size_t GetMaxNoPartTransOpt() const 
	{ 
		return m_maxNoPartTransOpt;
	}
	inline size_t GetMaxPhraseLength() const 
	{ 
		return m_maxPhraseLength;
	}
	const std::vector<LexicalReordering*> &GetReorderModels() const
	{
		return m_reorderModels;
	}
	float GetWeightDistortion() const
	{
		return m_weightDistortion;
	}
	float GetWeightWordPenalty() const
	{
		return m_weightWordPenalty;
	}
	bool IsWordDeletionEnabled() const
	{
		return m_wordDeletionEnabled;
	}
	size_t GetMaxHypoStackSize() const
	{
		return m_maxHypoStackSize;
	}
	size_t IsPathRecoveryEnabled() const
	{
		return m_recoverPath;
	}
	int GetMaxDistortion() const
	{
		return m_maxDistortion;
	}
	float GetBeamWidth() const
	{
		return m_beamWidth;
	}
	int GetKBestCubePruning() const
	{
		return m_kbestCubePruning;
	}
	//! returns the total number of score components across all types, all factors
	size_t GetTotalScoreComponents() const
	{
		return m_scoreIndexManager.GetTotalNumberOfScores();
	}
	const ScoreIndexManager& GetScoreIndexManager() const
	{
		return m_scoreIndexManager;
	}

	size_t GetLMSize() const
	{
		return m_languageModel.size();
	}
	const LMList &GetAllLM() const
	{
		return m_languageModel;
	}
	size_t GetPhraseDictionarySize() const
	{
		return m_phraseDictionary.size();
	}
	const std::vector<PhraseDictionary*> &GetPhraseDictionaries() const
	{
		return m_phraseDictionary;
	}
	const std::vector<GenerationDictionary*> &GetGenerationDictionaries() const
	{
		return m_generationDictionary;
	}
	size_t GetGenerationDictionarySize() const
	{
		return m_generationDictionary.size();
	}
	size_t GetVerboseLevel() const
	{
		return m_verboseLevel;
	}
	void SetVerboseLevel(int x) const { m_verboseLevel = x; }
	bool GetReportSegmentation() const
	{
		return m_reportSegmentation;
	}
	bool GetReportAllFactors() const
	{
		return m_reportAllFactors;
	}
	bool IsDetailedTranslationReportingEnabled() const
	{
		return m_isDetailedTranslationReportingEnabled;
	}
	void ResetSentenceStats(const InputType& source) const
	{
		m_sentenceStats = std::auto_ptr<SentenceStats>(new SentenceStats(source));
	}
	bool IsLabeledNBestList() const
	{
		return m_labeledNBestList;
	}
	bool NBestIncludesAlignment() const
	{
		return m_nBestIncludesAlignment;
	}
	const std::vector<std::string> &GetDescription() const
	{
		return m_parameter->GetParam("description");
	}

	// for mert
	size_t GetNBestSize() const
	{
		return m_nBestSize;
	}
	const std::string &GetNBestFilePath() const
	{
		return m_nBestFilePath;
	}
  	bool IsNBestEnabled() const {
	  return (!m_nBestFilePath.empty()) || m_mbr || m_outputSearchGraph;
  	}
	size_t GetNBestFactor() const
	{
		return m_nBestFactor;
	}
	bool GetOutputWordGraph() const
	{ return m_outputWordGraph; }

	//! Sets the global score vector weights for a given ScoreProducer.
	void SetWeightsForScoreProducer(const ScoreProducer* sp, const std::vector<float>& weights);
	InputTypeEnum GetInputType() const {return m_inputType;}
	size_t GetNumInputScores() const {return m_numInputScores;}
	const InputType* GetInput() const { return m_input; }
	void InitializeBeforeSentenceProcessing(InputType const&) const;
	void CleanUpAfterSentenceProcessing() const;
	SentenceStats& GetSentenceStats() const
	{
		return *m_sentenceStats;
	}
	const std::vector<float>& GetAllWeights() const
	{
		return m_allWeights;
	}
	const DistortionScoreProducer *GetDistortionScoreProducer() const { return m_distortionScoreProducer; }
	const WordPenaltyProducer *GetWordPenaltyProducer() const { return m_wpProducer; }
	const UnknownWordPenaltyProducer *GetUnknownWordPenaltyProducer() const { return m_unknownWordPenaltyProducer; }

	bool UseDistortionFutureCosts() const {return m_useDistortionFutureCosts;}
	bool GetDistinctNBest() const {return m_onlyDistinctNBest;}
	const std::string& GetFactorDelimiter() const {return m_factorDelimiter;}
	size_t GetMaxNumFactors(FactorDirection direction) const { return m_maxFactorIdx[(size_t)direction]+1; }
	size_t GetMaxNumFactors() const { return m_maxNumFactors; }
	bool UseMBR() const { return m_mbr; }
	size_t GetMBRSize() const { return m_mbrSize; }
	float GetMBRScale() const { return m_mbrScale; }
	
	bool UseTimeout() const { return m_timeout; }
	size_t GetTimeoutThreshold() const { return m_timeout_threshold; }
	
	size_t GetOutputSearchGraph() const { return m_outputSearchGraph; }
		
	XmlInputType GetXmlInputType() const { return m_xmlInputType; }

	bool GetUseTransOptCache() const { return m_useTransOptCache; }

	void AddTransOptListToCache(const Phrase &sourcePhrase, const TranslationOptionList &transOptList) const
	{
		m_transOptCache[sourcePhrase] = transOptList;
	}

	const TranslationOptionList* FindTransOptListInCache(const Phrase &sourcePhrase) const;
};
