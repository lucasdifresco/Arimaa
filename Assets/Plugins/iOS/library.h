
#pragma once
#ifndef LIBRARY_H
#define LIBRARY_H

#define WIN32_LEAN_AND_MEAN

#ifdef BUILD_MY_DLL
#define ARIMAENGINE_API __declspec(dllexport)
#else
#define ARIMAENGINE_API __declspec(dllimport)
#endif

extern "C" void InitBot();
extern "C" void InterruptBot();
extern "C" void MoveBot(const char* state, int difficulty, char* move, int length);
extern "C" const char* MoveBot2(const char* state, int difficulty);

#endif
