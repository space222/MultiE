#include <print>
#include <cmath>
#include <string>
#include "gekko.h"
#define instr return [](gekko& cpu, u32 opc)
#define instr_nop return [](gekko&,u32){}
#define BO ((opc>>21)&0x1F)
#define BI ((opc>>16)&0x1F)
#define crbD BO
#define crbA BI
#define crbB ((opc>>11)&0x1F)
#define rD cpu.r[(opc>>21)&0x1F]
#define rS rD
#define rA cpu.r[(opc>>16)&0x1F]
#define rB cpu.r[(opc>>11)&0x1F]
#define zrA (((opc>>16)&0x1F) ? rA : 0)
#define IMM16 (opc&0xffff)
#define SIMM16 s16(opc&0xffff)
#define crfD ((opc>>23)&7)
#define crfS ((opc>>18)&7)
#define Rc (opc&1)
#define MB ((opc>>6)&0x1F)
#define ME ((opc>>1)&0x1F)
#define SH ((opc>>11)&0x1F)
#define frD (*(double*)&cpu.f[((opc>>21)&0x1F)<<3])
#define frD_PS1 cpu.ps1[((opc>>21)&0x1F)]
#define frA (*(double*)&cpu.f[((opc>>16)&0x1F)<<3])
#define frA_PS1 cpu.ps1[((opc>>16)&0x1F)]
#define frB (*(double*)&cpu.f[((opc>>11)&0x1F)<<3])
#define frB_PS1 cpu.ps1[((opc>>11)&0x1F)]
#define frC (*(double*)&cpu.f[((opc>>6)&0x1F)<<3])
#define frC_PS1 cpu.ps1[((opc>>6)&0x1F)]
#define frS frD
#define MSR_TO_SRR_MASK 0x87c0ffff

static gekko::instr_type decode_59(u32 opcode)
{
	switch( (opcode>>1)&0x1F )
	{
	case 18: instr { frD = (float)(frA / frB); if( cpu.hid2.b.pse ) { frD_PS1 = frD; } if(Rc){ /*cpu.updateCR1();*/ } }; // fdivs
	case 20: instr { frD = (float)(frA - frB); if( cpu.hid2.b.pse ) { frD_PS1 = frD; } if(Rc){ /*cpu.updateCR1();*/ } }; // fsubs
	case 21: instr { frD = (float)(frA + frB); if( cpu.hid2.b.pse ) { frD_PS1 = frD; } if(Rc){ /*cpu.updateCR1();*/ } }; // fadds
	case 24: instr { frD = (float)(1.0 / frB); if( cpu.hid2.b.pse ) { frD_PS1 = frD; } if(Rc){ /*cpu.updateCR1();*/ } }; // fres	
	case 25: instr { frD = (float)(frA * frC); if( cpu.hid2.b.pse ) { frD_PS1 = frD; } if(Rc){ /*cpu.updateCR1();*/ } }; // fmuls
	case 28: instr { frD = (float)(frA * frC - frB); if( cpu.hid2.b.pse ) { frD_PS1 = frD; } if(Rc){ /*cpu.updateCR1();*/ } }; // fmsubs
	case 29: instr { frD = (float)(frA * frC + frB); if( cpu.hid2.b.pse ) { frD_PS1 = frD; } if(Rc){ /*cpu.updateCR1();*/ } }; // fmadds
	case 30: instr { frD = (float)-(frA * frC - frB); if( cpu.hid2.b.pse ) { frD_PS1 = frD; } if(Rc){ /*cpu.updateCR1();*/ } }; // fnmsubs
	case 31: instr { frD = (float)-(frA * frC + frB); if( cpu.hid2.b.pse ) { frD_PS1 = frD; } if(Rc){ /*cpu.updateCR1();*/ } }; // fnmadds
	default:
		std::println("Unimpl opc59, bot5 = {:05b}", (opcode>>1)&0x1f);
		break;
	}
	return nullptr;
}

static gekko::instr_type decode_63(u32 opcode)
{
	if( opcode & BIT(5) ) switch( (opcode>>1) & 0x1F )
	{
	case 18: instr { frD = frA / frB; }; // fdiv
	case 20: instr { frD = frA - frB; }; // fsub
	case 21: instr { frD = frA + frB; }; // fadd
	case 23: instr { frD = ((frA >= 0.0f) ? frC : frB); }; // fsel
	case 25: instr { frD = frA * frC; }; // fmul
	case 26: instr { frD = 1.0/std::sqrt(frB); }; // frsqrte
	case 28: instr { frD = frA * frC - frB; }; // fmsub
	case 29: instr { frD = frA * frC + frB; }; // fmadd
	case 30: instr { frD = -(frA * frC - frB); }; // fNmsub
	case 31: instr { frD = -(frA * frC + frB); }; // fNmadd
	default:
		std::println("Unimpl opc63, bot5 = {:05b}", (opcode>>1)&0xf);
		return nullptr;
	}
	
	switch( (opcode>>1) & 0x3FF )
	{
	case 32: // fcmpo
	case 0: instr { // fcmpu
			u32 c = 0;
			if( std::isnan(frA) || std::isnan(frB) ) // frA or frB are NaN
			{
				c = 0b0001;
			} else if( frA < frB ) {
				c = 0b1000;
			} else if( frA > frB ) {
				c = 0b0100;
			} else {
				c = 0b0010;
			}
			cpu.fpscr.b.fprf = c;
			cpu.cond.b[crfD] = c;
		};
		
	case 64: instr { // mcrfs
			u32 fex = cpu.fpscr.b.fex;
			u32 vx = cpu.fpscr.b.vx;
			u32 f = (cpu.fpscr.v >> (31-(crfS*4)))&0xf;
			cpu.fpscr.v &= ~(0xf << (31-(crfS*4)));
			cpu.fpscr.b.fex = fex;
			cpu.fpscr.b.vx = vx;
			cpu.cond.b[crfD] = f;
		};

	case 70: instr { u32 bit = ((opc>>21)&0x1F); if(bit!=1&&bit!=2) cpu.fpscr.v &= ~(1u<<(31-bit)); }; // mtfsb0.
	case 38: instr { u32 bit = ((opc>>21)&0x1F); if(bit!=1&&bit!=2) cpu.fpscr.v |= 1u<<(31-bit); }; // mtfsb1.
	case 12: instr { frD = (float)frB; }; // frsp
	case 14: instr { frD = std::bit_cast<double>((u64)(u32)(s32)std::clamp<double>(frB, -0x80000000ll, 0x7fffffffll)); }; // fctiw
	case 15: instr { frD = std::bit_cast<double>((u64)(u32)(s32)std::clamp<double>(frB, -0x80000000ll, 0x7fffffffll)); }; // fctiwz
	case 40: instr { frD = std::bit_cast<double>(std::bit_cast<u64>(frB)^BITL(63)); }; // fneg
	
	case 72: instr { frD = frB; }; // fmr
	
	case 134: instr { // mtfsfi
			u32 imm = (opc>>12)&0xf;
			u32 fex = cpu.fpscr.b.fex;
			u32 vx = cpu.fpscr.b.vx;
			cpu.fpscr.v &= ~(0xf<< (31-(crfD*4)) );
			cpu.fpscr.v |= (imm << (31-(crfD*4)) );	
			cpu.fpscr.b.fex = fex;
			cpu.fpscr.b.vx = vx;
		};

	case 136: instr { frD = std::bit_cast<double>(std::bit_cast<u64>(frB)|BITL(63)); }; // fnabs
	case 264: instr { frD = std::bit_cast<double>(std::bit_cast<u64>(frB)&~BITL(63)); }; // fabs
	
	case 583: instr { frD = std::bit_cast<double>((u64)cpu.fpscr.v); }; // mffs
	
	case 711: instr { // mtfsf
			u32 FM = (opc>>17)&0xff;
			u32 mask = 0;
			for(u32 i = 0; i < 8; ++i) { if(FM&BIT(i)) mask |= 0xf << ((i)*4); }
			u32 fex = cpu.fpscr.b.fex;
			u32 vx = cpu.fpscr.b.vx;
			//u32 fx = cpu.fpscr.b.fx;
			cpu.fpscr.v = (cpu.fpscr.v&~mask) | (mask&((u32)std::bit_cast<u64>(frB)));
			cpu.fpscr.b.fex = fex;
			cpu.fpscr.b.vx = vx;
			//cpu.fpscr.b.fx = fx | (FM>>7);
		};
	default:
		std::println("Unimpl opc63, bot10 = {}dec", (opcode>>1)&0x3ff);
		break;
	}	
	return nullptr;
}

static gekko::instr_type decode_31(u32);

static gekko::instr_type decode_19(u32 opcode)
{
	switch( (opcode>>1)&0x3ff )
	{
	case 0: instr { cpu.cond.cr[crfD] = cpu.cond.cr[crfS]; }; // mcrf
	case 16: instr { // BCLRx
			if( !(BO&BIT(2)) ) cpu.CTR -= 1;
			bool ctr_ok = (BO&BIT(2)) || ((cpu.CTR!=0)^((BO&BIT(1))>>1));
			bool cond_ok = (BO&BIT(4)) || (cpu.cond.bit(BI)==((BO&BIT(3))?1:0));
			if( ctr_ok && cond_ok )
			{
				u32 temp = cpu.LR;
				if(opc&1) { cpu.LR=cpu.pc; }
				cpu.pc = temp;
			}
		};
	case 50: instr { cpu.pc = cpu.SRR0 & ~3; cpu.msr.v &= ~MSR_TO_SRR_MASK; cpu.msr.v |= cpu.SRR1 & MSR_TO_SRR_MASK; cpu.msr.b.pow=0; }; // rfi
	case 33: instr { cpu.cond.set_bit(crbD, 1^(cpu.cond.bit(crbA) | cpu.cond.bit(crbB))); }; // crnor
	case 129: instr { cpu.cond.set_bit(crbD, cpu.cond.bit(crbA) & (1^cpu.cond.bit(crbB))); }; // crandc
	case 150: instr_nop; // isync
	case 193: instr { cpu.cond.set_bit(crbD, cpu.cond.bit(crbA) ^ cpu.cond.bit(crbB)); }; // crxor
	case 225: instr { cpu.cond.set_bit(crbD, 1^(cpu.cond.bit(crbA) & cpu.cond.bit(crbB))); }; // crnand
	case 257: instr { cpu.cond.set_bit(crbD, cpu.cond.bit(crbA) & cpu.cond.bit(crbB)); }; // crand
	case 289: instr { cpu.cond.set_bit(crbD, cpu.cond.bit(crbA) ^ (1^cpu.cond.bit(crbB))); }; // creqv
	case 417: instr { cpu.cond.set_bit(crbD, cpu.cond.bit(crbA) | (1^cpu.cond.bit(crbB))); }; // crorc
	case 449: instr { cpu.cond.set_bit(crbD, cpu.cond.bit(crbA) | cpu.cond.bit(crbB)); }; // cror
	case 528: instr { // bcctr
			bool cond_ok = (BO&BIT(4)) || (cpu.cond.bit(BI)==((BO>>3)&1)); 
			if( cond_ok ) 
			{
				if( opc&1 ) { cpu.LR = cpu.pc; }
				cpu.pc = cpu.CTR & ~3;
			}
		};
	default:
		std::println("Unimpl opc 19, bot10 = {}dec", (opcode>>1)&0x3ff);
		return nullptr;
	}
}

static gekko::instr_type decode_opcode(u32 opcode)
{
	switch( opcode>>26 )
	{
	case 7: instr { rD = s32(rA) * SIMM16; }; // mulli
	case 8: instr { u32 t=~rA; u64 a = t; a += (u32)SIMM16; a += 1; rD=a; cpu.xer.b.ca = a>>32; }; // subfic

	case 10: instr { //cmpli
			cpu.cond.cr[crfD].lt = u32(rA)<IMM16;
			cpu.cond.cr[crfD].gt = u32(rA)>IMM16;
			cpu.cond.cr[crfD].eq = u32(rA)==IMM16;
			cpu.cond.cr[crfD].so = cpu.xer.b.so;
		};
	case 11: instr { //cmpi
			cpu.cond.cr[crfD].lt = s32(rA)<SIMM16;
			cpu.cond.cr[crfD].gt = s32(rA)>SIMM16;
			cpu.cond.cr[crfD].eq = s32(rA)==SIMM16;
			cpu.cond.cr[crfD].so = cpu.xer.b.so;
		};
	case 12: instr { u64 a = rA; a += SIMM16; cpu.xer.b.ca = a>>32; rD=a; }; // addic
	case 13: instr { u64 a = rA; a += SIMM16; cpu.xer.b.ca = a>>32; rD=a; cpu.setCR0(rD); }; // addic
	case 14: instr { rD = zrA + SIMM16; }; // addi
	case 15: instr { rD = zrA + (IMM16<<16); }; // addis
	case 16: instr { // BCx
			if( !(BO&BIT(2)) ) { cpu.CTR -= 1; }
			bool ctr_ok = (BO&BIT(2)) || ((cpu.CTR!=0)^((BO&BIT(1))>>1));
			bool cond_ok = (BO&BIT(4)) || (cpu.cond.bit(BI)==((BO&BIT(3))?1:0));
			if( ctr_ok && cond_ok )
			{
				u32 exts = s16(opc&~3); 
				if(!(opc&BIT(1))) { exts+=cpu.pc-4; } 
				if(opc&1) { cpu.LR=cpu.pc; }
				cpu.pc = exts;
			}
		};
	case 17: instr { cpu.SRR0 = cpu.pc; cpu.SRR1 = cpu.msr.v & MSR_TO_SRR_MASK; cpu.msr.b.ee = cpu.msr.b.ri = 0; cpu.pc = 0xc00; }; // sc (syscall)	
	case 18: instr { u32 exts = (s32(opc<<6)>>6)&~3; if(!(opc&BIT(1))) { exts+=cpu.pc-4; } if(opc&1) { cpu.LR=cpu.pc; } cpu.pc = exts;}; // Bx
	case 19: return decode_19(opcode);
	case 20: instr { // rlwimi
			u32 me = 31-ME;
			u32 mb = 31-MB;
			u32 r = std::rotl(rS, SH);
			u32 mask = (mb>=me) ? (((1u<<(mb+1))-1) & ~((1u<<(me))-1)) : (~((1u<<(me))-1) | ((1u<<(mb+1))-1));
			//std::println("mask = ${:X}", mask);
			rA = (r & mask) | (rA & ~mask);
			if(Rc) { cpu.setCR0(rA); }
		};
	case 21: instr { // rlwinm
			u32 me = 31-ME;
			u32 mb = 31-MB;
			std::println("${:X}: rS = ${:X}", cpu.pc-4, rS);
			u32 r = std::rotl(rS, SH);
			u32 mask = (mb>=me) ? (((1u<<(mb+1))-1) & ~((1u<<(me))-1)) : (~((1u<<(me))-1) | ((1u<<(mb+1))-1));
			//std::println("mask = ${:X}", mask);
			rA = r & mask;
			std::println("mask = ${:X}, SH={}, result=${:X}", mask, SH, rA);
			if(Rc) { cpu.setCR0(rA); }
		};

	case 23: instr { // rlwnm
			u32 me = 31-ME;
			u32 mb = 31-MB;
			u32 r = std::rotl(rS, rB&0x1F);
			u32 mask = (mb>=me) ? (((1u<<(mb+1))-1) & ~((1u<<(me))-1)) : (~((1u<<(me))-1) | ((1u<<(mb+1))-1));
			//std::println("mask = ${:X}", mask);
			rA = r & mask;
			if(Rc) { cpu.setCR0(rA); }
		};
	case 24: instr { rA = rS | IMM16; }; // ori
	case 25: instr { rA = rS | (IMM16<<16); }; // oris
	case 26: instr { rA = rS ^ IMM16; }; // xori
	case 27: instr { rA = rS ^ (IMM16<<16); }; // xoris
	case 28: instr { rA = rS & IMM16; cpu.setCR0(rA); }; // andi.
	case 29: instr { rA = rS & (IMM16<<16); cpu.setCR0(rA); }; // andis.

	case 31: return decode_31(opcode);
	case 32: instr { rD = cpu.read(zrA+SIMM16, 32); }; // lwz
	case 33: instr { u32 EA=rA+SIMM16; rA=EA; rD = cpu.read(EA, 32); }; // lwzu
	case 34: instr { rD = (u8)cpu.read(zrA+SIMM16, 8); }; // lbz
	case 35: instr { u32 EA=rA+SIMM16; rA=EA; rD = (u8)cpu.read(EA, 8); }; // lbzu
	case 36: instr { cpu.write(zrA + SIMM16, rS, 32); }; // stw
	case 37: instr { cpu.write(rA + SIMM16, rS, 32); rA += SIMM16; }; // stwu	
	case 38: instr { cpu.write(zrA + SIMM16, rS, 8); }; // stb
	case 39: instr { cpu.write(rA + SIMM16, rS, 8); rA += SIMM16; }; // stbu
	case 40: instr { rD = (u16)cpu.read(zrA+SIMM16, 16); }; // lhz
	case 41: instr { u32 EA=rA+SIMM16; rA=EA; rD = (u16)cpu.read(EA, 16); }; // lhzu
	case 42: instr { rD = (s16)cpu.read(zrA+SIMM16, 16); }; // lha
	case 43: instr { u32 EA=rA+SIMM16; rA=EA; rD = (s16)cpu.read(EA, 16); }; // lhau
	case 44: instr { cpu.write(zrA + SIMM16, rS, 16); }; // sth
	case 45: instr { cpu.write(rA + SIMM16, rS, 16); rA += SIMM16; }; // sthu
	case 46: instr { //lmw
			u32 d = (opc>>21)&0x1F;
			u32 EA = zrA + SIMM16;
			while( d <= 31 )
			{
				cpu.r[d] = cpu.read(EA, 32);
				d += 1;
				EA += 4;
			}	
		};
	case 47: instr { //stmw
			u32 s = (opc>>21)&0x1F;
			u32 EA = zrA + SIMM16;
			while( s <= 31 )
			{
				cpu.write(EA, cpu.r[s], 32);
				s += 1;
				EA += 4;
			}	
		};
	case 48: instr { frD = std::bit_cast<float>((u32)cpu.read(rA+SIMM16, 32)); }; // lfs	
	case 49: instr { frD = std::bit_cast<float>((u32)cpu.read(rA+SIMM16, 32)); rA+=SIMM16; }; // lfsu
	case 50: instr { frD = std::bit_cast<double>(cpu.read(rA+SIMM16, 64)); }; // lfd	
	case 51: instr { frD = std::bit_cast<double>(cpu.read(rA+SIMM16, 64)); rA+=SIMM16; }; // lfdu
	case 52: instr { cpu.write(zrA+SIMM16, std::bit_cast<u32>((float)frD), 32); }; // stfs
	case 53: instr { cpu.write(rA+SIMM16, std::bit_cast<u32>((float)frD), 32); rA+=SIMM16; }; // stfsu
	case 54: instr { cpu.write(zrA+SIMM16, std::bit_cast<u64>(frD), 64); }; // stfd
	case 55: instr { cpu.write(rA+SIMM16, std::bit_cast<u64>(frD), 64); rA+=SIMM16; }; // stfdu
	
	case 4:
	case 56:
	case 57:
	case 60:
	case 61: instr_nop; //todo: ps ld/st

	case 59: return decode_59(opcode);
	case 63: return decode_63(opcode);
	default:
		std::println("Gecko: Undef toplevel opc = ${:X}", opcode>>26);
		return nullptr;
	}
}

void gekko::step()
{
	if( msr.b.ee && irq_line )
	{
		SRR0 = pc;
		SRR1 = msr.v & MSR_TO_SRR_MASK;
		msr.b.ee = msr.b.ri = 0;
		pc = 0x500;
		//std::println("ExtIRQ not yet implemented");
		//exit(1);
	} else {
		u32 opc = read(pc, 32);
		pc += 4;
		
		auto I = decode_opcode(opc);
		if( !I )
		{
			std::println("Ended at ${:X}", pc-4);
			exit(1);
		}
		
		I(*this, opc);
	}
}

void gekko::reset()
{

}

#define OE (opc&BIT(10))
#define bOE BIT(9)

static gekko::instr_type decode_31(u32 opcode)
{
	switch( (opcode>>1)&0x3ff )
	{
	// starting with the instructions where the bit that would be OE is required to be 1(one)
	case 512: instr {  // mcrxr
			cpu.cond.cr[crfD].lt = cpu.xer.b.so;
			cpu.cond.cr[crfD].gt = cpu.xer.b.ov;
			cpu.cond.cr[crfD].eq = cpu.xer.b.ca;
			cpu.cond.cr[crfD].so = 0; //todo: or can that bit in xer have value?
			cpu.xer.v &= 0x0fffFFFFu; 
		};
		
	case 533: instr { // lswx
			u32 EA = zrA+rB;
			u32 i = 0;
			u32 r = (((opc>>21)&0x1F)-1)&0x1F;
			u32 n = cpu.xer.b.bc;
			while( n > 0 )
			{
				if( i == 0 )
				{
					r = (r + 1) & 0x1F;
					cpu.r[r] = 0;
				}
				cpu.r[r] |= ((u8)cpu.read(EA,8))<<(24-i);
				i += 8;
				if( i==32 ) { i=0; }
				EA += 1;
				n -= 1;
			}
		};
		
	case 534: instr { rD = __builtin_bswap32((u32)cpu.read(zrA+rB, 32)); }; // lwbrx
	
	case 535: instr { frD = std::bit_cast<float>((u32)cpu.read(zrA+rB,32)); if(cpu.hid2.b.pse) { frD_PS1 = frD; } }; // lfsx
	case 536: instr { rA = ((rB&BIT(5)) ? 0 : (rS>>(rB&0x1F))); if(Rc){cpu.setCR0(rA);} }; // srw

	case 567: instr { frD = std::bit_cast<float>((u32)cpu.read(rA+rB,32)); rA+=rB; if(cpu.hid2.b.pse) { frD_PS1 = frD; } }; // lfsx

	case 597: instr { // lswi
			u32 EA = zrA;
			u32 i = 0;
			u32 r = (((opc>>21)&0x1F)-1)&0x1F;
			u32 n = ((opc>>11)&0x1F);
			if( n == 0 ) { n = 32; }
			while( n > 0 )
			{
				if( i == 0 )
				{
					r = (r + 1) & 0x1F;
					cpu.r[r] = 0;
				}
				cpu.r[r] |= ((u8)cpu.read(EA,8))<<(24-i);
				i += 8;
				if( i==32 ) { i=0; }
				EA += 1;
				n -= 1;
			}
		};

	case 599: instr { frD = std::bit_cast<double>(cpu.read(zrA+rB, 64)); }; // lfdx
	case 631: instr { frD = std::bit_cast<double>(cpu.read(rA+rB, 64)); rA+=rB; }; // lfdux

	case 661: instr { // stswx
			u32 EA = zrA+rB;
			u32 i = 0;
			u32 r = (((opc>>21)&0x1F)-1)&0x1F;
			u32 n = cpu.xer.b.bc;
			while( n > 0 )
			{
				if( i == 0 ) { r = (r + 1) & 0x1F; }
				cpu.write(EA, cpu.r[r]>>(24-i), 8);
				i += 8;
				if( i==32 ) { i=0; }
				EA += 1;
				n -= 1;
			}
		};
	case 662: instr { cpu.write(zrA+rB, __builtin_bswap32(rS), 32); }; // stwbrx
	case 663: instr { cpu.write(zrA+rB, std::bit_cast<u32>((float)frS), 32); }; // stfsx

	case 695: instr { cpu.write(rA+rB, std::bit_cast<u32>((float)frS), 32); rA+=rB; }; // stfsux

	case 725: instr { // stswi
			u32 EA = zrA;
			u32 i = 0;
			u32 r = (((opc>>21)&0x1F)-1)&0x1F;
			u32 n = ((opc>>11)&0x1F);
			if( n == 0 ) { n = 32; }
			while( n > 0 )
			{
				if( i == 0 ) { r = (r + 1) & 0x1F; }
				cpu.write(EA, cpu.r[r]>>(24-i), 8);
				i += 8;
				if( i==32 ) { i=0; }
				EA += 1;
				n -= 1;
			}
		};

	case 727: instr { cpu.write(zrA+rB, std::bit_cast<u64>(frS), 64); }; // stfdx

	case 759: instr { cpu.write(rA+rB, std::bit_cast<u64>(frS), 64); rA+=rB; }; // stfdux

	case 790: instr { rD = __builtin_bswap16((u16)cpu.read(zrA+rB, 16)); }; // lhbrx

	case 792: instr { // sraw
			if( rB & BIT(5) )
			{
				cpu.xer.b.ca = (rS>>31)&1;
				rA = s32(rS)>>31;
			} else if( (rB&0x1F) == 0 ) {
				cpu.xer.b.ca = 0;
				rA = rS;
			} else {
				cpu.xer.b.ca = (rS&BIT(31)) && (rS&(BIT((rB&0x1F))-1));
				rA = s32(rS) >> (rB&0x1F);
			}
			if(Rc) { cpu.setCR0(rA); }
		};
	case 824: instr { // srawi
			if( SH == 0 )
			{
				cpu.xer.b.ca = 0;
				rA = rS;
			} else {
				cpu.xer.b.ca = (rS&BIT(31)) && (rS&(BIT(SH)-1));
				rA = s32(rS) >> SH;
			}
			if(Rc) { cpu.setCR0(rA); }
		};

	case 918: instr { cpu.write(zrA+rB, __builtin_bswap16((u16)rS), 16); }; // sthbrx

	case 922: instr { rA = (s16)rS; if(Rc) { cpu.setCR0(rA); } }; // extsh
	
	case 954: instr { rA = (s8)rS; if(Rc) { cpu.setCR0(rA); } }; // extsb
	
	case 983: instr { cpu.write(zrA+rB, (u32)std::bit_cast<u64>(frS), 32); }; // stfiwx
	
	case 566: instr_nop; // tlbsync
	case 598: instr_nop; // sync
	case 854: instr_nop; // eieio
	case 982: instr_nop; // icbi
	case 1014: instr_nop; // dcbz
	
	// all the instructions where bit10 is either OE or must be zero
	case 0: instr { //cmp
			cpu.cond.cr[crfD].lt = s32(rA)<s32(rB);
			cpu.cond.cr[crfD].gt = s32(rA)>s32(rB);
			cpu.cond.cr[crfD].eq = rA==rB;
			cpu.cond.cr[crfD].so = cpu.xer.b.so;
		};
	case bOE|8:
	case 8: instr { // subfc
			u32 t = ~rA;
			u64 res = t;
			res += rB;
			res += 1;
			if(OE) 
			{
				cpu.xer.b.ov = ((res^t)&(res^rB)&BIT(31))>>31;
				cpu.xer.b.so |= cpu.xer.b.ov;
			}
			rD = res;
			cpu.xer.b.ca = (res>>32)&1;
			if(Rc) { cpu.setCR0(rD); }
		};
	case bOE|10:
	case 10: instr { // addc
			u64 res = rA;
			res += rB;
			if(OE) 
			{
				cpu.xer.b.ov = (((res^rA)&(res^rB)&BIT(31))?1:0);
				cpu.xer.b.so |= cpu.xer.b.ov;
			}
			rD = res;
			cpu.xer.b.ca = ((res>>32)&1);
			if(Rc) { cpu.setCR0(rD); }		
		};	
	case 11: instr { u64 a = rA; a *= rB; rD=a>>32; if(Rc) { cpu.setCR0(rD); } }; // mulhwu

	case 19: instr { rD = cpu.cond.get(); }; // mfcr
	case 20: instr { rD = cpu.read(zrA+rB, 32); cpu.reserve = 1; }; // lwarx
	case 150: instr { // stwcx.
			cpu.cond.cr[0].lt = cpu.cond.cr[0].gt = 0;
			cpu.cond.cr[0].so = cpu.xer.b.so;
			if( cpu.reserve )
			{
				cpu.reserve = 0;
				cpu.write(zrA+rB, rS, 32);
				cpu.cond.cr[0].eq = 1;
			} else {
				cpu.cond.cr[0].eq = 0;
			}
		};

	case 23: instr { rD = cpu.read(zrA+rB, 32); }; // lwzx
	case 24: instr { rA = ((rB&BIT(5)) ? 0 : (rS<<(rB&0x1F))); if(Rc) { cpu.setCR0(rA); } }; //slw
	case 26: instr { rA = std::countl_zero(rS); if(Rc){cpu.setCR0(rA); }}; //cntlzw
	case 28: instr { rA = rS & rB; if(Rc) { cpu.setCR0(rA); } }; // and
	case 32: instr { //cmpl
			cpu.cond.cr[crfD].lt = u32(rA)<u32(rB);
			cpu.cond.cr[crfD].gt = u32(rA)>u32(rB);
			cpu.cond.cr[crfD].eq = rA==rB;
			cpu.cond.cr[crfD].so = cpu.xer.b.so;
		};
	case bOE|40:
	case 40: instr { // subf
			u32 t = ~rA;
			u64 res = t;
			res += rB;
			res += 1;
			if(OE) 
			{
				cpu.xer.b.ov = ((res^t)&(res^rB)&BIT(31))>>31;
				cpu.xer.b.so |= cpu.xer.b.ov;
			}
			rD = res;
			if(Rc) { cpu.setCR0(rD); }
		};

	case 55: instr { u32 EA=rA+rB; rA=EA; rD = cpu.read(EA, 32); }; // lwzux

	case 60: instr { rA = rS & ~rB; if(Rc) { cpu.setCR0(rA); }}; // andc

	case 75: instr { s64 a = s32(rA); a *= s32(rB); rD=a>>32; if(Rc) { cpu.setCR0(rD); } }; // mulhw

	case 83: instr { rD = cpu.msr.v; }; // mFmsr

	case 87: instr { rD = (u8)cpu.read(zrA+rB, 8); }; // lbzx
	case bOE|104:
	case 104: instr { // neg
			if( rA == BIT(31) ) 
			{ 
				rD = rA; if(OE) { cpu.xer.b.so=cpu.xer.b.ov=1; } 
			} else { 
				rD = ~rA + 1; if(OE) { cpu.xer.b.ov=0; }
			}
			if(Rc) { cpu.setCR0(rD); }
		};

	case 119: instr { u32 EA=rA+rB; rA=EA; rD = (u8)cpu.read(EA, 8); }; // lbzux

	case 124: instr { rA = ~(rS | rB); if(Rc) { cpu.setCR0(rA); } }; // nor
	case bOE|136:
	case 136: instr { // subfe
			u32 t = ~rA;
			u64 res = t;
			res += rB;
			res += cpu.xer.b.ca;
			if(OE)
			{
				cpu.xer.b.so |= (cpu.xer.b.ov = (((res^rB)&(res^t)&BIT(31))>>31));
			}
			rD = res;
			cpu.xer.b.ca = (res>>32)&1;
			if(Rc) { cpu.setCR0(rD); }
		};
	case bOE|138:
	case 138: instr { // adde
			u64 res = rA;
			res += rB;
			res += cpu.xer.b.ca;
			if(OE) 
			{
				cpu.xer.b.ov = (((res^rA)&(res^rB)&BIT(31))?1:0);
				cpu.xer.b.so |= cpu.xer.b.ov;
			}
			rD = res;
			cpu.xer.b.ca = ((res>>32)&1);
			if(Rc) { cpu.setCR0(rD); }		
		};
		
	case 144: instr { // mtcrf
			u32 crm = (opc>>12)&0xff;
			u32 v = cpu.cond.get();
			u32 mask = 0;
			for(u32 i = 0; i < 8; ++i) { mask |= ((crm&BIT(i)) ? (0xf<<((i)*4)) : 0); }
			v = (v&~mask) | (rS&mask);
			cpu.cond.set(v);	
		};
		
	case 146: instr { cpu.msr.v = rS; }; // mTmsr

	case 151: instr { cpu.write(zrA+rB, rS, 32); }; // stwux

	case 183: instr { cpu.write(rA+rB, rS, 32); rA+=rB; }; // stwux
	case bOE|200:
	case 200: instr { // subfze
			u32 t = ~rA;
			u64 res = t;
			res += cpu.xer.b.ca;
			if(OE) 
			{
				cpu.xer.b.ov = ((res^t)&(res^0)&BIT(31))>>31;
				cpu.xer.b.so |= cpu.xer.b.ov;
			}
			rD = res;
			cpu.xer.b.ca = ((res>>32)&1);
			if(Rc) { cpu.setCR0(rD); }
		};
	case bOE|202:
	case 202: instr { // addze
			u64 res = rA;
			res += cpu.xer.b.ca;
			if(OE) 
			{
				cpu.xer.b.ov = (((res^rA)&(res^0)&BIT(31))?1:0);
				cpu.xer.b.so |= cpu.xer.b.ov;
			}
			rD = res;
			cpu.xer.b.ca = ((res>>32)&1);
			if(Rc) { cpu.setCR0(rD); }		
		};
	case 215: instr { cpu.write(zrA+rB, rS, 8); }; // stbx
	case bOE|232:
	case 232: instr { // subfme
			u32 t = ~rA;
			u64 res = t;
			res += cpu.xer.b.ca;
			res += 0xffffFFFFu;
			if(OE)
			{
				cpu.xer.b.so |= (cpu.xer.b.ov = (((res^BIT(31))&(res^t)&BIT(31))>>31));
			}
			rD = res;
			cpu.xer.b.ca = (res>>32)&1;
			if(Rc) { cpu.setCR0(rD); }
		};
	case bOE|234:
	case 234: instr { // addme
			u64 res = rA;
			res += 0xffffFFFFu;
			res += cpu.xer.b.ca;
			if(OE) 
			{
				cpu.xer.b.ov = (((res^rA)&(res^BIT(31))&BIT(31))?1:0);
				cpu.xer.b.so |= cpu.xer.b.ov;
			}
			rD = res;
			cpu.xer.b.ca = ((res>>32)&1);
			if(Rc) { cpu.setCR0(rD); }		
		};
	case bOE|235:
	case 235: instr { // mullw
			s64 a = s32(rA); a*=s32(rB); rD=a; 
			if( OE )
			{
				cpu.xer.b.ov = ((a != s32(rD)) ? 1:0);
				cpu.xer.b.so |= cpu.xer.b.ov;
			}
			if(Rc) { cpu.setCR0(rD); }
		};

	case 247: instr { cpu.write(rA+rB, rS, 8); rA+=rB; }; // stbux

	case bOE|266:
	case 266: instr { // add
			u64 res = rA;
			res += rB;
			if(OE) 
			{
				cpu.xer.b.ov = (((res^rA)&(res^rB)&BIT(31))?1:0);
				cpu.xer.b.so |= cpu.xer.b.ov;
			}
			rD = res;
			if(Rc) { cpu.setCR0(rD); }		
		};
	case 279: instr { rD = (u16)cpu.read(zrA+rB, 16); }; // lhzx
		
	case 284: instr { rA = ~(rS^rB); if(Rc) { cpu.setCR0(rA); } }; // eqv
	case 311: instr { u32 EA=rA+rB; rA=EA; rD = (u16)cpu.read(EA, 16); }; // lhzux

	case 316: instr { rA = rS ^ rB; if(Rc) { cpu.setCR0(rA); } }; // xor

	case 339: instr { u32 spr = (opc>>11)&0x3ff; rD = cpu.get_spr( ((spr&0x1F)<<5) | ((spr>>5)&0x1F) ); }; // mfspr

	case 343: instr { rD = (s16)cpu.read(zrA+rB, 16); }; // lhax

	case 371: instr { u32 tbr = (opc>>11)&0x3ff; rD = cpu.get_tbr( ((tbr&0x1F)<<5) | ((tbr>>5)&0x1F) ); }; // mftb

	case 375: instr { u32 EA=rA+rB; rA=EA; rD = (s16)cpu.read(EA, 16); }; // lhaux

	case 407: instr { cpu.write(zrA+rB, rS, 16); }; // sthx
	case 412: instr { rA = rS | ~rB; if(Rc) { cpu.setCR0(rA); } }; // orc

	case 439: instr { cpu.write(rA+rB, rS, 16); rA+=rB; }; // sthux
	case 444: instr { rA = rS | rB; if(Rc) { cpu.setCR0(rA); } }; // or

	case bOE|459:
	case 459: instr { // divwu
			if( rB==0 )
			{
				// rD is probably deterministic, just dunno it
				if( OE ) { cpu.xer.b.so = cpu.xer.b.ov = 1; }
			} else {
				rD = u32(rA)/u32(rB);
				if( OE ) { cpu.xer.b.ov = 0; }
			}			
			if(Rc) { cpu.setCR0(rD); }
		};

	case 467: instr { u32 spr = (opc>>11)&0x3ff; cpu.set_spr( ((spr&0x1F)<<5) | ((spr>>5)&0x1F) , rS ); }; // mTspr

	case 476: instr { rA = ~(rS & rB); if(Rc) { cpu.setCR0(rA); } }; // nand
	
	case bOE|491:
	case 491: instr { // divw
			if( rB==0 || (rA==0x80000000u&&rB==0xffffFFFFu) )
			{
				// rD is probably deterministic, just don't know what
				if( OE ) { cpu.xer.b.so = cpu.xer.b.ov = 1; }
			} else {
				rD = s32(rA)/s32(rB);
				if( OE ) { cpu.xer.b.ov = 0; }
			}			
			if(Rc) { cpu.setCR0(rD); }
		};
	
	case 54: instr_nop; // dcbst
	case 86: instr_nop; // dcbf
	case 246: instr_nop; // dcbtst
	case 278: instr_nop; // dcbt
	case 470: instr_nop; // dcbi
	//anything >=512 belongs in the first switch
	default:
		std::println("Unimpl opc31 bot10 = {}dec", (opcode>>1)&0x3ff);
		break;
	}	
	//return nullptr;
	instr_nop;
}

u32 gekko::get_spr(u32 spr)
{
	switch( spr )
	{
	case 1: return xer.v;
	case 8: return LR;
	case 9: return CTR;
	
	case 920: return hid2.v;
	default: break;
	}
	std::println("gekko: unimpl spr = {}", spr);
	return SPRs[spr];
}

void gekko::set_spr(u32 spr, u32 v)
{
	switch( spr )
	{
	case 1: xer.v = v; return;
	case 8: LR = v; return;
	case 9: CTR = v; return;
	
	case 920: hid2.v = v; return;
	default: break;
	}
	SPRs[spr] = v;
	std::println("gekko: unimpl spr = {}", spr);
}














