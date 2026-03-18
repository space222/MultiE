#include <print>
#include "util.h"
#include "snes.h"

void snes::io_write(u8 bank, u32 a, u8 v)
{
	if( a < 0x2000 ) { do_master_cycles(2); ram[a] = v; return; }
	//std::println("io wr ${:X}:${:X} = ${:X}", bank, a, v);
	switch( a )
	{
	case 0x2100: ppu.inidisp = v&0x8F; return;
	case 0x2101: ppu.objsel = v; return;
	case 0x2102: ppu.oamaddl = v; ppu.oamadd=((ppu.oamaddh&1)<<8)|v; ppu.internal_oamadd=(ppu.oamadd<<1); return;
	case 0x2103: ppu.oamaddh = v&0x81; ppu.oamadd&=0xff; ppu.oamadd|=(v&1)<<8; ppu.internal_oamadd=(ppu.oamadd<<1); return;
	
	case 0x2104: /*todo: oam write*/ return;
	
	case 0x2105: ppu.bgmode = v; return;
	case 0x2106: ppu.mosaic = v; return;
	case 0x2107: ppu.bg1sc = v; return;
	case 0x2108: ppu.bg2sc = v; return;
	case 0x2109: ppu.bg3sc = v; return;
	case 0x210A: ppu.bg4sc = v; return;
	case 0x210B: ppu.bg12nba = v; return;
	case 0x210C: ppu.bg34nba = v; return;
	case 0x2115: ppu.vmain = v&0x8F; if( v&15 ) { std::println("VMAIN = ${:X}", v&0x8f); } return;
	
	case 0x210D: ppu.bg1hofs = (v<<8)|(ppu.bgofs_latch&~7)|(ppu.bghofs_latch&7);
		     ppu.bgofs_latch = ppu.bghofs_latch = v;
		     return;
	case 0x210F: ppu.bg2hofs = (v<<8)|(ppu.bgofs_latch&~7)|(ppu.bghofs_latch&7);
		     ppu.bgofs_latch = ppu.bghofs_latch = v;
		     return;
	case 0x2111: ppu.bg3hofs = (v<<8)|(ppu.bgofs_latch&~7)|(ppu.bghofs_latch&7);
		     ppu.bgofs_latch = ppu.bghofs_latch = v;
		     return;
	case 0x2113: ppu.bg4hofs = (v<<8)|(ppu.bgofs_latch&~7)|(ppu.bghofs_latch&7);
		     ppu.bgofs_latch = ppu.bghofs_latch = v;
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
		     
	case 0x2116: ppu.wmaddl = v; return;
	case 0x2117: ppu.wmaddh = v; return;
	
	case 0x2118: { u32 waddr = ((ppu.wmaddh<<8)|ppu.wmaddl)&0x7fff;
		       ppu.vram[(waddr*2)&0xffff] = v;
		       if( !(ppu.vmain & BIT(7)) ) { ppu.wmaddl+=1; if(!ppu.wmaddl) ppu.wmaddh++; }
		      }return;
	case 0x2119: { u32 waddr = ((ppu.wmaddh<<8)|ppu.wmaddl)&0x7fff;
		       ppu.vram[(waddr*2 + 1)&0xffff] = v;
		       if( (ppu.vmain & BIT(7)) ) { ppu.wmaddl+=1; if(!ppu.wmaddl) ppu.wmaddh++; }
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
	case 0x212C: ppu.tm = v&0x1f; return;
	case 0x212D: ppu.ts = v&0x1f; return;
	case 0x212E: ppu.tmw = v&0x1f; return;
	case 0x212F: ppu.tsw = v&0x1f; return;
	case 0x2130: ppu.cgwsel = v&0xf3; return;
	case 0x2131: ppu.cgadsub = v; return;
	case 0x2132: ppu.coldata = v; return;
	case 0x2133: ppu.setini = v&0xcf; return;
	

	case 0x2140: apu.to_spc[0] = v; return;
	case 0x2141: apu.to_spc[1] = v; return;
	case 0x2142: apu.to_spc[2] = v; return;
	case 0x2143: apu.to_spc[3] = v; return;
	
	case 0x2180:{
		     u32 addr = (io.wmaddh<<16)|(io.wmaddm<<8)|io.wmaddl;
		     ram[addr++] = v;
		     io.wmaddh = addr>>16;
		     io.wmaddm = addr>>8;
		     io.wmaddl = addr;
		     }return;
			
	case 0x2181: io.wmaddl = v; return;
	case 0x2182: io.wmaddm = v; return;
	case 0x2183: io.wmaddh = v&1; return;
	
	case 0x4016: /*todo joypad*/ return;
	
	case 0x4200: io.nmitimen = v&0xb1; return;
	case 0x4201: io.wrio = v; return;
	case 0x4202: io.wrmpya = v; return;
	case 0x4203: io.wrmpyb = v; /*todo: multiply*/ return;
	case 0x4204: io.wrdivl = v; return;
	case 0x4205: io.wrdivh = v; return;
	case 0x4206: io.wrdivb = v; return;
	case 0x4207: io.htimel = v; return;
	case 0x4208: io.htimeh = v&1; return;
	case 0x4209: io.vtimel = v; return;
	case 0x420A: io.vtimeh = v&1; return;
	case 0x420B: io.mdmaen = v;
		     for(u32 i = 0; i < 8; ++i) { if( io.mdmaen & BIT(i) ) { run_dma(i); io.mdmaen&=~BIT(i); } }
		     return;
	case 0x420C: io.hdmaen = v; return;	
	case 0x420D: io.memsel = v&1; return;
	

	default:
		if( a >= 0x4300 && a < 0x4380 ) { ppu.dmaregs[a&0x7f] = v; return; }
		std::println("io wr ${:X}:${:X} = ${:X}", bank, a, v);
		//exit(1);
		return;
	}
}

u8 snes::io_read(u8 bank, u32 a)
{
	if( a < 0x2000 ) { do_master_cycles(2); return ram[a]; }
	switch( a )
	{
	case 0x2140: return apu.to_cpu[0];
	case 0x2141: return apu.to_cpu[1];
	case 0x2142: return apu.to_cpu[2];
	case 0x2143: return apu.to_cpu[3];
	case 0x4210: { u8 v = ppu.rdnmi; ppu.rdnmi &= 0x7f; return 2 | v; }
	}
	std::println("${:X}: io rd ${:X}:${:X}", cpu.pc, bank, a);
	exit(1);
	return 0;
}

void snes::write(u32 a, u8 v)
{
	u8 bank = a>>16;
	a &= 0xffff;
	// WRAM region is same in all mapping modes
	if( bank == 0x7E || bank == 0x7F ) { do_master_cycles(2); ram[((bank&1)<<16)|a] = v; return; }
	// as is the io region, although io/rw might fall through to cart expansion io
	if( (bank&0x7F) < 0x40 && a < 0x8000 ) { io_write(bank, a, v); return; }


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
		return ROM[(bank*1024*32 + (a&0x7fff)) % ROM.size()];
	}
	
	std::println("snes rd ${:X}:${:X}", bank, a);
	return 0;
}

void snes::do_master_cycles(s64 master_cycles)
{
	ppu_mc(master_cycles);

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
	const u16 baddr = 0x2100 | ppu.dmaregs[rb|1];
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
		
		if( param & 0x80 )
		{
			//std::println("dma to mem rd ${:X}, wr ${:X}", baddr+mod, (bank<<16)|src);
			write((bank<<16)|src, io_read(0, baddr+mod));
		} else {
			//std::println("dma to dev rd ${:X}, wr ${:X}", (bank<<16)|src, baddr+mod);
			io_write(0, baddr+mod, read((bank<<16)|src));
		}
		
		if( ((param>>3) & 3) == 0 )
		{
			src += 1;
		} else if( ((param>>3) & 3) == 2 ) {
			src -= 1;
		}
	}
	
	
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
		
	return true;
}


