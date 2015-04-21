#include "global.h"
#include "timer.h"
#include "lemmatizer.h"
#include "fileReader.h"
#include "tokenizer.h"
#include "dirReader.h"

#ifdef windows
string delimeter = "\\";
#endif

#ifdef linux
string delimeter = "/";
#endif

set<string> stopWords;
vector<string> files;
set<string> queries[20];
string directoryName;

// Store info for each file - max_tf and doc length.
map<int, pair<int,int> > docInfoLemmas;
int maxDocLemma=1;

// Variable used to calculate the total number of lemmas/stems (Words).
long nFileWords = 0;
double avgDocLen = 0.0;

// Map to store the LEMMAS
map<string, vector<pair<int, int> > > lemmas;

double queryScore1[20][1400];
double queryScore2[20][1400];
vector<int> queryScoreIndex1[20];
vector<int> queryScoreIndex2[20];

int collectionSize;

lemmatizer *lemmatize = new lemmatizer();


void addStopWords(char file[])
{
	fileReader *f = new fileReader(file);
	if(f->_errorReadingFile)
		return;
	while(f->getNextWord())
		stopWords.insert(f->word);
	delete f;
}

// Add lemma of token in the lemma dictionary.
void addQueryLemma(string word, int queryNo) {
	// Lemmatize the word.
	char *c = lemmatize->Lemmatize(word.c_str());
	string lemma(c);
	if(lemma.length()<1)
		return;
	// Check if it is part of stop-words.
	if(stopWords.count(lemma))
		return;
	// Add in dictionary.
	if(queries[queryNo-1].size()==0)
	{
		set<string> ss;
		ss.insert(lemma);
		queries[queryNo-1] = ss;
	}
	else 
	{
		if(!queries[queryNo-1].count(lemma))
			queries[queryNo-1].insert(lemma);
	}
}

// Parse the word.
void parseQueryWord(string word, int queryNo) {
	tokenizer *t = new tokenizer(word);
	t->tokenize();
	if(t->_skipWord)
		return;
	// Check if token is part of stop-words.
	if(stopWords.count(t->_word))
		return;
	// Add lemma of token in the dictionary.
	addQueryLemma(t->_word, queryNo);
	delete t;
}

// Parse the query file.
void parseQueryFile(string file) {
	fileReader *f = new fileReader((char *)file.c_str());
#ifdef windows
	regex r("[a-zA-Z][0-9]+:");
#endif // windows

#ifdef linux
	regex_t r;
	const char *reg = "[a-zA-Z][0-9]+:";
	regcomp(&r,reg,REG_EXTENDED);
#endif // linux

	if(f->_errorReadingFile)
		return;
	int queryNo = 0;;
	while(f->getNextWord())
	{
		// Skip the file until we are in the <TEXT> & </TEXT> tag.
		if(f->checkWord(r)) {
			queryNo++;
			continue;
		}
		// If read word then tokenize it.
		if(f->word.length()>0)
			parseQueryWord(f->word,queryNo);
	}
	delete f;
}

// Add lemma of token in the lemma dictionary.
void addLemma(string word, int doc_id) {
	// Lemmatize the word.
	char *c = lemmatize->Lemmatize(word.c_str());
	string lemma(c);
	if(lemma.length()<1)
		return;
	// Check if it is part of stop-words.
	if(stopWords.count(lemma))
		return;
	// Increase the number of words in file.
	nFileWords++;
	// Add in dictionary.
	if(lemmas[lemma].size()==0)
		lemmas[lemma].push_back(pair<int,int>(doc_id,1));
	else {
		if((lemmas[lemma][lemmas[lemma].size()-1]).first == doc_id) {
			(lemmas[lemma][lemmas[lemma].size()-1]).second++;
			if(maxDocLemma < (lemmas[lemma][lemmas[lemma].size()-1]).second)
				maxDocLemma = (lemmas[lemma][lemmas[lemma].size()-1]).second;
		}
		else
			lemmas[lemma].push_back(pair<int,int>(doc_id,1));
	}
}

// Parse the word.
void parseWord(string word, int doc_id) {
	tokenizer *t = new tokenizer(word);
	t->tokenize();
	if(t->_skipWord)
		return;
	// Check if token is part of stop-words.
	if(stopWords.count(t->_word))
		return;
	// Add lemma of token in the dictionary.
	addLemma(t->_word, doc_id);
	delete t;
}


void readDir(char dirName[])
{
	dirReader *d = new dirReader(dirName);
	if(d->_errorReadingDir)
		return;
	while(d->getNexFileName())
	{
		if(d->nextFileName == "." || d->nextFileName == "..")
			continue;
		files.push_back(d->nextFileName);
	}
#ifdef linux
	sort(files.begin(),files.end());
#endif // linux
	delete d;
}

// Parse the file.
void parseFile(string file, int doc_id) {
	fileReader *f = new fileReader((char *)file.c_str());
	if(f->_errorReadingFile)
		return;
	bool textField = false;
	while(f->getNextWord())
	{
		// Skip the file until we are in the <TEXT> & </TEXT> tag.
		if(f->checkWord("<TEXT>")) {
			textField = true;
			continue;
		}
		if(f->checkWord("</TEXT>"))
			textField = false;
		if(!textField)
			continue;
		// If read word then tokenize it.
		if(f->word.length()>0)
			parseWord(f->word, doc_id);
	}
	delete f;
}

// Parse the vector of the files in the directory.
void parseDir(char dirName[]) {
	string file;
	string fileName(dirName);
	fileName += delimeter;
	for(int i=0; i<files.size(); i++) {
		file = fileName + files[i];
		maxDocLemma = 1;
		nFileWords = 0;
		parseFile(file,i+1);
		docInfoLemmas[i+1] = pair<int,int>(maxDocLemma,nFileWords);
		avgDocLen+=nFileWords;
	}
	avgDocLen/=files.size();
}

void printHeadline(string file)
{
	fileReader *f = new fileReader((char *)file.c_str());
	if(f->_errorReadingFile)
		return;
	bool titleField = false;
	while(f->getNextWord())
	{
		// Skip the file until we are in the <TEXT> & </TEXT> tag.
		if(f->checkWord("<TITLE>")) {
			titleField = true;
			continue;
		}
		if(f->checkWord("</TITLE>")) {
			titleField = false;
			break;
		}
		if(!titleField)
			continue;
		// If read word in title then print it.
		if(f->word.length()>0)
			cout << f->word << " ";
	}
	delete f;
}

void printTopResultsForQuery(double qs[], int topN = 5)
{
	priority_queue<pair<double, int> > q;
	for (int i = 0; i < collectionSize; i++) {
		q.push(pair<double, int>(qs[i], i+1));
	}
	for (int i = 0; i < topN; i++) {
		int ki = q.top().second;
		cout << "\n" << i+1 << "\t" << qs[ki-1] << "\t" << files[ki] << "\t";
		printHeadline(directoryName+delimeter+files[ki]);
		q.pop();
	}
}

void printTopResultsForQueries()
{
	for(int i=0; i<20; i++)
	{
		cout << "Query " << i+1 << " : ";
		for(set<string>::iterator it = queries[i].begin(); it!=queries[i].end(); it++)
		{
			cout << *it << " ";
		}
		cout << "\nMethod 1";
		cout << "\nRank\tScore\tDocName\t\tHeadline";
		printTopResultsForQuery(queryScore1[i]);
		cout << "\nMethod 2";
		cout << "\nRank\tScore\tDocName\t\tHeadline";
		printTopResultsForQuery(queryScore2[i]);
		cout<<"\n\n";
	}
}

void processQueries()
{
	memset(queryScore1,0,sizeof(queryScore1[0][0])*20*1400);
	memset(queryScore2,0,sizeof(queryScore2[0][0])*20*1400);
	for(int i=0; i<20; i++)
	{
		for(set<string>::iterator it = queries[i].begin(); it!=queries[i].end(); it++ )
		{
			vector<pair<int, int> > v = lemmas[*it];
			if(v.size()<=0)
				continue;
			double s_const = log10(collectionSize/(double)v.size()) / (double)log10(collectionSize);
			int k=0;
			for(int j=0; j<collectionSize; j++)
			{
				int tf = 0;
				if(k<v.size())
				{
					if(v[k].first == j)
					{
						tf = v[k].second;
						k++;
					}
				}
				queryScore1[i][j]+=(0.4 + 0.6*log10(tf+0.5)/log10(docInfoLemmas[j].first + 1.0)) * s_const;
				queryScore2[i][j]+=(0.4 + 0.6*(tf/(tf+0.5+1.5*(docInfoLemmas[j].second/(double)avgDocLen)))*s_const);
			}
		}
	}
	printTopResultsForQueries();
}

int main(int argc, char *argv[])
{
	timer *t = new timer();
	lemmatize->LoadBinary("lem-m-en.bin");
	addStopWords(argv[2]);
	directoryName = argv[1];
	readDir((char *)directoryName.c_str());
	parseDir((char *)directoryName.c_str());
	collectionSize = docInfoLemmas.size();
	parseQueryFile(argv[3]);
	processQueries();
	t->stopTimer();
	cout << endl << t->getTimeTaken() << " ms\n";
#ifdef windows
	_getch();
#endif // windows

	return 0;
}