#include <cstdlib>
#include <cstring>
#include <bit>
#include <print>
#include "gekko.h"

typedef void (*gekko_instr)(gekko&, u32);
#define nopped [](gekko&, u32){}

#define instr [](gekko& cpu, u32 opc) 

#define rD cpu.r[(opc>>21)&0x1f]
#define fD cpu.f[(opc>>21)&0x1f]
#define rA cpu.r[(opc>>16)&0x1f]
#define rB cpu.r[(opc>>11)&0x1f]
#define fB cpu.f[(opc>>11)&0x1f]
#define A0 ( ((opc>>16)&0x1f) ? cpu.r[(opc>>16)&0x1f] : 0 )
#define simm16 ( (s16)(opc&0xffff) )
#define carry cpu.xer.b.CA
#define OE (opc&BIT(10))  


static u32 crmasks[] = { 0xf0000000u, 0x0f000000u, 0x00f00000u, 0xf0000u, 0xf000u, 0xf00u, 0xf0u, 0xfu };

gekko_instr decode_31(u32 opcode)
{
	const u32 btm10 = (opcode>>1)&0x3ff;
	
	switch( btm10 )
	{
	case 0: return instr {
			u32 c = cpu.xer.b.SO;
			if( s32(rA) < s32(rB) )
			{
				c |= 8;
			} else if( s32(rA) > s32(rB) ) {
				c |= 4;
			} else {
				c |= 2;
			}
			u32 crfD = (opc>>(31-8))&7;
			cpu.cr.v &= ~crmasks[crfD];
			cpu.cr.v |= (c << ((crfD^7)*4));
		}; // cmp
	case 32: return instr {
			u32 c = cpu.xer.b.SO;
			if( rA < rB )
			{
				c |= 8;
			} else if( rA > rB ) {
				c |= 4;
			} else {
				c |= 2;
			}
			u32 crfD = (opc>>(31-8))&7;
			cpu.cr.v &= ~crmasks[crfD];
			cpu.cr.v |= (c << ((crfD^7)*4));
		}; // cmpl
	case 19: return instr { rD = cpu.cr.v; }; // mfcr
	
	case 11: return instr { // mulhwu/.
			u64 a = rA;
			a *= rB;
			rD = a>>32;
			if( opc & 1 ) { cpu.crlog(rD); }
		};
	
	case 23: return instr { u32 EA = A0+rB; rD = cpu.read(EA,32); }; // lwzx
	
	case 24: return instr {
			const u32 sh = rB&0x3F;
			if( sh > 31 )
			{
				rA = 0;
			} else {
				rA = rD << sh;
			}
			if( opc&1 ) { cpu.crlog(rA); }	
		}; // slw/.
	
	case 28: return instr { rA = rD & rB; if( opc & 1 ) { cpu.crlog(rA); } }; // and/.
	case 60: return instr { rA = rD & ~rB; if( opc & 1 ) { cpu.crlog(rA); } }; // andc/.
	
	case 75: return instr { rD = (s64(s32(rA)) * s64(s32(rB)))>>32; if(opc&1){cpu.crlog(rD);} }; // mulhw/.
	
	case 83: return instr { rD = cpu.msr.v; }; // mfmsr
	
	case 87: return instr { u32 EA = A0 + rB; rD = cpu.read(EA, 8); }; // lbzx
	
	case 104|BIT(9):
	case 104: return instr { // neg/o.
			if( rA == 0x80000000u )
			{
				rD = rA;
				if( OE ) { cpu.xer.b.SO = cpu.xer.b.OV = 1; }
			} else {
				rD = 1 + ~rA;			
				if( OE ) { cpu.xer.b.OV = 0; }
			}
			if( opc & 1 ) { cpu.crlog(rD); }
		};
	
	case 119: return instr { u32 EA = rA + rB; rD = cpu.read(EA, 8); rA = EA; }; // lbzux
	
	case 136: return instr { // subfe/o.
			u64 t = (rA ^ 0xffffFFFFu);
			t += rB;
			t += carry;
			if( OE )
			{
				cpu.xer.b.OV = (((t^rB) & (t^rA) & BIT(31))?1:0);
				cpu.xer.b.SO |= cpu.xer.b.OV;
			}
			rD = t;
			if( opc&1 ) { cpu.crlog(rD); }
		};
	
	case 144: return instr { // mtcrf
			const u32 crm = (opc>>12)&0xff;
			u32 mask = 0;
			for(u32 i = 0; i < 8; ++i)
			{
				if( crm & (1u<<i) )
				{
					mask |= 0xf<<(i*4);
				}
			}
			cpu.cr.v = (rD & mask) | (cpu.cr.v & ~mask);
		};
	
	case 151: return instr { cpu.write(A0+rB, rD, 32); }; // stwx

	case 124: return instr { rA = ~(rD | rB);  if( opc & 1 ) { cpu.crlog(rA); } }; // nor/.
	case 444: return instr { rA =  (rD | rB);  if( opc & 1 ) { cpu.crlog(rA); } }; // or/.
	case 412: return instr { rA =  (rD | ~rB); if( opc & 1 ) { cpu.crlog(rA); } }; // orc/.
	
	case 316: return instr { rA = rD ^ rB; if( opc & 1) { cpu.crlog(rA); } }; // xor/.
	
	case 146: return instr { cpu.msr.v = rD; cpu.msr.b.pad0 = cpu.msr.b.pad1 = cpu.msr.b.pad2 = 0; }; // mtmsr
	
	case 210: return nopped; // todo: mtsr
	
	case 26: return instr { // cntlzw/.
			rA = std::countl_zero(rD);
			if( opc & 1 ) { cpu.crlog(rA); }
		};

	case 40|BIT(9): // fallthru
	case 40: return instr { // subf/o.
			u64 t = rA ^ 0xffffFFFFu;
			t += rB;
			t += 1;
	//std::println("${:X}: r{}({}) = r{}({}) - r{}({})", cpu.pc-4, (opc>>21)&0x1f, u32(t), (opc>>16)&0x1f, rA, (opc>>11)&0x1f, rB);
			if( OE )
			{
				cpu.xer.b.OV = (( (t^rB) & (t^rA) & BIT(31) ) ? 1 : 0);
				cpu.xer.b.SO |= cpu.xer.b.OV;
			}
			rD = t;
			if( opc & 1 ) { cpu.crlog(rD); }	
		};
		
	case 202|BIT(9): // fallthru
	case 202: return instr { // addze/o.
			u64 t = rA;
			t += carry;
			if( opc & BIT(9) )
			{
				cpu.xer.b.OV = (( (t^0) & (t^rA) & BIT(31) ) ? 1 : 0);
				cpu.xer.b.SO |= cpu.xer.b.OV;
			}
			carry = t>>32;
			rD = t;
			if( opc & 1 ) { cpu.crlog(rD); }	
		}; 

	case 266|BIT(9): // fallthru
	case 266: return instr { // add/o.
			u64 t = rA;
			t += rB;
			if( OE )
			{
				cpu.xer.b.OV = (( (t^rB) & (t^rA) & BIT(31) ) ? 1 : 0);
				cpu.xer.b.SO |= cpu.xer.b.OV;
			}
			rD = t;
			if( opc & 1 ) { cpu.crlog(rD); }	
		}; 
	case 10|BIT(9): // fallthru
	case 10: return instr { // addc/o.
			u64 t = rA;
			t += rB;
			if( OE )
			{
				cpu.xer.b.OV = (( (t^rB) & (t^rA) & BIT(31) ) ? 1 : 0);
				cpu.xer.b.SO |= cpu.xer.b.OV;
			}
			cpu.xer.b.CA = t>>32;
			rD = t;
			if( opc & 1 ) { cpu.crlog(rD); }	
		};
	case 138|BIT(9): // fallthru
	case 138: return instr { // adde/o.
			u64 t = rA;
			t += rB;
			t += cpu.xer.b.CA;
			if( OE )
			{
				cpu.xer.b.OV = (( (t^rB) & (t^rA) & BIT(31) ) ? 1 : 0);
				cpu.xer.b.SO |= cpu.xer.b.OV;
			}
			cpu.xer.b.CA = t>>32;
			rD = t;
			if( opc & 1 ) { cpu.crlog(rD); }	
		};
		
	case 8|BIT(9):
	case 8: return instr { // subfc/o.
			u64 t = rA^0xffffFFFFu;
			t += rB;
			t += 1;
			if( OE )
			{
				cpu.xer.b.OV = (( (t^rB) & (t^rA) & BIT(31) ) ? 1 : 0);
				cpu.xer.b.SO |= cpu.xer.b.OV;
			}
			carry = t>>32;
			rD = t;
			if( opc&1 ) { cpu.crlog(rD); }
		};


	case 339: return instr { u32 spr = (opc>>11)&0x3ff; spr = ((spr<<5)&0x3ff)|(spr>>5); rD = cpu.read_spr(spr); }; // mfspr
	case 467: return instr { u32 spr = (opc>>11)&0x3ff; spr = ((spr<<5)&0x3ff)|(spr>>5); cpu.write_spr(spr, rD); }; // mtspr

	case 371: return instr { // mftb
			u32 spr = (opc>>11)&0x3ff; spr = ((spr<<5)&0x3ff)|(spr>>5);
			if( spr == 268 )
			{
				rD = cpu.time_base;
			} else if( spr == 269 ) {
				rD = cpu.time_base>>32;
			}	
		};

	case 459|BIT(9):
	case 459: return instr {
			if( OE )
			{
				cpu.xer.b.OV = ((rB==0)?1:0);
				cpu.xer.b.SO |= cpu.xer.b.OV;
			}
			if( rB == 0 ) { if( opc & 1 ) { cpu.cr.b.SO = cpu.xer.b.SO; } return; }
			rD = rA / rB;
			if( opc & 1 ) { cpu.crlog(rD); }
		}; // divwu/o.
		
	case 235|BIT(9):
	case 235: return instr {
			s64 res = s32(rA);
			res *= s32(rB);
			rD = res;
			if( OE )
			{
				u32 top = res>>32;
				cpu.xer.b.OV = (top==0 && !(rD&BIT(31)))||(top==0xffffFFFFu && (rD&BIT(31)));
				cpu.xer.b.SO |= cpu.xer.b.OV;
			}
			if( opc&1 ) { cpu.crlog(rD); }
		}; // mullw/o.
	
	case 279: return instr { // lhzx
			u32 EA = A0 + rB;
			rD = cpu.read(EA, 16);	
		};

	case 536: return instr {
			rA = rD >> (rB & 0x1f);
			if( opc & 1 ) { cpu.crlog(rA); }
		}; // srw/.
	
	case 824: return instr {
			const u32 SH = (opc>>11)&0x1f;
			if( SH == 0 )
			{
				carry = 0;
				rA = rD;
				return;
			}
			const u32 bitsout = rD & ((1ull<<(SH+1))-1);
			bool neg = rD & 1;
			rA = s32(rD) >> SH;
			carry = (bitsout && neg);
			cpu.crlog(rA);
		}; // srawi
	
	case 86: return nopped; // dcbf
	case 470: return nopped; // dcbi
	case 598: return nopped; // sync
	
	case 792: return instr {
			const u32 SH = rB & 0x3F;
			if( SH == 0 )
			{
				carry = 0;
				rA = rD;
				if( opc&1 ) { cpu.crlog(rA); }
				return;
			} else if( SH > 31 ) {
				carry = rD>>31;
				rA = ((rD&BIT(31)) ? 0xffffFFFFu : 0);
				if( opc&1 ) { cpu.crlog(rA); }
				return;
			}
			const u32 bitsout = rD & ((1ull<<(SH+1))-1);
			bool neg = rD & 1;
			rA = s32(rD) >> SH;
			carry = (bitsout && neg);
			cpu.crlog(rA);
		}; // sraw/.
	
	case 982: return nopped; // icbi
	default: std::println("In decode_31: btm10 = {}", btm10); break;
	}
	return nullptr;
}

gekko_instr decode_19(u32 opcode)
{
	const u32 btm10 = (opcode >> 1) & 0x3ff;
	
	switch( btm10 )
	{
	
	case 16: return instr {
			const u32 BO = (opc>>21)&0x1f;
			const u32 BI = 31 - ((opc>>16)&0x1f);
			if( !(BO & 4) ) cpu.ctr -= 1;
			//std::println("ctr = ${:X}", cpu.ctr);
			bool ctr_ok = (BO&4) || ((cpu.ctr!=0) ^ ((BO&2)?1:0));
			bool cond_ok = (BO&0x10) || (((cpu.cr.v>>BI)&1) ^ (((BO>>3)&1)^1));
			if( ctr_ok && cond_ok )
			{
				if( opc & 1 ) { std::println("did a link in a blr!"); exit(1); cpu.lr = cpu.pc; }
				cpu.pc = cpu.lr & ~3;			
			}
			//std::println("blr to ${:X}", cpu.pc);
		}; // blr
	
	case 50: return instr {
			(void)opc;
			std::println("${:X} rfi", cpu.pc-4);
			cpu.pc = cpu.srr0;
			cpu.msr.v = cpu.srr1;
		};
	
	case 150: return nopped; // isync
	
	case 193: return instr {
			const u32 crbD = 31 - ((opc>>21)&0x1f);
			const u32 crbA = 31 - ((opc>>16)&0x1f);
			const u32 crbB = 31 - ((opc>>11)&0x1f);
			cpu.cr.v &= ~(1u<<crbD);
			cpu.cr.v |= (((cpu.cr.v>>crbA)&1)^((cpu.cr.v>>crbB)&1))<<crbD;
		}; // crxor
		
	case 289: return instr {
			const u32 crbD = 31 - ((opc>>21)&0x1f);
			const u32 crbA = 31 - ((opc>>16)&0x1f);
			const u32 crbB = 31 - ((opc>>11)&0x1f);
			cpu.cr.v &= ~(1u<<crbD);
			cpu.cr.v |= (1^(((cpu.cr.v>>crbA)&1)^((cpu.cr.v>>crbB)&1)))<<crbD;
		}; // creqv
		
		
	case 528: return instr {
			const u32 BO = (opc>>21)&0x1f;
			const u32 BI = 31 - ((opc>>16)&0x1f);
			const bool cond_ok = (BO&0x10)||(1^(((cpu.cr.v>>BI)&1) ^ ((BO>>3)&1)));
			if( cond_ok )
			{
				if( opc&1 ) cpu.lr = cpu.pc;
				cpu.pc = cpu.ctr&~3;
			}
		}; // bcctr
	default: std::println("In decode_19: btm10 = {}", btm10); break;
	}
	return nullptr;
}

gekko_instr decode_63(u32 opcode)
{
	u32 btm10 = (opcode >> 1) & 0x3ff;
	
	switch( btm10 )
	{
	case 12: return instr {
			fD = (double)(float)fB;
		}; // frsp todo: actual rounding modes and flags
	
	case 38: return nopped; //todo: mtfsb1
	case 70: return nopped; //todo: mtfsb0
	
	case 72: return instr { fD = fB; }; // fmr todo: flags
	
	
	case 711: return nopped; // todo mtfsf
	default: std::println("In decode_63: btm10 = {}", btm10); break;
	}
	return nullptr;
}

gekko_instr decode(u32 opcode)
{
	const u32 top6 = opcode >> 26;
	
	switch( top6 )
	{
	case 4: return nopped; // todo: bunch of paired single math
	
	
	case 7: return instr { rD = rA * simm16; }; // mulli
	case 8: return instr{u64 t=(rA^0xffffFFFFu);t+=(u32)(s32)simm16;t+=1;carry=t>>32;rD=t; };//subfic
	case 10: return instr {
			u32 c = cpu.xer.b.SO;
			if( rA < (opc&0xffff) )
			{
				c |= 8;
			} else if( rA > (opc&0xffff) ) {
				c |= 4;
			} else {
				c |= 2;
			}
			u32 crfD = (opc>>(31-8))&7;
			cpu.cr.v &= ~crmasks[crfD];
			cpu.cr.v |= (c << ((crfD^7)*4));
		}; // cmpli
	case 11: return instr {
			u32 c = cpu.xer.b.SO;
			if( s32(rA) < simm16 )
			{
				c |= 8;
			} else if( s32(rA) > simm16 ) {
				c |= 4;
			} else {
				c |= 2;
			}
			u32 crfD = (opc>>(31-8))&7;
			cpu.cr.v &= ~crmasks[crfD];
			cpu.cr.v |= (c << ((crfD^7)*4));
		}; // cmpi
	case 12: return instr { u64 t = rA; t += (u32)(s32)simm16; carry = t>>32; rD = t; }; // addic
	case 13: return instr { u64 t = rA; t += (u32)(s32)simm16; carry = t>>32; rD = t; cpu.crlog(rD); }; // addic.
	case 14: return instr { rD = A0 + simm16; };    // addi
	case 15: return instr { rD = A0 + (opc<<16); }; // addis
	case 16: return instr {
			const u32 BO = (opc>>21)&0x1f;
			const u32 BI = 31 - ((opc>>16)&0x1f);
			if( !(BO & 4) ) cpu.ctr -= 1;
			//std::println("ctr = ${:X}", cpu.ctr);
			bool ctr_ok = (BO&4) || ((cpu.ctr!=0) ^ ((BO&2)?1:0));
			bool cond_ok = (BO&0x10) || (((cpu.cr.v>>BI)&1) ^ (((BO>>3)&1)^1));
			if( ctr_ok && cond_ok )
			{
				if( opc & 1 ) { cpu.lr = cpu.pc; }
				cpu.pc = ((opc&2) ? 0 : (cpu.pc-4)) + s16(opc&0xfffc);			
			}
			//std::println("bcc to ${:X}", cpu.pc);
		}; // bc
	case 17: return nopped; // sc todo: actually do system call?
	case 18: return instr {
			//std::println("b opcode = ${:X}", opc);
			if( opc & 1 ) { cpu.lr = cpu.pc; }
			s32 soff = (opc&~3)<<6;
			soff >>= 6;
			cpu.pc = ( (opc&2) ? 0 : (cpu.pc-4) ) + soff;
			//std::println("branch to ${:X}", cpu.pc);
		}; // b
	case 19: return decode_19(opcode);
	case 20: return instr {
			u32 SH = (opc>>11)&0x1f;
			//std::println("SH = {}", SH);
			u32 r = std::rotl(rD, SH);
			u32 ME = (opc>>1)&0x1f;
			u32 MB = (opc>>6)&0x1f;
			//std::println("ME = {} | MB = {}", ME, MB);
			u32 m = 0;
			while( MB != ME )
			{
				m |= 1<<(31-MB);
				MB = (MB+1)&31;
			}
			m |= 1<<(31-MB);
			r = (r & m) | (rA & ~m);
			//std::println("rD = ${:X}, r = ${:X}, m = ${:X}", rD, r, m);
			rA = r;
			if( opc & 1 ) { cpu.crlog(rA); }
		}; // rlwimi	
	case 21: return instr {
			u32 SH = (opc>>11)&0x1f;
			//std::println("SH = {}", SH);
			u32 r = std::rotl(rD, SH);
			u32 ME = (opc>>1)&0x1f;
			u32 MB = (opc>>6)&0x1f;
			//std::println("ME = {} | MB = {}", ME, MB);
			u32 m = 0;
			while( MB != ME )
			{
				m |= 1<<(31-MB);
				MB = (MB+1)&31;
			}
			m |= 1<<(31-MB);
			r &= m;
			//std::println("rD = ${:X}, r = ${:X}, m = ${:X}", rD, r, m);
			rA = r;
			if( opc & 1 ) { cpu.crlog(rA); }
			//exit(1);
		}; // rlwinm
	
	case 24: return instr { rA = rD | (opc&0xffff); }; // ori
	case 25: return instr { rA = rD | (opc<<16); };    // oris
	case 26: return instr { rA = rD ^ (opc&0xffff); }; // xori
	case 27: return instr { rA = rD ^ (opc<<16); };    // xoris
	case 28: return instr { rA = rD & (opc&0xffff); cpu.crlog(rA); }; // andi.
	case 29: return instr { rA = rD & (opc<<16); cpu.crlog(rA); }; // andis.
	
	case 31: return decode_31(opcode);
	case 32: return instr { u32 EA = A0 + simm16; rD = cpu.read(EA, 32); }; //lwz
	case 33: return instr { u32 EA = rA + simm16; rD = cpu.read(EA, 32); rA = EA; }; // lwzu
	case 34: return instr { u32 EA = A0 + simm16; rD = cpu.read(EA, 8); }; // lbz
	case 35: return instr { u32 EA = rA + simm16; rD = cpu.read(EA, 8); rA = EA; }; // lbzu
	case 36: return instr { u32 EA = A0 + simm16; cpu.write(EA, rD, 32); }; // stw
	case 37: return instr { u32 EA = rA + simm16; cpu.write(EA, rD, 32); rA = EA; }; // stwu
	case 38: return instr { u32 EA = A0 + simm16; cpu.write(EA, rD&0xff, 8); }; // stb
	case 39: return instr { u32 EA = rA + simm16; cpu.write(EA, rD&0xff, 8); rA = EA; }; // stbu
	case 40: return instr { u32 EA = A0 + simm16; rD = cpu.read(EA, 16); }; // lhz
	case 41: return instr { u32 EA = rA + simm16; rD = cpu.read(EA, 16); rA = EA; }; // lhzu
	case 42: return instr { u32 EA = A0 + simm16; rD = (s16)cpu.read(EA, 16); }; // lha
	case 43: return instr { u32 EA = rA + simm16; rD = (s16)cpu.read(EA, 16); rA = EA; }; // lha
	case 44: return instr { u32 EA = A0 + simm16; cpu.write(EA, rD&0xffff, 16); }; // sth
	case 45: return instr { u32 EA = rA + simm16; cpu.write(EA, rD&0xffff, 16); rA = EA; }; // sthu
	case 46: return instr{ u32 EA =A0+simm16; u32 rg= (opc>>21)&0x1f; while( rg<32 ){cpu.r[rg] =cpu.read(EA,32); rg++; EA+=4;}};//lmw 
	case 47: return instr{ u32 EA =A0+simm16; u32 rg= (opc>>21)&0x1f; while( rg<32 ){cpu.write(EA,cpu.r[rg],32); rg++; EA+=4;}};//lmw 

	case 50: return instr { 
			u32 EA = A0+simm16; u64 t = cpu.read(EA,32); t<<=32; t|=cpu.read(EA+4,32); memcpy(&cpu.f[(opc>>21)&0x1f], &t, 8);
			//std::println("loaded f{} = {}", (opc>>21)&0x1f, cpu.f[(opc>>21)&0x1f]);
		}; // lfd
	
	case 52: return instr {
			u32 EA = A0+simm16; u32 v = 0; float vf = fD; memcpy(&v,&vf,4); cpu.write(EA, v, 4);	
		}; // stfs
						
	case 54: return instr {
			u32 EA = A0+simm16; cpu.writed(EA, fD);	
		}; // stfd
			
	case 56: return nopped; // todo psq_l
	
	case 63: return decode_63(opcode);
	default: std::println("In decode: undef opcode top6 = {}", top6); break;
	}
	return nullptr;
}

void gekko::step()
{
	time_base += 1;
	
	if( irq_line && msr.b.EE )
	{
		srr1 = msr.v;
		msr.b.EE = 0;
		srr0 = pc;
		pc = 0x00000500u;
	}

	u32 opcode = fetcher(pc);
	//std::println("${:X} = ${:X}", pc, opcode);
	pc += 4;
	
	auto i = decode(opcode);
	if( i )
	{
		i(*this, opcode);
	} else {
		std::println("${:X}: Undef opcode. halting", pc-4, opcode);
		exit(1);
	}
}

u32 gekko::read_spr(u32 spr)
{
	//std::println("Read spr {}", spr);
	switch( spr )
	{
	case 1: return xer.v;
	case 8: return lr;
	case 9: return ctr;
	case 26: return srr0;
	case 272: return sprg0;
	case 273: return sprg1;
	case 274: return sprg2;
	case 275: return sprg3;
	
	case 912:
	case 913:
	case 914:
	case 915:
	case 916:
	case 917:
	case 918: // gqr
	case 919: return 0;
	case 920: return hid2.v;
	case 1008: return hid0.v;
	case 1009: return hid1.v;
	case 1017: { u32 t = l2cr; l2cr &= ~1; std::println("${:X}: rd spr 1017", pc-4); return t; }
	default:
		std::println("read spr {} unimpl.", spr);
		//exit(1);
	}
	return 0;
}

void gekko::write_spr(u32 spr, u32 val)
{
	switch( spr )
	{
	case 1: xer.v = val; return;
	case 8: lr = val; return;
	case 9: ctr = val; /*std::println("set ctr = ${:X}", val);*/ return;
	case 26: srr0 = val; return;
	
	case 272: sprg0 = val; return;
	case 273: sprg1 = val; return;
	case 274: sprg2 = val; return;
	case 275: sprg3 = val; return;
	
	case 912:
	case 913:
	case 914:
	case 915:
	case 916:
	case 917:
	case 918: // gqr
	case 919: return;
	case 920: hid2.v = val; return;
	case 1008: hid0.v = val; return;
	case 1009: hid1.v = val; return;
	case 1017: l2cr = val&~1; if( l2cr & 0x200000 ) { l2cr |= 1; } return;
	default:
		std::println("${:X}: write spr {} = ${:X} unimpl.", pc-4, spr, val);
		//exit(1);
	}
	return;
}

u32 gekko::fetch(u32 /*addr*/)
{
	return 0;
}

u32 gekko::read(u32 addr, int size)
{
	return reader(addr,size);
}

void gekko::write(u32 addr, u32 v, int size)
{
	writer(addr,v,size);
}

void gekko::reset()
{
	l2cr = 0;
	irq_line = false;
}






