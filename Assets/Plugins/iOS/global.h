
/*
 * global.h
 * Author: davidwu
 *
 * Various generic useful things used throughout the program.
 */

#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <map>
#include <vector>
#include <string>
#include <stdint.h>
#include <cassert>
using namespace std;

//GLOBAL DEFINES AND FLAGS----------------------------------------------------
#define ARIMAADEBUG(x) x
//#define ARIMAADEBUG(x) ((void)0);

#define DEBUGASSERT(x) ARIMAADEBUG(assert(x);)

//See searchthread.h: BOOST_USE_WINDOWS_H

//Perform a board check consistency upon entry into captree
//(such as to detect whether we are in the middle of a tempstep by the caller code)
//#define CHECK_CAPTREE_CONSISTENCY

//Perform checks to make sure feature groups are not getting indices out of bounds
//#define FEATURE_GROUP_INDEX_CHECK


//GLOBAL FUNCTIONS------------------------------------------------------------
namespace Global
{
	//ERRORS----------------------------------

	//Report fatal error message and exit
	void fatalError(const char* s);
	void fatalError(const string& s);

	//TIME------------------------------------

	//Get string describing the current date, suitable for filenames
	string getTimeString();

	//STRINGS---------------------------------

	//To string conversions
	string charToString(char c);
	string intToString(int x);
	string doubleToString(double x);
	string uint64ToHexString(uint64_t x);

	//String to conversions
	int stringToInt(const string& str);
	uint64_t stringToUInt64(const string& str);
	double stringToDouble(const string& str);

	bool tryStringToInt(const string& str, int& x);
	bool tryStringToUInt64(const string& str, uint64_t& x);
	bool tryStringToDouble(const string& str, double& x);

	//Trim whitespace off both ends of string
	string trim(const string& s);

	//Convert to upper or lower case
	string toUpper(const string& s);
	string toLower(const string& s);

	string strprintf(const char* fmt, ...);

	//BITS-----------------------------------

	//Splitting of uint64s
	uint32_t highBits(uint64_t x);
	uint32_t lowBits(uint64_t x);
	uint64_t combine(uint32_t hi, uint32_t lo);

	//Hashing functions
	uint32_t jenkinsMix(uint32_t a, uint32_t b, uint32_t c);
	uint64_t getHash(const char* str);
	uint64_t getHash(const int* input, int len);


	//IO-------------------------------------

	//Read entire file whole
	string readFile(const char* filename);
	string readFile(const string& filename);

	//Read file into separate lines, using the specified delimiter character(s).
	//The delimiter characters are NOT included.
	vector<string> readFileLines(const char* filename, char delimiter = ' ');
	vector<string> readFileLines(const string& filename, char delimiter = ' ');

	//USER IO----------------------------

	//Display a message and ask the user to press a key to continue
	void pauseForKey();
}

//Named pairs and triples of data values
#define STRUCT_NAMED_PAIR(A,B,C,D,E) struct E {A B; C D; inline E() {B = A(); D = C();} inline E(A s_n_p_arg_0, C s_n_p_arg_1) {B = s_n_p_arg_0; D = s_n_p_arg_1;}}
#define STRUCT_NAMED_TRIPLE(A,B,C,D,E,F,G) struct G {A B; C D; E F; inline G() {B = A(); D = C(); F = E();} inline G(A s_n_p_arg_0, C s_n_p_arg_1, E s_n_p_arg_2) {B = s_n_p_arg_0; D = s_n_p_arg_1; F = s_n_p_arg_2;}}
#define STRUCT_NAMED_QUAD(A,B,C,D,E,F,G,H,I) struct I {A B; C D; E F; G H; inline I() {B = A(); D = C(); F = E(); H = G();} inline I(A s_n_p_arg_0, C s_n_p_arg_1, E s_n_p_arg_2, G s_n_p_arg_3) {B = s_n_p_arg_0; D = s_n_p_arg_1; F = s_n_p_arg_2; H = s_n_p_arg_3;}}

//SHORTCUTS FOR std::map------------------------------------------------

template<typename A, typename B>
bool map_contains(const map<A,B>& m, const A& key)
{
	return m.find(key) != m.end();
}

template<typename B>
bool map_contains(const map<string,B>& m, const char* key)
{
	return m.find(string(key)) != m.end();
}

template<typename A, typename B>
B map_get(const map<A,B>& m, const A& key)
{
	typename map<A,B>::const_iterator it = m.find(key);
	if(it == m.end())
		Global::fatalError("map_get: key not found");
	return it->second;
}

template<typename B>
B map_get(const map<string,B>& m, const char* key)
{
	typename map<string,B>::const_iterator it = m.find(string(key));
	if(it == m.end())
		Global::fatalError(string("map_get: key \"") + string(key) + string("\" not found"));
	return it->second;
}

template<typename A, typename B>
B map_try_get(const map<A,B>& m, const A& key)
{
	typename map<A,B>::const_iterator it = m.find(key);
	if(it == m.end())
		return B();
	return it->second;
}

template<typename B>
B map_try_get(const map<string,B>& m, const char* key)
{
	typename map<string,B>::const_iterator it = m.find(string(key));
	if(it == m.end())
		return B();
	return it->second;
}




#endif
