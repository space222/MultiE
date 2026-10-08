#pragma once
#include <cmath>
#include <utility>
#include "itypes.h"

/*namespace ym2612_internal 
{
	constexpr std::array<u32, 256> build_frac()
	{
		std::array<u32, 256> R;
		for(u32 i = 0; i < 256; ++i)
		{
			double d = -((i+1)/256.0);
			d = std::exp2(d);
			R[i] = static_cast<u32>(round(d*(1<<11)));
		}
		return R;
	}
}*/

class ym2612
{
public:
	ym2612() { reset(); }
	void data(u8);
	void ctrl(u8 p, u8 v);
	u8 stat() { return status; }
	
	u8 last_port{}, reg{}, status{};
	
	u8 R[0x100];

	void cycle();
	
	void reset();

	u64 stamp{};
	
	struct phaset
	{
		u32 fnum, block, detune, mult;
		u32 counter;
		u32 out{}, old{}, older{};
	};

	struct operatr
	{
		u32 atten{0x100}, adsr, rates[4];
		u32 suslevel, TL, key_scale;
		bool polarity;
		phaset phz;
		//(adsr==0?(atten^0x3ff):atten)
		u32 level() { return std::min<u32>(atten + (TL<<3), 0x3ff); }
	};
	
	struct channel
	{
		u32 fnum, block, algo{7}, feedback;
		operatr op[4];
	} chan[6];
	
	void op_phase(u32 ch, u32 oper, u32 phase_add=0);
	void run_timers();
	
	u32 busy_clear{};
	
	u32 phase_clkdiv{}, env_clkdiv{}, env_clock{};
	
	u32 timerA{}, timerB{}, timerB_div{};
	
	float currentL;	
	//static constexpr std::array<u32, 256> pow2_frac = ym2612_internal::build_frac();
	static u32 detune[];
	static u32 envtable[];
};



