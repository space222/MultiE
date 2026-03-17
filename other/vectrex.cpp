#include <print>
#include "util.h"
#include "vectrex.h"

u8 Vectrex::rd_pia(u16 a)
{
	a &= 0xF;
	//std::println("${:X}: PIA rd ${:X}", cpu.pc-1, a);
	switch( a )
	{

	case 2: return pia.DDRB;
	case 3: return pia.DDRA;
	
	case 4: pia.IFR &= ~BIT(6); 
		if( !(pia.IFR & 0x7F) ) { pia.IFR &= 0x7F; }
		cpu.irq_line = (pia.IER & pia.IFR & 0x7F);
		return pia.T1C&0xff;
	case 5: return pia.T1C>>8;
	case 6: return pia.T1L_L;
	case 7: return pia.T1L_H;
	case 8: pia.IFR &= ~BIT(5); 
		if( !(pia.IFR & 0x7F) ) { pia.IFR &= 0x7F; }
		cpu.irq_line = (pia.IER & pia.IFR & 0x7F);
		return pia.T2C>>8;
	case 9: return pia.T2C&0xff;
	case 10: return pia.SR;
	case 11: return pia.ACR;
	case 12: return pia.PCR;
	case 13: return pia.IFR;
	case 14: return pia.IER;
	}
	return 0;
}

void Vectrex::wr_pia(u16 a, u8 v)
{
	a &= 0xF;
std::println("${:X}: PIA wr ${:X} = ${:X}", cpu.pc-1, a, v);	
	switch( a )
	{
	case 0: pia.ORB = v; sendMUX(); return;
	case 15:
	case 1: pia.ORA = v; beam.dx = DAC = v; sendMUX(); return;
	case 2: pia.DDRB = v; return;
	case 3: pia.DDRA = v; return;

	case 4:
	case 6: pia.T1L_L = v; return;
	case 7: pia.T1L_H = v; return;
	case 5: pia.T1L_H = v; 
		pia.T1C = (pia.T1L_H<<8)|pia.T1L_L; 
		pia.IFR&=~BIT(6);
		if( !(pia.IFR & 0x7F) ) { pia.IFR &= 0x7F; }
		cpu.irq_line = (pia.IER & pia.IFR & 0x7F);
		if( pia.ACR & BIT(7) ) { pia.T1_PB7 = 0; }
		return;
		
	case 8: pia.T2L_L = v; return;
	case 9: pia.T2L_H = v; 
		pia.T2C = (pia.T2L_H<<8)|pia.T2L_L; 
		pia.IFR &= ~BIT(5); 
		if( !(pia.IFR & 0x7F) ) { pia.IFR &= 0x7F; }
		cpu.irq_line = (pia.IER & pia.IFR & 0x7F);
		return;
	case 10: pia.SR = v; return;
	case 11: pia.ACR = v; return;
	case 12: pia.PCR = v; return;
	case 13: pia.IFR &= ~v; pia.IFR&=0x7f; pia.IFR |= ((pia.IFR&0x7f) ? 0x80 : 0); return;
	case 14: if( v & 0x80 ) { pia.IER |= v; } else { pia.IER &= ~v; } pia.IER |= 0x80; return;
	}
}

void Vectrex::sendMUX()
{
	if( (pia.ORB & 1) == 1 ) return;
	switch( (pia.ORB>>1) & 3 )
	{
	case 0: beam.dy = DAC; return;
	case 1: beam.doff = DAC; return;
	case 2: beam.Z = DAC; return;
	case 3: return; //internal.txt: "Connected to sound output line via divider network" ???
	}
}

u8 Vectrex::read(u16 a)
{
	if( a < 0x8000 ) { return cart[a]; }
	if( a < 0xc800 ) { std::println("${:X}: rd ub${:X}", cpu.pc-1, a); return 0; } // unmapped space
	if( a < 0xd000 ) { return ram[a&0x3ff]; }
	if( a < 0xd7ff ) { return rd_pia(a); }
	if( a < 0xe000 ) { std::println("${:X}: rd ub${:X}", cpu.pc-1, a); return 0; } // PIA/RAM bus conflict
	return bios[a&0x1fff];
}

void Vectrex::write(u16 a, u8 v)
{
	if( a >= 0xc800 && a < 0xd000 ) { ram[a&0x3ff] = v; return; }
	if( a >= 0xd000 && a < 0xd800 ) { wr_pia(a,v); return; }
	std::println("Unimpl. Wr ${:X} = ${:X}", a, v);
	exit(1);
}

void Vectrex::cycle()
{
	stamp += 1;
	if( stamp & 1 )
	{
		//run timers, including shifter and the modes that alter RAMP
		pia.T1C -= 1;
		if( pia.T1C == 0 )
		{
			if( pia.ACR & BIT(7) )
			{
				pia.T1_PB7 ^= 1;
			}
			if( pia.ACR & BIT(6) )
			{
				pia.T1C = (pia.T1L_H<<8)|pia.T1L_L;
			}
			pia.IFR |= 0x80|BIT(6);
			cpu.irq_line = (pia.IER & pia.IFR & 0x7F);
		}
		
		if( pia.ACR & BIT(7) )
		{
			if( pia.T1_PB7 ) 
			{
				pia.ORB |= BIT(7);
			} else {
				pia.ORB &= ~BIT(7);
			}
		}
		
		if( !(pia.ACR & BIT(5)) )
		{
			pia.T2C -= 1;
			if( pia.T2C == 0 )
			{ //todo: extra latch for "one shot" or ???
				pia.IFR |= 0x80|BIT(5);
				cpu.irq_line = (pia.IER & pia.IFR & 0x7F);			
			}		
		}
	}
	
	if( ((pia.PCR>>1) & 7) == 6 )
	{
		beam.px = beam.py = 0;
		beam.dx = beam.dy = 0;
		screen[320*640 + 320] += 0.5f;
		return;
	}
	
	// calculate how many pixels we're going to move
	// possibly towards zero, or in beam.dx,beam.dy direction, or if ZERO staying at zero
	// along the way, if !BLANK, brighten phosphors based on beam.Z
	if( (pia.ORB & BIT(7)) == 0 )
	{
		u32 x = beam.px + 320;
		u32 y = beam.py + 240;
		screen[y*640 + x] += 0.5f;
		beam.py = std::clamp(beam.py + beam.dy * (10.f/256), -240.f, 240.f);
		beam.px = std::clamp(beam.px + beam.dx * (10.f/256), -320.f, 320.f);
	}
}

void Vectrex::run_frame()
{
	for(u32 y = 0; y < 480; ++y)
	{
		for(u32 x = 0; x < 640; ++x)
		{
			screen[y*640 + x] = std::max<float>(screen[y*640 + x]-0.2f, 0.f);
		}
	}

	u64 target = last_target + 30000;
	while( stamp < target )
	{
		u64 cycles = cpu.step();
		for(u32 i = 0; i < cycles; ++i)
		{
			cycle();
		}
	}
	last_target = target;
	
	for(u32 y = 0; y < 480; ++y)
	{
		for(u32 x = 0; x < 640; ++x)
		{
			float C = screen[y*640 + x];
			C = std::min<float>(C, 1.f);
			u32 rgb = C*255;
			rgb = (rgb<<24)|(rgb<<16)|(rgb<<8);
			fbuf[y*640 + x] = rgb;
		}
	}
}

bool Vectrex::loadROM(std::string fname)
{
	if( !freadall(bios, fopen("./bios/vectrex.bios", "rb"), 8_KB) )
	{
		std::println("Unable to open Vectrex BIOS (./bios/vectrex.bios)");
		return false;
	}

	if( !freadall(cart, fopen(fname.c_str(), "rb"), 32_KB) )
	{
		std::println("Unable to open Vectrex cart ({})", fname);
		return false;
	}
	
	cpu.writer = [&](u16 a, u8 v) { write(a, v); };
	cpu.reader = [&](u16 a)->u8 { return read(a); };
	
	return true;
}


void Vectrex::reset()
{
	cpu.reset(__builtin_bswap16(*(u16*)&bios[0x1ffe]));
	memset(screen, 0, 640*480*4);
}


