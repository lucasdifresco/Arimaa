
/*
 * main.cpp
 * Author: davidwu
 */
#include "pch.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <cstdlib>
#include "global.h"
#include "rand.h"
#include "board.h"
#include "setup.h"
#include "tests.h"
#include "arimaaio.h"
#include "command.h"
#include "init.h"
#include "main.h"

using namespace std;
using namespace ArimaaIO;

static int devMain(int argc, const char* const *argv);
static int callMain(int argc, const char* const *argv);

// int main(int argc, char* argv[])
// {	
//   	bool isDev = false;

// 	Init::init(isDev);
// 	//Init::init(isDev,494955846428891747ULL);
// 	ArimaaIO::setDefaultDirectory("data");

// 	return MainFuncs::getMove(argc,argv);
// 	//return devMain(argc, argv);
// }


//--------------------------------------------------------------------------------

static int devMain(int argc, const char* const *argv)
{
	if(argc <= 0)
		return EXIT_FAILURE;
	if(argc > 1)
	{
		return callMain(argc-1,argv+1);
	}
	else
	{
		while(cin.good())
		{
			cout << ">";

			string s;
			getline(cin,s);
			s = Global::trim(s);

			string lower = Global::toLower(s);
			if(lower == string() || lower == string("q") || lower == string("quit") || lower == string("exit"))
				break;

			vector<string> words;
			istringstream sin(s);
			while(sin.good())
			{
				string word;
				sin >> word;

				if(word.length() > 0)
					words.push_back(word);
			}

			int numWords = words.size();
			std::vector<const char*> newArgv(numWords);
			for (int i = 0; i < numWords; i++)
				newArgv[i] = words[i].c_str();

			callMain(numWords, newArgv.data());
			cout << endl;
		}
	}

	return EXIT_SUCCESS;
}

static int callMain(int argc, const char* const *argv)
{
	if(argc <= 0)
	{
		cout << "callMain with no args!" << endl;
		return EXIT_FAILURE;
	}

	string commandName = string(argv[0]);

	if(commandName == string("?"))
	{
		int i = 0;
		for(int j = 0; j<MainFuncs::numCommands; j++)
		{
			cout << MainFuncs::commandArr[j].name << " ";
			if(++i >= 4)
			{i = 0; cout << endl;}
		}
		cout << endl;
		return EXIT_SUCCESS;
	}

	if(MainFuncs::commandMap.find(commandName) == MainFuncs::commandMap.end())
	{
		cout << "Command not found: " << commandName << endl;
		return EXIT_FAILURE;
	}

	MainFuncEntry entry = MainFuncs::commandMap[commandName];
	DEBUGASSERT(entry.func != NULL);

	int result = entry.func(argc, argv);
	if(result != EXIT_SUCCESS)
		cout << "Usage: " << commandName << " " << entry.doc << endl;

	return result;
}

//-------------------------------------------------------------------------------
int MainFuncs::init(int argc, const char* const *argv)
{
	if(argc != 1 && argc != 2)
		return EXIT_FAILURE;

	if(argc == 1)
		Init::init(Init::ARIMAA_DEV);
	else
		Init::init(Init::ARIMAA_DEV,Global::stringToUInt64(string(argv[1])));

	cout << "SEED " << Init::getSeed() << endl;

	return EXIT_SUCCESS;
}

//--------------------------------------------------------------------------------

int MainFuncs::numCommands = 36;
MainFuncEntry MainFuncs::commandArr[36] =
{
		MainFuncEntry("init", MainFuncs::init, "<seed>"),
		MainFuncEntry("getMove", MainFuncs::getMove, ""),
};

static map<string,MainFuncEntry> initCommandMap()
{
	map<string,MainFuncEntry> m;
	for(int i = 0; i< MainFuncs::numCommands; i++)
		m[MainFuncs::commandArr[i].name] = MainFuncs::commandArr[i];

	return m;
}

map<string,MainFuncEntry> MainFuncs::commandMap = initCommandMap();

MainFuncEntry::MainFuncEntry()
{
	func = NULL;
}

MainFuncEntry::MainFuncEntry(const char* n, MainFunc f, const char* d)
{
	name = string(n);
	func = f;
	doc = string(d);
}

MainFuncEntry::MainFuncEntry(string n, MainFunc f, string d)
{
	name = n;
	func = f;
	doc = d;
}

MainFuncEntry::~MainFuncEntry()
{

}


