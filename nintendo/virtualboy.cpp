#include <bit>
#include "virtualboy.h"

static u32 sized_read(u8* data, u32 addr, int sz)
{
	if( sz == 8 ) return data[addr];
	if( sz == 16 ) return *(u16*)&data[addr&~1];
	return *(u32*)&data[addr&~3];
}

static void sized_write(u8* data, u32 addr, u32 v, int sz)
{
	if( sz == 8 ) { data[addr] = v; }
	else if( sz == 16 ) { *(u16*)&data[addr&~1] = v; }
	else { *(u32*)&data[addr&~3] = v; }
}

void virtualboy::write_miscio(u32 addr, u32 v, int sz)
{
	if( addr == 0x02000020 )
	{
		if( (v & 9) == 9 ) 
		{
			printf("Timer control write = $%X\n", v);
			//exit(1);
		}
		return;
	}
	if( addr == 0x02000028 )
	{
		if( v & 4 )
		{
			auto keys = SDL_GetKeyboardState(nullptr);
			padkeys = 2;
			if( keys[SDL_SCANCODE_Z] ) padkeys ^= BIT(2);
			if( keys[SDL_SCANCODE_X] ) padkeys ^= BIT(3);
			if( keys[SDL_SCANCODE_RIGHT] ) padkeys ^= BIT(8);
			if( keys[SDL_SCANCODE_LEFT] ) padkeys ^= BIT(9);
			if( keys[SDL_SCANCODE_DOWN] ) padkeys ^= BIT(10);
			if( keys[SDL_SCANCODE_UP] ) padkeys ^= BIT(11);
			if( keys[SDL_SCANCODE_S] ) padkeys ^= BIT(12);
			if( keys[SDL_SCANCODE_A] ) padkeys ^= BIT(13);
		}
		return;
	}
	printf("IO Write%i $%X = $%X\n", sz, addr, v);
}

u32 virtualboy::read_miscio(u32 addr, int sz)
{
	if( addr == 0x02000010 ) 
	{
		return padkeys&0xff;
	}
	if( addr == 0x02000014 ) 
	{
		return padkeys>>8;
	}
	if( addr == 0x02000028 ) return 0;
	printf("IO Read%i <$%X\n", sz, addr);
	return 0;
}

static u16 rotit = 1;

u32 virtualboy::read(u32 addr, int sz)
{
	addr &= 0x07FFFFFF;
	if( addr >= 0x07000000 )
	{
		addr -= 0x07000000;
		if( addr >= ROM.size() ) addr %= ROM.size();
		return sized_read(ROM.data(), addr, sz);
	}
	if( addr >= 0x06000000 )
	{
		printf("vb: use of cart ram <$%X\n", addr);
		//exit(1);
		return 0;
	}
	if( addr >= 0x05000000 )
	{
		return sized_read(ram, addr&0xffff, sz);	
	}
	if( addr >= 0x04000000 )
	{
		printf("vb: use of cart exp hw <$%X\n", addr);
		exit(1);
		return 0;
	}
	if( addr >= 0x03000000 )
	{
		printf("vb: use of unmapped space <$%X\n", addr);
		exit(1);
		return 0;
	}
	if( addr >= 0x02000000 )
	{
		return read_miscio(addr, sz);	
	}
	if( addr >= 0x01000000 )
	{
		return 0; //todo: sound
	}
	
	// under this line is all VIP
	addr &= 0x7FFFF;
	if( addr < 0x00040000 ) return sized_read(vram, addr, sz);
	if( addr < 0x5F800 ) return 0; // unmapped space or unused mmio
	if( addr >= 0x7E000 ) return sized_read(&vram[(addr-0x7E000) + 0x1E000], 0, sz);
	if( addr >= 0x7C000 ) return sized_read(&vram[(addr-0x7C000) + 0x16000], 0, sz);
	if( addr >= 0x7A000 ) return sized_read(&vram[(addr-0x7A000) + 0x0E000], 0, sz);
	if( addr >= 0x78000 ) return sized_read(&vram[(addr-0x78000) + 0x06000], 0, sz);
	
	if( sz == 32 )
	{
		addr &= ~3;
		u32 res = read(addr, 16);
		res |= read(addr+2, 16)<<16;
		return res;
	}
	if( sz == 8 )
	{
		u16 v = read(addr&~1, 16);
		return (v>>((addr&1)?8:0)) & 0xff;
	}
	
	printf("vb: read%i <$%X\n", sz, addr);
	
	if( addr == 0x5F800 ) return INTPND;
	if( addr == 0x5F802 ) return INTENB;
	if( addr == 0x5F820 ) 
	{
		return 0x40|DPSTTS;
	}
	if( addr == 0x5F840 ) return rotit = std::rotr(rotit, 1);
	return 0;
}

void virtualboy::write(u32 addr, u32 v, int sz)
{
	addr &= 0x7FFFFFF;
	if( addr >= 0x07000000 ) return;
	if( addr >= 0x06000000 )
	{
		printf("vb: use of cart ram $%X = $%X\n", addr, v);
		//exit(1);
		return;
	}
	if( addr >= 0x05000000 )
	{
		sized_write(ram, addr&0xffff, v, sz);
		return;
	}
	if( addr >= 0x04000000 )
	{
		printf("vb: use of cart exp hw $%X = $%X\n", addr, v);
		exit(1);
		return;
	}
	if( addr >= 0x03000000 )
	{
		printf("vb: use of unmapped space $%X = $%X\n", addr, v);
		exit(1);
		return;
	}
	if( addr >= 0x02000000 )
	{
		write_miscio(addr, v, sz);
		return;
	}
	if( addr >= 0x01000000 )
	{
		return; //todo: snd
	}
	// under this line is all VIP
	addr &= 0x7FFFF;
	if( addr < 0x00040000 ) { sized_write(vram, addr, v, sz); return; }
	if( addr < 0x5F800 ) return; // unmapped space or unused mmio
	if( addr >= 0x7E000 ) { sized_write(&vram[0x1E000], (addr-0x7E000), v, sz); return; }
	if( addr >= 0x7C000 ) { sized_write(&vram[0x16000], (addr-0x7C000), v, sz); return; }
	if( addr >= 0x7A000 ) { sized_write(&vram[0x0E000], (addr-0x7A000), v, sz); return; }
	if( addr >= 0x78000 ) { sized_write(&vram[0x06000], (addr-0x78000), v, sz); return; }
	
	printf("vb: write%i $%X = $%X\n", sz, addr, v);
	
	if( addr == 0x5F804 )
	{
		printf("INTCLR = $%X\n", v);
		INTPND &= ~v;
		return;
	}
	if( addr == 0x5F802 )
	{
		printf("INTENB = $%X\n", v);
		INTENB = v;
		return;
	}
	
	if( addr == 0x5F848 )
	{
		if( sz == 32 ) { objctrl[1] = v>>16; printf("vb: 32bit write obj ctrl0 = $%X\n", v); }
		objctrl[0] = v;
		return;
	}
	if( addr == 0x5F84A )
	{
		objctrl[1] = v;
		return;
	}
	if( addr == 0x5F84C )
	{
		if( sz == 32 ) { objctrl[3] = v>>16; printf("vb: 32bit write obj ctrl2 = $%X\n", v); }
		objctrl[2] = v;
		return;
	}
	if( addr == 0x5F84E )
	{
		objctrl[3] = v;
		return;
	}	
}

void virtualboy::step()
{
	if( INTPND & INTENB ) cpu.irq(4, 0xFE40, 0xffffFE40);
	u64 cc = cpu.step();
	stamp += cc;
}

#define FCLK 0x80
#define FRAMESTART 0x10
#define GAMESTART 8
#define XPEND 0x4000
#define LFBEND 2
#define RFBEND 4
#define DPBSY 0x3c
#define L0BSY 2
#define R0BSY 3

void virtualboy::run_frame()
{
	// 0 ms FCLK in DPSTTS goes high. 
	INTPND |= FRAMESTART | GAMESTART;
	DPSTTS |= FCLK;
	while( stamp < last_target + 60000 ) step();
	
	INTPND |= XPEND; 
	
	// 3 ms The appropriate left frame buffer begins to display.
	DPSTTS &= ~DPBSY;
	DPSTTS |= BIT(L0BSY + which_buffer);
	while( stamp < last_target + 160000 ) step();
	// 8 ms The left frame buffer finishes displaying.
	
	INTPND |= LFBEND;
	DPSTTS &= ~DPBSY;
	while( stamp < last_target + 200000 ) step();
	// 10 ms FCLK goes low. 
	DPSTTS &= ~FCLK;
	while( stamp < last_target + 260000 ) step();
	// 13 ms The appropriate right frame buffer begins to display. 
	DPSTTS &= ~DPBSY;
	DPSTTS |= BIT(R0BSY + which_buffer);
	while( stamp < last_target + 360000 ) step();
	// 18 ms The right frame buffer finishes displaying. 
	
	INTPND |= RFBEND;
	DPSTTS &= ~DPBSY;
	while( stamp < last_target + 400000 ) step();
	// 20 ms frame complete	
	last_target += 400000;
	
	for(u32 x = 0; x < 384; ++x)
	{
		for(u32 y = 0; y < 224; y += 4)
		{
			u8 L = vram[0 + ((which_buffer)?0x8000:0) + x*(256/4) + (y/4)];
			u8 R = vram[0x10000 + ((which_buffer)?0x8000:0) + x*(256/4) + (y/4)];
			fbuf[y*384 + x] = (L&3)<<30;
			fbuf[(y+1)*384 + x] = (L&0xc)<<28;
			fbuf[(y+2)*384 + x] = (L&0x30)<<26;
			fbuf[(y+3)*384 + x] = (L&0xc0)<<24;
			fbuf[y*384 + x] |= (R&3)<<14;
			fbuf[(y+1)*384 + x] |= (R&0xc)<<12;
			fbuf[(y+2)*384 + x] |= (R&0x30)<<10;
			fbuf[(y+3)*384 + x] |= (R&0xc0)<<8;
		}
	}
	
	which_buffer ^= 2;
	
	u32 objgroup = 3;
	
	for(u32 i = 0; i < 0x6000; ++i) vram[0 + ((which_buffer)?0x8000:0) + i] = vram[0x10000 + ((which_buffer)?0x8000:0) + i] = 0;
	
	for(int world = 31; world >= 0; world-=1)
	{
		u16 *attr = (u16*)&vram[0x3D800 + world*0x20];
		if( attr[0] & BIT(6) ) break;
		if( (attr[0]&0xc000) == 0 ) continue;
		if( ((attr[0]>>12)&3) == 3 ) 
		{ 
			printf("world %i is sprites\n", world); 
			draw_obj_group(objgroup); 
			objgroup = (objgroup-1)&3; 
			continue; 
		}
		if( ((attr[0]>>12)&3) == 0 )
		{
			draw_normal_bg(attr);
			continue;		
		}
		
	}
}

void virtualboy::draw_normal_bg(u16* attr)
{
	u16* map =(u16*) &vram[0x20000 + 0x2000*(attr[0]&15)];
	
	int GX = attr[1] & 0x3ff;
	int GP = attr[2] & 0x3ff;
	int GY = attr[3];
	
	for(int y = 0; y < 224; ++y)
	{
		for(int x = 0; x < 384; ++x)
		{
			const int mapx = x/8;
			const int mapy = y/8;
			u16 map_entry = map[mapy*64 + mapx];
			u16 JCA = map_entry & 0x7ff;
			u16* td;
			if( JCA < 512 ) td =(u16*) &vram[0x6000 + JCA*16];
			else if( JCA < 1024 ) td =(u16*) &vram[0xE000 + JCA*16];
			else if( JCA < 1024+512 ) td =(u16*) &vram[0x16000 + JCA*16];
			else td =(u16*) &vram[0x1E000 + JCA*16];
			
			u8 X = x&7;
			u8 Y = y&7;
			if( map_entry & BIT(13) ) X ^= 7;
			if( map_entry & BIT(12) ) Y ^= 7;
			
			u16 px = (td[Y]>>((X&7)*2))&3;
			if( !px ) continue;
			if( attr[0]&0x8000 ) set_fb_pixel(0, (GX-GP)+x, GY+y, px);
			if( attr[0]&0x4000 ) set_fb_pixel(1, (GX+GP)+x, GY+y, px);
		}
	}
}

void virtualboy::draw_obj_group(u32 group)
{
	u32 start_index = 0;
	if( group != 0 ) start_index = (objctrl[group-1] + 1) & 0x3ff;
	u32 end_index = objctrl[group] & 0x3ff;
	
	for(u32 i = end_index; i != start_index; i = (i-1)&0x3ff) // is start_index drawn? should this be i != (start_index+1)&0x3ff?
	{
		draw_sprite(i);
	}
}

void virtualboy::draw_sprite(u32 index)
{
	u16* attr = (u16*)&vram[0x3E000 + index*8];
	if( (attr[1]&0xc000) == 0 ) return;
	printf("about to draw sprite $%X\n", index);
	
	u32 JCA = attr[3] & 0x7ff;
	u16* td;
	if( JCA < 512 ) td =(u16*) &vram[0x6000 + JCA*16];
	else if( JCA < 1024 ) td =(u16*) &vram[0xE000 + JCA*16];
	else if( JCA < 1024+512 ) td =(u16*) &vram[0x16000 + JCA*16];
	else td =(u16*) &vram[0x1E000 + JCA*16];
	
	int JP = attr[1] & 0x3ff;
	int JX = attr[0] & 0x3ff;
	int JY = attr[2] & 0xff;
	
	for(u32 y = 0; y < 8 && JY+y < 224; ++y)
	{
		for(u32 x = 0; x < 8; ++x)
		{
			u8 Y = y;
			u8 X = x;
			if( attr[3] & BIT(12) ) Y ^= 7;
			if( attr[3] & BIT(13) ) X ^= 7;
		
			u32 px = (td[Y] >> (X*2))&3;
			if( !px ) continue;
			if( attr[1]&0x8000 )
			{ // left
				set_fb_pixel(0, (JX-JP)+x, JY+y, px);			
			}
			if( attr[1]&0x4000 )
			{ // right
				set_fb_pixel(1, (JX+JP)+x, JY+y, px);	
			}
		}		
	}
}

void virtualboy::set_fb_pixel(u32 lr, int x, int y, u32 p)
{
	if( x < 0 || x > 383 ) return;
	if( y < 0 || y > 223 ) return;
	u16* fb = (u16*) &vram[(lr ? 0x10000 : 0) + (which_buffer ? 0x8000 : 0)];
	fb[x*(256/8) + (y/8)] &= ~(3<<((y&7)*2));
	fb[x*(256/8) + (y/8)] |= (p<<((y&7)*2));
}

bool virtualboy::loadROM(const std::string fname)
{
	FILE* fp = fopen(fname.c_str(), "rb");
	if( !fp ) return false;
	
	fseek(fp, 0, SEEK_END);
	auto fsz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	ROM.resize(fsz);
	[[maybe_unused]] int u = fread(ROM.data(), 1, fsz, fp);
	fclose(fp);
	return true;
}

void virtualboy::reset()
{
	cpu.read = [&](u32 addr, int sz) { return read(addr,sz); };
	cpu.write= [&](u32 addr, u32 v, int sz) { write(addr,v,sz); };
	cpu.reset();
	stamp = last_target = 0;
	INTPND = INTENB = 0;
	DPSTTS = 0;
	which_buffer = 0;
	for(u32 i = 0; i < 256*1024; ++i) vram[i] = 0;
}

virtualboy::~virtualboy()
{
	FILE* fp = fopen("vram.bin", "wb");
	if( !fp ) return;
	[[maybe_unused]] int u = fwrite(vram+0x6000, 1, 0x2000, fp);
	u = fwrite(vram+0xE000, 1, 0x2000, fp);
	u = fwrite(vram+0x16000,1, 0x2000, fp);
	u = fwrite(vram+0x1E000,1, 0x2000, fp);
	fclose(fp);
}

