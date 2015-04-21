#pragma once
#include "global.h"

class indexFileReader
{
public:
	map<string, vector<pair<int, int> > > _index;
	map<string, vector< postingEntry > > _projIndex;
	map<int, string> _wordAtLocation;
	map<int, pair<int, int> > _docInfo;
	bool _compressed;
	string _fileName;
	ifstream fin;
	bool _errorReadingFile;
	bool _isPostingEntry;
	int _avgDocLen;

	indexFileReader(string file, bool posting = false, bool compressed = false): _fileName(file),_compressed(compressed),_isPostingEntry(posting)
	{
		// Open file and check if it is opened correctly.
		fin.open((const char*)_fileName.c_str());
		if (!fin) {
			_errorReadingFile = true;
			cout<<_fileName<<" not opened correctly\n";
			return;
		}
	}

	virtual ~indexFileReader(void)
	{
		// Close file read.
		fin.close();
	}

	void readIndexCompressed()
	{

	}

	void readIndexUncompressed()
	{
		string word;
		int l;
		int loc = fin.tellg();
		while(fin >> l)
		{
			fin >> word;
			if(isdigit(word[0]))
				break;
			_wordAtLocation[loc] = word;
			loc = (int)fin.tellg()+2;
		}
		loc = l;
		int n = atoi(word.c_str());
		int freq, doc_id=1;
		do 
		{
			if(n==-1)
				fin >> n;
			for(int i=0; i<n; i++)
			{
				fin >> freq;
				_index[_wordAtLocation[loc]].push_back(pair<int,int>(doc_id,freq));
			}
			for(int i=0; i<n; i++)
			{
				fin >> doc_id;
				_index[_wordAtLocation[loc]][i].first = doc_id;
			}
			n=-1;
		} while (fin >> loc);
	}

	void readProjectIndex()
	{
		string word;
		while (fin >> word)
		{
			uint n;
			fin >> n;
			for (uint i = 0; i < n; i++)
			{
				uint docId, tf;
				fin >> docId >> tf;
				_projIndex[word].push_back(postingEntry(tf, docId));
			}
		}
	}

	void readIndex()
	{
		if(_isPostingEntry)
			readProjectIndex();
		else if(_compressed)
			readIndexCompressed();
		else
			readIndexUncompressed();
	}

	void readDocInfo()
	{
		int doc_id, maxFreq, docLen;
		_avgDocLen = 0;
		while(fin >> doc_id)
		{
			fin >> maxFreq >> docLen;
			_docInfo[doc_id] = pair<int,int>(maxFreq,docLen);
			_avgDocLen+=docLen;
		}
		_avgDocLen/=_docInfo.size();
	}
};

