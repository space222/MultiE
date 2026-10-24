#include <string>
#include <cstdio>
#include <cstdlib>
#include <utility>
#include "macplus.h"
#include "util.h"
extern console* sys;
extern u8 byte2gcr[]; //at bottom


bool dolog = false;

void macplus::step()
{
	cpu.step();
	u64 cc = cpu.icycles;
	stamp += cc;
	
	// run one sec irq
	onesec += cc;
	if( onesec >= 7833600 )
	{
		onesec -= 7833600;
		via_vIFR |= BIT(0);
		if( via_vIFR & via_vIER & 0x7f )
		{
			via_vIFR |= BIT(7);
			cpu.setIPL(cpu.getIPL()|1);
		}
	}

	// run via timer 2
	via_tmr2div += cc;
	while( via_tmr2div >= 10 )
	{
		via_tmr2div -= 10;
		via_tmr2 -= 1;
		if( via_tmr2 == 0 && !via_tmr2irq )
		{
			via_tmr2irq = 1;
			via_vIFR |= BIT(5);
			if( via_vIFR & via_vIER & 0x7f )
			{
				via_vIFR |= BIT(7);
				cpu.setIPL(cpu.getIPL()|1);
			}
		}
	}
	
	// run floppy
	floppy_div += cc;
	while( floppy_div >= 128 )
	{
		floppy_div -= 128;
		floppyA_data = floppyA->data[floppyA->track_offsets[(via_vBufA&BIT(5))?1:0][floppyA_track]+(curtrack_bitcount/8)];
		curtrack_bitcount += 8;
		if( curtrack_bitcount >= floppyA->track_lengths[(via_vBufA&BIT(5))?1:0][floppyA_track] )
		{
			curtrack_bitcount = 0;
		}
	}
	
	int tachmax = 0;
	if( floppyA_track >= 64 ) { tachmax = 6632; }
	else if( floppyA_track >= 48 ) { tachmax = 7460; }
	else if( floppyA_track >= 32 ) { tachmax = 8289; }
	else if( floppyA_track >= 16 ) { tachmax = 9117; }
	else { tachmax = 9946; }

	tachdiv += cc;
	while( tachdiv >= tachmax )
	{
		tachdiv -= tachmax;
		tach ^= 0x80;
	}
	/*
Zone 0	Tracks 00 – 15	393.8 RPM	~787.6 Hz	Toggle every 9,946 cycles
Zone 1	Tracks 16 – 31	429.6 RPM	~859.2 Hz	Toggle every 9,117 cycles
Zone 2	Tracks 32 – 47	472.5 RPM	~945.0 Hz	Toggle every 8,289 cycles
Zone 3	Tracks 48 – 63	525.0 RPM	~1050.0 Hz	Toggle every 7,460 cycles
Zone 4	Tracks 64 – 79	590.6 RPM	~1181.2 Hz	Toggle every 6,632 cycles
	*/
	
	// keyboard shifting
	if( kb_shift )
	{
		kb_shift -= cc;
		if( kb_shift <= 0 )
		{
			kb_shift = (kb_cmd ? 8000 : 0);
			kb_cmd = 0;
			via_vIFR |= BIT(2);
			std::println("IRQ b2 keyboard");
			if( (via_vIFR & via_vIER & 0x7f) )
			{
				via_vIFR |= BIT(7);
				cpu.setIPL(cpu.getIPL()|1);
			}
		}
	}
}

void macplus::run_frame()
{
	target = last_target + 120384;
	while( stamp < target )
	{
		step();
	}
	
	// vblank start
	via_vIFR |= BIT(1);
	if( via_vIFR & via_vIER & 0x7f )
	{
		via_vIFR |= BIT(7);
		cpu.setIPL(cpu.getIPL()|1);
	}
	
	target = last_target + 130240;
	while( stamp < target )
	{
		step();
	}
	last_target = target;
	
	for(u32 y = 0; y < 342; ++y)
	{
		for(u32 x = 0; x < 512; ++x)
		{
			u8 v = mainRAM[0x3FA700 + (y*(512/8)) + (x/8)];
			v >>= 7^(x&7);
			fbuf[512*y + x] = (v&1) ? 0 : 0xffFFff00;
		}
	}
}

void macplus::write(u32 a, u32 v, int sz)
{
	a &= 0xffFFff;
	if( a < 0x400000 ) return sized_write_be(mainRAM, a, v, sz);

	if( a >= 0xD80000 && a < 0xE00000 )
	{
		switch( (a-0xDFE1FF)/512 )
		{
		case 8:
			//std::println("IWMw Motor Off");
			iwm_enable = 0;
			return;
		case 12:
			//std::println("IWMw q6 low");
			iwm_q6 = 0;
			return;
		case 13:
			//std::println("IWMw q6 high");
			iwm_q6 = 1;
			return;
		case 14:
			//std::println("IWMw q7 low");
			iwm_q7 = 0;
			return;
		case 15:
			//std::println("IWMw q7 high, latch = ${:X}", v);
			iwm_q7 = 1;
			if( iwm_q6 == 1 )
			{
				iwm_mode = v;
				if( iwm_mode != 0x1F )
				{
					std::println(stderr, "iwm mode set to ${:X}!", v);
					//exit(1);
				}
			}
			return;
		default: break;
		}
		std::println("Unimpl IWM wr{} ${:X} {} = ${:X}", sz, a, (a-0xDFE1FF)/512, v);
		exit(1);
		//return;
	}
	
	if( a >= 0xE80000 && a < 0xF00000 )
	{
		switch( (a-0xEFE1FE)/512 )
		{
		case 0:
			//std::println("VIA BufB = ${:X}", v);
			return;
		case 2:
			//std::println("VIA DirB = ${:X}", v);
			return;
		case 3:
			//std::println("VIA DirA = ${:X}", v);
			return;
		
		case 8:
			//std::println("VIA Tmr2_lo = ${:X}", v);
			via_tmr2latch = v;
			return;
		case 9:
			//std::println("VIA Tmr2_hi = ${:X}", v);
			via_tmr2irq = 0;
			via_tmr2 = (v<<8)|via_tmr2latch;
			
			via_vIFR &= ~BIT(5);
			if( !(via_vIFR & via_vIER & 0x7F) )
			{
				via_vIFR &= ~BIT(7);
				cpu.setIPL(cpu.getIPL()&~1);
				//std::println("IRQ cleared, via_vIFR=${:X}, via_vIER=${:X}", via_vIFR, via_vIER);
			}
			return;
		case 10:
			via_vSR = v;
			if( v == 0x16 )
			{
				std::println("model number request");
				keys.push_front(3);
				kb_shift = 8000;
				kb_cmd = 0x16;
				return;
			}
			if( v == 0x36 )
			{
				std::println("keyboard ping");
				keys.push_front(0x7D);
				kb_shift = 8000;
				kb_cmd = 0x36;
				return;
			}
			if( v == 0x10 || v == 0x14 )
			{
				std::println("key read");
				keys.push_front(last_key);
				last_key = 0x7B;
				kb_shift = 8000;
				kb_cmd = v;
				return;
			}
			std::println("VIA vSR = ${:X}", v);
			return;
		case 11:
			std::println("VIA vACR = ${:X}", v);
			via_vACR = v;
			return;
		case 12:
			std::println("VIA vPCR = ${:X}", v);
			via_vPCR = v;
			return;
		case 13:
			//std::println("VIA vIFR = ${:X}", v);
			via_vIFR &= ~v;
			if( (via_vIFR & via_vIER & 0x7f) )
			{
				via_vIFR |= BIT(7);
				cpu.setIPL(cpu.getIPL()|1);
				//std::println("IRQ from vIFR write");
			} else {
				via_vIFR &= ~BIT(7);
				cpu.setIPL(cpu.getIPL()&~1);
				//std::println("IRQ cleared, via_vIFR written with ${:X}", v);
			}
			return;
		case 14:
			//std::println("VIA vIER = ${:X}", v);
			if( v & 0x80 )
			{
				via_vIER |= v&0x7f;
			} else {
				via_vIER &= ~(v&0x7f);
			}
			if( (via_vIFR & via_vIER & 0x7f) )
			{
				via_vIFR |= BIT(7);
				cpu.setIPL(cpu.getIPL()|1);
			} else {
				via_vIFR &= ~BIT(7);
				cpu.setIPL(cpu.getIPL()&~1);
			}
			return;
		case 15:
			//std::println("VIA BufA = ${:X}", v);
			//via_vIFR &= ~BIT(1);
			//if( !(via_vIFR & via_vIER & 0x7f) )
			//{
			//	via_vIFR &= ~BIT(7);
			//	cpu.setIPL(cpu.getIPL()&~1);
			//}
			via_vIFR &= ~BIT(0);
			if( !(via_vIFR & via_vIER & 0x7F) )
			{
				via_vIFR &= ~BIT(7);
				cpu.setIPL(cpu.getIPL()&~1);
			}
			via_vBufA = v;
			return;
		default: break;
		}
		std::println("Unimpl VIA wr{} ${:X} {} = ${:X}", sz, a, (a-0xEFE1FE)/512, v);
		exit(1);
	}
	
	if( a == 0xBFFFF9 )
	{
		std::println("SCC ctrlB wr = ${:X}", v);
		return;
	}
	if( a == 0xBFFFF9+2 )
	{
		std::println("SCC ctrlA wr = ${:X}", v);
		return;
	}
	if( a >= 0x580000 && a < 0x600000 )
	{
		std::println("scsi wr{} ${:X} = ${:X}", sz, a, v);
		return;
	}

	
	std::println("Unimpl wr{} ${:X} = ${:X}", sz, a, v);
	exit(1);
	//Running = false;
}

u32 macplus::read(u32 a, int sz)
{
	a &= 0xffFFff;
	if( in_reset || (a >= 0x400000 && a < 0x500000) ) return sized_read_be(bios, a&0x1ffff, sz);
	if( a < 0x400000 ) return sized_read_be(mainRAM, a, sz);

	if( a >= 0xF80000 )
	{
		//std::println("Unimpl rd{} ${:X}", sz, a);
		return 0xffffFFFFu;
	}
	if( a >= 0xF00000 )
	{
		std::println("Unimpl rd{} ${:X}", sz, a);
		return 0xffffFFFFu;
	}
		
	if( a >= 0xD80000 && a < 0xE00000 )
	{
		switch( (a-0xDFE1FF)/512 )
		{
		case 0:
			//std::println("IWMr CA0 off");
			iwm_ca0 = 0;
			break;
		case 1:
			//std::println("IWMr CA0 on");
			iwm_ca0 = 1;
			break;
		case 2:
			//std::println("IWMr CA1 off");
			iwm_ca1 = 0;
			break;
		case 3:
			//std::println("IWMr CA1 on");
			iwm_ca1 = 1;
			break;
		case 4:
			//std::println("IWMr CA2 off");
			iwm_ca2 = 0;
			break;
		case 5:
			//std::println("IWMr CA2 on");
			iwm_ca2 = 1;
			break;
		case 6:
			//std::println("IMRr LSTRB off");
			iwm_strobe = 0;
			break;
		case 7:
			//std::println("IMRr LSTRB on");
			if( iwm_strobe == 0 )
			{
				write_iwm_cmd();
			}
			iwm_strobe = 1;
			break;
		case 8:
			//std::println("IWMr enable Off");
			iwm_enable = 0;
			break;
		case 9:
			//std::println("IWMr enable On");
			iwm_enable = 1;
			break;
		case 10:
			//std::println("IWMr internal select");
			iwm_internal = 1;
			break;
		case 11:
			//std::println("IWMr ext select");
			iwm_internal = 0;
			break;
		case 12:
			//std::println("IWMr q6 low");
			iwm_q6 = 0;
			break;
		case 13:
			//std::println("IWMr q6 high");
			iwm_q6 = 1;
			break;
		case 14:
			//std::println("IWMr q7 low");
			iwm_q7 = 0;
			break;
		case 15:
			//std::println("IWMr q7 high");
			iwm_q7 = 1;
			break;
		default: 	
			std::println(stderr, "Unimpl IWM rd{} ${:X} {}", sz, a, (a-0xDFE1FF)/512);
			exit(1);
		}
		if( iwm_q6==1 && iwm_q7==0 )
		{
			u8 val = (iwm_mode&0x1F)|(read_sense()?0x80:0)|(iwm_enable?0x20:0);
			//std::println(stderr, "iwm status read ${:X}", val);
			return val;
		} else if( iwm_q6==0 && iwm_q7==0 ) {
			if( !iwm_enable ) { return 0xff; }
			u8 val = std::exchange(floppyA_data, 0);
			//std::println(stderr, "iwm data read got ${:X}", val);
			return val;
		} else {
			std::println("IWM read with ENABLE={}, Q6={}, Q7={}", iwm_enable, iwm_q6, iwm_q7);
			//char c;
			//std::cin >> c;
		}
		//exit(1);
		return 0x80;
	}
	
	if( a >= 0xE80000 && a < 0xF00000 )
	{
		switch( (a-0xEFE1FE)/512 )
		{
		case 0:
			//std::println("VIA Read BufB");
			return mouse_down ? 0xf7 : 0xff;
		case 2:
			//std::println("VIA Read DirB");
			return 0x87;
		case 8:
			//std::println("VIAr Tmr2_lo");
			return via_tmr2;
		case 9:
			//std::println("VIAr Tmr2_hi");
			return via_tmr2>>8;			
		case 10:
			std::println("VIA read vSR");
			if( (via_vACR&0x1C)==0xC && !keys.empty() )
			{
				u8 v = keys.back();
				keys.pop_back();
				via_vIFR &= ~BIT(2);
				if( !(via_vIFR & via_vIER & 0x7F) )
				{
					via_vIFR &= ~BIT(7);
					cpu.setIPL(cpu.getIPL()&~1);
				}
				std::println("vSR read got ${:X}", v);
				via_vSR = v;
			}
			return via_vSR;
		case 11:
			std::println("VIA read vACR");
			return via_vACR;
		case 12:
			std::println("VIA read vPCR");
			return via_vPCR;
		case 13:
			//std::println("VIA read vIFR");
			return via_vIFR;
		case 14:
			//std::println("VIA read vIER");
			return via_vIER|0x80;
		case 15:
			//std::println("VIA read BufA");
			via_vIFR &= ~BIT(0);
			if( !(via_vIFR & via_vIER & 0x7F) )
			{
				via_vIFR &= ~BIT(7);
				cpu.setIPL(cpu.getIPL()&~1);
			}
			return via_vBufA|0x80;
		default: break;
		}
		std::println("Unimpl VIA rd{} ${:X} {}", sz, a, (a-0xEFE1FE)/512);
		exit(1);
		//return 0;
	}
	
	if( a >= 0x580000 && a < 0x600000 )
	{
		std::println("scsi rd{} ${:X}", sz, a);
		return 0xffFF;
	}
	
	if( a == 0x9FFFF7 )
	{
		std::println("9FFFF7 ??");
		return 0;
	}
	if( a == 0x9FFFF8 )
	{
		std::println("SCC ctrlB rd");
		return 0;
	}
	if( a == 0x9FFFF8+2 )
	{
		std::println("SCC ctrlA rd");
		return 0;
	}	
	if( a == 0x9FFFF8+6 )
	{
		std::println("SCC dataA rd");
		return 0;
	}	
	
	std::println("Unimpl read{} ${:X}", sz, a);

	exit(1);
	//return 0;
}


void macplus::reset()
{
	//memset(&cpu, 0, sizeof(cpu));
	cpu.read_code16 = cpu.mem_read16 = [&](u32 a)->u16 { return read(a, 16); };
	cpu.mem_read8 = [&](u32 a) -> u8 
	{ 
		return read(a, 8);
	};
	cpu.mem_read32= [&](u32 a) -> u32
		{ return (read(a, 16)<<16)|read(a+2,16); };
	cpu.intack = []{};
	cpu.mem_write8= [&](u32 a, u8 v) { write(a, v, 8); };
	cpu.mem_write16= [&](u32 a, u16 v) { write(a, v, 16); };
	cpu.mem_write32= [&](u32 a, u32 v) 
		{ 
			write(a, v>>16, 16);
			write(a+2, v&0xffff, 16);
		};

	cpu.sr.raw = 0x2700;
	cpu.pending_irq = 0;
	cpu.r[15] = cpu.r[16] = read(0x400000, 32);
	cpu.pc = read(0x400004, 32);
	cpu.intack = []{};
	
	stamp = last_target = 0;
	onesec = via_tmr2irq = 0;
	tachdiv = floppy_div = 0;
}

bool macplus::loadROM(const std::string fname)
{
	FILE* fp = fopen("./bios/macplus.rom", "rb");
	if(!fp)
	{
		printf("Unable to load macplus.rom\n");
		return false;
	}
	
	[[maybe_unused]] int unu = fread(bios, 1, 512*1024, fp);
	fclose(fp);
	
	floppyA = MOOF::load(fname.c_str());
	if( !floppyA ) return false;
	return true;
}

void macplus::write_iwm_cmd()
{
	u8 REG = (iwm_ca1<<2)|(iwm_ca0<<1)|((via_vBufA&BIT(5))?1:0);
	//std::println(stderr, "IWM Strobe Reg ${:X}", REG);
	switch( REG )
	{
	case 0: // track dir
		floppyA_tdir = iwm_ca2;
		return;
	case 2: // track step
		if( floppyA_tdir )
		{
			if( floppyA_track ) { floppyA_track -= 1; curtrack_bitcount = 0; }
		} else {
			if( floppyA_track < 79 ) { floppyA_track += 1; curtrack_bitcount = 0; }
		}
		//std::println(stderr, "Floppy track at {}", floppyA_track);
		return;
	case 4: // motor (again?)
		floppyA_motor = iwm_ca2;
		return;
	case 6: // eject
		return;
	default:
		std::println(stderr, "unimpl iwm drive reg ${:X} = {}", REG, iwm_ca2);
		exit(1);
	}
}

u8 macplus::read_sense()
{
	u8 REG = (iwm_ca2<<3)|(iwm_ca1<<2)|(iwm_ca0<<1)|((via_vBufA&BIT(5))?1:0);
	//std::println(stderr, "IWM Look at ${:X}", REG);
	switch( REG )
	{
	case 1: // CISTN (disk inserted)
		return floppyA ? (iwm_internal ? 0 : 0x80) : 0x80;
	case 2: // head stepping
		return 0x80;
	case 3: // not write protected
		return 0x80;
	case 4: // MOTOR status
		return 0;//!floppyA_motor ? 0 : 0x80;
	case 5: // at track zero
		//std::println(stderr, "Look at TK0");
		return (floppyA_track==0) ? 0 : 0x80;
	
	case 6: // other TACH?
	case 7: // TACH?
		return tach;
	case 8: // RDDATA0
		return 0x80;
	case 9: // RDDATA1
		return 0x80;
	case 0xA: // not a thing?
		return 0;
	case 0xB: // not a thing?
		return 0;
	case 0xC: // sides
		return (floppyA->sides != 0 ? 0x80 : 0);
	
	case 0xE: // drive installed
		return (iwm_internal ? 0 : 0x80);
	case 0xF: // also drive installed?
		return (iwm_internal ? 0 : 0x80);
	default:
		std::println(stderr, "read sense needs reg ${:X}", REG);
		exit(1);
	}
	return 0;
}

void macplus::mouse_move(u32 x, u32 y, int relx, int rely)
{
	*(u16*)&mainRAM[0x82C] = *(u16*)&mainRAM[0x828] = __builtin_bswap16(u16(y));
	*(u16*)&mainRAM[0x82E] = *(u16*)&mainRAM[0x82A] = __builtin_bswap16(u16(x));
	mainRAM[0x8CE] = mainRAM[0x8CF] = 0xff;
}

void macplus::key_event(int sc, u8 orit)
{
	switch( sc )
	{
	case SDLK_BACKQUOTE: last_key = 0x65; break;
	case SDLK_1: last_key = 0x25; break;
	case SDLK_2: last_key = 0x27; break;
	case SDLK_3: last_key = 0x29; break;
	case SDLK_4: last_key = 0x2b; break;
	case SDLK_5: last_key = 0x2f; break;
	case SDLK_6: last_key = 0x2d; break;
	case SDLK_7: last_key = 0x35; break;
	case SDLK_8: last_key = 0x39; break;
	case SDLK_9: last_key = 0x33; break;
	case SDLK_0: last_key = 0x3b; break;
	case SDLK_MINUS: last_key = 0x37; break;
	case SDLK_EQUALS: last_key = 0x31; break;
	case SDLK_BACKSPACE: last_key = 0x67; break;			
	
	case SDLK_TAB: last_key = 0x61; break;
	case SDLK_q: last_key = 0x19; break;
	case SDLK_w: last_key = 0x1b; break;
	case SDLK_e: last_key = 0x1d; break;
	case SDLK_r: last_key = 0x1f; break;
	case SDLK_t: last_key = 0x23; break;
	case SDLK_y: last_key = 0x21; break;
	case SDLK_u: last_key = 0x41; break;
	case SDLK_i: last_key = 0x45; break;
	case SDLK_o: last_key = 0x3f; break;
	case SDLK_p: last_key = 0x47; break;
	case SDLK_LEFTBRACKET: last_key = 0x43; break;
	case SDLK_RIGHTBRACKET: last_key = 0x3d; break;
	case SDLK_BACKSLASH: last_key = 0x55; break;
	
	case SDLK_CAPSLOCK: last_key=0x73; break;
	case SDLK_a: last_key = 0x1; break;
	case SDLK_s: last_key = 0x3; break;
	case SDLK_d: last_key = 0x5; break;
	case SDLK_f: last_key = 0x7; break;
	case SDLK_g: last_key = 0xb; break;
	case SDLK_h: last_key = 0x9; break;
	case SDLK_j: last_key = 0x4d; break;
	case SDLK_k: last_key = 0x51; break;
	case SDLK_l: last_key = 0x4b; break;
	case SDLK_SEMICOLON: last_key = 0x53; break;
	case SDLK_QUOTE: last_key = 0x4f; break;
	case SDLK_RETURN: last_key = 0x49; break;

	case SDLK_LSHIFT: last_key = 0x71; break;
	case SDLK_z: last_key = 0xd; break;
	case SDLK_x: last_key = 0xf; break;
	case SDLK_c: last_key = 0x11; break;
	case SDLK_v: last_key = 0x13; break;
	case SDLK_b: last_key = 0x17; break;
	case SDLK_n: last_key = 0x5b; break;
	case SDLK_m: last_key = 0x5d; break;
	case SDLK_COMMA: last_key = 0x57; break;
	case SDLK_PERIOD: last_key = 0x5f; break;
	case SDLK_SLASH: last_key = 0x59; break;

	case SDLK_RCTRL:
	case SDLK_LCTRL: last_key = 0x75; break;
	case SDLK_RSHIFT: last_key = 0x6F; break;
	case SDLK_SPACE: last_key = 0x63; break;
	default: return;
	}		
	last_key |= orit;
}

