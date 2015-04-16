#pragma once
#include "global.h"

class fileReader
{

public:
	ifstream fin;
	bool errorReadingFile;
	string word;
	string fileName;
	bool textField;

	fileReader(char file[])
	{
		// Open file and check if it is opened correctly.
		fin.open(file);
		if (!fin) {
			errorReadingFile = true;
			cout<<file<<" not opened correctly\n";
			return;
		}
		fileName = file;
		textField = false;
		errorReadingFile = false;
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
				textField = true;
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

	


