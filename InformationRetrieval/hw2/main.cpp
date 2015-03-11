// Necessary includes for the program
#include<iostream>
#include "porter.h"
#include <fstream>
#include <map>
#include <string>
#include <list>
#include <set>
#include <algorithm>
#include <vector>
#include "RdrLemmatizer.h"

#define windows

#ifdef linux
#include <dirent.h>
#include <sys/time.h>
#endif

#ifdef windows
#include <windows.h>
#include <tchar.h>
#include <conio.h>
#endif



using namespace std;

// Variable used to calculate the total number of lemmas (Words).
long nWords = 0;

// Variables used to calculate the distinct count of lemmas and stems.
int distinctCountLemmas = 0, distinctCountStems = 0;

#ifdef windows
string delimeter = "\\";
#endif

#ifdef linux
string delimeter = "/";
#endif

// Vector to store the file names.
vector<string> files;

// Vector to store the stop words.
set<string> stopWords;

// Map to store the TOKENS
map<string, int> lemmas;

// Map to store the STEMS
map<string, int> stems;

// Vector to keep the sorted dictionary of lemmas and stems.
vector<pair<string,int> > sortedLemmas;
vector<pair<string,int> > sortedStems;

// Sets used to store lemmas and stems in each file and hence used to assist in calculating the distinct lemmas and stems in each file.
set<string> fileLemmas;
set<string> fileStems;

// Initialize the lemmatizer object.
RdrLemmatizer *lemmatize = new RdrLemmatizer();

// Function declaration.
void tokenize(string &);

// Recursively remove the special characters at the right end from the word to be tokenized. E.g. "hello)/(" becomes "hello".
void rightStrip(string &word) {
	char rchar = word[word.length()-1];
	while(!isalnum(rchar)) {
		word.erase(word.length()-1);
		if(word.length()==0)
			return;
		rchar = word[word.length()-1];
	}
}

// Recursively remove the special characters at the left end from the word to be tokenized. E.g. ")/(hello" becomes "hello".
void leftStrip(string &word) {
	char lchar = word[0];
	while(!isalnum(lchar)) {
		word = word.substr(1);
		if(word.length()==0)
			return;
		lchar = word[0];
	}
}

// Remove the possessives while tokenizing. E.g. "denny's" becomes "denny" & "dennys'" becomes "dennys"
void possessiveStrip(string &word) {
	if(word.length()<2)
		return;
	char l1 = word[word.length()-1];
	char l2 = word[word.length()-2];
	if(l1=='s' && l2 == '\'')
		word.erase(word.length()-2,2);
	else if(l1 == '\'')
		word.erase(word.length()-1);
}

// Checks whether the word to be tokenized is an acronym and changes it. E.g. "u.s.a." becomes "usa".
string acronymCheck(string word) {
	string changedWord = "";
	if(word.length()<2)
		return word;
	for(int i=0; i<word.length(); i++) {
		char ch = word[i];
		if(i%2==0) {
			if(ch<97 || ch>122)
				return word;
			else 
				changedWord += ch;
		}
		else {
			if(ch!='.')
				return word;
		}
	}
	return changedWord;
}

// Split the word if it has hyphen "-". E.g. "middle-class" becomes two words "middle" & "class".
bool removeHyphen(string &word) {
	string splitWord;
	bool hasHyphen = false;
	int i=0;
	for(i=0; i<word.length(); i++) {
		if(word[i]=='-') {
			hasHyphen = true;
			splitWord = word.substr(0,i);
			tokenize(splitWord);
			break;
		}
	}
	if(hasHyphen) {
		splitWord = word.substr(i+1);
		tokenize(splitWord);
	}
	return hasHyphen;
}

// Add lemma of token in the lemma dictionary.
void addLemma(string word) {
	// Lemmatize the word.
	char *c = lemmatize->Lemmatize(word.c_str());
	string lemma(c);
	// Check if it is part of stopwords.
	if(stopWords.count(lemma))
		return;
	// Add in dictionary.
	if(lemmas[lemma] == 0)
		lemmas[lemma] = 1;
	else
		lemmas[lemma]++;
	fileLemmas.insert(lemma);
}

// Add stem in the stem dictionary.
void addStem(string word) {
	// Convert word from string to char*
	char* c = (char*)word.c_str();
	// Stem the word.
	char* st = porter_stem(c);
	if(!st)
		return;
	string stem(st);
	// Check if it is part of stopwords.
	if(stopWords.count(stem))
		return;
	// Add in dictionary.
	if(stems[stem] == 0)
		stems[stem] = 1;
	else
		stems[stem]++;
	fileStems.insert(stem);
}

// Return the number of words that have occurred only once in the dictionary.
int oneOccurence(map<string,int> m) {
	int oneOccurence = 0;
	for (map<string, int>::iterator it = m.begin(); it != m.end(); it++) {
		if(it->second == 1)
			oneOccurence++;
	}
	return oneOccurence;
}

// Prints the first 'n' words from the dictionary.
void getTop(vector<pair<string,int> > m,int n) {
	for(int i=0; i<n; i++)
		cout << " " << m[i].first << "\t: " << m[i].second << endl;
}

// Tokenize the word.
void tokenize(string &word) {
	// Skip empty words
	if (word.length() == 0)
		return;
	// Convert the word in lower case.
	transform(word.begin(), word.end(), word.begin(), ::tolower);
	// Right strip the word.
	rightStrip(word);
	// Skip empty words
	if (word.length() == 0)
		return;
	// Left strip the word.
	leftStrip(word);
	// Skip empty words
	if (word.length() == 0)
		return;
	// Strip the words from possessive.
	possessiveStrip(word);
	// Skip empty words
	if (word.length() == 0)
		return;
	// Check if word is an acronym and transform it.
	word=acronymCheck(word);
	// Skip empty words
	if (word.length() == 0)
		return;
	// Check if word contains hyphen. If it does than return since tokenize is called again internally.
	if(removeHyphen(word))
		return;

	// Check if token is part of stopwords.
	if(stopWords.count(word))
		return;

	// Add lemma of token in the dictionary.
	addLemma(word);
	// Increase the total word count.
	nWords++;
	// Get and stem in dictionary from the token.
	addStem(word);
}

// Parse the file.
int parseFile(const char file[]) {
	string word;
	bool textField = false;
	// Open file and check if it is opened correctly.
	ifstream fin(file);
	if (!fin) {
		cout<<file<<" not opened correctly\n";
		return -1;
	}
	// Loop until the file ends and get word.
	while (fin >> word) {
		// Skip the file until we are in the <TEXT> & </TEXT> tag.
		if(word == "<TEXT>") {
			textField = true;
			continue;
		}
		if(word == "</TEXT>")
			textField = false;
		if(!textField)
			continue;
		// If read word then tokenize it.
		if(word.length()>0)
			tokenize(word);
	}
	// Close file read.
	fin.close();
	return 0;
}

// Sort in decreasing order of value - second parameter in the pair.
bool cmp(const pair<string, int>  &p1, const pair<string, int> &p2) {
	return p1.second > p2.second;
}

// Sort the vectors.
void sortDictionaries() {
	copy(lemmas.begin(), lemmas.end(), back_inserter(sortedLemmas));
	sort(sortedLemmas.begin(), sortedLemmas.end(), cmp);

	copy(stems.begin(), stems.end(), back_inserter(sortedStems));
	sort(sortedStems.begin(), sortedStems.end(), cmp);
}

#ifdef linux
// Read the directory.
void readDir(char dirName[]) {
	DIR *pdir = NULL;
	struct dirent *pent = NULL;
	// Open directory and get list of all the files in it.
	pdir = opendir(dirName);
	if(!pdir)
		cout<<"Error reading directory";
	// Read the directory.
	while(pent=readdir(pdir)) {
		if(!pent)
			break;
		if(strcmp(pent->d_name,".")==0 || strcmp(pent->d_name,"..")==0)
			continue;
		files.push_back(pent->d_name);
	}
	// Close the directory.
	closedir(pdir);
	// Sort the files vector in increasing alphabetical order.
	sort(files.begin(),files.end());
}
#endif

#ifdef windows
// Read the directory.
void readDir(char dirName[]) {
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	char xx[] = "\\*.*";
	char* newDirName = new char[strlen(dirName)+strlen(xx)+1];
	strcpy(newDirName,dirName);
	strcat(newDirName,xx);
	TCHAR  *directory = newDirName;
	hFind = FindFirstFile(directory, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) 
	{
		cout<<"Error reading directory\n";
		return;
	} 
	else 
	{
		do
		{
			//ignore current and parent directories
			if(_tcscmp(FindFileData.cFileName, TEXT("."))==0 || _tcscmp(FindFileData.cFileName, TEXT(".."))==0)
				continue;
			files.push_back(FindFileData.cFileName);
		}
		while (FindNextFile(hFind, &FindFileData));
		FindClose(hFind);
	}
}
#endif

// Parse the vector of the files in the directory.
void parseDirectory(char dirName[]) {
	string file;
	string fileName(dirName);
	fileName += delimeter;
	for(int i=0; i<files.size(); i++) {
		file = fileName + files[i];
		const char *fn = file.c_str();
		fileLemmas.clear();
		fileStems.clear();
		parseFile(fn);
		distinctCountLemmas+=fileLemmas.size();
		distinctCountStems+=fileStems.size();
	}
}

// Print the results required.
void printResults() {
	cout << " Number of lemmas = " << nWords;
	cout << "\n Number of unique lemmas = " << lemmas.size();
	cout << "\n Number of lemmas that occur only once = " << oneOccurence(lemmas);
	cout << "\n Average number of distinct lemmas = " << distinctCountLemmas/(float)(files.size() - 2);
	cout << "\n Top 30 lemmas : \n";
	getTop(sortedLemmas,30);
	cout << "\n Number of distinct stems = " << stems.size();
	cout << "\n Number of stems that occur only once = " << oneOccurence(stems);
	cout << "\n Average number of distinct stems = " << distinctCountStems/(float)(files.size() - 2);
	cout << "\n Top 30 stems : \n";
	getTop(sortedStems,30);
}

// Read the stop-word list.
void readStopWords(char file[]) {
	string word;
	bool textField = false;
	// Open file and check if it is opened correctly.
	ifstream fin(file);
	if (!fin) {
		cout<<file<<" not opened correctly\n";
		return;
	}
	// Loop until the file ends and get word.
	while (fin >> word) {
		// Add to stop word list.
		stopWords.insert(word);
	}
	// Close file read.
	fin.close();
	return;
}


// Main function.
int main(int argc, char *argv[]) {

	// Timer initialization.
#ifdef linux
	struct timeval start, end;
	long mtime, seconds, useconds;    
	// Start the timer.
	gettimeofday(&start, NULL);
#endif

#ifdef windows
	DWORD dw1 = GetTickCount();
#endif

	// 	string ch="are";
	// 	char* c=(char*)ch.c_str();
	// 	char* s = porter_stem(c);
	// 	if(!s)
	// 		return 0;
	// 	else
	// 		string st(s);

	// Load the binary file for English character set to get the lemmatizer to work.
	lemmatize->LoadBinary("lem-m-en.bin");

	char *dirName = argv[1];

	readStopWords(argv[2]);

	readDir(dirName);

	// Parse the directory to tokenize.
	parseDirectory(dirName);

	// Sort the dictionaries into vectors.
	sortDictionaries();

	// Print the results.
	printResults();

	// End timer and calculate time.
#ifdef linux
	gettimeofday(&end, NULL);
	seconds  = end.tv_sec  - start.tv_sec;
	useconds = end.tv_usec - start.tv_usec;
	mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;
	cout<<"\n Total time taken : "<<mtime<<" milliSeconds"<<endl;
#endif
#ifdef windows
	DWORD dw2 = GetTickCount();
	cout<<"\n Total time taken : "<<(dw2-dw1)<<" milliSeconds"<<endl;
	_getch();
#endif

	return 0;
}
