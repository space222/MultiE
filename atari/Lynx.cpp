#include <print>
#include <vector>
#include <string>
#include "Lynx.h"

#define SUZYHREV 0xFC88
#define IODAT    0xFD8B
#define SYSCTL1  0xFD87
#define RCART    0xFCB2
#define PALSTART 0xFDA0
#define PALEND   0xFDBF
#define DISPADRH 0xFD95
#define DISPADRL 0xFD94
#define SPRBEGIN 0xFC00
#define SPREND   0xFC7F
#define SPRG0    0xFC91
#define DISPCTL  0xFD92
#define KEYPAD   0xFCB0

u8 Lynx::mikey_read(u16 addr)
{
	if( addr >= PALSTART && addr <= PALEND )
	{
		return palette[addr&0x1F];
	}
	std::println("Mikey <${:X}", addr);
	return 0;
}

u8 Lynx::suzy_read(u16 addr)
{
	if( addr == SUZYHREV )
	{ // hardware version
		return 1;
	}
	if( addr == RCART )
	{
		u8 r = rom[(cart_block<<cart_shift)|(cart_offset&cart_mask)];
		//std::println("cart read [${:X}:${:X}] got ${:X}", cart_block, cart_offset, r);
		cart_offset += 1;
		return r;
	}
	if( addr >= SPRBEGIN && addr <= SPREND )
	{
		return spr[addr&0x7f];
	}
	if( addr == KEYPAD )
	{
		return 0; //todo
	}
	
	std::println("Suzy <${:X}", addr);
	return 0;
}

void Lynx::mikey_write(u16 addr, u8 v)
{
	if( addr == IODAT )
	{
		cart_data = v;
		return;
	}
	if( addr == SYSCTL1 )
	{
		if( (cart_strobe&1) && !(v&1) )
		{
			cart_block <<= 1;
			cart_block |= (cart_data>>1)&1;	
		}
		if( !(v&1) )
		{
			cart_offset = 0;
		}
		cart_strobe = v;
		return;	
	}
	if( addr >= PALSTART && addr <= PALEND )
	{
		palette[addr&0x1F] = v;
		return;
	}
	if( addr == DISPADRH )
	{
		fb_addr = (fb_addr&0xff)|(v<<8);
		return;
	}
	if( addr == DISPADRL )
	{
		fb_addr = (fb_addr&0xff00)|v;
		return;
	}
	if( addr == DISPCTL )
	{
		dispctl = v;
		return;
	}
	std::println("Mikey ${:X} = ${:X}", addr, v);
}

void Lynx::suzy_write(u16 addr, u8 v)
{
	if( addr >= SPRBEGIN && addr <= SPREND )
	{
		spr[addr&0x7f] = v;
		if( !(addr & 1) ) spr[(addr&0x7f)|1] = 0;
		return;
	}
	if( addr == SPRG0 )
	{
		if( v & 1 )
		{
			suzy_draw();
		}
		return;
	}
	std::println("Suzy ${:X} = ${:X}", addr, v);
}

u8 Lynx::read(u16 addr)
{
	if( addr >= 0xfffa || addr == 0xfff8 )
	{
		if( mmctrl_vectors() ) return bios[addr&0x1ff];
		return RAM[addr];
	}
	if( addr == 0xfff9 )
	{
		return mmctrl;
	}
	if( addr >= 0xfe00 )
	{
		if( mmctrl_rom() ) return bios[addr&0x1ff];
		return RAM[addr];
	}
	if( addr >= 0xfd00 )
	{
		if( mmctrl_mikey() ) return mikey_read(addr);
		return RAM[addr];
	}
	if( addr >= 0xfc00 )
	{
		if( mmctrl_suzy() ) return suzy_read(addr);
		return RAM[addr];
	}
	return RAM[addr];
}

void Lynx::write(u16 addr, u8 v)
{
	if( addr >= 0xfffa || addr == 0xfff8 )
	{
		if( !mmctrl_vectors() ) RAM[addr] = v;
		return;
	}
	if( addr == 0xfff9 )
	{
		mmctrl = v;
		return;
	}
	if( addr >= 0xfe00 )
	{
		if( !mmctrl_rom() ) RAM[addr] = v;
		return;
	}
	if( addr >= 0xfd00 )
	{
		if( mmctrl_mikey() )
		{
			mikey_write(addr, v);
		} else {
			RAM[addr] = v;
		}
		return;
	}
	if( addr >= 0xfc00 )
	{
		if( mmctrl_suzy() )
		{
			suzy_write(addr, v);
		} else {
			RAM[addr] = v;
		}
		return;
	}
	RAM[addr] = v;
}

void Lynx::fb_write(int x, int y, u8 v)
{
	if( dispctl & BIT(2) )
	{ // 4bpp
		RAM[fb_addr + y*80 + (x>>1)] &= 0xf<<((x&1)*4);
		RAM[fb_addr + y*80 + (x>>1)] |= (v&15)<<(((x&1)^1)*4);	
	} else {
		// 2bpp
		RAM[fb_addr + y*40 + (x>>2)] &= ~( 3<<((x&3)*2) );
		RAM[fb_addr + y*40 + (x>>2)] |= (v&3)<<((x&3)*2);	
	}
}

void Lynx::suzy_draw()
{
	u16 offs = *(u16*)&spr[0x10];
	//u16 hsz_offs = *(u16*)&spr[0x28];
	//u16 vsz_offs = *(u16*)&spr[0x2A];
	
	while( offs )
	{
		
		std::println("drawing sprite at ${:X}", offs);

		u8 sprctl0 = RAM[offs];
		u8 sprctl1 = RAM[offs+1];
		u8 sprcoll = RAM[offs+2];
		u16 ptr = *(u16*)&RAM[offs+3];
		u16 data = *(u16*)&RAM[offs+5];
		u16 hpos = *(u16*)&RAM[offs+7];
		u16 vpos = *(u16*)&RAM[offs+9];
		u16 hsz =  *(u16*)&RAM[offs+11];
		u16 vsz =  *(u16*)&RAM[offs+13];
		u16 stretch=*(u16*)&RAM[offs+15];
		u16 tilt = *(u16*)&RAM[offs+17];
		
		std::println("ctl0\t${:X}", sprctl0);
		std::println("ctl1\t${:X}", sprctl1);
		std::println("coll\t${:X}", sprcoll);
		std::println("ptr\t${:X}", ptr);
		std::println("data\t${:X}", data);
		std::println("hpos\t{}", short(hpos));
		std::println("vpos\t{}", short(vpos));
		std::println("hsz\t${:X}", hsz);
		std::println("vsz\t${:X}", vsz);
		offs = ptr;
		
		for(u32 i = 0; i < 0x10; ++i) std::print("${:X} ", RAM[data+i]);
		std::println("\n----");
		
		if( sprctl1 & 0x80 )
		{ //totally literal
			u8 ls = RAM[data];
			int y = vpos;
			while( ls )
			{
				for(u32 i = 0; i < ls; ++i)
				{
					u8 b = RAM[data+1+i];
					switch( sprctl0>>6 )
					{
					case 0:
						for(u32 bit = 0; bit < 8; ++bit) fb_write(hpos+i*8+bit, y, (b>>(7-bit))&1);
						break;
					case 1:
						for(u32 bit = 0; bit < 4; ++bit) fb_write(hpos+i*4+bit, y, (b>>((3-bit)*2))&3); 
						break;
					case 3:
						fb_write(hpos+i*2, y, b>>4);
						fb_write(hpos+i*2+1, y, b&15);
						break;
					}				
				}
				data += ls;
				ls = RAM[data];
				y += 1;
				if( y >= 102 ) break;
			}
			continue;
		}
		
		//bitstream of packed and unpacked(kinda literal?) data
		//todo
	}

}

void Lynx::run_frame()
{
	for(u32 i = 0; i < 35000; ++i)
	{
		//std::println("${:X}", cpu.pc);
		cycle();
	}
	
	if( dispctl & BIT(2) )
	{
		for(u32 y = 0; y < 102; ++y)
		{
			for(u32 x = 0; x < 160; ++x)
			{
				u8 b = RAM[(fb_addr + (y*80) + (x>>1)) & 0xffff];
				b >>= ((x&1) ? 4 : 0);
				b &= 15;
				u8 R = palette[0x10 + b]&15;
				u8 B = palette[0x10 + b]>>4;
				u8 G = palette[b]&15;
				R = (R<<4)|R;
				G = (G<<4)|G;
				B = (B<<4)|B;
				fbuf[y*160+x] = (R<<24)|(G<<16)|(B<<8);	
			}
		}
	} else {
		for(u32 y = 0; y < 102; ++y)
		{
			for(u32 x = 0; x < 160; ++x)
			{
				u8 b = RAM[(fb_addr + (y*40) + (x>>2)) & 0xffff];
				b >>= (((x&3)^3)*2);
				b &= 3;
				u8 R = palette[0x10 + b]&15;
				u8 B = palette[0x10 + b]>>4;
				u8 G = palette[b]&15;
				R = (R<<4)|R;
				G = (G<<4)|G;
				B = (B<<4)|B;
				fbuf[y*160+x] = (R<<24)|(G<<16)|(B<<8);	
			}
		}	
	}
}

void Lynx::reset()
{
	mmctrl = 0;
	cpu.reader = [&](coru6502&, u16 a)->u8{ return read(a); };
	cpu.writer = [&](coru6502&, u16 a, u8 v) { write(a,v); };
	cpu.cpu_type = CPU_65C02;
	cycle = cpu.run();
	for(u32 i = 0; i < 0x10000; ++i) RAM[i] = 0xff;
}

bool Lynx::loadROM(const std::string fname)
{
	FILE* fp = fopen("./bios/lynxboot.img", "rb");
	if( !fp )
	{
		std::println("Need lynxboot.img in ./bios");
		return false;
	}
	[[maybe_unused]] int unu = fread(bios, 1, 512, fp);
	fclose(fp);
	
	fp = fopen(fname.c_str(), "rb");
	if( !fp )
	{
		std::println("Unable to open <{}>", fname);
		return false;
	}
	fseek(fp, 0, SEEK_END);
	auto fsz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	rom.resize(fsz);
	unu = fread(rom.data(), 1, fsz, fp);
	fclose(fp);
	if( rom[0] == 'L' && rom[1] == 'Y' && rom[2] == 'N' && rom[3] == 'X' )
	{
		std::println("Lynx ROM has unsupported header, will ignore it");
		rom.erase(rom.begin(), rom.begin()+64);
	}
	fsz -= 64;
	if( fsz > 128*1024 ) 
	{
		cart_mask = 0x3ff;
		cart_shift = 10;
	} else if( fsz > 64*1024 ) {
		cart_mask = 0x1ff;
		cart_shift = 9;
	} else {
		cart_mask = 0xff;
		cart_shift = 8;
	}
	return true;
}


