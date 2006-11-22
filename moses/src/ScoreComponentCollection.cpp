// $Id$

#include "ScoreComponentCollection.h"
#include "StaticData.h"

ScoreComponentCollection::ScoreComponentCollection()
  : m_scores(StaticData::Instance()->GetTotalScoreComponents(), 0.0f)
  , m_sim(&StaticData::Instance()->GetScoreIndexManager())
{}

