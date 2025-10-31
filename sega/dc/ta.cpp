#include <print>
#include "dreamcast.h"

#define V0 (V&1)
#define V1 ((V&2)?1:0)
#define V2 ((V&4)?1:0)
#define V3 ((V&8)?1:0)
#define V4 ((V&16)?1:0)
#define V5 ((V&32)?1:0)
#define V6 ((V&64)?1:0)
#define V7 ((V&128)?1:0)
#define V8 ((V&256)?1:0)
#define V9 ((V&512)?1:0)
#define V10 ((V&1024)?1:0)
#define U0 (U&1)
#define U1 ((U&2)?1:0)
#define U2 ((U&4)?1:0)
#define U3 ((U&8)?1:0)
#define U4 ((U&16)?1:0)
#define U5 ((U&32)?1:0)
#define U6 ((U&64)?1:0)
#define U7 ((U&128)?1:0)
#define U8 ((U&256)?1:0)
#define U9 ((U&512)?1:0)
#define U10 ((U&1024)?1:0)

struct color4
{
	u8 r,g,b,a;
};

struct rend_context
{
	u8* fbuf;
	float* depth;
	int stride; // in pixels
	std::function<color4(u32 id, float u, float v)> texture_sample;// = [](u32, float, float)->u32{ return 0; };
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

	//a.p[2] = 1 / a.p[2];
	//b.p[2] = 1 / b.p[2];
	//c.p[2] = 1 / c.p[2];
	
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
				
				float Z = a.p[2] * wa + b.p[2] * wb + c.p[2] * wc;
				if( Z < ctx->depth[Y*ctx->stride + X] ) continue;
				ctx->depth[Y*ctx->stride + X] = Z;
				
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
				default: std::println("tri rendered with bpp={}", rendbpp); //exit(1);
				}
			}
		}
	}
}

void render_tri_tex(rend_context* ctx, rvertex a, rvertex b, rvertex c, u32 texid=0)
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
	
	//a.p[2] = 1 / a.p[2];
	//b.p[2] = 1 / b.p[2];
	//c.p[2] = 1 / c.p[2];
	a.t[0] *= a.p[2];
	a.t[1] *= a.p[2];
	b.t[0] *= b.p[2];
	b.t[1] *= b.p[2];
	c.t[0] *= c.p[2];
	c.t[1] *= c.p[2];
	
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
				
				float Z = (a.p[2] * wa + b.p[2] * wb + c.p[2] * wc);
				if( Z < ctx->depth[Y*ctx->stride + X] ) continue;
				ctx->depth[Y*ctx->stride + X] = Z;
				
				float U = a.t[0] * wa + b.t[0] * wb + c.t[0] * wc;
				float V = a.t[1] * wa + b.t[1] * wb + c.t[1] * wc;
				U *= 1/Z;
				V *= 1/Z;
				color4 C = ctx->texture_sample(texid, U, V);
				u16 p = (C.r>>3)<<11;
				p |= (C.g>>2)<<5;
				p |= (C.b>>3);
				((u16*)ctx->fbuf)[Y*ctx->stride + X] = p;
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
	v1.t[0] = a.u;
	v1.t[1] = a.v;
	
	v2.p[0] = b.x;
	v2.p[1] = b.y;
	v2.p[2] = b.z;
	v2.c[0] = b.r;
	v2.c[1] = b.g;
	v2.c[2] = b.b;
	v2.t[0] = b.u;
	v2.t[1] = b.v;

	v3.p[0] = c.x;
	v3.p[1] = c.y;
	v3.p[2] = c.z;
	v3.c[0] = c.r;
	v3.c[1] = c.g;
	v3.c[2] = c.b;
	v3.t[0] = c.u;
	v3.t[1] = c.v;
	
	rend_context ctx;
	ctx.clip_x1 = ctx.clip_y1 = 0;
	ctx.clip_x2 = 640;
	ctx.clip_y2 = 480;
	ctx.stride = 640;
	ctx.fbuf = &vram[holly.fb_w_sof1&0x7fffff];
	ctx.depth = depth;
	//std::println("Rendered TRI! {},{}, {},{}, {},{}", v1.p[0], v1.p[1], v2.p[0], v2.p[1], v3.p[0], v3.p[1]);
	//if( ta_obj_ctrl & BIT(3) )
	//{
	//	std::println("\t with UV ({}, {}) ({}, {}) ({}, {})", v1.t[0], v1.t[1], v2.t[0], v2.t[1], v3.t[0], v3.t[1]);
	//	std::println("\t tex ctrl = ${:X}, twiddled = {:X}", ta_tex_ctrl, ta_tex_ctrl&BIT(26));
	//}
	rendbpp = (holly.fb_r_ctrl>>2)&3;
	if( ta_clear )
	{
		ta_clear = false;
		u32 mult = 2;
		if( rendbpp == 2 ) mult = 3;
		if( rendbpp == 3 ) mult = 4;
		memset(vram+(holly.fb_w_sof1&0x7ffffc), 0, 640*480*mult);
		for(u32 i = 0; i < 640*480; ++i) depth[i] = std::bit_cast<float>(isp.backgnd_d);
		//std::println("clearing vram wsof=${:X}, rsof=${:X}", holly.fb_w_sof1, holly.fb_r_sof1);
	}
	if( ta_obj_ctrl & BIT(3) )
	{
		static const u32 uvsize[] = { 8, 16, 32, 64, 128, 256, 512, 1024 };
		auto texsamp = [&](u32, float u, float v)->color4 {
			u32 addr = ta_tex_ctrl&0x1fffff;
			addr <<= 3;
			u32 W = uvsize[(ta_tsp_mode>>3)&7];
			u32 H = uvsize[ta_tsp_mode&7];
			u32 U = std::clamp(u*W, 0.f, (float)W);
			u32 V = std::clamp(v*H, 0.f, (float)H);
			
			if( ta_tex_ctrl & BIT(26) )
			{
				//std::println("nontwiddle");
				//exit(1);
				if( ta_tex_ctrl & BIT(25) ) W = (holly.text_control&0x1f)*32;
				addr += V*W*2 + U*2;
			} else {
				if( W == H )
				{
					u32 r = V0|(U0<<1)|(V1<<2)|(U1<<3)|(V2<<4)|(U2<<5)|(V3<<6)|(U3<<7)|(V4<<8)|(U4<<9)|
						(V5<<10)|(U5<<11)|(V6<<12)|(U6<<13)|(V7<<14)|(U7<<15)|
						(V8<<16)|(U8<<17)|(V9<<18)|(U9<<19)|(V10<<20)|(U10<<21);
					addr += r<<1;
				} else {
					std::println("Use of {}x{} twiddled texture.", W, H);
					exit(1);
				}
			}

			u16 p = *(u16*)&vram[((addr&0xFFFFF8)>>1)|((addr&4)<<20)|(addr&3)];
			color4 res;
			
			switch( (ta_tex_ctrl>>27) & 7 )
			{
			case 7:
			case 0: // 1555
				res.r = ((p>>10)&31)<<3;
				res.g = ((p>>5)&31)<<3;
				res.b = ((p&31)<<3);
				res.a = ((p&0x8000) ? 0xff : 0);
				break;
			case 1: // 565
				res.r = ((p>>11)&31)<<3;
				res.g = ((p>>5)&63)<<2;
				res.b = (p&31)<<3;
				res.a = 0xff;
				break;
			case 2: // 4444
				res.r = ((p>>8)&15)<<4;
				res.g = ((p>>4)&15)<<4;
				res.b = ((p>>0)&15)<<4;
				res.a = ((p>>12)&15)<<4;
				break;
			default:
				std::println("Unimpl tex format {}", (ta_tex_ctrl>>27)&7);
				exit(1);
			}
			return res;
		};
		ctx.texture_sample = texsamp;
		//u32 addr = ta_tex_ctrl&0x1fffff;
		//addr <<= 3;
		//std::println("Texture address = ${:X}", addr); // calogo 3B6740  // bluelight 39C780
		render_tri_tex(&ctx, v1,v2,v3);
	} else {
		render_tri_shaded(&ctx, v1,v2,v3 );
	}
}

void dreamcast::ta_vertex_in()
{
	if( ta_col_type > 1 ) { std::println("ta_col_type = ${:X}", ta_col_type); }
	
	ta_vertex v;
	v.x = std::bit_cast<float>(ta_q[1]);
	v.y = std::bit_cast<float>(ta_q[2]);
	v.z = std::bit_cast<float>(ta_q[3]);
	//std::println("vertex in {},{}", v.x, v.y);
	if( ta_obj_ctrl & BIT(3) )
	{ //todo: 16bit packed
		v.u = std::bit_cast<float>(ta_q[4]);
		v.v = std::bit_cast<float>(ta_q[5]);
	}
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
	//std::println("TA: got cmd = ${:X}", ta_q.front());
	u32 paractrl = ta_q[0];
	u32 ptype = paractrl>>29;
	if( ptype == 4 )
	{
		//std::println("PRIM List ${:X}", paractrl);
		ta_prim_type = ptype;
		ta_col_type = (paractrl>>4)&3;
		ta_obj_ctrl = paractrl&0xffff;
		ta_tex_ctrl = ta_q[3];
		ta_tsp_mode = ta_q[2];
		ta_active_list = (paractrl>>24)&7;
	} else if( ptype == 5 ) {
		ta_prim_type = ptype;
		ta_col_type = (paractrl>>4)&3;
		ta_obj_ctrl = paractrl&0xffff;
		ta_tex_ctrl = ta_q[3];
		ta_tsp_mode = ta_q[2];
	} else if( ptype == 7 ) {
		ta_vertex_in();
		if( paractrl & BIT(28) ) ta_vertq.clear();
	} else if( ptype == 0 ) {
		//todo: list types other than opaque and transparent
		u32 endlisttype = ta_active_list;
		if( ta_active_list == -1 )
		{
			std::println("Ended list with no active list");
			return;
		}
		//std::println("End list {}", endlisttype);
		if( endlisttype == 0 )
		{
			holly.sb_istnrm |= OPAQUE_LIST_CMPL_IRQ_BIT;
		} else if( endlisttype == 2 ) {
			holly.sb_istnrm |= TRANSP_LIST_CMPL_IRQ_BIT;
		} else if( endlisttype == 4 ) {
			holly.sb_istnrm |= BIT(21); // end list punch thru		
		} else {
			std::println("Unsupported list completion {}", endlisttype);
			exit(1);
		}
	}
	
	ta_q.erase(ta_q.begin(), ta_q.begin()+8);
}

void dreamcast::ta_input(u32 v)
{
	//std::println("TA: Input = ${:X}", v);
	ta_q.push_back(v);
	
	if( ta_q.size() == 8 )
	{
		//if( ta_q[0] == 0 && ta_q[1] == 0 ) { ta_q.pop_front(); ta_q.pop_front(); return; }
		ta_run();
	}
}






