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
		return 0; //sound reads undefined??
	}
	
	// under this line is all VIP
	addr &= 0x7FFFF;
	if( addr < 0x00040000 ) return sized_read(vram, addr, sz);
	if( addr < 0x5F800 ) return 0; // unmapped space or unused mmio
	if( addr >= 0x7E000 ) return sized_read(&vram[0x1E000], (addr-0x7E000), sz);
	if( addr >= 0x7C000 ) return sized_read(&vram[0x16000], (addr-0x7C000), sz);
	if( addr >= 0x7A000 ) return sized_read(&vram[0x0E000], (addr-0x7A000), sz);
	if( addr >= 0x78000 ) return sized_read(&vram[0x06000], (addr-0x78000), sz);
	
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
	
	if( addr == 0x5F848 )
	{
		return objctrl[0];
	}
	if( addr == 0x5F84A )
	{
		return objctrl[1];
	}
	if( addr == 0x5F84C )
	{
		return objctrl[2];
	}
	if( addr == 0x5F84E )
	{
		return objctrl[3];
	}
	if( addr == 0x5F844 ) return 2;
	
	if( addr == 0x5F860 ) return bgpal[0];
	if( addr == 0x5F862 ) return bgpal[1];
	if( addr == 0x5F864 ) return bgpal[2];
	if( addr == 0x5F866 ) return bgpal[3];
	if( addr == 0x5F868 ) return objpal[0];
	if( addr == 0x5F86A ) return objpal[1];
	if( addr == 0x5F86C ) return objpal[2];
	if( addr == 0x5F86E ) return objpal[3];
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
		addr &= 0x7ff;
		if( addr < 0x280 ) { wavram[addr>>2] = v & 0x3F; return; }
		if( addr < 0x300 ) { modram[(addr-0x280)>>2] = v; return; }
		if( addr < 0x400 ) return; // unused space
		if( addr == 0x580 )
		{	// the shut-sound-off port
			if( v & 1 )
			{
				for(u32 i = 0; i < 6; ++i) channel[i][0] = 0;
			}
			return;
		}
		const u32 C = (addr>>6)&7;
		const u32 R = (addr>>2)&15;
		if( C > 5 || (R & 8) ) return;
		channel[C][R] = v;
		if( R == 0 )
		{
			chanpos[C] = 0;
			chancycles[C] = 0;
			chaninterval[C] = (v&0x1F)+1;
			chan_int_cycles[C] = 0;
		}
		return;
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
	
	if( sz == 32 )
	{
		addr &= ~3;
		write(addr, v&0xffff, 16);
		write(addr+2, v>>16, 16);
		return;
	}
	
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
		objctrl[2] = v;
		return;
	}
	if( addr == 0x5F84E )
	{
		objctrl[3] = v;
		return;
	}
	
	if( addr == 0x5F860 ) { bgpal[0] = v; return; }
	if( addr == 0x5F862 ) { bgpal[1] = v; return; }
	if( addr == 0x5F864 ) { bgpal[2] = v; return; }
	if( addr == 0x5F866 ) { bgpal[3] = v; return; }
	if( addr == 0x5F868 ) { objpal[0] = v; return; }
	if( addr == 0x5F86A ) { objpal[1] = v; return; }
	if( addr == 0x5F86C ) { objpal[2] = v; return; }
	if( addr == 0x5F86E ) { objpal[3] = v; return; }

	if( addr == 0x5F822 )
	{
		if( v & 1 )
		{
			INTPND &= ~(BIT(15)|0x1f);
			INTENB &= ~(BIT(15)|0x1f);		
		}
	
		DPSTTS &= ~(BIT(1)|BIT(8)|BIT(9)|BIT(10));
		v &= (BIT(1)|BIT(8)|BIT(9)|BIT(10));
		DPSTTS |= v;
		return;
	}	
}

void virtualboy::step()
{
	if( INTPND & INTENB ) cpu.irq(4, 0xFE40, 0xffffFE40);
	u64 cc = cpu.step();
	stamp += cc;
	
	snd_clock(cc);
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
	//frame_divider += 1;
	//if( frame_divider >= 6 )
	//{
	//	frame_divider = 0;
	//	return;
	//}

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
		const u32 BGM = ((attr[0]>>12)&3);
		if( BGM == 3 ) 
		{ 
			//printf("world %i is sprites\n", world); 
			draw_obj_group(objgroup); 
			objgroup = (objgroup-1)&3; 
			continue; 
		}
		if( BGM == 0 )
		{
			draw_normal_bg(attr);
			continue;		
		}
		if( BGM == 2 )
		{
			draw_affine_bg(attr);
			continue;		
		}
	}
}
void virtualboy::draw_normal_bg(u16* attr)
{
	u16* map =(u16*) &vram[0x20000 + 0x2000*(attr[0]&15)];
	
	int GX = s16(attr[1]<<6)>>6;
	int GP = s16(attr[2]<<6)>>6;
	int GY = s16(attr[3]);
	
	int MX = (s16(attr[4]<<3)>>3);
	int MY = (s16(attr[6]<<3)>>3);
	int MP = (s16(attr[5]<<1)>>1);
	
	int W = (s16(attr[7]<<3)>>3) + 1;
	int H = s16(attr[8]) + 1;
	if( H < 8 ) H = 8;
	
	for(int y = 0; y < H; ++y)
	{
		for(int x = 0; x < W; ++x)
		{	
			if( attr[0] & 0x8000 )
			{
				int X = (x + MX - MP);
				int Y = (y + MY - MP);
				const int mapx = X/8;
				const int mapy = Y/8;
				u16 map_entry =
					((mapx<0||mapx>=64||mapy<0||mapy>=64)&&(attr[0]&BIT(7))) 
					? *(u16*)&vram[0x20000+attr[10]*2] : map[(mapy&63)*64+(mapx&63)];
				u16 JCA = map_entry & 0x7ff;
				u16* td;
				if( JCA < 512 ) td =(u16*) &vram[0x6000 + JCA*16];
				else if( JCA < 1024 ) td =(u16*) &vram[0xE000 + (JCA-512)*16];
				else if( JCA < 1024+512 ) td =(u16*) &vram[0x16000 + (JCA-1024)*16];
				else td =(u16*) &vram[0x1E000 + (JCA-(1024+512))*16];
				
				X &= 7;
				Y &= 7;
				if( map_entry & BIT(13) ) X ^= 7;
				if( map_entry & BIT(12) ) Y ^= 7;
				
				u16 px = (td[Y]>>(X*2))&3;
				if( px ) 
				{
					px = (bgpal[map_entry>>14]>>(px*2))&3;
					set_fb_pixel(0, (GX-GP)+x, GY+y, px);
				}
			}
			if( attr[0] & 0x4000 ) 
			{
				int X = (x + MX + MP);
				int Y = (y + MY + MP);
				const int mapx = X/8;
				const int mapy = Y/8;
				u16 map_entry =
					((mapx<0||mapx>=64||mapy<0||mapy>=64)&&(attr[0]&BIT(7)))
					? *(u16*)&vram[0x20000+attr[10]*2] : map[(mapy&63)*64+(mapx&63)];
				u16 JCA = map_entry & 0x7ff;
				u16* td;
				if( JCA < 512 ) td =(u16*) &vram[0x6000 + JCA*16];
				else if( JCA < 1024 ) td =(u16*) &vram[0xE000 + (JCA-512)*16];
				else if( JCA < 1024+512 ) td =(u16*) &vram[0x16000 + (JCA-1024)*16];
				else td =(u16*) &vram[0x1E000 + (JCA-(1024+512))*16];
				
				X &= 7;
				Y &= 7;
				if( map_entry & BIT(13) ) X ^= 7;
				if( map_entry & BIT(12) ) Y ^= 7;
				
				u16 px = (td[Y]>>(X*2))&3;
				if( px ) 
				{
					px = (bgpal[map_entry>>14]>>(px*2))&3;
					set_fb_pixel(1, (GX+GP)+x, GY+y, px);
				}
			}			
		}
	}
}

void virtualboy::draw_affine_bg(u16* attr)
{
	u16* map =(u16*) &vram[0x20000 + 0x2000*(attr[0]&15)];
	
	int GX = s16(attr[1]<<6)>>6;
	int GP = s16(attr[2]<<6)>>6;
	int GY = s16(attr[3]);
	int W = (attr[7] & 0x3ff) + 1;  // W different for affine
	int H = s16(attr[8]) + 1;
	
	u32 paramoffs = attr[9]*2;
	s16* param = (s16*) &vram[0x20000 + paramoffs];
	
	for(int y = 0; y < H; ++y)
	{
		float MX = param[0] / 8.f;
		float MY = param[2] / 8.f;
		float DX = param[3] / 512.f;
		float DY = param[4] / 512.f;
		int MP = param[1];
				
		paramoffs += 16;
		paramoffs &= 0x1fff0;
		param = (s16*) &vram[0x20000 + paramoffs];
		
		for(int x = 0; x < W; ++x)
		{	
			if( attr[0] & 0x8000 )
			{
				int X = 0;
				int Y = 0;
				if( MP < 0 )
				{
					X = MX + DX*(x-MP);
					Y = MY + DY*(x-MP);
				} else {
					X = MX + DX*x;
					Y = MY + DY*x;
				}
				const int mapx = X/8;
				const int mapy = Y/8;
				u16 map_entry =
					((mapx<0||mapx>=64||mapy<0||mapy>=64)&&(attr[0]&BIT(7))) 
					? *(u16*)&vram[0x20000+attr[10]*2] : map[(mapy&63)*64+(mapx&63)];
				u16 JCA = map_entry & 0x7ff;
				u16* td;
				if( JCA < 512 ) td =(u16*) &vram[0x6000 + JCA*16];
				else if( JCA < 1024 ) td =(u16*) &vram[0xE000 + (JCA-512)*16];
				else if( JCA < 1024+512 ) td =(u16*) &vram[0x16000 + (JCA-1024)*16];
				else td =(u16*) &vram[0x1E000 + (JCA-(1024+512))*16];
				
				X &= 7;
				Y &= 7;
				if( map_entry & BIT(13) ) X ^= 7;
				if( map_entry & BIT(12) ) Y ^= 7;
				
				u16 px = (td[Y]>>(X*2))&3;
				if( px ) 
				{
					px = (bgpal[map_entry>>14]>>(px*2))&3;
					set_fb_pixel(0, (GX-GP)+x, GY+y, px);
				}
			}
			if( attr[0] & 0x4000 ) 
			{
				int X = 0;
				int Y = 0;
				if( MP >= 0 )
				{
					X = MX + DX*(x-MP);
					Y = MY + DY*(x-MP);
				} else {
					X = MX + DX*x;
					Y = MY + DY*x;
				}
				const int mapx = X/8;
				const int mapy = Y/8;
				u16 map_entry =
					((mapx<0||mapx>=64||mapy<0||mapy>=64)&&(attr[0]&BIT(7))) 
					? *(u16*)&vram[0x20000+attr[10]*2] : map[(mapy&63)*64+(mapx&63)];
				u16 JCA = map_entry & 0x7ff;
				u16* td;
				if( JCA < 512 ) td =(u16*) &vram[0x6000 + JCA*16];
				else if( JCA < 1024 ) td =(u16*) &vram[0xE000 + (JCA-512)*16];
				else if( JCA < 1024+512 ) td =(u16*) &vram[0x16000 + (JCA-1024)*16];
				else td =(u16*) &vram[0x1E000 + (JCA-(1024+512))*16];
				
				X &= 7;
				Y &= 7;
				if( map_entry & BIT(13) ) X ^= 7;
				if( map_entry & BIT(12) ) Y ^= 7;
				
				u16 px = (td[Y]>>(X*2))&3;
				if( px ) 
				{
					px = (bgpal[map_entry>>14]>>(px*2))&3;
					set_fb_pixel(1, (GX+GP)+x, GY+y, px);
				}
			}			
		}
	}
}

void virtualboy::draw_obj_group(u32 group)
{
	u32 start_index = 0;
	if( group != 0 ) start_index = (objctrl[group-1] + 0) & 0x3ff;
	u32 end_index = objctrl[group] & 0x3ff;
	
	for(u32 i = end_index; i != start_index; i = (i-1)&0x3ff)
	{
		draw_sprite(i);
	}
}

void virtualboy::draw_sprite(u32 index)
{
	u16* attr = (u16*)&vram[0x3E000 + index*8];
	if( (attr[1]&0xc000) == 0 ) return;
	//printf("about to draw sprite $%X\n", index);
	
	u32 JCA = attr[3] & 0x7ff;
	u16* td;
	if( JCA < 512 ) td =(u16*) &vram[0x6000 + JCA*16];
	else if( JCA < 1024 ) td =(u16*) &vram[0xE000 + (JCA-512)*16];
	else if( JCA < 1024+512 ) td =(u16*) &vram[0x16000 + (JCA-1024)*16];
	else td =(u16*) &vram[0x1E000 + (JCA-(1024+512))*16];
	
	int JP = s16(attr[1]<<6)>>6;
	int JX = s16(attr[0]<<6)>>6;
	int JY = attr[2] & 0xff;
	
	for(u32 y = 0; y < 8 && JY+y < 224; ++y)
	{
		const u8 Y = ( ( attr[3] & BIT(12) ) ? (y^7) : y );
		const u16 v = td[Y];
		
		for(u32 x = 0; x < 8; ++x)
		{
			const u8 X = ( (attr[3] & BIT(13)) ? (x^7) : x );
			const u32 px = (v >> (X*2))&3;
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
	frame_divider = 0;
	for(u32 i = 0; i < 256*1024; ++i) vram[i] = 0;
}

void virtualboy::snd_clock(u64 cc)
{
	// run the auto deactivation interval
	for(u32 i = 0; i < 6; ++i)
	{
		if( !(channel[i][0]&0x80) || !(channel[i][0]&BIT(5)) ) continue;
		chan_int_cycles[i] += cc;
		if( chan_int_cycles[i] >= 77 )
		{
			chan_int_cycles[i] -= 77;
			chaninterval[i] -= 1;
			if( chaninterval[i] == 0 ) channel[i][0] = 0;
		}
	}
	
	// run the frequency timer and step wave ram position for channels 1-5
	for(u32 i = 0; i < 5; ++i)
	{
		if( !(channel[i][0]&0x80) ) continue;
		chancycles[i] += cc;
		const u32 upper = (2048 - (((channel[i][3]&7)<<8)|channel[i][2])) * 4;
		if( chancycles[i] >= upper )
		{
			chancycles[i] -= upper;
			chanpos[i] += 1;
			chanpos[i] &= 31;
		}
	}

	// calculate the final sample if it's time (emulator uses 44.1KHz)
	sample_cycles += cc;
	if( sample_cycles >= 453 )
	{
		sample_cycles -= 453;
		u32 L=0, R=0;
		for(u32 i = 0; i < 5; ++i)
		{
			if( !(channel[i][0]&0x80) ) continue;
			const u32 wav = channel[i][6] & 7;
			if( wav > 4 ) continue;
			L += wavram[wav*32 + chanpos[i]] * (channel[i][1]>>4);
			R += wavram[wav*32 + chanpos[i]] * (channel[i][1]&15);
		}
		float LF = (L / float(0x7ff)) * 2 - 1;
		float RF = (R / float(0x7ff)) * 2 - 1;
		audio_add(LF, RF);
	}
}


