
/*
 * global.cpp
 * Author: davidwu
 */

#include <cstdio>
#include <cstdarg>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <cctype>
#include <string>
#include <vector>
#include "global.h"
using namespace std;

//ERRORS----------------------------------

void Global::fatalError(const char* s)

{
	cout << "\nFATAL ERROR:\n" << s << endl;
	exit(EXIT_FAILURE);
}

void Global::fatalError(const string& s)
{
	cout << "\nFATAL ERROR:\n" << s << endl;
	exit(EXIT_FAILURE);
}

//TIME------------------------------------

string Global::getTimeString()
{
	time_t rawtime;
	time(&rawtime);
	tm* ptm = gmtime(&rawtime);

	ostringstream out;
	out << (ptm->tm_year+1900) << "-"
			<< (ptm->tm_mon+1) << "-"
			<< (ptm->tm_mday);
	return out.str();
}

//STRINGS---------------------------------

string Global::charToString(char c)
{
	char buf[2];
	buf[0] = c;
	buf[1] = 0;
	return string(buf);
}

string Global::intToString(int x)
{
  stringstream ss;
  ss << x;
  return ss.str();
}

string Global::doubleToString(double x)
{
  stringstream ss;
  ss << x;
  return ss.str();
}

string Global::uint64ToHexString(uint64_t x)
{
	char buf[32];
	sprintf(buf,"%016llx",x); //Spurious g++ warning on this line?
	return string(buf);
}

bool Global::tryStringToInt(const string& str, int& x)
{
	int val = 0;
	istringstream in(trim(str));
	in >> val;
	if(in.fail() || in.peek() != EOF)
		return false;
	x = val;
	return true;
}

int Global::stringToInt(const string& str)
{
	int val = 0;
	istringstream in(trim(str));
	in >> val;
	if(in.fail() || in.peek() != EOF)
		fatalError(string("could not parse int: ") + str);
	return val;
}

bool Global::tryStringToUInt64(const string& str, uint64_t& x)
{
	uint64_t val = 0;
	istringstream in(trim(str));
	in >> val;
	if(in.fail() || in.peek() != EOF)
		return false;
	x = val;
	return true;
}

uint64_t Global::stringToUInt64(const string& str)
{
	uint64_t val = 0;
	istringstream in(trim(str));
	in >> val;
	if(in.fail() || in.peek() != EOF)
		fatalError(string("could not parse uint64: ") + str);
	return val;
}

bool Global::tryStringToDouble(const string& str, double& x)
{
	double val = 0;
	istringstream in(trim(str));
	in >> val;
	if(in.fail() || in.peek() != EOF)
		return false;
	x = val;
	return true;
}

double Global::stringToDouble(const string& str)
{
	double val = 0;
	istringstream in(trim(str));
	in >> val;
	if(in.fail() || in.peek() != EOF)
		fatalError(string("could not parse double: ") + str);
	return val;
}

string Global::trim(const string& s)
{
	size_t p2 = s.find_last_not_of(" \t\r\n");
	if (p2 == string::npos)
		return string();
	size_t p1 = s.find_first_not_of(" \t\r\n");
	if (p1 == string::npos)
		p1 = 0;

	return s.substr(p1,(p2-p1)+1);
}

string Global::toUpper(const string& s)
{
	string t = s;
	int len = t.length();
	for(int i = 0; i<len; i++)
		t[i] = toupper(t[i]);
	return t;
}

string Global::toLower(const string& s)
{
	string t = s;
	int len = t.length();
	for(int i = 0; i<len; i++)
		t[i] = tolower(t[i]);
	return t;
}

static string vformat (const char *fmt, va_list ap)
{
	// Allocate a buffer on the stack that's big enough for us almost
	// all the time.  Be prepared to allocate dynamically if it doesn't fit.
	size_t size = 4096;
	char stackbuf[size];
	std::vector<char> dynamicbuf;
	char *buf = &stackbuf[0];

	while(true)
	{
		// Try to vsnprintf into our buffer.
		int needed = vsnprintf(buf, size, fmt, ap);
		// NB. C99 (which modern Linux and OS X follow) says vsnprintf
		// failure returns the length it would have needed.  But older
		// glibc and current Windows return -1 for failure, i.e., not
		// telling us how much was needed.

		if(needed <= (int)size && needed >= 0)
			return std::string(buf, (size_t)needed);

		// vsnprintf reported that it wanted to write more characters
		// than we allotted.  So try again using a dynamic buffer.  This
		// doesn't happen very often if we chose our initial size well.
		size = (needed > 0) ? (needed+1) : (size*2);
		dynamicbuf.resize(size+1);
		buf = &dynamicbuf[0];
	}
}

string Global::strprintf(const char* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
	std::string buf = vformat (fmt, ap);
	va_end (ap);
	return buf;
}



//BITS-----------------------------------

uint32_t Global::highBits(uint64_t x)
{
	return (uint32_t)((x >> 32) & 0xFFFFFFFF);
}

uint32_t Global::lowBits(uint64_t x)
{
	return (uint32_t)(x & 0xFFFFFFFF);
}

uint64_t Global::combine(uint32_t hi, uint32_t lo)
{
	return ((uint64_t)hi << 32) | (uint64_t)lo;
}

//Robert Jenkins' 96 bit Mix Function
uint32_t Global::jenkinsMix(uint32_t a, uint32_t b, uint32_t c)
{
  a=a-b;  a=a-c;  a=a^(c >> 13);
  b=b-c;  b=b-a;  b=b^(a << 8);
  c=c-a;  c=c-b;  c=c^(b >> 13);
  a=a-b;  a=a-c;  a=a^(c >> 12);
  b=b-c;  b=b-a;  b=b^(a << 16);
  c=c-a;  c=c-b;  c=c^(b >> 5);
  a=a-b;  a=a-c;  a=a^(c >> 3);
  b=b-c;  b=b-a;  b=b^(a << 10);
  c=c-a;  c=c-b;  c=c^(b >> 15);
  return c;
}

uint64_t Global::getHash(const char* str)
{
	uint64_t m1 = 123456789;
	uint64_t m2 = 314159265;
	uint64_t m3 = 958473711;
	while(*str != '\0')
	{
		char c = *str;
		m1 = m1 * 31 + (uint64_t)c;
		m2 = m2 * 317 + (uint64_t)c;
		m3 = m3 * 1609 + (uint64_t)c;
		str++;
	}
	uint32_t lo = jenkinsMix(lowBits(m1),highBits(m2),lowBits(m3));
	uint32_t hi = jenkinsMix(highBits(m1),lowBits(m2),highBits(m3));
	return combine(hi,lo);
}

uint64_t Global::getHash(const int* input, int len)
{
  uint64_t m1 = 123456789;
  uint64_t m2 = 314159265;
  uint64_t m3 = 958473711;
  uint32_t m4 = 0xCAFEBABEU;
  for(int i = 0; i<len; i++)
  {
    int c = input[i];
    m1 = m1 * 31 + (uint64_t)c;
    m2 = m2 * 317 + (uint64_t)c;
    m3 = m3 * 1609 + (uint64_t)c;
    m4 += (uint32_t)c;
    m4 += (m4 << 10);
    m4 ^= (m4 >> 6);
  }
  m4 += (m4 << 3);
  m4 ^= (m4 >> 11);
  m4 += (m4 << 15);

  uint32_t lo = jenkinsMix(lowBits(m1),highBits(m2),lowBits(m3));
  uint32_t hi = jenkinsMix(highBits(m1),lowBits(m2),highBits(m3));
  return combine(hi ^ m4, lo + m4);
}

//IO-------------------------------------

//Read entire file whole
string Global::readFile(const char* filename)
{
	ifstream ifs(filename);
	if(!ifs.good())
		Global::fatalError(string("File not found: ") + filename);
	string str((istreambuf_iterator<char>(ifs)), istreambuf_iterator<char>());
	return str;
}

string Global::readFile(const string& filename)
{
	return readFile(filename.c_str());
}

//Read file into separate lines, using the specified delimiter character(s).
//The delimiter characters are NOT included.
vector<string> Global::readFileLines(const char* filename, char delimiter)
{
	ifstream ifs(filename);
	if(!ifs.good())
		Global::fatalError(string("File not found: ") + filename);

	vector<string> vec;
	string line;
	while(getline(ifs,line,delimiter))
		vec.push_back(line);
	return vec;
}

vector<string> Global::readFileLines(const string& filename, char delimiter)
{
	return readFileLines(filename.c_str(), delimiter);
}

//USER IO----------------------------

void Global::pauseForKey()
{
  cout << "Press any key to continue..." << endl;
  cin.get();
}








