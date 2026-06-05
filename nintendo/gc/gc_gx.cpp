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
		u32 fullsize = numverts*gx.vatcache[gx_fifo[0]&7] + 3;
		if( gx_fifo.size() < fullsize ) return;
		std::println("Tri draw {} verts", numverts);
		std::println("CP 70 = ${:X}", (gx.cp[0x70]>>1)&7);
		std::println("CP B2 = ${:X}", gx.cp[0xb2]);
		std::println("CP 5{} = ${:X}",(gx_fifo[0]&7), (gx.cp[0x50+(gx_fifo[0]&7)]>>13)&3);
		s16* verts = (s16*)&mem1[gx.cp[0xa0]];
		for(u32 i = 0; i < numverts; ++i)
		{
			//std::println("vert: {}, {}, {}", (s16)__builtin_bswap16(verts[i*3]), (s16)__builtin_bswap16(verts[i*3+1]), (s16)__builtin_bswap16(verts[i*3+2])  );
		}
		u32* cols = (u32*)&mem1[gx.cp[0xa2]];
		for(u32 i = 0; i < numverts; ++i)
		{
			//std::println("color: ${:X}", __builtin_bswap32(cols[i]));
		}
		for(u32 i = 0; i < fullsize; ++i) gx_fifo.pop_front();
		//std::println("vert size = {} bytes", gx.vatcache[gx_fifo[0]&7]);
		//exit(1);
	
		}continue;
	case 0x80>>3:{ // Draw Quads
		if( gx_fifo.size() < 3 ) return;
		u32 numverts = (gx_fifo[1]<<8)|gx_fifo[2];
		u32 fullsize = numverts*gx.vatcache[gx_fifo[0]&7] + 3;
		if( gx_fifo.size() < fullsize ) return;
		std::println("Quad draw {} verts", numverts);
		std::println("CP 70 = ${:X}", (gx.cp[0x70]>>1)&7);
		std::println("CP B2 = ${:X}", gx.cp[0xb2]);
		std::println("CP 5{} = ${:X}",(gx_fifo[0]&7), (gx.cp[0x50+(gx_fifo[0]&7)]>>13)&3);
		
		for(u32 i = 0; i < fullsize; ++i) gx_fifo.pop_front();
		
		}continue;

	case 1:{ // CP Regs
		if( gx_fifo.size() < 6 ) return;		
		u32 old = gx.cp[gx_fifo[1]];
		gx.cp[gx_fifo[1]] = (gx_fifo[2]<<24)|(gx_fifo[3]<<16)|(gx_fifo[4]<<8)|gx_fifo[5];
		if( gx.cp[gx_fifo[1]] != old && gx_fifo[1] >= 0x50 && gx_fifo[1] < 0x98 )
		{
			gx.vatcache[gx_fifo[1]&7] = gx_vert_size(gx_fifo[1]&7);
		}
		for(u32 i = 0; i < 6; ++i) gx_fifo.pop_front();
		}continue;
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

const u32 coordsize[] = { 1,1,2,2,4,4,4,4 };
const u32 colorsize[] = { 2, 3, 4, 2, 3, 4,4,4 };

u32 GameCube::gx_vert_size(u32 ca)
{ // this'll be cached/updated when cp regs 0x50-0x98 actually change
	u32 vsize = 0;
	u32 vcd1 = gx.cp[0x50+ca];
	u32 vcd2 = gx.cp[0x60+ca];
	u32 vatA = gx.cp[0x70+ca];
	u32 vatB = gx.cp[0x80+ca];
	u32 vatC = gx.cp[0x90+ca];
	
	u32 pos = ((vcd1>>9) & 3);
	if( pos ) { vsize += ( (pos>=2) ? (pos-1) : (coordsize[(vatA>>1)&7]*((vatA&1)?3:2)) ); }
	u32 norm = ((vcd1>>11) & 3);
	if( norm ) { vsize += ( (norm>=2) ? ((norm-1)*((vatA&BIT(31))?3:1)) : (coordsize[(vatA>>10)&7]*((vatA&BIT(9))?9:3)) ); }
	u32 col0 = ((vcd1>>13) & 3);
	if( col0 ) { vsize += ( (col0>=2) ? (col0-1) : (colorsize[(vatA>>14)&7]*1) ); }
	u32 col1 = ((vcd1>>15) & 3);
	if( col1 ) { vsize += ( (col1>=2) ? (col1-1) : (colorsize[(vatA>>18)&7]*1) ); }
	u32 t0 = ((vcd2>>0) & 3);
	if( t0 ) { vsize += ( (t0>=2) ? (t0-1) : (coordsize[(vatA>>22)&7]*((vatA&BIT(21))?2:1)) ); }
	
	
	return vsize + std::popcount(vcd1 & 0x1ff); /*todo: all the direct matrix indices are 1 byte each?*/
}




