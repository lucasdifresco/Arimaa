
/*
 * learner.cpp
 * Author: davidwu
 */
#include "pch.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <vector>
#include "global.h"
#include "gameiterator.h"
#include "feature.h"
#include "featurearimaa.h"
#include "featuremove.h"
#include "learner.h"
#include "arimaaio.h"

using namespace std;
using namespace ArimaaIO;


//-------------------------------------------------------------------------

Learner::Learner()
{

}
Learner::~Learner()
{

}

//-------------------------------------------------------------------------

NaiveBayes::NaiveBayes(ArimaaFeatureSet afset, int nFeatures, double pstr)
:afset(afset)
{
	numFeatures = nFeatures;
  totalFreq = 0;
  featureFreq[0].resize(numFeatures);
  featureFreq[1].resize(numFeatures);
  outcomeFreq[0] = 0;
  outcomeFreq[1] = 0;
  priorStrength = pstr;
}

NaiveBayes::~NaiveBayes()
{

}

void NaiveBayes::train(GameIterator& iter)
{
  cout << "Training NaiveBayes" << endl;

  int numTrained = 0;
  while(iter.next())
  {
    if(numTrained%1000 == 0)
      cout << "Training NB: " << numTrained << endl;
    numTrained++;

    int winningTeam;
		vector<vector<findex_t> > teams;
		iter.computeMoveFeatures(afset, winningTeam,teams);

		int size = teams.size();
		for(int i = 0; i<size; i++)
		{
			totalFreq++;

			int outcome = (i == winningTeam) ? 1 : 0;
			outcomeFreq[outcome]++;

			int teamsize = teams[i].size();
			for(int j = 0; j<teamsize; j++)
				featureFreq[outcome][teams[i][j]]++;
		}
  }

  computeLogOdds();

  cout << "Trained on " << numTrained << " instances" << endl;
}

void NaiveBayes::computeLogOdds()
{
	logWinOdds = log((double)outcomeFreq[1] / (double)outcomeFreq[0]);

  double virtualLoss = priorStrength * outcomeFreq[0] / (double)totalFreq;
  double virtualWin = priorStrength * outcomeFreq[1] / (double)totalFreq;

  logWinOddsIfNoFeatures = logWinOdds;
  logLikelihoodIfPresent.resize(numFeatures);
  logLikelihoodIfAbsent.resize(numFeatures);
	for(int i = 0; i<numFeatures; i++)
	{
		double pFeatureGLoss = (featureFreq[0][i]+virtualLoss)/(outcomeFreq[0]+virtualLoss*2);
		double pFeatureGWin = (featureFreq[1][i]+virtualWin)/(outcomeFreq[1]+virtualWin*2);
		logLikelihoodIfPresent[i] = log(pFeatureGWin / pFeatureGLoss);

		double pNoFeatureGLoss = (outcomeFreq[0]-featureFreq[0][i]+virtualLoss)/(outcomeFreq[0]+virtualLoss*2);
		double pNoFeatureGWin = (outcomeFreq[1]-featureFreq[1][i]+virtualWin)/(outcomeFreq[1]+virtualWin*2);
		logLikelihoodIfAbsent[i] = log(pNoFeatureGWin / pNoFeatureGLoss);

		logWinOddsIfNoFeatures += logLikelihoodIfAbsent[i];
	}
}

double NaiveBayes::evaluate(const vector<findex_t>& team)
{
	int teamSize = team.size();
	double logOdds = logWinOddsIfNoFeatures;
  for(int j = 0; j<teamSize; j++)
  {
  	logOdds += logLikelihoodIfPresent[team[j]] - logLikelihoodIfAbsent[team[j]];
  }
  return logOdds;
}

void NaiveBayes::outputToFile(const char* file)
{
  ofstream out;
  out.open(file,ios::out);
  out.precision(14);
  out << numFeatures << endl;
  out << priorStrength << endl;
  out << totalFreq << endl;
  out << outcomeFreq[0] << " " << outcomeFreq[1] << endl;

  for(int i = 0; i<(int)featureFreq[0].size(); i++)
    out << featureFreq[0][i] << " " << featureFreq[1][i] << endl;
  out.close();
}

NaiveBayes NaiveBayes::inputFromFile(ArimaaFeatureSet afset, const char* file)
{
  ifstream in;
  in.open(file,ios::in);
  if(in.fail())
  {
    cout << "Failed to input from " << file << endl;
    return NaiveBayes(afset,0,0);
  }
  int numFeatures;
  double priorStrength;
  in >> numFeatures;
  in >> priorStrength;
  NaiveBayes nb(afset,numFeatures,priorStrength);
  in >> nb.totalFreq;
  in >> nb.outcomeFreq[0];
  in >> nb.outcomeFreq[1];
  for(int i = 0; i<numFeatures; i++)
  {
    in >> nb.featureFreq[0][i];
    in >> nb.featureFreq[1][i];
  }
  in.close();

  nb.computeLogOdds();

  return nb;
}

//-------------------------------------------------------------------------------------------

BradleyTerry::BradleyTerry(ArimaaFeatureSet afset, int numIterations)
:afset(afset), numIterations(numIterations)
{
	numFeatures = afset.fset->numFeatures;
  gamma.resize(numFeatures);
  logGamma.resize(numFeatures);

  for(int i = 0; i<numFeatures; i++)
    gamma[i] = 1;
  for(int i = 0; i<numFeatures; i++)
  	logGamma[i] = 0;
}

BradleyTerry::~BradleyTerry()
{

}

STRUCT_NAMED_TRIPLE(int,match,int,team,int,degree,MTD);

//Encodes a sequence of MTDs for matches/teams in increasing order.
//Format for each MTD in the stream is a sequence of bytes encoding the difference in the match and team from
//the previous. This is done together by the delta in the number of teams that occurs if you concatenate all matches
//together consecutively. Then, if the degree is not 1, this is followed by bytes that encode the degree.
//
//Actually, we always encode x = 2*delta + (degree != 1), so that we can tell when there are bytes following that encode the
//degree or not.
//
//For encoding any of the x's or the degree's, we write its bits in the form a0a1a2... where each ai is 7 bits,
//then we write the bytes:
//  <bit 7: hasNext, bit 6-0: a0>
//  <bit 7: hasNext, bit 6-0: a1>
//  <bit 7: hasNext, bit 6-0: a2>
//  ...

struct MTDByteStream
{
	vector<uint8_t> bytes;
	MTD writeHead;

	MTD readHead;
	int readIndex;

	MTDByteStream()
	{
		writeHead = MTD(0,0,0);

		readHead = MTD(0,0,0);
		readIndex = 0;
	}

	void resetWrite()
	{
		writeHead = MTD(0,0,0);
		bytes.clear();
		resetRead();
	}

	void resetRead()
	{
		readHead = MTD(0,0,0);
		readIndex = 0;
	}

	void writeUInt(uint32_t x)
	{
		int numNewBytes = 0;
		uint8_t newBytes[5];

		do
		{
			newBytes[numNewBytes++] = x & 0x7F;
			x = x >> 7;
		} while(x > 0);

		for(int i = numNewBytes-1; i > 0; i--)
			bytes.push_back(newBytes[i] | 0x80);
		bytes.push_back(newBytes[0]);
	}

	uint32_t readUInt()
	{
		uint32_t value = 0;
		bool hasNext = true;
		while(hasNext)
		{
			uint8_t byte = bytes[readIndex++];
			hasNext = ((byte & 0x80) != 0);
			value = value * 128 + (byte & 0x7F);
		}
		DEBUGASSERT(readIndex <= (int)bytes.size());
		return value;
	}

	void write(const MTD& mtd, const vector<int>& matchNumTeams)
	{
		if(mtd.degree == 0)
			return;

		//Compute the number of teams between this and the previous
		int delta = -writeHead.team;
		for(int m = writeHead.match; m < mtd.match; m++)
			delta += matchNumTeams[m];
		delta += mtd.team;

		//Assert that the top three bits are zero, which also means delta is nonnegative
		DEBUGASSERT((delta & 0xE0000000) == 0);

		delta = 2*delta + (mtd.degree != 1);

		writeUInt(delta);

		if(mtd.degree != 1)
		{
			//Fold the degree to force it to be positive. Zero is unused because of the return at the top,
			//so we map negatives to the positive evens and positives to the positive odds
			int folded = (mtd.degree < 0) ? -mtd.degree * 2 - 2 : mtd.degree * 2 - 1;
			writeUInt(folded);
		}

		writeHead = mtd;
	}

	bool canRead()
	{
		return readIndex < (int)bytes.size();
	}

	MTD read(const vector<int>& matchNumTeams)
	{
		int delta = readUInt();
		int degree = (delta & 0x1) ? readUInt() : 1;
		delta = delta >> 1;

		//Compute the team and match based on the delta from the previous
		delta += readHead.team;
		int match = readHead.match;
		while(delta - matchNumTeams[match] >= 0)
		{
			delta = delta - matchNumTeams[match];
			match++;
		}
		int team = delta;

		//Unfold the degree
		if(degree & 0x1)
			degree = (degree + 1) >> 1;
		else
			degree = -((degree >> 1) + 1);

		readHead = MTD(match,team,degree);
		return readHead;
	}
};

static double computeLogProb(const vector<vector<double> >& matchTeamStrength, const vector<int>& matchWinners,
		const vector<double>& matchWeight)
{
	int numMatches = (int)matchTeamStrength.size();
	double logProbSum = 0.0;
	for(int m = 0; m<numMatches; m++)
	{
		const vector<double>& teamStrength = matchTeamStrength[m];

		double winLogProb = log(teamStrength[matchWinners[m]]);

		int numTeams = (int)teamStrength.size();
		double totalStrength = 0.0;
		for(int t = 0; t<numTeams; t++)
			totalStrength += teamStrength[t];

		double loseLogProb = -log(totalStrength);
		logProbSum += matchWeight[m] * (winLogProb + loseLogProb);
	}
	return logProbSum;
}

static void updateLogGamma(findex_t feature, double deltaLogGamma, const vector<int>& matchNumTeams,
		vector<MTDByteStream>& featureMatchTeams, vector<vector<double> >& matchTeamLogStrength,
		vector<vector<double> >& matchTeamStrength)
{
	MTDByteStream& matchTeamList = featureMatchTeams[feature];

	matchTeamList.resetRead();
	while(matchTeamList.canRead())
	{
		const MTD& mtd = matchTeamList.read(matchNumTeams);
		int match = mtd.match;
		int team = mtd.team;
		int degree = mtd.degree;
		DEBUGASSERT(match >= 0);
		DEBUGASSERT(team >= 0);
		DEBUGASSERT(match < (int)matchTeamLogStrength.size());
		DEBUGASSERT(team < (int)matchTeamLogStrength[match].size());
		matchTeamLogStrength[match][team] += deltaLogGamma * degree;
		matchTeamStrength[match][team] = exp(matchTeamLogStrength[match][team]);
	}
}

static vector<double> trainGradientHelper(ArimaaFeatureSet afset, int numIters, vector<MTDByteStream>& featureMatchTeams,
		const vector<int>& matchNumTeams, const vector<int>& matchWinners, const vector<double>& matchWeight,
		const vector<bool>& isUnused)
{
	int numFeatures = featureMatchTeams.size();
	int numMatches = matchNumTeams.size();
	int priorIndex = afset.fset->priorIndex;

	vector<double> logGamma;
	logGamma.resize(numFeatures,0.0);

	vector<vector<double> > matchTeamLogStrength;
	vector<vector<double> > matchTeamStrength;
	matchTeamLogStrength.reserve(numMatches);
	for(int m = 0; m<numMatches; m++)
	{
		int numTeams = matchNumTeams[m];
		vector<double> teamLogStrength;
		vector<double> teamStrength;
		teamLogStrength.resize(numTeams,0.0);
		teamStrength.resize(numTeams,1.0);

		matchTeamLogStrength.push_back(teamLogStrength);
		matchTeamStrength.push_back(teamStrength);
	}

	vector<double> deltaSize;
	vector<int> lastType;
	static const int LAST_FAIL = 0;
	static const int LAST_POS = 1;
	static const int LAST_NEG = 2;
	lastType.resize(numFeatures);
	deltaSize.resize(numFeatures,1.0);

	double logProb = computeLogProb(matchTeamStrength,matchWinners,matchWeight);
	for(int iter = 0; iter < numIters; iter++)
	{
		cout << "Training BT iteration " << iter << "/" << numIters << " logProb " << logProb << endl;
		for(int f = 0; f<numFeatures; f++)
		{
			if(f == priorIndex || isUnused[f])
				continue;

			//Twiddle the feature positive to see if the log prob improves
			updateLogGamma(f,deltaSize[f],matchNumTeams,featureMatchTeams,matchTeamLogStrength,matchTeamStrength);
			double logProbPos = computeLogProb(matchTeamStrength,matchWinners,matchWeight);

			if(logProbPos > logProb)
			{
				logGamma[f] += deltaSize[f];
				logProb = logProbPos;
				cout << f << " " << afset.fset->getName(f) << " " << logGamma[f] << " [+" << deltaSize[f] << "] logProb " << logProb << endl;

				if(lastType[f] == LAST_POS)
					deltaSize[f] *= 1.2;
				else
					deltaSize[f] *= 0.9;
				lastType[f] = LAST_POS;
				continue;
			}

			//Twiddle the feature negative to see if the log prob improves
			updateLogGamma(f,-2.0*deltaSize[f],matchNumTeams,featureMatchTeams,matchTeamLogStrength,matchTeamStrength);
			double logProbNeg = computeLogProb(matchTeamStrength,matchWinners,matchWeight);

			if(logProbNeg > logProb)
			{
				logGamma[f] -= deltaSize[f];
				logProb = logProbNeg;
				cout << f << " " << afset.fset->getName(f) << " " << logGamma[f] << " [-" << deltaSize[f] << "] logProb " << logProb << endl;

				if(lastType[f] == LAST_NEG)
					deltaSize[f] *= 1.2;
				else
					deltaSize[f] *= 0.9;
				lastType[f] = LAST_NEG;
				continue;
			}

			//Restore old feature value
			updateLogGamma(f,deltaSize[f],matchNumTeams,featureMatchTeams,matchTeamLogStrength,matchTeamStrength);

			cout << f << " " << afset.fset->getName(f) << " " << logGamma[f] << " [!" << deltaSize[f] << "] logProb " << logProb << endl;
			deltaSize[f] *= 0.6;
			lastType[f] = LAST_FAIL;
		}
	}

	return logGamma;

}

static void addPrior(ArimaaFeatureSet afset, vector<int>& matchWinners,
		vector<int>& matchNumTeams, vector<double>& matchWeight, vector<MTDByteStream>& featureMatchTeams,
		vector<bool>& isUnused)
{
	const vector<FeatureSet::PriorMatch> pMatches = afset.fset->getPriorMatches();
	int priorIndex = afset.fset->priorIndex;

	int numMatches = pMatches.size();
	for(int i = 0; i<numMatches; i++)
	{
		MTD mtd;

		int match = matchWinners.size();

		matchWinners.push_back(0);
		matchNumTeams.push_back(2);
		matchWeight.push_back(pMatches[i].weight);

		mtd.match = match;
		mtd.team = 0;
		mtd.degree = 1;
		featureMatchTeams[pMatches[i].winner].write(mtd,matchNumTeams);

		mtd.match = match;
		mtd.team = 1;
		mtd.degree = 1;
		featureMatchTeams[pMatches[i].loser].write(mtd,matchNumTeams);

		match++;

		if(pMatches[i].winner != priorIndex && pMatches[i].loser != priorIndex)
		{
			isUnused[pMatches[i].winner] = false;
			isUnused[pMatches[i].loser] = false;
		}
	}
}

void BradleyTerry::train(GameIterator& iter)
{
	vector<bool> isUnused;
	vector<int> matchWinners;
	vector<int> matchNumTeams;
	vector<double> matchWeight;
	vector<MTDByteStream> featureMatchTeams;
	isUnused.resize(numFeatures,true);
	featureMatchTeams.resize(numFeatures);

	int match = 0;
	addPrior(afset,matchWinners,matchNumTeams,matchWeight,featureMatchTeams,isUnused);
	match = matchWinners.size();

	int winningTeam;
	vector<vector<findex_t> > teams;
  while(iter.next())
  {
		iter.computeMoveFeatures(afset,winningTeam,teams);
		matchWinners.push_back(winningTeam);
		matchNumTeams.push_back((int)teams.size());
		matchWeight.push_back(1.0);

		int numTeams = teams.size();
		for(int t = 0; t<numTeams; t++)
		{
			const vector<findex_t>& members = teams[t];
			int numMembers = members.size();
			for(int i = 0; i<numMembers; i++)
			{
				MTD mtd;
				mtd.match = match;
				mtd.team = t;
				mtd.degree = 1;
				featureMatchTeams[members[i]].write(mtd,matchNumTeams);
				isUnused[members[i]]= false;
			}
		}
		match++;
  }

	logGamma = trainGradientHelper(afset,numIterations,featureMatchTeams,matchNumTeams,matchWinners,matchWeight,isUnused);
	for(int i = 0; i<numFeatures; i++)
		gamma[i] = exp(logGamma[i]);

}

void BradleyTerry::train(const vector<vector<vector<findex_t> > >& matches, const vector<int>& winners)
{
	vector<bool> isUnused;
	vector<int> matchWinners;
	vector<int> matchNumTeams;
	vector<double> matchWeight;
	vector<MTDByteStream> featureMatchTeams;
	isUnused.resize(numFeatures,true);
	featureMatchTeams.resize(numFeatures);

	int match = 0;
	addPrior(afset,matchWinners,matchNumTeams,matchWeight,featureMatchTeams,isUnused);
	match = matchWinners.size();

	int numMatches = matches.size();
	for(int m = 0; m<numMatches; m++)
	{
		const vector<vector<findex_t> >& teams = matches[m];
		matchWinners.push_back(winners[m]);
		matchNumTeams.push_back((int)teams.size());
		matchWeight.push_back(1.0);

		int numTeams = teams.size();
		for(int t = 0; t<numTeams; t++)
		{
			const vector<findex_t>& members = teams[t];
			int numMembers = members.size();
			for(int i = 0; i<numMembers; i++)
			{
				MTD mtd;
				mtd.match = match;
				mtd.team = t;
				mtd.degree = 1;
				featureMatchTeams[members[i]].write(mtd,matchNumTeams);
				isUnused[members[i]] = false;
			}
		}
		match++;
	}

	logGamma = trainGradientHelper(afset,numIterations,featureMatchTeams,matchNumTeams,matchWinners,matchWeight,isUnused);
	for(int i = 0; i<numFeatures; i++)
		gamma[i] = exp(logGamma[i]);
}

double BradleyTerry::evaluate(const vector<findex_t>& team)
{
  double str = 0.0;
  for(int i = 0; i<(int)team.size(); i++)
    str += logGamma[team[i]];
  return str;
}

void BradleyTerry::outputToFile(const char* file)
{
  ofstream out;
  out.open(file,ios::out);
  out.precision(20);
  out << "BradleyTerry2" << endl;
  out << numFeatures << endl;
  out << numIterations << endl;
  out << (int)0 << endl;  //used to be prior index, NOT USED

  out << "#GameIterator::GAME_KEEP_PROP "<< GameIterator::GAME_KEEP_PROP << endl;
  out << "#GameIterator::MOVE_KEEP_PROP "<< GameIterator::MOVE_KEEP_PROP << endl;
  out << "#GameIterator::MOVE_KEEP_THRESHOLD "<< GameIterator::MOVE_KEEP_THRESHOLD << endl;

  for(int i = 0; i<numFeatures; i++)
    out << afset.fset->getName(i) << "\t" << logGamma[i] << endl;
  out.close();
}

#include "defaultmoveweights.h"

BradleyTerry BradleyTerry::inputFromDefault(ArimaaFeatureSet afset)
{
  return BradleyTerry(afset, 0);

  istringstream in(DEFAULT_MOVE_WEIGHTS);
  if(in.fail())
  {
    Global::fatalError(string("BradleyTerry::inputFromDefault: Failed to input"));
    return BradleyTerry(afset,0);
  }
  return inputFromIStream(afset,in);
}

BradleyTerry BradleyTerry::inputFromFile(ArimaaFeatureSet afset, const char* file)
{
  ifstream in;
  in.open(file,ios::in);
  if(in.fail())
  {
    Global::fatalError(string("BradleyTerry::inputFromFile: Failed to input from ") + string(file));
    return BradleyTerry(afset,0);
  }
  BradleyTerry t = inputFromIStream(afset,in);
  in.close();
  return t;
}

BradleyTerry BradleyTerry::inputFromIStream(ArimaaFeatureSet afset, istream& in)
{
	string header;
	in >> header;
	if(header != string("BradleyTerry2"))
  {
		Global::fatalError(string("BradleyTerry::inputFromIStream: Invalid header"));
    return BradleyTerry(afset,0);
  }

  int numFeatures;
  int numIterations;
  findex_t priorIndex;
  in >> numFeatures;
  in >> numIterations;
  in >> priorIndex; //NOT USED
  BradleyTerry bt(afset,numIterations);

  string line;
  while(!in.fail())
  {
  	getline(in,line);
  	if(in.fail())
  		break;

  	if(line.find('#') != string::npos)
  		line = line.substr(0,line.find_first_of('#'));

  	if(Global::trim(line).length() == 0)
  		continue;

  	istringstream iss(line);
    string name;
    string buf;

  	iss >> name;
  	int i = afset.fset->getFeature(name);
  	if(i == -1)
  	{
  		cout << "BradleyTerry::inputFromIStream: Feature " << name << " not found" << endl;
  		continue;
  	}

  	iss >> buf;
  	bt.logGamma[i] = Global::stringToDouble(buf);
  }

  for(int i = 0; i<bt.numFeatures; i++)
  	bt.gamma[i] = exp(bt.logGamma[i]);

  return bt;
}

//---------------------------------------------------------------------------------------

/*
LeastSquares::LeastSquares(const FeatureSet& fset, int numIterations, findex_t priorIndex)
:fset(fset), numIterations(numIterations), priorIndex(priorIndex)
{
	numFeatures = fset.numFeatures;
  gamma.resize(numFeatures);
  logGamma.resize(numFeatures);

  for(int i = 0; i<numFeatures; i++)
    gamma[i] = 1;

  computeLogGammas();
}

LeastSquares::~LeastSquares()
{

}

void LeastSquares::train(MatchIterator& iter)
{
  cout << "Training Least-Squares" << endl;

  //Extract all the data into memory
  vector<int> winners; //For each match, which team won?
  vector<unsigned long long> numWins; //How many wins does each feature have in total?
  vector<vector<vector<pair<findex_t,double> > > > matches; //All matches
  vector<bool> unused; //For each feature, which ones are unused?
  vector<unsigned long long> numOccurrences; //How many times does each feature occur?
  extractAllEvalFromIter(iter,matches,winners);

  //Training initialization
  int numFeatures = gamma.size();
  numOccurrences.resize(numFeatures);
  numWins.resize(numFeatures);
  unused.resize(numFeatures);
  for(int i = 0; i<numFeatures; i++)
  {
    numWins[i] = 0;
    unused[i] = false;
  }

  //Look for unused features
  int numMatches = matches.size();
  for(int j = 0; j<numMatches; j++)
  {
    const vector<vector<findex_t> >& match = matches[j];
    int numTeams = match.size();
    for(int t = 0; t<numTeams; t++)
    {
      const vector<findex_t>& team = match[t];
      int numMems = team.size();
      for(int m = 0; m<numMems; m++)
        numOccurrences[team[m]]++;
    }
  }
  for(int i = 0; i<numFeatures; i++)
  {
    if(numOccurrences[i] <= 0)
      unused[i] = true;
  }

  //Add a prior if one was requested
  if(priorIndex >= 0 && priorIndex < numFeatures)
  {
    for(int i = 0; i<numFeatures; i++)
    {
      if(priorIndex == i)
        continue;

      vector<vector<findex_t> > teams;
      vector<findex_t> featurePrior(1,priorIndex);
      teams.push_back(featurePrior);
      vector<findex_t> featurei(1,i);
      teams.push_back(featurei);

      matches.push_back(teams);
      winners.push_back(0);

      matches.push_back(teams);
      winners.push_back(1);
    }
  }
  numMatches = matches.size();

  //Count wins and look for unused features
  cout << "Counting wins" << endl;
  int numTeamsTotal = 0;
  for(int j = 0; j<numMatches; j++)
  {
    const vector<vector<findex_t> >& match = matches[j];
    int numTeams = match.size();
    numTeamsTotal += numTeams;
    for(int t = 0; t<numTeams; t++)
    {
    	const vector<findex_t>& team = match[t];
      int numMems = team.size();
      for(int m = 0; m<numMems; m++)
        if(t == winners[j])
          numWins[team[m]]++;
    }
  }

  //Compute strengths
  cout << "Precomputing match and team strengths" << endl;
  cout << "Building feature -> match map" << endl;
  int* matchSizes = new int[numMatches];             //Maps [match] -> number of teams in match
  double** matchStrengths = new double*[numMatches]; //Maps [match][team] -> strength of team
  vector<vector<pair<int,int> > > featureTeams;      //Maps [feature] -> list of pairs of (match,team) containing the feature
  featureTeams.resize(numFeatures);

  //For each match
  for(int i = 0; i<numMatches; i++)
  {
  	vector<vector<findex_t> >& match = matches[i];
  	int numTeams = match.size();
  	matchSizes[i] = numTeams;
  	double* teamStrengths = new double[numTeams];

  	//For each team in the match
		for(int t = 0; t<numTeams; t++)
		{
			const vector<findex_t>& team = match[t];

			//Find the team strength - product of members
			//And also record for each feature the (match,team) for that feature
			double teamStr = 1;
			int numMems = team.size();
			for(int m = 0; m<numMems; m++)
			{
				if(team[m] < 0 || team[m] >= numFeatures)
					cout << "ERROR, bad feature " << team[m] << endl;
				teamStr *= gamma[team[m]];
				featureTeams[team[m]].push_back(make_pair(i,t));
			}
			teamStrengths[t] = teamStr;
		}

  	matchStrengths[i] = teamStrengths;

  	//Delete as we go to save memory
  	match.clear();
  }

  //Destruct the inner vectors, since we don't need them any more
	matches.clear();

  //Perform main iteration
  for(int iters = 0; iters < numIterations; iters++)
  {
    cout << "LeastSquares training iteration: " << iters << endl;
    //For each feature, update its weight
    for(int i = 0; i<numFeatures; i++)
    {
      if(unused[i])
        continue;
      if(i == priorIndex)
        continue;

      if(gamma[i] <= 0)
      	cout << "BT ERROR! Gamma <= 0... " << i << endl;

      //For each match
      int featureTeamsIdx = 0;
      double baseExpectedWins = 0;
      for(int j = 0; j<numMatches; j++)
      {
        double matchStr = 0;  //Overall total strength in this match
        double matchAllyStr = 0;   //Overall strength of allies

        //For each team in the match
        int numTeams = matchSizes[j];
        for(int t = 0; t<numTeams; t++)
        {
          double teamStr = matchStrengths[j][t];
          matchStr += teamStr;

          pair<int,int> nextTeamWithFeature = featureTeams[i][featureTeamsIdx];
          if(nextTeamWithFeature.first == j && nextTeamWithFeature.second == t)
          {
          	featureTeamsIdx++;

          	//Find the combined strength of the other members
          	double teamAllyStr = teamStr/gamma[i];
          	matchAllyStr += teamAllyStr;
          }
        }
        baseExpectedWins += matchAllyStr/matchStr;
      }

      //Update!
      double oldGamma = gamma[i];
      double newGamma = numWins[i]/baseExpectedWins;
      gamma[i] = newGamma;

      //Update team strengths
      double ratio = newGamma/oldGamma;
      int featureTeamsSize = featureTeams[i].size();
      for(int a = 0; a<featureTeamsSize; a++)
      {
      	pair<int,int> nextTeamWithFeature = featureTeams[i][a];
      	matchStrengths[nextTeamWithFeature.first][nextTeamWithFeature.second] *= ratio;
      }

      cout << i << " " << fset.featureName[i] << " " << gamma[i] << " from " << oldGamma << endl;
    }
  }

  for(int i = 0; i<numMatches; i++)
  	delete[] matchStrengths[i];
  delete[] matchStrengths;

  computeLogGammas();

  cout << "Total Teams: " << numTeamsTotal << endl;
  cout << "Total Matches: " << numMatches << endl;
  cout << "Average teams per match: " << (double)numTeamsTotal/numMatches << endl;
  cout << "Done!" << endl;

}


void LeastSquares::computeLogGammas()
{
  for(int i = 0; i<numFeatures; i++)
  {
  	double d = gamma[i];
  	if(d <= 0)
  		cout << "BradleyTerry ERROR - Gamma <= 0!" << fset.featureName[i] << " " << d << endl;
    logGamma[i] = log(d);
  }
}

double LeastSquares::evaluate(const vector<findex_t>& team)
{
  double str = 1.0;
  for(int i = 0; i<(int)team.size(); i++)
    str *= gamma[team[i]];
  return str;
}

double LeastSquares::evaluateLog(const vector<findex_t>& team)
{
  double str = 0.0;
  for(int i = 0; i<(int)team.size(); i++)
    str += logGamma[team[i]];
  return str;
}

void LeastSquares::outputToFile(const char* file)
{
  ofstream out;
  out.open(file,ios::out);
  out.precision(14);
  out << numFeatures << endl;
  out << numIterations << endl;
  out << priorIndex << endl;

  for(int i = 0; i<numFeatures; i++)
    out << fset.featureName[i] << " " << gamma[i] << endl;
  out.close();
}

LeastSquares LeastSquares::inputFromFile(const FeatureSet& fset, const char* file)
{
  ifstream in;
  in.open(file,ios::in);
  if(in.fail())
  {
    cout << "Failed to input from " << file << endl;
    return LeastSquares(fset,0,-1);
  }
  int numFeatures;
  int numIterations;
  findex_t priorIndex;
  in >> numFeatures;
  in >> numIterations;
  in >> priorIndex;
  LeastSquares bt(fset,numIterations,priorIndex);

  double junk;
  string name;
  while(!in.fail())
  {
  	in >> name;
  	int i = fset.getFeature(name);
  	if(i != -1)
  	{
  			in >> bt.gamma[i];
  	}
  	else
  	{
  		cout << "Feature " << name << " not found" << endl;
  		in >> junk;
  	}
  }
  in.close();

  bt.computeLogGammas();

  return bt;
}


*/


//---------------------------------------------------------------------------------------

/*
SVMLearner::SVMLearner(double c)
{
  svmparams.svm_type = C_SVC;
  svmparams.kernel_type = LINEAR;
  svmparams.cache_size = 200;
  svmparams.eps = 0.001;
  svmparams.C = c;
  svmparams.nr_weight = 0;
  svmparams.weight_label = NULL;
  svmparams.weight = NULL;
  svmparams.shrinking = 1;
  svmparams.probability = 0;

  svmparams.degree = 3;
  svmparams.gamma = 0;
  svmparams.coef0 = 0;
  svmparams.nu = 0.5;
  svmparams.p = 0.1;

  svmmodel = NULL;
}

SVMLearner::~SVMLearner()
{
  if(svmmodel != NULL)
    svm_destroy_model(svmmodel);
  for(int i = 0; i<svmprob.l; i++)
    delete[] svmprob.x[i];
  delete[] svmprob.x;
  delete[] svmprob.y;
}

static struct svm_node* convertTeamToSVMNodes(const vector<findex_t>& t)
{
  vector<findex_t> team = t;
  std::sort(team.begin(),team.end());
  int numMembers = team.size();
  struct svm_node* nodes = new struct svm_node[numMembers+1];
  for(int f = 0; f<numMembers; f++)
  {
    nodes[f].index = team[f];
    nodes[f].value = 1.0;
  }
  nodes[numMembers].index = -1;
  nodes[numMembers].value = 0.0;
  return nodes;
}

void SVMLearner::train(MatchIterator& iter)
{
  cout << "Training SVM" << endl;

  //Extract all the data into memory
  vector<int> winners;
  vector<vector<vector<findex_t> > > matches;
  extractAllFromIter(iter,matches,winners);

  //Count instances of the problem
  int numInstances = 0;
  int numMatches = matches.size();
  for(int m = 0; m<numMatches; m++)
    numInstances += matches[m].size();

  //Initialize svm problem
  svmprob.l = numInstances;
  svmprob.x = new struct svm_node*[numInstances];
  svmprob.y = new double[numInstances];

  //Convert data to libsvm format
  int i = 0;
  int totalNumTeams = 0;
  for(int m = 0; m<numMatches; m++)
  {
    vector<vector<findex_t> >& match = matches[m];
    int numTeams = match.size();
    for(int t = 0; t<numTeams; t++)
    {
      totalNumTeams += numTeams;
      vector<findex_t>& team = match[t];
      svmprob.x[i] = convertTeamToSVMNodes(team);
      svmprob.y[i] = (t == winners[m]) ? 1.0 : 0.0;
      i++;
    }
    match.clear();
  }
  winners.clear();
  matches.clear();

  if(i != svmprob.l)
    cout << "ERROR: i != svmprob.l" << endl;

  //Scale weight penalties
  svmparams.nr_weight = 2;
  svmparams.weight_label = new int[2];
  svmparams.weight = new double[2];
  svmparams.weight_label[0] = 0;
  svmparams.weight_label[1] = 1;
  svmparams.weight[0] = (double)numMatches/totalNumTeams;
  svmparams.weight[1] = 1.0;

  const char* err = svm_check_parameter(&svmprob,&svmparams);
  if(err != NULL)
  {
    cout << err << endl;
    return;
  }

  cout << "Data converted. Begin training..." << endl;
  svmmodel = svm_train(&svmprob,&svmparams);
}

double SVMLearner::evaluate(const vector<findex_t>& team)
{
  struct svm_node* nodes = convertTeamToSVMNodes(team);
  double decision_values[1];
  svm_predict_values(svmmodel,nodes,decision_values);
  delete[] nodes;
  return svm_get_label(svmmodel,0) == 1 ? decision_values[0] : -decision_values[0];
}

void SVMLearner::outputToFile(const char* file)
{
  if(svmmodel != NULL)
  {
    int success = svm_save_model(file, svmmodel);
    if(success == -1)
      cout << "Error - could not save model" << endl;
  }
}

SVMLearner SVMLearner::inputFromFile(const char* file)
{
  SVMLearner learner(1.0);
  learner.svmmodel = svm_load_model(file);
  if(svm_check_probability_model(learner.svmmodel) == 0)
    cout << "Error - no probability model" << endl;
  return learner;
}

*/


