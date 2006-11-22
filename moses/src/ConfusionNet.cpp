// $Id$

#include "ConfusionNet.h"
#include <sstream>

#include "FactorCollection.h"
#include "Util.h"
#include "PhraseDictionaryTreeAdaptor.h"
#include "TranslationOptionCollectionConfusionNet.h"
#include "StaticData.h"
#include "Sentence.h"
#include "UserMessage.h"

struct CNStats {
	size_t created,destr,read,colls,words;

	CNStats() : created(0),destr(0),read(0),colls(0),words(0) {}
	~CNStats() {print(std::cerr);}

	void createOne() {++created;}
	void destroyOne() {++destr;}

	void collect(const ConfusionNet& cn)
	{
		++read;
		colls+=cn.GetSize();
		for(size_t i=0;i<cn.GetSize();++i)
			words+=cn[i].size();
	}
	void print(std::ostream& out) const
	{
		if(created>0)
			{
				out<<"confusion net statistics:\n"
					" created:\t"<<created<<"\n"
					" destroyed:\t"<<destr<<"\n"
					" succ. read:\t"<<read<<"\n"
					" columns:\t"<<colls<<"\n"
					" words:\t"<<words<<"\n"
					" avg. word/column:\t"<<words/(1.0*colls)<<"\n"
					" avg. cols/sent:\t"<<colls/(1.0*read)<<"\n"
					"\n\n";
			}
	}

};

CNStats stats;


ConfusionNet::ConfusionNet(FactorCollection* p) 
	: InputType(),m_factorCollection(p) {stats.createOne();}
ConfusionNet::~ConfusionNet() {stats.destroyOne();}

ConfusionNet::ConfusionNet(Sentence const& s)
{
	data.resize(s.GetSize());
	for(size_t i=0;i<s.GetSize();++i)
		data[i].push_back(std::make_pair(s.GetWord(i),0.0));
}


void ConfusionNet::SetFactorCollection(FactorCollection *p) 
{
	m_factorCollection=p;
}
bool ConfusionNet::ReadF(std::istream& in,
												 const std::vector<FactorType>& factorOrder,
												 int format) 
{
	TRACE_ERR( "read confusion net with format "<<format<<"\n");
	switch(format) 
		{
		case 0: return ReadFormat0(in,factorOrder);
		case 1: return ReadFormat1(in,factorOrder);
		default: 
			stringstream strme;
			strme << "ERROR: unknown format '"<<format
							 <<"' in ConfusionNet::Read";
			UserMessage::Add(strme.str());
		}
	return false;
}

int ConfusionNet::Read(std::istream& in,
											 const std::vector<FactorType>& factorOrder, 
											 FactorCollection &factorCollection) 
{
	SetFactorCollection(&factorCollection);
	int rv=ReadF(in,factorOrder,0);
	if(rv) stats.collect(*this);
	return rv;
}


void ConfusionNet::String2Word(const std::string& s,Word& w,
															 const std::vector<FactorType>& factorOrder) 
{
	std::vector<std::string> factorStrVector = Tokenize(s, "|");
	for(size_t i=0;i<factorOrder.size();++i) 
		w.SetFactor(factorOrder[i],
								m_factorCollection->AddFactor(Input,factorOrder[i],
																							factorStrVector[i]));
}

bool ConfusionNet::ReadFormat0(std::istream& in,
															 const std::vector<FactorType>& factorOrder) 
{
	assert(m_factorCollection);
	Clear();
	std::string line;
	while(getline(in,line)) {
		std::istringstream is(line);
		std::string word;double prob;
		Column col;
		while(is>>word>>prob) {
			Word w;
			String2Word(word,w,factorOrder);
			if(prob<0.0) 
				{
					TRACE_ERR("WARN: negative prob: "<<prob<<" ->set to 0.0\n");
					prob=0.0;
				}
			else if (prob>1.0)
				{
					TRACE_ERR("WARN: prob > 1.0 : "<<prob<<" -> set to 1.0\n");
					prob=1.0;
				}
			col.push_back(std::make_pair(w,std::max(static_cast<float>(log(prob)),
																							LOWEST_SCORE)));
		}
		if(col.size()) {
			data.push_back(col);
			ShrinkToFit(data.back());
		}
		else break;
	}
	return !data.empty();
}
bool ConfusionNet::ReadFormat1(std::istream& in,
															 const std::vector<FactorType>& factorOrder) 
{
	assert(m_factorCollection);
	Clear();
	std::string line;
	if(!getline(in,line)) return 0;
	size_t s;
	if(getline(in,line)) s=atoi(line.c_str()); else return 0;
	data.resize(s);
	for(size_t i=0;i<data.size();++i) {
		if(!getline(in,line)) return 0;
		std::istringstream is(line);
		if(!(is>>s)) return 0;
		std::string word;double prob;
		data[i].resize(s);
		for(size_t j=0;j<s;++j)
			if(is>>word>>prob) {
				data[i][j].second = (float) log(prob); 
				if(data[i][j].second<0) {
					TRACE_ERR("WARN: neg costs: "<<data[i][j].second<<" -> set to 0\n");
					data[i][j].second=0.0;}
				String2Word(word,data[i][j].first,factorOrder);
			} else return 0;
	}
	return !data.empty();
}

void ConfusionNet::Print(std::ostream& out) const {
	out<<"conf net: "<<data.size()<<"\n";
	for(size_t i=0;i<data.size();++i) {
		out<<i<<" -- ";
		for(size_t j=0;j<data[i].size();++j)
			out<<"("<<data[i][j].first.ToString()<<", "<<data[i][j].second<<") ";
		out<<"\n";
	}
	out<<"\n\n";
}

Phrase ConfusionNet::GetSubString(const WordsRange&) const {
	TRACE_ERR("ERROR: call to ConfusionNet::GetSubString\n");
	abort();
	return Phrase(Input);
}

std::string ConfusionNet::GetStringRep(const vector<FactorType> factorsToPrint) const{ //not well defined yet
	TRACE_ERR("ERROR: call to ConfusionNet::GeStringRep\n");
	abort();
	return "";
}
#pragma warning(disable:4716)
const Word& ConfusionNet::GetWord(size_t) const {
	TRACE_ERR("ERROR: call to ConfusionNet::GetFactorArray\n");
	abort();
}
#pragma warning(default:4716)

std::ostream& operator<<(std::ostream& out,const ConfusionNet& cn) 
{
	cn.Print(out);return out;
}

TranslationOptionCollection* 
ConfusionNet::CreateTranslationOptionCollection() const 
{
	size_t maxNoTransOptPerCoverage = StaticData::Instance()->GetMaxNoTransOptPerCoverage();
	TranslationOptionCollection *rv= new TranslationOptionCollectionConfusionNet(*this, maxNoTransOptPerCoverage);
	assert(rv);
	return rv;
}

