#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "aes.h"
extern console* sys;

int irq_count = 0;
bool rtc_active = false;

static u32 c16to32(u16 c)
{
	u8 DB = (c&BIT(15))?4:0;
	u8 r = (c>>8)&0xf;
	r <<= 4;
	r |= (c&BIT(14))?8:0;
	r |= DB;
	u8 g = (c>>4)&0xf;
	g <<= 4;
	g |= (c&BIT(13))?8:0;
	g |= DB;
	u8 b = c & 15;
	b <<= 4;
	b |= (c&BIT(12))?8:0;
	b |= DB;
	return (r<<24)|(g<<16)|(b<<8);
}

static u8 sprite_hshrink[] = {
0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,
0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,
0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,
0,0,1,0,1,0,0,0,1,0,0,0,1,0,0,0,
0,0,1,0,1,0,0,0,1,0,0,0,1,0,1,0,
0,0,1,0,1,0,1,0,1,0,0,0,1,0,1,0,
0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,
1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,
1,0,1,0,1,0,1,0,1,1,1,0,1,0,1,0,
1,0,1,1,1,0,1,0,1,1,1,0,1,0,1,0,
1,0,1,1,1,0,1,0,1,1,1,0,1,0,1,1,
1,0,1,1,1,0,1,1,1,1,1,0,1,0,1,1,
1,0,1,1,1,0,1,1,1,1,1,0,1,1,1,1,
1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1,
1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};

void AES::draw_scanline(int line)
{
	const u32 backdrop = c16to32(__builtin_bswap16(*(u16*)&palette[palbank*0x2000 + 0x1ffe]));
	for(u32 i = 0; i < 320; ++i) fbuf[line*320 + i] = backdrop;
	if( bios_grom ) { return; } // not yet supporting bios graphics
	//todo: sprites
	
	
	
	if( s1.empty() ) return;
	for(int i = 0; i < 320; ++i)
	{
		int tX = i/8;
		int tY = 2 + line/8;
		
		int map_entry = vram[0x7000 + tX*0x20 + tY];
		int tile_num = map_entry & 0xfff;
		int palnum = map_entry >> 12;
		
		int x = i&7;
		u8 b = s1[(tile_num<<5) | ((x&4)?0:BIT(4)) | ((x&2)?BIT(3):0) | (line&7)];
		b >>= (x&1)?4:0;
		b &= 15;
		if( b == 0 ) continue;
		u16 c = (palette[palbank*0x2000 + palnum*32 + b*2]<<8)|palette[palbank*0x2000 + palnum*32 + b*2 + 1];
		fbuf[line*320 + i] = c16to32(c);
	}
}

void AES::write(u32 addr, u32 v, int size)
{
	addr &= 0xffFFff;
	if( addr >= 0x100000 && addr <= 0x1FFFFF )
	{
		addr &= 0xffff;
		if( size == 16 )
		{
			wram[addr] = v>>8;
			addr += 1;
		}
		wram[addr] = v;
		return;
	}
	if( addr >= 0x200000 && addr <= 0x2FFFFF )
	{
		pbank2 = v&3;
		return;
	}
	if( addr >= 0x300000 && addr <= 0x3FFFFF )
	{
		if( addr == 0x300001 ) { return; } // kick watchdog
		if( addr == 0x320000 ) { z80_cmd = v; if( v==1) z80_reply = 1; return; }
		if( addr == 0x380031 || addr == 0x380041 ) { return; } // LED stuff?
		if( addr == 0x380065 || addr == 0x380067 ) { return; } // coin lockout?
		if( addr == 0x3A0001 ) { return; } // normal (nondarkened) gfx
		if( addr == 0x3A0003 ) { bios_vectors = true; return; }
		if( addr == 0x3A0013 ) { bios_vectors = false; return; }
		if( addr == 0x3A0007 || addr == 0x3A0015 ) { return; } // disable memory card
		if( addr == 0x3A000B ) { bios_grom = true; return; }
		if( addr == 0x3A001B ) { bios_grom = false; return; }
		if( addr == 0x3A000F ) { palbank = 1; return; }
		if( addr == 0x3A001F ) { palbank = 0; return; }
		if( addr == 0x3A000D ) { return; } // lock backupram
		if( addr == 0x3A001D ) { return; } // unlock backupram
		if( addr == 0x3C0004 ) { REG_VRAMMOD = v; return; }
		if( addr == 0x3C0000 ) { REG_VRAMADDR = v; return; }
		if( addr == 0x3C0002 )
		{
			printf("VRAM Write $%X = $%X\n", REG_VRAMADDR, v);
			vram[REG_VRAMADDR] = v;
			REG_VRAMADDR += REG_VRAMMOD;
			return;		
		}
		if( addr == 0x3C000C )
		{
			//if( size != 16 ) { printf("AES: 3C000C, non-16bit write = $%X\n", v); exit(1); }
			if( v & 1 ) irq3_active = false;
			if( v & 2 ) timer_irq_active = false;
			if( v & 4 ) vblank_irq_active = false;			
			return;
		}
		if( addr == 0x3C0006 ) 
		{ 
			if( size != 16 ) { printf("AES: 3C0006, non-16bit write = $%X\n", v); exit(1); }
			REG_LSPCMODE = v; return; 
		}
		if( addr == 0x380021 || addr == 0x380001 ) { return; } // SLOT or joypad stuff
		
		
		if( addr == 0x380051 )
		{
			printf("RTC51 = $%X\n", v);
			if( v == 4 ) 
			{
				irq_count = 29;
				rtc_active = !rtc_active;
			}
			return;
		}
	}
	if( addr >= 0x400000 && addr <= 0x4FFFFF )
	{
		addr &= 0x1fff;
		addr += palbank*0x2000;
		if( size == 16 )
		{
			palette[addr] = v>>8;
			addr += 1;
		}
		palette[addr] = v;
		return;
	}
	if( addr >= 0xD00000 && addr <= 0xDFFFFF )
	{
		addr &= 0xffff;
		if( size == 16 )
		{
			backup[addr] = v>>8;
			addr += 1;
		}
		backup[addr] = v;
		return;
	}
	printf("W%i $%X = $%X\n", size, addr, v);
	exit(1);
}

u32 AES::read(u32 addr, int size)
{
	addr &= 0xffFFff;
	if( addr < 0x100000 )
	{
		u16 res = 0;
		if( addr < 256 && bios_vectors )
		{
			if( size == 16 ) { res = bios[addr]<<8; addr += 1; }
			return res|bios[addr];
		}
		if( size == 16 ) { res = p1[pbank1*0x100000 + addr]<<8; addr+=1; }
		return res|p1[pbank1*0x100000 + addr];
	}
	if( addr >= 0x100000 && addr <= 0x1FFFFF )
	{
		u16 res = 0;
		addr &= 0xffff;
		if( size == 16 )
		{
			res = wram[addr]<<8;
			addr += 1;
		}
		return res|wram[addr];
	}
	if( addr >= 0x200000 && addr <= 0x2FFFFF )
	{
		u16 res = 0;
		addr &= 0xffff;
		if( size == 16 )
		{
			res = p1[(pbank2+1)*0x100000 + (addr&0xfffff)]<<8;
			addr += 1;
		}
		return res|p1[(pbank2+1)*0x100000 + (addr&0xfffff)];
	}
	if( addr >= 0x300000 && addr <= 0x3FFFFF )
	{
		if( addr == 0x300001 ) { return 0xff; } // dip switches (active low)
		if( addr == 0x320000 ) { return z80_reply; }
		if( addr == 0x300081 ) { return 0; } // not active low?
		if( addr == 0x3C0004 && size == 16 ) { return REG_VRAMMOD; }
		if( (addr == 0x3C0002 || addr == 0x3C0008 || addr == 0x3C000A ) && size == 16 ) 
		{ 
			printf("VRAM read $%X\n", REG_VRAMADDR); 
			return vram[REG_VRAMADDR]; 
		}
		if( addr == 0x3C0006 ) { return line_counter<<7; }
		if( addr == 0x380000 ) { return 0x7f; } // STATUS_B
		
		if( addr == 0x300000 ) { return 0xff; } // joypad 1
		if( addr == 0x340000 ) { return 0xff; } // joypad 2
		
		if( addr == 0x320001 ) { return 0x3f | rtc_val; }
		printf("IO%i <$%X\n", size, addr);
		exit(1);
		return 0;
	}
	if( addr >= 0x400000 && addr <= 0x4FFFFF )
	{
		addr &= 0x1fff;
		addr += palbank*0x2000;
		u16 res = 0;
		if( size == 16 )
		{
			res = palette[addr]<<8;
			addr += 1;
		}
		return res|palette[addr];
	}
	if( addr >= 0xC00000 && addr <= 0xCFFFFF )
	{
		addr &= 0x1FFFF;
		u16 res = 0;
		if( size == 16 ) { res = bios[addr]<<8; addr += 1; }
		return res|bios[addr];
	}
	if( addr >= 0xD00000 && addr <= 0xDFFFFF )
	{
		u16 res = 0;
		addr &= 0xffff;
		if( size == 16 )
		{
			res = backup[addr]<<8;
			addr += 1;
		}
		return res|backup[addr];
	}
	printf("R%i <$%X\n", size, addr);
	exit(1);
	return 0;
}

bool AES::loadNEO(const std::string fname)
{
	FILE* fp = fopen(fname.c_str(), "rb");
	if( !fp )
	{
		printf("Unable to load <%s>\n", fname.c_str());
		return false;
	}

	fseek(fp, 0, SEEK_END);
	auto fsz = ftell(fp);
	fseek(fp, 4, SEEK_SET); // skip ident bytes
	
	u32 psize, s1size, csize, v1size, v2size, msize;
	int unu = fread(&psize, 1, 4, fp);
	unu = fread(&s1size, 1, 4, fp);
	unu = fread(&msize, 1, 4, fp);
	unu = fread(&v1size, 1, 4, fp);
	unu = fread(&v2size, 1, 4, fp);
	unu = fread(&csize, 1, 4, fp);
	fseek(fp, 4096, SEEK_SET);
	
	p1.resize(psize);
	unu = fread(p1.data(), 1, psize, fp);
	for(u32 i = 0; i < p1.size(); i+=2)
	{
		*(u16*)&p1[i] = __builtin_bswap16(*(u16*)&p1[i]);
	}
	s1.resize(s1size);
	unu = fread(s1.data(), 1, s1size, fp);
	return true;
}

bool AES::loadROM(const std::string fname)
{
	std::string sysrom = "./bios/aes.bios";
	FILE* fp = fopen(sysrom.c_str(), "rb");
	if( !fp )
	{
		printf("Unable to open <%s>\n", sysrom.c_str());
		return false;
	}
	[[maybe_unused]] int unu = fread(bios, 1, 0x20000, fp);
	fclose(fp);
	for(u32 i = 0; i < 0x20000; i+=2)
	{
		*(u16*)&bios[i] = __builtin_bswap16(*(u16*)&bios[i]);
	}
	
	if( fname.ends_with(".neo") )
	{
		return loadNEO(fname);	
	}
	
	std::string prefix = fname.substr(0, fname.rfind('.'));
	std::string promfile = prefix + ".p1";
	fp = fopen(promfile.c_str(), "rb");
	if( !fp )
	{
		printf("Unable to open <%s>\n", promfile.c_str());
		return false;
	}
	
	fseek(fp, 0, SEEK_END);
	auto fsz = ftell(fp);
	if( fsz < 0x100000 ) fsz = 0x100000;
	if( fsz & 1 ) fsz += 1;
	fseek(fp, 0, SEEK_SET);
	p1.resize(fsz);
	unu = fread(p1.data(), 1, fsz, fp);
	fclose(fp);
	
	for(u32 i = 0; i < p1.size(); i+=2)
	{
		*(u16*)&p1[i] = __builtin_bswap16(*(u16*)&p1[i]);
	}
	
	std::string zfile = prefix + ".m1";
	fp = fopen(zfile.c_str(), "rb");
	if( !fp )
	{
		printf("Unable to open <%s>\n", zfile.c_str());
		return false;
	}
	fseek(fp, 0, SEEK_END);
	fsz = ftell(fp);
	if( fsz < 0x10000 ) fsz = 0x10000;
	fseek(fp, 0, SEEK_SET);
	m1.resize(fsz);
	unu = fread(m1.data(), 1, fsz, fp);
	fclose(fp);
	
	std::string fixfile = prefix + ".s1";
	fp = fopen(fixfile.c_str(), "rb");
	if( !fp )
	{
		printf("Unable to open <%s>\n", fixfile.c_str());
		return false;
	}
	fseek(fp, 0, SEEK_END);
	fsz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	s1.resize(fsz);
	unu = fread(s1.data(), 1, fsz, fp);
	fclose(fp);
	return true;
}

void AES::run_frame()
{
	for(u32 line = 0; line < 262; ++line)
	{
		line_counter = line;
		u64 target = last_target + 763;
		while( mstamp < target )
		{
			cpu.step();
			mstamp += cpu.icycles;
		}
		last_target = target;
		if( line < 224 ) draw_scanline(line);	
	}
	
	if( !vblank_irq_active ) cpu.pending_irq = 1; //only if not ack'd or not?
		
	if( rtc_active && irq_count )
	{
		irq_count--;
		if( !irq_count ) 
		{
			rtc_val ^= 0x40;
			irq_count = 30;
		}
	}
	//printf("$%X\n", cpu.pc);
}

void AES::reset()
{
	memset(&cpu, 0, sizeof(cpu));
	cpu.mem_read8 = [](u32 addr)->u8 {return dynamic_cast<AES*>(sys)->read(addr, 8); };
	cpu.mem_read16 =[](u32 addr)->u16{return dynamic_cast<AES*>(sys)->read(addr, 16); };
	cpu.read_code16 = cpu.mem_read16;
	cpu.mem_read32 =[](u32 addr)->u32{return(dynamic_cast<AES*>(sys)->read(addr, 16)<<16)|(dynamic_cast<AES*>(sys)->read(addr+2, 16)); };
	cpu.mem_write8 =[](u32 addr, u8 v){ dynamic_cast<AES*>(sys)->write(addr, v, 8); };
	cpu.mem_write16 =[](u32 addr, u16 v){dynamic_cast<AES*>(sys)->write(addr, v, 16); };
	cpu.mem_write32 =[](u32 addr, u32 v)
		{dynamic_cast<AES*>(sys)->write(addr, v>>16, 16); dynamic_cast<AES*>(sys)->write(addr+2, v, 16); };
	cpu.intack = []{};
	bios_vectors = true;
	bios_grom = true;
	cpu.pc = cpu.mem_read32(4);
	cpu.r[15] = cpu.mem_read32(0);
	cpu.r[16] = cpu.r[15];
	cpu.sr.raw = 0x2700;
	printf("AES: Start PC = $%X\n", cpu.pc);
	printf("AES: Start SP = $%X\n", cpu.r[15]);
	palbank = 0;
	pbank1 = 0;
	pbank2 = 0;
	spu.reset();
	
	rtc_val = 0x40;
	
	vblank_irq_active = irq3_active = timer_irq_active = false;
}

