#pragma once
#include "global.h"

class fileWriter
{
public:
	ofstream f;
	bool errorReadingFile;
	string _fileName;
	int _version;
	bool _compressed;
	map<string, vector<pair<int, int> > > _index;
	map<string, int> wordPointer;
	map<string, unsigned char> wordCountPointer;
	map<int, pair<int, int> > _docInfo;

	fileWriter(char file[], map<int, pair<int, int> > doc_info)
	{
		// Open file and check if it is opened correctly.
		f.open(file);
		if (!f) {
			errorReadingFile = true;
			cout<<file<<" not opened correctly\n";
			return;
		}
		_fileName = file;
		_docInfo = doc_info;
	}

	fileWriter(char file[], map<string, vector<pair<int, int> > > ind, int version = 1, bool compressed = false)
	{
		// Open file and check if it is opened correctly.
		f.open(file);
		if (!f) {
			errorReadingFile = true;
			cout<<file<<" not opened correctly\n";
			return;
		}
		_fileName = file;
		_version = version;
		_compressed = compressed;
		_index = ind;
	}

	virtual ~fileWriter(void)
	{
		f.close();
	}

	void writeDocInfoToFile()
	{
		for(map<int, pair<int, int> >::iterator it = _docInfo.begin(); it != _docInfo.end(); it++)
		{
			f << it->first << " " << it->second.first << " " << it->second.second << endl;
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
	void gammaCode() {
		// Select each word in the dictionary.
		int i=0;
		for(map<string, vector<pair<int, int> > >::iterator it = _index.begin(); it != _index.end(); it++,i++) {
			// Write the file address pointer to every 8th word for k=8 block coding.
			if(i%8==0)
				f << wordPointer[it->first] << " ";
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
	void deltaCode() {
		// Select each word in the dictionary.
		for(map<string, vector<pair<int, int> > >::iterator it = _index.begin(); it != _index.end(); it++) {
			// Write the file address pointer to every word for front coding.
			f << wordPointer[it->first] << " " << wordCountPointer[it->first] << " ";
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

	// Block code the dictionary.
	void blockCode() {
		// Take each word.
		for(map<string, vector<pair<int, int> > >::iterator it = _index.begin(); it != _index.end(); it++) {
			// Get the current file pointer location and store it so that the word can be pointed to later while storing posting.
			int l = f.tellp();
			wordPointer[it->first] = l;
			// Write the length of the word as unsigned char and the word itself in the file. No delimiter between the words. The length of the word can uniquely identify the word end.
			unsigned char c = it->first.length();
			f << c << it->first;
		}
		// At the end enter the \n character for convenience and readability.
		f << endl;
	}

	// Front code the dictionary.
	void frontCode() {
		string s(""), ss("");
		int fp=0;
		set <string> lcps;
		for (map<string, vector<pair<int, int> > >::iterator it = _index.begin(); it != _index.end(); it++) {
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
				wordPointer[first] = fp;
				wordCountPointer[first] = (unsigned char)0;
				// Append words atop first word if there are any
				if (lcps.size() != 1) {
					int x = 0;
					// Generate the compressed strings
					for (set<string>::iterator iter = lcps.begin(); iter != lcps.end(); iter++) {
						// Write '*' as the first character after the common letters.
						if (*iter == first)
							coded[ci++] = '*';
						// Write the length of the uncommon part of the word remaining to be written and also store another character '#' to distinguish that uncommon part start.
						else {
							coded[ci++] = (unsigned char)((*iter).size() - cid);
							coded[ci++] = '#';
						}
						// Store the pointer address of the words same as that of the root - common part pointer address.
						// Also, store the number 'x' in increasing order so as to identify that this word is will occur after 'x' occurrences of the '#' or '* symbols.
						// This will help uniquely identify the words.
						x++;
						wordPointer[*iter] = fp;
						wordCountPointer[*iter] = (unsigned char)x;
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
			wordPointer[first] = fp;
			wordCountPointer[first] = (unsigned char)0;
			// Append words atop first word if there are any
			if (lcps.size() != 1) {
				int x = 0;
				// Generate the compressed strings
				for (set<string>::iterator iter = lcps.begin(); iter != lcps.end(); iter++) {
					// Write '*' as the first character after the common letters.
					if (*iter == first)
						coded[ci++] = '*';
					// Write the length of the uncommon part of the word remaining to be written and also store another character '#' to distinguish that uncommon part start.
					else {
						coded[ci++] = (unsigned char)((*iter).size() - cid);
						coded[ci++] = '#';
					}
					// Store the pointer address of the words same as that of the root - common part pointer address.
					// Also, store the number 'x' in increasing order so as to identify that this word is will occur after 'x' occurrences of the '#' or '* symbols.
					// This will help uniquely identify the words.
					x++;
					wordPointer[*iter] = fp;
					wordCountPointer[*iter] = (unsigned char)x;
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

	void writeCompressed()
	{
		switch (_version)
		{
		// Version-1 Block coding & Gamma coding.
		case 1:	
			blockCode();
			gammaCode();
			break;
		// Version-2 Front coding & Delta coding.
		case 2:	
			frontCode();
			deltaCode();
			break;
		default:
			break;
		}
	}

	void writeUncompressed()
	{
		for(map<string, vector<pair<int, int> > >::iterator it = _index.begin(); it != _index.end(); it++) {
			// Get the current file pointer location and store it so that the word can be pointed to later while storing posting.
			int l = f.tellp();
			wordPointer[it->first] = l;
			// Write the length of the word as unsigned char and the word itself in the file. No delimiter between the words. The length of the word can uniquely identify the word end.
			int c = it->first.length();
			f << c << " " << it->first;
			f << endl;
		}
		for(map<string, vector<pair<int, int> > >::iterator it = _index.begin(); it != _index.end(); it++) {
			// Write the file address pointer to every word.
			f << wordPointer[it->first];
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

	void writeIndexToFile()
	{
		// Write the compressed version.
		if(_compressed)
			writeCompressed();
		// Write the uncompressed version.
		else
			writeUncompressed();
	}
};

