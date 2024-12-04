#pragma once
#include <concepts>
#include <bit>
#include <functional>
#include "itypes.h"

//typedef void (*c80286_writer)(u32 addr, u8 val);
//typedef u8 (*c80286_reader)(u32 addr);
//typedef void (*c80286_port_out)(u16, u16, int);
//typedef u16 (*c80286_port_in)(u16, int);

union mreg86
{
	struct
	{
		u8 l, h;
	} PACKED b;
	u16 v;
} PACKED;

union c80286_flags
{
	struct
	{
		unsigned int C : 1;
		unsigned int pad0 : 1;
		unsigned int P : 1;
		unsigned int pad1 : 1;
		unsigned int A : 1;
		unsigned int pad2 : 1;
		unsigned int Z : 1;
		unsigned int S : 1;
		unsigned int T : 1;
		unsigned int I : 1;						
		unsigned int D : 1;	
		unsigned int O : 1;
		unsigned int top4 : 4;
	} b;
	u16 v;
} PACKED;

class c80286
{
public:
	
	void reset();
	u64 step();
		
	u8 read_next()
	{
		//todo: protected mode
		return read((seg[1]<<4)+(IP++));
	}
	
	u16 read_next16()
	{
		u16 res = read_next();
		res |= read_next()<<8;
		return res;
	}
	
	u16 read16(u16 s, u16 off)
	{
		u16 res = read((s<<4) + off); off++;
		res |= read((s<<4) + off)<<8;
		return res;	
	}
	
	void write16(u16 s, u16 off, u16 v)
	{
		write((s<<4)+off, v);	off++;
		write((s<<4)+off, v>>8);
	}
	
	bool is_prefix(u8);
	
	void setSZP8(std::same_as<u8> auto v)
	{
		F.b.Z = (v==0)?1:0;
		F.b.S = (v&0x80)?1:0;
		F.b.P = (std::popcount(v)&1)?0:1;
	}
	
	void setSZP16(std::same_as<u16> auto v)
	{
		F.b.Z = (v==0)?1:0;
		F.b.S = (v&0x8000)?1:0;
		F.b.P = (std::popcount(u8(v))&1)?0:1;
	}
	
	void push(u16);
	u16 pop();
	u8 add8(u8,u8,u8);
	u8 sub8(u8,u8,u8);
	u16 add16(u16,u16,u16);
	u16 sub16(u16,u16,u16);
	
	u8 seg_override(u8 index);
	
	//c80286_reader read;
	//c80286_writer write;
	//c80286_port_out port_out;
	//c80286_port_in port_in;
	std::function<u8(u32)> read;
	std::function<void(u32,u8)> write;
	std::function<void(u16,u16,int)> port_out;
	std::function<u16(u16,int)> port_in;
	
	struct {
		u8 irr, isr, imr;
		u8 icw[4];
		u8 ocw[3];
		u8 read_reg;
		u8 state;
	} pic;
	
	bool halted;
	u32 prefix;
	u16 start_ip, start_cs;
	u16 IP;
	u16 seg[6];
	mreg86 r[8];
	c80286_flags F;
	bool irq_blocked;
};

