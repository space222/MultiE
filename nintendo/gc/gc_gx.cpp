#include <print>
#include <string>
#include "GameCube.h"

void GameCube::gx_exec()
{
while( gx_fifo.size() )
{
	if( gx_fifo[0] == 0 || gx_fifo[0] == 0x48 ) { gx_fifo.pop_front(); continue; } // nop
	
	if( gx_fifo[0] == 0x61 )
	{ // BP Regs
		if( gx_fifo.size() < 5 ) return;		
		gx.bp[gx_fifo[1]] = (gx_fifo[2]<<16)|(gx_fifo[3]<<8)|gx_fifo[4];
		for(u32 i = 0; i < 5; ++i) gx_fifo.pop_front();
		continue;
	}
	
	switch( gx_fifo[0]>>3 )
	{
	case 0x12:{ // Draw Triangles
		if( gx_fifo.size() < 3 ) return;
		u32 numverts = (gx_fifo[1]<<8)|gx_fifo[2];
		std::println("Tri draw {} verts", numverts);
		std::println("CP 70 = ${:X}", (gx.cp[0x70]>>1)&7);
		std::println("CP B2 = ${:X}", gx.cp[0xb2]);
		std::println("CP 5{} = ${:X}",(gx_fifo[0]&7), (gx.cp[0x50+(gx_fifo[0]&7)]>>13)&3);
		s16* verts = (s16*)&mem1[gx.cp[0xa0]];
		for(u32 i = 0; i < numverts; ++i)
		{
			std::println("vert: {}, {}, {}", (s16)__builtin_bswap16(verts[i*3]), (s16)__builtin_bswap16(verts[i*3+1]), (s16)__builtin_bswap16(verts[i*3+2])  );
		}
		u32* cols = (u32*)&mem1[gx.cp[0xa2]];
		for(u32 i = 0; i < numverts; ++i)
		{
			std::println("color: ${:X}", __builtin_bswap32(cols[i]));
		}		
		exit(1);
	
		}continue;
	case 0x80>>3:{ // Draw Quads
		if( gx_fifo.size() < 3 ) return;
		u32 numverts = (gx_fifo[1]<<8)|gx_fifo[2];
		std::println("Quad draw {} verts", numverts);
		std::println("CP 70 = ${:X}", (gx.cp[0x70]>>1)&7);
		std::println("CP B2 = ${:X}", gx.cp[0xb2]);
		std::println("CP 5{} = ${:X}",(gx_fifo[0]&7), (gx.cp[0x50+(gx_fifo[0]&7)]>>13)&3);
		s16* verts = (s16*)&mem1[gx.cp[0xa0]];
		for(u32 i = 0; i < numverts; ++i)
		{
			std::println("vert: {}, {}, {}", (s16)__builtin_bswap16(verts[i*3]), (s16)__builtin_bswap16(verts[i*3+1]), (s16)__builtin_bswap16(verts[i*3+2])  );
		}
		exit(1);
		}continue;

	case 1: // CP Regs
		if( gx_fifo.size() < 6 ) return;		
		gx.cp[gx_fifo[1]] = (gx_fifo[2]<<24)|(gx_fifo[3]<<16)|(gx_fifo[4]<<8)|gx_fifo[5];
		for(u32 i = 0; i < 6; ++i) gx_fifo.pop_front();
		continue;
	case 2:{ // XF Regs
		if( gx_fifo.size() < 3 ) return;
		u32 regs = (((gx_fifo[1]<<8)|gx_fifo[2])&0xf) + 1;
		if( gx_fifo.size() < 5+regs*4 ) return;
		u32 addr1 = (gx_fifo[3]<<8)|gx_fifo[4];
		gx_fifo.pop_front(); gx_fifo.pop_front(); gx_fifo.pop_front(); gx_fifo.pop_front(); gx_fifo.pop_front();
		for(u32 i = 0; i < regs; ++i, ++addr1)
		{
			gx.xf[addr1] = (gx_fifo[0]<<24)|(gx_fifo[1]<<16)|(gx_fifo[2]<<8)|gx_fifo[3];
			std::println("${:X}: xf(${:X}) = ${:X}, {}f", cpu.pc-4, addr1, gx.xf[addr1], std::bit_cast<float>(gx.xf[addr1]));
			gx_fifo.pop_front(); gx_fifo.pop_front(); gx_fifo.pop_front(); gx_fifo.pop_front();
		}
		}continue;
	default:
		std::println("Unimpl GX Command = ${:X}", gx_fifo[0]);
		exit(1);
		return;
	}
}
}

