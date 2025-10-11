#include <print>
#include "dreamcast.h"

struct rend_context
{
	u8* fbuf;
	int stride; // in pixels
	std::function<u32(u32 id, float u, float v)> texture_sample = [](u32, float, float)->u32{ return 0; };
	int clip_x1, clip_y1, clip_x2, clip_y2;
};

struct vec4
{
	float& operator[](int i) { return v[i]; }
	const float& operator[](int i) const { return v[i]; }
	float v[4];
	//float x, y, z, w;
};

struct rvertex
{
	vec4 p, c, t;
};

inline float edge_func(const rvertex& a, const rvertex& b, const rvertex& c)
{
	return (b.p[0] - a.p[0]) * (c.p[1] - a.p[1]) - (b.p[1] - a.p[1]) * (c.p[0] - a.p[0]);
}

inline float max(float x, float y, float z) { return std::max(x, std::max(y, z)); }
inline float min(float x, float y, float z) { return std::min(x, std::min(y, z)); }

static u32 rendbpp = 0;

void render_tri_shaded(rend_context* ctx, rvertex a, rvertex b, rvertex c)
{
	float w = edge_func(a, b, c);
	if( w < 0 )
	{
		std::swap(a,b);
		w = -w;
	}

	int minX = std::max((float)ctx->clip_x1, min(a.p[0], b.p[0], c.p[0]));
	int minY = std::max((float)ctx->clip_y1, min(a.p[1], b.p[1], c.p[1]));
	int maxX = std::min((float)ctx->clip_x2, max(a.p[0], b.p[0], c.p[0]));
	int maxY = std::min((float)ctx->clip_y2, max(a.p[1], b.p[1], c.p[1]));
	
	for(int Y = minY; Y < maxY; ++Y)
	{
		for(int X = minX; X < maxX; ++X)
		{
			rvertex P;
			P.p[0] = X;
			P.p[1] = Y;
			const auto ABP = edge_func(a, b, P);
			const auto BCP = edge_func(b, c, P);
			const auto CAP = edge_func(c, a, P);
			if( ABP >= 0 && BCP >= 0 && CAP >= 0 )
			{
				float wa = BCP/w;
				float wb = CAP/w;
				float wc = ABP/w;
			
				float R = a.c[0] * wa + b.c[0] * wb + c.c[0] * wc;
				float G = a.c[1] * wa + b.c[1] * wb + c.c[1] * wc;
				float B = a.c[2] * wa + b.c[2] * wb + c.c[2] * wc;
				switch( rendbpp )
				{
				case 3:{
					u32 rgb = u32(B*255)<<8;
					rgb |= u32(G*255)<<16;
					rgb |= u32(R*255)<<24;
					rgb |= 0xff;//000000;
					((u32*)ctx->fbuf)[Y*ctx->stride + X] = rgb;
				}break;
				case 1:{
					u16 rgb = B*31;
					rgb |= u16(G*63)<<5;
					rgb |= u16(R*31)<<11;
					((u16*)ctx->fbuf)[Y*ctx->stride + X] = rgb;				
				}break;
				default: std::println("tri rendered with bpp={}", rendbpp); exit(1);
				}
			}
		}
	}
}

void dreamcast::ta_draw_tri()
{
	ta_vertex a,b,c;
	a = ta_vertq[0];
	b = ta_vertq[1];
	c = ta_vertq[2];
	ta_vertq.pop_front();
	
	rvertex v1,v2,v3;
	v1.p[0] = a.x;
	v1.p[1] = a.y;
	v1.p[2] = a.z;
	v1.c[0] = a.r;
	v1.c[1] = a.g;
	v1.c[2] = a.b;

	v2.p[0] = b.x;
	v2.p[1] = b.y;
	v2.p[2] = b.z;
	v2.c[0] = b.r;
	v2.c[1] = b.g;
	v2.c[2] = b.b;

	v3.p[0] = c.x;
	v3.p[1] = c.y;
	v3.p[2] = c.z;
	v3.c[0] = c.r;
	v3.c[1] = c.g;
	v3.c[2] = c.b;
	
	rend_context ctx;
	ctx.clip_x1 = ctx.clip_y1 = 0;
	ctx.clip_x2 = 640;
	ctx.clip_y2 = 480;
	ctx.stride = 640;
	ctx.fbuf = &vram[holly.fb_w_sof1&0x7fffff];
	std::println("Rendered TRI! {},{}, {},{}, {},{}", v1.p[0], v1.p[1], v2.p[0], v2.p[1], v3.p[0], v3.p[1]);
	rendbpp = (holly.fb_r_ctrl>>2)&3;
	if( ta_clear )
	{
		ta_clear = false;
		u32 mult = 2;
		if( rendbpp == 2 ) mult = 3;
		if( rendbpp == 3 ) mult = 4;
		memset(vram+(holly.fb_w_sof1&0x7ffff0), 0, 640*480*mult);
	}
	render_tri_shaded(&ctx, v1,v2,v3 );	
}

void dreamcast::ta_vertex_in()
{
	if( ta_col_type > 1 ) { std::println("ta_col_type = ${:X}", ta_col_type); }
	
	ta_vertex v;
	v.x = std::bit_cast<float>(ta_q[1]);
	v.y = std::bit_cast<float>(ta_q[2]);
	v.z = std::bit_cast<float>(ta_q[3]);
	std::println("vertex in {},{}", v.x, v.y);
	if( ta_col_type == 1 )
	{
		v.r = std::bit_cast<float>(ta_q[5]);
		v.g = std::bit_cast<float>(ta_q[6]);
		v.b = std::bit_cast<float>(ta_q[7]);
	} else if( ta_col_type == 0 ) {
		u32 packed = ta_q[6];
		v.r = ((packed>>16)&0xff)/255.f;
		v.g = ((packed>>8)&0xff)/255.f;
		v.b = ((packed&0xff))/255.f;	
	}
	ta_vertq.push_back(v);
	
	if( ta_prim_type == 4 )
	{
		if( ta_vertq.size() == 3 )
		{
			ta_draw_tri();
		}	
	} else if( ta_prim_type == 5 ) {
		std::println("sprite vert recv");
	}
}

void dreamcast::ta_run()
{
	std::println("TA: got cmd = ${:X}", ta_q.front());
	u32 paractrl = ta_q[0];
	u32 ptype = paractrl>>29;
	if( ptype == 4 )
	{
		ta_prim_type = ptype;
		ta_col_type = (paractrl>>4)&3;
	} else if( ptype == 5 ) {
		ta_prim_type = ptype;
		ta_col_type = (paractrl>>4)&3;
	} else if( ptype == 7 ) {
		ta_vertex_in();
		if( paractrl & BIT(28) ) ta_vertq.clear();
	}
	

	ta_q.erase(ta_q.begin(), ta_q.begin()+8);
}

void dreamcast::ta_input(u32 v)
{
	//std::println("TA: Input = ${:X}", v);
	ta_q.push_back(v);
	
	if( ta_q.size() == 8 )
	{
		if( ta_q[0] == 0 && ta_q[1] == 0 ) { ta_q.pop_front(); ta_q.pop_front(); return; }
		ta_run();
	}
}

