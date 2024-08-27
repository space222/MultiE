#pragma once
#include "itypes.h"

class SN79489
{
public:
	SN79489();
	void reset();
	u8 clock(u64 cycles);
	void out(u8);

private:
	u16 tone[4];
	u16 cnt[4];
	u8 p[4];
	u8 vol[4];
	u16 lfsr;
	u8 chan, reg;
	void run();
	u8 total;
	u64 stamp;
};

