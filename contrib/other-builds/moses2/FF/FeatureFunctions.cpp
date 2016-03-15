/*
 * FeatureFunctions.cpp
 *
 *  Created on: 27 Oct 2015
 *      Author: hieu
 */

#include <boost/foreach.hpp>
#include "FeatureFunctions.h"
#include "StatefulFeatureFunction.h"
#include "../System.h"
#include "../Scores.h"
#include "../MemPool.h"

#include "SkeletonStatelessFF.h"
#include "SkeletonStatefulFF.h"
#include "WordPenalty.h"
#include "PhrasePenalty.h"
#include "Distortion.h"
#include "LexicalReordering.h"
#include "../TranslationModel/PhraseTableMemory.h"
#include "../TranslationModel/ProbingPT.h"
#include "../TranslationModel/UnknownWordPenalty.h"
#include "../LM/LanguageModel.h"
//#include "../LM/LanguageModelDALM.h"
#include "../LM/KENLM.h"
#include "../SCFG/TargetPhraseImpl.h"
#include "util/exception.hh"

using namespace std;

namespace Moses2
{
FeatureFunctions::FeatureFunctions(System &system)
:m_system(system)
,m_ffStartInd(0)
{
	//m_registry.PrintFF();
}

FeatureFunctions::~FeatureFunctions() {
	RemoveAllInColl(m_featureFunctions);
}

void FeatureFunctions::Load()
{
  // load, everything but pts
  BOOST_FOREACH(const FeatureFunction *ff, m_featureFunctions) {
	  FeatureFunction *nonConstFF = const_cast<FeatureFunction*>(ff);
	  PhraseTable *pt = dynamic_cast<PhraseTable*>(nonConstFF);

	  if (pt) {
		  // do nothing. load pt last
	  }
	  else {
		  cerr << "Loading " << nonConstFF->GetName() << endl;
		  nonConstFF->Load(m_system);
		  cerr << "Finished loading " << nonConstFF->GetName() << endl;
	  }
  }

  // load pt
  BOOST_FOREACH(const PhraseTable *pt, m_phraseTables) {
	  PhraseTable *nonConstPT = const_cast<PhraseTable*>(pt);
	  cerr << "Loading " << nonConstPT->GetName() << endl;
	  nonConstPT->Load(m_system);
	  cerr << "Finished loading " << nonConstPT->GetName() << endl;
  }
}

void FeatureFunctions::Create()
{
  const Parameter &params = m_system.params;

  const PARAM_VEC *ffParams = params.GetParam("feature");
  UTIL_THROW_IF2(ffParams == NULL, "Must have [feature] section");

  BOOST_FOREACH(const std::string &line, *ffParams) {
	  cerr << "line=" << line << endl;
	  FeatureFunction *ff = Create(line);

	  m_featureFunctions.push_back(ff);

	  StatefulFeatureFunction *sfff = dynamic_cast<StatefulFeatureFunction*>(ff);
	  if (sfff) {
		  sfff->SetStatefulInd(m_statefulFeatureFunctions.size());
		  m_statefulFeatureFunctions.push_back(sfff);
	  }

	  if (ff->HasPhraseTableInd()) {
		  ff->SetPhraseTableInd(m_withPhraseTableInd.size());
		  m_withPhraseTableInd.push_back(ff);
	  }

	  PhraseTable *pt = dynamic_cast<PhraseTable*>(ff);
	  if (pt) {
		  pt->SetPtInd(m_phraseTables.size());
		  m_phraseTables.push_back(pt);
	  }
  }
}

FeatureFunction *FeatureFunctions::Create(const std::string &line)
{
	vector<string> toks = Tokenize(line);

	FeatureFunction *ret = m_registry.Construct(m_ffStartInd, toks[0], line);
	if (ret == NULL) {
		cerr << "NADDA:" << line << endl;
	}
	m_ffStartInd += ret->GetNumScores();

	return ret;
}

const FeatureFunction *FeatureFunctions::FindFeatureFunction(const std::string &name) const
{
  BOOST_FOREACH(const FeatureFunction *ff, m_featureFunctions) {
	  if (ff->GetName() == name) {
		  return ff;
	  }
  }
  return NULL;
}

const PhraseTable *FeatureFunctions::GetPhraseTablesExcludeUnknownWordPenalty(size_t ptInd)
{
	// assume only 1 unk wp
	std::vector<const PhraseTable*> tmpVec(m_phraseTables);
	std::vector<const PhraseTable*>::iterator iter;
	for (iter = tmpVec.begin(); iter != tmpVec.end(); ++iter) {
		const PhraseTable *pt = *iter;
		  const UnknownWordPenalty *unkWP = dynamic_cast<const UnknownWordPenalty *>(pt);
		  if (unkWP) {
			  tmpVec.erase(iter);
			  break;
		  }
	}

	const PhraseTable *pt = tmpVec[ptInd];
	return pt;
}

void
FeatureFunctions::EvaluateInIsolation(MemPool &pool, const System &system,
		  const Phrase &source, TargetPhrase &targetPhrase) const
{
  SCORE estimatedScore = 0;

  BOOST_FOREACH(const FeatureFunction *ff, m_featureFunctions) {
	  Scores& scores = targetPhrase.GetScores();
	  ff->EvaluateInIsolation(pool, system, source, targetPhrase, scores, &estimatedScore);
  }

  targetPhrase.SetEstimatedScore(estimatedScore);
}

void FeatureFunctions::EvaluateAfterTablePruning(MemPool &pool, const TargetPhrases &tps, const Phrase &sourcePhrase) const
{
  BOOST_FOREACH(const FeatureFunction *ff, m_featureFunctions) {
	  ff->EvaluateAfterTablePruning(pool, tps, sourcePhrase);
  }

}
/*
void
FeatureFunctions::EvaluateInIsolation(MemPool &pool, const System &system,
		  const Phrase &source, SCFG::TargetPhrase &targetPhrase) const
{
  SCORE estimatedScore = 0;

  BOOST_FOREACH(const FeatureFunction *ff, m_featureFunctions) {
	  Scores& scores = targetPhrase.GetScores();
	  ff->EvaluateInIsolation(pool, system, source, targetPhrase, scores, &estimatedScore);
  }

  //targetPhrase.SetEstimatedScore(estimatedScore);
}
*/
}

