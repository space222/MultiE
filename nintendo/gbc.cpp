#include "gbc.h"
extern console* sys;

void gbc::mapper_write(u16 addr, u8 v)
{
	if( mapper == 0 )
	{
		printf("Mapper 0: Attempted ROM write ignored\n");
		return;
	}
	if( mapper == 1 )
	{
		if( addr >= 0x2000 && addr <= 0x3FFF )
		{
			v &= 0x1f;
			if( v == 0 ) v = 1;
			prg_bank = v;
			return;
		}
		return;
	}
	if( mapper == 2 )
	{
		if( addr >= 0x4000 ) return;
		if( addr & BIT(8) )
		{
			prg_bank = v&0xf;
			if( prg_bank == 0 ) prg_bank = 1;		
		} else {
			mbc_ram_en = v;
		}	
		return;
	}
	if( mapper == 3 )
	{
		if( addr < 0x2000 )
		{
			mbc_ram_en = v&0xf;
			return;
		}
		if( addr >= 0x2000 && addr <= 0x3FFF )
		{
			v &= 0x7f;
			if( v == 0 ) v = 1;
			prg_bank = v;
			if( (v*0x4000) >= ROM.size() ) { printf("v = $%X\n", v); exit(1); }
			return;
		}
		if( addr >= 0x4000 && addr <= 0x5FFF )
		{
			if( v < 4 ) eram_bank = v & 0xf;
			return;
		}
	}
	if( mapper == 5 )
	{
		if( addr < 0x2000 )
		{
			mbc_ram_en = v&0xf;
			return;
		}
		if( addr >= 0x2000 && addr <= 0x2FFF )
		{
			prg_bank = (prg_bank&0xff00) | v;
			return;
		}
		if( addr >= 0x3000 && addr <= 0x3FFF )
		{
			prg_bank = (prg_bank&0xff) | ((v&1)<<8);
			return;
		}
		if( addr >= 0x4000 && addr <= 0x5FFF )
		{
			eram_bank = v&0xf;
			return;
		}
	}
}

void gbc::io_write(u8 port, u8 v)
{
	if( port == 0x44 ) return;
	if( port == 4 )
	{
		div = 0;
		return;	
	}
	/*if( port == 0x45 )
	{
		io[port] = v;
		if( (io[0x41] & BIT(6)) && io[0x45]==io[0x44] ) io[0xf] |= 2;
		return;
	}*/
	if( port == 0x00 )
	{
		io[port] &= ~0xf0;
		io[port] |= v & 0x30;
		return;
	}
	if( port == 0x46 )
	{
		io[port] = v;
		for(u32 i = 0; i < 0xa0; ++i)
		{
			oam[i] = read((v<<8)|i);
		}
		return;
	}
	if( port >= 0x10 && port <= 0x3F ) { apu_write(port, v); return; }
	if( isGBC )
	{
		if( port == 0x69 )
		{
			cbgpal[io[0x68]&0x3F] = v;
			if( io[0x68] & BIT(7) ) io[0x68] = 0x80|((io[0x68]+1)&0x3F);
			return;
		}	
		if( port == 0x6B )
		{
			cobjpal[io[0x6A]&0x3F] = v;
			if( io[0x6A] & BIT(7) ) io[0x6A] = 0x80|((io[0x6A]+1)&0x3F);
			return;
		}
		if( port == 0x4F )
		{
			io[0x4F] = 0xfe|v;
			vram_bank = v&1;
			return;
		}
		if( port == 0x70 )
		{
			io[0x70] = v&7;
			wram_bank = v&7;
			if( wram_bank == 0 ) wram_bank = 1;
			return;
		}
		if( port == 0x4D )
		{
			printf("Program attempted gbc speed switch.\n");	
			if( v & 1 ) io[0x4D] ^= 0x80;
			io[0x4D] &= 0x80;
			return;
		}
		if( port == 0x55 )
		{
			if( !(v&0x80) )
			{
				u16 src = (io[0x51]<<8)|io[0x52];
				u16 dst = (io[0x53]<<8)|io[0x54];
				src &= ~0xf;
				dst &= ~0xf;
				u16 len = (v+1)<<4;
				printf("dma $%X to $%X, $%X bytes\n", src, dst, len);
				for(u32 i = 0; i < len; ++i)
				{
					vram[(vram_bank*0x2000) + (dst&0x1fff)] = read(src);
					dst += 1;
					src += 1;
				}			
			}
		
			return;
		}
	}
	io[port] = v;
	//printf("GB(C): IO Wr $%X = $%X\n", port, v);
}

u8 gbc::io_read(u8 port)
{
	switch( port )
	{
	// JOYP $00
	case 0x00:{ 
		u8 v = 0xf;
		auto keys = SDL_GetKeyboardState(nullptr);
		if( !(io[0] & BIT(5)) )
		{
			if( keys[SDL_SCANCODE_Z] ) v ^= 1;
			if( keys[SDL_SCANCODE_X] ) v ^= 2;
			if( keys[SDL_SCANCODE_A] ) v ^= 4;
			if( keys[SDL_SCANCODE_S] ) v ^= 8;
		} else if( !(io[0] & BIT(4)) ) {
			if( keys[SDL_SCANCODE_RIGHT] ) v ^= 1;
			if( keys[SDL_SCANCODE_LEFT] ) v ^= 2;
			if( keys[SDL_SCANCODE_UP] ) v ^= 4;
			if( keys[SDL_SCANCODE_DOWN] ) v ^= 8;
		}
		return (io[0]&~15)|v;	
		}
	// LCD_STAT $41
	case 0x41: return (io[0x41]&~7)|lcd_mode|(io[0x44]==io[0x45]?4:0);
	case 0x44: return io[0x44];
	case 0x04: return div>>8;
	
	// NR52 main audio control
	case 0x26: return (io[0x26]&0x80)|(chan[0].active?1:0)|(chan[1].active?2:0)|(chan[2].active?4:0)|(chan[3].active?8:0);
	default: break;
	}
	
	if( isGBC )
	{
		if( port == 0x69 )
		{
			return cbgpal[io[0x68]&0x3f];
		}
		if( port == 0x6B )
		{
			return cobjpal[io[0x6A]&0x3f];
		}
		if( port == 0x55 ) return 0xff;
		if( port == 0x70 ) return io[0x70];
		//if( port == 0x4F ) return 0xfe|vram_bank;
		if( !(port >= 0x10 && port <= 0x40) ) printf("GB(C): IO Rd $%X\n", port);
	}
	return io[port];
}

void gbc::run_frame()
{
	win_started = false;
	win_line = 0;
	u64 target = last_target;
	if( !(io[0x40]&0x80) )
	{
		//lcd disabled
		lcd_mode = 0;
		target += (154*456);
		while( cpu.stamp < target ) timer_inc(cpu.step());
		last_target = target;
		memset(fbuf, 0, 160*144*4);
		return;
	}


	for(u32 line = 0; line < 144; ++line)
	{
		io[0x44] = line;
		if( line == io[0x4A] ) win_started = true;
		if( (io[0x41] & BIT(6)) && io[0x45]==io[0x44] ) io[0xf] |= 2;
		
		lcd_mode = 0;
		if( (io[0x41] & BIT(3)) ) io[0xf] |= 2;
		target += (176);
		while( cpu.stamp < target ) timer_inc(cpu.step());
		
		lcd_mode = 2;
		if( (io[0x41] & BIT(5)) ) io[0xf] |= 2;
		target += (80);
		while( cpu.stamp < target ) timer_inc(cpu.step());
		
		draw_scanline(line);
			
		lcd_mode = 3;
		target += (200);
		while( cpu.stamp < target ) timer_inc(cpu.step());
		
		if( win_started ) win_line += 1;
	}
	
	if( io[0x40] & 0x80 ) io[0xf] |= 1; // vblank irq requested
	
	// vblank is a bit more streamlined
	lcd_mode = 1;
	if( (io[0x41] & BIT(4)) ) io[0xf] |= 2;
	for(u32 line = 144; line < 154; ++line)
	{
		io[0x44] = line;
		if( (io[0x41] & BIT(6)) && io[0x45]==io[0x44] ) io[0xf] |= 2;
		
		target += (456); //todo: *2 in double speed mode
		while( cpu.stamp < target ) timer_inc(cpu.step());
	}
	last_target = target;
}

#define TCTRL io[7]
#define TMA   io[6]
#define TCOUNT io[5]
static int tcycle_counts[] = { 1024, 16, 64, 256 };

void gbc::timer_inc(u64 c)
{
	for(u64 i = 0; i < c; ++i) apu_tick();
	u16 divb4 = div;
	div += c;
	if( (divb4&BIT(12)) && !(div&BIT(12)) ) div_apu_tick();
	if( TCTRL & 4 )
	{
		timer_cycle_count += c;
		if( timer_cycle_count >= tcycle_counts[TCTRL&3] )
		{
			timer_cycle_count -= tcycle_counts[TCTRL&3];
			TCOUNT++;
			if( TCOUNT == 0 )
			{
				TCOUNT = TMA;
				io[0xf] |= 4;
			}
		}
	}
}

void gbc::write(u16 addr, u8 v)
{
	if( addr == 0xffff ) { io[0xff] = v & 0x1F; return; }
	if( addr >= 0xff80 ) { hram[addr&0x7f] = v; return; }
	if( addr >= 0xff00 ) { io_write(addr&0xff, v); return; }
	if( addr >= 0xfea0 ) return;
	if( addr >= 0xfe00 ) { oam[addr&0xff] = v; return; }
	if( addr >= 0xe000 ) { write(0xc000 | (addr&0x1fff), v); return; }
	if( addr >= 0xd000 ) { wram[wram_bank*0x1000 + (addr&0xfff)] = v; return; }
	if( addr >= 0xc000 ) { wram[addr&0xfff] = v; return; }
	if( addr >= 0xa000 ) 
	{
		//if( mapper == 3 && eram_bank > 3 ) return;
		if( mapper == 3 && mbc_ram_en != 10 ) return;
		if( mapper == 2 )
		{
			if( mbc_ram_en == 10 )
			{
				eram[addr&0x1ff] = v&0xf;
			}		
			return;
		}
		eram_bank&=3;
		eram[eram_bank*0x2000 + (addr&0x1fff)] = v;
		return; 
	}
	if( addr >= 0x8000 ) { vram[vram_bank*0x2000 + (addr&0x1fff)] = v; return; }
	mapper_write(addr, v);
	//printf("GB(C):$%X: Write $%X = $%X\n", cpu.PC, addr, v);
}

u8 gbc::read(u16 addr)
{
	if( addr == 0xffff ) return io[0xff];
	if( addr >= 0xff80 ) return hram[addr&0x7f];
	if( addr >= 0xff00 ) return io_read(addr&0xff);
	if( addr >= 0xfea0 ) return ((addr>>4)&0xf)|(addr&0xf0);
	if( addr >= 0xfe00 ) return oam[addr&0xff];
	if( addr >= 0xe000 ) return read(0xc000 | (addr&0x1fff));
	if( addr >= 0xd000 ) return wram[wram_bank*0x1000 + (addr&0xfff)];
	if( addr >= 0xc000 ) return wram[addr&0xfff];
	if( addr >= 0xa000 ) 
	{ 
		//if( mapper == 3 && eram_bank > 3 ) return 0; 
		if( mapper == 3 && mbc_ram_en != 10 ) return 0;
		if( mapper == 2 )
		{
			return ( mbc_ram_en == 10 ) ? eram[addr&0x1ff] : 0;
		}
		eram_bank &= 3;
		return eram[eram_bank*0x2000 + (addr&0x1fff)]; 
	}
	if( addr >= 0x8000 ) return vram[vram_bank*0x2000 + (addr&0x1fff)];

	if( addr >= 0x4000 ) return ROM[prg_bank*0x4000 + (addr&0x3fff)];
	return ROM[addr];
}

static u32 gbpal[] = { 0xFFFFFF00, 0xC0C0C000, 0x60606000, 0x20202000 };

void gbc::draw_scanline(u32 line)
{
	if( isGBC ) { draw_color_scanline(line); return; }

	u8 scy = io[0x42];
	u8 scx = io[0x43];
	u8 winx = io[0x4B];
	if( winx > 7 ) 
	{
		winx -= 7;
	} else {
		winx = 0; // cheap hack for zelda
	}
	
	u8 ids[10];
	u32 num = 0;
	if( io[0x40] & 2 )
	{
		for(u32 i = 0; i < 40 && num < 10; ++i)
		{
			u8 height = (io[0x40]&4)?16:8;
			if( int(line) >= (int(oam[i*4])-16) && line - (int(oam[i*4])-16) < height )
			{
				ids[num++] = i;
			}
		}
	}
		
	u8 sprbuf[160];
	memset(sprbuf, 0, 160);
	for(u32 spr = 0; spr < num; ++spr)
	{
		int x = (int)(oam[ids[spr]*4 + 1]) - 8;
		int y = (int)(oam[ids[spr]*4]) - 16;
		int loff = line;
		loff -= y;
		u8 tile = oam[ids[spr]*4 + 2];
		u8 attr = oam[ids[spr]*4 + 3];

		if( attr&BIT(6) )
		{
			if( io[0x40]&4 )
			{
				tile &= 0xFE;
			}
			loff ^= (io[0x40]&4)?15:7;
		}

		u16 taddr = tile*16 + (loff<<1);
		for(int p = x, i = 0; p < x+8 && p < 160; ++p, ++i)
		{
			if( p < 0 || sprbuf[p] ) continue;
			sprbuf[p] = (vram[taddr]>>(((attr&BIT(5))?0:7)^(i&7)))&1;
			sprbuf[p] |= ((vram[taddr+1]>>(((attr&BIT(5))?0:7)^(i&7)))&1)<<1;
			if( sprbuf[p] ) sprbuf[p] |= attr&0x90;
		}
	}
	
	for(u32 px = 0; px < 160; ++px)
	{
		u32 ntaddr = (io[0x40]&BIT(3))?0x1c00:0x1800;
		
		u8 ey = line+scy;
		u8 ex = px+scx;
		
		u8 tile = 0; //vram[ntaddr + (ey/8)*32 + (ex/8)];
		//u16 taddr_base = 0; //(io[0x40]&BIT(4)) ? (0 + tile*16) : (0x1000 + (s8(tile)*16));
		
		if( (io[0x40]&BIT(5)) && win_started && px >= winx )
		{
			ey = win_line;
			ex = px-winx;
			tile = vram[((io[0x40]&BIT(6))?0x1c00:0x1800) + (ey/8)*32 + (ex/8)];
		} else {
			tile = vram[ntaddr + (ey/8)*32 + (ex/8)];
		}
		u16 taddr_base = (io[0x40]&BIT(4)) ? (0 + tile*16) : (0x1000 + (s8(tile)*16));
		
		u8 b0 = vram[taddr_base + ((ey&7)<<1)] >> (7-(ex&7));
		b0 &= 1;
		u8 b1 = vram[taddr_base + ((ey&7)<<1) + 1] >> (7-(ex&7));
		b1 &= 1;
		b0 |= b1<<1;
		
		if( !(sprbuf[px]&0x80) && sprbuf[px] )
		{
			fbuf[line*160 + px] = gbpal[(io[0x48+((sprbuf[px]&BIT(4))?1:0)]>>((sprbuf[px]&3)*2))&3];
		} else if( (io[0x40]&1) && b0 ) {
			fbuf[line*160 + px] = gbpal[(io[0x47]>>(b0*2))&3];
		} else if( sprbuf[px]&3 ) {
			fbuf[line*160 + px] = gbpal[(io[0x48+((sprbuf[px]&BIT(4))?1:0)]>>((sprbuf[px]&3)*2))&3];
		} else {
			fbuf[line*160 + px] = gbpal[(io[0x47]&3)];
		}
	}
}

void gbc::reset()
{
	lcd_mode = 2;
	cpu.write = [](u16 a, u8 v) { dynamic_cast<gbc*>(sys)->write(a,v); };
	cpu.read = [](u16 a)-> u8 { return dynamic_cast<gbc*>(sys)->read(a); };
	cpu.reset();
	if( isGBC ) cpu.r.b[6] = 0x11;
	cpu.stamp = last_target = 0;
	cpu.halted = false;
	cpu.ime = false;
	io[0x50] = 0;
	io[0x40] = 0x80;
	io[0xff] = io[0xf] = 0;
	io[0] = 0xff;
	
	prg_bank = 1;
	wram_bank = 1;
	vram_bank = 0;
	eram_bank = 0;
	div = 0;
	timer_cycle_count = 0;
	apu_cycles_to_sample = 0;
	chan[0].active = chan[1].active = chan[2].active = chan[3].active = false;
}

bool gbc::loadROM(const std::string fname)
{
	FILE* fp = fopen(fname.c_str(), "rb");
	if( !fp ) return false;
	
	fseek(fp, 0, SEEK_END);
	auto fsz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	ROM.resize(fsz);
	[[maybe_unused]] int unu = fread(ROM.data(), 1, fsz, fp);
	fclose(fp);
	
	cartram_file = fname;
	if(auto p = fname.rfind('.'); p != std::string::npos ) 
	{
		cartram_file.erase(p);
	}
	cartram_file += ".ram";
	//printf("ram file = <%s>\n", cartram_file.c_str());
	fp = fopen(cartram_file.c_str(), "rb");
	if( fp )
	{  //todo: tell user about any fail
		unu = fread(eram, 1, 32*1024, fp);
		fclose(fp);
	}	
	
	switch( ROM[0x147] )
	{
	case 0: 
	case 8:
	case 9:
		mapper = 0; break;
	case 1: 
	case 2:
	case 3: mapper = 1; break;
	
	case 5:
	case 6: mapper = 2; break;
	case 0x0F:
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13: mapper = 3; break;
	case 0x19:
	case 0x1A:
	case 0x1B:
	case 0x1C:
	case 0x1D:
	case 0x1E: mapper = 5; break;
	//case 0x22: mapper = 7; break;
	case 0xFE: mapper = 0xc3; break;
	case 0xFF: mapper = 0xc1; break;
	default:
		printf("GB(C): ROM Type $%X, not supported\n", ROM[0x147]);
		return false;
	}
	
	isGBC = ((ROM[0x143]&0x80)==0x80);
	if( isGBC )
	{
		printf("GB(C): in color mode\n");
	} else {
		printf("GB(C): in monochrome mode\n");
	}

	return true;
}

gbc::~gbc()
{
	FILE* fp = fopen(cartram_file.c_str(), "wb");
	if( ! fp ) return;
	[[maybe_unused]] int unu = fwrite(eram, 1, 32*1024, fp);
	fclose(fp);
}

u32 color555to32(u16 c)
{
	u8 R = c & 0x1f;
	u8 G = (c>>5) & 0x1f;
	u8 B = (c>>10) & 0x1f;
	R <<= 3;
	G <<= 3;
	B <<= 3;
	return (R<<24)|(G<<16)|(B<<8);
}

void gbc::draw_color_scanline(u32 line)
{
	u8 scy = io[0x42];
	u8 scx = io[0x43];
	u8 winx = io[0x4B];
	if( winx > 7 ) 
	{
		winx -= 7;
	} else {
		winx = 0; // cheap hack for zelda
	}
	
	u8 ids[10];
	u32 num = 0;
	if( io[0x40] & 2 )
	{
		for(u32 i = 0; i < 40 && num < 10; ++i)
		{
			u8 height = (io[0x40]&4)?16:8;
			if( int(line) >= (int(oam[i*4])-16) && line - (int(oam[i*4])-16) < height )
			{
				ids[num++] = i;
			}
		}
	}
		
	u8 sprbuf[160];
	memset(sprbuf, 0, 160);
	for(u32 spr = 0; spr < num; ++spr)
	{
		int x = (int)(oam[ids[spr]*4 + 1]) - 8;
		int y = (int)(oam[ids[spr]*4]) - 16;
		int loff = line;
		loff -= y;
		u8 tile = oam[ids[spr]*4 + 2];
		u8 attr = oam[ids[spr]*4 + 3];

		if( attr&BIT(6) )
		{
			if( io[0x40]&4 )
			{
				tile &= 0xFE;
			}
			loff ^= (io[0x40]&4)?15:7;
		}

		u16 taddr = ((attr&BIT(3))?0x2000:0) + tile*16 + (loff<<1);
		for(int p = x, i = 0; p < x+8 && p < 160; ++p, ++i)
		{
			if( p < 0 || sprbuf[p] ) continue;
			sprbuf[p] = (vram[taddr]>>(((attr&BIT(5))?0:7)^(i&7)))&1;
			sprbuf[p] |= ((vram[taddr+1]>>(((attr&BIT(5))?0:7)^(i&7)))&1)<<1;
			if( sprbuf[p] ) { sprbuf[p] |= (attr&0x80)|((attr&7)<<2); }
		}
	}
	
	for(u32 px = 0; px < 160; ++px)
	{
		u32 ntaddr = (io[0x40]&BIT(3))?0x1c00:0x1800;
		
		u8 ey = line+scy;
		u8 ex = px+scx;
		
		if( (io[0x40]&BIT(5)) && win_started && px >= winx )
		{
			ey = win_line;
			ex = px-winx;
			ntaddr = ((io[0x40]&BIT(6))?0x1c00:0x1800);
		}
		
		u16 tile = vram[ntaddr + (ey/8)*32 + (ex/8)];		
		u8 attr = vram[0x2000 + ntaddr + (ey/8)*32 + (ex/8)]; 
		u16 taddr_base = (io[0x40]&BIT(3)) ? (0 + tile*16) : (0x1000 + (s8(tile)*16));
		
		u8 b0 = vram[taddr_base + ((attr&BIT(3)) ? 0x2000 : 0) + ((ey&7)<<1)] >> (7-(ex&7));
		b0 &= 1;
		u8 b1 = vram[taddr_base + ((attr&BIT(3)) ? 0x2000 : 0) + ((ey&7)<<1) + 1] >> (7-(ex&7));
		b1 &= 1;
		b0 |= b1<<1;
		
		if( !(sprbuf[px]&0x80) && sprbuf[px] )
		{
			u16 c = *(u16*)&cobjpal[(sprbuf[px]&0x1f)<<1];
			fbuf[line*160 + px] = color555to32(c);
		} else if( b0 ) {
			u16 c = *(u16*)&cbgpal[(((attr&7)<<2)|b0)<<1];
			fbuf[line*160 + px] = color555to32(c);
		} else if( sprbuf[px]&3 ) {
			u16 c = *(u16*)&cobjpal[(sprbuf[px]&0x1f)<<1];
			fbuf[line*160 + px] = color555to32(c);
		} else {
			u16 c = *(u16*)&cbgpal[(((attr&7)<<2)|b0)<<1];
			fbuf[line*160 + px] = color555to32(c);
		}
	}
}


