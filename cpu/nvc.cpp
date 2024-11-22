#include <cstdio>
#include <cstdlib>
#include "nvc.h"

void nvc::exec(u16 iw)
{
	if( (iw>>13) == 4 )
	{
		if( isCond((iw>>9)&15) )
		{
			icyc = 3;
			pc -= 2;
			iw &= 0x1ff;
			if( iw & BIT(8) ) iw |= ~0x1ff;
			pc += s16(iw);
			pc &= ~1;	
		}
		return;
	}

	const u32 reg2 = (iw>>5)&0x1f;
	const u32 reg1 = iw&0x1f;
	
	switch( iw>>10 )
	{
	// moves
	case 0b000000: r[reg2] = r[reg1]; break;
	case 0b010000: r[reg2] = ((reg1&0x10) ? (reg1|~0x1f) : reg1); break;
	case 0b101000: r[reg2] = r[reg1] + s16(read(pc, 16)); pc += 2; break;
	case 0b101111: r[reg2] = r[reg1] + (read(pc, 16)<<16); pc += 2; break;
	
	// io
	case 0b111000: r[reg2] = read(r[reg1] + s16(read(pc,16)), 8); pc+=2; break;
	case 0b111001: r[reg2] = read(r[reg1] + s16(read(pc,16)), 16); pc+=2; break;
	case 0b111011: r[reg2] = read(r[reg1] + s16(read(pc,16)), 32); pc+=2; break;
	case 0b110000: r[reg2] = (s32)(s8)read(r[reg1] + s16(read(pc,16)), 8); pc+=2; break;
	case 0b110001: r[reg2] = (s32)(s16)read(r[reg1] + s16(read(pc,16)), 16); pc+=2; break;
	case 0b110011: r[reg2] = read(r[reg1] + s16(read(pc,16)), 32); pc+=2; break;
	case 0b110100:
	case 0b111100: write(r[reg1] + s16(read(pc,16)), r[reg2], 8); pc+=2; break;
	case 0b110101:
	case 0b111101: write(r[reg1] + s16(read(pc,16)), r[reg2], 16); pc+=2; break;
	case 0b111111:
	case 0b110111: write(r[reg1] + s16(read(pc,16)), r[reg2], 32); pc+=2; break;
	
	// integer math
	case 0b010001: r[reg2] = add(r[reg2], ((reg1&0x10) ? (reg1|~0x1f) : reg1)); break;
	case 0b000001: r[reg2] = add(r[reg2], r[reg1]); break;
	case 0b101001: r[reg2] = add(r[reg1], (s32)s16(read(pc,16))); pc+=2; break;
	case 0b010011: add(r[reg2], ~((reg1&0x10) ? (reg1|~0x1f) : reg1), 1); break;
	case 0b000011: add(r[reg2], ~r[reg1], 1); break;
	case 0b000010: r[reg2] = add(r[reg2], ~r[reg1], 1); break;
	case 0b001000:{
		icyc = 13;
		s64 res = s32(r[reg2]);
		res *= s32(r[reg1]);
		setsz(res);
		setov(res != (s64)(s32)u32(res));
		r[reg2] = res;
		r[30] = res>>32;
		} break;
	case 0b001010:{
		icyc = 13;
		u64 res = r[reg2];
		res *= r[reg1];
		setsz(res);
		setov(res != (u64)u32(res));
		r[reg2] = res;
		r[30] = res>>32;	
		} break;
	case 0b001001:
		icyc = 38;
		if( r[reg2] == 0x80000000 && r[reg1] == 0xffffFFFFu )
		{
			setov(true);
			r[30] = 0;
		} else {
			setov(false);
			r[reg2] = s32(r[reg2]) / s32(r[reg1]);
			r[30] = s32(r[reg2]) % s32(r[reg1]);
		}
		setsz(r[reg2]);
		break;
	case 0b001011:
		icyc = 36;
		setov(false);
		r[reg2] /= r[reg1];
		r[30] = r[reg2] % r[reg1];
		setsz(r[reg2]);
		break;
		
	// setf
	case 0b010010: r[reg2] = ( isCond(reg1&0xf) ? 1 : 0 ); break;
	
	// control
	case 0b010110: PSW &=~BIT(12); icyc = 12; break;
	case 0b011110: PSW |= BIT(12); icyc = 12; break;
	case 0b000110: pc = r[reg1]; icyc = 3; break;
	case 0b011100: icyc = 8; sys[reg1] = r[reg2]; printf("sysreg %i = $%X\n", reg1, r[reg2]); break; //todo: some are read-only
	case 0b011101: icyc = 8; r[reg2] = sys[reg1]; break;
	case 0b101010:{
		icyc = 3;
		iw &= 0x3ff;
		if( iw & BIT(9) ) iw |= ~0x3ff;
		u32 disp = (iw<<16)|read(pc, 16);
		pc -= 2;
		pc += disp;
		printf("relative branch to $%X\n", pc);
		} break;
	case 0b101011:{
		icyc = 3;
		iw &= 0x3ff;
		if( iw & BIT(9) ) iw |= ~0x3ff;
		u32 disp = (iw<<16)|read(pc, 16);
		pc -= 2;
		r[31] = pc + 4;
		pc += disp;
		printf("relative blink to $%X\n", pc);
		} break;
	case 0b011010: halted = true; break;

	// logical
	case 0b000100: setcy(((u64(r[reg2])<<(r[reg1]&0x1f))>>32)&1); r[reg2] <<= (r[reg1]&0x1f); setsz(r[reg2]); setov(0); break;
	case 0b010100: setcy(((u64(r[reg2])<<reg1)>>32)&1); r[reg2] <<= reg1; setsz(r[reg2]); setov(0); break;
	case 0b010111: if( reg1 ) { setcy((r[reg2]>>(reg1-1))&1); r[reg2] = s32(r[reg2])>>reg1; } setsz(r[reg2]); setov(0); break;
	case 0b010101: if( reg1 ) { setcy((r[reg2]>>(reg1-1))&1); r[reg2] >>= reg1; } setsz(r[reg2]); setov(0); break;
	case 0b101101: r[reg2] = r[reg1] & read(pc,16); pc+=2; setsz(r[reg2]); setov(0); break;
	case 0b101100: r[reg2] = r[reg1] | read(pc,16); pc+=2; setsz(r[reg2]); setov(0); break;
	case 0b101110: r[reg2] = r[reg1] ^ read(pc,16); pc+=2; setsz(r[reg2]); setov(0); break;
	case 0b001101: r[reg2] &= r[reg1]; setsz(r[reg2]); setov(0); break;
	case 0b001100: r[reg2] |= r[reg1]; setsz(r[reg2]); setov(0); break;
	case 0b001110: r[reg2] ^= r[reg1]; setsz(r[reg2]); setov(0); break;
	case 0b001111: r[reg2] = ~r[reg1]; setsz(r[reg2]); setov(0); break;
	
	// fp
	case 0b111110: iw = read(pc, 16); pc += 2; exec_fp(reg2, reg1, iw); break;
	
	default: printf("NVC: Unimpl 6bit opcode $%X\n", iw>>10); exit(1);
	}
}

void nvc::exec_fp(const u32 reg2, const u32 reg1, u16 iw)
{
	switch( iw >> 10 )
	{
	case 0b000100: f(reg2, f(reg2) + f(reg1)); setsz(r[reg2]); setov(0); setcy(PSW&2); break;
	case 0b000101: f(reg2, f(reg2) - f(reg1)); setsz(r[reg2]); setov(0); setcy(PSW&2); break;
	case 0b000110: f(reg2, f(reg2) * f(reg1)); setsz(r[reg2]); setov(0); setcy(PSW&2); break;
	case 0b000111: f(reg2, f(reg2) / f(reg1)); setsz(r[reg2]); setov(0); setcy(PSW&2); break;
	
	case 0b000010: f(reg2, r[reg1]); setsz(r[reg2]); setov(0); setcy(PSW&2); break;
	case 0b000011: r[reg2] = f(reg1); setsz(r[reg2]); setov(0); break;
	
	
	case 0b001100: icyc = 9; r[reg2] = s32(r[reg2]) * (s32)((s16(r[reg1])<<15)>>15); break;
	default:
		printf("NVC: Unimpl fp instruction $%X\n", iw>>10);
		exit(1);
	}
}

u64 nvc::step()
{
	r[0] = 0;
	icyc = 1;
	u16 iw = read(pc, 16);
	printf("nvc: pc = $%X, iw = $%X\n", pc, iw);
	//if( pc == 0x700004A ) exit(1);
	if( pc == 0 ) exit(1);
	pc += 2;
	exec(iw);
	return icyc;
}

void nvc::reset()
{
	pc = 0xffff'fff0;
	for(u32 i = 0; i < 32; ++i) { r[i] = 0; sys[i] = 0; }
	ECR = 0x0000FFF0;
	PSW = 0x8000;
	halted = false;
}

