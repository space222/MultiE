#pragma once
#include <functional>
#include "itypes.h"

//typedef void (*r3000_write)(u32, u32, int);
//typedef u32 (*r3000_read)(u32, int);

struct r3000
{
	u32 pc, hi, lo;
	u32 npc, nnpc;
	u32 r[32];
	u32 c[32];
	u32 in_delay;

	u32 ld_r[2];
	u32 ld_v[2];
	
	void reset();
	void exec(u32);
	void gte_exec(u32);
	void cop0(u32);
	void advance_load();
	void step();
	void irq();
	
	void gte_load_d(u32 r, u32 val);
	void gte_load_c(u32 r, u32 val);
	u32 gte_get_d(u32);
	u32 gte_get_c(u32);
	//r3000_read read;
	//r3000_write write;
	
	void check_irqs()
	{
		auto& cpu = *this;
		if( (cpu.c[13]&BIT(10)) && (cpu.c[12] & (1u<<10)) && (cpu.c[12]&1) && !cpu.in_delay )
		{
			cpu.c[14] = cpu.pc;
			cpu.pc = 0x80000080;
			cpu.npc = cpu.pc + 4;
			cpu.nnpc = cpu.npc + 4;
			cpu.c[12] = ((cpu.c[12]&~0xff)|((cpu.c[12]&0xf)<<2));
			cpu.c[13] &= ~0xff;
			cpu.advance_load();
		}
	}
	
	std::function<u32(u32, int)> read;
	std::function<void(u32, u32, int)> write;
	
	//giant pile of GTE regs
	s16 ZSF3, ZSF4, VX0, VY0, VX1, VY1, VX2, VY2, VZ0, VZ1, VZ2, DQA;
	s16 IR0, IR1, IR2, IR3, SX0, SY0, SX1, SY1, SX2, SY2, SXP, SYP;
	u16 OTZ, SZ0, SZ1, SZ2, SZ3, H;
	u32 flags;
	u32 RES1, IRGB, LZCS, LZCR;
	s32 TRX, TRY, TRZ, DQB, MAC0, MAC1, MAC2, MAC3, OFX, OFY;
	s32 RBK, GBK, BBK, RFC, GFC, BFC;
	u8 RGBC[4], RGB0[4], RGB1[4], RGB2[4];
	
	s16 RT11, RT12, RT13, RT21, RT22, RT23, RT31, RT32, RT33;
	s16 L11, L12, L13, L21, L22, L23, L31, L32, L33;
	s16 LR1, LR2, LR3, LG1, LG2, LG3, LB1, LB2, LB3;
	
	s16 limA1S(s64 v) { return clamp(v, -0x8000, 0x7fff, 24); }
	s16 limA2S(s64 v) { return clamp(v, -0x8000, 0x7fff, 23); }
	s16 limA3S(s64 v) { return clamp(v, -0x8000, 0x7fff, 22); }
	s16 limA1U(s64 v) { return clamp(v, 0, 0x7fff, 24); }
	s16 limA2U(s64 v) { return clamp(v, 0, 0x7fff, 23); }
	s16 limA3U(s64 v) { return clamp(v, 0, 0x7fff, 22); }
	s16 limA1C(s64 v, u32 lm) { return clamp(v, lm?0:-0x8000, 0x7fff, 24); }
	s16 limA2C(s64 v, u32 lm) { return clamp(v, lm?0:-0x8000, 0x7fff, 23); }
	s16 limA3C(s64 v, u32 lm) { return clamp(v, lm?0:-0x8000, 0x7fff, 22); }
	s16 limB1(s64 v) { return clamp(v, 0, 255, 21); }
	s16 limB2(s64 v) { return clamp(v, 0, 255, 20); }
	s16 limB3(s64 v) { return clamp(v, 0, 255, 19); }
	s16 limC(s64 v) { return clamp(v, 0, 0xffff, 18); }
	s16 limD1(s64 v) { return clamp(v, -0x400, 0x3ff, 14); }
	s16 limD2(s64 v) { return clamp(v, -0x400, 0x3ff, 13); }
	s16 limE(s64 v) { return clamp(v, 0, 0xfff, 12); }
	
	s64 F(s64);
	s16 G(s32, u32);
	u16 D(s64);
	s16 B(s64, u32, u32);
	s16 Bz(s32, s64, u32);
	s64 A(u32, s64);
	s16 limH(s64);
	u8 C(u32 index, s32 value);
	
	s32 macclamp(s64);
	s64 clamp(s64, s64, s64, u32);
	u64 strange_div(u16 h, u16 SZ);
	
	void RTP(u32 opc, u32 index, bool dq);
	void NCD(u32 opc, u32 index);
	void NCC(u32 opc, u32 index);
	void NC(u32 opc, u32 index);
	void NCLIP(u32);
	void RTPT(u32);
	void RTPS(u32);
	void AVZ3(u32);
	void AVZ4(u32);
	void NCDS(u32);
	void MVMVA(u32);
	void OP(u32);
	void DPCS(u32);
	void INTPL(u32);
	void GPF(u32);
	void SQR(u32);
	void CDP(u32);
	void CC(u32);
	void DCPL(u32);
	void DCPT(u32);
	void GPL(u32);
};










