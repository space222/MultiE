#include <cstdlib>
#include <print>
#include <bit>
#include "arm7di.h"

/*
	ARM7DI, currently only used by the Dreamcast sound chip (Yamaha AICA)
	Reuses a bunch of code from the ARM7TDMI from the GBA emulator, just with
	thumb ripped out. Still need to alter the decode function to only include
	instructions that exist in the 7DI
*/

#define SBIT (opc&BIT(20))
#define setNZ(a) cpu.cpsr.b.Z = ((a)==0?1:0); cpu.cpsr.b.N = ((a)>>31)&1

static auto undef_instr = [](arm& cpu, u32 opc) { std::println("${:X}: unimpl opc ${:X}", cpu.r[15]-(cpu.cpsr.b.T?4:8), opc); exit(1); };

void arm7_mul(arm& cpu, u32 opc);
void arm7_single_swap(arm& cpu, u32 opc);
void arm7_xfer_half(arm& cpu, u32 opc);
void arm7_xfer_signed(arm& cpu, u32 opc);
void arm7_mrs(arm& cpu, u32 opc);
void arm7_msr_reg(arm& cpu, u32 opc);
void arm7_msr_imm(arm& cpu, u32 opc);
u32 dataproc_op2(arm& cpu, u32 opc);
void arm7_and(arm& cpu, u32 opc);
void arm7_eor(arm& cpu, u32 opc);
void arm7_sub(arm& cpu, u32 opc);
void arm7_rsb(arm& cpu, u32 opc);
void arm7_add(arm& cpu, u32 opc);
void arm7_adc(arm& cpu, u32 opc);
void arm7_sbc(arm& cpu, u32 opc);
void arm7_rsc(arm& cpu, u32 opc);
void arm7_tst(arm& cpu, u32 opc);
void arm7_teq(arm& cpu, u32 opc);
void arm7_cmp(arm& cpu, u32 opc);
void arm7_cmn(arm& cpu, u32 opc);
void arm7_orr(arm& cpu, u32 opc);
void arm7_mov(arm& cpu, u32 opc);
void arm7_bic(arm& cpu, u32 opc);
void arm7_mvn(arm& cpu, u32 opc);
u32 arm7_ldst_shift(arm& cpu, u32 opc);
void arm7_ldst_reg(arm& cpu, u32 opc);
void arm7_ldst_imm(arm& cpu, u32 opc);
void arm7_ldst_m(arm& cpu, u32 opc);

arm7_instr arm7di::decode_arm(u32 cc)
{
	if( (cc&0xfcf) == 9 ) { return arm7_mul; }
	if( (cc&0xfbf) == 0x109) { return arm7_single_swap; }

	//if( (cc&0xe0f) == 0xB ) { return arm7_xfer_half; }
	//if( (cc&0xe1d) == 0x1D ){ return arm7_xfer_signed; }
	
	if( (cc&0xfbf) == 0x100 ) { return arm7_mrs; }
	if( (cc&0xfbf) == 0x120 ) { return arm7_msr_reg; }
	if( (cc&0xfb0) == 0x320 ) { return arm7_msr_imm; }

	if( (cc&0xfb0) == 0x300 ) { return undef_instr; }
	if( (cc&0xe00) == 0x000 || (cc&0xe00) == 0x200 )
	{  // data proccessing
		switch( (cc>>5) & 15 )
		{
		case 0: return arm7_and;
		case 1: return arm7_eor;
		case 2: return arm7_sub;
		case 3: return arm7_rsb;
		case 4: return arm7_add;
		case 5: return arm7_adc;
		case 6: return arm7_sbc;
		case 7: return arm7_rsc;
		case 8: return arm7_tst;
		case 9: return arm7_teq;
		case 10: return arm7_cmp;
		case 11: return arm7_cmn;
		case 12: return arm7_orr;
		case 13: return arm7_mov;
		case 14: return arm7_bic;
		default: break;
		}
		return arm7_mvn;	
	}
	
	if( (cc&0xe00) == 0x400 ) return arm7_ldst_imm;
	if( (cc&0xe00) == 0x600 ) return arm7_ldst_reg;
	if( (cc&0xe00) == 0x800 ) return arm7_ldst_m;

	if( (cc&0xe00) == 0xa00 )
	{  // B / BL
		return [](arm& cpu, u32 opc) {
			if( opc & BIT(24) )
			{
				cpu.r[14] = cpu.r[15] - 4;
			}
			s32 offs = s32(opc<<8)>>8;
			cpu.r[15] += offs*4;
			//std::println("arm7 b(l) to ${:X}", cpu.r[15]);
			cpu.flushp();
		};
	}

	if( (cc&0xf00) == 0xf00 )
	{   // SWI
		return [](arm& cpu, u32) {
			cpu.spsr_svc = cpu.cpsr.v;
			cpu.switch_to_mode(ARM_MODE_SUPER);
			cpu.cpsr.b.M = ARM_MODE_SUPER;
			cpu.cpsr.b.I = 1;
			cpu.r[14] = cpu.r[15]-4;
			cpu.r[15] = 8;
			cpu.flushp();
		};
	}

	std::println("Reached end of decode_arm()");
	return undef_instr;
}

void arm7di::flushp() 
{
	r[15] &= ~3;
	decode = read(r[15], 32, ARM_CYCLE::N);
	r[15] += 4;
	fetch = read(r[15], 32, ARM_CYCLE::S);
}

void arm7di::step()
{
	if( irq_line && cpsr.b.I==0 )
	{
		spsr_irq = cpsr.v;
		switch_to_mode(ARM_MODE_IRQ);
		cpsr.b.M = ARM_MODE_IRQ;
		cpsr.b.I = 1;
		r[14] = r[15];// + (cpsr.b.T?2:0);
		//cpsr.b.T = 0;
		r[15] = 0x18;
		flushp();
	}

	execute = decode;
	decode = fetch;
	r[15] += 4;
	//std::println("ARM7DI:${:X}: opc = ${:X}", r[15]-8, execute);
	fetch = read(r[15]&~3, 32, ARM_CYCLE::X);
	u32 opc = execute;
	
	//if( r[15] > 0x08000740 ) std::println("${:X}:{:X}: opc = ${:X}", r[15] - (cpsr.b.T?4:8), u32(cpsr.b.T), opc);
	
	/*if( r[15] >= 0x10000000u )
	{
		std::println("${:X} too big, halting", (r[15]&(cpsr.b.T?~1:~3)) - (cpsr.b.T?4:8));
		exit(1);
	}*/
	
	if( isCond(opc>>28) )
	{
 		decode_arm(((opc>>16)&0xff0) | ((opc>>4)&15))(*this, opc);
	}
	
	stamp+=1;
}

void arm7di::reset()
{
	//todo: copied from my old emu, need to double check
	//std::println("arm7di::reset()");
	cpsr.b.M = ARM_MODE_USER;
	r[13] = 0x03007F00;
	r13_svc = 0x03007FE0; 
	r13_irq =  0x03007FA0;
	switch_to_mode(ARM_MODE_SUPER);
	spsr_svc = 0x9f;
	r[15] = 0;//0x08000000u;
	cpsr.b.T = 0;
	cpsr.b.I = 0;
	stamp = 0;
	flushp();
	irq_line = false;
}

bool arm7di::isCond(u8 cc)
{
	switch(cc)
	{
	case 0: return cpsr.b.Z==1;
	case 1: return cpsr.b.Z==0;
	case 2: return cpsr.b.C==1;
	case 3: return cpsr.b.C==0;
	case 4: return cpsr.b.N==1;
	case 5: return cpsr.b.N==0;
	case 6: return cpsr.b.V==1;
	case 7: return cpsr.b.V==0;
	case 8: return cpsr.b.C==1 && cpsr.b.Z==0;
	case 9: return cpsr.b.C==0 || cpsr.b.Z==1;
	case 10: return cpsr.b.N==cpsr.b.V;
	case 11: return cpsr.b.N!=cpsr.b.V;
	case 12: return cpsr.b.Z==0 && cpsr.b.N==cpsr.b.V;
	case 13: return cpsr.b.Z==1 || cpsr.b.N!=cpsr.b.V; 
	case 14: return true;
	default: break;
	}
	return false; 
}

void arm7di::switch_to_mode(u32 mode) 
{
	mode &= 0x1f;
	mode |= 0x10; // ?? check
	switch( cpsr.b.M )
	{
	case ARM_MODE_IRQ:
		std::swap(r[14], r14_irq);
		std::swap(r[13], r13_irq);
		break;
	case ARM_MODE_ABORT:
		std::swap(r[14], r14_abt);
		std::swap(r[13], r13_abt);
		break;
	case ARM_MODE_SUPER:
		std::swap(r[14], r14_svc);
		std::swap(r[13], r13_svc);
		break;
	case ARM_MODE_UNDEF:
		std::swap(r[14], r14_und);
		std::swap(r[13], r13_und);
		break;
	case ARM_MODE_FIQ:
		for(u32 i = 8; i < 15; ++i) std::swap(fiq[i], r[i]);
		break;
	default: break;
	}
	
	switch( mode )
	{
	case ARM_MODE_IRQ:
		std::swap(r[14], r14_irq);
		std::swap(r[13], r13_irq);
		break;
	case ARM_MODE_ABORT:
		std::swap(r[14], r14_abt);
		std::swap(r[13], r13_abt);
		break;
	case ARM_MODE_SUPER:
		std::swap(r[14], r14_svc);
		std::swap(r[13], r13_svc);
		break;
	case ARM_MODE_UNDEF:
		std::swap(r[14], r14_und);
		std::swap(r[13], r13_und);
		break;
	case ARM_MODE_FIQ:
		for(u32 i = 8; i < 15; ++i) std::swap(fiq[i], r[i]);
		break;
	default: break;
	}
	
	cpsr.b.M = mode;
}

void arm7di::swi() { }

arm7di::arm7di()
{
	armV = 4; // ARM7DI isn't v4, but armV is really to tell between 4 and 5.
}










