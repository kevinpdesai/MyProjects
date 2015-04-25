#pragma once
#include "global.h"

class dirReader
{
public:
	string _dirFullName;
	bool _errorReadingDir;
	string nextFileName;

#ifdef windows
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
#endif // windows

#ifdef linux
	DIR *pdir;
	struct dirent *pent;
#endif // linux

	dirReader(const char dirName[])
	{
#ifdef windows
		char xx[] = "\\*.*";
		char* newDirName = new char[strlen(dirName)+strlen(xx)+1];
		strcpy(newDirName,dirName);
		strcat(newDirName,xx);
		_dirFullName = newDirName;
		TCHAR  *directory = newDirName;
		hFind = FindFirstFile(directory, &FindFileData);
		if (hFind == INVALID_HANDLE_VALUE) 
		{
			cout<<"Error reading directory\n";
			_errorReadingDir = true;
			return;
		} 
		_errorReadingDir = false;
		_dirFullName = newDirName;

#endif // windows

#ifdef linux
		// Open directory and get list of all the files in it.
		pdir = opendir(dirName);
		if(!pdir)
		{
			cout<<"Error reading directory";
			_errorReadingDir = true;
			return;
		}
		_errorReadingDir = false;
		_dirFullName = dirName;
#endif // linux
	}

	~dirReader(void)
	{
#ifdef windows
		FindClose(hFind);
#endif // windows

#ifdef linux
		closedir(pdir);
#endif // linux
	}

	bool getNexFileName()
	{
#ifdef windows
		if(FindNextFile(hFind, &FindFileData))
		{
			nextFileName = FindFileData.cFileName;
			return true;
		}
		return false;
#endif // windows

#ifdef linux
		if(pent=readdir(pdir)) {
			nextFileName = pent->d_name;
			return true;
		}
		return false;
#endif // linux		
	}
};

