#include <cstdio>
#include <vector>
#include <utility>
#include <algorithm>
#include "psx.h"

int dither[] = {
 -4, +0, -3, +1,
 +2, -2, +3, -1,
 -3, +1, -4, +0,
 +3, -1, +2, -2
};

inline u16 color32to16(u32 c)
{
	u8 B = ((c>>16)&0xff)>>3;
	u8 G = ((c>>8)&0xff)>>3;
	u8 R = (c&0xff)>>3;
	return (B<<10)|(G<<5)|R;
}

#define Cv(a) c[a]&0xff, (c[a]>>8)&0xff, (c[a]>>16)&0xff

void psx::gpu_gp0(u32 val)
{
	gpubuf.push_back(val);
	const u8 cmd = (gpubuf[0]>>24);
	const u8 top3 = cmd>>5;
	
	/*if( gpubuf.size() == 1 ) 
	{
		printf("new ");
		printf("gp0 = $%X, curcmd = $%X\n", val, cmd);
	} else {
		printf("gp0 = $%X\n", val);
	}*/
	
	if( top3 == 5 )
	{
		// cpu to vram. todo: mask bit
		if( gpubuf.size() < 3 ) return;
		P sz;
		sz.x = gpubuf[2]&0x3ff;
		sz.y = (gpubuf[2]>>16)&0x1ff;
		sz.x = ((sz.x-1)&0x3ff)+1;
		sz.y = ((sz.y-1)&0x1ff)+1;
		s32 words = (sz.x * sz.y) + 1;
		//if( words & 1 ) words++;
		words >>= 1;
		if( gpubuf.size() < u32(words + 3) ) return;
		P pos;
		pos.x = (gpubuf[1] & 0x3ff);
		pos.y = (gpubuf[1]>>16) & 0x1ff;
		u16* temp = (u16*)&gpubuf[3];
		try {
		for(s16 Y = pos.y; Y < pos.y+sz.y; ++Y)
		{
			for(s16 X = pos.x; X < pos.x+sz.x; ++X)
			{
				VRAM.at(Y*1024 + X) = *temp; ++temp;
			}
		}
		} catch(...) {}
		gpubuf.clear();
		return;
	}
	
	if( top3 == 6 )
	{
		if( gpubuf.size() < 3 ) return;
		gpustat |= (1u<<27);
		gpu_startx = gpu_rx = gpubuf[1]&0xffff;
		gpu_starty = gpu_ry = gpubuf[1]>>16;
		gpu_rw = gpubuf[2]&0xffff;
		gpu_rh = gpubuf[2]>>16;
		gpu_read_active = true;
		gpubuf.clear();
		return;
	}
	
	if( top3 == 4 )
	{  //vram2vram
		if( gpubuf.size() < 4 ) return;
		u16 sx = gpubuf[1] & 0x3ff;
		u16 sy = (gpubuf[1]>>16) & 0x1ff;
		u16 dx = gpubuf[2] & 0x3ff;
		u16 dy = (gpubuf[2]>>16) & 0x1ff;
		u16 w = gpubuf[3];
		w = ((w-1) & 0x3ff) + 1;
		u16 h = gpubuf[3]>>16;
		h = ((h-1) & 0x3ff) + 1;
		for(u32 y = 0; y < h; ++y)
		{
			for(u32 x = 0; x < w; ++x) 
			{
				u32 destaddr = (dy+y)*1024 + (dx+x);
				u32 srcaddr = (sy+y)*1024 + (sx+x);
				VRAM[destaddr&0x7ffff] = VRAM[srcaddr&0x7ffff];
			}
		}
		gpubuf.clear();
		return;
	}
	
	if( top3 == 3 )
	{  // rectangles
		if( gpubuf.size() < u32(2 + ((cmd&0x18)?0:1) + ((cmd&4)?1:0)) ) return;
		P pos(gpubuf[1]);
		u32 index = 2;
		P sz;
		u32 clutUV = 0;
		if( cmd & 4 ) clutUV = gpubuf[index++];
		switch( (cmd&0x18)>>3 )
		{
		case 0: sz = P(gpubuf[index]); break;
		case 1: sz = P(1,1); break;
		case 2: sz = P(8,8); break;
		case 3: sz = P(16,16); break;
		}
		pos.x += offset_x;
		pos.y += offset_y;
		if( cmd & 4 )
		{
			if( last_clut_value != ((clutUV>>16)&0x7fff) )
			{
				last_clut_value = ((clutUV>>16)&0x7fff);
				reload_clut_cache();
			}
			u8 V = clutUV>>8;
			u8 U = clutUV;
			for(int Y = pos.y; Y < pos.y+sz.y; ++Y)
			{
				if( Y >= area_y1 && Y <= area_y2 )
				{
					for(int X = pos.x, rowU = U; X < pos.x+sz.x; ++X)
					{
						if( X >= area_x1 && X <= area_x2 )
						{
							u16 C = sample_tex(texpage, rowU, V);
							if( (cmd & 2) && (C&0x8000) )
							{
								u8 R = C&0x1f;
								u8 G = (C>>5)&0x1f;
								u8 B = (C>>10)&0x1f;
								VRAM[Y*1024 + X] = semitransp(VRAM[Y*1024+X], R<<3, G<<3, B<<3);
							} else {
								if( C ) VRAM[Y*1024 + X] = C;
							}
						}
						rowU += (texpage&(1u<<12)) ? -1 : 1;
					}
				}
				V += (texpage&(1u<<13))? -1 : 1;
			}	
		} else {
			s16 final_x = std::clamp(pos.x+sz.x, area_x1+0, area_x2+0);
			s16 final_y = std::clamp(pos.y+sz.y, area_y1+0, area_y2+0);
			pos.x = std::max(pos.x, area_x1);
			pos.y = std::max(pos.y, area_y1);
			u16 C = color32to16(gpubuf[0]);
			for(int Y = pos.y; Y < final_y; ++Y)
			{
				for(int X = pos.x; X < final_x; ++X)
				{
					if( cmd & 2 )
					{
						u8 R = gpubuf[0]&0xff;
						u8 G = gpubuf[0]>>8;
						u8 B = gpubuf[0]>>16;
						VRAM[Y*1024 + X] = semitransp(VRAM[Y*1024+X], R, G, B);
					} else {
						VRAM[Y*1024 + X] = C;
					}
				}
			}
		}
		
		gpubuf.clear();
		return;
	}
	
	if( top3 == 7 )
	{
		switch( cmd )
		{
		case 0xE1:
			gpustat &= ~0x87ffu;
			gpustat |= (val&0x7ffu)|((val<<4)&0x8000u);
			texpage = val;
			break;
		case 0xE2:
			gpu_texmask_x = val & 0x1f;
			gpu_texmask_y = (val>>5) & 0x1f;
			gpu_texoffs_x = (val>>10) & 0x1f;
			gpu_texoffs_y = (val>>15) & 0x1f;
			break;
		case 0xE3:
			area_x1 = val & 0x3ff;
			area_y1 = (val>>10) & 0x1ff;
			break;
		case 0xE4:
			area_x2 = val & 0x3ff;
			area_y2 = (val>>10) & 0x1ff;
			break;
		case 0xE5:
			offset_x = (val<<5);
			offset_x >>= 5;
			offset_y = ((val>>11)&0x7ff)<<5;
			offset_y >>= 5;
			//printf("GPU: offset = (%i, %i)\n", offset_x, offset_y);
			break;
		case 0xE6: break;
		case 0xE0:
		case 0xEE: break;
		default:
			printf("GPU: Unimpl env cmd = $%X\n", cmd);
			break;
		}
		gpubuf.clear();
		return;
	}
	
	if( top3 == 0 )
	{
		switch( cmd )
		{
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x08:
		case 0x00: break; // nop?
		case 0x01: // clear cache (includes CLUT?)
			last_clut_value = 0xffff;
			break;
		case 0x02:{ // fill vram
			//currently assuming all the offset/mask stuff doesn't apply, but that draw area *does*
			if( gpubuf.size() < 3 ) return;
			P pos(gpubuf[1]);
			P sz(gpubuf[2]);
			s16 final_x = std::min(pos.x + 0 + sz.x, area_x2+0);
			s16 final_y = std::min(pos.y + 0 + sz.y, area_y2+0);
			pos.x = std::clamp(pos.x+0, area_x1+0, area_x2+0);
			pos.y = std::clamp(pos.y+0, area_y1+0, area_y2+0);
			u16 V = color32to16(gpubuf[0]);
			for(s16 Y = pos.y; Y < final_y; ++Y)
			{
				for(s16 X = pos.x; X < final_x; ++X)
				{
					VRAM[Y*1024 + X] = V; 			
				}		
			}
			}break;
		default: break;
		}
		gpubuf.clear();
		return;
	}
	
	if( top3 == 1 )
	{
		u32 verts = (cmd&8) ? 4 : 3;
		if( gpubuf.size() < verts + ((cmd&0x10)?verts:1) + ((cmd&4)?verts:0) ) return;
		
		u32 c[4];
		P v[4];
		u8 S[4];
		u8 T[4];
		u16 page=0, clut=0;
		u32 index = 0;
		c[0] = gpubuf[0]; c[3] = c[2] = c[1] = c[0];
		for(u32 i = 0; i < verts; ++i)
		{
			if( (cmd&0x10) || index==0 ) { c[i] = gpubuf[index++]; }
			v[i] = P(gpubuf[index++]);
			if( cmd&4 )
			{
				S[i] = gpubuf[index]&0xff;
				T[i] = (gpubuf[index]>>8)&0xff;
				if( i == 1 ) 
				{
					page = gpubuf[index]>>16; 
					set_texpage(page);
				} else if( i == 0 ) { 
					clut = gpubuf[index]>>16;
					if( last_clut_value != (clut&0x7fff) )
					{
						last_clut_value = (clut&0x7fff);
						reload_clut_cache();
					}
				} 
				index += 1;
			}
		}
		v[0].x += offset_x;
		v[1].x += offset_x;
		v[2].x += offset_x;
		v[3].x += offset_x;
		v[0].y += offset_y;
		v[1].y += offset_y;
		v[2].y += offset_y;
		v[3].y += offset_y;
		
		gpu_curcmd = cmd;
								
		if( cmd&4 )
		{
			if( cmd&1 )
			{
				draw_tex_tri(v[0], S[0], T[0], v[1], S[1], T[1], v[2], S[2], T[2]);
				if( verts == 4 ) draw_tex_tri(v[1], S[1], T[1], v[2], S[2], T[2], v[3], S[3], T[3]);
			} else {
				draw_tex_shaded_tri(v[0], S[0], T[0], 
						    v[1], S[1], T[1], 
						    v[2], S[2], T[2],
					           Cv(0), Cv(1), Cv(2));
				if( verts == 4 ) draw_tex_shaded_tri(v[1], S[1], T[1], 
								     v[2], S[2], T[2], 
								     v[3], S[3], T[3],
								    Cv(1), Cv(2), Cv(3));
			}
		} else {
			draw_tri(v[0], Cv(0), v[1], Cv(1), v[2], Cv(2));		
			if( verts == 4 ) draw_tri(v[1], Cv(1), v[2], Cv(2), v[3], Cv(3));
		}
		
		gpubuf.clear();
		return;
	}
	
	if( top3 == 2 )
	{
		if( cmd & 8 )
		{
			
			if( gpubuf.size() > 3 &&
				 (gpubuf.back() & 0xf000f000) == 0x50005000 )
			{
				if( cmd&0x10 )
				{
				
				} else {
					//int line = 0;
					for(u32 i = 2; i < gpubuf.size()-1; ++i)
					{
						//line++;
						//printf("%i: $%X, $%X\n", line, gpubuf[i-1], gpubuf[i]);
						draw_line_mono(gpubuf[0], gpubuf[i-1], gpubuf[i]);
					}
					
				}
				gpubuf.clear();
			}
			return;
		}
		if( gpubuf.size() < ((cmd&8)?2:1) + 2 ) return;
		gpubuf.clear();
		return;	
	}
}

void psx::draw_line_mono(u32 c, P v0, P v1)
{
	v1.x += offset_x;
	v0.x += offset_x;
	v1.y += offset_y;
	v0.y += offset_y;
	u16 cv = color32to16(c);
	
	return;
	
	if( v0.y == v1.y )
	{
		if( v0.x > v1.x ) std::swap(v1, v0);
		for(; v0.x <= v1.x; ++v0.x) VRAM[v0.y*1024 + v0.x] = cv;
		return;
	} else if( v0.x == v1.x ) {
		if( v0.y > v1.y ) std::swap(v1, v0);
		for(; v0.y <= v1.y; ++v0.y) VRAM[v0.y*1024 + v0.x] = cv;
		return;
	}
		
	int dx = (v1.x - v0.x)&0x7fffFFFFu;
	int sx = (v0.x < v1.x) ? 1 : -1;
	int dy = (v1.y - v0.y)|0x80000000u;
	int sy = (v0.y < v1.y) ? 1 : -1;
	int error = dx + dy;

	while(true)
	{
		VRAM[v0.y*1024 + v0.x] = cv;
		if( v0.x == v1.x && v0.y == v1.y ) break;
		int e2 = 2 * error;
		if( e2 >= dy )
		{
			if( v0.x == v1.x ) break;
			error = error + dy;
			v0.x += sx;
		}
		if( e2 <= dx )
		{
			if( v0.y == v1.y ) break;
			error = error + dx;
			v0.y += sy;
		}
	}
}


void psx::gpu_gp1(u32 val)
{
	switch( (val>>24)&0x3F )
	{
	case 0: 
		offset_x = offset_y = 0;
		area_x1 = area_y1 = 0;
		area_x2 = 1024;
		area_y2 = 512;
		gpubuf.clear(); 
		last_clut_value = 0xffff;
		gpu_dispmode = 0;
		break;
	case 1: 
		gpubuf.clear();
		last_clut_value = 0xffff;
		break;
	case 5:
		gpu_dispstart = val;
		break;
	case 6:
		gpu_hdisp = val;
		break;
	case 7:
		gpu_vdisp = val;
		break;
	case 8:
		gpu_dispmode = val;
		break;
	default:
		//printf("Unimpl gpu gp1 cmd = $%X\n", (val>>24)&0x3F);
		break;
	}
}

void psx::set_texpage(u16 val)
{
	val &= 0x1ff;
	gpustat = (gpustat&~0x1ff)|val;
	texpage = (texpage&~0x1ff)|val;	
	return;
}

void psx::reload_clut_cache()
{
	u32 start_x = (last_clut_value&0x3F)*16;
	u32 start_y = (last_clut_value>>6);
	for(u32 i = 0; i < 256; ++i)
	{
		clut_cache[i] = VRAM[(start_y*1024) + start_x + i];	
	}
}

u16 psx::sample_tex(u16 tp, u8 u, u8 v)
{
	u32 start_x = (tp&0xf)*64;
	u32 start_y = (tp&0x10)?256:0;
	
	//apply texture window and mask
	//u = (u & ((gpu_texmask_x*8)-1)) | ((gpu_texmask_x&gpu_texoffs_x)*8);
	//v = (v & ((gpu_texmask_y*8)-1)) | ((gpu_texmask_y&gpu_texoffs_y)*8);
	u = (u & ~(gpu_texmask_x * 8)) | ((gpu_texoffs_x & gpu_texmask_x) * 8);
	v = (v & ~(gpu_texmask_y * 8)) | ((gpu_texoffs_y & gpu_texmask_y) * 8);

	u32 bpp = (tp>>7)&3;
	
	if( bpp >= 2 ) return VRAM[(start_y+v)*1024 + start_x + u];
	if( bpp == 1 )
	{
		u16 temp = VRAM[(start_y+v)*1024 + start_x + (u>>1)];
		if( u&1 ) return clut_cache[temp>>8];
		return clut_cache[temp&0xff];	
	}
	u16 temp = VRAM[(start_y+v)*1024 + start_x + (u>>2)];
	return clut_cache[(temp>>((u&3)*4))&0xf];
}

s32 orient2d(const psx::P& a, const psx::P& b, const psx::P& c)
{
    //return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
    return (a.y - b.y)*c.x + (b.x - a.x)*c.y + (a.x*b.y-a.y*b.x);
}

void psx::draw_tex_shaded_tri(P v0, u32 u0, u32 V0, P v1, u32 u1, u32 V1, P v2, u32 u2, u32 V2,
		u32 r0, u32 g0, u32 b0, u32 r1, u32 g1, u32 b1, u32 r2, u32 g2, u32 b2	)
{
	// Compute triangle bounding box
	s16 minX = std::min( std::min(v0.x, v1.x), v2.x );
	s16 minY = std::min( std::min(v0.y, v1.y), v2.y );
	s16 maxX = std::max( std::max(v0.x, v1.x), v2.x );
	s16 maxY = std::max( std::max(v0.y, v1.y), v2.y );

	// Clip against screen bounds
	minX = std::max(minX, area_x1);
	minY = std::max(minY, area_y1);
	maxX = std::min(maxX, area_x2);
	maxY = std::min(maxY, area_y2);
	
	s32 area = orient2d(v0, v1, v2);
	if( area < 0 ) 
	{
		area = -area;
		std::swap(v0, v1);
		std::swap(u0, u1);
		std::swap(V0, V1);
		std::swap(r0, r1);
		std::swap(g0, g1);
		std::swap(b0, b1);
	}
	
	int w0_x_step = v1.y - v2.y;
	int w1_x_step = v2.y - v0.y;
	int w2_x_step = v0.y - v1.y;
	
	int w0_y_step = v2.x - v1.x;
	int w1_y_step = v0.x - v2.x;
	int w2_y_step = v1.x - v0.x;

	int w0_row = orient2d(v1, v2, { minX, minY });
	int w1_row = orient2d(v2, v0, { minX, minY });
	int w2_row = orient2d(v0, v1, { minX, minY });
	
	bool TL0=false, TL1=false, TL2=false;
	if( gpu_curcmd & 2  )
	{
		P edge0 = v2 - v1;
		P edge1 = v0 - v2;
		P edge2 = v1 - v0;
		TL0 = ((edge0.y == 0 && edge0.x > 0) || edge0.y > 0);
		TL1 = ((edge1.y == 0 && edge1.x > 0) || edge1.y > 0);
		TL2 = ((edge2.y == 0 && edge2.x > 0) || edge2.y > 0);
	}
	
	P p;
	for (p.y = minY; p.y <= maxY; p.y++) 
	{
		int w0 = w0_row;
		int w1 = w1_row;
		int w2 = w2_row;
		for (p.x = minX; p.x <= maxX; p.x++) // < or <= ?
		{
			//if( (w0 > 0 || (w0 == 0 && TL0)) 
			// && (w1 > 0 || (w1 == 0 && TL1)) 
			 //&& (w2 > 0 || (w2 == 0 && TL2)) 
			 if( (w0|w1|w2) > 0 )
			{
				u32 U = (u0*w0 + u1*w1 + u2*w2)/(area);
				u32 V = (V0*w0 + V1*w1 + V2*w2)/(area);
				u16 C = sample_tex(texpage, U, V);
				if( C )
				{
					int R = (r0*w0 + r1*w1 + r2*w2)/(area);
					int G = (g0*w0 + g1*w1 + g2*w2)/(area);
					int B = (b0*w0 + b1*w1 + b2*w2)/(area);
					R *= (C&0x1F)<<3;
					G *= ((C>>5)&0x1F)<<3;
					B *= ((C>>10)&0x1F)<<3;
					R /= 128;
					G /= 128;
					B /= 128;
					if( texpage & (1u<<9) )
					{
						int dith = dither[(p.y&3)*4 + (p.x&3)];
						R += dith;
						G += dith;
						B += dith;
					}
					R = std::clamp(R, 0, 255);
					G = std::clamp(G, 0, 255);
					B = std::clamp(B, 0, 255);
					if( (gpu_curcmd & 2) && (C&0x8000) )
					{
						VRAM[p.y*1024 + p.x] = semitransp(VRAM[p.y*1024 + p.x], R, G, B);
					} else {
						R >>= 3;
						G >>= 3;
						B >>= 3;
						//R &= 0x1F; G &= 0x1F; B &= 0x1F;
						u16 res = (B<<10)|(G<<5)|R;
						VRAM[p.y*1024 + p.x] = res;
					}
				}
			}
		
			w0 += w0_x_step;
			w1 += w1_x_step;
			w2 += w2_x_step;
		}
		
		w0_row += w0_y_step;
		w1_row += w1_y_step;
		w2_row += w2_y_step;
	}
}

void psx::draw_tex_tri(P v0, u32 u0, u32 V0, P v1, u32 u1, u32 V1, P v2, u32 u2, u32 V2)
{
	// Compute triangle bounding box
	s16 minX = std::min( std::min(v0.x, v1.x), v2.x );
	s16 minY = std::min( std::min(v0.y, v1.y), v2.y );
	s16 maxX = std::max( std::max(v0.x, v1.x), v2.x );
	s16 maxY = std::max( std::max(v0.y, v1.y), v2.y );

	// Clip against screen bounds
	minX = std::max(minX, area_x1);
	minY = std::max(minY, area_y1);
	maxX = std::min(maxX, area_x2);
	maxY = std::min(maxY, area_y2);

	if( ! (gpu_curcmd & 2) ) { maxX-=1; maxY-=1; }

	s32 area = orient2d(v0, v1, v2);
	if( area < 0 ) 
	{
		area = -area;
		std::swap(v0, v1);
		std::swap(u0, u1);
		std::swap(V0, V1);
	}
	
	int w0_x_step = v1.y - v2.y;
	int w1_x_step = v2.y - v0.y;
	int w2_x_step = v0.y - v1.y;
	
	int w0_y_step = v2.x - v1.x;
	int w1_y_step = v0.x - v2.x;
	int w2_y_step = v1.x - v0.x;

	int w0_row = orient2d(v1, v2, { minX, minY });
	int w1_row = orient2d(v2, v0, { minX, minY });
	int w2_row = orient2d(v0, v1, { minX, minY });
	
	bool TL0, TL1, TL2;
	{
		P edge0 = v2 - v1;
		P edge1 = v0 - v2;
		P edge2 = v1 - v0;
		TL0 = ((edge0.y == 0 && edge0.x > 0) || edge0.y > 0);
		TL1 = ((edge1.y == 0 && edge1.x > 0) || edge1.y > 0);
		TL2 = ((edge2.y == 0 && edge2.x > 0) || edge2.y > 0);
	}
	
	P p;
	for (p.y = minY; p.y <= maxY; p.y++) 
	{
		int w0 = w0_row;
		int w1 = w1_row;
		int w2 = w2_row;
		for (p.x = minX; p.x <= maxX; p.x++) 
		{
			//if( (w0 > 0 || (w0 == 0 && TL0)) 
			// && (w1 > 0 || (w1 == 0 && TL1)) 
			// && (w2 > 0 || (w2 == 0 && TL2)) 
			if( (w0|w1|w2) > 0 )
			{
				u8 U = (u0*w0 + u1*w1 + u2*w2)/(area);
				u8 V = (V0*w0 + V1*w1 + V2*w2)/(area);
				u16 C = sample_tex(texpage, U, V);
				if( (gpu_curcmd & 2) && (C&0x8000) )
				{
					VRAM[p.y*1024 + p.x] = semitransp(VRAM[p.y*1024 + p.x], 
								(C&0x1f)<<3, ((C>>5)&0x1F)<<3, ((C>>10)&0x1F)<<3);
				} else {
					if( C ) VRAM[p.y*1024 + p.x] = C;
				}
			}
		
			w0 += w0_x_step;
			w1 += w1_x_step;
			w2 += w2_x_step;
		}
		
		w0_row += w0_y_step;
		w1_row += w1_y_step;
		w2_row += w2_y_step;
	}
}

void psx::draw_tri(P v0, u32 r0, u32 g0, u32 b0,
	      P v1, u32 r1, u32 g1, u32 b1,
	      P v2, u32 r2, u32 g2, u32 b2)
{
	// Compute triangle bounding box
	s16 minX = std::min( std::min(v0.x, v1.x), v2.x );
	s16 minY = std::min( std::min(v0.y, v1.y), v2.y );
	s16 maxX = std::max( std::max(v0.x, v1.x), v2.x );
	s16 maxY = std::max( std::max(v0.y, v1.y), v2.y );

	// Clip against screen bounds
	minX = std::max(minX, area_x1);
	minY = std::max(minY, area_y1);
	maxX = std::min(maxX, area_x2);
	maxY = std::min(maxY, area_y2);

	s32 area = orient2d(v0, v1, v2);
	if( area < 0 ) 
	{
		area = -area;
		std::swap(v0, v1);
		std::swap(r0, r1);
		std::swap(g0, g1);
		std::swap(b0, b1);
	}
	
	int w0_x_step = v1.y - v2.y;
	int w1_x_step = v2.y - v0.y;
	int w2_x_step = v0.y - v1.y;
	
	int w0_y_step = v2.x - v1.x;
	int w1_y_step = v0.x - v2.x;
	int w2_y_step = v1.x - v0.x;

	int w0_row = orient2d(v1, v2, { minX, minY });
	int w1_row = orient2d(v2, v0, { minX, minY });
	int w2_row = orient2d(v0, v1, { minX, minY });
	
	P p;
	for (p.y = minY; p.y <= maxY; p.y++) 
	{
		int w0 = w0_row;
		int w1 = w1_row;
		int w2 = w2_row;
		for (p.x = minX; p.x <= maxX; p.x++) 
		{
			if( (w0|w1|w2) > 0 )
			{
				int R = (r0*w0 + r1*w1 + r2*w2)/(area);
				int G = (g0*w0 + g1*w1 + g2*w2)/(area);
				int B = (b0*w0 + b1*w1 + b2*w2)/(area);
				if( texpage & (1u<<9) )
				{
					int dith = dither[(p.y&3)*4 + (p.x&3)];
					R += dith; R = std::clamp(R, 0, 255);
					G += dith; G = std::clamp(G, 0, 255);
					B += dith; B = std::clamp(B, 0, 255);
				}
				if( gpu_curcmd&2 )
				{
					VRAM[p.y*1024 + p.x] = semitransp(VRAM[p.y*1024 + p.x], R, G, B);
				} else {
					R >>= 3;
					G >>= 3;
					B >>= 3;
					VRAM[p.y*1024 + p.x] = (B<<10)|(G<<5)|R;
				}
			}
		
			w0 += w0_x_step;
			w1 += w1_x_step;
			w2 += w2_x_step;
		}
		
		w0_row += w0_y_step;
		w1_row += w1_y_step;
		w2_row += w2_y_step;
	}
}

u32 psx::gpuread()
{
	if( !gpu_read_active ) return gpu_rval;
	
	gpu_rval = VRAM[gpu_ry*1024 + gpu_rx];
	gpu_rx += 1;
	if( gpu_rx >= gpu_startx + gpu_rw ) 
	{
		gpu_ry += 1;
		gpu_rx = gpu_startx;
	}
	
	if( gpu_ry < gpu_starty + gpu_rh )
	{
		gpu_rval |= VRAM[gpu_ry*1024 + gpu_rx]<<16;
		gpu_rx += 1;
		if( gpu_rx >= gpu_startx + gpu_rw ) 
		{
			gpu_ry += 1;
			gpu_rx = gpu_startx;
		}
	}
	
	if( gpu_ry >= gpu_starty + gpu_rh )
	{
		gpustat &= ~(1u<<27);
		gpu_read_active = false;	
	}
	
	return gpu_rval;
}

u16 psx::semitransp(u16 back, u8 rf, u8 gf, u8 bf)
{
	int rb = (back&0x1F)<<3;
	int gb = ((back>>5)&0x1f)<<3;
	int bb = ((back>>10)&0x1f)<<3;
	
	switch( (texpage>>5) & 3 )
	{
	case 0: rb = (rb>>1) + (rf>>1); gb = (gb>>1) + (gf>>1); bb = (bb>>1) + (bf>>1); break;
	case 1: rb += rf; gb += gf; bb += bf; break;
	case 2: rb -= rf; gb -= gf; bb -= bf; break;
	case 3: rb += rf>>2; gb += gf>>2; bb += bf>>2; break;
	}
	
	rb = std::clamp(rb, 0, 255)>>3;
	gb = std::clamp(gb, 0, 255)>>3;
	bb = std::clamp(bb, 0, 255)>>3;
	return (bb<<10)|(gb<<5)|rb;
}

