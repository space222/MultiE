#pragma once
#include <print>
#include <functional>
#include "itypes.h"

class tlcs900h
{
public:
	void reset();
	u64 step();
	
	
	u8 fetch()
	{
		u8 val = read(pc, 8);
		pc += 1;
		return val;
	}
	
	u16 fetch16()
	{
		u16 val = fetch();
		val |= fetch()<<8;
		return val;
	}
	
	u32 fetch24()
	{
		u32 val = fetch16();
		return val|(fetch()<<16);
	}
	
	u32 fetch32()
	{
		u32 val = fetch16();
		val |= fetch16()<<16;
		return val;
	}
	
	u8 pop8();
	u16 pop16();
	u32 pop32();
	void push8(u8 v);
	void push16(u16 v);
	void push32(u32 v);
	
	void reg8(u32 reg, u8 v) 
	{
		rb[(F.b.rfp&3)*16 + (reg&6)*2 + ((reg&1)^1)] = v;
	}
	
	void reg16(u32 reg, u16 v)
	{
		if( reg < 4 )
		{
			*(u16*)&rb[(F.b.rfp&3)*16 + reg*4] = v;
		} else {
			*(u16*)&ind[(reg-4)*4] = v;
		}
	}
	
	void reg32(u32 reg, u32 v) 
	{
		if( reg < 4 )
		{
			*(u32*)&rb[(F.b.rfp&3)*16 + reg*4] = v;
		} else {
			*(u32*)&ind[(reg-4)*4] = v;
		}
	}
	
	u8 reg8(u32 reg) { return rb[(F.b.rfp&3)*16 + (reg&6)*2 + ((reg&1)^1)]; }
	u16 reg16(u32 reg) { return (reg < 4) ? *(u16*)&rb[(F.b.rfp&3)*16 + reg*4] : *(u16*)&ind[(reg-4)*4]; }
	u32 reg32(u32 reg) { return (reg < 4) ? *(u32*)&rb[(F.b.rfp&3)*16 + reg*4] : *(u32*)&ind[(reg-4)*4]; }
	
	u8 rb[16*4 + 4];
	union alignas(4) {
		u8 ind[16];
		u32 ind32[5];
	};
	
	bool cond(u32);
	
	u32 pc;
	u8 prefix;
	bool halted;

	union flag_t
	{
		struct {
			bitfield c : 1;
			bitfield n : 1;
			bitfield v : 1;
			bitfield pad0 : 1;
			bitfield h : 1;
			bitfield pad1 : 1;
			bitfield z : 1;
			bitfield s : 1;
			bitfield rfp : 3;
			bitfield max : 1;
			bitfield iff : 3;
			bitfield sysm : 1;		
		} PACKED b;
		u16 v;
	} PACKED F;
	u8 f2;

	u64 cycles;
	
	std::function<u32(u32, int)> read;
	std::function<void(u32,u32,int)> write;
	
	u8 setsvz8(const u8 v)
	{
		F.b.z = ((v==0) ? 1:0);
		F.b.s = ((v>>7)&1);
		F.b.v = (std::popcount(v)&1)^1;
		return v;
	}

	u16 setsvz16(const u16 v)
	{
		F.b.z = ((v==0) ? 1:0);
		F.b.s = ((v>>15)&1);
		F.b.v = (std::popcount(v)&1)^1;
		return v;
	}
	
	u32 setsvz32(const u32 v)
	{
		F.b.z = ((v==0) ? 1:0);
		F.b.s = ((v>>31)&1);
		F.b.v = 0;//(std::popcount(v)&1);//^1;
		return v;
	}
	
	u8 adc8(u8 a, u8 b, u8 c)
	{
		u16 res = a;
		res += b;
		res += c;
		setsvz8(res);
		F.b.v = (((res^a)&(res^b)&0x80)? 1:0);
		F.b.c = (res>>8)&1;
		F.b.n = 0;
		F.b.h = (((a&15)+(b&15)+c)>>4)&1;
		F.b.z = (((res&0xff)==0)? 1:0);
		return res;
	}
	
	u16 adc16(u16 a, u16 b, u16 c)
	{
		u32 res = a;
		res += b;
		res += c;
		setsvz16(res);
		F.b.v = (((res^a)&(res^b)&0x8000)? 1:0);
		F.b.c = (res>>16)&1;
		F.b.n = 0;
		F.b.h = (((a&15)+(b&15)+c)>>4)&1;
		F.b.z = (((res&0xffff)==0)? 1:0);
		return res;
	}
	
	u32 adc32(u32 a, u32 b, u32 c)
	{
		u64 res = a;
		res += b;
		res += c;
		setsvz32(res);
		F.b.v = (((res^a)&(res^b)&BIT(31))? 1:0);
		F.b.c = (res>>32)&1;
		F.b.n = 0;
		F.b.h = 0;// (((a&15)+(b&15)+c)>>4)&1;
		F.b.z = ((u32(res)==0)? 1:0);
		return res;
	}
	
	u32 rrc(u32 v, u32 sa, int sz)
	{
		sa &= 15;
		if( sa == 0 ) sa = 16;
		switch( sz )
		{
		case 8:
			v &= 0xff;
			for(u32 i = 0; i < sa; ++i) v = (v>>1)|(v<<7);
			F.b.c = (v>>7)&1;
			setsvz8(v);
			v &= 0xff;
			break;
		case 16:
			v &= 0xffff;
			for(u32 i = 0; i < sa; ++i) v = (v>>1)|(v<<15);
			F.b.c = (v>>15)&1;
			setsvz16(v);
			v &= 0xffff;
			break;
		case 32:
			for(u32 i = 0; i < sa; ++i) v = (v>>1)|(v<<31);
			F.b.c = (v>>31)&1;
			setsvz32(v);		
			break;		
		}
		F.b.h = F.b.n = 0;
		return v;	
	}

	u32 rlc(u32 v, u32 sa, int sz)
	{
		sa &= 15;
		if( sa == 0 ) sa = 16;
		switch( sz )
		{
		case 8:
			v &= 0xff;
			for(u32 i = 0; i < sa; ++i) v = (v<<1)|(v>>7);
			setsvz8(v);
			v &= 0xff;
			break;
		case 16:
			v &= 0xffff;
			for(u32 i = 0; i < sa; ++i) v = (v<<1)|(v>>15);
			setsvz16(v);
			v &= 0xffff;
			break;
		case 32:
			for(u32 i = 0; i < sa; ++i) v = (v<<1)|(v>>31);
			setsvz32(v);		
			break;		
		}
		F.b.c = v&1;
		F.b.h = F.b.n = 0;
		return v;	
	}

	u32 rl(u32 v, u32 sa, int sz)
	{
		sa &= 15;
		if( sa == 0 ) sa = 16;
		switch( sz )
		{
		case 8:
			v &= 0xff;
			for(u32 i = 0; i < sa; ++i) { u32 oc = F.b.c; F.b.c = (v>>7)&1; v = ((v<<1)|oc)&0xff; }
			setsvz8(v);
			v &= 0xff;
			break;
		case 16:
			v &= 0xffff;
			for(u32 i = 0; i < sa; ++i) { u32 oc = F.b.c; F.b.c = (v>>15)&1; v = (v<<1)|oc; }
			setsvz16(v);
			v &= 0xffff;
			break;
		case 32:
			for(u32 i = 0; i < sa; ++i) { u32 oc = F.b.c; F.b.c = (v>>31)&1; v = (v<<1)|oc; }
			setsvz32(v);		
			break;		
		}
		F.b.h = F.b.n = 0;
		return v;	
	}

	u32 rr(u32 v, u32 sa, int sz)
	{
		sa &= 15;
		if( sa == 0 ) sa = 16;
		switch( sz )
		{
		case 8:
			v &= 0xff;
			for(u32 i = 0; i < sa; ++i) { u32 oc = F.b.c; F.b.c = v&1; v = (v>>1)|(oc<<7); }
			setsvz8(v);
			v &= 0xff;
			break;
		case 16:
			v &= 0xffff;
			for(u32 i = 0; i < sa; ++i) { u32 oc = F.b.c; F.b.c = v&1; v = (v>>1)|(oc<<15); }
			setsvz16(v);
			v &= 0xffff;
			break;
		case 32:
			for(u32 i = 0; i < sa; ++i) { u32 oc = F.b.c; F.b.c = v&1; v = (v>>1)|(oc<<31); }
			setsvz32(v);		
			break;		
		}
		F.b.h = F.b.n = 0;
		return v;	
	}
	
	u32 sra(u32 v, u32 sa, int sz)
	{
		sa &= 15;
		if( sa == 0 ) sa = 16;
		switch( sz )
		{
		case 8:
			v &= 0xff;
			v = s8(v) >> (sa-1);
			F.b.c = v&1;
			v = s8(v) >> 1;
			v = setsvz8(v);
			break;
		case 16:
			v &= 0xffff;
			v = s16(v) >> (sa-1);
			F.b.c = v&1;
			v = s16(v) >> 1;
			v = setsvz16(v);
			break;
		case 32:
			v = s32(v) >> (sa-1);
			F.b.c = v&1;
			v = s32(v) >> 1;
			setsvz32(v);
			break;		
		}
		F.b.h = F.b.n = 0;
		return v;	
	}
	
	u32 srl(u32 v, u32 sa, int sz)
	{
		sa &= 15;
		if( sa == 0 ) sa = 16;
		switch( sz )
		{
		case 8:
			v &= 0xff;
			v = u8(v) >> (sa-1);
			F.b.c = v&1;
			v = (v) >> 1;
			v = setsvz8(v);
			break;
		case 16:
			v &= 0xffff;
			v = u16(v) >> (sa-1);
			F.b.c = v&1;
			v = (v) >> 1;
			v = setsvz16(v);
			break;
		case 32:
			v = (v) >> (sa-1);
			F.b.c = v&1;
			v = (v) >> 1;
			setsvz32(v);
			break;		
		}
		F.b.h = F.b.n = 0;
		return v;	
	}
	
	u32 sll(u32 v, u32 sa, int sz)
	{
		sa &= 15;
		if( sa == 0 ) sa = 16;
		switch( sz )
		{
		case 8:
			v &= 0xff;
			v = (v) << (sa-1);
			F.b.c = (v>>7)&1;
			v = (v) << 1;
			v = setsvz8(v);
			break;
		case 16:
			v &= 0xffff;
			v = (v) << (sa-1);
			F.b.c = (v>>15)&1;
			v = (v) << 1;
			v = setsvz16(v);
			break;
		case 32:
			v = (v) << (sa-1);
			F.b.c = (v>>31)&1;
			v = (v) << 1;
			setsvz32(v);
			break;		
		}
		F.b.h = F.b.n = 0;
		return v;	
	}

	u32 opstart;
	
private:
	u64 regop();
	u64 dst();
	u64 src();
	int opsize;
	u32 EA;
	bool was_ldar;
	void memaddr(u8);
	void swi(u8 sn);
	
	u32 regind;
	u32 regmap(u8 reg);
	void regmap(u8 reg, u32 v, int sz);
};

#define case1(n)   case (n)
#define case2(n)   case1(n): case1((n)+1)
#define case4(n)   case2(n): case2((n)+2)
#define case8(n)   case4(n): case4((n)+4)
#define case16(n)  case8(n): case8((n)+8)


