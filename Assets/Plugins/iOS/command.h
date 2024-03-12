
/*
 * command.h
 * Author: davidwu
 */

#ifndef COMMAND_H
#define COMMAND_H

#include <map>
#include <string>
#include <vector>

using namespace std;

namespace Command
{
	//Return a vector of the args that were part of the main command (not flags)
	//including the zeroth arg (the program name)
	vector<string> parseCommand(int argc, const char* const *argv);

	//Return a map of flags -> values. For example "-b abc" becomes "b" -> "abc"
	map<string,string> parseFlags(int argc, const char* const *argv);

	//Same, but specify in space-separated string list
	//what flags are required, allowed, and what flags must have
	//nonempty arguments
	map<string,string> parseFlags(int argc, const char* const *argv,
			string required, string allowed, string empty, string nonempty);
	map<string,string> parseFlags(int argc, const char* const *argv,
			const char* required, const char* allowed, const char* empty, const char* nonempty);
}




#endif
