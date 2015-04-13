#include "global.h"
#include "timer.h"
#include "lemmatizer.h"
#include "fileReader.h"
#include "dirReader.h"
#include "tokenizer.h"
#include "stemmer.h"
#include "fileWriter.h"

#ifdef windows
string delimeter = "\\";
#endif

#ifdef linux
string delimeter = "/";
#endif

set<string> stopWords;
vector<string> files;

// Store info for each file - max_tf and doc length.
map<int, pair<int,int> > docInfoLemmas;
int maxDocLemma=1;

// Store info for each file - max_tf and doc length.
map<int, pair<int,int> > docInfoStems;
int maxDocStem=1;

// Variable used to calculate the total number of lemmas/stems (Words).
long nFileWords = 0;

// Map to store the LEMMAS
map<string, vector<pair<int, int> > > lemmas;

// Map to store the STEMS
map<string, vector<pair<int, int> > > stems;

lemmatizer *lemmatize = new lemmatizer();

void addStopWords(char file[])
{
	fileReader *f = new fileReader(file);
	if(f->errorReadingFile)
		return;
	while(f->getNextWord())
		stopWords.insert(f->word);
	delete f;
}

void readDir(char dirName[])
{
	dirReader *d = new dirReader(dirName);
	if(d->errorReadingDir)
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

// Add stem in the stem dictionary.
void addStem(string word, int doc_id) {
	// Convert word from string to char*
	char* c = (char*)word.c_str();
	// Stem the word.
	char* st = porter_stem(c);
	string stem(st);
	if(stem.length()<1)
		return;
	// Check if it is part of stop-words.
	if(stopWords.count(stem))
		return;
	// Add in dictionary.
	if(stems[stem].size()==0)
		stems[stem].push_back(pair<int,int>(doc_id,1));
	else {
		if((stems[stem][stems[stem].size()-1]).first == doc_id) {
			(stems[stem][stems[stem].size()-1]).second++;
			if(maxDocStem < (stems[stem][stems[stem].size()-1]).second)
				maxDocStem = (stems[stem][stems[stem].size()-1]).second;
		}
		else
			stems[stem].push_back(pair<int,int>(doc_id,1));
	}
}

// Parse the word.
void parseWord(string word, int doc_id) {
	tokenizer *t = new tokenizer(word);
	t->tokenize();
	if(t->skipWord)
		return;
	// Increase the number of words in file.
	nFileWords++;
	// Check if token is part of stop-words.
	if(stopWords.count(t->_word))
		return;
	// Add lemma of token in the dictionary.
	addLemma(t->_word, doc_id);
	// Get and stem in dictionary from the token.
	addStem(t->_word, doc_id);
	delete t;
}

// Parse the file.
void parseFile(string file, int doc_id) {
	fileReader *f = new fileReader((char *)file.c_str());
	if(f->errorReadingFile)
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
		maxDocStem = 1;
		nFileWords = 0;
		parseFile(file,i+1);
		docInfoLemmas[i+1] = pair<int,int>(maxDocLemma,nFileWords);
		docInfoStems[i+1] = pair<int,int>(maxDocStem,nFileWords);
	}
}

// Write the indices.
void writeIndex()
{
	fileWriter *fw = new fileWriter("Index_Version1.uncompress",lemmas);
	fw->writeIndexToFile();
	cout << "\nSize of index Version 1 uncompressed (lemmas) = " << fw->f.tellp() << " B";
	fw = new fileWriter("Index_Version2.uncompress",stems,2);
	fw->writeIndexToFile();
	cout << "\nSize of index Version 2 uncompressed (stems) = " << fw->f.tellp() << " B";
	fw = new fileWriter("Index_Version1.compress",lemmas,1,true);
	fw->writeIndexToFile();
	cout << "\nSize of index Version 1 compressed (lemmas, block, gamma) = " << fw->f.tellp() << " B";
	fw = new fileWriter("Index_Version2.compress",stems,2,true);
	fw->writeIndexToFile();
	cout << "\nSize of index Version 2 compressed (stems, front, delta) = " << fw->f.tellp() << " B";
	cout << "\n\nNumber of inverted lists in index Version 1 (lemmas) = " << lemmas.size();
	cout << "\nNumber of inverted lists in index Version 2 (stems) = " << stems.size();
	fw = new fileWriter("doc_info_lemmas",docInfoLemmas);
	fw->writeDocInfoToFile();
	fw = new fileWriter("doc_info_stems",docInfoStems);
	fw->writeDocInfoToFile();
	delete fw;
}

int main(int argc, char *argv[]) {
	timer *t = new timer();
	lemmatize->LoadBinary("lem-m-en.bin");
	addStopWords(argv[2]);
	readDir(argv[1]);
	parseDir(argv[1]);
	writeIndex();
	t->stopTimer();
	cout << endl << t->getTimeTaken() << " ms" << endl;
#ifdef windows
	_getch();
#endif // windows
	return 0;
}