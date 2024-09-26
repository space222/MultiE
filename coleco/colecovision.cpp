#include <functional>
#include <vector>
#include "Settings.h"
#include "colecovision.h"
#include "z80.h"
extern console* sys;
	
u8 colecovision::cv_read(u16 addr)
{
	//printf("CV: read $%X\n", addr);
	if( addr >= 0x8000 ) return rom[addr&0x7fff];
	if( addr >= 0x6000 ) return ram[addr&0x3ff];
	if( addr < 0x2000 ) return bios[addr];
	return 0;
}

void colecovision::cv_write(u16 addr, u8 v)
{
	if( addr >= 0x8000 ) return;
	if( addr >= 0x6000 ) { ram[addr&0x3ff] = v; return; }
	return;
}

u8 colecovision::cv_in(u16 p)
{
	p &= 0xff;
	if( p == 0xbf ) 
	{
		vdp.vdp_ctrl_latch = false; 
		vdp.vdp_ctrl_stat &= ~0x80;
		//cpu.irq_line = 0;
		return vdp.vdp_ctrl_stat; 
	}
	if( p == 0xbe )
	{
		u8 val = vdp.rdbuf;
		vdp.rdbuf = vdp.vram[vdp.vdp_addr&0x3fff];
		vdp.vdp_addr+=1;	
		return val;
	}
	if( p == 0xfc )
	{
		auto keys = SDL_GetKeyboardState(nullptr);
		u8 v = 0xff;
		if( keys[SDL_SCANCODE_S] ) v ^= 1;
		return v;	
	}
	if( p == 0xff ) { return 0xff; }
	
	printf("CV: port read $%X\n", p);
	return 0xff;
}

void colecovision::cv_out(u16 p, u8 v)
{
	p &= 0xff;
	if( p == 0xff ) { psg.out(v); return; }
	if( p == 0xbf )
	{
		vdp.ctrl(v);
		return;
	}
	if( p == 0xbe )
	{
		vdp.data(v);
		return;
	}
	
	printf("CV: port $%X = $%X\n", p, v);
}

void colecovision::reset()
{
	cpu.read = [](u16 a)->u8 { return dynamic_cast<colecovision*>(sys)->cv_read(a); };
	cpu.write= [](u16 a, u8 v) { dynamic_cast<colecovision*>(sys)->cv_write(a,v); };
	cpu.in = [](u16 p) ->u8 { return dynamic_cast<colecovision*>(sys)->cv_in(p); };
	cpu.out = [](u16 p, u8 v) { dynamic_cast<colecovision*>(sys)->cv_out(p,v); };
	cpu.pc = 0;
	cpu.halted = false;
	cpu.irq_line = cpu.nmi_line = 0;
	sample_cycles = stamp = last_target = 0;
	vdp.reset();
}

void colecovision::run_frame()
{
	for(u32 line = 0; line < 262; ++line)
	{
		//set_vcount(line);	
		u64 target = last_target + 227;
		while( stamp < target )
		{
			u64 c = cpu.step();
			stamp += c;
			u8 snd = psg.clock(c);
			sample_cycles += c;
			if( sample_cycles >= 81 )
			{
				sample_cycles -= 81;
				float sm = Settings::mute ? 0 : ((snd/60.f)*2 - 1);
				audio_add(sm,sm);
			}
		}
		last_target = target;
		if( line < 192 )
		{
			vdp.draw_scanline(line);
		} else if( line == 192 ) {
			vdp.vdp_ctrl_stat |= 0x80;
			if( vdp.vdp_regs[1] & BIT(5) )
			{
				cpu.nmi_line = 1;			
			}
		}
	}
}

bool colecovision::loadROM(const std::string fname)
{
	FILE* fp = fopen("./bios/colecovision.rom", "rb");
	if( !fp )
	{
		printf("Unable to load colecovision.rom bios\n");
		return false;
	}
	[[maybe_unused]] int unu = fread(bios, 1, 0x2000, fp);
	fclose(fp);
	
	fp = fopen(fname.c_str(), "rb");
	if( !fp )
	{
		printf("Unable to load rom\n");
		return false;
	}
	
	unu = fread(rom, 1, 0x8000, fp);
	fclose(fp);
	return true;
}

//static u32 cv_palette[] = { 0, 0, 0x16c537, 0x55da70, 0x4b4aeb, 0x756dfc, 
//	0xd14743, 0x3aeaf4, 0xfc4a4a, 0xff7070, 0xd1bd4a, 0xe5cb78, 0x16ac30, 0xc550b5, 0xc8c8c8, 0xffffff };

/*void colecovision::vdp_scanline(u32 line)
{
	u32* FL = &fbuf[line*256];
	if( !(vdp_regs[1]&BIT(6)) )
	{
		memset(FL, 0, 256*4);
		return;
	}
	
	u8 M3 = (vdp_regs[0]&2);
	u8 M2 = (vdp_regs[1]&8);
	u8 M1 = (vdp_regs[1]&0x10);
	
	for(u32 X = 0; X < 256; ++X)
	{
		if( 0 ) //X < 8 && (vdp_regs[0]&BIT(5)) )
		{
			u32 c = cv_palette[(vdp_regs[7]>>4)];
			FL[X] = c<<8;
			continue;
		}

		u32 y = line;
		u32 x = X;
		int tile_x = x / 8;
		int tile_y = y / 8;
		u8 p = 0;
		
		if( M3 )
		{
			u16 map_addr = ((vdp_regs[2]&0xf)*0x400) + (tile_y*32 + tile_x);
			u8 nt = vram[map_addr];	
			u16 tile_addr = ((vdp_regs[4]&4)*0x800) + ((line/64)*256*8) + nt*8 + (y&7);
			u8 d = vram[tile_addr];
			d >>= 7^(x&7);
			d &= 1;
			u8 ct = vram[((vdp_regs[3]&0x80)*0x40) + ((line/64)*256*8) + (nt*8) + (y&7)];
			p = ( d ? (ct>>4) : (ct&0xf) );	
		} else {
			u16 map_addr = ((vdp_regs[2]&0xf)*0x400) + (tile_y*32 + tile_x);
			u8 nt = vram[map_addr];	
			u16 tile_addr = ((vdp_regs[4]&7)*0x800) + (nt*8 + (y&7));
			u8 d = vram[tile_addr];
			d >>= 7^(x&7);
			d &= 1;
			p = (d ? (vram[(vdp_regs[3]*0x40) + (nt>>3)]>>4) : (vram[(vdp_regs[3]*0x40) + (nt>>3)]&0xf));
		}
		u32 c = cv_palette[p];
		FL[X] = c<<8;
	}
}

void colecovision::eval_sprites(u32 line)
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

*/

