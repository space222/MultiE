#include <cstdio>
#include <cstdlib>
#include <print>
#include <bit>
#include <cstring>
#include "itypes.h"
#include "286.h"

#define ES seg[0]
#define CS seg[1]
#define SS seg[2]
#define DS seg[3]
#define ES_DESC desc[0]
#define CS_DESC desc[1]
#define SS_DESC desc[2]

#define CPL (CS&3)

#define MODRM8W modrm.set(imm8()); lea() /*; if( !validate_lea(true, true, 1) ) return*/
#define MODRM8  modrm.set(imm8()); lea() /*; if( !validate_lea(false, true, 1) ) return*/
#define MODRM16W modrm.set(imm8()); lea()/*; if( !validate_lea(true, true, 2) ) return*/
#define MODRM16  modrm.set(imm8()); lea()/*; if( !validate_lea(false, true, 2) ) return*/

#define AX regs[0].w
#define AH regs[0].b.h
#define AL regs[0].b.l

#define CX regs[1].w
#define CL regs[1].b.l
#define DX regs[2].w
#define BX regs[3].w
#define SP regs[4].w
#define BP regs[5].w
#define SI regs[6].w
#define DI regs[7].w

void x286::step()
{
	u32 t = 0;
	start_ip = IP;
	start_cs = CS;
	u8 opc = read(CS_DESC.base + IP, 8);
	IP += 1;
	
	PDATA = 0xff;
	REP = LOCK = 0;
	while( is_prefix(opc) )
	{
		opc = read(CS_DESC.base + IP, 8);
		IP += 1;
	}

	//std::println("${:02X}:${:02X}: opc = ${:X}, pdata=${:X}", start_cs, start_ip, opc, PDATA);
	
	switch( opc )
	{
	case 0x00: // add rm8, r8
		MODRM8W;
		rm8(add8(rm8(), r8()));
		break;
	case 0x01: // add rm16, r16
		MODRM16W;
		rm16(add16(rm16(), r16()));
		break;
	case 0x02: // add r8, rm8
		MODRM8;
		r8(add8(r8(), rm8()));
		break;
	case 0x03: // add r16, rm16
		MODRM16;
		r16(add16(r16(), rm16()));
		break;
	case 0x04: // add al, imm8
		AL = add8(AL, imm8());
		break;
	case 0x05: // add ax, imm16
		AX = add16(AX, imm16());
		break;
		
	case 0x08: // or rm8, r8
		MODRM8W;
		rm8(psz8(rm8()|r8()));
		F.b.C = F.b.O = F.b.A = 0;
		break;
	case 0x09: // or rm16, r16
		MODRM16W;
		rm16(psz16(rm16()|r16()));
		F.b.C = F.b.O = F.b.A = 0;
		break;
	case 0x0A: // or r8, rm8
		MODRM8;
		r8(psz8(r8()|rm8()));
		F.b.C = F.b.O = F.b.A = 0;
		break;
	case 0x0B: // or r16, rm16	
		MODRM16;
		r16(psz16(r16()|rm16()));
		F.b.C = F.b.O = F.b.A = 0;
		break;		
	case 0x0C: // or al, imm8
		AL |= imm8();
		psz8(AL);
		F.b.C = F.b.O = F.b.A = 0;
		break;
	case 0x0D: // or ax, imm16
		AX |= imm16();
		psz16(AX);
		F.b.C = F.b.O = F.b.A = 0;
		break;
		
	case 0x0F: // extension
		ext0f();
		break;
	case 0x10: // adc rm8, r8
		MODRM8W;
		rm8(add8(rm8(), r8(), F.b.C));
		break;
	case 0x11: // adc rm16, r16
		MODRM16W;
		rm16(add16(rm16(), r16(), F.b.C));
		break;
	case 0x12: // adc r8, rm8
		MODRM8;
		r8(add8(r8(), rm8(), F.b.C));
		break;
	case 0x13: // adc r16, rm16
		MODRM16;
		r16(add16(r16(), rm16(), F.b.C));
		break;		
	case 0x14: // adc al, imm8
		t = imm8();
		AL = add8(AL, t, F.b.C);
		break;
	case 0x15: // adc ax, imm16
		t = imm16();
		AX = add16(AX, t, F.b.C);
		break;
		
	case 0x18: // sbb rm8, r8
		MODRM8W;
		rm8(add8(rm8(), r8()^0xff, F.b.C^1));
		F.b.C ^= 1;
		F.b.A ^= 1;
		break;
	case 0x19: // sbb rm16, r16
		MODRM16W;
		rm16(add16(rm16(), r16()^0xffff, F.b.C^1));
		F.b.C ^= 1;
		F.b.A ^= 1;
		break;
	case 0x1A: // sbb r8, rm8
		MODRM8;
		r8(add8(r8(), rm8()^0xff, F.b.C^1));
		F.b.C ^= 1;
		F.b.A ^= 1;
		break;
	case 0x1B: // sbb r16, rm16
		MODRM16;
		r16(add16(r16(), rm16()^0xffff, F.b.C^1));
		F.b.C ^= 1;
		F.b.A ^= 1;
		break;
	case 0x1C: // sbb al, imm8
		AL = add8(AL, imm8()^0xff, F.b.C^1);
		F.b.C ^= 1;
		F.b.A ^= 1;
		break;
	case 0x1D: // sbb ax, imm16
		AX = add16(AX, imm16()^0xffff, F.b.C^1);
		F.b.C ^= 1;
		F.b.A ^= 1;
		break;
		
	case 0x20: // and rm8, r8
		MODRM8W;
		rm8(psz8(rm8()&r8()));
		F.b.C = F.b.O = F.b.A = 0;
		break;
	case 0x21: // and rm16, r16
		MODRM16W;
		rm16(psz16(rm16()&r16()));
		F.b.C = F.b.O = F.b.A = 0;
		break;
	case 0x22: // and r8, rm8
		MODRM8;
		r8(psz8(r8()&rm8()));
		F.b.C = F.b.O = F.b.A = 0;
		break;
	case 0x23: // and r16, rm16
		MODRM16;
		r16(psz16(r16()&rm16()));
		F.b.C = F.b.O = F.b.A = 0;
		break;		
	case 0x24: // and al, imm8
		AL = psz8(AL&imm8());
		F.b.C = F.b.O = F.b.A = 0;
		break;
	case 0x25: // and ax, imm16
		AX = psz16(AX&imm16());
		F.b.C = F.b.O = F.b.A = 0;
		break;
		
	case 0x28: // sub rm8, r8
		MODRM8W;
		rm8(add8(rm8(), r8()^0xff, 1));
		F.b.C ^= 1;
		F.b.A ^= 1;
		break;
	case 0x29: // sub rm16, r16
		MODRM16W;
		rm16(add16(rm16(), r16()^0xffff, 1));
		F.b.C ^= 1;
		F.b.A ^= 1;
		break;
	case 0x2A: // sub r8, rm8
		MODRM8;
		r8(add8(r8(), rm8()^0xff, 1));
		F.b.C ^= 1;
		F.b.A ^= 1;
		break;
	case 0x2B: // sub r16, rm16
		MODRM16;
		r16(add16(r16(), rm16()^0xffff, 1));
		F.b.C ^= 1;
		F.b.A ^= 1;
		break;		
	case 0x2C: // sub al, imm8
		AL = add8(AL, imm8()^0xff, 1);
		F.b.C ^= 1;
		F.b.A ^= 1;
		break;
	case 0x2D: // sub ax, imm16
		AX = add16(AX, imm16()^0xffff, 1);
		F.b.C ^= 1;
		F.b.A ^= 1;
		break;
		
	case 0x30: // xor rm8, r8
		MODRM8W;
		rm8(psz8(rm8()^r8()));
		F.b.C = F.b.O = F.b.A = 0;
		break;
	case 0x31: // xor rm16, r16
		MODRM16W;
		rm16(psz16(rm16()^r16()));
		F.b.C = F.b.O = F.b.A = 0;
		break;
	case 0x32: // xor r8, rm8
		MODRM8;
		r8(psz8(r8()^rm8()));
		F.b.C = F.b.O = F.b.A = 0;
		break;
	case 0x33: // xor r16, rm16
		MODRM16;
		r16(psz16(r16()^rm16()));
		F.b.C = F.b.O = F.b.A = 0;
		break;
	case 0x34: // xor al, imm8
		AL = psz8(AL^imm8());
		F.b.C = F.b.O = F.b.A = 0;
		break;
	case 0x35: // xor ax, imm16
		AX = psz16(AX^imm16());
		F.b.C = F.b.O = F.b.A = 0;
		break;
		
	case 0x37: // aaa
		t = AL;
		if( (AL&15) > 9 || F.b.A )
		{
			AX += 0x106;
			F.b.A = F.b.C = 1;
		} else {
			F.b.A = F.b.C = 0;
		}
		F.b.O = 0;
		psz8(AL);
		AL &= 15;
		break;
		
	case 0x38: // cmp rm8, r8
		MODRM8;
		(add8(rm8(), r8()^0xff, 1));
		F.b.C ^= 1;
		F.b.A ^= 1;
		break;
	case 0x39: // cmp rm16, r16
		MODRM16;
		(add16(rm16(), r16()^0xffff, 1));
		F.b.C ^= 1;
		F.b.A ^= 1;
		break;
	case 0x3A: // cmp r8, rm8
		MODRM8;
		(add8(r8(), rm8()^0xff, 1));
		//if( start_ip == 0x44f && start_debug ) { std::println("cmp r8, [${:X}:${:X}]", segment, offset); }
		F.b.C ^= 1;
		F.b.A ^= 1;
		break;
	case 0x3B: // cmp r16, rm16
		MODRM16;
		(add16(r16(), rm16()^0xffff, 1));
		F.b.C ^= 1;
		F.b.A ^= 1;
		break;		
	case 0x3C: // cmp al, imm8
		add8(AL, imm8()^0xff, 1);
		F.b.C ^= 1;
		F.b.A ^= 1;
		break;
	case 0x3D: // cmp ax, imm16
		add16(AX, imm16()^0xffff, 1);
		F.b.C ^= 1;
		F.b.A ^= 1;
		break;

	case 0x3F: // aas
		if( (AL&15) > 9 || F.b.A == 1 )
		{
			AX -= 6;
			AH -= 1;
			F.b.A = F.b.C = 1;
		} else {
			F.b.A = F.b.C = 0;
		}
		psz8(AL);
		F.b.O = 0;
		AL &= 15;
		break;

	case 0x40:
	case 0x41:
	case 0x42:
	case 0x43:
	case 0x44:
	case 0x45:
	case 0x46:
	case 0x47: // inc r16
		t = regs[opc&7].w;
		F.b.A = ((t&0xf)==0xf);
		regs[opc&7].w += 1;
		F.b.O = !(t&0x8000)&&(regs[opc&7].w&0x8000);
		psz16(regs[opc&7].w);
		break;		
	case 0x48:
	case 0x49:
	case 0x4A:
	case 0x4B:
	case 0x4C:
	case 0x4D:
	case 0x4E:
	case 0x4F: // dec r16
		t = regs[opc&7].w;
		F.b.A = ((t&0xf)==0);
		regs[opc&7].w -= 1;
		F.b.O = (t&0x8000)&&!(regs[opc&7].w&0x8000);
		psz16(regs[opc&7].w);
		break;
		
	case 0x50:
	case 0x51:
	case 0x52:
	case 0x53:
	case 0x54:
	case 0x55:
	case 0x56:
	case 0x57: // push r16
		push16(regs[opc&7].w);
		break;
	case 0x58:
	case 0x59:
	case 0x5A:
	case 0x5B:
	case 0x5C:
	case 0x5D:
	case 0x5E:
	case 0x5F:{ // pop r16
		auto p = pop16();
		if( p ) regs[opc&7].w = p.value();
		}break;
		
	case 0x1F:{ // pop ds
		auto p = pop16();
		if( p ) setsr(3, p.value());
		}break;
	case 0x07:{ // pop es
		auto p = pop16();
		if( p ) setsr(0, p.value());
		}break;
	case 0x17:{ // pop ss
		auto p = pop16();
		if( p )
		{
			setsr(2, p.value());
			irq_blocked = true;
		}
		}break;
		
	case 0x0E: // push cs
		push16(CS);
		break;
	case 0x16: // push ss
		push16(SS);
		break;
	case 0x1E: // push ds
		push16(DS);
		break;
	case 0x06: // push es
		push16(ES);
		break;
	
	case 0x60: // pusha
		if( !validate_mem(SS, SS_DESC, (SP-2)&0xffff, true, true, true, 2) ) { return; }
		if( !validate_mem(SS, SS_DESC, (SP-16)&0xffff, true, true, true, 2) ) { return; }
		t = SP;
		for(u32 i = 0; i < 8; ++i)
		{
			SP -= 2;
			write(SS_DESC.base+SP, (i==4) ? t : regs[i].w, 16);
		}
		break;
	case 0x61: // popa
		if( !validate_mem(SS, SS_DESC, SP, true, false, true, 2) ) { return; }
		if( !validate_mem(SS, SS_DESC, (SP+14)&0xffff, true, false, true, 2) ) { return; }
		for(u32 i = 0; i < 8; ++i)
		{
			if( i != 3 ) regs[7^i].w = read(SS_DESC.base + SP, 16);
			SP += 2;
		}
		break;
		
	case 0x68: // push imm16
		push16(imm16());
		break;
	case 0x6A: // push imm8
		push16((s16)s8(imm8()));
		break;
		
	case 0x69: // imul r16, rm16, imm16
		MODRM16;
		t = (s32)s16(rm16()) * (s32)s16(imm16());
		r16(t);
		F.b.O = ( s32(t) != s16(t) ? 1 : 0);
		F.b.C = F.b.O;
		psz16(t>>16);
		F.b.A = 1;
		break;
	case 0x6B: // imul r16, rm16, imm8
		MODRM16;
		t = (s32)s16(rm16()) * (s32)s8(imm8());
		r16(t);
		F.b.O = ( s32(t) != s16(t) ? 1 : 0);
		F.b.C = F.b.O;
		psz16(t>>16);
		F.b.A = 1;
		break;
			
	case 0x63:{ // arpl rm16, r16
		MODRM16W;
		u16 a = rm16();
		u16 b = r16();
		if( (a&3) < (b&3) )
		{
			F.b.Z = 1;
			a &= ~3;
			a |= b&3;
		} else {
			F.b.Z = 0;
		}
		}break;
		
	case 0x70:
	case 0x71:
	case 0x72:
	case 0x73:
	case 0x74:
	case 0x75:
	case 0x76:
	case 0x77:
	case 0x78:
	case 0x79:
	case 0x7A:
	case 0x7B:
	case 0x7C:
	case 0x7D:
	case 0x7E:
	case 0x7F: // Jcc rel8
		t = imm8();
		if( cond(opc&15) )
		{
			u32 temp = IP + (s8)t;
			if( opsz == 16 ) temp &= 0xffff;
			if( temp > CS_DESC.limit ) 
			{ 
				general_pfault(); 
			} else {
				IP = temp;
			}
		}
		break;
	
	case 0x82: //fallthru: aliases 0x80
	case 0x80: // alu rm8, imm8
		modrm.set(imm8());
		lea();
		switch( modrm.reg )
		{
		case 0: // add
			if( !validate_lea(true, true, 1) ) return;
			rm8(add8(rm8(), imm8()));
			break;
		case 1: // or
			if( !validate_lea(true, true, 1) ) return;
			rm8(psz8(rm8()|imm8()));
			F.b.C = F.b.O = F.b.A = 0;
			break;
		case 2: // adc
			if( !validate_lea(true, true, 1) ) return;
			rm8(add8(rm8(), imm8(), F.b.C));
			break;
		case 3: // sbb
			if( !validate_lea(true, true, 1) ) return;
			rm8(add8(rm8(), imm8()^0xff, F.b.C^1));
			F.b.C ^= 1;
			F.b.A ^= 1;
			break;
		case 4: // and
			if( !validate_lea(true, true, 1) ) return;
			rm8(psz8(rm8()&imm8()));
			F.b.C = F.b.O = F.b.A = 0;
			break;
		case 5: // sub
			if( !validate_lea(true, true, 1) ) return;
			rm8(add8(rm8(), imm8()^0xff, 1));
			F.b.C ^= 1;
			F.b.A ^= 1;
			break;
		case 6: // xor
			if( !validate_lea(true, true, 1) ) return;
			rm8(psz8(rm8()^imm8()));
			F.b.C = F.b.O = F.b.A = 0;
			break;
		case 7: // cmp
			if( !validate_lea(false, true, 1) ) return;
			(add8(rm8(), imm8()^0xff, 1));
			F.b.C ^= 1;
			F.b.A ^= 1;
			break;
		}
		break;
	case 0x81: // alu rm16, imm16
		modrm.set(imm8());
		lea();
		switch( modrm.reg )
		{
		case 0: // add
			if( !validate_lea(true, true, 2) ) return;
			rm16(add16(rm16(), imm16()));
			break;		
		case 1: // or
			if( !validate_lea(true, true, 2) ) return;
			rm16(psz16(rm16()|imm16()));
			F.b.C = F.b.O = F.b.A = 0;
			break;
		case 2: // adc
			if( !validate_lea(true, true, 2) ) return;
			rm16(add16(rm16(), imm16(), F.b.C));
			break;
		case 3: // sbb
			if( !validate_lea(true, true, 2) ) return;
			rm16(add16(rm16(), imm16()^0xffff, F.b.C^1));
			F.b.C ^= 1;
			F.b.A ^= 1;
			break;
		case 4:{ // and
			//if( !validate_lea(true, true, 2) ) return;
			rm16(psz16(rm16()&imm16()));
			F.b.C = F.b.O = F.b.A = 0;
			}break;
		case 5: // sub
			if( !validate_lea(true, true, 2) ) return;
			rm16(add16(rm16(), imm16()^0xffff, 1));
			F.b.C ^= 1;
			F.b.A ^= 1;
			break;
		case 6: // xor
			if( !validate_lea(true, true, 2) ) return;
			rm16(psz16(rm16()^imm16()));
			F.b.C = F.b.O = F.b.A = 0;
			break;
		case 7: // cmp
			if( !validate_lea(false, true, 2) ) return;
			(add16(rm16(), imm16()^0xffff, 1));
			F.b.C ^= 1;
			F.b.A ^= 1;
			break;			
		}
		break;
	case 0x83: // alu rm16, imm8
		modrm.set(imm8());
		lea();
		switch( modrm.reg )
		{
		case 0: // add
			if( !validate_lea(true, true, 2) ) return;
			rm16(add16(rm16(), (s8)imm8()));
			break;
		case 1: // or
			if( !validate_lea(true, true, 2) ) return;
			rm16(psz16(rm16()|(s8)imm8()));
			F.b.C = F.b.O = F.b.A = 0;
			break;
		case 2: // adc
			if( !validate_lea(true, true, 2) ) return;
			rm16(add16(rm16(), (s8)imm8(), F.b.C));
			break;
		case 3: // sbb
			if( !validate_lea(true, true, 2) ) return;
			rm16(add16(rm16(), 0xffff^(u16)(s8)imm8(), F.b.C^1));
			F.b.C ^= 1;
			F.b.A ^= 1;
			break;
		case 4: // and
			if( !validate_lea(true, true, 2) ) return;
			rm16(psz16(rm16()&(s8)imm8()));
			F.b.C = F.b.O = F.b.A = 0;
			break;
		case 5: // sub
			if( !validate_lea(true, true, 2) ) return;
			rm16(add16(rm16(), 0xffff^(u16)(s8)imm8(), 1));
			F.b.C ^= 1;
			F.b.A ^= 1;
			break;
		case 6: // xor
			if( !validate_lea(true, true, 2) ) return;
			rm16(psz16(rm16()^(s8)imm8()));
			F.b.C = F.b.O = F.b.A = 0;
			break;
		case 7: // cmp
			if( !validate_lea(false, true, 2) ) return;
			(add16(rm16(), 0xffff^(u16)(s8)imm8(), 1));
			F.b.C ^= 1;
			F.b.A ^= 1;
			break;
		}
		break;
	case 0x84: // test rm8, r8
		MODRM8;
		psz8(rm8()&r8());
		F.b.C = F.b.O = F.b.A = 0;
		break;
	case 0x85: // test rm16, r16
		MODRM16;
		psz16(rm16()&r16());
		F.b.C = F.b.O = F.b.A = 0;
		break;
	case 0x86: // xchg rm8, r8
		MODRM8W;
		t = rm8();
		rm8(r8());
		r8(t);
		break;
	case 0x87: // xchg rm16, r16
		MODRM16W;
		t = rm16();
		rm16(r16());
		r16(t);
		break;		
	case 0x88: // mov rm8, r8
		MODRM8W;
		rm8(r8());
		break;
	case 0x89: // mov rm16, r16
		MODRM16W;
		rm16(r16());
		break;
	case 0x8A: // mov r8, rm8
		MODRM8;
		r8(rm8());
		break;
	case 0x8B: // mov r16, rm16
		MODRM16;
		r16(rm16());
		break;
	case 0x8C: // mov rm16, Sreg
		MODRM16W;
		rm16(seg[modrm.reg]);	
		break;
	case 0x8D: // lea r16, rm16
		modrm.set(imm8());
		lea();
		regs[modrm.reg].w = offset;
		break;
	case 0x8E: // mov Sreg, rm16
		MODRM16;
		setsr(modrm.reg, rm16());
		if( modrm.reg == 2 )
		{
			irq_blocked = true;
		}
		break;
	case 0x8F:{ // pop rm16
		MODRM16W;
		auto p = pop16();
		if( p )
		{
			rm16(p.value());
		}
		}break;
	case 0x90: break; // nop
	case 0x91:
	case 0x92:
	case 0x93:
	case 0x94:
	case 0x95:
	case 0x96:
	case 0x97: // xchg ax, r16
		t = AX;
		AX = regs[opc&7].w;
		regs[opc&7].w = t;
		break;
	case 0x98: AX = (s8)AL; break; // cbw
	case 0x99: DX = ((AX&0x8000)?0xffff:0); break; // cwd
	case 0x9A:{ // call ptr16:16
			u16 o = imm16();
			u16 s = imm16();
			far_call(s, o);	
		}break;
		
	case 0x9C: // pushf
		F.b.pad1 = 1;
		F.b.pad3 = F.b.pad5 = 0;
		push16(F.v);
		break;
	case 0x9D:{ // popf
		auto p = pop16();
		if( !p ) return;
		u16 oldF = F.v;
		F.v = p.value();
		if( !ring0() ) { F.v = (F.v&~0x3000)|(oldF&0x3000); }   // real mode or rings123 cant change IOPL
		if( !inPMode() ) { F.v = (F.v&~0x4000)|(oldF&0x4000); } // real mode cant change TF
		if( inPMode() && CPL > F.b.IOPL ) { F.v = (F.v&~0x0200)|(oldF&0x0200); } // ring greater than IOPL cant change IF
		F.b.pad1 = 1;
		F.b.pad3 = F.b.pad5 = 0;
		}break;
	case 0x9E: // sahf
		F.v = (F.v&~0xff) | AH;
		F.b.pad1 = 1;
		F.b.pad3 = F.b.pad5 = 0;
		break;
	case 0x9F: // lahf
		F.b.pad1 = 1;
		F.b.pad3 = F.b.pad5 = 0;
		AH = F.v;
		AH &= 0xD7;
		AH |= 2;
		break;
	case 0xA0: // mov al, moffs8
		t = imm16();
		if( PDATA==0xff ) PDATA = 3;
		if( !validate_mem(seg[PDATA], desc[PDATA], t, PDATA==2, false, true, 1) ) return;
		AL = read(desc[PDATA].base+t, 8);
		break;
	case 0xA1: // mov ax, moffs16
		t = imm16();
		if( PDATA==0xff ) PDATA = 3;
		if( !validate_mem(seg[PDATA], desc[PDATA], t, PDATA==2, false, true, 2) ) return;
		AX = read(desc[PDATA].base+t, 16);
		break;
	case 0xA2: // mov moffs8, al
		t = imm16();
		if( PDATA==0xff ) PDATA = 3;
		if( !validate_mem(seg[PDATA], desc[PDATA], t, PDATA==2, true, true, 1) ) return;
		write(desc[PDATA].base+t, AL, 8);
		break;
	case 0xA3: // mov ax, moffs16
		t = imm16();
		if( PDATA==0xff ) PDATA = 3;
		if( !validate_mem(seg[PDATA], desc[PDATA], t, PDATA==2, true, true, 2) ) return;
		write(desc[PDATA].base+t, AX, 16);
		break;
		
		
	case 0xA4: // movsb
		if( REP && CX == 0 ) break;
		if( PDATA==0xff ) PDATA = 3;
		if( !validate_mem(seg[PDATA], desc[PDATA], SI, PDATA==2, false, true, 1) ) return;
		if( !validate_mem(ES, ES_DESC, DI, false, true, true, 1) ) return;
		write(ES_DESC.base + DI, read(desc[PDATA].base+SI, 8), 8);
		SI += (F.b.D ? -1 : 1);
		DI += (F.b.D ? -1 : 1);
		if( REP )
		{
			CX -= 1;
			CS = start_cs;
			IP = start_ip;
		}
		break;
	case 0xA5: // movsw
		if( REP && CX == 0 ) break;
		if( PDATA==0xff ) PDATA = 3;
		if( !validate_mem(seg[PDATA], desc[PDATA], SI, PDATA==2, false, true, 2) ) return;
		if( !validate_mem(ES, ES_DESC, DI, false, true, true, 2) ) return;
		write(ES_DESC.base + DI, read(desc[PDATA].base+SI, 16), 16);
		SI += (F.b.D ? -2 : 2);
		DI += (F.b.D ? -2 : 2);
		if( REP )
		{
			CX -= 1;
			CS = start_cs;
			IP = start_ip;
		}
		break;
	case 0xA6:{ // cmpsb
		if( REP && CX == 0 ) break;
		if( PDATA == 0xff ) PDATA = 3;
		if( !validate_mem(seg[PDATA], desc[PDATA], SI, PDATA==2, false, true, 1) ) return;
		if( !validate_mem(ES, ES_DESC, DI, false, false, true, 1) ) return;
		
		add8(read(ES_DESC.base + DI, 8)^0xff, read(desc[PDATA].base + SI, 8), 1);
		F.b.C ^= 1;
		F.b.A ^= 1;
		SI += (F.b.D ? -1 : 1);
		DI += (F.b.D ? -1 : 1);
		if( REP ) CX--;
		if( (REP==0xF3 && F.b.Z==1) || (REP==0xF2 && F.b.Z==0) )
		{
			CS = start_cs;
			IP = start_ip;
		}
	}break;
	case 0xA7: // cmpsw
		if( REP && CX == 0 ) break;
		if( PDATA == 0xff ) PDATA = 3;
		if( !validate_mem(seg[PDATA], desc[PDATA], SI, PDATA==2, false, true, 2) ) return;
		if( !validate_mem(ES, ES_DESC, DI, false, false, true, 2) ) return;
		
		add16(0xffff^read(ES_DESC.base+DI, 16), read(desc[PDATA].base+SI,16), 1);
		F.b.C ^= 1;
		F.b.A ^= 1;
		SI += (F.b.D ? -2 : 2);
		DI += (F.b.D ? -2 : 2);
		if( REP ) CX--;
		if( (REP==0xF3 && F.b.Z==1) || (REP==0xF2 && F.b.Z==0) )
		{
			CS = start_cs;
			IP = start_ip;
		}
		break;			
	case 0xA8: // test al, imm8
		psz8(AL & imm8());
		F.b.C = F.b.O = F.b.A = 0;
		break;
	case 0xA9: // test ax, imm16
		psz16(AX & imm16());
		F.b.C = F.b.O = F.b.A = 0;
		break;
	case 0xAA: // stosb
		if( REP && CX == 0 ) break;
		if( !validate_mem(ES, ES_DESC, DI, false, true, true, 1) ) return;
		write(ES_DESC.base + DI, AL, 8);
		DI += (F.b.D ? -1 : 1);
		if( REP )
		{
			CX -= 1;
			CS = start_cs;
			IP = start_ip;
		}
		break;
	case 0xAB: // stosw
		if( REP && CX == 0 ) break;
		if( !validate_mem(ES, ES_DESC, DI, false, true, true, 2) ) return;
		write(ES_DESC.base + DI, AX, 16);
		DI += (F.b.D ? -2 : 2);
		if( REP )
		{
			CX -= 1;
			CS = start_cs;
			IP = start_ip;
		}
		break;
	case 0xAC: // lodsb
		if( REP && CX == 0 ) break;
		if( PDATA == 0xff ) PDATA = 3;
		if( !validate_mem(seg[PDATA], desc[PDATA], SI, PDATA==2, false, true, 1) ) return;
		AL = read(desc[PDATA].base + SI, 8);
		SI += (F.b.D ? -1 : 1);
		if( REP )
		{
			CX -= 1;
			CS = start_cs;
			IP = start_ip;
		}
		break;
	case 0xAD: // lodsw
		if( REP && CX == 0 ) break;
		if( PDATA == 0xff ) PDATA = 3;
		if( !validate_mem(seg[PDATA], desc[PDATA], SI, PDATA==2, false, true, 2) ) return;
		AX = read(desc[PDATA].base + SI, 16);
		SI += (F.b.D ? -2 : 2);
		if( REP )
		{
			CX -= 1;
			CS = start_cs;
			IP = start_ip;
		}
		break;		
	case 0xAE:{ // scasb
		if( REP && CX == 0 ) break;
		if( !validate_mem(ES, ES_DESC, DI, false, false, true, 1) ) return;
		
		add8(read(ES_DESC.base + DI, 8)^0xff, AL, 1);
		F.b.C ^= 1;
		F.b.A ^= 1;
		DI += (F.b.D ? -1 : 1);
		if( REP ) CX--;
		if( (REP==0xF3 && F.b.Z==1) || (REP==0xF2 && F.b.Z==0) )
		{
			CS = start_cs;
			IP = start_ip;
		}
	}break;
	case 0xAF: // scasw
		if( REP && CX == 0 ) break;
		if( !validate_mem(ES, ES_DESC, DI, false, false, true, 2) ) return;
		
		add16(0xffff^read(ES_DESC.base+DI, 16), AX, 1);
		F.b.C ^= 1;
		F.b.A ^= 1;
		DI += (F.b.D ? -2 : 2);
		if( REP ) CX--;
		if( (REP==0xF3 && F.b.Z==1) || (REP==0xF2 && F.b.Z==0) )
		{
			CS = start_cs;
			IP = start_ip;
		}
		break;
	case 0xB0:
	case 0xB1:
	case 0xB2:
	case 0xB3:
	case 0xB4:
	case 0xB5:
	case 0xB6:
	case 0xB7: // mov r8, imm8
		t = imm8();
		if( opc&4 )
		{
			regs[opc&3].b.h = t;
		} else {
			regs[opc&3].b.l = t;
		}
		break;
	case 0xB8:
	case 0xB9:
	case 0xBA:
	case 0xBB:
	case 0xBC:
	case 0xBD:
	case 0xBE:
	case 0xBF: // mov r16, imm16
		regs[opc&7].w = imm16();
		break;
		
	case 0xC0: // shift / rotate 8bit by imm8
		MODRM8W;
		switch( modrm.reg )
		{
		case 0:	rm8(rol8(rm8(), imm8()&0x1F)); break;
		case 1: rm8(ror8(rm8(), imm8()&0x1F)); break;
		case 2: rm8(rcl8(rm8(), imm8()&0x1F)); break;
		case 3:	rm8(rcr8(rm8(), imm8()&0x1F)); break;
		case 4: rm8(shl8(rm8(), imm8()&0x1F)); break;
		case 5: rm8(shr8(rm8(), imm8()&0x1F)); break;
		case 6:	rm8(shl8(rm8(), imm8()&0x1F)); break;
		case 7: rm8(sar8(rm8(), imm8()&0x1F)); break;
		}
		break;
	case 0xC1: // shift / rotate 16bit by imm8
		MODRM16W;
		switch( modrm.reg )
		{
		case 0:	rm16(rol16(rm16(), imm8()&0x1F)); break;
		case 1: rm16(ror16(rm16(), imm8()&0x1F)); break;
		case 2: rm16(rcl16(rm16(), imm8()&0x1F)); break;
		case 3:	rm16(rcr16(rm16(), imm8()&0x1F)); break;
		case 4: rm16(shl16(rm16(), imm8()&0x1F)); break;
		case 5: rm16(shr16(rm16(), imm8()&0x1F)); break;
		case 6:	rm16(shl16(rm16(), imm8()&0x1F)); break;
		case 7: rm16(sar16(rm16(), imm8()&0x1F)); break;
		}
		break;
	case 0xC2: // ret near imm16
		t = imm16();
		if( auto p = pop16(); p )
		{
			IP = p.value();
			if( IP > CS_DESC.limit ) { general_pfault(); return; }
			SP += t;
		}
		break;	
	case 0xC3: // ret near
		if( auto p = pop16(); p )
		{
			IP = p.value();
			if( IP > CS_DESC.limit ) { general_pfault(); return; }
		}
		break;
	case 0xC4:{ // les r16, m16:16
		modrm.set(imm8());
		lea();
		if( !validate_mem(segment, desc[PDATA], offset, PDATA==2, false, true, 2) ) return;
		if( !validate_mem(segment, desc[PDATA], (offset+2)&0xffff, PDATA==2, false, true, 2) ) return;
		u16 offs = read(desc[PDATA].base + offset, 16);
		u16 sg = read(desc[PDATA].base + ((offset+2)&0xffff), 16);
		if( !setsr(0, sg) ) return;
		regs[modrm.reg].w = offs;	
		}break;		
	case 0xC5:{ // lds r16, m16:16
		modrm.set(imm8());
		lea();
		if( !validate_mem(segment, desc[PDATA], offset, PDATA==2, false, true, 2) ) return;
		if( !validate_mem(segment, desc[PDATA], (offset+2)&0xffff, PDATA==2, false, true, 2) ) return;
		u16 offs = read(desc[PDATA].base + offset, 16);
		u16 sg = read(desc[PDATA].base + ((offset+2)&0xffff), 16);
		if( !setsr(3, sg) ) return;
		regs[modrm.reg].w = offs;	
		}break;
	case 0xC6: // 8bit stuff
		modrm.set(imm8());
		switch( modrm.reg )
		{
		case 0: // mov rm8, imm8
			lea();
			if( !validate_lea(true, true, 1) ) return;	
			rm8(imm8());
			break;
		default:
			std::println("Unimpl opc C6 /{:X}", u32(modrm.reg));
			exit(1);
		}
		break;
	case 0xC7: // 16bit stuff
		modrm.set(imm8());
		switch( modrm.reg )
		{
		case 0: // mov rm16, imm16
			lea();
			if( !validate_lea(true, true, 2) ) return;	
			rm16(imm16());
			break;
		default:
			std::println("Unimpl opc C6 /{:X}", u32(modrm.reg));
			exit(1);
		}
		break;
	case 0xC8:{ // enter (todo: better verify this, validate the [BP] reads)
		u16 sz = imm16();
		u8 level = imm8() & 31;
		if( !push16(BP) ) return;
		u32 temp = SP;
		if( level )
		{
			for(u32 i = 1; i < level; ++i)
			{
				BP -= 2;
				if( !push16(read(SS_DESC.base + BP, 16)) ) return;			
			}
			if( !push16(temp) ) return;
		}
		BP = temp;
		SP -= sz;
		}break;
	case 0xC9: // leave
		SP = BP;
		if( auto p = pop16(); p )
		{
			BP = p.value();
		}
		break;
	
	case 0xCA:{ // ret far imm16
		u16 s, o;
		t = imm16();
		if( auto p = pop16(); p )
		{
			o = p.value();
		} else return;
		if( auto p = pop16(); p )
		{
			s = p.value();
		} else return;
		far_ret(s, o, F.v);
		SP += t;		
		}break;	
	case 0xCB:{ // ret far
		u16 s, o;
		if( auto p = pop16(); p )
		{
			o = p.value();
		} else return;
		if( auto p = pop16(); p )
		{
			s = p.value();
		} else return;
		far_ret(s, o, F.v);		
		}break;
	case 0xCC: // int 3
		interrupt(3);
		break;
	case 0xCD: // int n
		t = imm8();
		if( hle_int(t) ) return;
		interrupt(t);
		break;
	case 0xCE: // into (4)
		if( F.b.O ) interrupt(4);
		break;
	case 0xCF:{ // iret (always far)
		u16 s, o, f;
		if( auto p = pop16(); p )
		{
			o = p.value();
		} else return;
		if( auto p = pop16(); p )
		{
			s = p.value();
		} else return;
		if( auto p = pop16(); p )
		{
			f = p.value();
		} else return;
		far_ret(s, o, f);		
		}break;
	case 0xD0: // shift / rotate 8bit by 1
		MODRM8W;
		switch( modrm.reg )
		{
		case 0:	rm8(rol8(rm8(), 1)); break;
		case 1: rm8(ror8(rm8(), 1)); break;
		case 2: rm8(rcl8(rm8(), 1)); break;
		case 3:	rm8(rcr8(rm8(), 1)); break;
		case 4: rm8(shl8(rm8(), 1)); break;
		case 5: rm8(shr8(rm8(), 1)); break;
		case 6:	rm8(shl8(rm8(), 1)); break;
		case 7: rm8(sar8(rm8(), 1)); break;
		}
		break;
	case 0xD1: // shift / rotate 16bit by 1
		MODRM16W;
		switch( modrm.reg )
		{
		case 0:	rm16(rol16(rm16(), 1)); break;
		case 1: rm16(ror16(rm16(), 1)); break;
		case 2: rm16(rcl16(rm16(), 1)); break;
		case 3:	rm16(rcr16(rm16(), 1)); break;
		case 4: rm16(shl16(rm16(), 1)); break;
		case 5: rm16(shr16(rm16(), 1)); break;
		case 6:	rm16(shl16(rm16(), 1)); break;
		case 7: rm16(sar16(rm16(), 1)); break;
		}
		break;
	case 0xD2: // shift / rotate 8bit by CL
		MODRM8W;
		switch( modrm.reg )
		{
		case 0:	rm8(rol8(rm8(), CL)); break;
		case 1: rm8(ror8(rm8(), CL)); break;
		case 2: rm8(rcl8(rm8(), CL)); break;
		case 3:	rm8(rcr8(rm8(), CL)); break;
		case 4: rm8(shl8(rm8(), CL)); break;
		case 5: rm8(shr8(rm8(), CL)); break;
		case 6:	rm8(shl8(rm8(), CL)); break;
		case 7: rm8(sar8(rm8(), CL)); break;
		}
		break;
	case 0xD3: // shift / rotate 16bit by CL
		MODRM16W;
		switch( modrm.reg )
		{
		case 0:	rm16(rol16(rm16(), CL)); break;
		case 1: rm16(ror16(rm16(), CL)); break;
		case 2: rm16(rcl16(rm16(), CL)); break;
		case 3:	rm16(rcr16(rm16(), CL)); break;
		case 4: rm16(shl16(rm16(), CL)); break;
		case 5: rm16(shr16(rm16(), CL)); break;
		case 6:	rm16(shl16(rm16(), CL)); break;
		case 7: rm16(sar16(rm16(), CL)); break;
		}
		break;
	case 0xD4:{ // aam
		t = imm8();
		if( t == 0 ) { divide_error(); return; }
		u8 tal = AL;
		AH = tal/t;
		AL = tal%t;
		psz8(AL);
		}break;
	case 0xD5: // aad
		t = imm8();
		AL = (AL + (AH*t));
		psz8(AL);
		AH = 0;
		break;
	case 0xD6: // salc (undocumented)
		AL = (F.b.C ? 0xff : 0);
		break;
	case 0xD7: // xlat/xlatb
		if( PDATA==0xff ) PDATA = 3;
		if( !validate_mem(seg[PDATA], desc[PDATA], (BX+AL)&0xffff, PDATA==2, false, true, 1) ) return;
		AL = read(desc[PDATA].base+((BX+AL)&0xffff), 8);
		break;
		
		
	case 0xDB: // fpu stuff
		modrm.set(imm8());
		lea();
		break;
		
	case 0xE0: // loopne rel8
		t = imm8();
		CX -= 1;
		if( F.b.Z == 0 && CX )
		{
			t = IP + s8(t);
			if( opsz == 16 ) t &= 0xffff;
			if( t > CS_DESC.limit ) { general_pfault(); return; }
			IP = t;		
		}
		break;
	case 0xE1: // loope rel8
		t = imm8();
		CX -= 1;
		if( F.b.Z == 1 && CX )
		{
			t = IP + s8(t);
			if( opsz == 16 ) t &= 0xffff;
			if( t > CS_DESC.limit ) { general_pfault(); return; }
			IP = t;		
		}
		break;
	case 0xE2: // loop rel8
		t = imm8();
		CX -= 1;
		if( CX )
		{
			t = IP + s8(t);
			if( opsz == 16 ) t &= 0xffff;
			if( t > CS_DESC.limit ) { general_pfault(); return; }
			IP = t;		
		}
		break;
	case 0xE3: // jcxz rel8
		t = imm8();
		if( CX == 0 )
		{
			IP += (s8)t;
			if( opsz == 16 ) IP &= 0xffff;
			if( IP > CS_DESC.limit ) { general_pfault(); }
		}
		break;
	case 0xE4: //in al, imm8
		if( inPMode() )
		{
			//todo: check IOPL and permmap
		}
		AL = in(imm8(), 8);
		break;
	case 0xE5: //in ax, imm8
		if( inPMode() )
		{
			//todo: check IOPL and permmap
		}
		AX = in(imm8(), 16);
		break;
	case 0xEC: //in al, dx
		if( inPMode() )
		{
			//todo: check IOPL and permmap
		}
		AL = in(DX, 8);
		break;
	case 0xED: //in ax, dx
		if( inPMode() )
		{
			//todo: check IOPL and permmap
		}
		AX = in(DX, 16);
		break;
	case 0xEE: //out dx, al
		if( inPMode() )
		{
			//todo: check IOPL and permmap
		}
		out(DX, AL, 8);
		break;
	case 0xEF: //out dx, ax
		if( inPMode() )
		{
			//todo: check IOPL and permmap
		}
		out(DX, AX, 16);
		break;
	case 0xE6: // out imm8, al
		if( inPMode() )
		{
			//todo: check IOPL and permmap
		}
		out(imm8(), AL, 8);
		break;
	case 0xE7: // out imm8, ax
		if( inPMode() )
		{
			//todo: check IOPL and permmap
		}
		out(imm8(), AX, 16);
		break;	
	case 0xE8: // call near rel16
		t = imm16();
		if( !push16(IP) ) { return; }
		t = IP + s16(t);
		if( opsz == 16 ) t &= 0xffff;
		if( t > CS_DESC.limit ) { general_pfault(); return; }
		IP = t;
		break;	
	case 0xE9: // jmp rel16
		t = imm16();
		t = IP + s16(t);
		if( opsz == 16 ) t &= 0xffff;
		if( t > CS_DESC.limit ) { general_pfault(); return; }
		IP = t;
		break;
	case 0xEA:{ // jmp far ptr16:16
		u16 offs = imm16();
		u16 sg = imm16();
		far_jump(sg, offs);		
		}break;
	case 0xEB: // jmp rel8
		t = imm8();
		t = IP + s8(t);
		if( opsz == 16 ) t &= 0xffff;
		if( t > CS_DESC.limit ) { general_pfault(); return; }
		IP = t;
		break;

	case 0xF4: // hlt (halt)
		if( !ring0() ) { general_pfault(); return; }
		halted = true;
		break;
	case 0xF5: F.b.C ^= 1; break; // cmc
	case 0xF6: // 8bit stuff
		modrm.set(imm8());
		switch( modrm.reg )
		{
		case 0: // test rm8, imm8
		case 1:
			lea();
			if( !validate_lea(false, true, 1) ) return;
			psz8(rm8()&imm8());
			F.b.C = F.b.O = F.b.A = 0;
			break;
		case 2: // not rm8
			lea();
			if( !validate_lea(true, true, 1) ) return;
			rm8(~rm8());
			break;
		case 3: // neg rm8
			lea();
			if( !validate_lea(true, true, 1) ) return;
			rm8(add8(0, rm8()^0xff, 1));
			F.b.C ^= 1;
			F.b.A ^= 1;
			break;
		case 4: // mul rm8
			lea();
			if( !validate_lea(true, true, 1) ) return;
			AX = AL * rm8();
			F.b.C = F.b.O = ( AH != 0 );
			F.b.A = 1;
			psz8(AH);
			break;
		case 5: // imul rm8
			lea();
			if( !validate_lea(true, true, 1) ) return;
			AX = s16(s8(AL)) * s16(s8(rm8()));
			F.b.C = F.b.O = ( AX != (u16)s8(AX) );
			F.b.A = 1;
			psz8(AH);
			break;
		case 6:{ // div rm8 (AX %/ rm8)
			lea();
			if( !validate_lea(true, true, 1) ) return;
			t = rm8();
			if( t == 0 ) { divide_error(); return; }
			u8 q = AX / t;
			u8 r = AX % t;
			AL = q;
			AH = r;
			psz8(AH);
			F.b.A = 1;
			}break;
		case 7:{ // idiv rm8 (AX %/ rm8)
			lea();
			if( !validate_lea(true, true, 1) ) return;
			t = rm8();
			if( t == 0 ) { divide_error(); return; }
			s16 q = s16(AX) / s8(t);
			s16 r = s16(AX) % s8(t);
			if( q > 127 || q < -128 ) { divide_error(); return; }
			AL = q;
			AH = r;
			psz8(AH);
			F.b.A = 1;
			}break;
		default:
			std::println("Unimpl opcode F6 /{:X}", u32(modrm.reg));
			exit(1);
		}
		break;
	case 0xF7: // 16bit stuff
		modrm.set(imm8());
		switch( modrm.reg )
		{
		case 0: // test rm16, imm16
		case 1:
			lea();
			if( !validate_lea(false, true, 2) ) return;
			psz16(rm16()&imm16());
			F.b.C = F.b.O = F.b.A = 0;
			break;
		case 2: // not rm16
			lea();
			if( !validate_lea(true, true, 2) ) return;
			rm16(~rm16());
			break;
		case 3: // neg rm16
			lea();
			if( !validate_lea(true, true, 2) ) return;
			rm16(add16(0, rm16()^0xffff, 1));
			F.b.C ^= 1;
			F.b.A ^= 1;
			break;
		case 4:{ // mul rm16
			lea();
			if( !validate_lea(true, true, 2) ) return;
			t = AX;
			t *= rm16();
			DX = t>>16;
			AX = t;
			F.b.C = F.b.O = ( DX != 0 );
			F.b.A = 1;
			psz16(DX);
			}break;
		case 5:{ // imul rm16
			lea();
			if( !validate_lea(true, true, 2) ) return;
			t = (s16)AX * (s16)rm16();
			DX = t>>16;
			AX = t;
			F.b.C = F.b.O = ( t != (u32)(s16)AX );
			F.b.A = 1;
			psz16(DX);
			}break;
		case 6:{ // div rm16
			lea();
			if( !validate_lea(true, true, 2) ) return;
			t = rm16();
			if( t == 0 ) { divide_error(); return; }			
			u32 v = (DX<<16)|AX;
			AX = v / t;
			DX = v % t;
			psz16(DX);
			F.b.A = 1;
			}break;
		case 7:{ // idiv rm16
			lea();
			if( !validate_lea(true, true, 2) ) return;
			t = rm16();
			if( t == 0 ) { divide_error(); return; }			
			u32 v = (DX<<16)|AX;
			AX = s32(v) / s16(t);
			DX = s32(v) % s16(t);
			psz16(DX);
			F.b.A = 1;
			}break;
		default:
			std::println("Unimpl opcode F7 /{:X}", u32(modrm.reg));
			exit(1);
		}
		break;
	case 0xF8: F.b.C = 0; break; // clc
	case 0xF9: F.b.C = 1; break; // stc
	
	case 0xFA: F.b.I = 0; break; // cli //todo: pmode checks
	case 0xFB: F.b.I = 1; irq_blocked = true; break; // sti	//todo: pmode checks
	case 0xFC: F.b.D = 0; break; // cld
	case 0xFD: F.b.D = 1; break; // std
	case 0xFE: // misc
		modrm.set(imm8());
		switch( modrm.reg )
		{
		case 0: // inc rm8
			lea();
			if( !validate_lea(true, true, 1) ) return;
			t = rm8();
			F.b.A = ((t&0xf)==0xf);
			F.b.O = (t==0x7f);
			t += 1;
			rm8(psz8(t));
			break;
		case 1: // dec rm8
			lea();
			if( !validate_lea(true, true, 1) ) return;
			t = rm8();
			F.b.A = ((t&0xf)==0);
			F.b.O = (t==0x80);
			t -= 1;
			rm8(psz8(t));
			break;
		default:
			std::println("Unimpl opc FE /{:X}", u32(modrm.reg));
			exit(1);
		}
		break;
	case 0xFF: // misc
		modrm.set(imm8());
		switch( modrm.reg )
		{
		case 0: // inc rm16
			lea();
			if( !validate_lea(true, true, 1) ) return;
			t = rm16();
			F.b.A = ((t&0xf)==0xf);
			F.b.O = (t==0x7fff);
			t += 1;
			rm16(psz16(t));
			break;
		case 1: // dec rm8
			lea();
			if( !validate_lea(true, true, 1) ) return;
			t = rm16();
			F.b.A = ((t&0xf)==0);
			F.b.O = (t==0x8000);
			t -= 1;
			rm16(psz16(t));
			break;
		case 2: // call near rm16
			lea();
			if( !validate_lea(false, true, 2) ) return;
			t = rm16();
			if( t > CS_DESC.limit ) { general_pfault(); return; }
			if( !push16(IP) ) { return; }
			IP = t;			
			break;
		case 3:{ // call far m16:16
			lea();
			if( !validate_lea(false, true, 2) ) return;
			if( !validate_mem(segment, desc[PDATA], (offset+2)&0xffff, PDATA==2, false, true, 2) ) return;
			u16 offs = read(desc[PDATA].base+offset, 16);
			u16 sg = read(desc[PDATA].base+((offset+2)&0xffff), 16);
			far_call(sg, offs);
			}break;
		case 4: // jmp rm16
			lea();
			if( !validate_lea(false, true, 2) ) return;
			t = rm16();
			if( t > CS_DESC.limit ) { general_pfault(); return; }
			IP = t;
			break;
		case 5:{ // jmp m16:16
			lea();
			if( !validate_lea(false, true, 2) ) return;
			if( !validate_mem(segment, desc[PDATA], (offset+2)&0xffff, PDATA==2, false, true, 2) ) return;
			u16 offs = read(desc[PDATA].base+offset, 16);
			u16 sg = read(desc[PDATA].base+((offset+2)&0xffff), 16);
			far_jump(sg, offs);
			}break;
		case 6: // push rm16
			lea();
			if( !validate_lea(false, true, 2) ) return;
			push16(rm16());
			break;
		default:
			std::println("Unimpl opc FF /{:X}", u32(modrm.reg));
			exit(1);
		}
		break;
	default:
		std::println("{:04X}:{:04X}: Unimpl opc ${:02X}", start_cs, start_ip, opc);
		//exit(1);
	}
}

void x286::ext0f()
{
	u32 t = 0;
	u8 opc = read(CS_DESC.base + IP, 8); IP += 1;
	
	switch( opc )
	{
	case 1:{ // bunch of stuff
		modrm.set(imm8());
		if( modrm.reg == 2 )
		{ // lgdt
			std::println("LGDT!");
			exit(1);
			if( !ring0() ) { general_pfault(); return; }
			lea();
			if( !validate_lea(false, true, 6) ) return;
			gdtr.limit = read16(linear);
			gdtr.base = read16(linear+2) | (read8(linear+4)<<16);
		} else if( modrm.reg == 3 ) {
			// lidt
			if( !ring0() ) { general_pfault(); return; }
			lea();
			if( !validate_lea(false, true, 6) ) return;
			idtr.limit = read16(linear);
			idtr.base = read16(linear+2) | (read8(linear+4)<<16);
		} else if( modrm.reg == 6 ) {
			// lmsw
			if( !ring0() ) { general_pfault(); return; }
			lea();
			if( !validate_lea(false, true, 2) ) return;
			u16 oldPE = cr[0]&1;
			cr[0] &= ~15;
			cr[0] |= rm16()&15;
			cr[0] |= oldPE;
		} else if( modrm.reg == 4 ) {
			// smsw
			lea();
			if( !validate_lea(true, true, 2) ) return;
			rm16(cr[0]);		
		}
		}break;
		
		
	case 0x40:
	case 0x41:
	case 0x42:
	case 0x43:
	case 0x44:
	case 0x45:
	case 0x46:
	case 0x47:
	case 0x48:
	case 0x49:
	case 0x4A:
	case 0x4B:
	case 0x4C:
	case 0x4D:
	case 0x4E:
	case 0x4F: // cmov r16, rm16
		modrm.set(imm8());
		lea();
		if( cond(opc&15) )
		{
			//todo: verify 16bit read
			r16(rm16());
		}
		break;
	
	case 0x80:
	case 0x81:
	case 0x82:
	case 0x83:
	case 0x84:
	case 0x85:
	case 0x86:
	case 0x87:
	case 0x88:
	case 0x89:
	case 0x8A:
	case 0x8B:
	case 0x8C:
	case 0x8D:
	case 0x8E:
	case 0x8F: // Jcc rel16
		t = imm16();
		if( cond(opc&15) )
		{
			u32 temp = IP + s16(t);
			if( opsz == 16 ) temp &= 0xffff;
			if( temp > CS_DESC.limit )
			{
				general_pfault();
			} else {
				IP = temp;
			}
		}
		break;
	case 0x90:
	case 0x91:
	case 0x92:
	case 0x93:
	case 0x94:
	case 0x95:
	case 0x96:
	case 0x97:
	case 0x98:
	case 0x99:
	case 0x9A:
	case 0x9B:
	case 0x9C:
	case 0x9D:
	case 0x9E:
	case 0x9F: // setcc rm8
		MODRM8W;
		rm8( cond(opc&15) );
		break;

	case 0xB2:{ // lss r16, m16:16
		modrm.set(imm8());
		lea();
		if( !validate_lea(false, true, 4) ) return;
		u16 offs = read(linear, 16);
		u16 sg = read(linear+2, 16);
		if( !setsr(2, sg) ) return;
		regs[modrm.reg].w = offs;	
		}break;

	default:
		std::println("{:04X}:{:04X}: Unimpl opc 0F {:02X}", start_cs, start_ip, opc);
		exit(1);
	}
}

bool x286::is_prefix(u8 p)
{
	switch(p)
	{
	case 0x26: PDATA = 0; return true; //es
	case 0x2E: PDATA = 1; return true; //cs
	case 0x36: PDATA = 2; return true; //ss
	case 0x3E: PDATA = 3; return true; //ds
	
	case 0xF2:
	case 0xF3: REP = p; return true;
	
	case 0xF0: LOCK = 1; return true;
	default: break;
	}
	return false;
}

bool x286::setsr(u32 sr, u16 v)
{
	if( cr[0] & 1 )
	{
		u32 doff = v&~7;
		bool ldt = v&BIT(2);
		
		if( sr == 2 )
		{ // SS checks a bit different
			if( (v&~3) == 0 )
			{ // SS can't be null selector
				general_pfault(0);
				return false;
			}

			if( (doff+5) > (ldt ? ldtr.limit : gdtr.limit) || (v&3) != CPL )
			{ // selector outside LDT/GDT limit or RPL != CPL
				general_pfault(v);
				return false;
			}
			doff += (ldt ? ldtr.base : gdtr.base);
		
			segdesc D;
			D.limit = read16(doff);
			D.base = read16(doff+2)|(read8(doff+4)<<8);
			D.access.v = read8(doff+5);
			
			if( D.access.b.dpl != CPL || ((D.access.v & 0x1A) != 0x12) )
			{  // DPL != CPL or not writable data segment
				general_pfault(v);
				return false;
			}
			
			if( ! D.access.b.p )
			{ // descriptor not present. manual psuedo-code has this *after* dpl!=cpl check
				stack_fault(v);
				return false;
			}
			
			seg[2] = v;
			desc[2] = D;			
			return false;
		}
		
		if( (v&~3) == 0 )
		{ // rest of SRs can get a null selector. (todo: no other tests here?)
			seg[sr] = v;
			return true;
		}
		
		if( (doff+5) > (ldt ? ldtr.limit : gdtr.limit) )
		{ // selector outside LDT/GDT limit
			general_pfault(v);
			return false;
		}	
		doff += (ldt ? ldtr.base : gdtr.base);

		segdesc D;
		D.limit = read16(doff);
		D.base = read16(doff+2)|(read8(doff+4)<<8);
		D.access.v = read8(doff+5);
		
		if( D.access.b.s == 0 || !((D.access.b.e==0) || (D.access.b.e && D.access.b.rw)) )
		{ // segment not data or readable code segment
			general_pfault(v);
			return false;
		}
				
		if( ((D.access.b.e==0) || (D.access.b.dc==0)) && 
			( (v&3) > D.access.b.dpl || CPL > D.access.b.dpl ) 
		  )
		{ // data or nonconforming code seg and ( rpl > dpl or cpl > dpl )
		  // readable conforming code segments do not priviledge-check selector loads
			general_pfault(v);
			return false;
		}

		if( ! D.access.b.p )
		{ // descriptor not present. manual psuedo-code has this *after* dpl!=cpl check
			segment_np(v);
			return false;
		}
		
		desc[sr] = D;
		seg[sr] = v;	
		return true;
	}
	
	desc[sr].base = v<<4;
	desc[sr].limit = 0xffff;
	seg[sr] = v;
	return true;
}

void x286::parity8(u8 v)
{
	F.b.P = std::popcount(v)^1;
}

u8 x286::imm8()
{
	u8 v = read8(CS_DESC.base + IP);
	IP += 1;
	return v;
}

u16 x286::imm16()
{
	u16 v = read8(CS_DESC.base + IP); IP += 1;
	v |= read8(CS_DESC.base + IP)<<8; IP += 1;
	return v;
}

bool x286::cond(u32 cc)
{
	cc &= 15;
	switch( cc )
	{
	case 0: return F.b.O==1;
	case 1: return F.b.O==0;
	case 2: return F.b.C==1;
	case 3: return F.b.C==0;
	case 4: return F.b.Z==1;
	case 5: return F.b.Z==0;	
	case 6: return F.b.C==1 || F.b.Z==1;
	case 7: return F.b.C==0 && F.b.Z==0;
	case 8: return F.b.S==1;
	case 9: return F.b.S==0;
	case 0xA: return F.b.P==1;
	case 0xB: return F.b.P==0;	
	case 0xC: return F.b.S!=F.b.O;
	case 0xD: return F.b.S==F.b.O;
	case 0xE: return F.b.Z==1 || F.b.S!=F.b.O;
	case 0xF: return F.b.Z==0 && F.b.S==F.b.O;
	default: break;
	}
	std::println("error in cond");
	exit(1);
	return false;
}

std::optional<u16> x286::pop16()
{
	if( !validate_mem(SS, SS_DESC, SP, true, false, true, 2) ) return {};
	u16 res = read(SS_DESC.base + SP, 16);
	SP += 2;
	return res;
}

bool x286::push16(u16 v)
{
	SP -= 2;
	if( !validate_mem(SS, SS_DESC, SP, true, true, true, 2) ) return false;
	//std::println("push16 v${:X} to s${:X}", v, SS_DESC.base + SP);
	write(SS_DESC.base + SP, v, 16);
	return true;
}

void x286::lea()
{
	isStack = (PDATA==2);
	
	if( modrm.mod == 0 && modrm.rm == 6 )
	{
		if( PDATA==0xff ) PDATA = 3;
		segment = seg[PDATA];
		offset = imm16();
		linear = desc[PDATA].base + offset;
	} else if( modrm.mod != 3 ) {
		if( PDATA==0xff ) PDATA = ((modrm.rm==2||modrm.rm==3||modrm.rm==6) ? 2 : 3);
		segment = seg[PDATA];
		isStack = (PDATA==2);
		offset = (modrm.mod ? (modrm.mod==2 ? imm16() : (s8)imm8()) : 0);
		switch( modrm.rm )
		{
		case 0: offset += BX + SI; break;
		case 1: offset += BX + DI; break;
		case 2: offset += BP + SI; break;
		case 3: offset += BP + DI; break;
		case 4: offset += SI; break;
		case 5: offset += DI; break;
		case 6: offset += BP; break;
		case 7: offset += BX; break;
		}
		offset &= 0xffff;
		linear = desc[PDATA].base + offset;
	}
	
	//std::println("linear = ${:X}", linear);	
}

void x286::general_pfault(u16 code)
{
	std::println("#GP");
	//*(int*)0 = 0xff;
}

void x286::stack_fault(u16 code)
{
	std::println("stack fault");
}

void x286::segment_np(u16 code)
{
}

void x286::divide_error()
{
}

u8 x286::add8(u8 a, u8 b, u8 c)
{
	u16 temp = a;
	temp += b;
	temp += c;
	F.b.C = temp>>8;
	F.b.Z = ((temp&0xff)==0);
	F.b.S = (temp&0x80)?1:0;
	F.b.O = ((temp^a)&(temp^b)&0x80)?1:0;
	parity8(temp);
	F.b.A = ((a&15)+(b&15)+c)>15?1:0;
	return temp;
}

u16 x286::add16(u16 a, u16 b, u16 c)
{
	u32 temp = a;
	temp += b;
	temp += c;
	F.b.C = temp>>16;
	F.b.Z = ((temp&0xffff)==0);
	F.b.S = (temp&0x8000)?1:0;
	F.b.O = ((temp^a)&(temp^b)&0x8000)?1:0;
	parity8(temp);
	F.b.A = ((a&15)+(b&15)+c)>15?1:0;
	return temp;
}

void x286::far_jump(u16 sg, u32 offs)
{
	if( !inPMode() )
	{
		CS = sg;
		CS_DESC.limit = 0xffff;
		CS_DESC.base = sg<<4;
		CS_DESC.access.v = 0x9B;
		IP = offs;
		return;
	}

	std::println("Protected mode far jump unimpl.");
	exit(1);
}

void x286::far_call(u16 sg, u32 offs)
{
	if( !validate_mem(SS, SS_DESC, (SP-4)&0xffff, true, true, true, 4) ) return;
	SP -= 2; write(SS_DESC.base + SP, CS, 16);
	SP -= 2; write(SS_DESC.base + SP, IP, 16);

	if( !inPMode() )
	{
		CS = sg;
		CS_DESC.base = sg<<4;
		IP = offs;
		if( IP > CS_DESC.limit ) { general_pfault(); return; }
		return;
	}
	
	std::println("Protected mode far call unimpl");
	exit(1);
}

u8 x286::shr8(u8 v, u32 a)
{
	a &= 0x1f;
	if( a == 0 ) return v;
	F.b.A = 1;
	if( a == 1 ) F.b.O = v>>7; else F.b.O = 0;
	v >>= a-1;
	F.b.C = v&1;
	v >>= 1;
	return psz8(v);
}

u16 x286::shr16(u16 v, u32 a)
{
	a &= 0x1f;
	if( a == 0 ) return v;
	if( a == 1 ) F.b.O = v>>15; else F.b.O = 0;
	v >>= a-1;
	F.b.C = v&1;
	v >>= 1;
	return psz16(v);
}

u8 x286::shl8(u8 v, u32 a)
{
	a &= 0x1f;
	if( a == 0 ) return v;
	v <<= a-1;
	F.b.C = v>>7;
	v <<= 1;
	F.b.A = 1;
	F.b.O = ((F.b.C != (v>>7))?1:0);
	return psz8(v);
}

u16 x286::shl16(u16 v, u32 a)
{
	a &= 0x1f;
	if( a == 0 ) return v;
	v <<= a-1;
	F.b.C = v>>15;
	v <<= 1;
	F.b.O = (F.b.C != (v>>15));
	return psz16(v);
}

u8 x286::sar8(u8 v, u32 a)
{
	a &= 0x1f;
	if( a == 0 ) return v;
	F.b.O = 0;
	F.b.A = 1;
	v = s8(v) >> (a-1);
	F.b.C = v&1;
	v = s8(v) >> 1;
	return psz8(v);
}

u16 x286::sar16(u16 v, u32 a)
{
	a &= 0x1f;
	if( a == 0 ) return v;
	F.b.O = 0;
	F.b.A = 1;
	v = s16(v) >> (a-1);
	F.b.C = v&1;
	v = s16(v) >> 1;
	return psz16(v);
}

u8 x286::rol8(u8 v, u32 a)
{
	a &= 0x1f;
	if( a == 0 ) return v;
	v = std::rotl(v, a);
	F.b.C = v&1;
	F.b.O = F.b.C ^ (v>>7);
	return v;
}

u8 x286::ror8(u8 v, u32 a)
{
	a &= 0x1f;
	if( a == 0 ) return v;
	v = std::rotr(v, a);
	F.b.C = v>>7;
	F.b.O = ((v>>6)&1) ^ (v>>7);
	return v;
}

u8 x286::rcl8(u8 v, u32 a)
{
	a &= 0x1f;
	if( a == 0 ) return v;
	for(u32 i = 0; i < a; ++i)
	{
		u8 c = F.b.C;
		F.b.C = v >> 7;
		v = (v<<1)|c;
	}
	F.b.O = F.b.C ^ (v>>7);
	return v;
}

u8 x286::rcr8(u8 v, u32 a)
{
	a &= 0x1f;
	if( a == 0 ) return v;
	for(u32 i = 0; i < a; ++i)
	{
		u8 c = F.b.C<<7;
		F.b.C = v & 1;
		v = (v>>1)|c;
	}
	F.b.O = ((v>>6)&1) ^ (v>>7);
	return v;
}

u16 x286::rol16(u16 v, u32 a)
{
	a &= 0x1f;
	if( a == 0 ) return v;
	v = std::rotl(v, a);
	F.b.C = v&1;
	F.b.O = F.b.C ^ (v>>15);
	return v;
}

u16 x286::ror16(u16 v, u32 a)
{
	a &= 0x1f;
	if( a == 0 ) return v;
	v = std::rotr(v, a);
	F.b.C = v>>15;
	F.b.O = ((v>>14)&1) ^ (v>>15);
	return v;
}

u16 x286::rcl16(u16 v, u32 a)
{
	a &= 0x1f;
	if( a == 0 ) return v;
	for(u32 i = 0; i < a; ++i)
	{
		u8 c = F.b.C;
		F.b.C = v >> 15;
		v = (v<<1)|c;
	}
	F.b.O = F.b.C ^ (v>>15);
	return v;
}

u16 x286::rcr16(u16 v, u32 a)
{
	a &= 0x1f;
	if( a == 0 ) return v;
	for(u32 i = 0; i < a; ++i)
	{
		u16 c = F.b.C<<15;
		F.b.C = v & 1;
		v = (v>>1)|c;
	}
	F.b.O = ((v>>14)&1) ^ (v>>15);
	return v;
}

void x286::interrupt(u8 n)
{
	if( !inPMode() )
	{
		bool db = push16(F.v) && push16(CS) && push16(IP);
		if( !db )
		{
			double_fault();
			return;
		}
		
		u32 vec = n*4;
		if( vec + 3 > idtr.limit ) { general_pfault(); return; }
		u16 offs = read(vec, 16);
		u16 sg = read(vec+2, 16);
		CS = sg;
		CS_DESC.base = sg<<4;
		IP = offs;
		return;
	}
	
	std::println("Interrupt in PMode unimpl");
	exit(1);
}

void x286::double_fault()
{
	std::println("double fault");
	exit(1);
	current_exception = 8;
	interrupt(8);
}

void x286::far_ret(u16 s, u32 o, u32 f)
{
	if( !inPMode() )
	{
		CS = s;
		CS_DESC.base = s<<4;
		IP = o;
		F.v = f;
		F.b.pad1 = 1; F.b.pad3 = F.b.pad5 = 0;
		if( IP > CS_DESC.limit ) { general_pfault(); return; }
		return;
	}
	
	std::println("far ret in pmode unimpl");
	exit(1);
}

