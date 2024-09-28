#include <SDL.h>
#include <thread>
#include "sms.h"
#include "Settings.h"
extern console* sys;
extern SDL_AudioDeviceID audio_dev;

#define MODE224 ( (vdp_regs[0]&BIT(2)) && (vdp_regs[1]&BIT(4)) )

u8 getCont1()
{
	auto keys = SDL_GetKeyboardState(nullptr);
	u8 val = 0xff;
	if( keys[SDL_SCANCODE_UP] ) val ^= 1;
	if( keys[SDL_SCANCODE_DOWN] ) val ^= 2;
	if( keys[SDL_SCANCODE_LEFT] ) val ^= 4;
	if( keys[SDL_SCANCODE_RIGHT] ) val ^= 8;
	if( keys[SDL_SCANCODE_Z] ) val ^= 0x10;
	if( keys[SDL_SCANCODE_X] ) val ^= 0x20;
	return val;
}

void SMS::vdp_ctrl(u8 val)
{
	if( vdp_ctrl_latch )
	{
		vdp_addr = (val<<8)|(vdp_addr&0xff);
		vdp_cd = vdp_addr>>14;
		vdp_addr &= 0x3fff;
		if( vdp_cd == 2 ) 
		{
			vdp_regs[val&0xf] = vdp_addr;
			//printf("vdp reg %i = $%X\n", val&0xf, vdp_addr&0xff);
		} else if( vdp_cd == 3 ) {
			rdbuf = cram[vdp_addr&0x1f];
		} else if( vdp_cd == 1 || vdp_cd == 0 ) {
			rdbuf = vram[vdp_addr&0x3fff];
		}
	} else {
		vdp_addr = (vdp_addr&0xff00) | val;			
	}
	vdp_ctrl_latch = !vdp_ctrl_latch;	
}

void SMS::vdp_data(u8 val)
{
	vdp_ctrl_latch = false;
	rdbuf = val;
	if( vdp_cd == 3 )
	{
		cram[vdp_addr&0x1f] = val;
		vdp_addr++;
	} else if( vdp_cd == 1 || vdp_cd == 0 ) {
		vram[vdp_addr&0x3fff] = val;	
		vdp_addr++;
	}
}

void SMS::write8(u16 addr, u8 val)
{
	if( addr >= 0xc000 )
	{
		RAM[addr&0x1fff] = val;
		if( addr >= 0xfffc )
		{
			//printf("%X = $%X\n", addr, val);
			//todo: mask val based on rom size
			bankreg[addr&3] = val;
		}
		return;
	}
}

u8 SMS::read8(u16 addr)
{
	if( addr >= 0xc000 ) return RAM[addr&0x1fff];
	if( addr < 0x400 ) return ROM[addr];
	if( addr < 0x4000 ) return ROM[(bankreg[1]*0x4000) + (addr&0x3fff)];
	if( addr < 0x8000 ) return ROM[(bankreg[2]*0x4000) + (addr&0x3fff)];
	return ROM[(bankreg[3]*0x4000) + (addr&0x3fff)];
}

void SMS::out8(u16 port, u8 val)
{
	port &= 0xff;
	if( port >= 0x80 && port < 0xC0 )
	{
		if( port & 1 )
		{
			vdp_ctrl(val);
		} else {
			vdp_data(val);
		}
		return;
	}
	if( port == 0x7F || port == 0x7E ) PSG.out(val);
	//printf("SMS out $%X = $%X\n", port, val);	
}

u8 SMS::in8(u16 port)
{
	port &= 0xff;
	//if( port != 0x7E ) printf("SMS in port $%X\n", port);
	if( port >= 0x80 && port < 0xC0 )
	{
		vdp_ctrl_latch = false;
		if( port & 1 )
		{
			u8 res = vdp_ctrl_stat;
			vdp_ctrl_stat = 0;
			//printf("VDP stat read\n");
			cpu.irq_line = 0;
			return res;
		} else {
			u8 rval = rdbuf;
			if( vdp_cd == 3 )
			{
				vdp_addr++;
				rdbuf = cram[vdp_addr&0x1f];
			} else if( vdp_cd == 1 || vdp_cd == 0 ) {
				vdp_addr++;
				rdbuf = vram[vdp_addr&0x3fff];
			}
			return rval;
		}
	}
	if( port == 0x7E || port == 0x40 || port == 0x41 ) return vcount;
	if( port == 0xDC ) return getCont1();
	return 0xff;
}

u8 sms_read(u16 a) { return ((SMS*)sys)->read8(a); }
void sms_write(u16 a, u8 v) { ((SMS*)sys)->write8(a, v); } 
u8 sms_in(u16 a) { return ((SMS*)sys)->in8(a); }
void sms_out(u16 a, u8 v) { ((SMS*)sys)->out8(a, v); } 

bool SMS::loadROM(std::string fname)
{
	FILE* fp = fopen(fname.c_str(), "rb");
	if( !fp )
	{
		return false;	
	}
	
	fseek(fp, 0, SEEK_END);
	auto fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	ROM.resize(fsize);
	[[maybe_unused]] int unu = fread(ROM.data(), 1, fsize, fp);
	fclose(fp);
	
	RAM.resize(0x2000);
	
	cpu.write = sms_write;
	cpu.read = sms_read;
	cpu.in = sms_in;
	cpu.out = sms_out;
	cpu.reset();

	return true;
}

void SMS::run_frame()
{
	for(u32 line = 0; line < 262; ++line)
	{
		set_vcount(line);	
		u64 target = last_target + 227;
		while( stamp < target )
		{
			u64 c = cpu.step();
			stamp += c;
			u8 snd = PSG.clock(c);
			snd_cycles += c;
			if( snd_cycles >= 81 )
			{
				snd_cycles -= 81;
				float sm = ((snd/60.f)*2 - 1);
				audio_add(sm,sm);
			}
		}
		last_target = target;
		if( (MODE224 && line < 224) || (line < 192) )
		{
			vdp_scanline(line);
		}
	}
	return;
}

void SMS::set_vcount(u32 line)
{
	if( MODE224 )
	{ // 224 line mode
		if( line == 224 )
		{ // signal vblank irq
			if( vdp_regs[1]&BIT(5) ) cpu.irq_line |= 1;
			vdp_ctrl_stat |= 0x80;
		}
		if( line <= 0xEA )
		{
			vcount = line;
		} else {
			vcount = (line-0xEB)+0xE5;
		}
	} else {
		// 192 line mode
		if( line == 192 )
		{ // signal vblank irq
			if( vdp_regs[1]&BIT(5) ) cpu.irq_line |= 1;
			vdp_ctrl_stat |= 0x80;
		}
		if( line <= 0xDA )
		{
			vcount = line;
		} else {
			vcount = (line-0xDB)+0xD5;
		}
	}
}

void SMS::vdp_scanline(u32 line)
{
	u32* FL = &fbuf[line*256];
	if( !(vdp_regs[1]&BIT(6)) )
	{
		memset(FL, 0, 256*4);
		return;
	}
	eval_sprites(line);

	for(u32 X = 0; X < 256; ++X)
	{
		if( X < 8 && (vdp_regs[0]&BIT(5)) )
		{
			u8 c = cram[(vdp_regs[7]&0xf)|0x10];
			u8 R = (c&3)<<6;
			u8 G = (c&12)<<4;
			u8 B = (c&0x30)<<2;
			FL[X] = (R<<24)|(G<<16)|(B<<8);
			continue;
		}

		u32 y = line;
		if( X > 63 || (vdp_regs[0] & BIT(7)) == 0 ) y = (line + vdp_regs[9]) % 223;
		u32 x = X;
		if( line > 15 || (vdp_regs[0] & BIT(6)) == 0 ) x = (X - vdp_regs[8]) & 0xff;
		int tile_x = x / 8;
		int tile_y = y / 8;
		
		const u16 map_base = ((vdp_regs[2]>>1)&7)*0x800 ; // 0x3800
		u16 map_addr = map_base + (tile_y*32 + tile_x)*2;
		u32 t_lo = vram[map_addr];
		u32 t_hi = vram[map_addr + 1];
		u32 tnum = ((t_hi<<8)&0x100) + t_lo;
		
		int line_x = (t_hi & 2) ? (x & 7) : ( 7 - (x & 7) );
		int line_y = (t_hi & 4) ? (7 - (y & 7)) : (y & 7);
		
		u16 tile_addr = ( tnum*32 + (line_y*4) );
		u8 p = (vram[tile_addr&0x3fff]>>line_x) & 1;
		p |= ((vram[(tile_addr+1)&0x3fff]>>line_x)&1) << 1;
		p |= ((vram[(tile_addr+2)&0x3fff]>>line_x)&1) << 2;
		p |= ((vram[(tile_addr+3)&0x3fff]>>line_x)&1) << 3;
		p |= (t_hi&8)<<1;
		
		for(u32 i = 0; i < num_sprites; ++i)
		{
			u32 sX = sprite_buf[i*2+1];
			if( !(X >= sX && (sX+8) > X) ) continue;
			
			u32 S = sprite_buf[i*2];
			u32 Y = vram[0x3F00 + S] + 1;
			u32 T = vram[0x3F81 + S*2];
			
			if( vdp_regs[1]&2 )
			{
				T &= 0xfe;
				if( line - Y > 7 ) T |= 1;
			}
			
			u16 tadr = (T*32 + ((line-Y)&7)*4);
			if( vdp_regs[6] & 4 ) tadr += 0x2000;
			u8 lx = (X-sX)^7;
			u8 sc = (vram[tadr&0x3fff]>>lx) & 1;
			sc |= ((vram[(tadr+1)&0x3fff]>>lx)&1) << 1;
			sc |= ((vram[(tadr+2)&0x3fff]>>lx)&1) << 2;
			sc |= ((vram[(tadr+3)&0x3fff]>>lx)&1) << 3;
			if( sc )
			{
				if( p == 0 || (t_hi & BIT(4)) == 0 ) p = sc+0x10;
			}
		}
			
		u8 c = cram[p];
		u8 R = (c&3)<<6;
		u8 G = (c&12)<<4;
		u8 B = (c&0x30)<<2;
		FL[X] = (R<<24)|(G<<16)|(B<<8);
	}
}

void SMS::eval_sprites(u32 line)
{
	num_sprites = 0;
	if( line == 0 ) return;

	for(u32 i = 0; i < 64; ++i)
	{
		u32 Y = vram[0x3F00 + i] + 1;
		if( !MODE224 && Y == 208 ) break;
		u32 h = (vdp_regs[1]&2) ? 16 : 8;
		if( line >= Y && (Y+h) > line )
		{
			sprite_buf[num_sprites*2] = i;
			sprite_buf[num_sprites*2 + 1] = vram[0x3F80 + i*2];
			num_sprites++;
			if( num_sprites == 8 ) break;
		}
	}
	return;	
}






