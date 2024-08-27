#pragma once
#include <string>
#include <vector>
#include "itypes.h"

class TAPFile
{
public:
	static TAPFile* load(const std::string&);

	u64 next() 
	{ 
		u64 retval = stamps[pos++]; 
		if(pos>=stamps.size()) pos = 0; 
		return retval; 
	}
	void rewind() { pos = 0; }

protected:
	std::vector<u32> stamps;
	u32 pos;
	TAPFile() { pos = 0; };
};



