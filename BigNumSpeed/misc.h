#pragma once

#include "stdafx.h"

template <class ConvType>
std::string ToString(ConvType obj)
{
	using std::ostringstream;

	ostringstream convert;
	convert << obj;
	return convert.str();
}