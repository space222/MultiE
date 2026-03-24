#include <print>
#include <SDL.h>
#include "util.h"
#include "snes.h"

void snes::io_write(u8 bank, u32 a, u8 v)
{
	if( a < 0x2000 ) { do_master_cycles(2); ram[a] = v; return; }
	//std::println("${:X}:${:X}: io wr ${:X}:${:X} = ${:X}", cpu.pb>>16, cpu.pc, bank, a, v);
	switch( a )
	{
	case 0x2100: ppu.inidisp = v&0x8F; return;
	case 0x2101: ppu.objsel = v; return;
	case 0x2102: ppu.oamaddl = v; ppu.oamadd=((ppu.oamaddh&1)<<8)|v; ppu.internal_oamadd=(ppu.oamadd<<1); return;
	case 0x2103: ppu.oamaddh = v&0x81; ppu.oamadd&=0xff; ppu.oamadd|=(v&1)<<8; ppu.internal_oamadd=(ppu.oamadd<<1); return;
	
	case 0x2104:{
			if( !(ppu.internal_oamadd & 1) ) ppu.oam_latch = v;
			if( ppu.internal_oamadd < 0x200 && (ppu.internal_oamadd&1) )
			{
				ppu.oam[ppu.internal_oamadd-1] = ppu.oam_latch;
				ppu.oam[ppu.internal_oamadd] = v;
			}
			if( ppu.internal_oamadd >= 0x200 ) ppu.oamhi[ppu.internal_oamadd&31] = v;
			ppu.internal_oamadd += 1;
			ppu.internal_oamadd &= 0x3ff;	
		}return;
	
	case 0x2105: ppu.bgmode = v; return;
	case 0x2106: ppu.mosaic = v; return;
	case 0x2107: ppu.bg1sc = v; return;
	case 0x2108: ppu.bg2sc = v; return;
	case 0x2109: ppu.bg3sc = v; return;
	case 0x210A: ppu.bg4sc = v; return;
	case 0x210B: ppu.bg12nba = v; return;
	case 0x210C: ppu.bg34nba = v; return;
	case 0x2115: ppu.vmain = v&0x8F; return;
	
	case 0x210D: ppu.bg1hofs = (v<<8)|(ppu.bgofs_latch&~7)|((ppu.bg1hofs>>8)&7); //snesdev wiki pseudocode is wrong
										     //this is based on fullsnes, but dunno if I understood it correctly
			//std::println("BG1H = ${:X}", ppu.bg1hofs);
		     ppu.bgofs_latch = v;
		     return;
	case 0x210F: ppu.bg2hofs = (v<<8)|(ppu.bgofs_latch&~7)|((ppu.bg2hofs>>8)&7);
		     //std::println("BG2H = ${:X}", ppu.bg2hofs);
		    ppu.bgofs_latch = v;
		     return;
	case 0x2111: ppu.bg3hofs = (v<<8)|(ppu.bgofs_latch&~7)|((ppu.bg3hofs>>8)&7);
		    //std::println("BG3H = ${:X}", ppu.bg3hofs);
		     ppu.bgofs_latch = v;
		     return;
	case 0x2113: ppu.bg4hofs = (v<<8)|(ppu.bgofs_latch&~7)|((ppu.bg4hofs>>8)&7);
		     ppu.bgofs_latch = v;
		     return;
	case 0x210E: ppu.bg1vofs = (v<<8)|ppu.bgofs_latch;
		     ppu.bgofs_latch = v;
		     return;
	case 0x2110: ppu.bg2vofs = (v<<8)|ppu.bgofs_latch;
		     ppu.bgofs_latch = v;
		     return;
	case 0x2112: ppu.bg3vofs = (v<<8)|ppu.bgofs_latch;
		     ppu.bgofs_latch = v;
		     return;
	case 0x2114: ppu.bg4vofs = (v<<8)|ppu.bgofs_latch;
		     ppu.bgofs_latch = v;
		     return;
		     
	case 0x211B: ppu.m7a = (v<<8)|ppu.m7latch;
		     ppu.m7latch = v;
		     return;
	case 0x211C: ppu.m7b = (v<<8)|ppu.m7latch;
		     ppu.m7latch = v;
		     io.multres = s32(s16(ppu.m7a)) * s8(v);
		     return;
	case 0x211D: ppu.m7c = (v<<8)|ppu.m7latch;
		     ppu.m7latch = v;
		     return;
	case 0x211E: ppu.m7d = (v<<8)|ppu.m7latch;
		     ppu.m7latch = v;
		     return;
	case 0x211F: ppu.m7x = (v<<8)|ppu.m7latch;
		     ppu.m7latch = v;
		     return;
	case 0x2120: ppu.m7y = (v<<8)|ppu.m7latch;
		     ppu.m7latch = v;
		     return;
		     
	case 0x2116: ppu.vmaddl = v; return;
	case 0x2117: ppu.vmaddh = v; return;
	
	case 0x2118: { 
			if( ppu.vmain&12 ) { std::println("VMAIN = ${:X}", ppu.vmain); exit(1); }
			u32 incr = 1;
			if( (ppu.vmain&3)== 1 ) incr = 32; else if( (ppu.vmain&3) ) incr = 128; 
			u32 vaddr = ((ppu.vmaddh<<8)|ppu.vmaddl)&0x7fff;
		       ppu.vram[(vaddr*2)&0xffff] = v;
		       if( !(ppu.vmain & BIT(7)) ) { vaddr+=incr; ppu.vmaddl=vaddr; ppu.vmaddh=vaddr>>8; }
		      }return;
	case 0x2119: { 
			if( ppu.vmain&12 ) { std::println("VMAIN = ${:X}", ppu.vmain); exit(1); }
			u32 incr = 1;
			if( (ppu.vmain&3)== 1 ) incr = 32; else if( (ppu.vmain&3) ) incr = 128; 
			u32 vaddr = ((ppu.vmaddh<<8)|ppu.vmaddl)&0x7fff;
		       ppu.vram[(vaddr*2 + 1)&0xffff] = v;
		       if( (ppu.vmain & BIT(7)) ) { vaddr+=incr; ppu.vmaddl=vaddr; ppu.vmaddh=vaddr>>8; }
		      }return;
		      
	case 0x2121: ppu.cgadd = v; ppu.cgram_byte=0; return;
	case 0x2122: if( ppu.cgram_byte == 0 )
		     {
		     	   ppu.cgram_latch = v;
		     } else {
		     	   ppu.cgram[ppu.cgadd] = (v<<8)|ppu.cgram_latch;
		     	   ppu.cgadd += 1;
		     }
		     ppu.cgram_byte ^= 1;
		     return;

	case 0x2123: ppu.w12sel = v; return;
	case 0x2124: ppu.w34sel = v; return;
	case 0x2125: ppu.wobjsel = v; return;
	case 0x2126: ppu.wh0 = v; return;
	case 0x2127: ppu.wh1 = v; return;
	case 0x2128: ppu.wh2 = v; return;
	case 0x2129: ppu.wh3 = v; return;
	case 0x212A: ppu.wbglog = v; return;
	case 0x212B: ppu.wobjlog = v&15; return;
	case 0x212C: ppu.tm = v&0x1f; /*std::println("TM = ${:X}", v);*/ return;
	case 0x212D: ppu.ts = v&0x1f; /*std::println("TS = ${:X}", v);*/ return;
	case 0x212E: ppu.tmw = v&0x1f; return;
	case 0x212F: ppu.tsw = v&0x1f; return;
	case 0x2130: ppu.cgwsel = v&0xf3; return;
	case 0x2131: ppu.cgadsub = v; return;
	case 0x2132: 
			if( v & 0x80 ) { ppu.fixed_color &= ~(31<<10); ppu.fixed_color |= (v&31)<<10; }
			if( v & 0x40 ) { ppu.fixed_color &= ~(31<<5); ppu.fixed_color |= (v&31)<<5; }
			if( v & 0x20 ) { ppu.fixed_color &= ~31; ppu.fixed_color |= v&31; }
			return;
	case 0x2133: ppu.setini = v&0xcf; return;
	
	case 0x2140: apu.to_spc[0] = v; return;
	case 0x2141: apu.to_spc[1] = v; return;
	case 0x2142: apu.to_spc[2] = v; return;
	case 0x2143: apu.to_spc[3] = v; return;
	
	case 0x2180:{
		     u32 addr = (io.wmaddh<<16)|(io.wmaddm<<8)|io.wmaddl;
		     ram[addr++] = v;
		     io.wmaddh = (addr>>16)&1;
		     io.wmaddm = addr>>8;
		     io.wmaddl = addr;
		     }return;
			
	case 0x2181: io.wmaddl = v; return;
	case 0x2182: io.wmaddm = v; return;
	case 0x2183: io.wmaddh = v&1; return;
	
	case 0x4016: /*todo joypad*/ return;
	
	case 0x4200: io.nmitimen = v&0xb1; if( !(v&BIT(5)) ) { cpu.do_irqs=cpu.irq_line=false; io.timeup&=0x7f; } return; /*todo: raise nmi if currently in vblank*/
	case 0x4201: io.wrio = v; return;
	case 0x4202: io.wrmpya = v; return;
	case 0x4203: io.wrmpyb = v; io.remain = io.wrmpya; io.remain *= io.wrmpyb; return;
	case 0x4204: io.wrdivl = v; return;
	case 0x4205: io.wrdivh = v; return;
	case 0x4206:{
		u16 dividend = io.wrdivh<<8; dividend |= io.wrdivl;
		if( v == 0 )
		{
			io.quot = 0xffff; io.remain = dividend;
		} else {
			io.quot = dividend/v;
			io.remain = dividend%v;
		}
		}return;
	case 0x4207: io.htimel = v; return;
	case 0x4208: io.htimeh = v&1; return;
	case 0x4209: io.vtimel = v; return;
	case 0x420A: io.vtimeh = v&1; return;
	case 0x420B: io.mdmaen = v;
		     for(u32 i = 0; i < 8; ++i) { if( io.mdmaen & BIT(i) ) { run_dma(i); io.mdmaen&=~BIT(i); } }
		     return;
	case 0x420C: io.hdmaen = v; return;	
	case 0x420D: io.memsel = v&1; return;
	
	case 0x211A: ppu.m7sel = v&0xc3; return;
	default:
		if( a >= 0x4300 && a < 0x4380 ) { ppu.dmaregs[a&0x7f] = v; return; }
		std::println("${:X}:${:X}: io wr ${:X}:${:X} = ${:X}", cpu.pb>>16, cpu.pc, bank, a, v);
		//exit(1);
		return;
	}
}

u8 snes::io_read(u8 bank, u32 a)
{
	if( a < 0x2000 ) { do_master_cycles(2); return ram[a]; }
	//std::println("${:X}:${:X}: io rd ${:X}:${:X}", cpu.pb>>16, cpu.pc, bank, a);
	switch( a )
	{
	case 0x4016: return 1; // joypad1 rd
	case 0x4017: return 1; // joypad2 rd
	
	case 0x4212: return (ppu.scanline>239 ? 0x80:0)|(ppu.master_cycles<340?0x40:0);
	
	case 0x4214: return io.quot;
	case 0x4215: return io.quot>>8;
	case 0x4216: return io.remain;
	case 0x4217: return io.remain>>8;
	
	case 0x4218: return keys(); //auto joypad 1l
	case 0x4219: return keys()>>8; //auto joypad 1h
	
	case 0x421A:
	case 0x421B:
	case 0x421C:
	case 0x421D: 
	case 0x421E:
	case 0x421F: return 0x00;
	
	case 0x2134: return io.multres;
	case 0x2135: return io.multres>>8;
	case 0x2136: return io.multres>>16;
	
	case 0x2137: ppu.vcounter = ppu.scanline; ppu.stat78|=BIT(6); return 0; //todo: HV counter latch
	case 0x2138:{ 
		u8 r = 0;
		if( ppu.internal_oamadd >= 0x200 )
		{
			r = ppu.oamhi[ppu.internal_oamadd&0x1f];
		} else {
			r = ppu.oam[ppu.internal_oamadd];
		}
		ppu.internal_oamadd += 1;
		ppu.internal_oamadd &= 0x3ff;
		return r;
		}
	
	case 0x213C: return 0;
	case 0x213D:{ u8 r = ppu.vcounter>>(ppu.vcounter_ff * 8); ppu.vcounter_ff^=1; return r; } //todo: counters
		
	case 0x213F: ppu.vcounter_ff=0; return std::exchange(ppu.stat78, ppu.stat78&~BIT(6)); //todo: counter latch values
	
	case 0x2140: return apu.to_cpu[0];
	case 0x2141: return apu.to_cpu[1];
	case 0x2142: return apu.to_cpu[2];
	case 0x2143: return apu.to_cpu[3];
	case 0x4210: { u8 v = ppu.rdnmi; ppu.rdnmi &= 0x7f; return 2 | v; }
	case 0x4211: cpu.do_irqs=cpu.irq_line=false; return std::exchange(io.timeup, io.timeup&0x7f);
	
	default:
		if( a >= 0x4300 && a < 0x4380 ) return ppu.dmaregs[a&0x7f];
		break;
	}
	std::println("${:X}:${:X}: io rd ${:X}:${:X}", cpu.pb>>16, cpu.pc, bank, a);
	//exit(1);
	return 0;
}

u16 snes::keys()
{
	auto k = SDL_GetKeyboardState(nullptr);
	u16 val = 0;
	if( k[SDL_SCANCODE_W] ) val ^= BIT(4);
	if( k[SDL_SCANCODE_Q] ) val ^= BIT(5);
	if( k[SDL_SCANCODE_X] ) val ^= BIT(6);
	if( k[SDL_SCANCODE_Z] ) val ^= BIT(15);
	if( k[SDL_SCANCODE_RETURN] ) val ^= BIT(12);
	
	if( k[SDL_SCANCODE_UP] ) val ^= BIT(11);
	if( k[SDL_SCANCODE_DOWN] ) val ^= BIT(10);
	if( k[SDL_SCANCODE_LEFT] ) val ^= BIT(9);
	if( k[SDL_SCANCODE_RIGHT] ) val ^= BIT(8);
	
	return val;
}


void snes::write(u32 a, u8 v)
{
	u8 bank = a>>16;
	a &= 0xffff;
	// WRAM region is same in all mapping modes
	if( bank == 0x7E || bank == 0x7F ) { do_master_cycles(2); ram[((bank&1)<<16)|a] = v; return; }
	// as is the io region, although io/rw might fall through to cart expansion io
	if( (bank&0x7F) < 0x40 && a < 0x8000 ) { io_write(bank, a, v); return; }

	if( bank >= 0x70 && bank < 0x80 && a < 0x8000 )
	{
		SRAM[((bank*32_KB) + a) % cart.ram_size] = v;
		save_written = true;
		return;
	}

	std::println("${:X}: snes wr ${:X}:${:X} = ${:X}", cpu.pc, bank, a, v);
}

u8 snes::read(u32 a)
{
	u8 bank = a>>16;
	a &= 0xffff;
	// WRAM region is same in all mapping modes
	if( bank == 0x7E || bank == 0x7F ) { do_master_cycles(2); return ram[((bank&1)<<16)|a]; }
	// as is the io region, although io/rw might fall through to cart expansion io
	if( (bank&0x7F) < 0x40 && a < 0x8000 ) { return io_read(bank, a); }
	
	if( a >= 0x8000 )
	{
		//std::println("snes rd ${:X}:${:X}", bank, a);
		return ROM[(((bank&0x7f)*0x8000) + (a&0x7fff))  % ROM.size()];
	}
	if( bank >= 0x70 && bank < 0x80 && a < 0x8000 )
	{
		return SRAM[((bank*32_KB) + a) % cart.ram_size];
	}
	
	
	std::println("snes rd ${:X}:${:X}", bank, a);
	return 0;
}

void snes::do_master_cycles(s64 master_cycles)
{
	ppu_mc(master_cycles);
	cpu_stamp += master_cycles * 24576000;
	spc_stamp += master_cycles;
	if( spc_stamp >= 487 ) { spc_stamp-=487; audio_add(apu.Left, apu.Right); }

	while( cpu_stamp > 0 )
	{
		cpu_stamp -= 21477272;
		spc_div += 1;
		if( spc_div >= 24 )
		{
			spc_div = 0;
			snd_clock();
		}
	}
}

void snes::run_frame()
{
	ppu.frame_complete = false;
	while( !ppu.frame_complete )
	{
		s64 master_cycles = cpu_run();
		do_master_cycles(master_cycles);
	}
}

void snes::run_dma(u32 cn)
{
	const u8 rb = cn<<4;
	u16 src = ppu.dmaregs[rb|2]|(ppu.dmaregs[rb|3]<<8);
	u8 bank = ppu.dmaregs[rb|4];
	u32 len = ppu.dmaregs[rb|5]|(ppu.dmaregs[rb|6]<<8);
	if( len == 0 ) len = 0x10000;
	const u16 baddr = ppu.dmaregs[rb|1];
	const u8 param = ppu.dmaregs[rb|0];
	
	//std::println("DMA src=${:X}, dst=${:X}, len=${:X}", src, baddr, len);
	for(u32 i = 0; i < len; ++i)
	{
		u16 mod = 0;
		switch( param & 7 )
		{
		case 5:
		case 1: mod = i&1; break;
		case 7:
		case 3: mod = (i>>1)&1; break;
		case 4: mod = i&3; break;
		default: break; //0,2,6 unmodded
		}
		u16 B_Bus = u8(baddr+mod) | 0x2100;
		if( param & 0x80 )
		{
			//std::println("dma to mem rd ${:X}, wr ${:X}", baddr+mod, (bank<<16)|src);
			write((bank<<16)|src, io_read(0, B_Bus));
		} else {
			//std::println("dma to dev rd ${:X}, wr ${:X}", (bank<<16)|src, baddr+mod);
			io_write(0, B_Bus, read((bank<<16)|src));
		}
		
		if( ((param>>3) & 3) == 0 )
		{
			src += 1;
		} else if( ((param>>3) & 3) == 2 ) {
			src -= 1;
		}
	}
	
	ppu.dmaregs[rb|5] = ppu.dmaregs[rb|6] = 0;
	ppu.dmaregs[rb|3] = src>>8;
	ppu.dmaregs[rb|2] = src;
}

bool snes::loadROM(std::string fname)
{
	if( !freadall(ROM, fopen(fname.c_str(), "rb")) )
	{
		std::println("Unable to open snes rom '{}'", fname);
		return false;
	}
	cpu.bus_read = [&](u32 a)->u8 { return read(a); };
	cpu.bus_write = [&](u32 a, u8 v) { write(a,v); };
	spc.reader = [&](spc700&, u16 a) { return spc_read(a); };
	spc.writer = [&](spc700&, u16 a, u8 v) { spc_write(a,v); };

	//find rom header to get board info: rom/ram sizes, coprocessor, mapping type (loRom, hiRom, exhiRom)
	u32 hd = 0x7fc0;
	if( (*(u16*)&ROM[0x7fde]^0xffff) == *(u16*)&ROM[0x7fdc] || (ROM[0x7fdc]=='C'&&ROM[0x7fdd]=='C'&&ROM[0x7fde]=='C'&&ROM[0x7fdf]=='S' ) )
	{
		std::println("SNES: Using LoROM Mapping");
		cart.mapping_type = MAPPING_LOROM;
		if( (ROM[hd+0x15]&15) != 0 )
		{
			std::println("warning: detected LoROM header, but {} != {}", ROM[hd+0x15]&15, 0);
		}
	} else if( (*(u16*)&ROM[0xffde]^0xffff) == *(u16*)&ROM[0xffdc] ) {
		std::println("SNES: Using HiROM Mapping");
		cart.mapping_type = MAPPING_HIROM;
		hd = 0xffc0;
		if( (ROM[hd+0x15]&15) != 1 )
		{
			std::println("warning: detected HiROM header, but {} != {}", ROM[hd+0x15]&15, 1);
		}
	} else {
		std::println("SNES: Failing into ExHiROM");
		cart.mapping_type = MAPPING_EXHIROM;
		hd = 0x40ffc0;
		if( (ROM[hd+0x15]&15) != 5 )
		{
			std::println("warning: detected ExHiROM header, but {} != {}", ROM[hd+0x15]&15, 5);
		}
	}
	
	cart.chipset = ROM[hd+0x16];
	cart.fastROM = (ROM[hd+0x15]>>4)&1;
	cart.rom_size = (ROM[hd+0x17]*32_KB);
	cart.ram_size = (1<<ROM[hd+0x18])*1024;
	SRAM.resize(cart.ram_size);
	
	savefile = fname;
	auto pos = fname.rfind('.');
	if( pos != std::string::npos ) savefile = savefile.substr(0, pos);
	savefile += ".ram";
	std::println("Save file: {}", savefile);
	
	freadall(SRAM, fopen(savefile.c_str(), "rb"));
	save_written = false;
	
	return true;
}


