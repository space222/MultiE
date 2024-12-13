// 6502 (and eventually at least 65C02 and maybe HuC6280) implementation as a C++20 coroutine
// Intention is to be at least bus-activity-level accurate (read/write called on the right cycle with correct address). 
// Internal operations otherwise may not match hardware.

#include <iostream>
#include <coroutine>
#include <functional>
#include "6502coru.h"

#define CYCLE co_yield opc
#define NEWCPU (cpu_type > CPU_6502)

Yieldable coru6502::run()
{
	u16 temp = 0;
	u8 t = 0;
	F.v = 0x34;
	irq_line = nmi_line = waiting = false;
	if( cpu_type == CPU_HUC6280 )
	{
		pc = read(0xfffe);
		pc |= read(0xffff)<<8;
	} else {
		pc = read(0xfffc);
		pc |= read(0xfffd)<<8;
	}
	
	while(true)
	{
		u8 I_FLAG = F.b.I;
		u8 opc = 0;
		
	if( !waiting ) {
		opc = read(pc);
		pc += 1;
		CYCLE;
		u8 oldC = F.b.C;		
		const u8 T = ( (cpu_type==CPU_HUC6280) ? F.b.t : 0 );
		F.b.t = (( cpu_type==CPU_HUC6280 ) ? 0 : 1);
		
		switch( opc )
		{	
		case 0x08: // php
			read(pc);
			CYCLE;
			write(0x100+s, F.v|0x30);
			s -= 1;
			CYCLE;
			break;
		
		case 0x10: // bpl
			temp = read(pc++);
			CYCLE;
			if( F.b.N == 0 )
			{
				temp = pc + (s8)temp;
				CYCLE;
				if( (temp&0xff00) != (pc&0xff00) ) 
				{
					CYCLE;
				}
				pc = temp;
			}
			break;
		
		case 0x18: // clc
			F.b.C = 0;
			CYCLE;
			break;
			
		case 0x20: // jsr
			t = read(pc++);
			CYCLE; // 2
			read(0x100+s); // ??
			CYCLE; // 3
			write(0x100+s, pc>>8);
			s -= 1;
			CYCLE; // 4
			write(0x100+s, pc);
			s -= 1;
			CYCLE; // 5
			pc = t | (read(pc)<<8);
			CYCLE; // 6
			break;
			
		case 0x24: // bit zp
			t = read(pc++);
			CYCLE;
			temp = read(t);
			F.b.Z = (((temp&a)==0)?1:0);
			F.b.N = temp>>7;
			F.b.V = (temp>>6)&1;
			CYCLE;
			break;
			
		case 0x28: // plp
			read(pc);
			CYCLE;
			s += 1;
			CYCLE;
			F.v = read(0x100+s);
			F.b.b = 0;
			CYCLE;
			break;
//--		
		case 0x29: // and imm
			a &= read(pc++);
			setnz(a);
			CYCLE;
			break;
		case 0x25: // and zp
			temp = read(pc++);
			CYCLE;
			t = read(temp);
			a &= t;
			setnz(a);
			CYCLE;
			break;
		case 0x35: // and zp, x
			temp = read(pc++);
			CYCLE;
			read(temp);
			temp = (temp+x)&0xff;
			CYCLE;
			t = read(temp);
			a &= t;
			setnz(a);
			CYCLE;		
			break;
		case 0x2D: // and abs
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read(temp);
			a &= t;
			setnz(a);
			CYCLE;		
			break;
		case 0x3D: // and abs, x
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read((temp&0xff00)+((temp+x)&0xff));
			if( (temp&0xff00) != ((temp+x)&0xff00) )
			{
				CYCLE;
				t = read(temp+x);
			}
			a &= t;
			setnz(a);
			CYCLE;
			break;
		case 0x39: // and abs, y
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read((temp&0xff00)+((temp+y)&0xff));
			if( (temp&0xff00) != ((temp+y)&0xff00) )
			{
				CYCLE;
				t = read(temp+y);
			}
			a &= t;
			setnz(a);
			CYCLE;		
			break;
		case 0x21: // and (ind, x)
			t = read(pc++);
			CYCLE; // 2
			read(t);
			t += x;
			CYCLE; // 3
			temp = read(t++);
			CYCLE; // 4
			temp |= read(t)<<8;
			CYCLE; // 5
			a &= read(temp);
			setnz(a);
			CYCLE; // 6		
			break;
		case 0x31: // and (ind),y
			t = read(pc++);
			CYCLE; // 2
			temp = read(t++);
			CYCLE; // 3
			temp |= read(t)<<8;
			CYCLE; // 4
			t = read((temp&0xff00)+((temp+y)&0xff));
			if( (temp&0xff00) != ((temp+y)&0xff00) )
			{
				CYCLE;  // 5
				t = read(temp+y);
			}
			a &= t;
			setnz(a);
			CYCLE; // 6
			break;
//--	
		case 0x09: // ora imm
			a |= read(pc++);
			setnz(a);
			CYCLE;
			break;
		case 0x05: // ora zp
			temp = read(pc++);
			CYCLE;
			t = read(temp);
			a |= t;
			setnz(a);
			CYCLE;
			break;
		case 0x15: // ora zp, x
			temp = read(pc++);
			CYCLE;
			read(temp);
			temp = (temp+x)&0xff;
			CYCLE;
			t = read(temp);
			a |= t;
			setnz(a);
			CYCLE;		
			break;
		case 0x0D: // ora abs
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read(temp);
			a |= t;
			setnz(a);
			CYCLE;		
			break;
		case 0x1D: // ora abs, x
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read((temp&0xff00)+((temp+x)&0xff));
			if( (temp&0xff00) != ((temp+x)&0xff00) )
			{
				CYCLE;
				t = read(temp+x);
			}
			a |= t;
			setnz(a);
			CYCLE;
			break;
		case 0x19: // ora abs, y
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read((temp&0xff00)+((temp+y)&0xff));
			if( (temp&0xff00) != ((temp+y)&0xff00) )
			{
				CYCLE;
				t = read(temp+y);
			}
			a |= t;
			setnz(a);
			CYCLE;		
			break;
		case 0x01: // ora (ind, x)
			t = read(pc++);
			CYCLE; // 2
			read(t);
			t += x;
			CYCLE; // 3
			temp = read(t++);
			CYCLE; // 4
			temp |= read(t)<<8;
			CYCLE; // 5
			a |= read(temp);
			setnz(a);
			CYCLE; // 6		
			break;
		case 0x11: // ora (ind),y
			t = read(pc++);
			CYCLE; // 2
			temp = read(t++);
			CYCLE; // 3
			temp |= read(t)<<8;
			CYCLE; // 4
			t = read((temp&0xff00)+((temp+y)&0xff));
			if( (temp&0xff00) != ((temp+y)&0xff00) )
			{
				CYCLE;  // 5
				t = read(temp+y);
			}
			a |= t;
			setnz(a);
			CYCLE; // 6
			break;
//--
		case 0x49: // eor imm
			a ^= read(pc++);
			setnz(a);
			CYCLE;
			break;
		case 0x45: // eor zp
			temp = read(pc++);
			CYCLE;
			t = read(temp);
			a ^= t;
			setnz(a);
			CYCLE;
			break;
		case 0x55: // eor zp, x
			temp = read(pc++);
			CYCLE;
			read(temp);
			temp = (temp+x)&0xff;
			CYCLE;
			t = read(temp);
			a ^= t;
			setnz(a);
			CYCLE;		
			break;
		case 0x4D: // eor abs
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read(temp);
			a ^= t;
			setnz(a);
			CYCLE;		
			break;
		case 0x5D: // eor abs, x
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read((temp&0xff00)+((temp+x)&0xff));
			if( (temp&0xff00) != ((temp+x)&0xff00) )
			{
				CYCLE;
				t = read(temp+x);
			}
			a ^= t;
			setnz(a);
			CYCLE;
			break;
		case 0x59: // eor abs, y
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read((temp&0xff00)+((temp+y)&0xff));
			if( (temp&0xff00) != ((temp+y)&0xff00) )
			{
				CYCLE;
				t = read(temp+y);
			}
			a ^= t;
			setnz(a);
			CYCLE;		
			break;
		case 0x41: // eor (ind, x)
			t = read(pc++);
			CYCLE; // 2
			read(t);
			t += x;
			CYCLE; // 3
			temp = read(t++);
			CYCLE; // 4
			temp |= read(t)<<8;
			CYCLE; // 5
			a ^= read(temp);
			setnz(a);
			CYCLE; // 6		
			break;
		case 0x51: // eor (ind),y
			t = read(pc++);
			CYCLE; // 2
			temp = read(t++);
			CYCLE; // 3
			temp |= read(t)<<8;
			CYCLE; // 4
			t = read((temp&0xff00)+((temp+y)&0xff));
			if( (temp&0xff00) != ((temp+y)&0xff00) )
			{
				CYCLE;  // 5
				t = read(temp+y);
			}
			a ^= t;
			setnz(a);
			CYCLE; // 6
			break;
//--
		case 0xA9: // lda imm
			a = read(pc++);
			setnz(a);
			CYCLE;
			break;
		case 0xA5: // lda zp
			temp = read(pc++);
			CYCLE;
			t = read(temp);
			a = t;
			setnz(a);
			CYCLE;
			break;
		case 0xB5: // lda zp, x
			temp = read(pc++);
			CYCLE;
			read(temp);
			temp = (temp+x)&0xff;
			CYCLE;
			t = read(temp);
			a = t;
			setnz(a);
			CYCLE;		
			break;
		case 0xAD: // lda abs
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read(temp);
			a = t;
			setnz(a);
			CYCLE;		
			break;
		case 0xBD: // lda abs, x
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read((temp&0xff00)+((temp+x)&0xff));
			if( (temp&0xff00) != ((temp+x)&0xff00) )
			{
				CYCLE;
				t = read(temp+x);
			}
			a = t;
			setnz(a);
			CYCLE;
			break;
		case 0xB9: // lda abs, y
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read((temp&0xff00)+((temp+y)&0xff));
			if( (temp&0xff00) != ((temp+y)&0xff00) )
			{
				CYCLE;
				t = read(temp+y);
			}
			a = t;
			setnz(a);
			CYCLE;		
			break;
		case 0xA1: // lda (ind, x)
			t = read(pc++);
			CYCLE; // 2
			read(t);
			t += x;
			CYCLE; // 3
			temp = read(t++);
			CYCLE; // 4
			temp |= read(t)<<8;
			CYCLE; // 5
			a = read(temp);
			setnz(a);
			CYCLE; // 6		
			break;
		case 0xB1: // lda (ind),y
			t = read(pc++);
			CYCLE; // 2
			temp = read(t++);
			CYCLE; // 3
			temp |= read(t)<<8;
			CYCLE; // 4
			t = read((temp&0xff00)+((temp+y)&0xff));
			if( (temp&0xff00) != ((temp+y)&0xff00) )
			{
				CYCLE;  // 5
				t = read(temp+y);
			}
			a = t;
			setnz(a);
			CYCLE; // 6
			break;
//--
		case 0xC9: // cmp imm
			cmp(a, read(pc++));
			CYCLE;
			break;
		case 0xC5: // cmp zp
			temp = read(pc++);
			CYCLE;
			t = read(temp);
			cmp(a, t);
			CYCLE;
			break;
		case 0xD5: // cmp zp, x
			temp = read(pc++);
			CYCLE;
			read(temp);
			temp = (temp+x)&0xff;
			CYCLE;
			t = read(temp);
			cmp(a, t);
			CYCLE;		
			break;
		case 0xCD: // cmp abs
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read(temp);
			cmp(a, t);
			CYCLE;		
			break;
		case 0xDD: // cmp abs, x
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read((temp&0xff00)+((temp+x)&0xff));
			if( (temp&0xff00) != ((temp+x)&0xff00) )
			{
				CYCLE;
				t = read(temp+x);
			}
			cmp(a, t);
			CYCLE;
			break;
		case 0xD9: // cmp abs, y
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read((temp&0xff00)+((temp+y)&0xff));
			if( (temp&0xff00) != ((temp+y)&0xff00) )
			{
				CYCLE;
				t = read(temp+y);
			}
			cmp(a, t);
			CYCLE;		
			break;
		case 0xC1: // cmp (ind, x)
			t = read(pc++);
			CYCLE; // 2
			read(t);
			t += x;
			CYCLE; // 3
			temp = read(t++);
			CYCLE; // 4
			temp |= read(t)<<8;
			CYCLE; // 5
			cmp(a, read(temp));
			CYCLE; // 6		
			break;
		case 0xD1: // cmp (ind),y
			t = read(pc++);
			CYCLE; // 2
			temp = read(t++);
			CYCLE; // 3
			temp |= read(t)<<8;
			CYCLE; // 4
			t = read((temp&0xff00)+((temp+y)&0xff));
			if( (temp&0xff00) != ((temp+y)&0xff00) )
			{
				CYCLE;  // 5
				t = read(temp+y);
			}
			cmp(a, t);
			CYCLE; // 6
			break;	
//--
		case 0x69: // adc imm
			a = add(a, read(pc++));
			CYCLE;
			if( NEWCPU && F.b.D ) CYCLE;
			break;
		case 0x65: // adc zp
			temp = read(pc++);
			CYCLE;
			t = read(temp);
			a = add(a, t);
			CYCLE;
			if( NEWCPU && F.b.D ) CYCLE;
			break;
		case 0x75: // adc zp, x
			temp = read(pc++);
			CYCLE;
			read(temp);
			temp = (temp+x)&0xff;
			CYCLE;
			t = read(temp);
			a = add(a, t);
			CYCLE;		
			if( NEWCPU && F.b.D ) CYCLE;
			break;
		case 0x6D: // adc abs
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read(temp);
			a = add(a, t);
			CYCLE;		
			if( NEWCPU && F.b.D ) CYCLE;
			break;
		case 0x7D: // adc abs, x
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read((temp&0xff00)+((temp+x)&0xff));
			if( (temp&0xff00) != ((temp+x)&0xff00) )
			{
				CYCLE;
				t = read(temp+x);
			}
			a = add(a, t);
			CYCLE;
			if( NEWCPU && F.b.D ) CYCLE;
			break;
		case 0x79: // adc abs, y
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read((temp&0xff00)+((temp+y)&0xff));
			if( (temp&0xff00) != ((temp+y)&0xff00) )
			{
				CYCLE;
				t = read(temp+y);
			}
			a = add(a, t);
			CYCLE;		
			if( NEWCPU && F.b.D ) CYCLE;
			break;
		case 0x61: // adc (ind, x)
			t = read(pc++);
			CYCLE; // 2
			read(t);
			t += x;
			CYCLE; // 3
			temp = read(t++);
			CYCLE; // 4
			temp |= read(t)<<8;
			CYCLE; // 5
			a = add(a, read(temp));
			CYCLE; // 6		
			if( NEWCPU && F.b.D ) CYCLE;
			break;
		case 0x71: // adc (ind),y
			t = read(pc++);
			CYCLE; // 2
			temp = read(t++);
			CYCLE; // 3
			temp |= read(t)<<8;
			CYCLE; // 4
			t = read((temp&0xff00)+((temp+y)&0xff));
			if( (temp&0xff00) != ((temp+y)&0xff00) )
			{
				CYCLE;  // 5
				t = read(temp+y);
			}
			a = add(a, t);
			CYCLE; // 6
			if( NEWCPU && F.b.D ) CYCLE;
			break;	
//--
		case 0xE9: // sbc imm
			a = add(a, ~read(pc++));
			CYCLE;
			if( NEWCPU && F.b.D ) CYCLE;
			break;
		case 0xE5: // sbc zp
			temp = read(pc++);
			CYCLE;
			t = read(temp);
			a = add(a, ~t);
			CYCLE;
			if( NEWCPU && F.b.D ) CYCLE;
			break;
		case 0xF5: // sbc zp, x
			temp = read(pc++);
			CYCLE;
			read(temp);
			temp = (temp+x)&0xff;
			CYCLE;
			t = read(temp);
			a = add(a, ~t);
			CYCLE;		
			if( NEWCPU && F.b.D ) CYCLE;
			break;
		case 0xED: // sbc abs
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read(temp);
			a = add(a, ~t);
			CYCLE;		
			if( NEWCPU && F.b.D ) CYCLE;
			break;
		case 0xFD: // sbc abs, x
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read((temp&0xff00)+((temp+x)&0xff));
			if( (temp&0xff00) != ((temp+x)&0xff00) )
			{
				CYCLE;
				t = read(temp+x);
			}
			a = add(a, ~t);
			CYCLE;
			if( NEWCPU && F.b.D ) CYCLE;
			break;
		case 0xF9: // sbc abs, y
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read((temp&0xff00)+((temp+y)&0xff));
			if( (temp&0xff00) != ((temp+y)&0xff00) )
			{
				CYCLE;
				t = read(temp+y);
			}
			a = add(a, ~t);
			CYCLE;		
			if( NEWCPU && F.b.D ) CYCLE;
			break;
		case 0xE1: // sbc (ind, x)
			t = read(pc++);
			CYCLE; // 2
			read(t);
			t += x;
			CYCLE; // 3
			temp = read(t++);
			CYCLE; // 4
			temp |= read(t)<<8;
			CYCLE; // 5
			a = add(a, ~read(temp));
			CYCLE; // 6		
			if( NEWCPU && F.b.D ) CYCLE;
			break;
		case 0xF1: // sbc (ind),y
			t = read(pc++);
			CYCLE; // 2
			temp = read(t++);
			CYCLE; // 3
			temp |= read(t)<<8;
			CYCLE; // 4
			t = read((temp&0xff00)+((temp+y)&0xff));
			if( (temp&0xff00) != ((temp+y)&0xff00) )
			{
				CYCLE;  // 5
				t = read(temp+y);
			}
			a = add(a, ~t);
			CYCLE; // 6
			if( NEWCPU && F.b.D ) CYCLE;
			break;	
//--
		case 0x2C: // bit abs
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read(temp);
			F.b.Z = (((t&a)==0)?1:0);
			F.b.N = t>>7;
			F.b.V = (t>>6)&1;
			CYCLE;
			break;
		case 0x30: // bmi
			temp = read(pc++);
			CYCLE;
			if( F.b.N == 1 )
			{
				temp = pc + (s8)temp;
				CYCLE;
				if( (temp&0xff00) != (pc&0xff00) ) 
				{
					CYCLE;
				}
				pc = temp;
			}
			break;
			
		case 0x38: // sec
			F.b.C = 1;
			CYCLE;
			break;
			
		case 0x40: // rti
			s += 1;
			F.v = read(0x100+s);
			CYCLE;
			s += 1;
			temp = read(0x100+s);
			CYCLE;
			s += 1;
			CYCLE;
			temp |= read(0x100+s)<<8;
			CYCLE;
			pc = temp;
			CYCLE;
			I_FLAG = F.b.I;
			break;
			
		case 0x48: // pha
			read(pc);
			CYCLE;
			write(0x100+s, a);
			s -= 1;
			CYCLE;
			break;
			
		case 0x4C: // jmp abs
			temp = read(pc);
			CYCLE;
			temp |= read(pc+1)<<8;
			pc = temp;
			CYCLE;
			break;
		case 0x6C: // jmp (ind)
			temp = read(pc++);
			CYCLE; // 2
			temp |= read(pc)<<8;
			CYCLE; // 3
			pc = read(temp);
			CYCLE; // 4
			if( cpu_type > CPU_6502 )
			{
				pc |= read(temp+1)<<8;			
			} else {
				pc |= read((temp&0xff00)+((temp+1)&0xff))<<8;
			}
			CYCLE; // 5
			break;
			
		case 0x50: // bvc
			temp = read(pc++);
			CYCLE;
			if( F.b.V == 0 )
			{
				temp = pc + (s8)temp;
				CYCLE;
				if( (temp&0xff00) != (pc&0xff00) ) 
				{
					CYCLE;
				}
				pc = temp;
			}
			break;
		
		case 0x58: // cli
			F.b.I = 0;
			CYCLE;
			break;
			
		case 0x60: // rts
			s += 1;
			CYCLE;
			temp = read(0x100+s);
			CYCLE;
			s += 1;
			CYCLE;
			temp |= read(0x100+s)<<8;
			CYCLE;
			pc = temp+1;
			CYCLE;
			break;
			
		case 0x68: // pla
			read(pc);
			CYCLE;
			s += 1;
			CYCLE;
			a = read(0x100+s);
			setnz(a);
			CYCLE;
			break;
			
		case 0x70: // bvs
			temp = read(pc++);
			CYCLE;
			if( F.b.V == 1 )
			{
				temp = pc + (s8)temp;
				CYCLE;
				if( (temp&0xff00) != (pc&0xff00) ) 
				{
					CYCLE;
				}
				pc = temp;
			}
			break;
		
		case 0x78: // sei
			F.b.I = 1;
			CYCLE;
			break;
			
		case 0x88: // dey
			y -= 1;
			setnz(y);
			CYCLE;
			break;

		case 0x8A: // txa
			a = x;
			setnz(a);
			CYCLE;
			break;
			
		case 0x90: // bcc
			temp = read(pc++);
			CYCLE;
			if( F.b.C == 0 )
			{
				temp = pc + (s8)temp;
				CYCLE;
				if( (temp&0xff00) != (pc&0xff00) ) 
				{
					CYCLE;
				}
				pc = temp;
			}
			break;

		case 0x98: // tya
			a = y;
			setnz(a);
			CYCLE;
			break;
			
		case 0x9A: // txs
			s = x;
			CYCLE;
			break;
					
		case 0xA8: // tay
			y = a;
			setnz(y);
			CYCLE;
			break;

		case 0xAA: // tax
			x = a;
			setnz(x);
			CYCLE;
			break;

		case 0xB0: // bcs
			temp = read(pc++);
			CYCLE;
			if( F.b.C == 1 )
			{
				temp = pc + (s8)temp;
				CYCLE;
				if( (temp&0xff00) != (pc&0xff00) ) 
				{
					CYCLE;
				}
				pc = temp;
			}
			break;
			
		case 0xB8: // clv
			F.b.V = 0;
			CYCLE;
			break;
			
		case 0xBA: // tsx
			x = s;
			setnz(x);
			CYCLE;
			break;
			
		case 0xC6: // dec zp
			temp = read(pc++);
			CYCLE;
			t = read(temp);
			CYCLE;
			write(temp, t);
			t -= 1;
			setnz(t);
			CYCLE;
			write(temp, t);
			CYCLE;
			break;
		case 0xD6: // dec zp, x
			temp = read(pc++);
			CYCLE;
			read(temp);
			temp = (temp+x)&0xff;
			CYCLE;
			t = read(temp);
			CYCLE;
			write(temp, t);
			t -= 1;
			setnz(t);
			CYCLE;
			write(temp, t);
			CYCLE;		
			break;
		case 0xCE: // dec abs
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read(temp);
			CYCLE;
			write(temp, t);
			t -= 1;
			setnz(t);
			CYCLE;
			write(temp, t);
			CYCLE;
			break;
		case 0xDE: // dec abs, x
			temp = read(pc++);
			CYCLE; // 2
			temp |= read(pc++)<<8;
			CYCLE; // 3
			read((temp&0xff00)+((temp+x)&0xff));
			temp += x;
			CYCLE; // 4
			t = read(temp);
			CYCLE; // 5
			write(temp, t);
			t -= 1;
			setnz(t);
			CYCLE; // 6
			write(temp, t);
			CYCLE; // 7		
			break;
			
		case 0xC8: // iny
			y += 1;
			setnz(y);
			CYCLE;
			break;		
			
		case 0xCA: // dex
			x -= 1;
			setnz(x);
			CYCLE;
			break;
			
		case 0xD0: // bne
			temp = read(pc++);
			CYCLE;
			if( F.b.Z == 0 )
			{
				temp = pc + (s8)temp;
				CYCLE;
				if( (temp&0xff00) != (pc&0xff00) ) 
				{
					CYCLE;
				}
				pc = temp;
			}
			break;
			
		case 0xD8: // cld
			F.b.D = 0;
			CYCLE;
			break;
			
		case 0xE0: // cpx imm
			cmp(x, read(pc++));
			CYCLE;
			break;
		case 0xE4: // cpx zp
			temp = read(pc++);
			CYCLE;
			cmp(x, read(temp));
			CYCLE;
			break;
		case 0xEC: // cpx abs
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			cmp(x, read(temp));
			CYCLE;
			break;
		case 0xC0: // cpy imm
			cmp(y, read(pc++));
			CYCLE;
			break;
		case 0xC4: // cpy zp
			temp = read(pc++);
			CYCLE;
			cmp(y, read(temp));
			CYCLE;
			break;
		case 0xCC: // cpy abs
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			cmp(y, read(temp));
			CYCLE;
			break;
			
		case 0xE6: // inc zp
			temp = read(pc++);
			CYCLE; //2
			t = read(temp);
			CYCLE; //3
			write(temp, t);
			CYCLE; //4
			write(temp, ++t);
			setnz(t);
			CYCLE; //5
			break;
		case 0xF6: // inc zp, x
			temp = read(pc++);
			CYCLE;
			read(temp);
			temp = (temp+x)&0xff;
			CYCLE;
			t = read(temp);
			CYCLE;
			write(temp, t);
			t += 1;
			setnz(t);
			CYCLE;
			write(temp, t);
			CYCLE;		
			break;
		case 0xEE: // inc abs
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read(temp);
			CYCLE;
			write(temp, t);
			t += 1;
			setnz(t);
			CYCLE;
			write(temp, t);
			CYCLE;
			break;
		case 0xFE: // inc abs, x
			temp = read(pc++);
			CYCLE; // 2
			temp |= read(pc++)<<8;
			CYCLE; // 3
			read((temp&0xff00)+((temp+x)&0xff));
			temp += x;
			CYCLE; // 4
			t = read(temp);
			CYCLE; // 5
			write(temp, t);
			t += 1;
			setnz(t);
			CYCLE; // 6
			write(temp, t);
			CYCLE; // 7		
			break;
			
		case 0xE8: // inx
			x += 1;
			setnz(x);
			CYCLE;
			break;
		
		case 0xEA: // nop
			CYCLE;
			break;
			
		case 0xF0: // beq
			temp = read(pc++);
			CYCLE;
			if( F.b.Z == 1 )
			{
				temp = pc + (s8)temp;
				CYCLE;
				if( (temp&0xff00) != (pc&0xff00) ) 
				{
					CYCLE;
				}
				pc = temp;
			}
			break;
			
		case 0xF8: // sed
			F.b.D = 1;
			CYCLE;
			break;
			
			
		case 0x4A: // lsr a
			F.b.C = a&1;
			a >>= 1;
			setnz(a);
			CYCLE;
			break;
		case 0x46: // lsr zp
			temp = read(pc++);
			CYCLE;
			t = read(temp);
			CYCLE;
			write(temp, t);
			F.b.C = t&1;
			t >>= 1;
			setnz(t);
			CYCLE;
			write(temp, t);
			CYCLE;
			break;
		case 0x56: // lsr zp, x
			temp = read(pc++);
			CYCLE;
			read(temp);
			temp = (temp+x)&0xff;
			CYCLE;
			t = read(temp);
			CYCLE;
			write(temp, t);
			F.b.C = t&1;
			t >>= 1;
			setnz(t);
			CYCLE;
			write(temp, t);
			CYCLE;
			break;
		case 0x4E: // lsr abs
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read(temp);
			CYCLE;
			write(temp, t);
			F.b.C = t&1;
			t >>= 1;
			setnz(t);
			CYCLE;
			write(temp, t);
			CYCLE;
			break;
		case 0x5E: // lsr abs, x
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			read((temp&0xff00)+((temp+x)&0xff));
			temp += x;
			CYCLE;
			t = read(temp);
			CYCLE;
			write(temp, t);
			F.b.C = t&1;
			t >>= 1;
			setnz(t);
			CYCLE;
			write(temp, t);
			CYCLE;
			break;
//--
		case 0x0A: // asl a
			F.b.C = a>>7;
			a <<= 1;
			setnz(a);
			CYCLE;
			break;
		case 0x06: // asl zp
			temp = read(pc++);
			CYCLE;
			t = read(temp);
			CYCLE;
			write(temp, t);
			F.b.C = t>>7;
			t <<= 1;
			setnz(t);
			CYCLE;
			write(temp, t);
			CYCLE;
			break;
		case 0x16: // asl zp, x
			temp = read(pc++);
			CYCLE;
			read(temp);
			temp = (temp+x)&0xff;
			CYCLE;
			t = read(temp);
			CYCLE;
			write(temp, t);
			F.b.C = t>>7;
			t <<= 1;
			setnz(t);
			CYCLE;
			write(temp, t);
			CYCLE;
			break;
		case 0x0E: // asl abs
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read(temp);
			CYCLE;
			write(temp, t);
			F.b.C = t>>7;
			t <<= 1;
			setnz(t);
			CYCLE;
			write(temp, t);
			CYCLE;
			break;
		case 0x1E: // asl abs, x
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			read((temp&0xff00)+((temp+x)&0xff));
			temp += x;
			CYCLE;
			t = read(temp);
			CYCLE;
			write(temp, t);
			F.b.C = t>>7;
			t <<= 1;
			setnz(t);
			CYCLE;
			write(temp, t);
			CYCLE;
			break;
//--
		case 0x2A: // rol a
			F.b.C = a>>7;
			a <<= 1;
			a |= oldC;
			setnz(a);
			CYCLE;
			break;
		case 0x26: // rol zp
			temp = read(pc++);
			CYCLE;
			t = read(temp);
			CYCLE;
			write(temp, t);
			F.b.C = t>>7;
			t <<= 1;
			t |= oldC;
			setnz(t);
			CYCLE;
			write(temp, t);
			CYCLE;
			break;
		case 0x36: // rol zp, x
			temp = read(pc++);
			CYCLE;
			read(temp);
			temp = (temp+x)&0xff;
			CYCLE;
			t = read(temp);
			CYCLE;
			write(temp, t);
			F.b.C = t>>7;
			t <<= 1;
			t |= oldC;
			setnz(t);
			CYCLE;
			write(temp, t);
			CYCLE;
			break;
		case 0x2E: // rol abs
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read(temp);
			CYCLE;
			write(temp, t);
			F.b.C = t>>7;
			t <<= 1;
			t |= oldC;
			setnz(t);
			CYCLE;
			write(temp, t);
			CYCLE;
			break;
		case 0x3E: // rol abs, x
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			read((temp&0xff00)+((temp+x)&0xff));
			temp += x;
			CYCLE;
			t = read(temp);
			CYCLE;
			write(temp, t);
			F.b.C = t>>7;
			t <<= 1;
			t |= oldC;
			setnz(t);
			CYCLE;
			write(temp, t);
			CYCLE;
			break;
//--
		case 0x6A: // ror a
			F.b.C = a&1;
			a >>= 1;
			a |= oldC<<7;
			setnz(a);
			CYCLE;
			break;
		case 0x66: // ror zp
			temp = read(pc++);
			CYCLE;
			t = read(temp);
			CYCLE;
			write(temp, t);
			F.b.C = t&1;
			t >>= 1;
			t |= oldC<<7;
			setnz(t);
			CYCLE;
			write(temp, t);
			CYCLE;
			break;
		case 0x76: // ror zp, x
			temp = read(pc++);
			CYCLE;
			read(temp);
			temp = (temp+x)&0xff;
			CYCLE;
			t = read(temp);
			CYCLE;
			write(temp, t);
			F.b.C = t&1;
			t >>= 1;
			t |= oldC<<7;
			setnz(t);
			CYCLE;
			write(temp, t);
			CYCLE;
			break;
		case 0x6E: // ror abs
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read(temp);
			CYCLE;
			write(temp, t);
			F.b.C = t&1;
			t >>= 1;
			t |= oldC<<7;
			setnz(t);
			CYCLE;
			write(temp, t);
			CYCLE;
			break;
		case 0x7E: // ror abs, x
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			read((temp&0xff00)+((temp+x)&0xff));
			temp += x;
			CYCLE;
			t = read(temp);
			CYCLE;
			write(temp, t);
			F.b.C = t&1;
			t >>= 1;
			t |= oldC<<7;
			setnz(t);
			CYCLE;
			write(temp, t);
			CYCLE;
			break;
			
		case 0x86: // stx zp
			t = read(pc++);
			CYCLE;
			write(t, x);
			CYCLE;		
			break;
		case 0x96: // stx zp, y
			t = read(pc++);
			CYCLE;
			read(t);
			t += y;
			CYCLE;
			write(t, x);
			CYCLE;		
			break;
		case 0x8E: // stx abs
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			write(temp, x);
			CYCLE;		
			break;
		case 0x84: // sty zp
			t = read(pc++);
			CYCLE;
			write(t, y);
			CYCLE;		
			break;
		case 0x94: // sty zp, x
			t = read(pc++);
			CYCLE;
			read(t);
			t += x;
			CYCLE;
			write(t, y);
			CYCLE;
			break;
		case 0x8C: // sty abs
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			write(temp, y);
			CYCLE;		
			break;
			
		case 0x85: // sta zp
			t = read(pc++);
			CYCLE;
			write(t, a);
			CYCLE;
			break;
		case 0x95: // sta zp, x
			t = read(pc++);
			CYCLE;
			read(t);
			t += x;
			CYCLE;
			write(t, a);
			CYCLE;
			break;
		case 0x8D: // sta abs
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			write(temp, a);
			CYCLE;
			break;
		case 0x9D: // sta abs, x
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			read((temp&0xff00)+((temp+x)&0xff));
			CYCLE;
			write(temp+x, a);
			CYCLE;		
			break;
		case 0x99: // sta abs, y
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			read((temp&0xff00)+((temp+y)&0xff));
			CYCLE;
			write(temp+y, a);
			CYCLE;		
			break;
		case 0x81: // sta (ind,x)
			t = read(pc++);
			CYCLE; // 2
			read(t);
			t += x;
			CYCLE; // 3
			temp = read(t++);
			CYCLE; // 4
			temp |= read(t)<<8;
			CYCLE; // 5
			write(temp, a);
			CYCLE; // 6		
			break;
		case 0x91: // sta (ind),y
			t = read(pc++);
			CYCLE; // 2
			temp = read(t++);
			CYCLE; // 3
			temp |= read(t)<<8;
			CYCLE; // 4
			read((temp&0xff00)+((temp+y)&0xff));
			CYCLE; // 5
			write(temp+y, a);
			CYCLE; // 6
			break;
			
		case 0xA2: // ldx imm
			x = read(pc++);
			setnz(x);
			CYCLE;
			break;
		case 0xA6: // ldx zp
			t = read(pc++);
			CYCLE;
			x = read(t);
			setnz(x);
			break;
		case 0xB6: // ldx zp, y
			t = read(pc++);
			CYCLE;
			read(t);
			t += y;
			CYCLE;
			x = read(t);
			setnz(x);
			CYCLE;		
			break;
		case 0xAE: // ldx abs
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			x = read(temp);
			setnz(x);
			CYCLE;
			break;
		case 0xBE: // ldx abs, y
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read((temp&0xff00)+((temp+y)&0xff));
			if( (temp&0xff00) != ((temp+y)&0xff00) )
			{
				CYCLE;
				t = read(temp+y);
			}
			x = t;
			setnz(x);
			CYCLE;
			break;
//--
		case 0xA0: // ldy imm
			y = read(pc++);
			setnz(y);
			CYCLE;
			break;
		case 0xA4: // ldy zp
			t = read(pc++);
			CYCLE;
			y = read(t);
			setnz(y);
			break;
		case 0xB4: // ldy zp, x
			t = read(pc++);
			CYCLE;
			read(t);
			t += x;
			CYCLE;
			y = read(t);
			setnz(y);
			CYCLE;		
			break;
		case 0xAC: // ldy abs
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			y = read(temp);
			setnz(y);
			CYCLE;
			break;
		case 0xBC: // ldy abs, x
			temp = read(pc++);
			CYCLE;
			temp |= read(pc++)<<8;
			CYCLE;
			t = read((temp&0xff00)+((temp+x)&0xff));
			if( (temp&0xff00) != ((temp+x)&0xff00) )
			{
				CYCLE;
				t = read(temp+x);
			}
			y = t;
			setnz(y);
			CYCLE;
			break;
		default:
			if( cpu_type <= CPU_6502 )
			{
				//todo: original 6502 unofficial opcodes
				printf("6502coru:$%X: Unimpl opc (possibly unofficial) $%X\n", pc, opc);
				exit(1);
				break;
			}
			printf("65C02coru:$%X: Unimpl opc $%X\n", pc, opc);

			// this point it's time for the 'C02
			// C++ coroutines can't co_yield from a function, so time to include more code
			#include "65C02coru.impl"
			break;		
		}
	} // end of waiting if
		
		u16 vectaddr = 0;
		
		if( nmi_line )
		{
			nmi_line = false;
			vectaddr = 0xfffa;			
		} else if( irq_line && !I_FLAG ) {
			vectaddr = 0xfffe;
		} else {
			continue;
		}
		
		// 7 cycles to handle interrupts
		read(pc);
		CYCLE;
		read(pc);
		CYCLE;
		write(0x100+s, pc>>8);
		s -= 1;
		CYCLE;
		write(0x100+s, pc);
		s -= 1;
		CYCLE;
		write(0x100+s, F.v&~0x10);
		s -= 1;
		CYCLE;
		pc = read(vectaddr);
		CYCLE;
		pc |= read(vectaddr+1)<<8;
		F.b.I = 1;
		CYCLE;
	} // end of 6502 coroutine while loop
}


