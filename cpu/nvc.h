#pragma once
#include <functional>
#include "itypes.h"

union nvcflags
{
	struct {
		unsigned int z : 1;
		unsigned int s : 1;
		unsigned int ov : 1;
		unsigned int cy : 1;	
	} PACKED b;
	u8 v;
} PACKED;

class nvc
{
public:
	void reset();
	u64 step();
	void exec(u16 opc);

	bool halted;
	u32 pc;
	u32 r[32];
	u32 sys[32];
	
	u32& ECR = sys[4];
	u32& PSW = sys[5];
	
	void setsz(u32 v)
	{
		PSW &= ~3;
		PSW |= (v==0)?1:0;
		PSW |= (v&BIT(31))?2:0;
	}
	
	void setov(bool b)
	{
		if( b )
		{
			PSW &= ~4;
		} else {
			PSW |= 4;
		}
	}
	
	void setcy(bool b)
	{
		if( b )
		{
			PSW &= ~8;
		} else {
			PSW |= 8;
		}
	}
	
	u32 add(u32 a, u32 b, u32 c = 0)
	{
		u64 res = c;
		res += a;
		res += b;
		setsz(res);
		setov((res^a)&(res^b)&BIT(31));
		setcy(res>>32);	
		return res;
	}
	
	bool isCond(u8 cc)
	{
		cc &= 15;
		nvcflags F;
		F.v = PSW;
		switch( cc )
		{
		case 0: return F.b.ov;
		case 1: return F.b.cy;
		case 2: return F.b.z;
		case 3: return F.b.cy || F.b.z;
		case 4: return F.b.s;
		case 5: return true;
		case 6: return F.b.ov ^ F.b.s;
		case 7: return (F.b.ov^F.b.s)||F.b.z;
		case 8: return !F.b.ov;
		case 9: return !F.b.cy;
		case 10: return !F.b.z;
		case 11: return !F.b.cy || !F.b.z;
		case 12: return !F.b.s;
		case 13: return false;
		case 14: return !(F.b.ov^F.b.s);
		case 15: return !((F.b.ov^F.b.s)||F.b.z);
		}
		return true;
	}
	
	std::function<u32(u32,int)> read;
	std::function<void(u32,u32,int)> write;
	
	u64 icyc;
};

