
/*
 * command.cpp
 * Author: davidwu
 */
#include "pch.h"

#include <iostream>
#include <sstream>
#include <map>
#include <string>
#include <vector>
#include "global.h"
#include "command.h"

using namespace std;

vector<string> Command::parseCommand(int argc, const char* const *argv)
{
	vector<string> vec;
	bool lastWasFlag = false;
	for(int i = 0; i<argc; i++)
	{
		const char* arg = argv[i];
		if(arg[0] == '\0')
			Global::fatalError("empty argument!");
		if(arg[0] == '-')
		{lastWasFlag = true; continue;}
		if(lastWasFlag)
		{lastWasFlag = false; continue;}

    vec.push_back(string(arg));
	}

	return vec;
}

map<string,string> Command::parseFlags(int argc, const char* const *argv)
{
	map<string,string> flagmap;

	bool onflag = false;
	string flagname;
	string flagval;

	for(int i = 1; i<argc; i++)
	{
		const char* arg = argv[i];
		if(arg[0] == '\0')
			Global::fatalError("Empty argument!");
		if(arg[0] == '-' && arg[1] == '\0')
			Global::fatalError("Empty flag!");

		//New flag!
		if(arg[0] == '-' && (arg[1] < '0' || arg[1] > '9'))
		{
			//Store old flag, if any
			if(onflag)
			{
				if(flagmap.find(flagname) != flagmap.end())
					Global::fatalError("Duplicate argument!");
				flagmap[flagname] = flagval;
			}

			//Grab flag name
			onflag = true;
			string s = string(arg);
			flagname = s.substr(1,s.length()-1);
			if(s.find_first_of(' ') != string::npos)
				Global::fatalError("Flag has space character in it");

			flagval = string();
			continue;
		}

		//Text part of the current flag
		if(onflag)
		{
			flagval.append(arg);

      if(flagmap.find(flagname) != flagmap.end())
         Global::fatalError("Duplicate argument!");
       flagmap[flagname] = flagval;

			onflag = false;
		}
	}

	//Store old flag, if any
	if(onflag)
	{
		if(flagmap.find(flagname) != flagmap.end())
			Global::fatalError("Duplicate argument!");
		flagmap[flagname] = flagval;
	}

	return flagmap;
}

map<string,string> Command::parseFlags(int argc, const char* const *argv,
		string required, string allowed, string empty, string nonempty)
{
	const map<string,string> flags = parseFlags(argc, argv);

	//Check that nonempty flags have actual data
	map<string,string> flagscopy = flags;
	string flag;

	istringstream inemp(empty);
	while(inemp.good())
	{
		flag = string();
		inemp >> flag;
		flag = Global::trim(flag);
		if(flag.length() == 0)
			break;
		map<string,string>::iterator it = flagscopy.find(flag);
		if(it != flagscopy.end())
		{
			if(it->second.length() != 0)
				Global::fatalError(string("Flag -") + flag + string(" takes no arguments"));
		}
	}

	istringstream innon(nonempty);
	while(innon.good())
	{
		flag = string();
		innon >> flag;
		flag = Global::trim(flag);
		if(flag.length() == 0)
			break;
		map<string,string>::iterator it = flagscopy.find(flag);
		if(it != flagscopy.end())
		{
			if(it->second.length() == 0)
				Global::fatalError(string("Flag -") + flag + string(" specified without arguments"));
		}
	}

	//Check that required strings are present
	istringstream inreq(required);
	while(inreq.good())
	{
		flag = string();
		inreq >> flag;
		flag = Global::trim(flag);
		if(flag.length() == 0)
			break;
		int numerased = flagscopy.erase(flag);
		if(numerased != 1)
			Global::fatalError(string("Required flag -") + flag + string(" not specified"));
	}

	//Remove allowed flags
	istringstream inall(allowed);
	while(inall.good())
	{
		flag = string();
		inall >> flag;
		flag = Global::trim(flag);
		if(flag.length() == 0)
			break;
		flagscopy.erase(flag);
	}

	//Make sure no flags left over
	if(flagscopy.size() != 0)
		Global::fatalError(string("Unknown flag -") + flagscopy.begin()->first);

	return flags;
}


map<string,string> Command::parseFlags(int argc, const char* const *argv,
		const char* required, const char* allowed, const char* empty, const char* nonempty)
{
	return parseFlags(argc, argv, string(required), string(allowed),
			string(empty), string(nonempty));
}


