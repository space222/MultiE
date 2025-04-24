#pragma once
#include <functional>
#include "itypes.h"

class rca1802
{
public:
	u64 step();
	void reset();
	
	std::function<u8(u16)> read;
	std::function<u8(u16)> in;
	std::function<void(u16,u32)> out;
	std::function<void(u16,u8)> write;
	
	bool waiting;
	
	u8 Q;
	u8 EF[4];

protected:
	u16 R[16];
	u8 D, DF, IE, P, X;
	u16 T;
	u8 pci; 
};

