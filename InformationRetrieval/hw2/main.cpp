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
#include <cmath>
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

// Uncompressed version index files.
ofstream version1_uncomp("Index_Version1.uncompress");
ofstream version2_uncomp("Index_Version2.uncompress");

// Compressed Version index files.
ofstream version1_comp("Index_Version1.compress");
ofstream version2_comp("Index_Version2.compress");

// Variable used to calculate the total number of lemmas/stems (Words).
long nFileWords = 0;

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

// Store info for each file - max_tf and doc length.
map<int, pair<int,int> > docInfoLemmas;
int maxDocLemma=1;

// Store info for each file - max_tf and doc length.
map<int, pair<int,int> > docInfoStems;
int maxDocStem=1;

// Map to store the LEMMAS
map<string, int> lemmasDict;
map<string, vector<pair<int, int> > > lemmas;

// Map to store the STEMS
map<string, int> stemsDict;
map<string, vector<pair<int, int> > > stems;

// Map to store pointers (addresses) for the index words.
map<string, int> blockCodingWordPointer;
map<string, int> frontCodingWordPointer;
map<string, unsigned char> frontCodingWordCountPointer;
map<string, int> lemmaUncompPointer;
map<string, int> stemUncompPointer;

// Initialize the Lemmatizer object.
RdrLemmatizer *lemmatize = new RdrLemmatizer();

// Function declaration.
void tokenize(string &, int);

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
bool removeHyphen(string &word, int doc_id) {
	string splitWord;
	bool hasHyphen = false;
	int i=0;
	for(i=0; i<word.length(); i++) {
		if(word[i]=='-') {
			hasHyphen = true;
			splitWord = word.substr(0,i);
			tokenize(splitWord, doc_id);
			break;
		}
	}
	if(hasHyphen) {
		splitWord = word.substr(i+1);
		tokenize(splitWord, doc_id);
	}
	return hasHyphen;
}

// Add lemma of token in the lemma dictionary.
void addLemma(string word, int doc_id) {
	// Lemmatize the word.
	char *c = lemmatize->Lemmatize(word.c_str());
	string lemma(c);
	if(lemma.length()<1)
		return;
	// Check if it is part of stopwords.
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
	// Check if it is part of stopwords.
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

// Return the number of words that have occurred only once in the dictionary.
int oneOccurence(map<string,vector<pair<int,int> > > m) {
	int oneOccurence = 0;
	for (map<string, vector<pair<int,int> > >::iterator it = m.begin(); it != m.end(); it++)
		if(it->second.size() == 1 && (it->second[0]).second == 1)
			oneOccurence++;
	return oneOccurence;
}

// Prints the first 'n' words from the dictionary.
void getTop(vector<pair<string,vector<pair<int, int> > > > m,int n) {
	for(int i=0; i<n; i++) {
		int t=0;
		cout << " " << m[i].first << "\t: " << m[i].second.size() << "\t: ";
		for(int j=0; j<m[i].second.size(); j++)
			t+=m[i].second.at(j).second;
			//cout << m[i].second.at(j).first << "-" << m[i].second.at(j).second << " , ";
		cout << t << endl;
	}
}

// Tokenize the word.
void tokenize(string &word, int doc_id) {
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
	if(removeHyphen(word, doc_id))
		return;

	// Return if the word contains non-alphabetic character.
	if(word.find_first_not_of("abcdefghijklmnopqrstuvwxyz") != std::string::npos)
		return;

	// Increase the number of words in file.
	nFileWords++;

	// Check if token is part of stopwords.
	if(stopWords.count(word))
		return;

	// Add lemma of token in the dictionary.
	addLemma(word, doc_id);
	
	// Get and stem in dictionary from the token.
	addStem(word, doc_id);
}

// Parse the file.
int parseFile(const char file[], int doc_id) {
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
			tokenize(word, doc_id);
	}
	// Close file read.
	fin.close();
	return 0;
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
		maxDocLemma = 1;
		maxDocStem = 1;
		nFileWords = 0;
		parseFile(fn,i+1);
		docInfoLemmas[i+1] = pair<int,int>(maxDocLemma,nFileWords);
		docInfoStems[i+1] = pair<int,int>(maxDocStem,nFileWords);
	}
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

// Block code the dictionary.
void blockCode(ofstream& f, map<string, vector<pair<int, int> > > m) {
	// Take each word.
	for(map<string, vector<pair<int, int> > >::iterator it = m.begin(); it != m.end(); it++) {
		// Get the current file pointer location and store it so that the word can be pointed to later while storing posting.
		int l = f.tellp();
		blockCodingWordPointer[it->first] = l;
		// Write the length of the word as unsigned char and the word itself in the file. No delimiter between the words. The length of the word can uniquely identify the word end.
		unsigned char c = it->first.length();
		f << c << it->first;
	}
	// At the end enter the \n character for convenience and readability.
	f << endl;
}

// Front code the dictionary.
void frontCode(ofstream& f, map<string, vector<pair<int, int> > > m) {
	string s(""), ss("");
	int fp=0;
	set <string> lcps;
	for (map<string, vector<pair<int, int> > >::iterator it = m.begin(); it != m.end(); it++) {
		ss = it->first;
		// Find out if the word currently as root and current word have at least the first 4 letters common. If so insert it into a set of strings to be front coded together.
		if (s.size() > 4 && ss.size() > 4 && s[0] == ss[0] && s[1] == ss[1] && s[2] == ss[2] && s[3] == ss[3])
				lcps.insert(ss);
		else {
			s = ss;
			// Skip string construction and compression if there are no words.
			if (lcps.size() == 0) {
				lcps.insert(s);
				continue;
			}
			string first = *(lcps.begin());
			char coded[2500];
			unsigned int cid = 1, ci = 0;
			bool incflag = true;

			// Get the maximum common string length in cid
			while (incflag) {
				cid++;
				for (set<string>::iterator iter = lcps.begin(); iter != lcps.end(); iter++) {
					if((*iter).length() <= cid) {
						incflag = false;
						break;
					}
					if((*iter)[cid] != first[cid])
						incflag = false;
				}
			}

			// Write to file the standard string starting with its length as unsigned char.
			coded[ci++] = (unsigned char)first.size();

			while (ci <= cid) {
				coded[ci] = first[ci-1];
				ci++;
			}

			// Store the pointer address to the first word or the root word in the common words.
			// Also, store the unsigned char '0' to identify that there is only one string and no common characters.
			frontCodingWordPointer[first] = fp;
			frontCodingWordCountPointer[first] = (unsigned char)0;
			// Append words atop first word if there are any
			if (lcps.size() != 1) {
				int x = 0;
				// Generate the compressed strings
				for (set<string>::iterator iter = lcps.begin(); iter != lcps.end(); iter++) {
					// Write '*' as the first character after the common letters.
					if (*iter == first)
						coded[ci++] = '*';
					// Write the length of the uncommon part of the word remaning to be written and also store another character '#' to distinguish that uncommon part start.
					else {
						coded[ci++] = (unsigned char)((*iter).size() - cid);
						coded[ci++] = '#';
					}
					// Store the pointer address of the words same as that of the root - common part pointer address.
					// Also, store the number 'x' in increasing order so as to identify that this word is will occur after 'x' occurences of the '#' or '* symbols.
					// This will help uniquely identify the words.
					x++;
					frontCodingWordPointer[*iter] = fp;
					frontCodingWordCountPointer[*iter] = (unsigned char)x;
					for (unsigned int i = cid; i < (*iter).size(); i++)
						coded[ci++] = (*iter)[i];
				}
			}

			coded[ci++] = '\0';
			f << coded;
			fp = f.tellp();
			lcps.clear();
			lcps.insert(s);
		}
	}
	// Check and do the same if the iteration through the dictionary has ended and there are still words present in the common set.
	if(lcps.size()!=0) {
		string first = *(lcps.begin());
		char coded[2500];
		unsigned int cid = 1, ci = 0;
		bool incflag = true;

		// Get the maximum common string length in cid
		while (incflag) {
			cid++;
			for (set<string>::iterator iter = lcps.begin(); iter != lcps.end(); iter++) {
				if((*iter).length() <= cid) {
					incflag = false;
					break;
				}
				if((*iter)[cid] != first[cid])
					incflag = false;
			}
		}

		// Write to file the standard string starting with its length as unsigned char.
		coded[ci++] = (unsigned char)first.size();

		while (ci <= cid) {
			coded[ci] = first[ci-1];
			ci++;
		}

		// Store the pointer address to the first word or the root word in the common words.
		// Also, store the unsigned char '0' to identify that there is only one string and no common characters.
		frontCodingWordPointer[first] = fp;
		frontCodingWordCountPointer[first] = (unsigned char)0;
		// Append words atop first word if there are any
		if (lcps.size() != 1) {
			int x = 0;
			// Generate the compressed strings
			for (set<string>::iterator iter = lcps.begin(); iter != lcps.end(); iter++) {
				// Write '*' as the first character after the common letters.
				if (*iter == first)
					coded[ci++] = '*';
				// Write the length of the uncommon part of the word remaning to be written and also store another character '#' to distinguish that uncommon part start.
				else {
					coded[ci++] = (unsigned char)((*iter).size() - cid);
					coded[ci++] = '#';
				}
				// Store the pointer address of the words same as that of the root - common part pointer address.
				// Also, store the number 'x' in increasing order so as to identify that this word is will occur after 'x' occurences of the '#' or '* symbols.
				// This will help uniquely identify the words.
				x++;
				frontCodingWordPointer[*iter] = fp;
				frontCodingWordCountPointer[*iter] = (unsigned char)x;
				for (unsigned int i = cid; i < (*iter).size(); i++)
					coded[ci++] = (*iter)[i];
			}
		}

		coded[ci++] = '\0';
		f << coded;
		fp = f.tellp();
		lcps.clear();
	}
}

// Get the binary string from the gamma/delta code.
string binary(long x)
{
	string s;
	do
	{
		s.push_back('0' + (x & 1));
	} while (x >>= 1);
	std::reverse(s.begin(), s.end());
	return s;
}

// Get unary code of the int.
string getUnaryCode(unsigned int x) {
	string ret = "";
	while(x>0) {
		ret += "1";
		x--;
	}
	ret += "0";
	return ret;
}

// Get gamma code of the gap.
string getGammaCode(unsigned int x) {
	return getUnaryCode(binary(x).length()-1) + binary(x).substr(1);
}

// Get delta code of the gap.
string getDeltaCode(unsigned int x) {
	return getGammaCode(binary(x).length()) + binary(x).substr(1);
}

// Gamma code the dictionary.
void gammaCode(ofstream& f, map<string, vector<pair<int, int> > > m) {
	// Select each word in the dictionary.
	int i=0;
	for(map<string, vector<pair<int, int> > >::iterator it = m.begin(); it != m.end(); it++,i++) {
		// Write the file address pointer to every 8th word for k=8 block coding.
		if(i%8==0)
			f << blockCodingWordPointer[it->first] << " ";
		// Write the size of the posting list.
		f << it->second.size() << " ";
		// Write the frequency of the word in each document after gamma encoding it.
		string gc = "";
		string a;
		int n=0;
		for(int j=0; j<it->second.size(); j++) {
			gc = gc + getGammaCode(it->second.at(j).second);
			// Write the code to the file byte by byte.
			while(gc.length()>=8) {
				a = gc.substr(0,8);
				n = 128*((a[0]=='1')?1:0) + 64*((a[1]=='1')?1:0) + 32*((a[2]=='1')?1:0) + 16*((a[3]=='1')?1:0) + 8*((a[4]=='1')?1:0) + 4*((a[5]=='1')?1:0) + 2*((a[6]=='1')?1:0) + ((a[7]=='1')?1:0);
				f << (unsigned char)n;
				gc = gc.substr(8);
			}
		}
		// Check if there is still code remaining. Append '0' at the end and write the bytes. The number of postings and the gamma code will determine the end of the file uniquely.
		if(gc!="") {
			n=0;
			for(int k=0; k<gc.length(); k++)
				n+=(128/((int)pow(2,k)))*((gc[k]=='1')?1:0);
			f << (unsigned char)n;
		}
		f << " ";
		gc="";
		n=0;
		// Get the first posting and get the gamma code for it.
		int cg, prev_cg = it->second.at(0).first;
		for(int j=0; j<it->second.size(); j++) {
			// Get the gap between the postings and get the gamma code for them.
			cg = it->second.at(j).first - prev_cg;
			prev_cg = it->second.at(j).first;
			gc = gc + getGammaCode(cg);
			// Write the code to the file byte by byte.
			while(gc.length()>=8) {
				a = gc.substr(0,8);
				n = 128*((a[0]=='1')?1:0) + 64*((a[1]=='1')?1:0) + 32*((a[2]=='1')?1:0) + 16*((a[3]=='1')?1:0) + 8*((a[4]=='1')?1:0) + 4*((a[5]=='1')?1:0) + 2*((a[6]=='1')?1:0) + ((a[7]=='1')?1:0);
				f << (unsigned char)n;
				gc = gc.substr(8);
			}
		}
		// Check if there is still code remaining. Append '0' at the end and write the bytes. The number of postings and the gamma code will determine the end of the file uniquely.
		if(gc!="") {
			n=0;
			for(int k=0; k<gc.length(); k++)
				n+=(128/((int)pow(2,k)))*((gc[k]=='1')?1:0);
			f << (unsigned char)n;
		}
	}
}

// Delta code the dictionary.
void deltaCode(ofstream& f, map<string, vector<pair<int, int> > > m) {
	// Select each word in the dictionary.
	for(map<string, vector<pair<int, int> > >::iterator it = m.begin(); it != m.end(); it++) {
		// Write the file address pointer to every word for front coding.
		f << frontCodingWordPointer[it->first] << " " << frontCodingWordCountPointer[it->first] << " ";
		// Write the size of the posting list.
		f << it->second.size() << " ";
		// Write the frequency of the word in each document after delta encoding it.
		string dc = "";
		string a;
		int n=0;
		for(int j=0; j<it->second.size(); j++) {
			dc = dc + getDeltaCode(it->second.at(j).second);
			// Write the code to the file byte by byte.
			while(dc.length()>=8) {
				a = dc.substr(0,8);
				n = 128*((a[0]=='1')?1:0) + 64*((a[1]=='1')?1:0) + 32*((a[2]=='1')?1:0) + 16*((a[3]=='1')?1:0) + 8*((a[4]=='1')?1:0) + 4*((a[5]=='1')?1:0) + 2*((a[6]=='1')?1:0) + ((a[7]=='1')?1:0);
				f << (unsigned char)n;
				dc = dc.substr(8);
			}
		}
		// Check if there is still code remaining. Append '0' at the end and write the bytes. The number of postings and the gamma code will determine the end of the file uniquely.
		if(dc!="") {
			n=0;
			for(int k=0; k<dc.length(); k++)
				n+=(128/((int)pow(2,k)))*((dc[k]=='1')?1:0);
			f << (unsigned char)n;
		}
		f << " ";
		dc="";
		n=0;
		// Get the first posting and get the delta code for it.
		int cg, prev_cg = it->second.at(0).first;
		for(int j=0; j<it->second.size(); j++) {
			// Get the gap between the postings and get the delta code for them.
			cg = it->second.at(j).first - prev_cg;
			prev_cg = it->second.at(j).first;
			dc = dc + getDeltaCode(cg);
			// Write the code to the file byte by byte.
			while(dc.length()>=8) {
				a = dc.substr(0,8);
				n = 128*((a[0]=='1')?1:0) + 64*((a[1]=='1')?1:0) + 32*((a[2]=='1')?1:0) + 16*((a[3]=='1')?1:0) + 8*((a[4]=='1')?1:0) + 4*((a[5]=='1')?1:0) + 2*((a[6]=='1')?1:0) + ((a[7]=='1')?1:0);
				f << (unsigned char)n;
				dc = dc.substr(8);
			}
		}
		// Check if there is still code remaining. Append '0' at the end and write the bytes. The number of postings and the delta code will determine the end of the file uniquely.
		if(dc!="") {
			n=0;
			for(int k=0; k<dc.length(); k++)
				n+=(128/((int)pow(2,k)))*((dc[k]=='1')?1:0);
			f << (unsigned char)n;
		}
	}
}

// Write index.
void writeIndex(ofstream& f, map<string, vector<pair<int, int> > > m, int version, bool comp = false) {
	// Write the compressed version.
	if(comp) {
		// Version-1 Block coding & Gamma coding.
		if(version==1) {
			blockCode(f,m);
			gammaCode(f,m);
		}
		// Version-2 Front coding & Delta coding.
		else {
			frontCode(f,m);
			deltaCode(f,m);
		}
	}
	// Write the uncompressed version.
	else {
		for(map<string, vector<pair<int, int> > >::iterator it = m.begin(); it != m.end(); it++) {
			// Get the current file pointer location and store it so that the word can be pointed to later while storing posting.
			int l = f.tellp();
			if(version==1)
				lemmaUncompPointer[it->first] = l;
			else
				stemUncompPointer[it->first] = l;
			// Write the length of the word as unsigned char and the word itself in the file. No delimiter between the words. The length of the word can uniquely identify the word end.
			int c = it->first.length();
			f << c << " " << it->first;
			f << endl;
		}
		for(map<string, vector<pair<int, int> > >::iterator it = m.begin(); it != m.end(); it++) {
			// Write the file address pointer to every word.
			if(version==1)
				f << lemmaUncompPointer[it->first];
			else
				f << stemUncompPointer[it->first] << " 0";
			// Write the size of the posting list.
			f << " " << it->second.size();
			// Write the frequency of the word in each document.
			for(int j=0; j<it->second.size(); j++)
				f << " " << it->second.at(j).second;
			// Write the posting document numbers.
			for(int j=0; j<it->second.size(); j++)
				f << " " << it->second.at(j).first;
			f << endl;
		}
	}
}

// Print the word info
void printWordInfo(string w, int version, bool comp = false) {
	if(version==1) {
		string a = lemmatize->Lemmatize(w.c_str());
		int tf=0;
		cout << "\n" << a << "\t";
		if(a.length()<8)
			cout << "\t";
		cout << lemmas[a].size();
		for(int i=0; i<lemmas[a].size(); i++)
			tf+=lemmas[a][i].second;
		cout << "\t" << tf << "\t";
		if(!comp)
			cout << lemmas[a].size()*8 << " B";
		else {
			double bt = 0;
			int prev_gap = 0;
			for(int i=0; i<lemmas[a].size(); i++) {
				bt += getGammaCode(lemmas[a][i].second).size() + getGammaCode(lemmas[a][i].first - prev_gap).size();
				prev_gap = lemmas[a][i].first;
			}
			cout << ceil(bt/8) << " B";
		}
	}
	else {
		string a = porter_stem((char*)w.c_str());
		int tf=0;
		cout << "\n" << a << "\t";
		if(a.length()<8)
			cout << "\t";
		cout << stems[a].size();
		for(int i=0; i<stems[a].size(); i++)
			tf+=stems[a][i].second;
		cout << "\t" << tf << "\t";
		if(!comp)
			cout << stems[a].size()*8 << " B";
		else {
			double bt = 0;
			int prev_gap = 0;
			for(int i=0; i<stems[a].size(); i++) {
				bt += getDeltaCode(stems[a][i].second).size() + getDeltaCode(stems[a][i].first - prev_gap).size();
				prev_gap = stems[a][i].first;
		}
			cout << ceil(bt/8) << " B";
		}
	}
}

// Print the results required.
void printResults(ofstream& f1, ofstream& f2, ofstream& f3, ofstream& f4) {
	cout << "\nSize of index Version 1 uncompressed (lemmas) = " << f1.tellp() << " B";
	cout << "\nSize of index Version 2 uncompressed (stems) = " << f2.tellp() << " B";
	cout << "\nSize of index Version 1 compressed (lemmas, block, gamma) = " << f3.tellp() << " B";
	cout << "\nSize of index Version 2 compressed (stems, front, delta) = " << f4.tellp() << " B";
	cout << "\n\nNumber of inverted lists in index Version 1 (lemmas) = " << lemmas.size();
	cout << "\nNumber of inverted lists in index Version 2 (stems) = " << stems.size();
	cout << "\n\nInverted list contains both the term frequency and the document id. \nVersion 1 uncompressed \nWord\t\tdf\ttf\tinverted list length";
	printWordInfo("reynolds",1);
	printWordInfo("nasa",1);
	printWordInfo("prandtl",1);
	printWordInfo("flow",1);
	printWordInfo("pressure",1);
	printWordInfo("boundary",1);
	printWordInfo("shock",1);
	cout << "\n\nVersion 2 uncompressed \nWord\t\tdf\ttf\tinverted list length";
	printWordInfo("reynolds",2);
	printWordInfo("nasa",2);
	printWordInfo("prandtl",2);
	printWordInfo("flow",2);
	printWordInfo("pressure",2);
	printWordInfo("boundary",2);
	printWordInfo("shock",2);
	cout << "\n\nVersion 1 compressed \nWord\t\tdf\ttf\tinverted list length";
	printWordInfo("reynolds",1,true);
	printWordInfo("nasa",1,true);
	printWordInfo("prandtl",1,true);
	printWordInfo("flow",1,true);
	printWordInfo("pressure",1,true);
	printWordInfo("boundary",1,true);
	printWordInfo("shock",1,true);
	cout << "\n\nVersion 2 compressed \nWord\t\tdf\ttf\tinverted list length";
	printWordInfo("reynolds",2,true);
	printWordInfo("nasa",2,true);
	printWordInfo("prandtl",2,true);
	printWordInfo("flow",2,true);
	printWordInfo("pressure",2,true);
	printWordInfo("boundary",2,true);
	printWordInfo("shock",2,true);
	cout << endl;
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

	// Load the binary file for English character set to get the lemmatizer to work.
	lemmatize->LoadBinary("lem-m-en.bin");

	// Get the directory name and read the directory. Also, get and read the stopwords list.
	char *dirName = argv[1];
	readStopWords(argv[2]);
	readDir(argv[1]);

	// Parse the directory to tokenize.
	parseDirectory(dirName);

	// Write the index files - compressed and uncompressed.
	writeIndex(version1_uncomp, lemmas, 1);
	writeIndex(version2_uncomp, stems, 2);
	writeIndex(version1_comp, lemmas, 1, true);
	writeIndex(version2_comp, stems, 2, true);
	
	// End timer and calculate time.
#ifdef linux
	gettimeofday(&end, NULL);
	seconds  = end.tv_sec  - start.tv_sec;
	useconds = end.tv_usec - start.tv_usec;
	mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;
	cout<<"\nTotal time taken to build and write all the 4 versions of index: "<<mtime<<" ms"<<endl;
	cout<<"\nTime taken to build and write 1 version of index: "<<mtime/4<<" ms"<<endl;
#endif
#ifdef windows
	DWORD dw2 = GetTickCount();
	cout<<"\nTotal time taken to build and write all 4 versions of index: "<<(dw2-dw1)<<" ms"<<endl;
	cout<<"\nTime taken to build and write 1 version of index: "<<(dw2-dw1)/4<<" ms"<<endl;
	_getch();
#endif

	// Print the results.
	printResults(version1_uncomp,version2_uncomp,version1_comp,version2_comp);

	// Close files.
	version1_uncomp.close();
	version2_uncomp.close();
	version1_comp.close();
	version2_comp.close();

#ifdef windows
	_getch();
#endif
	return 0;
}