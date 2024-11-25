#pragma once
#include <functional>
#include <cstring>
#include "itypes.h"

union nvcflags
{
	struct {
		unsigned int z : 1;
		unsigned int s : 1;
		unsigned int ov : 1;
		unsigned int cy : 1;
		unsigned int pad : 4;
	} PACKED b;
	u8 v;
} PACKED;

class nvc
{
public:
	void reset();
	u64 step();
	void exec(u16 opc);
	void exec_fp(u32,u32,u16);
	
	void irq(u32,u16,u32);
	
	bool halted;
	u32 pc;
	u32 r[32];
	u32 sys[32];
	
	float f(u32 reg) 
	{
		float t = 0;
		memcpy(&t, &r[reg], 4);
		return t; 
	}
	void f(u32 reg, float v) 
	{ 
		memcpy(&r[reg], &v, 4);
	}
	
	u32& ECR = sys[4];
	u32& PSW = sys[5];
	
	void setsz(u32 v)
	{
		PSW &= ~3;
		PSW |= ( (v==0)?1:0 );
		PSW |= ( (v&BIT(31))?2:0 );
	}
	
	void setov(bool b)
	{
		if( b )
		{
			PSW |= 4;
		} else {
			PSW &= ~4;
		}
	}
	
	void setcy(bool b)
	{
		if( b )
		{
			PSW |= 8;
		} else {
			PSW &= ~8;
		}
	}
	
	u32 add(u32 a, u32 b, u32 c = 0)
	{
		u64 res = c;
		res += a;
		res += b;
		setsz(res);
		setov((res^a)&(res^b)&BIT(31));
		setcy(((res>>32)&1)^c);
		return res;
	}
	
	bool isCond(u8 cc)
	{
		cc &= 15;
		nvcflags F;
		F.v = PSW;
		bool res = false;
		switch( cc & 7 )
		{
		case 0: res = F.b.ov; break;
		case 1: res = F.b.cy; break;
		case 2: res = F.b.z; break;
		case 3: res = (F.b.cy||F.b.z); break;
		case 4: res = F.b.s; break;
		case 5: res = true; break;
		case 6: res = (F.b.s^F.b.ov); break;
		case 7: res = ((F.b.s^F.b.ov)||F.b.z); break;
		}
		if( cc & BIT(3) ) res = !res;
		return res;
	}
	
	std::function<u32(u32,int)> read;
	std::function<void(u32,u32,int)> write;
	
	u64 icyc;
};

