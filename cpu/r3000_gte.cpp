#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <bit>
#include "r3000.h"

#define LOAD(R, V) ld_r[1] = (R); ld_v[1] = (V)
#define DEST(R) if( (R) == ld_r[0] ) ld_r[0] = ld_v[0] = 0

#define S ((opc>>21)&0x1F)
#define T ((opc>>16)&0x1F)
#define imm5 ((opc>>6)&0x1F)
#define imm16 (opc&0xffff)
#define simm16 s16(u16(opc&0xffff))

#define LM ((opc>>10)&1)
#define SF ((opc>>19)&1)

s64 r3000::clamp(s64 v, s64 min, s64 max, u32 f)
{
	if( min > v )
	{
		flags |= (1u<<f);
		return min;
	} else if( max < v ) {
		flags |= (1u<<f);
		return max;
	}
	return v;
}

s64 r3000::A(u32 index, s64 v)
{
	if( v < -(1LL<<43) )
	{
		flags |= 0x8000000u >> (index - 1);
        }

        if( v >= (1LL<<43) )
        {
		flags |= 0x40000000u >> (index - 1);
        }

	v <<= 20;
	v >>= 20;
	return v;
}

s16 r3000::Bz(s32 value, s64 old, u32 lm)
{
        if( old < -0x8000 )
        {
		flags |= 0x400000;
        }

        if( old > 0x7fff ) 
        {
		flags |= 0x400000;
        }

        if( lm && value < 0 )
        {
		return 0;
        }

        if( !lm && value < -0x8000 )
        {
		return -0x8000;
        }

        if( value > 0x7fff )
        {
		return 0x7fff;
        }

	return value;
}

s16 r3000::B(s64 value, u32 lm, u32 index)
{
	if( lm && value < 0 )
	{
		flags |= 0x1000000 >> (index - 1);
		return 0;
        }

        if( !lm && value < -0x8000 ) 
        {
		flags |= 0x1000000 >> (index - 1);
		return -0x8000;
        }

        if( value > 0x7fff )
        {
		flags |= 0x1000000 >> (index - 1);
		return 0x7fff;
        }

	return value;
}

u16 r3000::D(s64 value) 
{
        if( value < 0 )
        {
		flags |= 0x40000;
		return 0;
        }

        if( value > 0xffff ) 
        {
		flags |= 0x40000;
		return 0xffff;
        }

	return value;
}

s64 r3000::F(s64 value) 
{
        if( value < -0x80000000ll )
        {
		flags |= 0x8000;
        } else if( value > 0x7fffffffll ) {
		flags |= 0x10000;
        }

	return value;
}

s16 r3000::G(s32 value, u32 index)
{
	if( value < -0x400 )
	{
		flags |= 0x4000 >> (index - 1);
		return -0x400;
	}

        if( value > 0x3ff )
        {
		flags |= 0x4000 >> (index - 1);
		return 0x3ff;
        }

	return value;
}

s16 r3000::limH(s64 value)
{
	if( value < 0 )
	{
		flags |= 0x1000;
		return 0;
	}

        if( value > 0x1000 )
        {
		flags |= 0x1000;
		return 0x1000;
        }

	return value;
}

u8 r3000::C(u32 index, s32 value)
{
	if( value < 0 )
	{
		flags |= 0x200000 >> (index - 1);
		return 0;
        }

        if( value > 0xff )
        {
		flags |= 0x200000 >> (index - 1);
		return 0xff;
        }

	return value;
}

s32 r3000::macclamp(s64 v)
{
	if( v > 0x7fffFFFFll ) return 0x7fffFFFF;
	else if( v < -0x80000000ll ) return -0x80000000;
	return v;
}

void r3000::RTP(u32 opc, u32 index, bool dq)
{
	u32 sf = SF*12;
        
        s64 tr_x = s64(TRX) << 12;
        s64 tr_y = s64(TRY) << 12;
        s64 tr_z = s64(TRZ) << 12;

        s64 temp[3];

	s64 vx = s64( (index == 0)? VX0 : ((index == 1) ? VX1 : VX2) );
	s64 vy = s64( (index == 0)? VY0 : ((index == 1) ? VY1 : VY2) );
	s64 vz = s64( (index == 0)? VZ0 : ((index == 1) ? VZ1 : VZ2) );

        temp[0] = A(1, tr_x + s64(RT11) * s64(vx));
        temp[1] = A(2, tr_y + s64(RT21) * s64(vx));
        temp[2] = A(3, tr_z + s64(RT31) * s64(vx));

        temp[0] = A(1, temp[0] + s64(RT12) * s64(vy));
        temp[1] = A(2, temp[1] + s64(RT22) * s64(vy));
        temp[2] = A(3, temp[2] + s64(RT32) * s64(vy));

        temp[0] = A(1, temp[0] + s64(RT13) * s64(vz));
        temp[1] = A(2, temp[1] + s64(RT23) * s64(vz));
        temp[2] = A(3, temp[2] + s64(RT33) * s64(vz));

        MAC1 = (temp[0] >> sf);
        MAC2 = (temp[1] >> sf);
        tr_z = temp[2];
        MAC3 = (tr_z >> sf);

        s64 zs = tr_z >> 12;

        IR1 = B(MAC1, LM, 1);
        IR2 = B(MAC2, LM, 2);
        IR3 = Bz(MAC3, zs, LM);

        u16 sz3 = D(zs);

        SZ0 = SZ1; SZ1 = SZ2; SZ2 = SZ3; SZ3 = sz3;

        u32 h_div_sz = 0;

        if( sz3 > (H / 2) )
        {
   		h_div_sz = strange_div(H, sz3); //         h_div_sz = Gte::divide(self.h, sz3);
        } else {
		flags |= 0x20000;
		h_div_sz = 0x1ffff;
        }

        auto sx2 = s64(OFX) + s64(IR1) * s64(h_div_sz);
        auto sx2_f = F(sx2) >> 16;
        auto sx2_f_g = G(s32(sx2_f), 1);

	SX0 = SX1; SX1 = SX2; SX2 = sx2_f_g;

        auto sy2 = s64(OFY) + s64(IR2)*s64(h_div_sz);
        auto sy2_f = F(sy2) >> 16;
       	auto sy2_f_g = G(s32(sy2_f), 2);
        
	SY0 = SY1; SY1 = SY2; SY2 = sy2_f_g;

        if( dq )
        {
		s64 depth = s64(DQB) + s64(DQA)*s64((s32)h_div_sz);
		MAC0 = F(depth);
		IR0 = limH(depth >> 12);
        }
}

void r3000::RTPS(u32 opc)
{
	RTP(opc, 0, true);
}

void r3000::RTPT(u32 opc)
{
	RTP(opc, 0, false);
	RTP(opc, 1, false);
	RTP(opc, 2, true);
}

void r3000::AVZ4(u32 )
{
	s64 temp = s64(ZSF4)*(s64(SZ0)+s64(SZ1)+s64(SZ2)+s64(SZ3));
	MAC0 = F(temp);
	OTZ = D(temp>>12);
}

void r3000::AVZ3(u32 )
{
	s64 temp = s64(ZSF3)*(s64(SZ1)+s64(SZ2)+s64(SZ3));
	MAC0 = F(temp);
	OTZ = D(temp>>12);
}

void r3000::NCLIP(u32 )
{
	flags = 0;
	s64 temp = (s64(SX0)*SY1);
	temp = (temp + s64(SX1)*SY2);
	temp = (temp + s64(SX2)*SY0);
	temp = (temp - s64(SX0)*SY2);
	temp = (temp - s64(SX1)*SY0);
	temp = (temp - s64(SX2)*SY1);
	MAC0 = F(temp);
}

void r3000::NCDS(u32 opc)
{
	NCD(opc, 0);
}

void r3000::NCD(u32 opc, u32 index) 
{
	s64 vx = s64( index == 0 ? VX0 : (index==1 ? VX1 : VX2) );
	s64 vy = s64( index == 0 ? VY0 : (index==1 ? VY1 : VY2) );
	s64 vz = s64( index == 0 ? VZ0 : (index==1 ? VZ1 : VZ2) );
	
	s64 temp[3];

        temp[0] = A(1, L11 * vx);
        temp[1] = A(2, L21 * vx);
        temp[2] = A(3, L31 * vx);

        temp[0] = A(1, temp[0] + L12 * vy);
        temp[1] = A(2, temp[1] + L22 * vy);
        temp[2] = A(3, temp[2] + L32 * vy);

        temp[0] = A(1, temp[0] + L13 * vz);
        temp[1] = A(2, temp[1] + L23 * vz);
        temp[2] = A(3, temp[2] + L33 * vz);

        MAC1 = (temp[0] >> (SF*12));
        MAC2 = (temp[1] >> (SF*12));
        MAC3 = (temp[2] >> (SF*12));

        IR1 = B(MAC1, LM, 1);
        IR2 = B(MAC2, LM, 2);
        IR3 = B(MAC3, LM, 3);

        s64 rbk = s64(RBK) << 12;
        s64 gbk = s64(GBK) << 12;
        s64 bbk = s64(BBK) << 12;

        s64 ir1 = s64(IR1);
        s64 ir2 = s64(IR2);
        s64 ir3 = s64(IR3);

        temp[0] = A(1, rbk + s64(LR1) * ir1);
        temp[1] = A(2, gbk + s64(LG1) * ir1);
        temp[2] = A(3, bbk + s64(LB1) * ir1);

        temp[0] = A(1, temp[0] + s64(LR2) * ir2);
        temp[1] = A(2, temp[1] + s64(LG2) * ir2);
        temp[2] = A(3, temp[2] + s64(LB2) * ir2);

        temp[0] = A(1, temp[0] + s64(LR3) * ir3);
        temp[1] = A(2, temp[1] + s64(LG3) * ir3);
        temp[2] = A(3, temp[2] + s64(LB3) * ir3);

        MAC1 = (temp[0] >> (SF*12));
        MAC2 = (temp[1] >> (SF*12));
        MAC3 = (temp[2] >> (SF*12));

        IR1 = B(MAC1, LM, 1);
        IR2 = B(MAC2, LM, 2);
        IR3 = B(MAC3, LM, 3);

        s64 prev_ir1 = s64(IR1);
        s64 prev_ir2 = s64(IR2);
        s64 prev_ir3 = s64(IR3);

        s64 R = s64(RGBC[0]) << 4;
        s64 g = s64(RGBC[1]) << 4; //(self.rgb.g as i64) << 4;
        s64 b = s64(RGBC[2]) << 4; //(self.rgb.b as i64) << 4;

        s64 rfc = s64(RFC)<<12; // (self.fc.x as i64) << 12;
        s64 gfc = s64(GFC)<<12; //(self.fc.y as i64) << 12;
        s64 bfc = s64(BFC)<<12; //(self.fc.z as i64) << 12;

        MAC1 = (A(1, rfc - R * prev_ir1) >> (SF*12));
        MAC2 = (A(2, gfc - g * prev_ir2) >> (SF*12));
        MAC3 = (A(3, bfc - b * prev_ir3) >> (SF*12));

        IR1 = B(MAC1, false, 1);
        IR2 = B(MAC2, false, 2);
        IR3 = B(MAC3, false, 3);

        MAC1 = (A(1, (R * prev_ir1) + s64(IR0) * s64(IR1)) >> (SF*12));
        MAC2 = (A(2, (g * prev_ir2) + s64(IR0) * s64(IR2)) >> (SF*12));
        MAC3 = (A(3, (b * prev_ir3) + s64(IR0) * s64(IR3)) >> (SF*12));

        IR1 = B(MAC1, LM, 1);
        IR2 = B(MAC2, LM, 2);
        IR3 = B(MAC3, LM, 3);

	RGB0[0] = RGB1[0];
	RGB0[1] = RGB1[1];
	RGB0[2] = RGB1[2];
	RGB0[3] = RGB1[3];
	RGB1[0] = RGB2[0];
	RGB1[1] = RGB2[1];
	RGB1[2] = RGB2[2];
	RGB1[3] = RGB2[3];

        RGB2[0] = C(1, MAC1 >> 4);
        RGB2[1] = C(2, MAC2 >> 4);
        RGB2[2] = C(3, MAC3 >> 4);
        RGB2[3] = RGBC[3];
}

void r3000::OP(u32 opc)
{
	s64 temp[3];
	temp[0] = s64(IR3)*s64(RT22) - s64(IR2)*s64(RT33);
	temp[1] = s64(IR1)*s64(RT33) - s64(IR3)*s64(RT11);
	temp[2] = s64(IR2)*s64(RT11) - s64(IR1)*s64(RT22);
	MAC1 = temp[0] >> (SF*12);
	MAC2 = temp[1] >> (SF*12);
	MAC3 = temp[2] >> (SF*12);
	IR1 = B(MAC1, LM, 1);
	IR2 = B(MAC2, LM, 2);
	IR3 = B(MAC3, LM, 3);
}

#define VX(a) s64((a==0) ? VX0 : (a==1) ? VX1 : (a==2) ? VX2 : s64(IR1))
#define VY(a) s64((a==0) ? VY0 : (a==1) ? VY1 : (a==2) ? VY2 : s64(IR2))
#define VZ(a) s64((a==0) ? VZ0 : (a==1) ? VZ1 : (a==2) ? VZ2 : s64(IR3))
#define MX11(a) s64((a==0) ? RT11 : (a==1) ? s64(L11) : (a==2) ? LR1 : -(RGBC[0]<<4))
#define MX12(a) s64((a==0) ? RT12 : (a==1) ? s64(L12) : (a==2) ? LR2 : s16(u16(RGBC[0])<<4))
#define MX13(a) s64((a==0) ? RT13 : (a==1) ? s64(L13) : (a==2) ? LR3 : s64(IR0))
#define MX21(a) s64((a==0) ? RT21 : (a==1) ? s64(L21) : (a==2) ? LG1 : RT13)
#define MX22(a) s64((a==0) ? RT22 : (a==1) ? s64(L22) : (a==2) ? LG2 : RT13)
#define MX23(a) s64((a==0) ? RT23 : (a==1) ? s64(L23) : (a==2) ? LG3 : RT13)
#define MX31(a) s64((a==0) ? RT31 : (a==1) ? s64(L31) : (a==2) ? LB1 : RT22)
#define MX32(a) s64((a==0) ? RT32 : (a==1) ? s64(L32) : (a==2) ? LB2 : RT22)
#define MX33(a) s64((a==0) ? RT33 : (a==1) ? s64(L33) : (a==2) ? LB3 : RT22)

#define CV1(a) s64((a==0) ? s64(TRX) : (a==1) ? s64(RBK) : (a==2 ? s64(RFC) : 0))
#define CV2(a) s64((a==0) ? s64(TRY) : (a==1) ? s64(GBK) : (a==2 ? s64(GFC) : 0))
#define CV3(a) s64((a==0) ? s64(TRZ) : (a==1) ? s64(BBK) : (a==2 ? s64(BFC) : 0))

void r3000::MVMVA(u32 opc)
{
	int mx = (opc >> 17) & 3;
	int v = (opc >> 15) & 3;
	int cv = (opc >> 13) & 3;
        s64 tr_x = CV1(cv) << 12;
        s64 tr_y = CV2(cv) << 12;
        s64 tr_z = CV3(cv) << 12;
	s64 vx = VX(v);
	s64 vy = VY(v);
	s64 vz = VZ(v);

        s64 temp[3];
	if( cv == 2 )
	{
		B( A(1, (tr_x + MX11(mx) * vx)) >>(SF*12) , 0, 1);
		B( A(2, (tr_y + MX21(mx) * vx)) >>(SF*12) , 0, 2);
		B( A(3, (tr_z + MX31(mx) * vx)) >>(SF*12) , 0, 3);
        	temp[0] = A(1, A(1, MX12(mx) * vy) + MX13(mx) * vz);
        	temp[1] = A(2, A(2, MX22(mx) * vy) + MX23(mx) * vz);
        	temp[2] = A(3, A(3, MX32(mx) * vy) + MX33(mx) * vz);
		MAC1 = (temp[0] >> (SF*12));
		MAC2 = (temp[1] >> (SF*12));
		MAC3 = (temp[2] >> (SF*12));
		IR1 = B(MAC1, LM, 1);
        	IR2 = B(MAC2, LM, 2);
        	IR3 = B(MAC3, LM, 3);	
	} else {
		temp[0] = A(1, tr_x + MX11(mx) * vx);
		temp[1] = A(2, tr_y + MX21(mx) * vx);
		temp[2] = A(3, tr_z + MX31(mx) * vx);
		temp[0] = A(1, temp[0] + MX12(mx) * vy);
		temp[1] = A(2, temp[1] + MX22(mx) * vy);
		temp[2] = A(3, temp[2] + MX32(mx) * vy);
		temp[0] = A(1, temp[0] + MX13(mx) * vz);
		temp[1] = A(2, temp[1] + MX23(mx) * vz);
		temp[2] = A(3, temp[2] + MX33(mx) * vz);
		MAC1 = (temp[0] >> (SF*12));
		MAC2 = (temp[1] >> (SF*12));
		MAC3 = (temp[2] >> (SF*12));
	        IR1 = B(MAC1, LM, 1);
        	IR2 = B(MAC2, LM, 2);
        	IR3 = B(MAC3, LM, 3);
	}
	
}

void r3000::NCC(u32 opc, u32 index)
{
	s64 vx = VX(index);
        s64 vy = VY(index);
        s64 vz = VZ(index);

	// light * vert
        s64 temp[3];
        temp[0] = A(1, s64(L11) * vx);
        temp[1] = A(2, s64(L21) * vx);
        temp[2] = A(3, s64(L31) * vx);
        temp[0] = A(1, temp[0] + s64(L12) * vy);
        temp[1] = A(2, temp[1] + s64(L22) * vy);
        temp[2] = A(3, temp[2] + s64(L32) * vy);
        temp[0] = A(1, temp[0] + s64(L13) * vz);
        temp[1] = A(2, temp[1] + s64(L23) * vz);
        temp[2] = A(3, temp[2] + s64(L33) * vz);
        MAC1 = (temp[0] >> (SF*12));
        MAC2 = (temp[1] >> (SF*12));
        MAC3 = (temp[2] >> (SF*12));
        IR1 = B(MAC1, LM, 1);
        IR2 = B(MAC2, LM, 2);
        IR3 = B(MAC3, LM, 3);

	// color * ir + bk
        s64 rbk = s64(RBK) << 12;
        s64 gbk = s64(GBK) << 12;
        s64 bbk = s64(BBK) << 12;
        s64 ir1 = s64(IR1);
        s64 ir2 = s64(IR2);
        s64 ir3 = s64(IR3);
        temp[0] = A(1, rbk + LR1 * ir1);
        temp[1] = A(2, gbk + LG1 * ir1);
        temp[2] = A(3, bbk + LB1 * ir1);
        temp[0] = A(1, temp[0] + LR2 * ir2);
        temp[1] = A(2, temp[1] + LG2 * ir2);
        temp[2] = A(3, temp[2] + LB2 * ir2);
        temp[0] = A(1, temp[0] + LR3 * ir3);
        temp[1] = A(2, temp[1] + LG3 * ir3);
        temp[2] = A(3, temp[2] + LB3 * ir3);
        MAC1 = (temp[0] >> (SF*12));
        MAC2 = (temp[1] >> (SF*12));
        MAC3 = (temp[2] >> (SF*12));
        IR1 = B(MAC1, LM, 1);
        IR2 = B(MAC2, LM, 2);
        IR3 = B(MAC3, LM, 3);

        s64 R = u64(RGBC[0]) << 4;
        s64 g = u64(RGBC[1]) << 4;
        s64 b = u64(RGBC[2]) << 4;

        ir1 = s64(IR1);
        ir2 = s64(IR2);
        ir3 = s64(IR3);
        
        MAC1 = (A(1, R * ir1) >> (SF*12));
        MAC2 = (A(2, g * ir2) >> (SF*12));
        MAC3 = (A(3, b * ir3) >> (SF*12));

        IR1 = B(MAC1, LM, 1);
        IR2 = B(MAC2, LM, 2);
        IR3 = B(MAC3, LM, 3);

        R = C(1, MAC1 >> 4);
        g = C(2, MAC2 >> 4);
        b = C(3, MAC3 >> 4);
        RGB0[0] = RGB1[0];
        RGB0[1] = RGB1[1];
        RGB0[2] = RGB1[2];
        RGB0[3] = RGB1[3];
        RGB1[0] = RGB2[0];
        RGB1[1] = RGB2[1];
        RGB1[2] = RGB2[2];
        RGB1[3] = RGB2[3];
	RGB2[0] = R;
	RGB2[1] = g;
	RGB2[2] = b;
	RGB2[3] = RGBC[3];
}

void r3000::DPCS(u32 opc)
{
	s64 R = RGBC[0]<<16;
	s64 G = RGBC[1]<<16;	
	s64 b = RGBC[2]<<16;
	
        MAC1 = (A(1, (s64((s32)RFC)<<12) - R) >> (SF*12));
        MAC2 = (A(2, (s64((s32)GFC)<<12) - G) >> (SF*12));
        MAC3 = (A(3, (s64((s32)BFC)<<12) - b) >> (SF*12));

        IR1 = B(MAC1, false, 1);
        IR2 = B(MAC2, false, 2);
        IR3 = B(MAC3, false, 3);

        MAC1 = (A(1, R + s64(IR1) * IR0) >> (SF*12));
        MAC2 = (A(2, G + s64(IR2) * IR0) >> (SF*12));
        MAC3 = (A(3, b + s64(IR3) * IR0) >> (SF*12));

        IR1 = B(MAC1, LM, 1);
        IR2 = B(MAC2, LM, 2);
        IR3 = B(MAC3, LM, 3);
	
        RGB0[0] = RGB1[0];
        RGB0[1] = RGB1[1];
        RGB0[2] = RGB1[2];
        RGB0[3] = RGB1[3];
        RGB1[0] = RGB2[0];
        RGB1[1] = RGB2[1];
        RGB1[2] = RGB2[2];
        RGB1[3] = RGB2[3];
	
	RGB2[0] = C(1, MAC1 >> 4);
	RGB2[1] = C(2, MAC2 >> 4);
	RGB2[2] = C(3, MAC3 >> 4); 
	RGB2[3] = RGBC[3];
}

void r3000::INTPL(u32 opc)
{
	s64 R = s64(IR1)<<12;
	s64 G = s64(IR2)<<12;	
	s64 b = s64(IR3)<<12;
	
        MAC1 = (A(1, (s64((s32)RFC)<<12) - R) >> (SF*12));
        MAC2 = (A(2, (s64((s32)GFC)<<12) - G) >> (SF*12));
        MAC3 = (A(3, (s64((s32)BFC)<<12) - b) >> (SF*12));

        IR1 = B(MAC1, false, 1);
        IR2 = B(MAC2, false, 2);
        IR3 = B(MAC3, false, 3);

        MAC1 = (A(1, R + s64(IR1) * IR0) >> (SF*12));
        MAC2 = (A(2, G + s64(IR2) * IR0) >> (SF*12));
        MAC3 = (A(3, b + s64(IR3) * IR0) >> (SF*12));

        IR1 = B(MAC1, LM, 1);
        IR2 = B(MAC2, LM, 2);
        IR3 = B(MAC3, LM, 3);
	
        RGB0[0] = RGB1[0];
        RGB0[1] = RGB1[1];
        RGB0[2] = RGB1[2];
        RGB0[3] = RGB1[3];
        RGB1[0] = RGB2[0];
        RGB1[1] = RGB2[1];
        RGB1[2] = RGB2[2];
        RGB1[3] = RGB2[3];
	
	RGB2[0] = C(1, MAC1 >> 4);
	RGB2[1] = C(2, MAC2 >> 4);
	RGB2[2] = C(3, MAC3 >> 4); 
	RGB2[3] = RGBC[3];
}

void r3000::GPF(u32 opc)
{
	//[MAC1,MAC2,MAC3] = (([IR1,IR2,IR3] * IR0) + [0,0,0]) SAR (sf*12)
	s64 temp[3];
	temp[0] = A(1, s64(IR1) * s64(IR0)) >> (SF*12);
	temp[1] = A(2, s64(IR2) * s64(IR0)) >> (SF*12);
	temp[2] = A(3, s64(IR3) * s64(IR0)) >> (SF*12);

	MAC1 = temp[0];
	MAC2 = temp[1];
	MAC3 = temp[2];

	IR1 = B(MAC1, LM, 1);
	IR2 = B(MAC2, LM, 2);
	IR3 = B(MAC3, LM, 3);

	*(u32*)&RGB0[0] = *(u32*)&RGB1[0];
	*(u32*)&RGB1[0] = *(u32*)&RGB2[0];
	RGB2[0] = C(1, MAC1 >> 4);
	RGB2[1] = C(2, MAC2 >> 4);
	RGB2[2] = C(3, MAC3 >> 4); 
	RGB2[3] = RGBC[3];
}

void r3000::SQR(u32 opc)
{
	MAC1 = A(1, s64(IR1)*IR1) >> (SF*12);
	MAC2 = A(2, s64(IR2)*IR2) >> (SF*12);
	MAC3 = A(3, s64(IR3)*IR3) >> (SF*12);
	IR1 = B(MAC1, 0, 1);
	IR2 = B(MAC2, 0, 2);
	IR3 = B(MAC3, 0, 3);
}

void r3000::CDP(u32 opc)
{
	MAC1 = A(1, A(1, A(1, (s64(RBK)<<12) + (s64(LR1)*s64(IR1))) + (s64(LR2)*s64(IR2))) + (s64(LR3)*s64(IR3)))>>(SF*12);
	MAC2 = A(2, A(2, A(2, (s64(GBK)<<12) + (s64(LG1)*s64(IR1))) + (s64(LG2)*s64(IR2))) + (s64(LG3)*s64(IR3)))>>(SF*12);
	MAC3 = A(3, A(3, A(3, (s64(BBK)<<12) + (s64(LB1)*s64(IR1))) + (s64(LB2)*s64(IR2))) + (s64(LB3)*s64(IR3)))>>(SF*12);
	IR1 = B(MAC1, LM, 1);
	IR2 = B(MAC2, LM, 2);
	IR3 = B(MAC3, LM, 3);
	s64 ir1 =  B( (s32) (A(1, ((s64(RFC) << 12) - ((s64(RGBC[0] << 4)) * s64(IR1))))>>(SF*12)), 0, 1);
	s64 ir2 =  B( (s32) (A(2, ((s64(GFC) << 12) - ((s64(RGBC[1] << 4)) * s64(IR2))))>>(SF*12)), 0, 2);
	s64 ir3 =  B( (s32) (A(3, ((s64(BFC) << 12) - ((s64(RGBC[2] << 4)) * s64(IR3))))>>(SF*12)), 0, 3);

	MAC1 = A(1, (s64(RGBC[0] << 4) * s64(IR1)) + (s64(IR0) * ir1)) >> (SF*12);
	MAC2 = A(2, (s64(RGBC[1] << 4) * s64(IR2)) + (s64(IR0) * ir2)) >> (SF*12);
	MAC3 = A(3, (s64(RGBC[2] << 4) * s64(IR3)) + (s64(IR0) * ir3)) >> (SF*12);
	IR1 = B(MAC1, LM, 1);
	IR2 = B(MAC2, LM, 2);
	IR3 = B(MAC3, LM, 3);
	*(u32*)&RGB0[0] = *(u32*)&RGB1[0];
	*(u32*)&RGB1[0] = *(u32*)&RGB2[0];
	RGB2[0] = C(1, MAC1>>4);
	RGB2[1] = C(2, MAC2>>4);
	RGB2[2] = C(3, MAC3>>4);
	RGB2[3] = RGBC[3];
}

void r3000::CC(u32 opc)
{
	MAC1 = A(1, A(1, A(1, (s64(RBK)<<12) + (s64(LR1)*s64(IR1))) + (s64(LR2)*s64(IR2))) + (s64(LR3)*s64(IR3)))>>(SF*12);
	MAC2 = A(2, A(2, A(2, (s64(GBK)<<12) + (s64(LG1)*s64(IR1))) + (s64(LG2)*s64(IR2))) + (s64(LG3)*s64(IR3)))>>(SF*12);
	MAC3 = A(3, A(3, A(3, (s64(BBK)<<12) + (s64(LB1)*s64(IR1))) + (s64(LB2)*s64(IR2))) + (s64(LB3)*s64(IR3)))>>(SF*12);
	IR1 = B(MAC1, LM, 1);
	IR2 = B(MAC2, LM, 2);
	IR3 = B(MAC3, LM, 3);
	
	MAC1 = A(1, (s64(RGBC[0] << 4)) * s64(IR1))>>(SF*12);
	MAC2 = A(2, (s64(RGBC[1] << 4)) * s64(IR2))>>(SF*12);
	MAC3 = A(3, (s64(RGBC[2] << 4)) * s64(IR3))>>(SF*12);

	IR1 = B(MAC1, LM, 1);
	IR2 = B(MAC2, LM, 2);
	IR3 = B(MAC3, LM, 3);
	*(u32*)&RGB0[0] = *(u32*)&RGB1[0];
	*(u32*)&RGB1[0] = *(u32*)&RGB2[0];
	RGB2[0] = C(1, MAC1>>4);
	RGB2[1] = C(2, MAC2>>4);
	RGB2[2] = C(3, MAC3>>4);
	RGB2[3] = RGBC[3];
}

void r3000::NC(u32 opc, u32 index)
{
        s64 temp[3];
        s64 vx = VX(index);
        s64 vy = VY(index);
        s64 vz = VZ(index);
        temp[0] = A(1, L11 * vx);
        temp[1] = A(2, L21 * vx);
        temp[2] = A(3, L31 * vx);
        temp[0] = A(1, temp[0] + L12 * vy);
        temp[1] = A(2, temp[1] + L22 * vy);
        temp[2] = A(3, temp[2] + L32 * vy);
        temp[0] = A(1, temp[0] + L13 * vz);
        temp[1] = A(2, temp[1] + L23 * vz);
        temp[2] = A(3, temp[2] + L33 * vz);

        MAC1 = (temp[0] >> (SF*12));
	MAC2 = (temp[1] >> (SF*12));
	MAC3 = (temp[2] >> (SF*12));

        IR1 = B(MAC1, LM, 1);
        IR2 = B(MAC2, LM, 2);
        IR3 = B(MAC3, LM, 3);

        s64 rbk = s64(RBK) << 12;
        s64 gbk = s64(GBK) << 12;
        s64 bbk = s64(BBK) << 12;

	s64 ir1 = IR1;
	s64 ir2 = IR2;
	s64 ir3 = IR3;

        temp[0] = A(1, rbk + LR1 * ir1);
        temp[1] = A(2, gbk + LG1 * ir1);
        temp[2] = A(3, bbk + LB1 * ir1);

        temp[0] = A(1, temp[0] + LR2 * ir2);
        temp[1] = A(2, temp[1] + LG2 * ir2);
        temp[2] = A(3, temp[2] + LB2 * ir2);

        temp[0] = A(1, temp[0] + LR3 * ir3);
        temp[1] = A(2, temp[1] + LG3 * ir3);
        temp[2] = A(3, temp[2] + LB3 * ir3);

        MAC1 = (temp[0] >> (SF*12));
        MAC2 = (temp[1] >> (SF*12));
        MAC3 = (temp[2] >> (SF*12));

        IR1 = B(MAC1, LM, 1);
        IR2 = B(MAC2, LM, 2);
        IR3 = B(MAC3, LM, 3);

	*(u32*)&RGB0[0] = *(u32*)&RGB1[0];
	*(u32*)&RGB1[0] = *(u32*)&RGB2[0];
        RGB2[0] = C(1, MAC1 >> 4);
        RGB2[1] = C(2, MAC2 >> 4);
        RGB2[2] = C(3, MAC3 >> 4);
        RGB2[3] = RGBC[3];
}

void r3000::DCPL(u32 opc)
{
	s64 R = A(1, s64(RGBC[0]<<4) * IR1);
	s64 G = A(1, s64(RGBC[1]<<4) * IR2);
	s64 b = A(1, s64(RGBC[2]<<4) * IR3);
	
        MAC1 = (A(1, (s64((s32)RFC)<<12) - R) >> (SF*12));
        MAC2 = (A(2, (s64((s32)GFC)<<12) - G) >> (SF*12));
        MAC3 = (A(3, (s64((s32)BFC)<<12) - b) >> (SF*12));

        IR1 = B(MAC1, false, 1);
        IR2 = B(MAC2, false, 2);
        IR3 = B(MAC3, false, 3);

        MAC1 = (A(1, R + s64(IR1) * IR0) >> (SF*12));
        MAC2 = (A(2, G + s64(IR2) * IR0) >> (SF*12));
        MAC3 = (A(3, b + s64(IR3) * IR0) >> (SF*12));

        IR1 = B(MAC1, LM, 1);
        IR2 = B(MAC2, LM, 2);
        IR3 = B(MAC3, LM, 3);
	
	*(u32*)&RGB0[0] = *(u32*)&RGB1[0];
	*(u32*)&RGB1[0] = *(u32*)&RGB2[0];
	RGB2[0] = C(1, MAC1 >> 4);
	RGB2[1] = C(2, MAC2 >> 4);
	RGB2[2] = C(3, MAC3 >> 4); 
	RGB2[3] = RGBC[3];
}

void r3000::DCPT(u32 opc)
{
	s64 R = s64(RGB0[0]<<16);
	s64 G = s64(RGB0[1]<<16);
	s64 b = s64(RGB0[2]<<16);
	
        MAC1 = (A(1, (s64(RFC)<<12) - R) >> (SF*12));
        MAC2 = (A(2, (s64(GFC)<<12) - G) >> (SF*12));
        MAC3 = (A(3, (s64(BFC)<<12) - b) >> (SF*12));

        IR1 = B(MAC1, false, 1);
        IR2 = B(MAC2, false, 2);
        IR3 = B(MAC3, false, 3);

        MAC1 = (A(1, R + s64(IR1) * IR0) >> (SF*12));
        MAC2 = (A(2, G + s64(IR2) * IR0) >> (SF*12));
        MAC3 = (A(3, b + s64(IR3) * IR0) >> (SF*12));

        IR1 = B(MAC1, LM, 1);
        IR2 = B(MAC2, LM, 2);
        IR3 = B(MAC3, LM, 3);
	
	*(u32*)&RGB0[0] = *(u32*)&RGB1[0];
	*(u32*)&RGB1[0] = *(u32*)&RGB2[0];
	RGB2[0] = C(1, MAC1 >> 4);
	RGB2[1] = C(2, MAC2 >> 4);
	RGB2[2] = C(3, MAC3 >> 4); 
	RGB2[3] = RGBC[3];
}

void r3000::GPL(u32 opc)
{
	MAC1 = A(1, s64(IR0) * IR1 + (s64(MAC1)<<(SF*12))) >> (SF*12);
	MAC2 = A(2, s64(IR0) * IR2 + (s64(MAC2)<<(SF*12))) >> (SF*12);
	MAC3 = A(3, s64(IR0) * IR3 + (s64(MAC3)<<(SF*12))) >> (SF*12);

        IR1 = B(MAC1, LM, 1);
        IR2 = B(MAC2, LM, 2);
        IR3 = B(MAC3, LM, 3);

  	*(u32*)&RGB0[0] = *(u32*)&RGB1[0];
	*(u32*)&RGB1[0] = *(u32*)&RGB2[0];
	RGB2[0] = C(1, MAC1 >> 4);
	RGB2[1] = C(2, MAC2 >> 4);
	RGB2[2] = C(3, MAC3 >> 4); 
	RGB2[3] = RGBC[3];
}

void r3000::gte_exec(u32 opc)
{
	if( (opc & (1u<<25)) == 0 )
	{
		switch( S )
		{
		case 0: LOAD(T, gte_get_d(((opc>>11)&0x1F))); DEST(T); break;
		case 2: LOAD(T, gte_get_c(((opc>>11)&0x1F))); DEST(T); break;
		case 4: gte_load_d(((opc>>11)&0x1F), r[T]); break;
		case 6: gte_load_c(((opc>>11)&0x1F), r[T]); break;
		default: printf("Unimpl cop2 opcode = $%X\n", S); exit(1); break;	
		}
		return;
	}
	
	flags = 0;
	
	switch( opc & 0x3F )
	{
	case 1: RTPS(opc); break; // passes tests
	case 6: NCLIP(opc); break; // passes tests
	case 0x0C: OP(opc); break; // passes tests
	case 0x10: DPCS(opc); break; // passes tests
	case 0x11: INTPL(opc); break; // passes tests
	case 0x12: MVMVA(opc); break; // passes tests
	case 0x13: NCDS(opc); break; // passes tests
	case 0x14: CDP(opc); break; // passes tests
	case 0x16: NCD(opc, 0); NCD(opc, 1); NCD(opc, 2); break; // passes tests
	case 0x1B: NCC(opc, 0); break; // passes tests
	case 0x1C: CC(opc); break; // passes tests
	case 0x1E: NC(opc, 0); break; // passes tests
	case 0x20: NC(opc, 0); NC(opc, 1); NC(opc, 2); break; // passes tests
	case 0x28: SQR(opc); break; // passes tests
	case 0x29: DCPL(opc); break; // passes tests
	case 0x2A: DCPT(opc); DCPT(opc); DCPT(opc); break; // passes tests
	case 0x2D: AVZ3(opc); break; // passes tests
	case 0x2E: AVZ4(opc); break; // passes tests
	case 0x30: RTPT(opc); break; // passes tests
	case 0x3D: GPF(opc); break; // passes tests
	case 0x3E: GPL(opc); break; // passes tests
	case 0x3F:
		NCC(opc, 0);
		NCC(opc, 1);
		NCC(opc, 2);
		break;  // passes tests
	default:
		fprintf(stderr, "Unimpl GTE opcode = $%X\n", opc&0x3F);
		exit(1);
	}
}

void r3000::gte_load_d(u32 dr, u32 val)
{
	switch( dr )
	{
	case 0: VX0 = val; VY0 = val>>16; break;
	case 1: VZ0 = val; break;
	case 2: VX1 = val; VY1 = val>>16; break;
	case 3: VZ1 = val; break;
	case 4: VX2 = val; VY2 = val>>16; break;
	case 5: VZ2 = val; break;
	case 6: RGBC[0] = val; RGBC[1] = val>>8;
		RGBC[2] = val>>16; RGBC[3] = val>>24;
		break;
	case 7: OTZ = val; break;
	case 8: IR0 = val; break;
	case 9: IR1 = val; break;
	case 10: IR2 = val; break;
	case 11: IR3 = val; break;
	case 12: SX0 = val; SY0 = val>>16; break;
	case 13: SX1 = val; SY1 = val>>16; break;
	case 14: SX2 = val; SY2 = val>>16; break;
	case 15: 
		SX0 = SX1; SY0 = SY1;
		SX1 = SX2; SY1 = SY2;
		SX2 = val; SY2 = val>>16; break;
	case 16: SZ0 = val; break;
	case 17: SZ1 = val; break;
	case 18: SZ2 = val; break;
	case 19: SZ3 = val; break;
	case 20: RGB0[0] = val; RGB0[1] = val>>8;
		 RGB0[2] = val>>16; RGB0[3] = val>>24;
		 break;
	case 21: RGB1[0] = val; RGB1[1] = val>>8;
		 RGB1[2] = val>>16; RGB1[3] = val>>24;
		 break;
	case 22: RGB2[0] = val; RGB2[1] = val>>8;
		 RGB2[2] = val>>16; RGB2[3] = val>>24;
		 break;
	case 23: RES1 = val; break;	
	case 24: MAC0 = val; break;
	case 25: MAC1 = val; break;
	case 26: MAC2 = val; break;
	case 27: MAC3 = val; break;
	
	case 28:
		IR1 = (val&0x1F)<<7;
		IR2 = ((val>>5)&0x1F)<<7;
		IR3 = ((val>>10)&0x1F)<<7;
		//IRGB = val&0x7fff;
		break;
	case 29: break;
	case 30: 
		LZCS = val; 
		if( LZCS & (1u<<31) ) 
		{
			LZCR = std::countl_one(val);
		} else {
			LZCR = std::countl_zero(val);
		}
		if( LZCR == 0 ) LZCR = 32;
		break;
	case 31: break;
	
	case 32: RT11 = val; RT12 = val>>16; break;
	case 33: RT13 = val; RT21 = val>>16; break;
	case 34: RT22 = val; RT23 = val>>16; break;
	case 35: RT31 = val; RT32 = val>>16; break;
	case 36: RT33 = val; break;
	
	case 37: TRX = val; break;
	case 38: TRY = val; break;
	case 39: TRZ = val; break;
	
	case 40: L11 = val; L12 = val>>16; break;
	case 41: L13 = val; L21 = val>>16; break;
	case 42: L22 = val; L23 = val>>16; break;
	case 43: L31 = val; L32 = val>>16; break;
	case 44: L33 = val; break;

	case 45: RBK = val; break;
	case 46: GBK = val; break;
	case 47: BBK = val; break;
	
	case 48: LR1 = val; LR2 = val>>16; break;
	case 49: LR3 = val; LG1 = val>>16; break;
	case 50: LG2 = val; LG3 = val>>16; break;
	case 51: LB1 = val; LB2 = val>>16; break;
	case 52: LB3 = val; break;
	
	case 53: RFC = val; break;
	case 54: GFC = val; break;
	case 55: BFC = val; break;	
	case 56: OFX = val; break;
	case 57: OFY = val; break;
	case 58: H = val; break;
	case 59: DQA = val; break;
	case 60: DQB = val; break;	
	case 61: ZSF3 = val; break;
	case 62: ZSF4 = val; break;
	case 63: flags = val&~0xfff; 
		flags &= 0x7fffFFFFu;
		if( (flags>>23) || ((flags>>13)&0x3f) ) flags |= (1u<<31);
		break;
	default:
		printf("Unimpl gte load r%i = $%X\n", dr, val);
		exit(1);	
	}
	return;
}

u32 rpair(s16 H, s16 L)
{
	u32 r = u16(H)<<16;
	r |= u16(L);
	return r;
}

u32 r3000::gte_get_d(u32 dr)
{
	switch( dr )
	{
	case 0: return rpair(VY0, VX0);
	case 1: return s32(VZ0);
	case 2: return rpair(VY1, VX1);
	case 3: return s32(VZ1);
	case 4: return rpair(VY2, VX2);
	case 5: return s32(VZ2);
	case 6: return (RGBC[3]<<24)|(RGBC[2]<<16)|(RGBC[1]<<8)|RGBC[0];
	case 7: return OTZ;
	case 8: return (IR0);
	case 9: return s32(IR1);
	case 10: return (IR2);
	case 11: return s32(IR3);
	case 12: return rpair(SY0, SX0);
	case 13: return rpair(SY1, SX1);
	case 14: return rpair(SY2, SX2);
	case 15: return rpair(SY2, SX2);
	case 16: return SZ0;
	case 17: return SZ1;
	case 18: return SZ2;
	case 19: return SZ3;	
	case 20: return (RGB0[3]<<24)|(RGB0[2]<<16)|(RGB0[1]<<8)|RGB0[0];
	case 21: return (RGB1[3]<<24)|(RGB1[2]<<16)|(RGB1[1]<<8)|RGB1[0];
	case 22: return (RGB2[3]<<24)|(RGB2[2]<<16)|(RGB2[1]<<8)|RGB2[0];	
	case 23: return RES1;
	case 24: return MAC0;
	case 25: return MAC1;
	case 26: return MAC2;
	case 27: return MAC3;
	case 28: //return IRGB&0xffff;
	case 29:{
		u8 R = std::clamp((IR1>>7), 0, 0x1f);
		u8 G = std::clamp((IR2>>7), 0, 0x1f);
		u8 B = std::clamp((IR3>>7), 0, 0x1f);
		return (R|(G<<5)|(B<<10))&0x7fff;
		}
	case 30: return LZCS;
	case 31: return LZCR;
	case 32: return rpair(RT12, RT11);
	case 33: return rpair(RT21, RT13);
	case 34: return rpair(RT23, RT22);
	case 35: return rpair(RT32, RT31);
	case 36: return s32(RT33);
	case 37: return TRX;
	case 38: return TRY;
	case 39: return TRZ;
	case 40: return rpair(s64(L12), s64(L11));
	case 41: return rpair(s64(L21), s64(L13));
	case 42: return rpair(s64(L23), s64(L22));
	case 43: return rpair(s64(L32), s64(L31));
	case 44: return s32(s64(L33));
	case 45: return RBK;
	case 46: return GBK;
	case 47: return BBK;
	case 48: return rpair(LR2, LR1);
	case 49: return rpair(LG1, LR3);
	case 50: return rpair(LG3, LG2);
	case 51: return rpair(LB2, LB1);
	case 52: return s32(LB3);
	case 53: return RFC;
	case 54: return GFC;
	case 55: return BFC;
	case 56: return OFX;
	case 57: return OFY;	
	case 58: return s32(s16(H));
	case 59: return s32(DQA);
	case 60: return DQB;
	case 61: return s32(ZSF3);
	case 62: return s32(ZSF4);
	case 63: flags &= 0x7fffFFFFu;
		 if( (flags>>23) || ((flags>>13)&0x3f) ) flags |= (1u<<31);
		 return flags;
	default:
		printf("Unimpl gte read r%i\n", dr);
		exit(1);	
	}
	return 0;
}

void r3000::gte_load_c(u32 cr, u32 val)
{
	gte_load_d(cr+32, val);
}

u32 r3000::gte_get_c(u32 cr)
{
	return gte_get_d(cr+32);
}


u32 divtable[] = { 
  0xFF,0xFD,0xFB,0xF9,0xF7,0xF5,0xF3,0xF1,0xEF,0xEE,0xEC,0xEA,0xE8,0xE6,0xE4,0xE3,
  0xE1,0xDF,0xDD,0xDC,0xDA,0xD8,0xD6,0xD5,0xD3,0xD1,0xD0,0xCE,0xCD,0xCB,0xC9,0xC8,
  0xC6,0xC5,0xC3,0xC1,0xC0,0xBE,0xBD,0xBB,0xBA,0xB8,0xB7,0xB5,0xB4,0xB2,0xB1,0xB0,
  0xAE,0xAD,0xAB,0xAA,0xA9,0xA7,0xA6,0xA4,0xA3,0xA2,0xA0,0x9F,0x9E,0x9C,0x9B,0x9A,
  0x99,0x97,0x96,0x95,0x94,0x92,0x91,0x90,0x8F,0x8D,0x8C,0x8B,0x8A,0x89,0x87,0x86,
  0x85,0x84,0x83,0x82,0x81,0x7F,0x7E,0x7D,0x7C,0x7B,0x7A,0x79,0x78,0x77,0x75,0x74,
  0x73,0x72,0x71,0x70,0x6F,0x6E,0x6D,0x6C,0x6B,0x6A,0x69,0x68,0x67,0x66,0x65,0x64,
  0x63,0x62,0x61,0x60,0x5F,0x5E,0x5D,0x5D,0x5C,0x5B,0x5A,0x59,0x58,0x57,0x56,0x55,
  0x54,0x53,0x53,0x52,0x51,0x50,0x4F,0x4E,0x4D,0x4D,0x4C,0x4B,0x4A,0x49,0x48,0x48,
  0x47,0x46,0x45,0x44,0x43,0x43,0x42,0x41,0x40,0x3F,0x3F,0x3E,0x3D,0x3C,0x3C,0x3B,
  0x3A,0x39,0x39,0x38,0x37,0x36,0x36,0x35,0x34,0x33,0x33,0x32,0x31,0x31,0x30,0x2F,
  0x2E,0x2E,0x2D,0x2C,0x2C,0x2B,0x2A,0x2A,0x29,0x28,0x28,0x27,0x26,0x26,0x25,0x24,
  0x24,0x23,0x22,0x22,0x21,0x20,0x20,0x1F,0x1E,0x1E,0x1D,0x1D,0x1C,0x1B,0x1B,0x1A,
  0x19,0x19,0x18,0x18,0x17,0x16,0x16,0x15,0x15,0x14,0x14,0x13,0x12,0x12,0x11,0x11,
  0x10,0x0F,0x0F,0x0E,0x0E,0x0D,0x0D,0x0C,0x0C,0x0B,0x0A,0x0A,0x09,0x09,0x08,0x08,
  0x07,0x07,0x06,0x06,0x05,0x05,0x04,0x04,0x03,0x03,0x02,0x02,0x01,0x01,0x00,0x00,
  0x00
};

u64 r3000::strange_div(u16 h, u16 SZ)
{
	u32 z = std::countl_zero(SZ);
        u64 n = u64(h) << z;
        u64 d = u64(SZ) << z;
        u64 u = divtable[(d - 0x7fc0) >> 7] + 0x101;
        u64 d2 = (0x2000080 - (d * u)) >> 8;
        u64 d3 = (0x80 + (d2 * u)) >> 8;
        return std::min(0x1fffful, (((n*d3) + 0x8000) >> 16));

/*	if( SZ == 0 || h < SZ*2 )
	{
		flags |= (1u<<17);
		return 0x1ffff;
	}
	
	u64 z = std::countl_zero(u16(SZ)); // count_leading_zeroes(SZ3)                ;z=0..0Fh (for 16bit SZ3)
	u64 n = (h << z);  //                               ;n=0..7FFF8000h
 	u64 d = (SZ << z);    //                           ;d=8000h..FFFFh
	u64 u = divtable[(d-0x7FC0) >> 7] + 0x101;  //        ;u=200h..101h
	d = ((0x2000080 - (d * u)) >> 8);         //    ;d=10000h..0FF01h
	d = ((0x0000080 + (d * u)) >> 8);  //           ;d=20000h..10000h
	n = std::min(0x1FFFFul, (((n*d) + 0x8000) >> 16)); //    ;n=0..1FFFFh
	return n;
*/
}

