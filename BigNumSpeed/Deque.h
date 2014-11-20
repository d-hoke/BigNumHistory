#pragma once
#include "stdafx.h"

template <class DequeType>
class Deque
{
	std::vector<DequeType> buffer;
	size_t ixStart;
	size_t szBuffer;

	size_t szDeque;

	size_t GetBufferIdx(size_t ixDeque)
	{
		size_t ixBuffer = ixDeque + ixStart;
		ixBuffer = ixBuffer >= szBuffer ? ixBuffer - szBuffer : ixBuffer;
		return ixBuffer;
	}

	void GrowTo(size_t newDequeSize)
	{
		if (newDequeSize > szDeque) {
			szDeque = newDequeSize;
			if (szDeque > szBuffer) {
				int szNewBuffer = szBuffer == 0 ? 16 : szBuffer * 2;
				Reserve(szNewBuffer);
			}
		}
	}

public:
	Deque(void) : szBuffer(0), ixStart(0), szDeque(0)
	{
		
	}
	~Deque(void) { }

	void Reserve(size_t szNewBuffer)
	{
		if (szNewBuffer > szBuffer) {
			// TODO: Start here
		}
	}

	DequeType Get(size_t ixDeque)
	{
		if (ixDeque < szDeque) {
			return buffer[GetBufferIdx(ixDeque)];
		} else {
			throw "Index out of bounds.";
		}
	}

	void Set(size_t ixDeque, DequeType value)
	{
		if (ixDeque < szDeque) {
			buffer[GetBufferIdx(ixDeque)] = value;
		} else {
			throw "Index out of bounds.";
		}
	}

	void PushBack(DequeType value)
	{
		GrowTo(szDeque + 1);
		
		Set(szDeque - 1, value);
	}

	void PopBack()
	{
		if (szDeque > 0) {
			szDeque--;
		}
	}

	std::string ToString()
	{
		std::string retStr = "";
		for (size_t i = 0; i < GetSize(); i++) {
			retStr = retStr + ::ToString(Get(i));
		}
		return retStr;
	}

	size_t GetSize()
	{
		return szDeque;
	}
};

