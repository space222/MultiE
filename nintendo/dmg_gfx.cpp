#include <cstdio>
#include "dmg.h"

u8 dmg_read(u16);
void dmg_write(u16, u8);
extern console* sys;
#define GB (*dynamic_cast<dmg*>(sys))
#define screen GB.fbuf

#define LCDC GB.IO[0x40]
#define STAT GB.IO[0x41]
#define LY   GB.IO[0x44]
#define LYC  GB.IO[0x45]
#define IF   GB.IO[0x0F]
#define IE   GB.IO[0xFF]
#define BGP  GB.IO[0x47]
#define OBP0 GB.IO[0x48]
#define OBP1 GB.IO[0x49]
#define SCX  GB.IO[0x43]
#define SCY  GB.IO[0x42]
#define WX   GB.IO[0x4B]
#define WY   GB.IO[0x4A]

u32 dmg_palette[] = { 0xFFFFFF00, 0xC0C0C000, 0x60606000, 0x20202000 };

int num_sprites = 0;
u8 sprites_online[10] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
u8 shifter_colors[16];
u8 shifter_src[16];
int current_dot = 0;
void gfx_find_sprites();

int end_mode_3 = 0;
int CurX = 0; // which actual LCD dot is next
int bgtileno = 0;
int bgtiled1, bgtiled2;

int scanlineSCX = 0;
int scanlineSCY = 0;
int scanlineLYC = 0;
int scanlineWX = 0;
int scanlineWY = 0;

int disabled_scanline = 0;

void gfx_dot()
{
	if( ! (LCDC & 0x80) )
	{
		LY = 0;
		STAT &= ~3;
		current_dot++;
		if( current_dot == 456 )
		{
			current_dot = 0;
			disabled_scanline++;
			if( disabled_scanline == 154 ) disabled_scanline = 0;
		}
		
		if( disabled_scanline < 144 && current_dot < 160)
		{
			screen[disabled_scanline*160 + current_dot] = dmg_palette[0];
		}
		return;
	}


	switch( STAT & 3 )
	{
	case 0: // HBLANK
		current_dot++;
		if( current_dot == 456 )
		{
			current_dot = 0;
			LY++;
			if( LY > 143 )
			{
				if( LCDC & 0x80 ) IF |= 1;  // vblank always happens if screen is on
				if( STAT & 0x10 ) IF |= 2;  // STAT has it's own mode-based vblank interrupt
				STAT = (STAT&~3) | 1;
			} else {
				if( LY == scanlineLYC )
				{
					if( STAT & 0x40 ) IF |= 2;
					STAT |= 4;
				} else {
					STAT &= ~4;
				}
				if( STAT & 0x20 ) IF |= 2;
				STAT = (STAT&~3) | 2;
			}
		}
		return;
	case 2: // search OAM
		if( current_dot == 0 )
		{
			scanlineSCX = SCX;
			scanlineSCY = SCY;
			scanlineWX = WX;
			scanlineWY = WY;
			scanlineLYC = LYC;
			gfx_find_sprites();
		} else if( current_dot == 79 ) {
			STAT = (STAT&~3) | 3;
			CurX = 0;
		}
		current_dot++;
		return;
	case 1: // VBLANK
		current_dot++;
		if( current_dot == 456 )
		{
			scanlineSCX = SCX;
			scanlineSCY = SCY;
			scanlineWX = WX;
			scanlineWY = WY;
			scanlineLYC = LYC;
			current_dot = 0;
			LY++;
			if( LY > 153 ) 
			{
				//u8* pixels;
				//u32 stride;
				//SDL_LockTexture(gfxtex, nullptr,(void**) &pixels,(int*) &stride);
				//memcpy(pixels, screen, 160*144*4);
				//SDL_UnlockTexture(gfxtex);
				LY = 0;
				STAT = (STAT&~3) | 2;	
			}
			if( LY == scanlineLYC )
			{
				if( STAT & 0x40 ) IF |= 2;
				STAT |= 4;
			} else {
				STAT &= ~4;
			}
		}
		return;
	case 3: // screen drawing
		current_dot++;		
		if( CurX > 159 )
		{
			if( current_dot >= end_mode_3 )
			{
				if( STAT & 8 ) IF |= 2;

				CurX = 0;
				STAT = (STAT&~3) | 0;
			}
			return;
		}
		break; // actual dot draw is the rest of function		
	}
	
	if( current_dot == 80 )
	{
		end_mode_3 = 80+168;
	}
	
	int d1 = 0; // needed outside the if, so sprites can only be behind non-zero pixels
	
	if( (LCDC & 0x20) && (LY >= scanlineWY) && (CurX >= (scanlineWX-7)) )
	{
		int st = (32 * ((LY-scanlineWY)>>3)) + (((CurX-(scanlineWX-7))>>3)&0x1F);
		int taddr = (LCDC&0x40) ? 0x1C00 : 0x1800;
		taddr += st;
		int tileno = GB.VRAM[taddr];
		
		int daddr = 0;
		if( !(LCDC & 0x10) )
		{
			if( tileno & 0x80 )
			{
				tileno &= 0x7f;
				daddr += 0x800;
			} else {
				daddr += 0x1000;
			}	
		}
		daddr += tileno*16;

		int row = (LY-scanlineWY)&7;

		d1 = GB.VRAM[daddr + row*2];
		int d2 = GB.VRAM[daddr + row*2 + 1];
		int shft = 7 - ((CurX-(scanlineWX-7))&7);
		d1 >>= shft;
		d1 &= 1;
		d2 >>= shft;
		d2 &= 1;
		d1 = (d2<<1)|d1;
		screen[LY*160 + CurX] = dmg_palette[(BGP>>(d1*2))&3];		
	} else {
		int scrollvalue = (scanlineSCY + (int)LY);
		
		int st = (32 * ((scrollvalue>>3)&0x1F)) + (((scanlineSCX+CurX)>>3) & 0x1F);
		int taddr = (LCDC&8) ? 0x1C00 : 0x1800;
		taddr += st;
		int tileno = GB.VRAM[taddr];
		
		int daddr = 0; //(LCDC&0x10) ? 0 : 0x800;
		if( !(LCDC & 0x10) )
		{
			if( tileno & 0x80 )
			{
				tileno &= 0x7f;
				daddr += 0x800;
			} else {
				daddr += 0x1000;
			}
		}
		daddr += tileno*16;

		scrollvalue &= 7;
		scrollvalue <<= 1;
		d1 = GB.VRAM[daddr + scrollvalue];
		int d2 = GB.VRAM[daddr + scrollvalue + 1];
		
		int shft = 7 - ((scanlineSCX+CurX)&7);
		d1 >>= shft;
		d1 &= 1;
		d2 >>= shft;
		d2 &= 1;
		d1 = (d2<<1)|d1;
		screen[LY*160 + CurX] = dmg_palette[(BGP>>(d1*2))&3];
	}
	
	if( (LCDC&2) ) for(int i = 0; i < num_sprites; ++i)
	{
		if( sprites_online[i] == 0xFF ) continue;
		//if(i == 0) printf("num sprites = %i\n", num_sprites);

		int sprX = GB.OAM[sprites_online[i]*4 + 1];
		int sprY = GB.OAM[sprites_online[i]*4];
		sprX -= 8;
		sprY -= 16;
			
		if( CurX - sprX < 8 && CurX - sprX >= 0 )
		{
			int attr = GB.OAM[sprites_online[i]*4 + 3];
			int stile = GB.OAM[sprites_online[i]*4 + 2];
			int saddr = 0;
			int rowoff = LY - sprY;
			if( attr & 0x40 )
			{
				rowoff = ((LCDC & 4) ? 15 : 7) - rowoff;
			}
			if( LCDC & 4 )
			{
				if( rowoff > 8 )
					saddr = (stile|1)*16 + (rowoff-8)*2;
				else
					saddr = (stile&0xFE)*16 + (rowoff)*2;
			} else {
				saddr = (stile*16) + (rowoff*2);
			}
			int sd1 = GB.VRAM[saddr];
			int sd2 = GB.VRAM[saddr+1];
			int shft = ((CurX - sprX)&7);
			if( !(attr & 0x20) ) shft = 7 - shft;
			sd1 >>= shft;
			sd1 &= 1;
			sd2 >>= shft;
			sd2 &= 1;
			sd1 = (sd2<<1)|sd1;
			u8 pal = (attr&0x10) ? OBP1 : OBP0;
			if( sd1 && !((attr&0x80) && d1) ) screen[LY*160 + CurX] = dmg_palette[(pal>>(sd1*2))&3];
			if( sd1 ) break;
		}	
	}

	CurX++;
	
	return;
}

void gfx_find_sprites()
{
	int sx = 0;
	int sprHeight = (LCDC&4) ? 16 : 8;
	
	for(int i = 0; i < 10; i++) sprites_online[i] = 0xff;
	
	for(int i = 0; i < 40 && sx < 10; ++i)
	{
		int sprY = GB.OAM[i*4];
		sprY -= 16;
		if( LY - sprY < sprHeight && LY - sprY >= 0 )
		{
			sprites_online[sx++] = i;		
		}
	}
	
	num_sprites = sx;
	
	for(int i = sx; i < 10; ++i) sprites_online[i] = 0xff;
	
	for(int i = 0; i < sx-1; ++i)
	{
		for(int y = i; y < sx; ++y)
		{
			if( GB.OAM[sprites_online[i]*4 + 1] > GB.OAM[sprites_online[y]*4 + 1] )
			{
				u8 temp = sprites_online[i];
				sprites_online[i] = sprites_online[y];
				sprites_online[y] = temp;
			}	
		}
	}
	return;
}

void gfx_cycles(int c)
{
	for(int i = 0; i < c; ++i) gfx_dot();
	return;
}

void snd_cycles(int) {}

#define TCTRL GB.IO[7]
#define TMA   GB.IO[6]
#define TCOUNT GB.IO[5]

//extern u8 TCTRL;
//extern u8 TCOUNT;
//extern u8 TMA;

int tcycle_counts[] = { 1024, 16, 64, 256 };

int cycles_until_div = 0;
int timer_cycle_count = 0;

int total_cycles = 0;

void system_cycles(int c)
{
	total_cycles += c;
	gfx_cycles(c);
	//snd_cycles(c);
	
	GB.DIV += c;
	
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
				IF |= 4;
			}
		}
	}
	
	return;
}








