#include <cstdio>
#include <cstdlib>
#include <cmath>
#include "nvc.h"

#define se16to32(a) ((s32)(s16)(a))
#define se8to32(a) ((s32)(s8)(a))

void nvc::exec(u16 iw)
{
	u32 temp = 0;

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
	const s32 simm5 = ( (reg1 & 0x10) ? (reg1|~0x1f) : reg1 );
	const u32 sa = reg1;
	
	switch( iw>>10 )
	{
	case 0x00: // MOV r2, r1
		r[reg2] = r[reg1];
		break;
	case 0x01: // ADD r2, r1
		r[reg2] = add(r[reg2], r[reg1]);
		break;
	case 0x02: // SUB r2, r1
		r[reg2] = add(r[reg2], ~r[reg1], 1);
		break;
	case 0x03: // CMP r2, r1
		add(r[reg2], ~r[reg1], 1);
		break;
	case 0x04: // SHL r2, r1
		temp = r[reg1]&0x1f;
		if( temp )
		{
			setcy(((u64(r[reg2])<<temp)>>32)&1);
			r[reg2] <<= temp;
		} else {
			setcy(0);
		}
		setsz(r[reg2]);
		setov(0);
		break;
	case 0x05: // SHR r2, r1
		temp = r[reg1]&0x1f;
		if( temp )
		{
			setcy((r[reg2]>>(temp-1))&1);
			r[reg2] >>= temp;
		} else {
			setcy(0);
		}
		setsz(r[reg2]);
		setov(0);
		break;		
	case 0x06: // jmp reg1
		pc = r[reg1] & ~1;
		break;
	case 0x07: // SAR r2, r1
		temp = r[reg1]&0x1f;
		if( temp )
		{
			setcy((s32(r[reg2])>>(temp-1))&1);
			r[reg2] = s32(r[reg2])>>temp;
		} else {
			setcy(0);
		}
		setsz(r[reg2]);
		setov(0);
		break;
	case 0x08:{ // MUL (signed)
		s64 A = s32(r[reg2]);
		A *= s32(r[reg1]);
		r[30] = A>>32;
		r[reg2] = A;
		setsz(r[reg2]);
		setov(A != (s64)s32(r[reg2]));
		}break;
	case 0x09: // DIV (signed)  //todo: div-zero exception
		if( r[reg2] == 0x80000000u && r[reg1] == 0xffffFFFFu )
		{
			setov(1);
			r[30] = 0;
		} else {
			setov(0);
			r[30] = s32(r[reg2]) % s32(r[reg1]);
			r[reg2] = s32(r[reg2]) / s32(r[reg1]);
		}
		setsz(r[reg2]);
		break;
	case 0x0A:{ // MULU (unsigned)
		u64 A = r[reg2];
		A *= r[reg1];
		r[30] = A>>32;
		r[reg2] = A;
		setsz(r[reg2]);
		setov(A != u64(r[reg2]));
		}break;	
	case 0x0B: // DIVU (unsigned) //todo: div-zero exception
		r[30] = r[reg2] % r[reg1];
		r[reg2] /= r[reg1];
		setsz(r[reg2]);
		setov(0);	
		break;
	case 0x0C: // OR r2, r1
		r[reg2] |= r[reg1]; 
		setsz(r[reg2]); 
		setov(0);
		break;
	case 0x0D: // AND r2, r1
		r[reg2] &= r[reg1]; 
		setsz(r[reg2]); 
		setov(0);
		break;
	case 0x0E: // XOR r2, r1
		r[reg2] ^= r[reg1]; 
		setsz(r[reg2]); 
		setov(0);
		break;
	case 0x0F: // NOT r2, r1
		r[reg2] = ~r[reg1];
		setsz(r[reg2]);
		setov(0);
		break;		
	case 0x10: // mov r2, simm5
		r[reg2] = simm5;
		break;		
	case 0x11: // ADD r2, simm5
		r[reg2] = add(r[reg2], simm5);
		break;
	case 0x12: // SETF r2, cond
		r[reg2] = (isCond(reg1&15) ? 1 : 0);	
		break;
	case 0x13: // CMP r2, simm5
		add(r[reg2], ~simm5, 1);
		break;
	case 0x14: // SHL r2, imm5
		if( sa )
		{
			setcy(((u64(r[reg2])<<sa)>>32)&1);
			r[reg2] <<= sa;
		} else {
			setcy(0);
		}
		setsz(r[reg2]);
		setov(0);
		break;	
	case 0x15: // SHR r2, imm5
		if( sa )
		{
			setcy((r[reg2]>>(sa-1))&1);
			r[reg2] >>= sa;
		} else {
			setcy(0);
		}
		setsz(r[reg2]);
		setov(0);
		break;
	case 0x16: // CLI
		PSW &= ~BIT(12);
		break;
	case 0x17: // SAR r2, imm5
		if( sa )
		{
			setcy((s32(r[reg2])>>(sa-1))&1);
			r[reg2] = s32(r[reg2])>>sa;
		} else {
			setcy(0);
		}
		setsz(r[reg2]);
		setov(0);
		break;
	case 0x19: // RETI
		if( PSW & BIT(15) )
		{
			printf("NP set on reti\n");
			exit(1);
		} else {
			pc =  sys[0];
			PSW = sys[1];
		}
		break;
	case 0x1A: // HALT
		halted = true;
		break;
	
	case 0x1C: // LDSR sr1, r2
		set_sysreg(reg1, r[reg2]);
		break;
	case 0x1D: // STSR r2, sr1
		r[reg2] = get_sysreg(reg1);
		break;
	case 0x1E: // SEI
		PSW |= BIT(12);
		break;
		
	case 0x28: // MOVEA r2, r1, simm16
		r[reg2] = r[reg1] + se16to32(imm16());
		break;	
	case 0x29: // ADDI
		r[reg2] = add(r[reg1], se16to32(imm16()));	
		break;
	case 0x2A: // JR rel26
		iw &= 0x3ff;
		if( iw & BIT(9) ) iw |= ~0x3ff;
		temp = ((iw<<16)|imm16()) & ~1;
		pc += temp - 4;
		break;
	case 0x2B: // JAL rel26
		r[31] = pc + 2;
		iw &= 0x3ff;
		if( iw & BIT(9) ) iw |= ~0x3ff;
		temp = ((iw<<16)|imm16()) & ~1;
		pc += temp - 4;
		break;
	case 0x2C: // ORI
		r[reg2] = r[reg1] | imm16();
		setsz(r[reg2]);
		setov(0);
		break;
	case 0x2D: // ANDI
		r[reg2] = r[reg1] & imm16();
		setsz(r[reg2]);
		setov(0);
		break;
	case 0x2E: // XORI
		r[reg2] = r[reg1] ^ imm16();
		setsz(r[reg2]);
		setov(0);
		break;	
	case 0x2F: // MOVHI r2, r1 + imm16<<16
		r[reg2] = r[reg1] + (imm16()<<16);
		break;
	
	case 0x30: // LD.B r2, [r1 + disp16]
		r[reg2] = se8to32(read(r[reg1] + se16to32(imm16()), 8)&0xff);
		break;
	case 0x31: // LD.H r2, [r1 + disp16]
		r[reg2] = se16to32(read(r[reg1] + se16to32(imm16()), 16)&0xffff);
		break;
	//case 0x32 invalid
	case 0x3B: // IN.W r2, [r1 + disp16] (same as LD.W)
	case 0x33: // LD.W r2, [r1 + disp16]
		r[reg2] = read(r[reg1] + se16to32(imm16()), 32);
		break;
		
		
	case 0x38: // IN.B r2, [r1 + disp16]
		r[reg2] = read(r[reg1] + se16to32(imm16()), 8) & 0xff;
		break;
	case 0x39: // IN.H r2, [r1 + disp16]
		r[reg2] = read(r[reg1] + se16to32(imm16()), 16) & 0xffff;
		break;
	
	case 0x3C: // OUT.B same as ST.B
	case 0x34: // ST.B r2, [r1 + disp16]
		write(r[reg1] + se16to32(imm16()), r[reg2]&0xff, 8);
		break;
	case 0x3D: // OUT.H same as ST.H
	case 0x35: // ST.H r2, [r1 + disp16]
		write(r[reg1] + se16to32(imm16()), r[reg2]&0xffff, 16);
		break;
	//case 0x36 invalid
	case 0x3F: // OUT.W same as ST.W
	case 0x37: // ST.W r2, [r1 + disp16]
		write(r[reg1] + se16to32(imm16()), r[reg2], 32);
		break;

	case 0x3E: // floating point and nintendo
		iw = imm16(); 
		exec_fp(reg2, reg1, iw);
		break;
	default: printf("NVC: Unimpl 6bit opcode $%X\n", iw>>10); exit(1);
	}
}

void nvc::exec_fp(const u32 reg2, const u32 reg1, u16 iw)
{
	switch( iw >> 10 )
	{
	case 0b000000: icyc = 8; { float temp = f(reg2) - f(reg1); u32 t = 0; memcpy(&t, &temp, 4); setsz(t); setcy(PSW&2); setov(0); break; }
	
	case 0b000100: f(reg2, f(reg2) + f(reg1)); setsz(r[reg2]); setov(0); setcy(PSW&2); break;
	case 0b000101: f(reg2, f(reg2) - f(reg1)); setsz(r[reg2]); setov(0); setcy(PSW&2); break;
	case 0b000110: f(reg2, f(reg2) * f(reg1)); setsz(r[reg2]); setov(0); setcy(PSW&2); break;
	case 0b000111: f(reg2, f(reg2) / f(reg1)); setsz(r[reg2]); setov(0); setcy(PSW&2); icyc = 44; break;
	
	case 0b000010: f(reg2, s32(r[reg1]) ); setsz(r[reg2]); setov(0); setcy(PSW&2); break;
	case 0b000011: r[reg2] = lround( f(reg1) ); setsz(r[reg2]); setov(0); break;
	case 0b001011: r[reg2] = s32( f(reg1) ); setsz(r[reg2]); setov(0); break;
	
	
	case 0b001100: icyc = 9; r[reg2] *= ((r[reg1]&0x10000)?(r[reg1]|~0x1ffff):(r[reg1]&0x1ffff)); break;
	case 0b001000: icyc = 6; r[reg2] = (r[reg2]&0xFFFF0000) | ((r[reg2] << 8)&0xFF00) | ((r[reg2] >> 8)&0xFF); break;
	case 0b001001: r[reg2] = (r[reg2]<<16)|(r[reg2]>>16); break;
	default:
		printf("NVC: Unimpl fp instruction $%X\n", iw>>10);
		exit(1);
	}
}

u16 nvc::imm16()
{
	u16 v = read(pc, 16);
	pc += 2;
	return v;
}

u64 nvc::step()
{
	r[0] = 0;
	if( halted ) return 4;
	icyc = 1;
	u16 iw = read(pc, 16);
	//printf("nvc: pc = $%X, iw = $%X\n", pc, iw);
	if( pc == 0 ) exit(1);
	pc += 2;
	exec(iw);
	return icyc;
}

void nvc::irq(u32 level, u16 code, u32 vector)
{
	if( PSW & (BIT(12) | BIT(15) | BIT(14)) ) return;
	u32 I = (PSW>>16)&15;
	if( level < I ) return;

	ECR = code;
	sys[0] = pc;
	sys[1] = PSW;
	
	PSW |= BIT(14)|BIT(12); // EP|ID
	PSW &= ~BIT(13); // address trap enable cleared
	level += 1;
	level &= 0xf;
	PSW &= ~0xf0000;
	PSW |= level<<16;
	halted = false;
	
	pc = vector;
}

void nvc::reset()
{
	pc = 0xffff'fff0;
	for(u32 i = 0; i < 32; ++i) { r[i] = 0; sys[i] = 0; }
	ECR = 0x0000FFF0;
	PSW = 0x8000;
	halted = false;
}

u32 nvc::get_sysreg(u32 s)
{
	switch( s )
	{
	case 6: return 0x00005346; // PIR
	case 7: return 0xE0; // TKCW
	case 24: return sys[24]&2; // CHCW
	case 30: return 4; //??
	case 31: return -s32(sys[31]); // absolute value??
	default: break;
	}
	return sys[s];
}

void nvc::set_sysreg(u32 s, u32 v)
{
	if( s == 4 ) return; // ECR is read-only
	sys[s] = v;
}

/*
// moves
	case 0b000000: r[reg2] = r[reg1]; break;
	case 0b010000: r[reg2] = ((reg1&0x10) ? (reg1|~0x1f) : reg1); break;
	case 0b101000: r[reg2] = r[reg1] + s16(read(pc, 16)); pc += 2; break;
	case 0b101111: r[reg2] = r[reg1] + (read(pc, 16)<<16); pc += 2; break;
	
	// io
	case 0b111000: r[reg2] = read(r[reg1] + s16(read(pc,16)), 8)&0xff;       pc+=2; break;
	case 0b111001: r[reg2] = read(r[reg1] + s16(read(pc,16)), 16)&0xffff;    pc+=2; break;
	case 0b111011: r[reg2] = read(r[reg1] + s16(read(pc,16)), 32);           pc+=2; break;
	case 0b110000: r[reg2] = (s32)(s8)read(r[reg1] + s16(read(pc,16)), 8);   pc+=2; break;
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
		if( r[reg2] == 0x80000000u && r[reg1] == 0xffffFFFFu )
		{
			setov(1);
			r[30] = 0;
		} else {
			setov(0);
			r[reg2] = s32(r[reg2]) / s32(r[reg1]);
			r[30] = s32(r[reg2]) % s32(r[reg1]);
		}
		setsz(r[reg2]);
		break;
	case 0b001011:
		icyc = 36;
		setov(0);
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
		//printf("relative branch to $%X\n", pc);
		} break;
	case 0b101011:{
		icyc = 3;
		iw &= 0x3ff;
		if( iw & BIT(9) ) iw |= ~0x3ff;
		u32 disp = (iw<<16)|read(pc, 16);
		pc -= 2;
		r[31] = pc + 4;
		pc += disp;
		//printf("relative blink to $%X\n", pc);
		} break;
	case 0b011010: halted = true; break;
	case 0b011001: pc = sys[0]; PSW = sys[1]; icyc = 10; break;

	// logical
	case 0b000100: setcy(((u64(r[reg2])<<(r[reg1]&0x1f))>>32)&1); r[reg2] <<= (r[reg1]&0x1f); setsz(r[reg2]); setov(0); break;
	case 0b000111: setcy((s32(r[reg2])>>((r[reg1]&0x1f)-1))&1); r[reg2] = s32(r[reg2]) >> (r[reg1]&0x1f); setsz(r[reg2]); setov(0); break;
	case 0b000101: setcy((s32(r[reg2])>>((r[reg1]&0x1f)-1))&1); r[reg2] >>= (r[reg1]&0x1f); setsz(r[reg2]); setov(0); break;
	
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
*/

