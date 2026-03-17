#include <print>
#include "snes.h"

void snes::ppu_draw_scanline()
{

}

void snes::ppu_mc(s64 mc)
{
	ppu.master_cycles += mc;
	if( ppu.master_cycles < 1364 ) return;

	if( ppu.scanline < 240 ) ppu_draw_scanline();
	ppu.scanline++;
	if( ppu.scanline == 240 )
	{
		//todo: vblank nmi
	}
	
	if( ppu.scanline == 263 )
	{
		ppu.frame_complete = true;
		ppu.scanline = 0;
	}
}

