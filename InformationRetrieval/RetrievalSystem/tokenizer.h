#pragma once
#include "global.h"
class tokenizer
{
public:
	string _word;
	int _len;
	bool _skipWord;

	tokenizer(string word):_word(word),_skipWord(false),_len(word.length())
	{
	}

	virtual ~tokenizer(void)
	{
	}

	bool isEmpty()
	{
		if(_len==0)
		{
			_skipWord = true;
			return true;
		}
		return false;
	}

	void toLower()
	{
		transform(_word.begin(), _word.end(), _word.begin(), ::tolower);
	}

	// Recursively remove the special characters at the right end from the word to be tokenized. E.g. "hello)/(" becomes "hello".
	void rightStrip() {
		char rchar = _word[_len-1];
		while(!isalnum(rchar)) {
			_word.erase(_len-1);
			_len = _word.length();
			if(isEmpty())
				return;
			rchar = _word[_len-1];
		}
	}

	// Recursively remove the special characters at the left end from the word to be tokenized. E.g. ")/(hello" becomes "hello".
	void leftStrip() {
		char lchar = _word[0];
		while(!isalnum(lchar)) {
			_word = _word.substr(1);
			_len = _word.length();
			if(isEmpty())
				return;
			lchar = _word[0];
		}
	}

	// Remove the possessives while tokenizing. E.g. "denny's" becomes "denny" & "dennys'" becomes "dennys"
	void possessiveStrip() {
		if(_len<2)
			return;
		char l1 = _word[_len-1];
		char l2 = _word[_len-2];
		if(l1=='s' && l2 == '\'')
		{
			_word.erase(_len-2,2);
			_len = _word.length();
		}
		else if(l1 == '\'')
		{
			_word.erase(_len-1);
			_len = _word.length();
		}
	}

	// Checks whether the word to be tokenized is an acronym and changes it. E.g. "u.s.a." becomes "usa".
	void acronymCheck() {
		string changedWord = "";
		if(_len<2)
			return;
		for(int i=0; i<_len; i++) {
			char ch = _word[i];
			if(i%2==0) {
				if(ch<97 || ch>122)
					return;
				else 
					changedWord += ch;
			}
			else {
				if(ch!='.')
					return;
			}
		}
		_word = changedWord;
		_len = _word.length();
	}

	// Check if there is a hyphen "-". If present, remove it and merge them ti make a single word. E.g. "middle-class" becomes one word "middleclass".
	void removeHyphen() {
		string changedWord = "";
		for(int i=0; i<_len; i++) {
			if(_word[i]=='-') {
				continue;
			}
			else
				changedWord += _word[i];
		}
		_word = changedWord;
		_len = _word.length();
	}

	bool isNonAlpha()
	{
		if(_word.find_first_not_of("abcdefghijklmnopqrstuvwxyz") != std::string::npos)
		{
			_skipWord = true;
			return true;
		}
		return false;
	}

	void tokenize()
	{
		if(isEmpty())
			return;
		toLower();
		// Right strip the word.
		rightStrip();
		// Skip empty words
		if (isEmpty())
			return;
		// Left strip the word.
		leftStrip();
		// Skip empty words
		if (isEmpty())
			return;
		// Strip the words from possessive.
		possessiveStrip();
		// Skip empty words
		if (isEmpty())
			return;
		// Check if word is an acronym and transform it.
		acronymCheck();
		// Skip empty words
		if (isEmpty())
			return;
		// Check if word contains hyphen. If it does than return since tokenize is called again internally.
		removeHyphen();
		if (isEmpty())
			return;
		// Return if the word contains non-alphabetic character.
		if(isNonAlpha())
			return;
	}
};

