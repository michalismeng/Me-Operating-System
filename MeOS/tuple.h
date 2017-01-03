#ifndef TUPLE_H_15112016
#define TUPLE_H_15112016

#include "types.h"

template<class X, class Y>
struct tuple
{
	X first;
	Y second;
};

template<class X, class Y>
tuple<X, Y> make_pair(X first, Y second)
{
	tuple<X, Y> t;
	t.first = first;
	t.second = second;

	return t;
}

#endif