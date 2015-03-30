/* A Dynamic Programming based C program to find maximum number of A's
   that can be printed using four keys */
#include <stdio.h>
#include <conio.h>
#include <string>
#include <iostream>
 
using namespace std;

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

string getDetlaCode(unsigned int x) {
	return getGammaCode(binary(x).length()) + binary(x).substr(1);
}
 
// Driver program
int main()
{
	int n = 10;
	cout << getGammaCode(n);
	_getch();
}