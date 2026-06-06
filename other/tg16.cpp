#include <algorithm>
#include <print>
#include <string>
#include "util.h"
#include "tg16.h"

const u32 reg5inc[] = { 1, 32, 64, 128 };

u8 tg16::io_read(u32 addr)
{
	addr &= 0x1fff;
	if( addr < 0x400 )
	{
		addr &= 3;
		if( addr == 0 ) { u8 res = vdc.stat.v; vdc.stat.v = 0; cpu.irq1_line = false; return res; }
		if( addr == 2 ) { return vdc.rdbuf; }	
		if( addr == 3 ) 
		{
			u8 val = vdc.rdbuf>>8;
			if( vdc.latch == 2 )
			{
				vdc.rdbuf = *(u16*)&VRAM[vdc.regs[1]*2];
				vdc.regs[1] += reg5inc[(vdc.regs[5]>>11)&3];
				vdc.regs[1] &= 0x7fff;
			}
			return val;
		}
		return 0;
	}
	if( addr == 0xc00 )
	{
		return tmr.value;
	}
	if( addr == 0x1403 ) { return (cpu.irq1_line ? BIT(1):0); }
	
	if( addr == 0x1000 ) return 0xb0|keys();
	if( addr >= 0x1A00 && addr < 0x1C00 ) return 0;
	std::println("TG16: io rd ${:X}", addr);
	return 0xff;
}

u8 tg16::keys()
{
	auto keys = SDL_GetKeyboardState(nullptr);
	u8 v = 0xf;
	if( keyport & 1 )
	{
		if( keys[SDL_SCANCODE_UP] ) v ^= 1;
		if( keys[SDL_SCANCODE_RIGHT] ) v ^= 2;
		if( keys[SDL_SCANCODE_DOWN] ) v ^= 4;
		if( keys[SDL_SCANCODE_LEFT] ) v ^= 8;
	} else {
		if( keys[SDL_SCANCODE_Z] ) v ^= 1;
		if( keys[SDL_SCANCODE_X] ) v ^= 2;
		if( keys[SDL_SCANCODE_A] ) v ^= 4;
		if( keys[SDL_SCANCODE_S] ) v ^= 8;
	}
	return v;
}

void tg16::io_write(u32 addr, u8 v)
{
	addr &= 0x1fff;
	if( addr < 0x400 )
	{
		addr &= 3;
		if( addr == 0 ) { vdc.latch = v&0x1F; return; }
		if( addr == 1 ) { return; }
		if( addr == 2 ) { vdc.regs[vdc.latch] &= 0xff00; vdc.regs[vdc.latch] |= v; return; }	
		if( addr == 3 )
		{
			vdc.regs[vdc.latch] &= 0xff;
			vdc.regs[vdc.latch] |= v<<8;
			if( vdc.latch == 0x12 )
			{
				std::println("dmalen hi written. dmalen =${:X}", vdc.regs[0x12]);
				exit(1);
				return;
			}
			if( vdc.latch == 2 )
			{
				vdc.regs[0] &= 0x7fff;
				*(u16*)&VRAM[vdc.regs[0]*2] = vdc.regs[2];
				vdc.regs[0] += reg5inc[(vdc.regs[5]>>11)&3];
				vdc.regs[0] &= 0x7fff;
				return;
			}
			if( vdc.latch == 1 )
			{
				vdc.regs[1] &= 0x7fff;
				vdc.rdbuf = *(u16*)&VRAM[vdc.regs[1]*2];
				vdc.regs[1] += reg5inc[(vdc.regs[5]>>11)&3];
				vdc.regs[1] &= 0x7fff;
				return;
			}
			if( vdc.latch == 0x13 )
			{
				vdc.do_sat_dma = true;
				return;
			}
			//if( vdc.latch==19 || vdc.latch == 0 || vdc.latch == 5 || vdc.latch == 6 || vdc.latch == 7 || vdc.latch == 8 ) return;
			//std::println("vdc reg ${:X} = ${:X}", vdc.latch, vdc.regs[vdc.latch]);
		}
		return;
	}
	if( addr < 0x800 )
	{
		addr &= 7;
		if( addr == 2 ) { vdc.paddr &= 0xff00; vdc.paddr |= v; return; }
		if( addr == 3 ) { vdc.paddr &= 0x00ff; vdc.paddr |= (v&1)<<8; return; }
		if( addr == 4 ) { vdc.palram[vdc.paddr] &= 0xff00; vdc.palram[vdc.paddr] |= v; return; }
		if( addr == 5 ) { vdc.palram[vdc.paddr&0x1ff] &= 0x00ff; vdc.palram[vdc.paddr&0x1ff] |= v<<8; vdc.paddr = (vdc.paddr+1)&0x1ff; return; }
		return;
	}
	if( addr < 0xC00 )
	{
		if( addr == 0x800 ) { pchan = v; if( pchan > 5 ) pchan = 5; return; }
		if( addr == 0x801 ) { /*todo: global volume */ return; }
		if( addr == 0x802 ) { psg[pchan].freq_lo = v; return; }
		if( addr == 0x803 ) { psg[pchan].freq_hi = v&15; return; }
		if( addr == 0x804 ) { psg[pchan].ctrl = v; if( (v&0xc0)==0x40 ) { psg[pchan].index=0; } return; }
		if( addr == 0x805 ) { psg[pchan].vol = v; return; }
		if( addr == 0x806 ) { psg[pchan].data = v; if( (v&0x40)==0 ) { psg[pchan].wave[psg[pchan].index++] = v; psg[pchan].index&=31; } return; }
		if( addr == 0x807 ) { psg[pchan].noise = v; return; }
		if( addr == 0x808 ) { psg[pchan].lfo_freq = v; return; }
		if( addr == 0x809 ) { psg[pchan].lfo_ctrl = v; return; }
		return;
	}
	if( addr < 0x1000 )
	{
		if( addr == 0xc00 )
		{
			tmr.reload = v&0x7f;
		}
		if( addr == 0xc01 )
		{
			tmr.ctrl = v;
			if( v & 1 )
			{
				tmr.div = 0;
				tmr.value = tmr.reload;
			}
		}
		return;
	}
	if( addr == 0x1402 )
	{
		cpu.irq_enable = v;
		return;
	}
	if( addr == 0x1403 )
	{
		cpu.timer_irq_line = false;
		return;
	}
	if( addr==0x1000 ) { keyport = v; return; }
	//std::println("TG16: io wr ${:X} = ${:X}", addr, v);
}

u8 tg16::read(u32 addr)
{
	u8 bank = addr>>13;
	if( bank < 0x80 )
	{
		if( ROM.size() > 300000 && ROM.size() < 400000 )
		{
			if( addr > ROM.size() )
			{
				return ROM[0x40000 + (addr&0x1ffff)];
			}
		}
		 return ROM[addr%ROM.size()];
	}
	if( bank == 0xF8 ) return wram[addr&0x1fff];
	if( bank == 0xFF ) return io_read(addr);
	std::println("TG16: Unimpl rd bank ${:X}, offset ${:X}", bank, addr&0x1fff);
	return 0xff;
}

void tg16::write(u32 addr, u8 v)
{
	u8 bank = addr>>13;
	if( bank < 0x80 ) { return; }
	if( bank == 0xF8 ) { wram[addr&0x1fff] = v; return; }
	if( bank == 0xFF ) { io_write(addr, v); return; }
	std::println("TG16: Unimpl wr ${:X} = ${:X}", addr, v);
}

void tg16::system_cycles(u64 cyc)
{
	for(u32 psg_chan = 0; psg_chan < 6; ++psg_chan)
	{
		auto& P = psg[psg_chan];
		if( (P.ctrl & 0x80) == 0 ) { P.left=P.right=0; continue; }
		if( (P.ctrl & 0xC0) == 0xC0 )
		{
			P.left = (((P.data&0x1F)/31.f)*2 - 1) * ((P.ctrl&31)/31.f) * (((P.vol>>4)&15)/15.f);
			P.right = (((P.data&0x1F)/31.f)*2 - 1)  * ((P.ctrl&31)/31.f) * ((P.vol&15)/15.f);
			continue;
		}
		u32 V = P.freq_lo;
		V |= (P.freq_hi&15)<<8;
		V <<= 1;  // freq is in ~3Mhz cycles, emulator keeps things in ~7Mhz cycles
		P.count += cyc;
		if( P.count >= V )
		{
			P.index = (P.index+1)&0x1F;
			P.count -= V;
		}
		P.left  = (((P.wave[P.index]&31)/31.f)*2 - 1) * ((P.ctrl&31)/31.f) * (((P.vol>>4)&15)/15.f);
		P.right = (((P.wave[P.index]&31)/31.f)*2 - 1) * ((P.ctrl&31)/31.f) * ((P.vol&15)/15.f);
	}
	
	audio_counter += cyc;
	if( audio_counter >= (7190000/44100) )
	{
		audio_counter -= (7190000/44100);
		float L=0, R=0;
		for(u32 i = 0; i < 6; ++i) { L+=psg[i].left; R+=psg[i].right; }
		audio_add(L, R);
	}
	
	if( tmr.ctrl & 1 )
	{
		tmr.div += cyc;
		if( tmr.div >= 1024 )
		{
			tmr.div -= 1024;
			tmr.value -= 1;
			if( tmr.value == 0xff )
			{
				cpu.timer_irq_line = true;
				tmr.value = tmr.reload;
			}
		}	
	}

}

const u32 batwidth[] = { 32, 64, 128, 128 };

void tg16::draw_line(u32 scanline, u32 bgline)
{
	if( !(vdc.regs[5] & 0xC0) )
	{
		memset(&fbuf[scanline*256], 0, 512);
		return;
	}

	memset(vdc.bg, 0, 256);
	if( vdc.regs[5] & 0x80 )
	{
		const u32 BW = batwidth[(vdc.regs[9]>>4)&3];
		const u32 BH = ((vdc.regs[9]&BIT(6))? 64:32);
		
		for(u32 x = 0; x < 256; ++x)
		{
			u32 X = (x+vdc.regs[7])&0x3ff;
			u32 tx = (X/8)&(BW-1);
			u32 Y = bgline;//((line+vdc.regs[8]));
			u32 ty = (Y/8)&(BH-1);
			u16 map_entry = *(u16*)&VRAM[((ty*BW) + tx)*2];
			u16 tile = (map_entry&0x7ff)*32;
			u16 w1 = *(u16*)&VRAM[(tile+(Y&7)*2)];
			u16 w2 = *(u16*)&VRAM[(tile+(Y&7)*2+16)];
			
			u8 bi = 7-(X&7);
			
			u8 b1 = (w1>>(bi))&1;
			u8 b2 = (w1>>(8+(bi)))&1;
			u8 b3 = (w2>>(bi))&1;
			u8 b4 = (w2>>(8+(bi)))&1;
			u8 pal = (b4<<3)|(b3<<2)|(b2<<1)|b1;
			
			vdc.bg[x] = (pal ? 1:0);
			if( pal ) { pal |= (map_entry>>8)&0xf0; }
			u16 col = vdc.palram[pal];
			u8 b = col&7;
			u8 r = (col>>3)&7;
			u8 g = (col>>6)&7;
			
			fbuf[scanline*256 + x] = (b<<12)|(g<<7)|(r<<2);
		}
	}
	
	const int spr_heights[] = { 16, 32, 64, 64 };
	
	if( vdc.regs[5] & 0x40 )
	{
		u8 sprnum[16];
		int sprs = 0;
		for(u32 i = 0; i < 64 && sprs<16; ++i)
		{
			int sheight = spr_heights[((vdc.sat[i*4+3]>>12)&3)];
			int Y = int(vdc.sat[i*4] & 0x3ff) - 64;
			if( int(scanline) - Y >= 0 && int(scanline) - Y < sheight )
			{
				sprnum[sprs++] = i;
			}		
		}
		
		u8 sbit[256] = {0};
		
		for(int S = 0; S < sprs; ++S)
		{
			u32 i = sprnum[S];
			int sheight = spr_heights[((vdc.sat[i*4+3]>>12)&3)];
			int swidth = ((vdc.sat[i*4+3]&BIT(8)) ? 32:16);
			int Y = int(scanline) - ((vdc.sat[i*4] & 0x3ff) - 64);
			int pX = (vdc.sat[i*4+1] & 0x3ff) - 32;
			
			if( vdc.sat[i*4+3]&BIT(15) ) { Y = (sheight-1)-Y; }
			
			int base_tile = ((vdc.sat[i*4+2]>>1)&0x3ff);
			if( vdc.sat[i*4+3] & BIT(8) ) base_tile &= ~1;
			if( ((vdc.sat[i*4+3]>>12)&3) == 1 ) base_tile &= ~2;
			else if( ((vdc.sat[i*4+3]>>12)&3) >= 2 ) base_tile &= ~6;
						
			for(int tx=0; tx<swidth; ++pX, ++tx)
			{
				if( pX < 0 || pX > 255 || sbit[pX] ) continue;
				
				int x = (vdc.sat[i*4+3]&BIT(11)) ? ((swidth - 1) - tx) : tx;
				int col_offset = (x / 16);
				int row_offset = (Y / 16)*2; //long narrow sprites still use tiles like there was a second column
				int tile_start = (base_tile + row_offset + col_offset) * 128;
					    
				u16 w1 = *(u16*)&VRAM[tile_start+(Y&15)*2+00];
				u16 w2 = *(u16*)&VRAM[tile_start+(Y&15)*2+32];
				u16 w3 = *(u16*)&VRAM[tile_start+(Y&15)*2+64];
				u16 w4 = *(u16*)&VRAM[tile_start+(Y&15)*2+96];
				
				u8 bi = (15-(x&15));
				u8 b0 = (w1>>bi)&1;
				u8 b1 = (w2>>bi)&1;
				u8 b2 = (w3>>bi)&1;
				u8 b3 = (w4>>bi)&1;
				u16 pal = (b3<<3)|(b2<<2)|(b1<<1)|b0;
				if( pal ) 
				{
					sbit[pX] = 1;
					//if( vdc.bg[pX] && !(vdc.sat[i*4+3]&0x80) ) continue;
					pal |= (vdc.sat[i*4+3]&0xf)<<4; 
					pal += 0x100; 
				} else {
					continue;
				}
				u16 col = vdc.palram[pal];
				u8 b = col&7;
				u8 r = (col>>3)&7;
				u8 g = (col>>6)&7;			
				fbuf[scanline*256 + pX] = (b<<12)|(g<<7)|(r<<2);
			}
		}
	}
}

void tg16::vdc_sat_dma()
{
	u32 src = vdc.regs[0x13]&0x7fff;
	for(u32 i = 0; i < 64*4; ++i, ++src)
	{
		vdc.sat[i] = *(u16*)&VRAM[src<<1];
	}
}

void tg16::run_frame()
{
	for(u32 line = 0; line < 263; ++line)
	{
		if( line == 242 )
		{
			if( (vdc.regs[0xF] & BIT(4)) || vdc.do_sat_dma )
			{
				vdc.do_sat_dma = false;
				vdc_sat_dma();
			}
			if( vdc.regs[5] & 8 )
			{
				vdc.stat.b.vd = 1;
				cpu.irq1_line = true;
			}
		}
		if( line+0x40 == vdc.regs[6] && (vdc.regs[5] & 4) )
		{
			vdc.stat.b.rr = 1;
			cpu.irq1_line = true;
		}		
				
		u64 target = last_target + 454;
		while( stamp < target )
		{
			u64 cc = cpu.step() * (cpu.high_speed?1:4);
			stamp += cc;
			system_cycles(cc);
		}
		last_target = target;
		
		//if( line >= 21 )
		//{
		//	if( line < 256 ) { draw_line(line-21, vdc.regs[8]); vdc.regs[8]+=1; }
			//else memset(&fbuf[(line-40)*256], 0, 512);
		//}
		if( line<242 ) 
		{ 
			draw_line(line-0, vdc.regs[8]-0);  vdc.regs[8]+=1; 
		}
		
	}
}

bool tg16::loadROM(std::string fname)
{
	if( !freadall(ROM, fopen(fname.c_str(), "rb")) ) { return false; }
	cpu.bus_read = [&](u32 addr)->u8 { return read(addr); };
	cpu.bus_write= [&](u32 addr, u8 v) { write(addr,v); };
	
	
	if( std::all_of(ROM.begin()+0x10, ROM.begin()+0x200, [&](u8 v){ return v==0; }) ) ROM.erase(std::begin(ROM), std::begin(ROM)+0x200);
	cpu.reset();
	return true;
}

void tg16::reset()
{
	stamp = last_target = 0;
	for(u32 i = 0; i < 0x20; ++i) vdc.regs[i]=0;
	
	tmr.div = tmr.ctrl = tmr.value = tmr.reload = 0;
	cpu.reset();
}



