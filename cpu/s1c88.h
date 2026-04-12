// Epson S1C88
#include <print>
#include <functional>
#include "itypes.h"

class s1c88
{
public:
	u16 IX, IY, PC, SP;
	u8 BR, CB, CC, EP, NB, SC, XP, YP;
	union {
		struct {
			u8 l, h;
		} PACKED b;		
		u16 v;	
	} PACKED BA, HL;
	u64 step();
	
	std::function<u8(u32)> read;
	std::function<void(u32,u8)> write;
	
	void reset()
	{
		CB = NB = 1;
		SC = 0xC0;
		PC = read16(0);
		XP = YP = EP = 0;
	}
	
	u32 getpc()
	{
		if( PC & 0x8000 )
		{
			return (CB<<15)|(PC & 0x7fff);		
		}
		
		return PC;	
	}

	u16 fetch16()
	{
		u16 t = read(getpc()); PC+=1;
		t |= read(getpc())<<8; PC+=1;
		return t;
	}
	
	void setc(bool v)
	{
		SC &= ~2;
		if( v ) SC |= 2;
	}
	
	void setv(bool v)
	{
		SC &= ~4;
		if( v ) SC |= 4;
	}
	
	void setn(bool v)
	{
		SC &= ~8;
		if( v ) SC |= 8;
	}
	
	void setz(bool v)
	{
		SC &= ~1;
		if( v ) SC |= 1;
	}
	
	u16 read16(u32 addr)
	{
		u16 r = read(addr);
		r |= read(addr+1)<<8;
		return r;
	}
	
	void write16(u32 addr, u16 v) { write(addr, v); write(addr+1, v>>8); }
	
	u64 extCE();
	u64 extCF();
	
	u16 pop16()
	{
		u16 r = pop();
		r |= pop()<<8;
		return r;
	}
	void push16(u16 v) { push(v>>8); push(v); }
	
	u8 pop() { u8 r = read(SP); SP+=1; return r; }
	void push(u8 v) { SP-=1; write(SP, v); }
	
	u8 add8(u8 a, u8 b, u8 c)
	{
		if( SC & BIT(4) )
		{
			std::println("Decimal mode todo");
			exit(1);
		}
	
		if( SC & BIT(5) )
		{
			u8 res = (a&15) + (b&15) + c;
			setc(res&0x10);
			res &= 15;
			setz(res==0);
			setn(res&8);
			setv((res^a)&(res^b)&8);
			return res;
		}
		
		u16 res = a;
		res += b;
		res += c;
		setc(res>>8);
		res &= 0xff;
		setz(res==0);
		setn(res&0x80);
		setv((res^a)&(res^b)&0x80);
		return res;
	}
	
	u16 add16(u16 a, u16 b, u16 c)
	{
		if( SC & BIT(4) )
		{
			std::println("Decimal mode todo");
			exit(1);
		}

		u32 res = a;
		res += b;
		res += c;
		setc(res>>16);
		res &= 0xffff;
		setz(res==0);
		setn(res&0x8000);
		setv((res^a)&(res^b)&0x8000);
		return res;	
	}
	

};

