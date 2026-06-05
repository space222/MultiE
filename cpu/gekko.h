#pragma once
#include <print>
#include <string>
#include <functional>
#include "itypes.h"

class gekko
{
public:
	void step();
	void reset();

	std::function<u64(u32, int)> read = [](u32,int)->u64 { std::println("you forgot to set read"); exit(1); };
	std::function<void(u32,u64,int)> write = [](u32,u64,int) { std::println("you forgot to set write"); exit(1); };
	
	std::function<u32(u32)> get_tbr = [](u32)->u32{ return 0; };

	u32 r[32];
	u32 pc, LR, CTR;
	double f[32];
	double ps1[32];
	bool irq_line, in_infinite_loop;
	u8 reserve;
	
	union xer_t {
		struct {
			bitfield bc : 7;
			bitfield pad : 21;
			bitfield pad4 : 1; // for mcrxr
			bitfield ca : 1;
			bitfield ov : 1;
			bitfield so : 1;
		} b;
		u32 v;	
	} PACKED xer;

	union cond_t
	{
		struct {
			bitfield so : 1;	
			bitfield eq : 1;
			bitfield gt : 1;
			bitfield lt : 1;
			bitfield pad : 4;
		} PACKED cr[8];
		u8 b[8];
		
		void set_bit(u32 bt, u32 val) { u32 old=get()&~BIT(31-bt); set(old|(val<<(31-bt))); }		
		u32 bit(u32 bt) { return (get()>>(31-bt))&1; }
		u32 get() { u32 res = 0; for(u32 i = 0; i < 8; ++i) { res |= b[i]<<((7-i)*4); } return res; }
		void set(u32 v) { for(u32 i = 0; i < 8; ++i) { b[i] = ((v>>((7-i)*4))&0xf); } }
	} PACKED cond;
	
	union hid2_t
	{
		struct {
			bitfield pad16 : 16;
			bitfield dqoee : 1;
			bitfield dcmee : 1;
			bitfield dncee : 1;
			bitfield dchee : 1;
			bitfield dqoerr: 1;
			bitfield dcmerr: 1;
			bitfield dncerr: 1;
			bitfield dcherr: 1;
			bitfield dmaql : 4;
			bitfield lce : 1;
			bitfield pse : 1;
			bitfield wpe : 1;
			bitfield lsqe : 1;
		} PACKED b;
		u32 v;	
	} PACKED hid2;
	
	union msr_t
	{
		struct {
			bitfield le : 1;
			bitfield ri : 1;
			bitfield pad0 : 2;
			bitfield dr : 1;
			bitfield ir : 1;
			bitfield ip : 1;
			bitfield pad1 : 1;
			bitfield fe1 : 1;
			bitfield be : 1;
			bitfield se : 1;
			bitfield fe0 : 1;
			bitfield me : 1;
			bitfield fp : 1;
			bitfield pr : 1;
			bitfield ee : 1;
			bitfield ile : 1;
			bitfield pad2 : 1;
			bitfield pow : 1;
			bitfield pad3 : 13;
		
		} PACKED b;
		u32 v;	
	} PACKED msr;
	
	union fpscr_t
	{
		struct {
			bitfield rn : 2;
			bitfield ni : 1;
			bitfield xe : 1;
			bitfield ze : 1;
			bitfield ue : 1;
			bitfield oe : 1;
			bitfield ve : 1;
			bitfield vxcvi : 1;
			bitfield vxsqrt: 1;
			bitfield vxsoft : 1;
			bitfield pad0 : 1;
			bitfield fprf : 5;
			bitfield fi : 1;
			bitfield fr : 1;
			bitfield vxvc : 1;
			bitfield vximz: 1;
			bitfield vxzdz : 1;
			bitfield vxidi: 1;
			bitfield vxisi : 1;
			bitfield vxsnan : 1;
			bitfield xx : 1;
			bitfield zx : 1;
			bitfield ux : 1;
			bitfield ox : 1;
			bitfield vx : 1;
			bitfield fex : 1;
			bitfield fx : 1;
		} PACKED b;
		u32 v;
	} PACKED fpscr;
	
	union gqr_t
	{
		struct {
			bitfield s_type : 3;
			bitfield pad0 : 5;
			bitfield s_scale : 6;
			bitfield pad1 : 2;
			bitfield l_type : 3;
			bitfield pad2 : 5;
			bitfield l_scale : 6;
			bitfield pad3 : 2;
		} PACKED b;
		u32 v;
	} PACKED gqr[8];
	
	float dequant(u32 val, u32 type, u32 scale);
	u64 quantize(double v, u32 type, u32 scale);
	
	void setCR0(u32 v)
	{
		cond.cr[0].eq = ((v==0) ? 1:0);
		cond.cr[0].lt = ((s32(v)<0) ? 1:0);
		cond.cr[0].gt = ((s32(v)>0) ? 1:0);
		cond.cr[0].so = xer.b.so;
	}
	
	u32 get_spr(u32);
	void set_spr(u32, u32);
	u32 SPRs[1024];
	u32& SRR0 = SPRs[26];
	u32& SRR1 = SPRs[27];
	
	using instr_type = std::function<void(gekko&,u32)>;
};

