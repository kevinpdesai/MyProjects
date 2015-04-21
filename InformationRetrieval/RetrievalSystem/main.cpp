#include "global.h"
#include "timer.h"
#include "lemmatizer.h"
#include "fileReader.h"
#include "CSVReader.h"
#include "dirReader.h"
#include "tokenizer.h"
#include "fileWriter.h"
#include "indexFileReader.h"

#ifdef windows
string delimeter = "\\";
#endif

#ifdef linux
string delimeter = "/";
#endif

const string lembin = "resources/lem-m-en.bin";
const string queryResults = "resources/queryResults.txt";
const string wordNetFile = "resources/wordNet.txt";
const string indexFile = "resources/chaputta.txt";

set<string> stopWords;
vector<string> files;

// file ID (para no) and corresponding file name
map <uint, string> fileNames;

// Map to store RFN of words
map <string, uint> wordNet;

// Map to store the lemmas, posting entry
map<string, vector< postingEntry > > lemmas;

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

// Add lemma of token in the lemma dictionary.
void addLemma(string word, uint doc_id) {
	// Lemmatize the word.
	char *c = lemmatize->Lemmatize(word.c_str());
	string lemma(c);
	if(lemma.length()<1)
		return;
	// Check if it is part of stop-words.
	if(stopWords.count(lemma))
		return;
	// Add in dictionary.
	if(lemmas.count(lemma) == 0)
		lemmas[lemma].push_back(postingEntry(1, doc_id));
	else {
		bool isentry = false;
		for (uint i = 0; i < lemmas[lemma].size(); i++) {
			if (lemmas[lemma][i].docId == doc_id) {
				lemmas[lemma][i].tf++;
				isentry = true; 
				break;
			}
		}
		if (!isentry) {
			lemmas[lemma].push_back(postingEntry(1, doc_id));
		}
	}
}

// Parse the word.
void parseWord(string word, uint doc_id) {
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

// Parse the file.
void parseFile(string file, uint doc_id) {
	fileReader *f = new fileReader((char *)file.c_str());
	if(f->_errorReadingFile)
		return;
	while(f->getNextWord())
	{
		// If read word then tokenize it.
		if(f->word.length()>0)
			parseWord(f->word, doc_id);
	}
	delete f;
}

vector<string> splitString(string original, char splitChar) {
	vector<string> result;
	uint starti = 0, endi = 0;
	for (uint i = 0; i < original.size(); i++) {
		if (original[i] == splitChar) {
			if (starti != endi)
				result.push_back(original.substr(starti, endi - starti));
			starti = endi = i + 1;
		}
		else
			endi++;
	}

	if (starti < original.size() - 1 && starti != endi)
		result.push_back(original.substr(starti, endi));
	return result;
}

// Parse the vector of the files in the directory.
void parseDir(char dirName[]) {
	string file;
	string fileName(dirName);
	ofstream ofs("conflicts.txt");
	fileName += delimeter;
	for(uint i=0; i<files.size(); i++) {
		file = fileName + files[i];
		uint id = atoi(splitString(files[i], '.')[1].c_str());
		if (fileNames.count(id)) {
			ofs << id << endl;
		}
		fileNames[id] = file;
		parseFile(file, id);
	}
	ofs.close();
}

// Write the indices.
void writeIndex()
{
	fileWriter *fw = new fileWriter("chaputta.txt",lemmas);
	fw->writeProjIndexToFile();
	delete fw;
}

void readIndexFromFile()
{
	// Open file and check if it is opened correctly.
	indexFileReader *idf = new indexFileReader("chaputta.txt",true);
	idf->readIndex();
	delete idf;
}

void writeWordNetToFile(string filename) {
	fileWriter *fw = new fileWriter("wordNet.txt", wordNet);
	fw->writeWordNetToFile();
	delete fw;
}

void readWordNetFromFile(string filename) {
	ifstream ifs(filename.c_str());
	if (!ifs) {
		cout << "Error reading file!" << endl;
		return;
	}
	string word;
	uint rfn;
	while (!ifs.eof()) {
		ifs >> word >> rfn;
		wordNet[word] = rfn;
	}
	ifs.close();
}


float calcBasicScore(const vector<string> &words, uint docid) {
	float score = 0.0;
	for (uint i = 0; i < words.size(); i++) {
		if (lemmas.count(words[i]) == 0)
			break;
		for (uint j = 0; j < lemmas[words[i]].size(); j++) {
			if (lemmas[words[i]][j].docId == docid) {
				score += (1 + log10(lemmas[words[i]][j].tf)) * log10 (float(fileNames.size()) / lemmas[words[i]].size());
			}
		}
	}
	return score;
}

float calcRocketsScore(const vector<string> &words, uint docid) {
	float score = 0.0;

	return score;
}

void processQuery(const string &query) {
	vector<string> _words = splitString(query, ' ');
	vector<string> words;
	for (uint i = 0; i < _words.size(); i++) {
		tokenizer *t = new tokenizer(_words[i]);
		t->tokenize();
		if (t->_skipWord)
			return;
		char *c = lemmatize->Lemmatize(t->_word.c_str());
		string lemma(c);
		if (lemma.length() < 1)
			return;
		if (stopWords.count(lemma))
			return;

		words.push_back(lemma);
		delete t;
		delete c;
	}
	// might be useful in future
	vector<uint> wrfns;
	for (uint i = 0; i < words.size(); i++) {
		if (wordNet.count(words[i]) == 0)
			wrfns.push_back(0);
		else
			wrfns.push_back(wordNet[words[i]]);
	}
	// heap for maintaining scores
	priority_queue < pair <float, uint> > qscore;
	// process all docids for query terms, get scores and heapify it
	for (map<uint, string>::iterator it = fileNames.begin(); it != fileNames.end(); it++) {
		float score = calcBasicScore(words, it->first);
		qscore.push(pair<float, uint>(score, it->first));
	}
	ofstream ofs(queryResults.c_str(), ios::app);
	ofs << "Query : " << query << endl;
	ofs << "Scoring : TF-IDF " << endl;
	// fetch top 10 results from heap
	for (uint i = 0; i < 10; i++) {
		ofs << "Doc ID = " << qscore.top().second << "\tScore = " << qscore.top().first << endl;
		qscore.pop();
	}
	ofs.close();
}

int main(int argc, char *argv[]) {
	timer *t = new timer();
	lemmatize->LoadBinary("lem-m-en.bin");
	addStopWords(argv[2]);
	readDir(argv[1]);
	parseDir(argv[1]);
	//writeIndex();
	
	readIndexFromFile();

	cout <<"Number of tokens in dict = " << lemmas.size() << endl;
	cout << "Number of files = " << fileNames.size() << endl;
	
	// read annotations file
	CSVReader anno(argv[3]);
	// populate wordNet dictionary by only considering first word in annotations text
	for (int i = 0; i < anno._data.size(); i++) {
		if (anno._data[i][3].size() == 0)
			continue;
		string word = splitString(anno._data[i][3], ' ')[0];
		tokenizer *t = new tokenizer(word);
		t->tokenize();
		if (t->_skipWord)
			continue;
		char *c = lemmatize->Lemmatize(t->_word.c_str());
		string lemma(c);
		if (lemma.length() < 1)
			continue;
		// Check if it is part of stop-words.
		if (stopWords.count(lemma))
			continue;
		if (wordNet.count(lemma) != 0)
			continue;
		wordNet[lemma] = atoi(anno._data[i][2].c_str());
	}
	// write wordNet dictionary to file
	writeWordNetToFile("wordNet.txt");

	// read wordNet from file
	readWordNetFromFile("wordNet.txt");

	cout << "WordNet size = " << wordNet.size() << endl;

	string query = "Ebola disease incubation period range";
	processQuery(query);

	// read judgment file
	//CSVReader judge(argv[4]);

	//readIndexFromFile();
	// time to execute entire program
	t->stopTimer();
	cout << endl << t->getTimeTaken() << " ms" << endl;
#ifdef windows
	_getch();
#endif // windows
	return 0;

}