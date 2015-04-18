#pragma once
#include "global.h"

class fileReader
{

public:
	ifstream fin;
	bool _errorReadingFile;
	string word;
	string _fileName;
	bool _textField;

	fileReader(char file[]):_fileName(file),_textField(false),_errorReadingFile(false)
	{
		// Open file and check if it is opened correctly.
		fin.open(file);
		if (!fin) {
			_errorReadingFile = true;
			cout<<file<<" not opened correctly\n";
			return;
		}
	}

	~fileReader(void)
	{
		// Close file read.
		fin.close();
	}

	bool getNextWord()
	{
		// Get the next word in the file.
		if(fin >> word) {
			if(checkWord("<TEXT>"))
				_textField = true;
			return true;
		}
		return false;
	}

	bool checkWord(string checkString)
	{
		if(word==checkString)
			return true;
		return false;
	}

#ifdef windows
	bool checkWord(regex reg1)
	{
		if(regex_search(word, reg1))
			return true;
		return false;
	}
#endif // windows

#ifdef linux
	bool checkWord(regex_t r)
	{
		regmatch_t match[5];
		if(regexec(&r,(const char*)word.c_str(), 5, match, 0)==0)
			return true;
		return false;
	}
#endif // linux

};

	


