
/*
 * array.h
 * A class encapsulating a statically sized array.
 *
 * Author: davidwu
 */

#ifndef ARRAY_H
#define ARRAY_H

#include <cstdlib>

template <class T>
class Array
{
	public:
	int size;
	T* list;

	Array()
	:size(0),list(NULL)
	{}

	Array(int size, T* list)
	:size(size),list(list)
	{}

	Array& operator=(const Array& other)
	{
		size = other.size;
		list = other.list;
		return *this;
	}

	T& operator[](int idx)
	{
		return list[idx];
	}

	static Array newArray(int size)
	{
		return Array(size,new T[size]);
	}

	static void deleteArray(Array& arr)
	{
		delete[] arr.list;
		arr.size = 0;
		arr.list = NULL;
	}

};

#endif
