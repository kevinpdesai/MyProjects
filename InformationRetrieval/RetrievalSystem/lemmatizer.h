/******************************************************************************
This file is part of the lemmagen library. It gives support for lemmatization.
Copyright (C) 2006-2007 Matjaz Jursic <matjaz@gmail.com>

The lemmagen library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
******************************************************************************/

#pragma once

//includes
#include "global.h"
#include <iomanip>
#include <sstream>

//defines
#define lemmData
#define DATA_LEN 8
#define DATA_TBL {0x0000000000000000}

//typedefs
typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int dword;
typedef unsigned long long qword;

//const variables that algorithm depends on
#define AddrLen 4
#define FlagLen 1
#define ModLen 1
#define LenSpecLen 1
#define CharLen 1
#define DataStart 0

#define RoundDataLen 8

#define BitDefault	0x00
#define BitAddChar	0x01
#define BitInternal	0x02
#define BitEntireWr	0x04

#define TypeRule		(BitDefault)
#define TypeRuleEw		(BitDefault | BitEntireWr)
#define TypeLeafAC		(BitDefault | BitAddChar)
#define TypeLeafACEw	(BitDefault | BitAddChar | BitEntireWr)
#define TypeIntr		(BitDefault | BitInternal)
#define TypeIntrAC		(BitDefault | BitAddChar | BitInternal)

//main data structure and it's length 
static dword iDataLenStatic = DATA_LEN;
static qword abDataStatic[] = DATA_TBL;

//helper macros for nicer code and faster execution
#if AddrLen == 3
#define GETDWORD(type, wVar, wAddr) \
	type wVar = *((dword*) &abData[wAddr]) & 0x00FFFFF
#else
#define GETDWORD(type, wVar, wAddr) \
	type wVar = *((dword *) &abData[wAddr]) 
#endif

#define GETBYTEMOVE(type, bByte, iSize) \
	type bByte = abData[iAddr]; \
	iAddr += iSize

#define GETDWORDMOVE(type, wVar, iSize) \
	GETDWORD(type, wVar, iAddr);  \
	iAddr += iSize

#define GETSTRINGMOVE(type, acString, iSize) \
	type acString = new char[iSize+1]; \
	strncpy(acString, (char*) &abData[iAddr], iSize); \
	acString[iSize] = NULL; \
	iAddr += iSize

class lemmatizer{
public:
	byte *abData;
	int iDataLen;

public:
	lemmatizer(byte *abData, int iDataLen) {
		this->abData = abData;
		this->iDataLen = iDataLen;
	}
	lemmatizer(const char *acFileName) {
		LoadBinary(acFileName);
	}
	lemmatizer() {
		this->abData = (byte*) abDataStatic;
		this->iDataLen = iDataLenStatic;
	}
	~lemmatizer() {
		if (this->abData != (byte*) abDataStatic)
			delete[] abData;
	}

	int SizeOfTree() const{
		return iDataLen - DataStart;
	}

	char *Lemmatize(const char *acWord, char *acOutBuffer = NULL) const{
		byte bWordLen = strlen(acWord);

		dword iAddr = DataStart;
		dword iParentAddr = DataStart;
		dword iTmpAddr;
		char bLookChar = bWordLen;
		byte bType = abData[iAddr];

		while(true) {
			iTmpAddr = iAddr+FlagLen+AddrLen;

			//check if additional characters match
			if ((bType & BitAddChar) == BitAddChar) {
				byte bNewSufxLen = abData[iTmpAddr];
				iTmpAddr += LenSpecLen;

				bLookChar -= bNewSufxLen;

				//test additional chars if ok
				if (bLookChar>=0)
					do bNewSufxLen--;
				while (bNewSufxLen!=255 && abData[iTmpAddr+bNewSufxLen] == (byte) acWord[bLookChar+bNewSufxLen]);

				//wrong node, take parents rule
				if (bNewSufxLen!=255) {	iAddr = iParentAddr; break; } 

				//right node, but we are at the end (there will be no new loop) finish by returning this rule
				if ((bType & ~BitEntireWr) == TypeLeafAC) break;

				//right node and we need to go on with sub nodes (it si probably type TypeIntrAC )
				//set iTmpAddr to start of hashtable
				iTmpAddr += abData[iTmpAddr-LenSpecLen];
			} 

			//move lookup char back
			bLookChar--;
			//check if we are still inside the word (bLookChar==0 when at the begining of word)
			if (bLookChar<0) {
				//this means that we are just one character in front of the word so we must look for entire word entries
				if((bType & BitInternal) == BitInternal) {
					//go to the hashtable position 0(NULL) and look idf address is not NULL
					iTmpAddr += ModLen;
					byte bChar = abData[iTmpAddr];
					GETDWORD(,iTmpAddr,iTmpAddr+CharLen);
					if (bChar == NULL && iTmpAddr!=NULL) {
						//we have a candidate for entire word, redirect addresses
						iParentAddr = iAddr;
						iAddr = iTmpAddr;
						bType = abData[iAddr];
						//increase look char (because we actually eat one character)
						bLookChar++;
					}
				}
				break;
			}

			//find best node in hash table
			if((bType & BitInternal) == BitInternal) {
				byte bMod = abData[iTmpAddr];
				byte bChar = acWord[bLookChar];

				iTmpAddr += ModLen + (bChar%bMod)*(AddrLen+CharLen); 

				iTmpAddr = abData[iTmpAddr] == bChar ? iTmpAddr + CharLen : iAddr + FlagLen;

				iParentAddr = iAddr;
				GETDWORD(,iAddr, iTmpAddr);
				bType = abData[iAddr];

				if ((bType & ~BitEntireWr) == TypeRule) break;
			}
		}
		//if this is entire-word node, and we are not at the beginning of word it's wrong node - take parents
		if((bType & BitEntireWr) == BitEntireWr && bLookChar!=0) {
			iAddr = iParentAddr;
			bType = abData[iAddr];
		}

		//search ended before we came to the node of type rule but current node is OK so find it's rule node
		if((bType & ~BitEntireWr) != TypeRule)  GETDWORD( ,iAddr, iAddr+FlagLen);

		//we have (100%) node of type rule for lemmatization - now it's straight forward to lemmatize
		//read out rule
		iTmpAddr = iAddr + FlagLen;
		byte iFromLen = abData[iTmpAddr];
		iTmpAddr += LenSpecLen;
		byte iToLen = abData[iTmpAddr];
		iTmpAddr += LenSpecLen;

		//prepare output buffer
		byte iStemLen = bWordLen - iFromLen;
		char *acReturn = acOutBuffer == NULL ? new char[iStemLen + iToLen + 1] : acOutBuffer;

		//do actual lemmatization using given rule
		memcpy(acReturn, acWord, iStemLen);
		memcpy(&acReturn[iStemLen], &abData[iTmpAddr], iToLen);
		acReturn[iStemLen + iToLen] = NULL;

		return acReturn;
	}

	void ToStringHex(ostream &os) const {
		os << dec << noshowbase;
		os << "#define RrdLemmData" << endl;
		os << "#define DATA_LEN " << iDataLen << endl;

		os << hex << right << setfill('0');
		os << "#define DATA_TBL {";

		int iLen = iDataLen/8;
		for(int i=0; i<iLen; i++) {
			if (i%5!=0) os << " ";
			else os << " \\" << endl << "\t";

			os << "0x" << setw(16) << ((qword*)abData)[i];

			if (i != iLen-1) os << ",";
		}

		os << " \\" << endl << "\t}" << endl;

		os.flush();
	}

	void ToString(ostream &os = cout, dword iStartAddr = DataStart, int iDepth = 0, 
		char *acParSufx = "", char *acParDev = "", char cNewChar=NULL) const {
			int iAddr = iStartAddr;
			dword *iSubs = NULL;
			byte *bSubs = NULL;
			int iSubsNum = 0;
			char *acSufx = NULL;
			char *acSufxDev = NULL;

			//node type
			GETBYTEMOVE(int, iType, FlagLen);

			char *acTypeName;
			switch (iType) {
			case TypeRule:     acTypeName="RULE"; break;
			case TypeRuleEw:   acTypeName="RULE(entireword)"; break;
			case TypeLeafAC:   acTypeName="LEAF"; break;
			case TypeLeafACEw: acTypeName="LEAF(entireword)"; break;
			case TypeIntr:     acTypeName="INTER-SHORT"; break;
			case TypeIntrAC:   acTypeName="INTER-LONG"; break;
			}

			os << setfill('\t') << setw(iDepth) << "" << "" << acTypeName << ":[Addr:" << iStartAddr << "]";

			//we have rule to display
			if((iType & ~BitEntireWr)==TypeRule) {
				GETBYTEMOVE(int, iFromLen, LenSpecLen);
				GETBYTEMOVE(int, iToLen, LenSpecLen);
				GETSTRINGMOVE(char*, acTo, iToLen);
				os << "[From:" << iFromLen << "][To:" << iToLen << ",\"" << acTo << "\"]";
			} 

			//we have node to display
			else {
				//eat up rule address
				GETDWORDMOVE(dword, iRuleAddress, AddrLen);

				//eat up characters
				int iNewSufxLen = 0;
				char* acNewSuffix;
				if ((iType & BitAddChar) == BitAddChar) {
					GETBYTEMOVE(, iNewSufxLen, LenSpecLen);
					GETSTRINGMOVE(, acNewSuffix, iNewSufxLen);
				} 

				//create and display suffixes 
				if (cNewChar != NULL) {
					int iSufxLen = 1 + iNewSufxLen + strlen(acParSufx) + 1;
					int iSufxDevLen = 2 + iNewSufxLen + strlen(acParDev) + 1;

					acSufx = new char[iSufxLen];
					acSufxDev = new char[iSufxDevLen];

					acSufxDev[0]='|';

					strncpy(&acSufx[0],acNewSuffix,iNewSufxLen);
					strncpy(&acSufxDev[1],acNewSuffix,iNewSufxLen);

					acSufx[0 + iNewSufxLen] = cNewChar;
					acSufxDev[1 + iNewSufxLen] = cNewChar;

					strcpy(&acSufx[1 + iNewSufxLen],acParSufx);
					strcpy(&acSufxDev[2 + iNewSufxLen],acParDev);

				} else {
					acSufx = "";
					acSufxDev = "|";
				}
				os << "[Suffix:" << acSufxDev << ",\"" << acSufx << "\"]";

				//display rule that applies to this node
				os << " ";
				ToString(os, iRuleAddress, 0, acSufx);

				//display hash table
				if ((iType & BitInternal) == BitInternal) {

					GETBYTEMOVE(, iSubsNum, ModLen);
					iSubs = new dword[iSubsNum];
					bSubs = new byte[iSubsNum];

					ostringstream ossLine1;
					ostringstream ossLine2;
					ostringstream ossLine3;

					int iEmptyEntryNum = 0;
					for (int i = 0; i< iSubsNum; i++) {
						GETBYTEMOVE(int, cSubChar, CharLen);
						GETDWORDMOVE(dword, iSubAddres, AddrLen);
						bSubs[i]=cSubChar;
						iSubs[i]=iSubAddres;

						if (iSubAddres==0) {
							ossLine1 << " | NULL";
							ossLine2 << right << " |" << setw(5) << (int) i;
							ossLine3 << " | NULL";
							iEmptyEntryNum++;

						} else {
							ossLine1 << right << " |" << setw(3) << (char) cSubChar << "="  << setw(3) << (int) cSubChar;
							ossLine2 << right << " |" << setw(7) << (int) i;
							ossLine3 << right << " |" << setw(7) << iSubAddres;
						}
					}

					os << " HASHTABLE:";
					os << "[Size/Divider:" << iSubsNum << "]";
					os << "[Entries:" << iSubsNum - iEmptyEntryNum << "]";
					os << "[Unused:" << setprecision(4) << (float) 100 *iEmptyEntryNum/iSubsNum << "%]";
					os << " ";

					os << endl << setfill('\t') << setw(iDepth+1) << "" << "." << setfill('-') << setw(ossLine3.str().length() + 8) << ".";
					os << endl << setfill('\t') << setw(iDepth+1) << "" << "|  Pos:" << ossLine2.str() << " |";
					os << endl << setfill('\t') << setw(iDepth+1) << "" << "| Char:" << ossLine1.str() << " |";
					os << endl << setfill('\t') << setw(iDepth+1) << "" << "| Addr:" << ossLine3.str() << " |";
					os << endl << setfill('\t') << setw(iDepth+1) << "" << "'" << setfill('-') << setw(ossLine3.str().length() + 8) << "'";
					os << endl;
				}
			}

			//display sub nodes
			for (int i = 0; i<iSubsNum; i++)
				if (iSubs[i]!=NULL) {
					ToString(os, iSubs[i], iDepth + 1, acSufx, acSufxDev, bSubs[i]);
					if (i<iSubsNum-1) os << endl;
				}

				os.flush();
	}

	void SaveBinary(ostream &os) const {
		os.write((char*) &iDataLen, 4);
		os.write((char*) abData, iDataLen);
		os.flush();
	}
	void LoadBinary(istream &is) {
		iDataLen =0;
		is.read((char*) &iDataLen, 4);
		abData = new byte[iDataLen];
		is.read((char*) abData, iDataLen);
	}
	void SaveBinary(const char *acFileName) const {
		ofstream ofs(acFileName, ios_base::out | ios_base::binary);
		SaveBinary(ofs);
		ofs.close();
	}
	void LoadBinary(const char *acFileName) {
		ifstream ifs(acFileName, ios_base::in | ios_base::binary);
		LoadBinary(ifs);
		ifs.close();
	}



};