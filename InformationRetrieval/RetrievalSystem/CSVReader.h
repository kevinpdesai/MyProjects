#pragma once
#include "global.h"

class CSVReader
{
public:
	string _filename;
	vector<vector <string> > _data;

	CSVReader(string filename) {
		_data.clear();
		ifstream ifs(filename.c_str());
		if (!ifs) {
			cout << "Error reading file!" << endl;
			return;
		}
		string line;
		while (getline(ifs, line)) {
			_data.push_back(splitString(line, ','));
		}
		ifs.close();
	}

	static vector<string> splitString(string original, char splitChar) {
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

	virtual ~CSVReader(void)
	{
	}
};

