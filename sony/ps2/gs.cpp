#include <print>
#include <string>
#include <deque>
#include "ps2.h"

inline u16 c32to16(u32 v)
{
	u8 R = v; R >>= 3;
	u8 G = v>>8; G >>= 3;
	u8 B = v>>19; B >>= 3;
	u8 A = v>>31;
	return (A<<15)|(B<<10)|(G<<5)|R;
}

void ps2::gs_set_pixel(u32 x, u32 y, u32 c)
{
	u64 scissor = gs.regs[0x40 + ((gs.regs[0]&BIT(9))?1:0)];
	u32 sx0 = scissor&0x7ff;
	u32 sx1 = (scissor>>16)&0x7ff;
	u32 sy0 = (scissor>>32)&0x7ff;
	u32 sy1 = (scissor>>48)&0x7ff;
	if( x < sx0 || x > sx1 || y < sy0 || y > sy1 ) return;
	
	u64 frame = gs.regs[0x4C + ((gs.regs[0]&BIT(9))?1:0)];
	u32 fbaddr = (frame & 0x1ff)*2048*4;
	u32 fbwidth = ((frame>>16)&0x3F)*64;
	
	switch( (frame>>24)&0x3F )
	{
	case 1:
	case 0:/*PSMCT32*/ *(u32*)&vram[fbaddr + y*fbwidth*4 + x*4] = c; break;
	case 2: // PSMCT16
	case 10: // PSMCT16S
		*(u32*)&vram[fbaddr + y*fbwidth*2 + x*2] = c32to16(c); break;
	default:
		std::println("GS: Unimpl framebuf format = {}", (frame>>24)&0x3f);
		exit(1);
	}
}

void ps2::gs_draw_sprite(vertex& a, vertex& b)
{
	if( a.x > b.x ) std::swap(a.x, b.x);
	if( a.y > b.y ) std::swap(a.y, b.y);
	
	u64 offset = gs.regs[0x18 + ((gs.regs[0]&BIT(9))?1:0)];
	float offset_x = ((offset&0xffff)/16.f);
	float offset_y = ((offset>>32)/16.f);
	a.x -= offset_x;
	a.y -= offset_y;
	b.x -= offset_x;
	b.y -= offset_y;
	
	u32 fbwidth = ((gs.regs[0x4C + ((gs.regs[0]&BIT(9))?1:0)]>>16)&0x3F)*64;
	u64 zbuf = gs.regs[0x4E + ((gs.regs[0]&BIT(9))?1:0)];
	if( ((zbuf>>24)&15) != 0 ) { std::println("Unexpected zbuf format = {}", ((zbuf>>24)&15)); }
	u32 zaddr = (zbuf & 0x1ff)*2048*4;
	
	if( zbuf & BITL(32) )
	{
		std::println("gs sprite: zbuffer masked!");
		exit(1);
	}
	
	u32 test = gs.regs[0x47 + ((gs.regs[0]&BIT(9))?1:0)];
	if( (test & BIT(16)) && ((test>>17)&3) == 1 )
	{
		std::println("Depth test ALWAYS passes");
		exit(1);
	}

	//std::println("sprite from {},{} to {},{}", a.x, a.y, b.x, b.y);
	
	for(u32 Y = a.y; Y < b.y; ++Y)
	{
		for(u32 X = a.x; X < b.x; ++X)
		{
			gs_set_pixel(X, Y, a.color);
			*(u32*)&vram[zaddr + fbwidth*4*Y + X*4] = 0;
		}
	}
}

void ps2::gs_draw_triangle(vertex& a, vertex& b, vertex& c)
{
	{
		u64 offset = gs.regs[0x18 + ((gs.regs[0]&BIT(9))?1:0)];
		float offset_x = ((offset&0xffff)/16.f);
		float offset_y = ((offset>>32)/16.f);
		a.x -= offset_x;
		a.y -= offset_y;
		b.x -= offset_x;
		b.y -= offset_y;	
		c.x -= offset_x;
		c.y -= offset_y;
	}
	
	u32 fbwidth = ((gs.regs[0x4C + ((gs.regs[0]&BIT(9))?1:0)]>>16)&0x3F)*64;
	u64 zbuf = gs.regs[0x4E + ((gs.regs[0]&BIT(9))?1:0)];
	if( ((zbuf>>24)&15) != 0 ) { std::println("Unexpected zbuf format = {}", ((zbuf>>24)&15)); }
	u32 zaddr = (zbuf & 0x1ff)*2048*4;
	//std::println("zaddr = ${:X}", zaddr);
	//std::println("fbaddr= ${:X}", (gs.regs[0x4C + ((gs.regs[0]&BIT(9))?1:0)]&0x1ff)*2048*4);
	
	if( a.y > b.y ) std::swap(a, b);
	if( b.y > c.y ) std::swap(b, c);
	if( a.y > b.y ) std::swap(a, b);
	
	//std::println("Tri c${:X}: {},{}, {},{}, {},{}", c.color, a.x, a.y, b.x, b.y, c.x, c.y);

	for(float Y = a.y; Y < b.y; ++Y)
	{
		float p1 = (Y - a.y)/(b.y - a.y);
		float p2 = (Y - a.y)/(c.y - a.y);
	
		float x1 = a.x + p1 * (b.x - a.x);
		float x2 = a.x + p2 * (c.x - a.x);
		float z1 = a.z + p1 * (b.z - a.z);
		float z2 = a.z + p2 * (c.z - a.z);
		if( x1 > x2 )
		{
			std::swap(x1, x2);
			std::swap(z1, z2);
		}
		for(float X = x1; X < x2; ++X)
		{
			if( X < 0 || Y < 0 ) continue;
			float Z = z1 + ((X-x1)/(x2-x1)) * (z2 - z1);
			if( Z < std::bit_cast<float>(*(u32*)&vram[zaddr + (int(Y)*fbwidth*4) + int(X)*4]) ) continue;
			*(u32*)&vram[zaddr + (int(Y)*fbwidth*4) + int(X)*4] = std::bit_cast<u32>(Z);
			gs_set_pixel(X, Y, c.color);
		}
	}
	for(float Y = b.y; Y < c.y; ++Y)
	{
		float p1 = (Y - b.y)/(c.y - b.y);
		float p2 = (Y - a.y)/(c.y - a.y);
	
		float x1 = b.x + p1 * (c.x - b.x);
		float x2 = a.x + p2 * (c.x - a.x);		
		float z1 = b.z + p1 * (c.z - b.z);
		float z2 = a.z + p2 * (c.z - a.z);
		if( x1 > x2 )
		{
			std::swap(x1, x2);
			std::swap(z1, z2);
		}
		for(float X = x1; X < x2; ++X)
		{
			if( X < 0 || Y < 0 ) continue;
			float Z = z1 + ((X-x1)/(x2-x1)) * (z2 - z1);
			if( Z < std::bit_cast<float>(*(u32*)&vram[zaddr + (int(Y)*fbwidth*4) + int(X)*4]) ) continue;
			*(u32*)&vram[zaddr + (int(Y)*fbwidth*4) + int(X)*4] = std::bit_cast<u32>(Z);
			gs_set_pixel(X, Y, c.color);
		}	
	}
	
	//std::println("TRIANGLE !");
	//exit(1);
}

static std::deque<ps2::vertex> VQ;

void ps2::gs_vertex_out(u64 vtx, bool kick)
{
	vertex V;
	V.color = gs.regs[1];
	V.x = (vtx & 0xffff)/16.f;
	V.y = ((vtx>>16)&0xffff)/16.f;
	V.z = (vtx>>32)/float(0xffffFFFFu);
	V.q = std::bit_cast<float>(u32(gs.regs[1]>>32));
	V.fog = gs.regs[0xA]>>56;
	if( gs.regs[0] & BIT(4) )
	{
		if( gs.regs[0] & BIT(8) )
		{ // use UV
			V.s /*really u*/ = (gs.regs[3]&0x3fff)/16.f;
			V.t /*really t*/ = ((gs.regs[3]>>16)&0x3fff)/16.f;
		} else {
			// use STQ
			V.s = std::bit_cast<float>(u32(gs.regs[2]));
			V.t = std::bit_cast<float>(u32(gs.regs[2]>>32));
		}
	}
	VQ.push_front(V);
	
	switch( gs.regs[0] & 15 )
	{
	case 0: // point
		V = VQ.back(); VQ.pop_back();
		if( !kick ) return;
		gs_set_pixel(V.x, V.y, V.color);
		break;
	case 3:{ // triangle
		if( VQ.size() < 3 ) return;
		vertex a, b, c;
		a = VQ.back(); VQ.pop_back();
		b = VQ.back(); VQ.pop_back();
		c = VQ.back(); VQ.pop_back();
		if( !kick ) return;
		gs_draw_triangle(a, b, c);	
		} break;
	case 6:{ // sprite
		if( VQ.size() < 2 ) return;
		vertex a, b;
		a = VQ.back(); VQ.pop_back();
		b = VQ.back(); VQ.pop_back();
		if( !kick ) { std::println("sprite with NO KICK"); return; }
		gs_draw_sprite(a, b);
		} break;
	default:
		std::println("GS: Unimpl primitive {}", gs.regs[0]&15);
		exit(1);
	}
}

void ps2::gs_run_fifo()
{
	while( true )
	{
		if( gs.fifo.empty() ) return;

		u128 GIFtag = gs.fifo.back();
		u32 format = (GIFtag>>58)&3;
		if( format == 3 ) format = 2;
		
		u32 NLOOP = GIFtag&0x7fff;
		u32 NREGS = (GIFtag>>60)&0xf;
		if( NREGS == 0 ) NREGS = 16;
		u64 regs = GIFtag>>64;
		
		if( format!=2 && ((GIFtag>>46)&1) )
		{ // PRE field enables setting PRIM
			gs.regs[0] = (GIFtag>>47)&0x7ff;
		}
		
		if( NLOOP == 0 )
		{
			gs.fifo.pop_back();
			continue;
		}
		
		//std::println("GIFtag = ${:X}", GIFtag);		
		
		switch( format )
		{
		case 0: // PACKED
			if( gs.fifo.size()-1 < NLOOP*NREGS ) return;
			gs.fifo.pop_back();
			//std::println("Would have PACKED");
			//std::println("NLOOP*NREGS = {} qwords, fifo sz = {} qwords", NLOOP*NREGS, gs.fifo.size());
			for( u32 loop = 0; loop < NLOOP; ++loop )
			{
				for(u32 r = 0; r < NREGS; ++r)
				{
					u128 value = gs.fifo.back();
					gs.fifo.pop_back();
					u32 dest = (regs >> (r*4)) & 15;
					switch( dest )
					{
					case 0: 
						gs.regs[0] = value&0x7ff;
						break;
					case 1: 
						gs.regs[1] &= ~0xffffFFFFull; 
						gs.regs[1] |= (value&0xff);
						gs.regs[1] |= ((value>>24)&0xff00);
						gs.regs[1] |= ((value>>48)&0xff0000);
						gs.regs[1] |= ((value>>72)&0xff000000u);
						break;
					case 2:
						gs.regs[2] = value;
						gs.regs[1] &= 0xffffFFFFu;
						gs.regs[1] |= value>>32;
						break;
					case 3:
						gs.regs[3] = (value&0x3fff)|((value>>16)&0x3fff0000u);
						break;
					case 4:
						//gs.regs[4] = (value>>44)&0xff00'0000'0000'0000ull;
						gs.regs[0xA] = (value>>44);
						gs.regs[4] = (value>>34)&0xffFFff0000'0000ull;
						gs.regs[4] |= (value>>16)&0xffff0000u;
						gs.regs[4] |= (value&0xffff);
						gs_vertex_out(gs.regs[4], !((value>>111)&1));
						break;
					case 5:
						gs.regs[5] = (value>>32)&0xFFffFFff0000'0000ull;
						gs.regs[5] |= (value>>16)&0xffff0000u;
						gs.regs[5] |= (value&0xffff);
						gs_vertex_out(gs.regs[5], !((value>>111)&1));
						break;
					case 0xE:
						dest = (value>>64)&0xff;
						gs.regs[dest] = u64(value);
						if( dest == 0xC )
						{
							gs.regs[0xA] = (value&0xff000000'0000'0000ull);
							gs_vertex_out(gs.regs[0xC]&0xFFffFF00000000ull, false);
						} else if( dest == 0xD ) {
							gs_vertex_out(gs.regs[0xD], false);
						} else if( dest == 0x4 ) {
							gs.regs[0xA] = (value&0xff000000'0000'0000ull);
							gs_vertex_out(gs.regs[0x4]&0xFFffFF00000000ull, true);
						} else if( dest == 0x5 ) {
							gs_vertex_out(gs.regs[0x5], true);
						} else if( dest == 0x54 ) {
							gs_hwreg_poke(value);
						} else if( dest == 0x53 ) {
							//todo: if not give dx/dy the start values here, then where?
							gs.dx = (gs.regs[0x51]>>32)&0x7ff;
							gs.dy = (gs.regs[0x51]>>48)&0x7ff;
						}
						//todo: ^ 0xE A+D might be able to draw kick?
						break;
					case 0xF: break; // nop
					default:
						gs.regs[dest] = u64(value);
						if( dest == 0xC )
						{
							gs_vertex_out(gs.regs[0xC], false);
						} else if( dest == 0xD ) {
							gs_vertex_out(gs.regs[0xD], false);
						}
						break;
					}
				}			
			}
			break;
		case 1:{ // REGLIST
			u32 rlsize = NREGS*NLOOP;
			if( rlsize & 1 ) rlsize += 1;
			rlsize >>= 1;
			if( gs.fifo.size()-1 < rlsize ) return;
			gs.fifo.pop_back();
			
			std::println("Would have REGLIST");
			std::println("NLOOP*NREGS = {} qwords, fifo sz = {} qwords", rlsize, gs.fifo.size());
			exit(1);
			}break;
		case 2: // IMAGE
			if( gs.fifo.size()-1 < NLOOP ) return;
			gs.fifo.pop_back();
			
			for(u32 i = 0; i < NLOOP; i++)
			{
				u128 v = gs.fifo.back();
				gs_hwreg_poke(v);
				gs_hwreg_poke(v>>64);
				gs.fifo.pop_back();
			}
			
			break;
		}
	}

	gs.fifo.clear(); // temporary
	return;
}

void ps2::gs_hwreg_poke(u64 v)
{
	*(u32*)&vram[gs.dy*640*4 + gs.dx*4] = v;
	gs.dx += 1;
	if( gs.dx >= 640 ) { gs.dy+=1; gs.dx = 0; }
	*(u32*)&vram[gs.dy*640*4 + gs.dx*4] = v>>32;
	gs.dx += 1;
	if( gs.dx >= 640 ) { gs.dy+=1; gs.dx = 0; }
}
 





