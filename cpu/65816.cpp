#include <print>
#include <cstdlib>
#include "65816.h"

/* https://discord.com/channels/465585922579103744/1186712601242124370/1238251692408045618
it works if I apply reads at the start of the 65816 CPU cycle 
and writes at the end (after advancing everything else by 6/8/12 mclk cycles):
*/

#define mcyc(a) co_yield (a)
#define read(cycles, addr) bus_read(addr); mcyc(cycles)
#define write(cycles, addr, v) mcyc(cycles); bus_write(addr, v)

Yieldable c65816::run()
{	
while(1)
{
	reg oper{}, addr{};
	u8 bank=0;
	u16 t=0;
	u32 t32=0;
	//std::println("About to run ${:X}", opc);
	switch( opc )
	{
	case 0x00: // brk 
		/*//(temporary impl for SST testing other instructions)
		std::println("${:X}:${:X}: BRK! do_irqs={}", pb>>16, pc, do_irqs);
		mcyc(1);
		*/
		//std::println("brk (or nmi) at ${:X}:${:X}", pb>>16, pc);
		if( !do_irqs )
		{
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		/*2.1*/ pc++;
		}
		/*2.2*/ write(6, S.v, pb>>16); S.v--;
		/*3.2*/ write(6, S.v, pc>>8); S.v--;
		/*4.2*/ write(6, S.v, pc); S.v--;
		/*5.1*/ if( do_irqs && irq_line )
			{
				addr.v = 0xFFEE;
			} else if( do_irqs && nmi_line ) {
				addr.v = 0xFFEA;	
				nmi_line = false;
			} else {
				addr.v = 0xFFE6; // actual brk
			}
		/*5.2*/ write(6, S.v, F.v); S.v--;
		/*6.2*/ oper.v = read(6, addr.v);
		/*7.1*/ F.b.I = 1; F.b.D = 0;
		/*7.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
			pb = 0; pc = oper.v;
			do_irqs = false;
		/*8.2*/ mcyc(6); fetch();
		break;
	case 0x01: // ora (dp, x)
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++; addr.v += X.v + D.v;
		/*2.2*/ mcyc(6);
		if( D.b.l )
		{
		/*3.2*/ mcyc(6);
		}
		/*4.2*/ oper.b.l = read(6, addr.v);
		/*5.2*/ oper.b.h = read(6, (addr.v+1)&0xffff); addr = oper;
		/*6.2*/ oper.v = read(6, db|addr.v); poll_irqs();
		if(!F.b.M)
		{
		/*7.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*8.1*/ A.v |= oper.v; if( F.b.M ) { setnz(A.b.l); } else { setnz(A); }
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x02: // cop
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		/*2.2*/ write(6, S.v, pb>>16); S.v--;
		/*3.1*/ pc++;
		/*3.2*/ write(6, S.v, pc>>8); S.v--;
		/*4.2*/ write(6, S.v, pc); S.v--;
		/*5.2*/ write(6, S.v, F.v); S.v--;
		/*6.2*/ addr.b.l = read(6, 0xFFE4);
		/*7.1*/ F.b.I = 1; F.b.D = 0;
		/*7.2*/ addr.b.h = read(6, 0xFFE5); poll_irqs();
		/*8.1*/ pc = addr.v; pb = 0;
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x03: // ora stack rel
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++; addr.v += S.v;
		/*3.2*/ oper.v = read(12, addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*4.2*/ addr.v++; oper.b.h = read(6, addr.v);
		}
		/*5.1*/ A.v |= oper.v; if( F.b.M ) { setnz(A.b.l); } else { setnz(A); }
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0x04: // tsb dp
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.1*/ pc++;
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ if(!D.b.l) { pc++; } addr.v = oper.v + D.v;
		/*3.2*/ oper.v = read(6, addr.v);
		if( !F.b.M )
		{
		/*4.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		}
		/*5.1*/ if(F.b.M) 
			{ 
				F.b.Z = (A.b.l&oper.b.l)==0; 
			} else { 
				F.b.Z = (A.v&oper.v)==0;
			}
			oper.v |= A.v;
		/*5.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*6.2*/ write(6, (addr.v+1)&0xffff, oper.b.h);
		}
		/*7.2*/ write(6, addr.v, oper.b.l); poll_irqs();
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x05: // ora direct
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.1*/ pc++;
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ if( !D.b.l ) { pc++; } addr.v = oper.v + D.v;
		/*3.2*/ oper.v = read(6, addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*4.2*/ addr.v++; oper.b.h = read(6, addr.v);
		}
		/*5.1*/ A.v|=oper.v; if( F.b.M ) { setnz(A.b.l); } else { setnz(A); }
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0x06: // asl dp
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/	mcyc(6);
		}
		/*3.1*/ pc++; addr.v = oper.v + D.v;
		/*3.2*/ oper.v = read(6, addr.v);
		if( !F.b.M )
		{
		/*4.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		}
		/*5.1*/ F.b.C = (F.b.M ? (oper.b.l>>7) : (oper.b.h>>7));
			if( F.b.M ) { oper.b.l <<= 1; setnz(oper.b.l); } else { oper.v <<= 1; setnz(oper); }
		/*5.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*6.2*/ write(6, (addr.v+1)&0xffff, oper.b.h);
		}
		/*7.2*/ write(6, addr.v, oper.b.l); poll_irqs();
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x07: // ora direct ind long
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.2*/ bank = read(6, (oper.v+2)&0xffff);
		/*6.2*/ oper.v = read(6, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*8.1*/ A.v |= oper.v; if( F.b.M ) { setnz(A.b.l); } else { setnz(A); }
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x08: // php
		/*1.1*/ pc++;
		/*1.2*/
		/*2.1*/
		/*2.2*/ write(12, S.v, F.v); poll_irqs();
		/*3.1*/ S.v--;
		/*3.2*/ mcyc(6); fetch();
			break;
	case 0x09: // ora imm
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc); poll_irqs();
		if( !F.b.M )
		{
		/*2.1*/ pc++;
		/*2.2*/ oper.b.h = read(6, pb|pc);
		}
		/*3.1*/ pc++;
			A.v |= oper.v;
			if( F.b.M )
			{
				setnz(A.b.l);
			} else {
				setnz(A);
			}
		/*3.2*/	mcyc(6); fetch();
			break;
	case 0x0A: // asl a
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ if( F.b.M )
			{
				F.b.C = A.b.l>>7;
				A.b.l <<= 1;
				setnz(A.b.l);
			} else {
				F.b.C = A.v>>15;
				A.v <<= 1;
				setnz(A);
			}
		/*2.2*/ mcyc(6); fetch();
			break;	
	case 0x0B: // phd
		/*1.1*/ pc++;
		/*2.2*/ write(12, S.v, D.b.h); S.v--;
		/*3.2*/ write(6, S.v, D.b.l); S.v--; poll_irqs();
		/*4.2*/ mcyc(6); fetch();	
			break;
	case 0x0C: // tsb abs
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ oper.v = read(6, (db|addr.v));
		if( !F.b.M )
		{
		/*4.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*5.1*/ if(F.b.M) { F.b.Z=(A.b.l&oper.b.l)==0; } else { F.b.Z=(A.v&oper.v)==0; } oper.v |= A.v;
		/*5.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*6.2*/ write(6, (db|addr.v)+1, oper.b.h);
		}
		/*7.2*/ write(6, (db|addr.v), oper.b.l);
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x0D: // ora abs
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ oper.v = read(6, db|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*4.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*5.1*/ A.v|=oper.v; if(F.b.M) { setnz(A.b.l); } else { setnz(A); }
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0x0E: // asl abs
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ oper.v = read(6, db|addr.v);
		if( !F.b.M )
		{
		/*4.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*5.1*/ if( F.b.M ) 
			{ 
				F.b.C=oper.b.l>>7; 
				oper.b.l<<=1; 
				setnz(oper.b.l); 
			} else { 
				F.b.C=oper.b.h>>7;
				oper.v <<= 1;
				setnz(oper);
			}
		/*5.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*6.2*/ write(6, (db|addr.v)+1, oper.b.h);
		}
		/*7.2*/ write(6, db|addr.v, oper.b.l); poll_irqs();
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x0F: // ora long
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ bank = read(6, pb|pc);
		/*4.1*/ pc++;
		/*4.2*/ oper.v = read(6, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ A.v |= oper.v;
			if( F.b.M ) { setnz(A.b.l); } else { setnz(A); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x10: // bpl rel
		/*1.1*/ pc++;
		/*1.2*/ oper.b.l = read(6, pb|pc); addr.v = pc + s8(oper.b.l) + 1; if( F.b.N==1 ) { poll_irqs(); }
		/*2.1*/ pc++; mcyc(3);
		if( F.b.N == 0 )
		{
		/*2.2*/ mcyc(3); poll_irqs();
		/*3.1*/ pc = addr.v; mcyc(3);
		}
		/*3.2*/ mcyc(3); fetch();
			break;
	case 0x11: //ora (dp), y
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.1*/ bank = (db>>16)+((addr.v + Y.v)>>16);
			oper.v = addr.v;
			addr.v += Y.v;
			mcyc(3);
		if( !(F.b.X && oper.b.h == addr.b.h) )
		{
		/*6.1*/ mcyc(6);
		}
		/*6.2*/ oper.v = read(3, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*8.1*/ A.v|=oper.v; if( F.b.M ) { setnz(A.b.l); } else { setnz(A); }
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x12: // ora direct ind
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.2*/ oper.v = read(6, db|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*6.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*7.1*/ A.v|=oper.v; if(F.b.M) { setnz(A.b.l); } else { setnz(A); }
		/*7.2*/ mcyc(6); fetch();
			break;
	case 0x13: // ora stack relative ind indexed
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		/*2.1*/ pc++; addr.v = oper.v + S.v;
		/*3.2*/ oper.b.l = read(12, addr.v);
		/*4.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		/*5.1*/ addr.v = oper.v;
			t32 = addr.v; t32 += Y.v;
			addr.v = t32;
			bank = (db>>16)+(t32>>16);
		/*6.2*/ oper.v = read(12, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*8.1*/ A.v|=oper.v; if(F.b.M){ setnz(A.b.l); }else { setnz(A); }
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x14: // trb dp
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.1*/ pc++;
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ if(!D.b.l) { pc++; } addr.v = oper.v + D.v;
		/*3.2*/ oper.v = read(6, addr.v);
		if( !F.b.M )
		{
		/*4.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		}
		/*5.1*/ if(F.b.M) 
			{ 
				F.b.Z = (A.b.l&oper.b.l)==0; 
			} else { 
				F.b.Z = (A.v&oper.v)==0;
			}
			oper.v &= ~A.v;
		/*5.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*6.2*/ write(6, (addr.v+1)&0xffff, oper.b.h);
		}
		/*7.2*/ write(6, addr.v, oper.b.l); poll_irqs();
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x15: // ora dp, x
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*3.1*/ pc++; addr.v += X.v + D.v; mcyc(9);
		if( D.b.l )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		}
		/*6.1*/ A.v|=oper.v; if(F.b.M) { setnz(A.b.l); } else { setnz(A); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x16: // asl dp, x
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*3.1*/ pc++; addr.v += X.v + D.v; mcyc(9);
		if( D.b.l )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, addr.v);
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		}
		/*6.1*/ if( F.b.M )
			{
				F.b.C = oper.b.l>>7;
				oper.b.l <<= 1;
				setnz(oper.b.l);
			} else {
				F.b.C = oper.b.h>>7;
				oper.v <<= 1;
				setnz(oper);
			}
		/*6.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*7.2*/ write(6, (addr.v+1)&0xffff, oper.b.h);
		}
		/*8.2*/ write(6, addr.v, oper.b.l); poll_irqs();
		/*9.2*/ mcyc(6); fetch();
			break;
	case 0x17: // ora direct ind indexed long (d),y
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.2*/ bank = read(6, (oper.v+2)&0xffff);
		/*6.1*/ t32 = addr.v; t32 += Y.v; bank += t32>>16; addr.v = t32;
		/*6.2*/ oper.v = read(6, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*8.1*/ A.v|=oper.v; if(F.b.M){ setnz(A.b.l); }else { setnz(A); }
		/*8.2*/ mcyc(6); fetch();
			break;			
	case 0x18: // clc
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ F.b.C = 0;
		/*2.2*/ mcyc(6); fetch();
			break;
	case 0x19: // ora abs, y
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; oper.v = addr.v;
			t32 = addr.v; t32 += Y.v;
			bank = t32>>16; bank += db>>16;
			addr.v = t32;
			mcyc(3);
		if( !(F.b.X && oper.b.h == addr.b.h) )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ A.v|=oper.v; if(F.b.M) { setnz(A.b.l); } else { setnz(A); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x1A: // inc a
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ if( F.b.M ) { A.b.l++; setnz(A.b.l); } else { A.v++; setnz(A); }
		/*2.2*/ mcyc(6); fetch();
			break;
	case 0x1B: // tcs
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ S = A;
		/*2.2*/ mcyc(6); fetch();
			break;
	case 0x1C: // trb abs
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ oper.v = read(6, (db|addr.v));
		if( !F.b.M )
		{
		/*4.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*5.1*/ if(F.b.M) { F.b.Z=(A.b.l&oper.b.l)==0; } else { F.b.Z=(A.v&oper.v)==0; } oper.v &=~A.v;
		/*5.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*6.2*/ write(6, (db|addr.v)+1, oper.b.h);
		}
		/*7.2*/ write(6, (db|addr.v), oper.b.l);
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x1D: // ora abs, x
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; oper.v = addr.v;
			t32 = addr.v; t32 += X.v;
			bank = t32>>16; bank += db>>16;
			addr.v = t32;
			mcyc(3);
		if( !(F.b.X && oper.b.h == addr.b.h) )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ A.v|=oper.v; if(F.b.M) { setnz(A.b.l); } else { setnz(A); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x1E: // asl abs, x
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; t32 = addr.v; t32 += X.v;
			addr.v = t32; bank = (db>>16)+(t32>>16);
		/*4.2*/ oper.v = read(12, (bank<<16)|addr.v);
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ if(F.b.M) 
			{
				F.b.C=oper.b.l>>7;
				oper.b.l<<=1;
				setnz(oper.b.l); 
			} else {
				F.b.C = oper.b.h>>7;
				oper.v <<= 1;
				setnz(oper);
			}
		/*6.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*7.2*/ write(6, ((bank<<16)|addr.v)+1, oper.b.h);
		}
		/*8.2*/ write(6, (bank<<16)|addr.v, oper.b.l); poll_irqs();
		/*9.2*/ mcyc(6); fetch();
			break;			
	case 0x1F: // ora long, x
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ bank = read(6, pb|pc);
		/*4.1*/ pc++; t32 = addr.v; t32 += X.v; bank += t32>>16; addr.v = t32;
		/*4.2*/ oper.v = read(6, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ A.v |= oper.v;
			if( F.b.M ) { setnz(A.b.l); } else { setnz(A); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x20: // jsr abs
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ read(6, pb|pc);
		/*4.1*/ pc--;
		/*4.2*/ write(6, S.v, pc>>8);
		/*5.2*/ write(6, (S.v-1)&0xffff, pc);
		/*6.1*/ S.v-=2; pc=addr.v;
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x21: // and (dp, x)
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++; addr.v += X.v + D.v;
		/*2.2*/ mcyc(6);
		if( D.b.l )
		{
		/*3.2*/ mcyc(6);
		}
		/*4.2*/ oper.b.l = read(6, addr.v);
		/*5.2*/ oper.b.h = read(6, (addr.v+1)&0xffff); addr = oper;
		/*6.2*/ oper.v = read(6, db|addr.v); poll_irqs();
		if(!F.b.M)
		{
		/*7.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*8.1*/ if( F.b.M ) { A.b.l&=oper.b.l; setnz(A.b.l); } else { A.v&=oper.v; setnz(A); }
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x22: // jsl (jsr long)
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ write(6, S.v, pb>>16);
		/*4.2*/ read(6, S.v);
		/*5.2*/ bank = read(6, pb|pc);
		/*6.2*/ write(6, (S.v-1)&0xffff, pc>>8);
		/*7.2*/ write(6, (S.v-2)&0xffff, pc); poll_irqs();
		/*8.1*/ pc++; S.v-=3; pc=addr.v; pb=bank<<16;
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x23: // and stack rel
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++; addr.v += S.v;
		/*3.2*/ oper.v = read(12, addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*4.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		}
		/*5.1*/ if( F.b.M )
			{
				A.b.l &= oper.b.l;
				setnz(A.b.l);
			} else {
				A.v &= oper.v;
				setnz(A);
			}
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0x24: // bit direct
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; addr.v += D.v;
		/*3.2*/ oper.v = read(6, addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*4.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		}
		/*5.1*/ if( F.b.M ) 
			{
				F.b.Z=((A.b.l&oper.b.l)==0); 
				F.b.N=(oper.b.l>>7)&1;
				F.b.V=(oper.b.l>>6)&1;
			} else {
				F.b.Z=((A.v&oper.v)==0);
				F.b.N=(oper.v>>15)&1;
				F.b.V=(oper.v>>14)&1;
			}
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0x25: // and direct
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.1*/ pc++;
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ if( !D.b.l ) { pc++; } addr.v = oper.v + D.v;
		/*3.2*/ oper.v = read(6, addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*4.2*/ addr.v++; oper.b.h = read(6, addr.v);
		}
		/*5.1*/ if( F.b.M ) { A.b.l&=oper.b.l; setnz(A.b.l); } else { A.v&=oper.v; setnz(A); }
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0x26: // rol dp
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/	mcyc(6);
		}
		/*3.1*/ pc++; addr.v = oper.v + D.v;
		/*3.2*/ oper.v = read(6, addr.v);
		if( !F.b.M )
		{
		/*4.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		}
		/*5.1*/ t = F.b.C;
			F.b.C = (F.b.M ? (oper.b.l>>7) : (oper.b.h>>7));
			if( F.b.M ) 
			{ 
				oper.b.l <<= 1; 
				oper.b.l |= t;
				setnz(oper.b.l); 
			} else { 
				oper.v <<= 1;
				oper.v |= t;
				setnz(oper);
			}
		/*5.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*6.2*/ write(6, (addr.v+1)&0xffff, oper.b.h);
		}
		/*7.2*/ write(6, addr.v, oper.b.l); poll_irqs();
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x27: // and direct ind long
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.2*/ bank = read(6, (oper.v+2)&0xffff);
		/*6.2*/ oper.v = read(6, (bank<<16)|addr.v); oper.v|=0xff00; poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*8.1*/ A.v &= oper.v; if( F.b.M ) { setnz(A.b.l); } else { setnz(A); }
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x28: // plp
		/*1.1*/ pc++;
		/*3.1*/ S.v++;
		/*3.2*/ oper.b.l = read(18, S.v); poll_irqs();
		/*4.1*/ F.v = oper.b.l; if( F.b.X ) { X.b.h = Y.b.h = 0; }
		/*4.2*/ mcyc(6); fetch();
			break;
	case 0x29: // and imm
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc); poll_irqs();
		if( !F.b.M )
		{
		/*2.1*/ pc++;
		/*2.2*/ oper.b.h = read(6, pb|pc);
		}
		/*3.1*/ pc++;
			if( F.b.M )
			{
				A.b.l &= oper.b.l;
				setnz(A.b.l);
			} else {
				A.v &= oper.v;
				setnz(A);
			}
		/*3.2*/	mcyc(6); fetch();
			break;
	case 0x2A: // rol a
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ t = F.b.C;
			if( F.b.M )
			{
				F.b.C = A.b.l>>7;
				A.b.l <<= 1; A.b.l |= t;
				setnz(A.b.l);
			} else {
				F.b.C = A.v>>15;
				A.v <<=1; A.v|=t;
				setnz(A);
			}
		/*2.2*/ mcyc(6); fetch();
			break;
	case 0x2B: // pld
		/*1.1*/ pc++;
		/*3.2*/ S.v++; oper.b.l = read(18, S.v);
		/*4.2*/ S.v++; oper.b.h = read(6, S.v); poll_irqs();
		/*5.1*/ D = oper; setnz(D);
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0x2C: // bit abs
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ oper.v = read(6, db|addr.v);
		if( !F.b.M )
		{
		/*4.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*5.1*/ if( F.b.M )
			{
				F.b.Z = (A.b.l&oper.b.l)==0;
				F.b.N = oper.b.l>>7;
				F.b.V = (oper.b.l>>6)&1;
			} else {
				F.b.Z = (A.v&oper.v)==0;
				F.b.N = oper.b.h>>7;
				F.b.V = (oper.b.h>>6)&1;
			}
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0x2D: // and abs
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ oper.v = read(6, db|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*4.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*5.1*/ if( F.b.M )
			{
				A.b.l &= oper.b.l;
				setnz(A.b.l);
			} else {
				A.v &= oper.v;
				setnz(A);
			}
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0x2E: // rol abs
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ oper.v = read(6, db|addr.v);
		if( !F.b.M )
		{
		/*4.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*5.1*/ t = F.b.C;
			if( F.b.M ) 
			{ 
				F.b.C=oper.b.l>>7; 
				oper.b.l <<= 1;
				oper.b.l |= t;
				setnz(oper.b.l); 
			} else { 
				F.b.C=oper.b.h>>7;
				oper.v <<= 1;
				oper.v |= t;
				setnz(oper);
			}
		/*5.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*6.2*/ write(6, (db|addr.v)+1, oper.b.h);
		}
		/*7.2*/ write(6, db|addr.v, oper.b.l); poll_irqs();
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x2F: // and long
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ bank = read(6, pb|pc);
		/*4.1*/ pc++;
		/*4.2*/ oper.v = read(6, (bank<<16)|addr.v); oper.v|=0xff00; poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ A.v &= oper.v;
			if( F.b.M ) { setnz(A.b.l); } else { setnz(A); }
		/*6.2*/ mcyc(6); fetch();
			break;		
	case 0x30: // bmi rel
		/*1.1*/ pc++;
		/*1.2*/ oper.b.l = read(6, pb|pc); addr.v = pc + s8(oper.b.l) + 1; if( F.b.N==0 ) { poll_irqs(); }
		/*2.1*/ pc++; mcyc(3);
		if( F.b.N == 1 )
		{
		/*2.2*/ mcyc(3); poll_irqs();
		/*3.1*/ pc = addr.v; mcyc(3);
		}
		/*3.2*/ mcyc(3); fetch();
			break;
	case 0x31: //and (dp), y
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.1*/ bank = (db>>16)+((addr.v + Y.v)>>16);
			oper.v = addr.v;
			addr.v += Y.v;
			mcyc(3);
		if( !(F.b.X && oper.b.h == addr.b.h) )
		{
		/*6.1*/ mcyc(6);
		}
		/*6.2*/ oper.v = read(3, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*8.1*/ if( F.b.M ) { A.b.l&=oper.b.l; setnz(A.b.l); } else { A.v&=oper.v; setnz(A); }
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x32: // and direct ind
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.2*/ oper.v = read(6, db|addr.v); oper.v|=0xff00; poll_irqs();
		if( !F.b.M )
		{
		/*6.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*7.1*/ A.v&=oper.v; if(F.b.M) { setnz(A.b.l); } else { setnz(A); }
		/*7.2*/ mcyc(6); fetch();
			break;
	case 0x33: // and stack relative ind indexed
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		/*2.1*/ pc++; addr.v = oper.v + S.v;
		/*3.2*/ oper.b.l = read(12, addr.v);
		/*4.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		/*5.1*/ addr.v = oper.v;
			t32 = addr.v; t32 += Y.v;
			addr.v = t32;
			bank = (db>>16)+(t32>>16);
		/*6.2*/ oper.v = read(12, (bank<<16)|addr.v); oper.v|=0xff00; poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*8.1*/ A.v&=oper.v; if(F.b.M){ setnz(A.b.l); }else { setnz(A); }
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x34: // bit db, x
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*3.1*/ pc++; addr.v += X.v + D.v; mcyc(9);
		if( D.b.l )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, addr.v); oper.v|=0xff00; poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		}
		/*6.1*/ if(F.b.M) 
			{ 
				F.b.Z = ((A.b.l&oper.b.l)==0);
				F.b.N = (oper.b.l>>7)&1;
				F.b.V = (oper.b.l>>6)&1;
			} else { 
				F.b.Z = ((A.v&oper.v)==0);
				F.b.N = (oper.v>>15)&1;
				F.b.V = (oper.v>>14)&1;
			}
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x35: // and db, x
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*3.1*/ pc++; addr.v += X.v + D.v; mcyc(9);
		if( D.b.l )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, addr.v); oper.v|=0xff00; poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		}
		/*6.1*/ A.v&=oper.v; if(F.b.M) { setnz(A.b.l); } else { setnz(A); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x36: // rol dp, x
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*3.1*/ pc++; addr.v += X.v + D.v; mcyc(9);
		if( D.b.l )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, addr.v);
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		}
		/*6.1*/ t = F.b.C;
			if( F.b.M )
			{
				F.b.C = oper.b.l>>7;
				oper.b.l <<= 1;
				oper.b.l |= t;
				setnz(oper.b.l);
			} else {
				F.b.C = oper.b.h>>7;
				oper.v <<= 1;
				oper.v |= t;
				setnz(oper);
			}
		/*6.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*7.2*/ write(6, (addr.v+1)&0xffff, oper.b.h);
		}
		/*8.2*/ write(6, addr.v, oper.b.l); poll_irqs();
		/*9.2*/ mcyc(6); fetch();
			break;
	case 0x37: // and direct ind indexed long (d),y
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.2*/ bank = read(6, (oper.v+2)&0xffff);
		/*6.1*/ t32 = addr.v; t32 += Y.v; bank += t32>>16; addr.v = t32;
		/*6.2*/ oper.v = read(6, (bank<<16)|addr.v); oper.v|=0xff00; poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*8.1*/ A.v&=oper.v; if(F.b.M){ setnz(A.b.l); }else { setnz(A); }
		/*8.2*/ mcyc(6); fetch();
			break;			
	case 0x38: // sec
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ F.b.C = 1;
		/*2.2*/ mcyc(6); fetch();
			break;
	case 0x39: // and abs, y
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; oper.v = addr.v;
			t32 = addr.v; t32 += Y.v;
			bank = t32>>16; bank += db>>16;
			addr.v = t32;
			mcyc(3);
		if( !(F.b.X && oper.b.h == addr.b.h) )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, (bank<<16)|addr.v); oper.v|=0xff00; poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ A.v&=oper.v; if(F.b.M) { setnz(A.b.l); } else { setnz(A); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x3A: // dec a
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ if( F.b.M ) { A.b.l--; setnz(A.b.l); } else { A.v--; setnz(A); }
		/*2.2*/ mcyc(6); fetch();
			break;
	case 0x3B: // tsc
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc);
		/*2.1*/ A = S; setnz(A); mcyc(3); poll_irqs();
		/*2.2*/ mcyc(3); fetch();
			break;
	case 0x3C: // bit abs, x
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; oper.v = addr.v;
			t32 = addr.v; t32 += X.v;
			bank = t32>>16; bank += db>>16;
			addr.v = t32;
			mcyc(3);
		if( !(F.b.X && oper.b.h == addr.b.h) )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ if( F.b.M )
			{
				F.b.Z = ((A.b.l&oper.b.l)==0);
				F.b.N = (oper.b.l>>7)&1;
				F.b.V = (oper.b.l>>6)&1;
			} else {
				F.b.Z = ((A.v&oper.v)==0);
				F.b.N = (oper.v>>15)&1;
				F.b.V = (oper.v>>14)&1;
			}
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x3D: // and abs, x
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; oper.v = addr.v;
			t32 = addr.v; t32 += X.v;
			bank = t32>>16; bank += db>>16;
			addr.v = t32;
			mcyc(3);
		if( !(F.b.X && oper.b.h == addr.b.h) )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, (bank<<16)|addr.v); oper.v|=0xff00; poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ A.v&=oper.v; if(F.b.M) { setnz(A.b.l); } else { setnz(A); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x3E: // rol abs, x
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; t32 = addr.v; t32 += X.v;
			addr.v = t32; bank = (db>>16)+(t32>>16);
		/*4.2*/ oper.v = read(12, (bank<<16)|addr.v);
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ t = F.b.C;
			if(F.b.M) 
			{
				F.b.C = oper.b.l>>7;
				oper.b.l <<=1;
				oper.b.l |= t;
				setnz(oper.b.l); 
			} else {
				F.b.C = oper.v>>15;
				oper.v <<= 1;
				oper.v |= t;
				setnz(oper);
			}
		/*6.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*7.2*/ write(6, ((bank<<16)|addr.v)+1, oper.b.h);
		}
		/*8.2*/ write(6, (bank<<16)|addr.v, oper.b.l); poll_irqs();
		/*9.2*/ mcyc(6); fetch();
			break;
	case 0x3F: // and long, x
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ bank = read(6, pb|pc);
		/*4.1*/ pc++; t32 = addr.v; t32 += X.v; bank += t32>>16; addr.v = t32;
		/*4.2*/ oper.v = read(6, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ if( F.b.M ) { A.b.l&=oper.b.l; setnz(A.b.l); } else { A.v&=oper.v; setnz(A); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x40: // rti
		/*1.1*/ pc++;
		/*3.2*/ S.v++; oper.b.l = read(18, S.v);
		/*4.1*/ F.v = oper.v;
		/*4.2*/ S.v++; oper.b.l = read(6, S.v);
		/*5.2*/ S.v++; oper.b.h = read(6, S.v);
		/*6.2*/ S.v++; pb = read(6, S.v); pb<<=16; poll_irqs();
		/*7.1*/ pc = oper.v; if(F.b.X) { X.v = X.b.l; Y.v = Y.b.l; }
		/*7.2*/ mcyc(6); fetch();
			break;
	case 0x41: // eor (dp, x)
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++; addr.v += X.v + D.v;
		/*2.2*/ mcyc(6);
		if( D.b.l )
		{
		/*3.2*/ mcyc(6);
		}
		/*4.2*/ oper.b.l = read(6, addr.v);
		/*5.2*/ oper.b.h = read(6, (addr.v+1)&0xffff); addr = oper;
		/*6.2*/ oper.v = read(6, db|addr.v); poll_irqs();
		if(!F.b.M)
		{
		/*7.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*8.1*/ A.v ^= oper.v; if( F.b.M ) { setnz(A.b.l); } else { setnz(A); }
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x42: // wdm (2 byte nop)
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ pc++;
		/*2.2*/ mcyc(6); fetch();
			break;
	case 0x43: // eor stack rel
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++; addr.v += S.v;
		/*3.2*/ oper.v = read(12, addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*4.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		}
		/*5.1*/ if( F.b.M )
			{
				A.b.l ^= oper.b.l;
				setnz(A.b.l);
			} else {
				A.v ^= oper.v;
				setnz(A);
			}
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0x44: // mvp (block move positive)
		/*1.1*/ pc++;
		/*1.2*/ db = read(6, pb|pc); db<<=16;
		/*2.1*/ pc++;
		/*2.2*/ bank = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ oper.b.l = read(6, (bank<<16)|X.v);
		/*4.2*/ write(6, (db|Y.v), oper.b.l);
		/*5.1*/ A.v--; if( F.b.X ) { X.b.l--; Y.b.l--; } else { X.v--; Y.v--; }
			if( A.v != 0xffff ) { pc -= 3; }
		/*6.2*/ mcyc(12); poll_irqs();
		/*7.2*/ mcyc(6); fetch();
			break;
	case 0x45: // eor direct
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.1*/ pc++;
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ if( !D.b.l ) { pc++; } addr.v = oper.v + D.v;
		/*3.2*/ oper.v = read(6, addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*4.2*/ addr.v++; oper.b.h = read(6, addr.v);
		}
		/*5.1*/ A.v^=oper.v; if( F.b.M ) { setnz(A.b.l); } else { setnz(A); }
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0x46: // lsr dp
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/	mcyc(6);
		}
		/*3.1*/ pc++; addr.v = oper.v + D.v;
		/*3.2*/ oper.v = read(6, addr.v);
		if( !F.b.M )
		{
		/*4.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		}
		/*5.1*/ F.b.C = oper.v&1;
			if( F.b.M ) { oper.b.l >>= 1; setnz(oper.b.l); } else { oper.v >>= 1; setnz(oper); }
		/*5.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*6.2*/ write(6, (addr.v+1)&0xffff, oper.b.h);
		}
		/*7.2*/ write(6, addr.v, oper.b.l); poll_irqs();
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x47: // eor direct ind long
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.2*/ bank = read(6, (oper.v+2)&0xffff);
		/*6.2*/ oper.v = read(6, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*8.1*/ A.v ^= oper.v; if( F.b.M ) { setnz(A.b.l); } else { setnz(A); }
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x48: // pha
		/*1.1*/ pc++;
		/*1.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*2.1*/ 
		/*2.2*/ write(6, S.v, A.b.h); S.v--;
		}
		/*3.1*/ //S.v--;
		/*3.2*/ write(6, S.v, A.b.l);poll_irqs();
		/*4.1*/ S.v--;
		/*4.2*/ mcyc(6); fetch();
			break;
	case 0x49: // eor imm
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc); poll_irqs();
		if( !F.b.M )
		{
		/*2.1*/ pc++;
		/*2.2*/ oper.b.h = read(6, pb|pc);
		}
		/*3.1*/ pc++;
			A.v ^= oper.v;
			if( F.b.M )
			{
				setnz(A.b.l);
			} else {
				setnz(A);
			}
		/*3.2*/	mcyc(6); fetch();
			break;
	case 0x4A: // lsr a
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ F.b.C = A.v&1;
			if( F.b.M )
			{
				A.b.l >>= 1;
				setnz(A.b.l);
			} else {
				A.v >>= 1;
				setnz(A);
			}
		/*2.2*/ mcyc(6); fetch();	
			break;
	case 0x4B: // phk
		/*1.1*/ pc++;
		/*2.2*/ write(12, S.v, pb>>16); poll_irqs();
		/*3.2*/ S.v--; mcyc(6); fetch();
			break;
	case 0x4C: // jmp abs
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc); poll_irqs();
		/*3.1*/ pc = addr.v;
		/*3.2*/ mcyc(6); fetch();
			break;
	case 0x4D: // eor abs
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ oper.v = read(6, db|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*4.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*5.1*/ A.v^=oper.v; if(F.b.M) { setnz(A.b.l); } else { setnz(A); }
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0x4E: // lsr abs
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ oper.v = read(6, db|addr.v);
		if( !F.b.M )
		{
		/*4.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*5.1*/ F.b.C = oper.v&1;
			if( F.b.M ) 
			{ 
				oper.b.l >>= 1; 
				setnz(oper.b.l); 
			} else { 
				oper.v >>= 1;
				setnz(oper);
			}
		/*5.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*6.2*/ write(6, (db|addr.v)+1, oper.b.h);
		}
		/*7.2*/ write(6, db|addr.v, oper.b.l); poll_irqs();
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x4F: // eor long
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ bank = read(6, pb|pc);
		/*4.1*/ pc++;
		/*4.2*/ oper.v = read(6, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ A.v ^= oper.v;
			if( F.b.M ) { setnz(A.b.l); } else { setnz(A); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x50: // bvc rel
		/*1.1*/ pc++;
		/*1.2*/ oper.b.l = read(6, pb|pc); addr.v = pc + s8(oper.b.l) + 1; if( F.b.V==1 ) { poll_irqs(); }
		/*2.1*/ pc++; mcyc(3);
		if( F.b.V == 0 )
		{
		/*2.2*/ mcyc(3); poll_irqs();
		/*3.1*/ pc = addr.v; mcyc(3);
		}
		/*3.2*/ mcyc(3); fetch();
			break;
	case 0x51: //eor (dp), y
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.1*/ bank = (db>>16)+((addr.v + Y.v)>>16);
			oper.v = addr.v;
			addr.v += Y.v;
			mcyc(3);
		if( !(F.b.X && oper.b.h == addr.b.h) )
		{
		/*6.1*/ mcyc(6);
		}
		/*6.2*/ oper.v = read(3, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*8.1*/ A.v^=oper.v; if( F.b.M ) { setnz(A.b.l); } else { setnz(A); }
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x52: // eor direct ind
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.2*/ oper.v = read(6, db|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*6.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*7.1*/ A.v^=oper.v; if(F.b.M) { setnz(A.b.l); } else { setnz(A); }
		/*7.2*/ mcyc(6); fetch();
			break;
	case 0x53: // eor stack relative ind indexed
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		/*2.1*/ pc++; addr.v = oper.v + S.v;
		/*3.2*/ oper.b.l = read(12, addr.v);
		/*4.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		/*5.1*/ addr.v = oper.v;
			t32 = addr.v; t32 += Y.v;
			addr.v = t32;
			bank = (db>>16)+(t32>>16);
		/*6.2*/ oper.v = read(12, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*8.1*/ A.v^=oper.v; if(F.b.M){ setnz(A.b.l); }else { setnz(A); }
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x54: // mvn (block move next / negative)
		/*1.1*/ pc++;
		/*1.2*/ db = read(6, pb|pc); db<<=16;
		/*2.1*/ pc++;
		/*2.2*/ bank = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ oper.b.l = read(6, (bank<<16)|X.v);
		/*4.2*/ write(6, (db|Y.v), oper.b.l);
		/*5.1*/ A.v--; if( F.b.X ) { X.b.l++; Y.b.l++; } else { X.v++; Y.v++; }
			if( A.v != 0xffff ) { pc -= 3; }
		/*6.2*/ mcyc(12); poll_irqs();
		/*7.2*/ mcyc(6); fetch();
			break;
	case 0x55: // eor db, x
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*3.1*/ pc++; addr.v += X.v + D.v; mcyc(9);
		if( D.b.l )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		}
		/*6.1*/ A.v^=oper.v; if(F.b.M) { setnz(A.b.l); } else { setnz(A); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x56: // lsr dp, x
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*3.1*/ pc++; addr.v += X.v + D.v; mcyc(9);
		if( D.b.l )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, addr.v);
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		}
		/*6.1*/ F.b.C = oper.b.l&1;
			if( F.b.M )
			{
				oper.b.l >>= 1;
				setnz(oper.b.l);
			} else {
				oper.v >>= 1;
				setnz(oper);
			}
		/*6.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*7.2*/ write(6, (addr.v+1)&0xffff, oper.b.h);
		}
		/*8.2*/ write(6, addr.v, oper.b.l); poll_irqs();
		/*9.2*/ mcyc(6); fetch();
			break;
	case 0x57: // eor direct ind indexed long (d),y
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.2*/ bank = read(6, (oper.v+2)&0xffff);
		/*6.1*/ t32 = addr.v; t32 += Y.v; bank += t32>>16; addr.v = t32;
		/*6.2*/ oper.v = read(6, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*8.1*/ A.v^=oper.v; if(F.b.M){ setnz(A.b.l); }else { setnz(A); }
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x58: // cli
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ F.b.I = 0;
		/*2.2*/ mcyc(6); fetch(); 
			break;
	case 0x59: // eor abs, y
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; oper.v = addr.v;
			t32 = addr.v; t32 += Y.v;
			bank = t32>>16; bank += db>>16;
			addr.v = t32;
			mcyc(3);
		if( !(F.b.X && oper.b.h == addr.b.h) )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ A.v^=oper.v; if(F.b.M) { setnz(A.b.l); } else { setnz(A); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x5A: // phy
		/*1.1*/ pc++;
		/*1.2*/ mcyc(6);
		if( !F.b.X )
		{
		/*2.1*/ mcyc(3);
		/*2.2*/ write(3, S.v, Y.b.h); S.v--;
		}
		/*3.1*/ //S.v--;
		/*3.2*/ write(6, S.v, Y.b.l); poll_irqs();
		/*4.1*/ S.v--;
		/*4.2*/ mcyc(6); fetch();
			break;
	case 0x5B: // tcd
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ D = A; setnz(A);
		/*2.2*/ mcyc(6); fetch();	
			break;
	case 0x5C: // jmp long
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ bank = read(6, pb|pc); poll_irqs();
		/*4.1*/ pb = bank<<16; pc = addr.v;
		/*4.2*/ mcyc(6); fetch();
			break;
	case 0x5D: // eor abs, x
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; oper.v = addr.v;
			t32 = addr.v; t32 += X.v;
			bank = t32>>16; bank += db>>16;
			addr.v = t32;
			mcyc(3);
		if( !(F.b.X && oper.b.h == addr.b.h) )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ A.v^=oper.v; if(F.b.M) { setnz(A.b.l); } else { setnz(A); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x5E: // lsr abs, x
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; t32 = addr.v; t32 += X.v;
			addr.v = t32; bank = (db>>16)+(t32>>16);
		/*4.2*/ oper.v = read(12, (bank<<16)|addr.v);
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ F.b.C = oper.v&1;
			if(F.b.M) 
			{
				oper.b.l >>=1;
				setnz(oper.b.l); 
			} else {
				oper.v >>= 1;
				setnz(oper);
			}
		/*6.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*7.2*/ write(6, ((bank<<16)|addr.v)+1, oper.b.h);
		}
		/*8.2*/ write(6, (bank<<16)|addr.v, oper.b.l); poll_irqs();
		/*9.2*/ mcyc(6); fetch();
			break;
	case 0x5F: // eor long, x
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ bank = read(6, pb|pc);
		/*4.1*/ pc++; t32 = addr.v; t32 += X.v; bank += t32>>16; addr.v = t32;
		/*4.2*/ oper.v = read(6, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ A.v ^= oper.v;
			if( F.b.M ) { setnz(A.b.l); } else { setnz(A); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x60: // rts
		/*1.1*/ pc++;
		/*3.1*/ S.v++;
		/*3.2*/ oper.b.l = read(18, S.v);
		/*4.2*/ oper.b.h = read(6, S.v+1);
		/*5.2*/ mcyc(6); poll_irqs();
		/*6.1*/ S.v++; pc = oper.v+1;
		/*6.2*/ mcyc(6); fetch();	
		break;
	case 0x61: // adc (dp, x)
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++; addr.v += X.v + D.v;
		/*2.2*/ mcyc(6);
		if( D.b.l )
		{
		/*3.2*/ mcyc(6);
		}
		/*4.2*/ oper.b.l = read(6, addr.v);
		/*5.2*/ oper.b.h = read(6, (addr.v+1)&0xffff); addr = oper;
		/*6.2*/ oper.v = read(6, db|addr.v); poll_irqs();
		if(!F.b.M)
		{
		/*7.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*8.1*/ if( F.b.M )
			{
				A.b.l = add8(A.b.l, oper.b.l);
			} else {
				A.v = add16(A.v, oper.v);
			}
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x62: // per rel long
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ oper.b.h = read(6, pb|pc);
		/*3.1*/ pc++; oper.v += pc;
		/*4.2*/ write(12, S.v, oper.b.h);
		/*5.2*/ write(6, S.v-1, oper.b.l); poll_irqs();
		/*6.1*/ S.v-=2;
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x63: // adc stack rel
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++; addr.v += S.v;
		/*3.2*/ oper.v = read(12, addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*4.2*/ addr.v++; oper.b.h = read(6, addr.v);
		}
		/*5.1*/ if( F.b.M ) { A.b.l = add8(A.b.l, oper.b.l); } else { A.v = add16(A.v, oper.v); }
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0x64: // stz direct
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.1*/ pc++;
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ if( D.b.l==0 ) { pc++; } addr.v = oper.v + D.v;
		/*3.2*/ write(6, addr.v, 0); poll_irqs();
		if( !F.b.M )
		{
		/*4.2*/ write(6, (addr.v+1)&0xffff, 0);
		}
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0x65: // adc direct
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.1*/ pc++;
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ if( !D.b.l ) { pc++; } addr.v = oper.v + D.v;
		/*3.2*/ oper.v = read(6, addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*4.2*/ addr.v++; oper.b.h = read(6, addr.v);
		}
		/*5.1*/ if( F.b.M ) { A.b.l=add8(A.b.l,oper.b.l); } else { A.v=add16(A.v, oper.v); }
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0x66: // ror dp
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/	mcyc(6);
		}
		/*3.1*/ pc++; addr.v = oper.v + D.v;
		/*3.2*/ oper.v = read(6, addr.v);
		if( !F.b.M )
		{
		/*4.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		}
		/*5.1*/ t = F.b.C;
			F.b.C = oper.v&1;
			if( F.b.M ) { oper.b.l >>= 1; oper.b.l|=t<<7; setnz(oper.b.l); } else { oper.v >>= 1; oper.v|=t<<15; setnz(oper); }
		/*5.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*6.2*/ write(6, (addr.v+1)&0xffff, oper.b.h);
		}
		/*7.2*/ write(6, addr.v, oper.b.l); poll_irqs();
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x67: // adc direct ind long
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.2*/ bank = read(6, (oper.v+2)&0xffff);
		/*6.2*/ oper.v = read(6, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*8.1*/ if( F.b.M ) { A.b.l=add8(A.b.l,oper.b.l); } else { A.v=add16(A.v,oper.v); }
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x68: // pla
		/*1.1*/ pc++;
		/*3.1*/ S.v++;
		/*3.2*/ oper.b.l = read(18, S.v); poll_irqs();
		if( !F.b.M )
		{
		/*4.1*/ S.v+=1;
		/*4.2*/ oper.b.h = read(6, S.v);
		}
		/*5.1*/ if( F.b.M )
			{
				A.b.l = oper.b.l;
				setnz(A.b.l);
			} else {
				A = oper;
				setnz(A);
			}
		/*5.2*/ mcyc(6); fetch();		
			break;
	case 0x69: // adc imm
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc); poll_irqs();
		if( !F.b.M )
		{
		/*2.1*/ pc++;
		/*2.2*/ oper.b.h = read(6, pb|pc);
		}
		/*3.1*/ pc++;
			if( F.b.M )
			{
				A.b.l = add8(A.b.l, oper.b.l);
			} else {
				A.v = add16(A.v, oper.v);
			}
		/*3.2*/	mcyc(6); fetch();
			break;
	case 0x6A: // ror a
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ t=F.b.C; 
			F.b.C = A.b.l&1;
			if( F.b.M )
			{
				A.b.l = (A.b.l>>1)|(t<<7);
				setnz(A.b.l);
			} else {
				A.v = (A.v>>1)|(t<<15);
				setnz(A);
			}
		/*2.2*/ mcyc(6); fetch();
			break;
	case 0x6B: // rtl
		/*1.1*/ pc++;
		/*3.2*/ oper.v = read(18, (S.v+1)&0xffff);
		/*4.2*/ oper.b.h = read(6, (S.v+2)&0xffff);
		/*5.2*/ bank = read(6, (S.v+3)&0xffff); poll_irqs();
		/*6.1*/ S.v+=3; pc = oper.v+1; pb = bank<<16;
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x6C: // jmp abs indirect
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ oper.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ addr.v = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff); poll_irqs();
		/*5.1*/ pc = addr.v;
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0x6D: // adc abs
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ oper.v = read(6, db|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*4.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*5.1*/ if(F.b.M) { A.b.l=add8(A.b.l,oper.b.l); } else { A.v=add16(A.v,oper.v); }
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0x6E: // ror abs
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ oper.v = read(6, db|addr.v);
		if( !F.b.M )
		{
		/*4.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*5.1*/ t = F.b.C;
			F.b.C = oper.v&1;
			if( F.b.M ) 
			{ 
				oper.b.l >>= 1;
				oper.b.l |= t<<7;
				setnz(oper.b.l); 
			} else { 
				oper.v >>= 1;
				oper.v |= t<<15;
				setnz(oper);
			}
		/*5.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*6.2*/ write(6, (db|addr.v)+1, oper.b.h);
		}
		/*7.2*/ write(6, db|addr.v, oper.b.l); poll_irqs();
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x6F: // adc long
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ bank = read(6, pb|pc);
		/*4.1*/ pc++;
		/*4.2*/ oper.v = read(6, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ if(F.b.M) { A.b.l=add8(A.b.l,oper.b.l); } else { A.v=add16(A.v,oper.v); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x70: // bvs rel
		/*1.1*/ pc++;
		/*1.2*/ oper.b.l = read(6, pb|pc); addr.v = pc + s8(oper.b.l) + 1; if( F.b.V==0 ) { poll_irqs(); }
		/*2.1*/ pc++; mcyc(3);
		if( F.b.V == 1 )
		{
		/*2.2*/ mcyc(3); poll_irqs();
		/*3.1*/ pc = addr.v; mcyc(3);
		}
		/*3.2*/ mcyc(3); fetch();
			break;
	case 0x71: //adc (dp), y
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.1*/ bank = (db>>16)+((addr.v + Y.v)>>16);
			oper.v = addr.v;
			addr.v += Y.v;
			mcyc(3);
		if( !(F.b.X && oper.b.h == addr.b.h) )
		{
		/*6.1*/ mcyc(6);
		}
		/*6.2*/ oper.v = read(3, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*8.1*/ if(F.b.M) { A.b.l=add8(A.b.l,oper.b.l); } else { A.v=add16(A.v,oper.v); }
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x72: // adc direct ind
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.2*/ oper.v = read(6, db|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*6.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*7.1*/ if(F.b.M) { A.b.l=add8(A.b.l,oper.b.l); } else { A.v=add16(A.v,oper.v); }
		/*7.2*/ mcyc(6); fetch();
			break;
	case 0x73: // adc stack relative ind indexed
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		/*2.1*/ pc++; addr.v = oper.v + S.v;
		/*3.2*/ oper.b.l = read(12, addr.v);
		/*4.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		/*5.1*/ addr.v = oper.v;
			t32 = addr.v; t32 += Y.v;
			addr.v = t32;
			bank = (db>>16)+(t32>>16);
		/*6.2*/ oper.v = read(12, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*8.1*/ if(F.b.M) { A.b.l=add8(A.b.l,oper.b.l); } else { A.v=add16(A.v,oper.v); }
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x74: // stz dp, x
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		/*2.1*/ pc++;
		/*3.1*/ addr.v = oper.v + X.v + D.v; mcyc(9);
		if( D.b.l )
		{
		/*3.2*/
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ write(3, addr.v, 0); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/	addr.v+=1; write(6, addr.v, 0);
		}
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x75: // adc dp, x
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*3.1*/ pc++; addr.v += X.v + D.v; mcyc(9);
		if( D.b.l )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		}
		/*6.1*/ if(F.b.M) { A.b.l=add8(A.b.l,oper.b.l); } else { A.v=add16(A.v,oper.v); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x76: // ror dp, x
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*3.1*/ pc++; addr.v += X.v + D.v; mcyc(9);
		if( D.b.l )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, addr.v);
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		}
		/*6.1*/ t = F.b.C;
			F.b.C = oper.v&1;
			if( F.b.M )
			{
				oper.b.l >>= 1;
				oper.b.l |= t<<7;
				setnz(oper.b.l);
			} else {
				oper.v >>= 1;
				oper.v |= t<<15;
				setnz(oper);
			}
		/*6.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*7.2*/ write(6, (addr.v+1)&0xffff, oper.b.h);
		}
		/*8.2*/ write(6, addr.v, oper.b.l); poll_irqs();
		/*9.2*/ mcyc(6); fetch();
			break;
	case 0x77: // adc direct ind indexed long (d),y
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.2*/ bank = read(6, (oper.v+2)&0xffff);
		/*6.1*/ t32 = addr.v; t32 += Y.v; bank += t32>>16; addr.v = t32;
		/*6.2*/ oper.v = read(6, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*8.1*/ if(F.b.M) { A.b.l=add8(A.b.l,oper.b.l); } else { A.v=add16(A.v,oper.v); }
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x78: // sei
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ F.b.I = 1;
		/*2.2*/ mcyc(6); fetch(); 
			break;
	case 0x79: // adc abs, y
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; oper.v = addr.v;
			t32 = addr.v; t32 += Y.v;
			bank = t32>>16; bank += db>>16;
			addr.v = t32;
			mcyc(3);
		if( !(F.b.X && oper.b.h == addr.b.h) )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ if(F.b.M) { A.b.l=add8(A.b.l,oper.b.l); } else { A.v=add16(A.v,oper.v); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x7A: // ply
		/*1.1*/ pc++;
		/*3.1*/ S.v++;
		/*3.2*/ oper.b.l = read(18, S.v); poll_irqs();
		if( !F.b.X )
		{
		/*4.1*/ S.v++;
		/*4.2*/ oper.b.h = read(6, S.v);
		}
		/*5.1*/ if( F.b.X )
			{
				Y.b.l = oper.b.l;
				setnz(Y.b.l);
			} else {
				Y = oper;
				setnz(Y);
			}
		/*5.2*/ mcyc(6); fetch();		
			break;
	case 0x7B: // tdc
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ A = D; setnz(A);
		/*2.2*/ mcyc(6); fetch();
			break;
	case 0x7C: // jmp (abs,x)
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ addr.v += X.v;
		/*4.2*/ oper.b.l = read(12, pb|addr.v);
		/*5.2*/ oper.b.h = read(6, pb|((addr.v+1)&0xffff)); //todo: carry or no?
			poll_irqs();
		/*6.1*/ pc = oper.v;
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x7D: // adc abs, x
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; oper.v = addr.v;
			t32 = addr.v; t32 += X.v;
			bank = t32>>16; bank += db>>16;
			addr.v = t32;
			mcyc(3);
		if( !(F.b.X && oper.b.h == addr.b.h) )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ if(F.b.M) { A.b.l=add8(A.b.l,oper.b.l); } else { A.v=add16(A.v,oper.v); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x7E: // ror abs, x
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; t32 = addr.v; t32 += X.v;
			addr.v = t32; bank = (db>>16)+(t32>>16);
		/*4.2*/ oper.v = read(12, (bank<<16)|addr.v);
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ t = F.b.C;
			F.b.C = oper.v&1;
			if(F.b.M) 
			{
				oper.b.l >>=1;
				oper.b.l |= t<<7;
				setnz(oper.b.l); 
			} else {
				oper.v >>= 1;
				oper.v |= t<<15;
				setnz(oper);
			}
		/*6.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*7.2*/ write(6, ((bank<<16)|addr.v)+1, oper.b.h);
		}
		/*8.2*/ write(6, (bank<<16)|addr.v, oper.b.l); poll_irqs();
		/*9.2*/ mcyc(6); fetch();
			break;
	case 0x7F: // adc long, x
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ bank = read(6, pb|pc);
		/*4.1*/ pc++; t32 = addr.v; t32 += X.v; bank += t32>>16; addr.v = t32;
		/*4.2*/ oper.v = read(6, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ if(F.b.M) { A.b.l=add8(A.b.l,oper.b.l); } else { A.v=add16(A.v,oper.v); }
		/*6.2*/ mcyc(6); fetch();
			break;	
	case 0x80: // bra
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc); addr.v = s8(oper.b.l) + pc + 1;
		/*2.1*/ pc++;
		/*2.2*/ mcyc(6); poll_irqs();
		/*3.1*/ pc = addr.v;
		/*3.2*/ mcyc(6); fetch();
			break;
	case 0x81: // sta (dp,x)
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		/*2.1*/ pc++; oper.v += X.v + D.v;
		/*2.2*/ mcyc(6);
		if( D.b.l )
		{
		/*3.2*/	mcyc(6);
		}
		/*4.2*/ addr.b.l = read(6, oper.v);
		/*5.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*6.2*/ write(6, db|addr.v, A.b.l); poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ write(6, (db|addr.v)+1, A.b.h);
		}
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x82: // brl
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ oper.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ mcyc(6); poll_irqs();
		/*4.1*/ pc += oper.v;
		/*4.2*/ mcyc(6); fetch();
			break;
	case 0x83: // sta stack rel
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++; addr.v += S.v;
		/*3.2*/ write(12, addr.v, A.b.l); poll_irqs();
		if( !F.b.M )
		{
		/*4.2*/ write(6, (addr.v+1)&0xffff, A.b.h);
		}
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0x84: // sty dp
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; addr.v += D.v;
		/*3.2*/ write(6, addr.v, Y.b.l);
		if( !F.b.X )
		{
		/*4.2*/ write(6, (addr.v+1)&0xffff, Y.b.h);
		}
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0x85: // sta dp
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; addr.v += D.v;
		/*3.2*/ write(6, addr.v, A.b.l);
		if( !F.b.M )
		{
		/*4.2*/ write(6, (addr.v+1)&0xffff, A.b.h);
		}
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0x86: // stx dp
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; addr.v += D.v;
		/*3.2*/ write(6, addr.v, X.b.l);
		if( !F.b.X )
		{
		/*4.2*/ write(6, (addr.v+1)&0xffff, X.b.h);
		}
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0x87: // sta direct ind long
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.2*/ bank = read(6, (oper.v+2)&0xffff);
		/*6.2*/ write(6, (bank<<16)|addr.v, A.b.l);
		if( !F.b.M )
		{
		/*7.2*/ write(6, ((bank<<16)|addr.v)+1, A.b.h);
		}
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x88: // dey
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ if( F.b.X )
			{
				Y.b.l--;
				setnz(Y.b.l);
			} else {
				Y.v--;
				setnz(Y);
			}
		/*2.2*/ mcyc(6); fetch();
			break;
	case 0x89: // bit imm
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc); poll_irqs();
		if( !F.b.M )
		{
		/*2.1*/ pc++;
		/*2.2*/ oper.b.h = read(6, pb|pc);
		}
		/*3.1*/ pc++; if( F.b.M ) { F.b.Z = (A.b.l&oper.b.l)==0; } else { F.b.Z = (A.v&oper.v)==0; }
		/*3.2*/ mcyc(6); fetch();		
			break;
	case 0x8A: // txa
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ if( F.b.M )
			{
				A.b.l = X.b.l;
				setnz(A.b.l);
			} else {
				A = X;
				setnz(A);
			}
		/*2.2*/ mcyc(6); fetch();
			break;
	case 0x8B: // phb
		/*1.1*/ pc++;
		/*1.2*/ mcyc(6); poll_irqs();
		/*2.2*/ write(6, S.v, db>>16);
		/*3.1*/ S.v--;
		/*3.2*/ mcyc(6); fetch();
			break;
	case 0x8C: // sty abs
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.2*/ pc++; addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; oper = Y;
		/*3.2*/ write(6, db|addr.v, oper.b.l); poll_irqs();
		if( !F.b.X )
		{
		/*4.2*/ write(6, (db|addr.v) + 1, oper.b.h);
		}
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0x8D: // sta abs
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.2*/ pc++; addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; oper = A;
		/*3.2*/ write(6, db|addr.v, oper.b.l); poll_irqs();
		if( !F.b.M )
		{
		/*4.2*/ write(6, (db|addr.v) + 1, oper.b.h);
		}
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0x8E: // stx abs
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.2*/ pc++; addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; oper = X;
		/*3.2*/ write(6, db|addr.v, oper.b.l); poll_irqs();
		if( !F.b.X )
		{
		/*4.2*/ write(6, (db|addr.v) + 1, oper.b.h);
		}
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0x8F: // sta long
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ bank = read(6, pb|pc);
		/*4.1*/ pc++; oper = A;
		/*4.2*/ write(6, (bank<<16)|addr.v, oper.b.l); poll_irqs();
		if( !F.b.M )
		{
			/*5.2*/ write(6, ((bank<<16)|addr.v)+1, oper.b.h);
		}
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x90: // bcc rel
		/*1.1*/ pc++;
		/*1.2*/ oper.b.l = read(6, pb|pc); addr.v = pc + s8(oper.b.l) + 1; if( F.b.C==1 ) { poll_irqs(); }
		/*2.1*/ pc++; mcyc(3);
		if( F.b.C == 0 )
		{
		/*2.2*/ mcyc(3); poll_irqs();
		/*3.1*/ pc = addr.v; mcyc(3);
		}
		/*3.2*/ mcyc(3); fetch();
			break;
	case 0x91: // sta (d), y
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.1*/ t32 = addr.v; t32 += Y.v;
			bank = (db>>16)+(t32>>16); addr.v = t32;
		/*6.2*/ write(12, (bank<<16)|addr.v, A.b.l); poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ write(6, ((bank<<16)|addr.v)+1, A.b.h);
		}
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x92: // sta direct ind
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.2*/ write(6, db|addr.v, A.b.l); poll_irqs();
		if( !F.b.M )
		{
		/*6.2*/ write(6, (db|addr.v)+1, A.b.h); //with carry?
		}
		/*7.2*/ mcyc(6); fetch();
			break;
	case 0x93: // sta stack rel ind indexed
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		/*2.1*/ pc++; oper.v += S.v;
		/*3.2*/ addr.b.l = read(12, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.1*/ t32 = addr.v; t32 += Y.v; addr.v = t32; bank = (db>>16)+(t32>>16);
		/*6.2*/ write(12, (bank<<16)|addr.v, A.b.l);
		if( !F.b.M )
		{
		/*7.2*/ write(6, ((bank<<16)|addr.v)+1, A.b.h);
		}
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x94: // sty dp, x
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++;
		/*3.1*/ addr.v += X.v + D.v; mcyc(9);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*4.2*/ write(3, addr.v, Y.b.l); poll_irqs();
		if( !F.b.X )
		{
		/*5.2*/ write(6, (addr.v+1)&0xffff, Y.b.h);
		}
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x95: // sta dp, x
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++;
		/*3.1*/ addr.v += X.v + D.v; mcyc(9);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*4.2*/ write(3, addr.v, A.b.l); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ write(6, (addr.v+1)&0xffff, A.b.h);
		}
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x96: // stx dp, y
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++;
		/*3.1*/ addr.v += Y.v + D.v; mcyc(9);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*4.2*/ write(3, addr.v, X.b.l); poll_irqs();
		if( !F.b.X )
		{
		/*5.2*/ write(6, (addr.v+1)&0xffff, X.b.h);
		}
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0x97: // sta long (d), y
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.2*/ bank = read(6, (oper.v+2)&0xffff);
		/*6.1*/ t32 = addr.v; t32 += Y.v; bank += t32>>16; addr.v=t32;
		/*6.2*/ write(6, (bank<<16)|addr.v, A.b.l); poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ write(6, ((bank<<16)|addr.v)+1, A.b.h);
		}
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0x98: // tya
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ if(F.b.M) { A.b.l=Y.b.l; setnz(A.b.l); } else { A=Y; setnz(A); }
		/*2.2*/ mcyc(6); fetch();
			break;		
	case 0x99: // sta abs, y
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; t32=addr.v; t32+=Y.v; bank=(db>>16)+(t32>>16);
			addr.v = t32;
		/*4.2*/ write(12, (bank<<16)|addr.v, A.b.l); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ write(6, ((bank<<16)|addr.v)+1, A.b.h);
		}
		/*6.2*/ mcyc(6); fetch();	
			break;
	case 0x9A: // txs
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ S = X;
		/*2.2*/ mcyc(6); fetch();
			break;	
	case 0x9B: // txy
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ if(F.b.X) { Y.b.l=X.b.l; setnz(Y.b.l); } else { Y=X; setnz(Y); }
		/*2.2*/ mcyc(6); fetch();
			break;
	case 0x9C: // stz abs
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ write(6, db|addr.v, 0); poll_irqs();
		if( !F.b.M )
		{
		/*4.2*/ write(6, (db|addr.v)+1, 0);
		}
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0x9D: // sta abs, x
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; t32=addr.v; t32+=X.v; bank=(db>>16)+(t32>>16);
			addr.v = t32;
		/*4.2*/ write(12, (bank<<16)|addr.v, A.b.l); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ write(6, ((bank<<16)|addr.v)+1, A.b.h);
		}
		/*6.2*/ mcyc(6); fetch();	
			break;
	case 0x9E: // stz abs, x
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; t32=addr.v; t32+=X.v; bank=(db>>16)+(t32>>16);
			addr.v = t32;
		/*4.2*/ write(12, (bank<<16)|addr.v, 0); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ write(6, ((bank<<16)|addr.v)+1, 0);
		}
		/*6.2*/ mcyc(6); fetch();	
			break;
	case 0x9F: // sta long, x
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ bank = read(6, pb|pc);
		/*4.1*/ pc++; t32 = addr.v; t32+=X.v; bank+=t32>>16; addr.v=t32;
		/*4.2*/ write(6, (bank<<16)|addr.v, A.b.l); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ write(6, ((bank<<16)|addr.v)+1, A.b.h);
		}
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0xA0: // ldy imm
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc); poll_irqs();
		if( !F.b.X )
		{
		/*2.1*/ pc++;
		/*2.2*/ oper.b.h = read(6, pb|pc);
		}
		/*3.1*/ pc++;
			if( F.b.X )
			{
				Y.b.l = oper.b.l;
				setnz(Y.b.l);
			} else {
				Y = oper;
				setnz(Y);
			}
		/*3.2*/ mcyc(6); fetch();
			break;
	case 0xA1: // lda (dp, x)
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++; addr.v += X.v + D.v;
		/*2.2*/ mcyc(6);
		if( D.b.l )
		{
		/*3.2*/ mcyc(6);
		}
		/*4.2*/ oper.b.l = read(6, addr.v);
		/*5.2*/ oper.b.h = read(6, (addr.v+1)&0xffff); addr = oper;
		/*6.2*/ oper.v = read(6, db|addr.v); poll_irqs();
		if(!F.b.M)
		{
		/*7.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*8.1*/ if( F.b.M ) { A.b.l=oper.b.l; setnz(A.b.l); } else { A=oper; setnz(A); }
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0xA2: // ldx imm
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc); poll_irqs();
		if( !F.b.X )
		{
		/*2.1*/ pc++;
		/*2.2*/ oper.b.h = read(6, pb|pc);
		}
		/*3.1*/ pc++;
			if( F.b.X )
			{
				X.b.l = oper.b.l;
				setnz(X.b.l);
			} else {
				X = oper;
				setnz(X);
			}
		/*3.2*/ mcyc(6); fetch();
			break;
	case 0xA3: // lda stack rel
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++; addr.v += S.v;
		/*3.2*/ oper.v = read(12, addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*4.2*/ addr.v++; oper.b.h = read(6, addr.v);
		}
		/*5.1*/ if( F.b.M ) { A.b.l=oper.b.l; setnz(A.b.l); } else { A=oper; setnz(A); }
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0xA4: // ldy direct
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.1*/ pc++;
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ if( !D.b.l ) { pc++; } addr.v = oper.v + D.v;
		/*3.2*/ oper.v = read(6, addr.v); poll_irqs();
		if( !F.b.X )
		{
		/*4.2*/ addr.v++; oper.b.h = read(6, addr.v);
		}
		/*5.1*/ if( F.b.X ) { Y.b.l=oper.b.l; setnz(Y.b.l); } else { Y=oper; setnz(Y); }
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0xA5: // lda direct
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.1*/ pc++;
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ if( !D.b.l ) { pc++; } addr.v = oper.v + D.v;
		/*3.2*/ oper.v = read(6, addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*4.2*/ addr.v++; oper.b.h = read(6, addr.v);
		}
		/*5.1*/ if( F.b.M ) { A.b.l=oper.b.l; setnz(A.b.l); } else { A=oper; setnz(A); }
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0xA6: // ldx direct
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.1*/ pc++;
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ if( !D.b.l ) { pc++; } addr.v = oper.v + D.v;
		/*3.2*/ oper.v = read(6, addr.v); poll_irqs();
		if( !F.b.X )
		{
		/*4.2*/ addr.v++; oper.b.h = read(6, addr.v);
		}
		/*5.1*/ if( F.b.X ) { X.b.l=oper.b.l; setnz(X.b.l); } else { X=oper; setnz(X); }
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0xA7: // lda direct ind long
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.2*/ bank = read(6, (oper.v+2)&0xffff);
		/*6.2*/ oper.v = read(6, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*8.1*/ if( F.b.M ) { A.b.l=oper.b.l; setnz(A.b.l); } else { A=oper; setnz(A); }
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0xA8: // tay
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ if( F.b.X ) { Y.b.l=A.b.l; setnz(Y.b.l); } else { Y=A; setnz(Y); }
		/*2.2*/ mcyc(6); fetch();
			break;
	case 0xA9: // lda imm
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc); poll_irqs();
		if( !F.b.M )
		{
		/*2.1*/ pc++;
		/*2.2*/ oper.b.h = read(6, pb|pc);
		}
		/*3.1*/ pc++;
			if( F.b.M )
			{
				A.b.l = oper.b.l;
				setnz(A.b.l);
			} else {
				A = oper;
				setnz(A);
			}
		/*3.2*/ mcyc(6); fetch();
			break;
	case 0xAA: // tax
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ if( F.b.X )
			{
				X.b.l = A.b.l;
				setnz(X.b.l);
			} else {
				X = A;
				setnz(X);
			}
		/*2.2*/ mcyc(6); fetch();
			break;			
	case 0xAB: // plb
		/*1.1*/ pc++;
		/*3.2*/ S.v++; db = read(18, S.v); poll_irqs();
		/*4.1*/ setnz(u8(db)); db <<= 16;
		/*4.2*/ mcyc(6); fetch();
			break;
	case 0xAC: // ldy abs
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ oper.v = read(6, db|addr.v); poll_irqs();
		if( !F.b.X )
		{
		/*4.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*5.1*/ if(F.b.X) { Y.b.l=oper.b.l; setnz(Y.b.l); } else { Y=oper; setnz(Y); }
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0xAD: // lda abs
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ oper.v = read(6, db|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*4.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*5.1*/ if(F.b.M) { A.b.l=oper.b.l; setnz(A.b.l); } else { A=oper; setnz(A); }
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0xAE: // ldx abs
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ oper.v = read(6, db|addr.v); poll_irqs();
		if( !F.b.X )
		{
		/*4.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*5.1*/ if(F.b.X) { X.b.l=oper.b.l; setnz(X.b.l); } else { X=oper; setnz(X); }
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0xAF: // lda long
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ bank = read(6, pb|pc);
		/*4.1*/ pc++;
		/*4.2*/ oper.v = read(6, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ if( F.b.M ) { A.b.l=oper.b.l; setnz(A.b.l); } else { A=oper; setnz(A); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0xB0: // bcs rel
		/*1.1*/ pc++;
		/*1.2*/ oper.b.l = read(6, pb|pc); addr.v = pc + s8(oper.b.l) + 1; if( F.b.C==0 ) { poll_irqs(); }
		/*2.1*/ pc++; mcyc(3);
		if( F.b.C == 1 )
		{
		/*2.2*/ mcyc(3); poll_irqs();
		/*3.1*/ pc = addr.v; mcyc(3);
		}
		/*3.2*/ mcyc(3); fetch();
			break;
	case 0xB1: //lda (dp), y
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.1*/ bank = (db>>16)+((addr.v + Y.v)>>16);
			oper.v = addr.v;
			addr.v += Y.v;
			mcyc(3);
		if( !(F.b.X && oper.b.h == addr.b.h) )
		{
		/*6.1*/ mcyc(6);
		}
		/*6.2*/ oper.v = read(3, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*8.1*/ if( F.b.M ) { A.b.l=oper.b.l; setnz(A.b.l); } else { A=oper; setnz(A); }
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0xB2: // lda direct ind
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.2*/ oper.v = read(6, db|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*6.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*7.1*/ if( F.b.M ) { A.b.l=oper.b.l; setnz(A.b.l); } else { A=oper; setnz(A); }
		/*7.2*/ mcyc(6); fetch();
			break;
	case 0xB3: // lda stack relative ind indexed
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		/*2.1*/ pc++; addr.v = oper.v + S.v;
		/*3.2*/ oper.b.l = read(12, addr.v);
		/*4.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		/*5.1*/ addr.v = oper.v;
			t32 = addr.v; t32 += Y.v;
			addr.v = t32;
			bank = (db>>16)+(t32>>16);
		/*6.2*/ oper.v = read(12, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*8.1*/ if( F.b.M ) { A.b.l=oper.b.l; setnz(A.b.l); } else { A=oper; setnz(A); }
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0xB4: // ldy dp, x
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*3.1*/ pc++; addr.v += X.v + D.v; mcyc(9);
		if( D.b.l )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, addr.v); poll_irqs();
		if( !F.b.X )
		{
		/*5.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		}
		/*6.1*/ if(F.b.X) { Y.b.l=oper.b.l; setnz(Y.b.l); } else { Y=oper; setnz(Y); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0xB5: // lda dp, x
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*3.1*/ pc++; addr.v += X.v + D.v; mcyc(9);
		if( D.b.l )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		}
		/*6.1*/ if(F.b.M) { A.b.l=oper.b.l; setnz(A.b.l); } else { A=oper; setnz(A); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0xB6: // ldx dp, y
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*3.1*/ pc++; addr.v += Y.v + D.v; mcyc(9);
		if( D.b.l )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, addr.v); poll_irqs();
		if( !F.b.X )
		{
		/*5.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		}
		/*6.1*/ if(F.b.X) { X.b.l=oper.b.l; setnz(X.b.l); } else { X=oper; setnz(X); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0xB7: // lda direct ind indexed long (d),y
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.2*/ bank = read(6, (oper.v+2)&0xffff);
		/*6.1*/ t32 = addr.v; t32 += Y.v; bank += t32>>16; addr.v = t32;
		/*6.2*/ oper.v = read(6, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*8.1*/ if(F.b.M) { A.b.l=oper.b.l; setnz(A.b.l); } else { A=oper; setnz(A); }
		/*8.2*/ mcyc(6); fetch();
			break;			
	case 0xB8: // clv
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ F.b.V = 0;
		/*2.2*/ mcyc(6); fetch();
			break;
	case 0xB9: // lda abs, y
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; oper.v = addr.v;
			t32 = addr.v; t32 += Y.v;
			bank = t32>>16; bank += db>>16;
			addr.v = t32;
			mcyc(3);
		if( !(F.b.X && oper.b.h == addr.b.h) )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ if(F.b.M) { A.b.l=oper.b.l; setnz(A.b.l); } else { A=oper; setnz(A); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0xBA: // tsx
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ if( F.b.X )
			{
				X.b.l = S.b.l;
				setnz(X.b.l);
			} else {
				X = S;
				setnz(X);
			}
		/*2.2*/ mcyc(6); fetch();
			break;	
	case 0xBB: // tyx
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ if( F.b.X )
			{
				X.b.l = Y.b.l;
				setnz(X.b.l);
			} else {
				X = Y;
				setnz(X);
			}
		/*2.2*/ mcyc(6); fetch();
			break;
	case 0xBC: // ldy abs, x
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; oper.v = addr.v;
			t32 = addr.v; t32 += X.v;
			bank = t32>>16; bank += db>>16;
			addr.v = t32;
			mcyc(3);
		if( !(F.b.X && oper.b.h == addr.b.h) )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.X )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ if(F.b.X) { Y.b.l=oper.b.l; setnz(Y.b.l); } else { Y=oper; setnz(Y); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0xBD: // lda abs, x
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; oper.v = addr.v;
			t32 = addr.v; t32 += X.v;
			bank = t32>>16; bank += db>>16;
			addr.v = t32;
			mcyc(3);
		if( !(F.b.X && oper.b.h == addr.b.h) )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ if(F.b.M) { A.b.l=oper.b.l; setnz(A.b.l); } else { A=oper; setnz(A); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0xBE: // ldx abs, y
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; oper.v = addr.v;
			t32 = addr.v; t32 += Y.v;
			bank = t32>>16; bank += db>>16;
			addr.v = t32;
			mcyc(3);
		if( !(F.b.X && oper.b.h == addr.b.h) )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.X )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ if(F.b.X) { X.b.l=oper.b.l; setnz(X.b.l); } else { X=oper; setnz(X); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0xBF: // lda long, x
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ bank = read(6, pb|pc);
		/*4.1*/ pc++; t32 = addr.v; t32 += X.v; bank += t32>>16; addr.v = t32;
		/*4.2*/ oper.v = read(6, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ if( F.b.M ) { A.b.l=oper.b.l; setnz(A.b.l); } else { A=oper; setnz(A); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0xC0: // cpy imm
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc); poll_irqs();
		if( !F.b.X )
		{
		/*2.1*/ pc++;
		/*2.2*/ oper.b.h = read(6, pb|pc);
		}
		/*3.1*/ pc++;
			if( F.b.X )
			{
				F.b.C = (Y.b.l>=oper.b.l);
				F.b.Z = (Y.b.l==oper.b.l);
				F.b.N = ((Y.b.l-oper.b.l)>>7);
			} else {
				F.b.C = (Y.v >= oper.v);
				F.b.Z = (Y.v == oper.v);
				F.b.N = ((Y.v-oper.v)>>15);
			}
		/*3.2*/ mcyc(6); fetch();
			break;
	case 0xC1: // cmp (dp, x)
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++; addr.v += X.v + D.v;
		/*2.2*/ mcyc(6);
		if( D.b.l )
		{
		/*3.2*/ mcyc(6);
		}
		/*4.2*/ oper.b.l = read(6, addr.v);
		/*5.2*/ oper.b.h = read(6, (addr.v+1)&0xffff); addr = oper;
		/*6.2*/ oper.v = read(6, db|addr.v); poll_irqs();
		if(!F.b.M)
		{
		/*7.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*8.1*/ if( F.b.M )
			{
				F.b.C = (A.b.l>=oper.b.l);
				F.b.Z = (A.b.l==oper.b.l);
				F.b.N = ((A.b.l-oper.b.l)>>7);
			} else {
				F.b.C = (A.v >= oper.v);
				F.b.Z = (A.v == oper.v);
				F.b.N = ((A.v-oper.v)>>15);
			}
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0xC2: // rep
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ mcyc(6); poll_irqs();
		/*3.1*/ F.v &= ~oper.v; if( F.b.X ) { X.b.h=Y.b.h=0; }
		/*3.2*/ mcyc(6); fetch();
			break;
	case 0xC3: // cmp stack rel
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++; addr.v += S.v;
		/*3.2*/ oper.v = read(12, addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*4.2*/ addr.v++; oper.b.h = read(6, addr.v);
		}
		/*5.1*/ if( F.b.M )
			{
				F.b.C = (A.b.l>=oper.b.l);
				F.b.Z = (A.b.l==oper.b.l);
				F.b.N = ((A.b.l-oper.b.l)>>7);
			} else {
				F.b.C = (A.v >= oper.v);
				F.b.Z = (A.v == oper.v);
				F.b.N = ((A.v-oper.v)>>15);
			}
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0xC4: // cpy direct
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.1*/ pc++;
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ if( !D.b.l ) { pc++; } addr.v = oper.v + D.v;
		/*3.2*/ oper.v = read(6, addr.v); poll_irqs();
		if( !F.b.X )
		{
		/*4.2*/ addr.v++; oper.b.h = read(6, addr.v);
		}
		/*5.1*/ if( F.b.X )
			{
				F.b.C = (Y.b.l>=oper.b.l);
				F.b.Z = (Y.b.l==oper.b.l);
				F.b.N = ((Y.b.l-oper.b.l)>>7);
			} else {
				F.b.C = (Y.v >= oper.v);
				F.b.Z = (Y.v == oper.v);
				F.b.N = ((Y.v-oper.v)>>15);
			}
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0xC5: // cmp direct
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.1*/ pc++;
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ if( !D.b.l ) { pc++; } addr.v = oper.v + D.v;
		/*3.2*/ oper.v = read(6, addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*4.2*/ addr.v++; oper.b.h = read(6, addr.v);
		}
		/*5.1*/ if( F.b.M )
			{
				F.b.C = (A.b.l>=oper.b.l);
				F.b.Z = (A.b.l==oper.b.l);
				F.b.N = ((A.b.l-oper.b.l)>>7);
			} else {
				F.b.C = (A.v >= oper.v);
				F.b.Z = (A.v == oper.v);
				F.b.N = ((A.v-oper.v)>>15);
			}
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0xC6: // dec dp
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/	mcyc(6);
		}
		/*3.1*/ pc++; addr.v = oper.v + D.v;
		/*3.2*/ oper.v = read(6, addr.v);
		if( !F.b.M )
		{
		/*4.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		}
		/*5.1*/ if( F.b.M ) { oper.b.l--; setnz(oper.b.l); } else { oper.v--; setnz(oper); }
		/*5.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*6.2*/ write(6, (addr.v+1)&0xffff, oper.b.h);
		}
		/*7.2*/ write(6, addr.v, oper.b.l); poll_irqs();
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0xC7: // cmp direct ind long
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.2*/ bank = read(6, (oper.v+2)&0xffff);
		/*6.2*/ oper.v = read(6, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*8.1*/ if( F.b.M )
			{
				F.b.C = (A.b.l>=oper.b.l);
				F.b.Z = (A.b.l==oper.b.l);
				F.b.N = ((A.b.l-oper.b.l)>>7);
			} else {
				F.b.C = (A.v >= oper.v);
				F.b.Z = (A.v == oper.v);
				F.b.N = ((A.v-oper.v)>>15);
			}
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0xC8: // iny
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ if( F.b.X ) { Y.b.l++; setnz(Y.b.l); } else { Y.v++; setnz(Y); }
		/*2.2*/ mcyc(6); fetch();
			break;
	case 0xC9: // cmp imm
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( !F.b.M )
		{
		/*2.1*/ pc++;
		/*2.2*/ oper.b.h = read(6, pb|pc);
		}
		/*3.1*/ pc++; 
			if( F.b.M )
			{
				F.b.C = (A.b.l>=oper.b.l);
				F.b.Z = (A.b.l==oper.b.l);
				F.b.N = ((A.b.l-oper.b.l)>>7);
			} else {
				F.b.C = (A.v>=oper.v);
				F.b.Z = (A.v==oper.v);
				F.b.N = ((A.v-oper.v)>>15);
			}
		/*3.2*/ mcyc(6); fetch();
			break;
	case 0xCA: // dex
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ if( F.b.X ) { X.b.l--; setnz(X.b.l); } else { X.v--; setnz(X); }
		/*2.2*/ mcyc(6); fetch();
			break;
	case 0xCB: // wai
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc);
		/*3.1*/ mcyc(9);
		while( !do_irqs ) 
		{
		/*3.2*/ poll_irqs(); //todo: how to leave loop
		}
			fetch();
			break;
	case 0xCC: // cpy abs
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ oper.v = read(6, db|addr.v); poll_irqs();
		if( !F.b.X )
		{
		/*4.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*5.1*/ if( F.b.X )
			{
				F.b.C = (Y.b.l >= oper.b.l);
				F.b.Z = (Y.b.l == oper.b.l);
				F.b.N = ((Y.b.l-oper.b.l)>>7);
			} else {
				F.b.C = (Y.v >= oper.v);
				F.b.Z = (Y.v == oper.v);
				F.b.N = ((Y.v-oper.v)>>15);
			}
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0xCD: // cmp abs
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ oper.v = read(6, db|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*4.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*5.1*/ if( F.b.M )
			{
				F.b.C = (A.b.l >= oper.b.l);
				F.b.Z = (A.b.l == oper.b.l);
				F.b.N = ((A.b.l-oper.b.l)>>7);
			} else {
				F.b.C = (A.v >= oper.v);
				F.b.Z = (A.v == oper.v);
				F.b.N = ((A.v-oper.v)>>15);
			}
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0xCE: // dec abs
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ oper.v = read(6, db|addr.v);
		if( !F.b.M )
		{
		/*4.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*5.1*/ if( F.b.M ) 
			{ 
				oper.b.l -= 1; 
				setnz(oper.b.l); 
			} else { 
				oper.v -= 1;
				setnz(oper);
			}
		/*5.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*6.2*/ write(6, (db|addr.v)+1, oper.b.h);
		}
		/*7.2*/ write(6, db|addr.v, oper.b.l); poll_irqs();
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0xCF: // cmp long
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ bank = read(6, pb|pc);
		/*4.1*/ pc++;
		/*4.2*/ oper.v = read(6, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ if( F.b.M )
			{
				F.b.C = (A.b.l >= oper.b.l);
				F.b.Z = (A.b.l == oper.b.l);
				F.b.N = ((A.b.l-oper.b.l)>>7);
			} else {
				F.b.C = (A.v >= oper.v);
				F.b.Z = (A.v == oper.v);
				F.b.N = ((A.v-oper.v)>>15);
			}
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0xD0: // bne rel
		/*1.1*/ pc++;
		/*1.2*/ oper.b.l = read(6, pb|pc); addr.v = pc + s8(oper.b.l) + 1; if( F.b.Z==1 ) { poll_irqs(); }
		/*2.1*/ pc++; mcyc(3);
		if( F.b.Z == 0 )
		{
		/*2.2*/ mcyc(3); poll_irqs();
		/*3.1*/ pc = addr.v; mcyc(3);
		}
		/*3.2*/ mcyc(3); fetch();
			break;
	case 0xD1: //cmp (dp), y
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.1*/ bank = (db>>16)+((addr.v + Y.v)>>16);
			oper.v = addr.v;
			addr.v += Y.v;
			mcyc(3);
		if( !(F.b.X && oper.b.h == addr.b.h) )
		{
		/*6.1*/ mcyc(6);
		}
		/*6.2*/ oper.v = read(3, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*8.1*/ if( F.b.M )
			{
				F.b.C = (A.b.l >= oper.b.l);
				F.b.Z = (A.b.l == oper.b.l);
				F.b.N = ((A.b.l-oper.b.l)>>7);
			} else {
				F.b.C = (A.v >= oper.v);
				F.b.Z = (A.v == oper.v);
				F.b.N = ((A.v-oper.v)>>15);
			}
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0xD2: // cmp direct ind
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.2*/ oper.v = read(6, db|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*6.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*7.1*/ if( F.b.M )
			{
				F.b.C = (A.b.l >= oper.b.l);
				F.b.Z = (A.b.l == oper.b.l);
				F.b.N = ((A.b.l-oper.b.l)>>7);
			} else {
				F.b.C = (A.v >= oper.v);
				F.b.Z = (A.v == oper.v);
				F.b.N = ((A.v-oper.v)>>15);
			}
		/*7.2*/ mcyc(6); fetch();
			break;
	case 0xD3: // cmp stack relative ind indexed
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		/*2.1*/ pc++; addr.v = oper.v + S.v;
		/*3.2*/ oper.b.l = read(12, addr.v);
		/*4.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		/*5.1*/ addr.v = oper.v;
			t32 = addr.v; t32 += Y.v;
			addr.v = t32;
			bank = (db>>16)+(t32>>16);
		/*6.2*/ oper.v = read(12, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*8.1*/ if( F.b.M )
			{
				F.b.C = (A.b.l >= oper.b.l);
				F.b.Z = (A.b.l == oper.b.l);
				F.b.N = ((A.b.l-oper.b.l)>>7);
			} else {
				F.b.C = (A.v >= oper.v);
				F.b.Z = (A.v == oper.v);
				F.b.N = ((A.v-oper.v)>>15);
			}
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0xD4: // pei
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; addr.v += D.v;
		/*3.2*/ oper.b.l = read(6, addr.v);
		/*4.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		/*5.2*/ write(6, S.v, oper.b.h); S.v--;
		/*6.2*/ write(6, S.v, oper.b.l); S.v--; poll_irqs();
		/*7.2*/ mcyc(6); fetch();
			break;
	case 0xD5: // cmp dp, x
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*3.1*/ pc++; addr.v += X.v + D.v; mcyc(9);
		if( D.b.l )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		}
		/*6.1*/ if( F.b.M )
			{
				F.b.C = (A.b.l >= oper.b.l);
				F.b.Z = (A.b.l == oper.b.l);
				F.b.N = ((A.b.l-oper.b.l)>>7);
			} else {
				F.b.C = (A.v >= oper.v);
				F.b.Z = (A.v == oper.v);
				F.b.N = ((A.v-oper.v)>>15);
			}
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0xD6: // dec dp, x
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*3.1*/ pc++; addr.v += X.v + D.v; mcyc(9);
		if( D.b.l )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, addr.v);
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		}
		/*6.1*/if( F.b.M )
			{
				oper.b.l -= 1;
				setnz(oper.b.l);
			} else {
				oper.v -= 1;
				setnz(oper);
			}
		/*6.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*7.2*/ write(6, (addr.v+1)&0xffff, oper.b.h);
		}
		/*8.2*/ write(6, addr.v, oper.b.l); poll_irqs();
		/*9.2*/ mcyc(6); fetch();
			break;
	case 0xD7: // cmp direct ind indexed long (d),y
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.2*/ bank = read(6, (oper.v+2)&0xffff);
		/*6.1*/ t32 = addr.v; t32 += Y.v; bank += t32>>16; addr.v = t32;
		/*6.2*/ oper.v = read(6, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*8.1*/ if( F.b.M )
			{
				F.b.C = (A.b.l >= oper.b.l);
				F.b.Z = (A.b.l == oper.b.l);
				F.b.N = ((A.b.l-oper.b.l)>>7);
			} else {
				F.b.C = (A.v >= oper.v);
				F.b.Z = (A.v == oper.v);
				F.b.N = ((A.v-oper.v)>>15);
			}
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0xD8: // cld
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ F.b.D = 0;
		/*2.2*/ mcyc(6); fetch();
			break;
	case 0xD9: // cmp abs, y
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; oper.v = addr.v;
			t32 = addr.v; t32 += Y.v;
			bank = t32>>16; bank += db>>16;
			addr.v = t32;
			mcyc(3);
		if( !(F.b.X && oper.b.h == addr.b.h) )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ if( F.b.M )
			{
				F.b.C = (A.b.l >= oper.b.l);
				F.b.Z = (A.b.l == oper.b.l);
				F.b.N = ((A.b.l-oper.b.l)>>7);
			} else {
				F.b.C = (A.v >= oper.v);
				F.b.Z = (A.v == oper.v);
				F.b.N = ((A.v-oper.v)>>15);
			}
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0xDA: // phx
		/*1.1*/ pc++;
		/*1.2*/ mcyc(6);
		if( !F.b.X )
		{
		/*2.1*/ mcyc(3);
		/*2.2*/ write(3, S.v, X.b.h); S.v--;
		}
		/*3.1*/ //S.v--;
		/*3.2*/ write(6, S.v, X.b.l); poll_irqs();
		/*4.1*/ S.v--;
		/*4.2*/ mcyc(6); fetch();
			break;
	// $DB stp
	case 0xDC: // jmp abs ind long
		/*1.1*/ pc++;
		/*1.2*/ oper.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ oper.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.2*/ if( D.b.l )
			{
				bank = read(6, (oper.v+2)&0xffff);
			} else {
				bank = read(6, (oper.b.h<<8)|((oper.b.l+2)&0xff));
			}
			poll_irqs();
		/*6.1*/ pc = addr.v; pb = bank<<16;
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0xDD: // cmp abs, x
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; oper.v = addr.v;
			t32 = addr.v; t32 += X.v;
			bank = t32>>16; bank += db>>16;
			addr.v = t32;
			mcyc(3);
		if( !(F.b.X && oper.b.h == addr.b.h) )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ if( F.b.M )
			{
				F.b.C = (A.b.l >= oper.b.l);
				F.b.Z = (A.b.l == oper.b.l);
				F.b.N = ((A.b.l-oper.b.l)>>7);
			} else {
				F.b.C = (A.v >= oper.v);
				F.b.Z = (A.v == oper.v);
				F.b.N = ((A.v-oper.v)>>15);
			}
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0xDE: // dec abs, x
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; t32 = addr.v; t32 += X.v;
			addr.v = t32; bank = (db>>16)+(t32>>16);
		/*4.2*/ oper.v = read(12, (bank<<16)|addr.v);
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ if(F.b.M) 
			{
				oper.b.l -= 1;
				setnz(oper.b.l); 
			} else {
				oper.v -= 1;
				setnz(oper);
			}
		/*6.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*7.2*/ write(6, ((bank<<16)|addr.v)+1, oper.b.h);
		}
		/*8.2*/ write(6, (bank<<16)|addr.v, oper.b.l); poll_irqs();
		/*9.2*/ mcyc(6); fetch();
			break;			
	case 0xDF: // cmp long, x
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ bank = read(6, pb|pc);
		/*4.1*/ pc++; t32 = addr.v; t32 += X.v; bank += t32>>16; addr.v = t32;
		/*4.2*/ oper.v = read(6, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ if( F.b.M )
			{
				F.b.C = (A.b.l >= oper.b.l);
				F.b.Z = (A.b.l == oper.b.l);
				F.b.N = ((A.b.l-oper.b.l)>>7);
			} else {
				F.b.C = (A.v >= oper.v);
				F.b.Z = (A.v == oper.v);
				F.b.N = ((A.v-oper.v)>>15);
			}
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0xE0: // cpx imm
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc); poll_irqs();
		if( !F.b.X )
		{
		/*2.1*/ pc++;
		/*2.2*/ oper.b.h = read(6, pb|pc);
		}
		/*3.1*/ pc++;
			if( F.b.X )
			{
				F.b.C = (X.b.l>=oper.b.l);
				F.b.Z = (X.b.l==oper.b.l);
				F.b.N = ((X.b.l-oper.b.l)>>7);
			} else {
				F.b.C = (X.v >= oper.v);
				F.b.Z = (X.v == oper.v);
				F.b.N = ((X.v-oper.v)>>15);
			}
		/*3.2*/ mcyc(6); fetch();
			break;
	case 0xE1: // sbc (dp, x)
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++; addr.v += X.v + D.v;
		/*2.2*/ mcyc(6);
		if( D.b.l )
		{
		/*3.2*/ mcyc(6);
		}
		/*4.2*/ oper.b.l = read(6, addr.v);
		/*5.2*/ oper.b.h = read(6, (addr.v+1)&0xffff); addr = oper;
		/*6.2*/ oper.v = read(6, db|addr.v); poll_irqs();
		if(!F.b.M)
		{
		/*7.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*8.1*/ if( F.b.M )
			{
				A.b.l = sub8(A.b.l, oper.b.l);
			} else {
				A.v = sub16(A.v, oper.v);
			}
		/*8.2*/ mcyc(6); fetch();
			break;

	case 0xE2: // sep
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ mcyc(6); poll_irqs();
		/*3.1*/ F.v |= oper.v; if( F.b.X ) { X.b.h = Y.b.h = 0; }
		/*3.2*/ mcyc(6); fetch();
			break;
	case 0xE3: // sbc stack rel
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++; addr.v += S.v;
		/*3.2*/ oper.v = read(12, addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*4.2*/ addr.v++; oper.b.h = read(6, addr.v);
		}
		/*5.1*/ if( F.b.M ) { A.b.l = sub8(A.b.l, oper.b.l); } else { A.v = sub16(A.v, oper.v); }
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0xE4: // cpx direct
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.1*/ pc++;
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ if( !D.b.l ) { pc++; } addr.v = oper.v + D.v;
		/*3.2*/ oper.v = read(6, addr.v); poll_irqs();
		if( !F.b.X )
		{
		/*4.2*/ addr.v++; oper.b.h = read(6, addr.v);
		}
		/*5.1*/ if( F.b.X )
			{
				F.b.C = (X.b.l>=oper.b.l);
				F.b.Z = (X.b.l==oper.b.l);
				F.b.N = ((X.b.l-oper.b.l)>>7);
			} else {
				F.b.C = (X.v >= oper.v);
				F.b.Z = (X.v == oper.v);
				F.b.N = ((X.v-oper.v)>>15);
			}
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0xE5: // sbc direct
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.1*/ pc++;
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ if( !D.b.l ) { pc++; } addr.v = oper.v + D.v;
		/*3.2*/ oper.v = read(6, addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*4.2*/ addr.v++; oper.b.h = read(6, addr.v);
		}
		/*5.1*/ if( F.b.M ) { A.b.l=sub8(A.b.l,oper.b.l); } else { A.v=sub16(A.v, oper.v); }
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0xE6: // inc dp
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/	mcyc(6);
		}
		/*3.1*/ pc++; addr.v = oper.v + D.v;
		/*3.2*/ oper.v = read(6, addr.v);
		if( !F.b.M )
		{
		/*4.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		}
		/*5.1*/ if( F.b.M ) { oper.b.l++; setnz(oper.b.l); } else { oper.v++; setnz(oper); }
		/*5.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*6.2*/ write(6, (addr.v+1)&0xffff, oper.b.h);
		}
		/*7.2*/ write(6, addr.v, oper.b.l); poll_irqs();
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0xE7: // sbc direct ind long
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.2*/ bank = read(6, (oper.v+2)&0xffff);
		/*6.2*/ oper.v = read(6, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*8.1*/ if( F.b.M ) { A.b.l=sub8(A.b.l,oper.b.l); } else { A.v=sub16(A.v,oper.v); }
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0xE8: // inx
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ if( F.b.X )
			{
				X.b.l += 1;
				setnz(X.b.l);
			} else {
				X.v += 1;
				setnz(X);
			}
		/*2.2*/ mcyc(6); fetch();
			break;
	case 0xE9: // sbc imm
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc); poll_irqs();
		if( !F.b.M )
		{
		/*2.1*/ pc++;
		/*2.2*/ oper.b.h = read(6, pb|pc);
		}
		/*3.1*/ pc++;
			if( F.b.M )
			{
				A.b.l = sub8(A.b.l, oper.b.l);
			} else {
				A.v = sub16(A.v, oper.v);
			}
		/*3.2*/	mcyc(6); fetch();
			break;
	case 0xEA: // nop
		/*1.1*/ pc++; 
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ 
		/*2.2*/	mcyc(6); fetch(); 
		break;
	case 0xEB: // xba
		/*1.1*/ pc++;
		/*2.2*/ mcyc(12); poll_irqs();
		/*3.1*/ t = A.b.l; A.b.l = A.b.h; A.b.h = t; setnz(A.b.l);
		/*3.2*/ mcyc(6); fetch();
			break;
	case 0xEC: // cpx abs
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ oper.v = read(6, db|addr.v); poll_irqs();
		if( !F.b.X )
		{
		/*4.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*5.1*/ if( F.b.X )
			{
				F.b.C = (X.b.l >= oper.b.l);
				F.b.Z = (X.b.l == oper.b.l);
				F.b.N = ((X.b.l-oper.b.l)>>7);
			} else {
				F.b.C = (X.v >= oper.v);
				F.b.Z = (X.v == oper.v);
				F.b.N = ((X.v-oper.v)>>15);
			}
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0xED: // sbc abs
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ oper.v = read(6, db|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*4.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*5.1*/ if(F.b.M) { A.b.l=sub8(A.b.l,oper.b.l); } else { A.v=sub16(A.v,oper.v); }
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0xEE: // inc abs
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ oper.v = read(6, db|addr.v);
		if( !F.b.M )
		{
		/*4.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*5.1*/ if( F.b.M ) 
			{ 
				oper.b.l += 1; 
				setnz(oper.b.l); 
			} else { 
				oper.v += 1;
				setnz(oper);
			}
		/*5.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*6.2*/ write(6, (db|addr.v)+1, oper.b.h);
		}
		/*7.2*/ write(6, db|addr.v, oper.b.l); poll_irqs();
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0xEF: // sbc long
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ bank = read(6, pb|pc);
		/*4.1*/ pc++;
		/*4.2*/ oper.v = read(6, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ if(F.b.M) { A.b.l=sub8(A.b.l,oper.b.l); } else { A.v=sub16(A.v,oper.v); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0xF0: // beq rel
		/*1.1*/ pc++;
		/*1.2*/ oper.b.l = read(6, pb|pc); addr.v = pc + s8(oper.b.l) + 1; if( F.b.Z==0 ) { poll_irqs(); }
		/*2.1*/ pc++; mcyc(3);
		if( F.b.Z == 1 )
		{
		/*2.2*/ mcyc(3); poll_irqs();
		/*3.1*/ pc = addr.v; mcyc(3);
		}
		/*3.2*/ mcyc(3); fetch();
			break;
	case 0xF1: //sbc (dp), y
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.1*/ bank = (db>>16)+((addr.v + Y.v)>>16);
			oper.v = addr.v;
			addr.v += Y.v;
			mcyc(3);
		if( !(F.b.X && oper.b.h == addr.b.h) )
		{
		/*6.1*/ mcyc(6);
		}
		/*6.2*/ oper.v = read(3, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*8.1*/ if(F.b.M) { A.b.l=sub8(A.b.l,oper.b.l); } else { A.v=sub16(A.v,oper.v); }
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0xF2: // sbc direct ind
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.2*/ oper.v = read(6, db|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*6.2*/ oper.b.h = read(6, (db|addr.v)+1);
		}
		/*7.1*/ if(F.b.M) { A.b.l=sub8(A.b.l,oper.b.l); } else { A.v=sub16(A.v,oper.v); }
		/*7.2*/ mcyc(6); fetch();
			break;
	case 0xF3: // sbc stack relative ind indexed
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		/*2.1*/ pc++; addr.v = oper.v + S.v;
		/*3.2*/ oper.b.l = read(12, addr.v);
		/*4.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		/*5.1*/ addr.v = oper.v;
			t32 = addr.v; t32 += Y.v;
			addr.v = t32;
			bank = (db>>16)+(t32>>16);
		/*6.2*/ oper.v = read(12, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*8.1*/ if(F.b.M) { A.b.l=sub8(A.b.l,oper.b.l); } else { A.v=sub16(A.v,oper.v); }
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0xF4: // pea abs
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ oper.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ write(6, S.v, oper.b.h); S.v--;
		/*4.2*/ write(6, S.v, oper.b.l); S.v--; poll_irqs();
		/*5.2*/ mcyc(6); fetch();
			break;
	case 0xF5: // sbc dp, x
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*3.1*/ pc++; addr.v += X.v + D.v; mcyc(9);
		if( D.b.l )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		}
		/*6.1*/ if(F.b.M) { A.b.l=sub8(A.b.l,oper.b.l); } else { A.v=sub16(A.v,oper.v); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0xF6: // inc dp, x
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*3.1*/ pc++; addr.v += X.v + D.v; mcyc(9);
		if( D.b.l )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, addr.v);
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, (addr.v+1)&0xffff);
		}
		/*6.1*/if( F.b.M )
			{
				oper.b.l += 1;
				setnz(oper.b.l);
			} else {
				oper.v += 1;
				setnz(oper);
			}
		/*6.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*7.2*/ write(6, (addr.v+1)&0xffff, oper.b.h);
		}
		/*8.2*/ write(6, addr.v, oper.b.l); poll_irqs();
		/*9.2*/ mcyc(6); fetch();
			break;
	case 0xF7: // sbc direct ind indexed long (d),y
		/*1.1*/ pc++;
		/*1.2*/ oper.v = read(6, pb|pc);
		if( D.b.l )
		{
		/*2.2*/ mcyc(6);
		}
		/*3.1*/ pc++; oper.v += D.v;
		/*3.2*/ addr.b.l = read(6, oper.v);
		/*4.2*/ addr.b.h = read(6, (oper.v+1)&0xffff);
		/*5.2*/ bank = read(6, (oper.v+2)&0xffff);
		/*6.1*/ t32 = addr.v; t32 += Y.v; bank += t32>>16; addr.v = t32;
		/*6.2*/ oper.v = read(6, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*7.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*8.1*/ if(F.b.M) { A.b.l=sub8(A.b.l,oper.b.l); } else { A.v=sub16(A.v,oper.v); }
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0xF8: // sec
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ F.b.D = 1;
		/*2.2*/ mcyc(6); fetch();
			break;
	case 0xF9: // sbc abs, y
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; oper.v = addr.v;
			t32 = addr.v; t32 += Y.v;
			bank = t32>>16; bank += db>>16;
			addr.v = t32;
			mcyc(3);
		if( !(F.b.X && oper.b.h == addr.b.h) )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ if(F.b.M) { A.b.l=sub8(A.b.l,oper.b.l); } else { A.v=sub16(A.v,oper.v); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0xFA: // plx
		/*1.1*/ pc++;
		/*3.1*/ S.v++;
		/*3.2*/ oper.b.l = read(18, S.v); poll_irqs();
		if( !F.b.X )
		{
		/*4.1*/ S.v++;
		/*4.2*/ oper.b.h = read(6, S.v);
		}
		/*5.1*/ if( F.b.X )
			{
				X.b.l = oper.b.l;
				setnz(X.b.l);
			} else {
				X = oper;
				setnz(X);
			}
		/*5.2*/ mcyc(6); fetch();		
		break;
	case 0xFB: // xce
		/*1.1*/ pc++;
		/*1.2*/ read(6, pb|pc); poll_irqs();
		/*2.1*/ t = F.b.C; F.b.C = E; E = t;
			if( E ) 
			{ 
				F.b.M = F.b.X = 1; 
				S.b.h = 1;
				X.b.h=Y.b.h=0;
				//std::println("Emu mode entered!");
			}
		/*2.2*/ mcyc(6); fetch();
			break;
	case 0xFC: // jsr (abs, x)
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ write(6, S.v, pc>>8); S.v--;
		/*3.2*/ write(6, S.v, pc); S.v--;
		/*4.2*/ addr.b.h = read(6, pb|pc);
		/*5.1*/ addr.v += X.v;
		/*6.2*/ oper.b.l = read(12, (pb|addr.v));
		/*7.2*/ oper.b.h = read(6, pb|((addr.v+1)&0xffff)); poll_irqs();
		/*8.1*/ pc = oper.v;
		/*8.2*/ mcyc(6); fetch();
			break;
	case 0xFD: // sbc abs, x
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; oper.v = addr.v;
			t32 = addr.v; t32 += X.v;
			bank = t32>>16; bank += db>>16;
			addr.v = t32;
			mcyc(3);
		if( !(F.b.X && oper.b.h == addr.b.h) )
		{
		/*4.1*/ mcyc(6);
		}
		/*4.2*/ oper.v = read(3, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ if(F.b.M) { A.b.l=sub8(A.b.l,oper.b.l); } else { A.v=sub16(A.v,oper.v); }
		/*6.2*/ mcyc(6); fetch();
			break;
	case 0xFE: // inc abs, x
		/*1.1*/ pc++;
		/*1.2*/ addr.b.l = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++; t32 = addr.v; t32 += X.v;
			addr.v = t32; bank = (db>>16)+(t32>>16);
		/*4.2*/ oper.v = read(12, (bank<<16)|addr.v);
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ if(F.b.M) 
			{
				oper.b.l += 1;
				setnz(oper.b.l); 
			} else {
				oper.v += 1;
				setnz(oper);
			}
		/*6.2*/ mcyc(6);
		if( !F.b.M )
		{
		/*7.2*/ write(6, ((bank<<16)|addr.v)+1, oper.b.h);
		}
		/*8.2*/ write(6, (bank<<16)|addr.v, oper.b.l); poll_irqs();
		/*9.2*/ mcyc(6); fetch();
			break;			
	case 0xFF: // sbc long, x
		/*1.1*/ pc++;
		/*1.2*/ addr.v = read(6, pb|pc);
		/*2.1*/ pc++;
		/*2.2*/ addr.b.h = read(6, pb|pc);
		/*3.1*/ pc++;
		/*3.2*/ bank = read(6, pb|pc);
		/*4.1*/ pc++; t32 = addr.v; t32 += X.v; bank += t32>>16; addr.v = t32;
		/*4.2*/ oper.v = read(6, (bank<<16)|addr.v); poll_irqs();
		if( !F.b.M )
		{
		/*5.2*/ oper.b.h = read(6, ((bank<<16)|addr.v)+1);
		}
		/*6.1*/ if(F.b.M) { A.b.l=sub8(A.b.l,oper.b.l); } else { A.v=sub16(A.v,oper.v); }
		/*6.2*/ mcyc(6); fetch();
			break;
	default:
		std::println("65816: Unimpl opc ${:X}", opc);
		exit(1);	
	}
}
}

u8 c65816::sub8(u8 a, u8 b)
{
	if( F.b.D == 0 ) { return add8(a,b^0xff); }
	s16 lo = s16(a&15) - s16(b&15) - (1-F.b.C);
	if( lo < 0 ) lo -= 6;
	u8 borrow = (lo < 0);
	s16 hisum = (a>>4) - (b>>4) - borrow;
	F.b.V = (((a>>4)^(b>>4))&((a>>4)^hisum)&8)!=0;
	if( hisum < 0 ) hisum -= 6;
	F.b.C = hisum>=0;
	u8 res = (((hisum&15)<<4)|(lo&15));
	F.b.Z = (res==0);
	F.b.N = res>>7;
	return res;
}

u16 c65816::sub16(u16 a, u16 b)
{
	if( F.b.D == 0 ) { return add16(a,b^0xffff); }
	u16 res = 0; u16 borrow = 1 - F.b.C;
	for(u32 i = 0; i < 16; i+=4)
	{
		s16 ds = ((a>>i)&15) - ((b>>i)&15) - borrow;
		if( i == 12 ) { F.b.V = (((a>>12)^(b>>12))&((a>>12)^ds)&8)!=0; }
		if( ds < 0 ) ds -= 6;
		borrow = (ds<0);
		res |= (ds&15)<<i;	
	}
	F.b.C = (borrow==0);
	F.b.Z = (res==0);
	F.b.N = (res>>15);
	return res;
}

u16 c65816::add16(u16 a, u16 b)
{
	if( F.b.D == 0 )
	{
		u32 res = a;
		res += b;
		res += F.b.C;
		F.b.C = res>>16;
		F.b.V = (((res^a)&(res^b))>>15)&1;
		F.b.Z = ((res&0xffff)==0);
		F.b.N = (res>>15)&1;
		return res;	
	}
	u16 res = 0;
	u16 carry = F.b.C;
	for(u32 i = 0; i < 16; i+=4)
	{
		u16 ds = ((a>>i)&15) + ((b>>i)&15) + carry;
		if( i==12 ) F.b.V = (~((a>>12)^(b>>12))&((a>>12)^ds)&8)!=0;
		if( ds > 9 ) ds += 6;
		carry = ds>15;
		res |= (ds&15)<<i;
	}
	F.b.C = carry;
	F.b.Z = (res==0);
	F.b.N = res>>15;
	return res;
}

u8 c65816::add8(u8 a, u8 b)
{
	if( F.b.D == 0 )
	{
		u16 res = a;
		res += b;
		res += F.b.C;
		F.b.C = res>>8;
		F.b.V = (((res^a)&(res^b))>>7)&1;
		F.b.Z = ((res&0xff)==0);
		F.b.N = (res>>7)&1;
		return res;
	}
	u16 lo = a&0xf;
	lo += b&0xf;
	lo += F.b.C;
	if( lo > 9 ) lo += 6;
	u8 carryToHi = (lo > 15)?1:0;
	u16 hisum = u16(a>>4) + u16(b>>4) + carryToHi;
	F.b.V = (~((a>>4) ^ (b>>4)) & ((a>>4)^hisum) & 8) != 0;
	if( hisum > 9 ) hisum += 6;
	F.b.C = hisum>15;
	u8 res = ((hisum&15)<<4)|(lo&15);
	F.b.Z = (res==0);
	F.b.N = res>>7;
	return res;
}

