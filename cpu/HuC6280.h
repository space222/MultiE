#pragma once
#include <print>
#include <functional>
#include "itypes.h"

class HuC6280
{
public:
	u64 step();
	void reset();
	
	std::function<u8(u32)> bus_read;
	std::function<void(u32, u8)> bus_write;
	u8 read(u16 addr) { return bus_read((mmr[addr>>13]<<13)|(addr&0x1fff)); }
	void write(u16 addr, u8 v) { bus_write((mmr[addr>>13]<<13)|(addr&0x1fff), v); }

	u8 fetch() { return read(pc++); }
	u16 fetch16() { u16 imm16 = read(pc++); imm16 |= read(pc++)<<8; return imm16; }
	
	u8 A, X, Y, S, mmr[8], lastmmr, irq_enable;
	u16 pc;
	u8 nextT, continueXfer;
	bool irq_blocked, irq1_line, timer_irq_line, high_speed;
	
	u16 xfer_src, xfer_dst, xfer_len, xfer_alt;
	
	u8 adc(u8, u8);
	u8 sbc(u8, u8);
	void cmp(u8 a, u8 b)
	{
		F.b.Z = ((a==b)? 1:0);
		F.b.C = ((a>=b)? 1:0);
		F.b.N = (((a-b)&0x80)? 1:0);
	}
	
	u8 setnz(u8 v)
	{
		F.b.Z = ((v==0) ? 1:0);
		F.b.N = ((v&0x80) ? 1:0);
		return v;
	}
	
	void push(u8 v)
	{
		write(0x2100+S, v);
		S-=1;
	}
	
	u8 pop()
	{
		S+=1;
		return read(0x2100+S);
	}

	union flag_t
	{
		struct {
			bitfield C : 1;
			bitfield Z : 1;
			bitfield I : 1;
			bitfield D : 1;
			bitfield B : 1;
			bitfield T : 1;
			bitfield V : 1;
			bitfield N : 1;
		} PACKED b;
		u8 v;
	} PACKED F;

};

