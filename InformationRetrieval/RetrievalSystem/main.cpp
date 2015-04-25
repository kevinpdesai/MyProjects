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

string dataDir, resourcesDir, filesDir;
string lembin, stopwFile, annoFile, queryResults, indexFile, wordNetFile, urlsFile, urlmapFile, docrfsFile, query;
bool hasUpdate = false;

set<string> stopWords;
vector<string> files;

// file ID (para no) and corresponding file name
map <uint, string> fileNames;

// map to map pno to urls
map <uint, string> pnourls;

//intermediate
map <uint, string> unourls;
map <uint, uint> pnouno;

// Map to store RFN of words
map <string, uint> wordNet;

map <uint, vector< pair <string, uint> > > docrfs;

// Map to store the lemmas, posting entry
map<string, vector< postingEntry > > lemmas;

lemmatizer *lemmatize = new lemmatizer();

void addStopWords(const char file[])
{
	fileReader *f = new fileReader(file);
	if(f->_errorReadingFile)
		return;
	while(f->getNextWord())
		stopWords.insert(f->word);
	delete f;
}

void readDir(const char dirName[])
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
void parseDir(const char dirName[]) {
	string file;
	string fileName(dirName);
	fileName += delimeter;
	for(uint i=0; i<files.size(); i++) {
		file = fileName + files[i];
		uint id = atoi(splitString(files[i], '.')[1].c_str());
		fileNames[id] = file;
		if(pnourls.count(id) == 0) {
			parseFile(file, id);
			hasUpdate = true;
		}
	}
}

// Parse the vector of the files in the directory.
// void parseDir(const char dirName[]) {
// 	string file;
// 	string fileName(dirName);
// 	fileName += "/";
// 	for(uint i=0; i<files.size(); i++) {
// 		file = fileName + files[i];
// 		uint id = atoi(splitString(files[i], '.')[1].c_str());
// 		fileNames[id] = file;
// 	}
// }
// 
// void parseDirAll(const char dirName[]) {
// 	string file;
// 	string fileName(dirName);
// 	string conff = resourcesDir + "/conflicts.txt";
// 	ofstream ofs(conff.c_str());
// 	fileName += "/";
// 	for(uint i=0; i<files.size(); i++) {
// 		file = fileName + files[i];
// 		uint id = atoi(splitString(files[i], '.')[1].c_str());
// 		if (fileNames.count(id)) { ofs << id << endl; }
// 		fileNames[id] = file;
// 		parseFile(file, id);
// 	}
// 	ofs.close();
// }


// Write the indices.
void writeIndex()
{
	fileWriter *fw = new fileWriter(indexFile,lemmas);
	fw->writeProjIndexToFile();
	delete fw;
}

void readIndexFromFile()
{
	// Open file and check if it is opened correctly.
	indexFileReader *idf = new indexFileReader(indexFile,true);
	idf->readIndex();
	lemmas = idf->_projIndex;
	delete idf;
}

void writeWordNetToFile(string filename) {
	fileWriter *fw = new fileWriter(filename, wordNet);
	fw->writeWordNetToFile();
	delete fw;
}

void readWordNetFromFile(string filename) {
	indexFileReader *idf = new indexFileReader(filename);
	idf->readWordNet();
	wordNet = idf->_wordNet;
	delete idf;
}

void writeDocRFsToFile(string filename) {
	fileWriter *fw = new fileWriter(filename, docrfs);
	fw->writeDocRFsToFile();
	delete fw;
}

void readDocRFsFile(string filename) {
	indexFileReader *idf = new indexFileReader(filename);
	idf->readDocRFsFile();
	docrfs = idf->_docRF;
	delete idf;
}

void writeURLMapToFile(string filename) {
	fileWriter *fw = new fileWriter(filename, pnourls);
	fw->writeURLMapToFile();
	delete fw;
}

void readURLMapFromFile (string filename) {
	indexFileReader *idf = new indexFileReader(filename);
	idf->readURL();
	pnourls = idf->_urls;
	delete idf;
}

float calcBasicScore(const vector<string> &words, uint docid) {
	float score = 0.0;
	for (uint i = 0; i < words.size(); i++) {
		if (lemmas.count(words[i]) == 0)
			continue;
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
	for (uint i=0; i<words.size(); i++) {
		if (lemmas.count(words[i]) == 0)
			continue;
		for (uint j = 0; j < lemmas[words[i]].size(); j++) {
			if (lemmas[words[i]][j].docId == docid) {
				float weight = 1, relf = 1;
				if (wordNet.count(words[i]) != 0) {
					int wrfn = wordNet[words[i]];
					if (wrfn == 1)			weight = 5;
					else if (wrfn == 8)	weight = 1.3;
					else if (wrfn == 6 || wrfn == 7) {
						weight = 1.25;
						for (uint k=0; k<docrfs[docid].size(); k++) {
							if (docrfs[docid][k].second == 6 || docrfs[docid][k].second == 7)	relf+=0.1;
						}
					}
					else {
						for (uint k=0; k<docrfs[docid].size(); k++) {
							if (docrfs[docid][k].second == wrfn)	relf += 0.25;
						}
					}
				}
				score += (0.75 * weight * relf) + (0.5 * (1 + log10(lemmas[words[i]][j].tf)) * log10 (float(fileNames.size()) / lemmas[words[i]].size()));
			}
		}
	}
	return score;
}

void processQuery(const string &query) {
	vector<string> _words = splitString(query, ' ');
	vector<string> words;
	for (uint i = 0; i < _words.size(); i++) {
		tokenizer *t = new tokenizer(_words[i]);
		t->tokenize();
		if (t->_skipWord)
			continue;
		char *c = lemmatize->Lemmatize(t->_word.c_str());
		string lemma(c);
		if (lemma.length() < 1)
			continue;
		words.push_back(lemma);
		delete t;
		delete c;
	}

	// heap for maintaining scores
	priority_queue < pair <float, uint> > qscore;

	// process all docids for query terms, get scores and heapify it
	for (map<uint, string>::iterator it = fileNames.begin(); it != fileNames.end(); it++) {
		float score;
		//score = calcBasicScore(words, it->first);
		score = calcRocketsScore(words, it->first);
		qscore.push(pair<float, uint>(score, it->first));
	}

	ofstream ofs(queryResults.c_str(), ios::app);
	ofs << "Query : " << query << endl;
	ofs << "Scoring : TF-IDF " << endl;
	// fetch top 10 results from heap
	for (uint i = 0; i < 10; i++) {
		ofs << "Doc ID = " << qscore.top().second << "\tScore = " << qscore.top().first << endl;
		ifstream ifs(fileNames[qscore.top().second].c_str());
		if (!ifs ) { cout << "File does not exist" << endl; continue; }
		int ccount = 0; char c;
		while (ccount < 300 && !ifs.eof()) {
			ccount++;
			ifs >> noskipws >> c;
			if (c == '\n' || c == '\t')
				c = ' ';
			cout << c;
		}
		if (!ifs.eof())
			cout << "...";
		cout << endl;
		ifs.close();
		if (pnourls.count(qscore.top().second) != 0)
			cout << pnourls[qscore.top().second] << endl;
		else
			cout << "URL unavailable" << endl;
		qscore.pop();
	}

	ofs.close();
}

void setMetaData() {
	//read urls.csv file
	CSVReader urls(urlsFile);
	for (uint i=0; i<urls._data.size(); i++)
		unourls[atoi(urls._data[i][0].c_str())] = urls._data[i][1];

	// read annotations file
	CSVReader anno(annoFile);
	// populate wordNet dictionary by only considering first word in annotations text
	for (uint i = 0; i < anno._data.size(); i++) {

		uint uno = atoi (anno._data[i][0].c_str());
		uint docid = atoi (anno._data[i][1].c_str());

		if (unourls.count (uno) != 0)
			pnourls[docid] = unourls[uno];

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

		docrfs[docid].push_back(pair<string, uint> (lemma, atoi(anno._data[i][2].c_str())));

		if (wordNet.count(lemma) != 0)
			continue;
		wordNet[lemma] = atoi(anno._data[i][2].c_str());
	}

	// write wordNet dictionary to file
	writeURLMapToFile(urlmapFile);
	writeWordNetToFile(wordNetFile);
	writeDocRFsToFile(docrfsFile);
}


void setupResources() {
	cout << "Setting up resources (index, wordnet, RFMap)" << endl;
	parseDir(filesDir.c_str());
	writeIndex();
	setMetaData();
}

int main(int argc, char *argv[]) {
	if (argc != 4) {
		cout << "Invalid arguments!" << endl;
		cout << "Usage: <data directory> <resources directory> <query>";
		exit(0);
	}
	else {
		dataDir = argv[1];
		resourcesDir = argv[2];
		filesDir = dataDir + "/files";
		annoFile = dataDir + "/annotations.csv";
		urlsFile = dataDir + "/urls.csv";
		urlmapFile = resourcesDir + "/urlmap.txt";
		lembin = resourcesDir + "/lem-m-en.bin";
		stopwFile = resourcesDir + "/common_words";
		indexFile = resourcesDir + "/index.txt";
		wordNetFile = resourcesDir + "/wordNet.txt";
		docrfsFile = resourcesDir + "/docrfsFile.txt";
		queryResults = resourcesDir + "/queryResults.txt";
		query = argv[3];
	}

	lemmatize->LoadBinary(lembin.c_str());

	addStopWords(stopwFile.c_str());
	readDir(filesDir.c_str());

	if (query[0] == '0') {
		setupResources();
	}
	else {
		// read URL map from file
		readURLMapFromFile(urlmapFile);
		// read file list
		parseDir(filesDir.c_str());

		if (! hasUpdate) {
			// read index
			readIndexFromFile();
			// read wordNet from file
			readWordNetFromFile(wordNetFile);
			// read RFMap from file
			readDocRFsFile(docrfsFile);
		} else {
			// index updated. write new index to file.
			writeIndex();
			// read urls.csv and annotations.csv and write wordNet, docrfs and urlmap.
			setMetaData();
		}

		//		cout <<"Number of tokens in dict = " << lemmas.size() << endl;
		//		cout << "Number of files = " << fileNames.size() << endl;
		//		cout << "WordNet size = " << wordNet.size() << endl;
		//		cout << "Document RFNS = " << docrfs.size() << endl;
	}

	timer *t = new timer();
	if (query[0] != '0'){
		processQuery(query);
	}

	// time to execute entire program
	t->stopTimer();
	cout << t->getTimeTaken() << " ms" << endl;
	return 0;
}
