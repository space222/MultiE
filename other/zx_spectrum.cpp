#include <print>
#include <string>
#include "zx_spectrum.h"

u8 zx_spectrum::read(u16 a)
{
	if( version == 128 )
	{
		if( a < 0x4000 ) { return bios[((bank_port&BIT(4))?0x4000:0)+a]; }
		if( a < 0x8000 ) { return ram[0x4000*5 + (a&0x3fff)]; }
		if( a < 0xC000 ) { return ram[0x4000*2 + (a&0x3fff)]; }
		return ram[(bank_port&7)*0x4000 + (a&0x3fff)];	
	}

	if( a < 0x4000 ) { return bios[a]; }
	return ram[a];
}

void zx_spectrum::write(u16 a, u8 v)
{
	if( version == 128 )
	{
		if( a < 0x4000 ) { return; }
		else if( a < 0x8000 ) { ram[0x4000*5 + (a&0x3fff)] = v; }
		else if( a < 0xC000 ) { ram[0x4000*2 + (a&0x3fff)] = v; }
		else ram[(bank_port&7)*0x4000 + (a&0x3fff)] = v;
		return;
	}
	ram[a] = v;
	return;
}

u8 zx_spectrum::in(u16 p)
{
	if( (p&0xff) == 0xfe )
	{
		u8 hi = p>>8;
		u8 v = 0xff;
		auto keys = SDL_GetKeyboardState(nullptr);
		if( !(hi & 1) )
		{
			if( keys[SDL_SCANCODE_LSHIFT] ) v &= ~1;
			if( keys[SDL_SCANCODE_Z] ) v &= ~2;
			if( keys[SDL_SCANCODE_X] ) v &= ~4;
			if( keys[SDL_SCANCODE_C] ) v &= ~8;
			if( keys[SDL_SCANCODE_V] ) v &= ~16;
		}
		if( !(hi & 2) )
		{
			if( keys[SDL_SCANCODE_A] ) v &= ~1;
			if( keys[SDL_SCANCODE_S] ) v &= ~2;
			if( keys[SDL_SCANCODE_D] ) v &= ~4;
			if( keys[SDL_SCANCODE_F] ) v &= ~8;
			if( keys[SDL_SCANCODE_G] ) v &= ~16;
		}
		if( !(hi & 4) )
		{
			if( keys[SDL_SCANCODE_Q] ) v &= ~1;
			if( keys[SDL_SCANCODE_W] ) v &= ~2;
			if( keys[SDL_SCANCODE_E] ) v &= ~4;
			if( keys[SDL_SCANCODE_R] ) v &= ~8;
			if( keys[SDL_SCANCODE_T] ) v &= ~16;		
		}
		if( !(hi & 8) )
		{
			if( keys[SDL_SCANCODE_1] ) v &= ~1;
			if( keys[SDL_SCANCODE_2] ) v &= ~2;
			if( keys[SDL_SCANCODE_3] ) v &= ~4;
			if( keys[SDL_SCANCODE_4] ) v &= ~8;
			if( keys[SDL_SCANCODE_5] ) v &= ~16;		
		}
		if( !(hi & 0x10) )
		{
			if( keys[SDL_SCANCODE_0] ) v &= ~1;
			if( keys[SDL_SCANCODE_9] ) v &= ~2;
			if( keys[SDL_SCANCODE_8] ) v &= ~4;
			if( keys[SDL_SCANCODE_7] ) v &= ~8;
			if( keys[SDL_SCANCODE_6] ) v &= ~16;		
		}
		if( !(hi & 0x20) )
		{
			if( keys[SDL_SCANCODE_P] ) v &= ~1;
			if( keys[SDL_SCANCODE_O] ) v &= ~2;
			if( keys[SDL_SCANCODE_I] ) v &= ~4;
			if( keys[SDL_SCANCODE_U] ) v &= ~8;
			if( keys[SDL_SCANCODE_Y] ) v &= ~16;		
		}
		if( !(hi & 0x40) )
		{
			if( keys[SDL_SCANCODE_RETURN] ) v &= ~1;
			if( keys[SDL_SCANCODE_L] ) v &= ~2;
			if( keys[SDL_SCANCODE_K] ) v &= ~4;
			if( keys[SDL_SCANCODE_J] ) v &= ~8;
			if( keys[SDL_SCANCODE_H] ) v &= ~16;		
		}
		if( !(hi & 0x80) )
		{
			if( keys[SDL_SCANCODE_SPACE] ) v &= ~1;
			if( keys[SDL_SCANCODE_RSHIFT] ) v &= ~2;
			if( keys[SDL_SCANCODE_M] ) v &= ~4;
			if( keys[SDL_SCANCODE_N] ) v &= ~8;
			if( keys[SDL_SCANCODE_B] ) v &= ~16;		
		}
		return v;	
	}

	std::println("in ${:X}", p);
	return 0xFF;
}

void zx_spectrum::out(u16 p, u8 v)
{
	if( version == 128 && p == 0x7FFD )
	{
		bank_port = v;
		return;
	}

	std::println("out ${:X} = ${:X}", p, v);
}

static u32 zx_pal[] = { 0, 0xcd, 0xcd0000, 0xcd00cd, 0xcd00, 0xcdcd, 0xcdcd00, 0xcdcdcd,
			0, 0xff, 0xff0000, 0xff00ff, 0xff00, 0xffff, 0xffff00, 0xffffff };

void zx_spectrum::run_frame()
{
	flashdiv = (flashdiv+1)&0x1F;
	
	for(u32 line = 0; line < 312; ++line)
	{
		target = last_target + 224;
		while( stamp < target )
		{
			u64 cc = cpu.step();
			stamp += cc;
			
			if( cpu.pc == 0x556 )
			{
				std::println("load tap file: IX = ${:X}, DE = ${:X}", (cpu.IXH()<<8)|cpu.IXL(), (cpu.D()<<8)|cpu.E());
				u16 addr = (cpu.IXH()<<8)|cpu.IXL();
				u16 len = (cpu.D()<<8)|cpu.E();
				
				if( cpu.fl.b.fC == 1 )
				{
					if( version == 128 )
					{
						for(u32 i = 0; i < len; ++i)
						{
							write(addr+i, *(tap.data()+tap_offset+3+i));
						}
					} else {
						memcpy(ram+addr, tap.data()+tap_offset+3, len);
					}
				}
								
				cpu.D() = 0;
				cpu.E() = 0;
				cpu.H() = 0;
				cpu.L() = 0;
				addr += len;
				cpu.A() = *(tap.data()+tap_offset);
				tap_offset += len+4;
				cpu.IXH() = addr>>8;
				cpu.IXL() = addr;
				cpu.fl.b.fC = 1;
				cpu.pc = 0x5DF;
			}
		}
		last_target = target;
		
		if( line == 192 )
		{
			cpu.irq_line = true;
		} else {
			cpu.irq_line = false;
		}
	}
	
	for(u32 y = 0; y < 192; ++y)
	{
		for(u32 x = 0; x < 256; ++x)
		{
			u32 addr_hi = ((y&0xc0)>>3) | (y&7);
			u32 addr_lo = ((y&0x38)<<2) | (x>>3);
			u8 B = 0;			
			if( version==128 )
			{
				B = ram[((bank_port&8)?7:5)*0x4000 + (addr_hi<<8) + addr_lo];
			} else {
				B = ram[0x4000 + (addr_hi<<8) + addr_lo];
			}
			B >>= (7^x)&7;
			B &= 1;
			
			u8 ab = (version==128 ? 
					  ram[((bank_port&8)?7:5)*0x4000 + 0x1800 + (y/8)*32 + (x/8)]
					: ram[ 0x5800 + (y/8)*32 + (x/8) ]);
			
			u8 ink = ab & 7;
			u8 bg = (ab >> 3) & 7;
			
			if( ab & 0x80 ) { B ^= (flashdiv>>4); }					
			fbuf[y*256+x] = zx_pal[((ab&BIT(6))?8:0) + (B ? ink : bg)]<<8;	
		}
	}
}

void zx_spectrum::reset()
{
	memset((void*)&cpu, 0, sizeof(cpu));
	cpu.read = [&](u16 a)->u8 { return read(a); };
	cpu.write= [&](u16 a, u8 v) { write(a,v); };
	cpu.in = [&](u16 p)-> u8 { return in(p); };
	cpu.out = [&](u16 p, u8 v) { out(p, v); };
	cpu.halted = false;
	stamp = target = last_target = 0;
	tap_offset = 0;
	bank_port = 0;
}

bool zx_spectrum::loadROM(const std::string fname)
{
	FILE* fp = fopen(version==128?"./bios/zx128.rom":"./bios/48.rom", "rb");
	if( !fp ) { std::println("Unable to load ZX bios"); return false; }
	[[maybe_unused]] int unu = fread(bios, 1, 32*1024, fp);
	fclose(fp);
	
	fp = fopen(fname.c_str(), "rb");
	if( !fp ) { std::println("unable to load tap file, continuing.."); return true; }
	fseek(fp, 0, SEEK_END);
	tap.resize(ftell(fp));
	fseek(fp, 0, SEEK_SET);
	unu = fread(tap.data(), 1, tap.size(), fp);
	fclose(fp);
	
	return true;
}

