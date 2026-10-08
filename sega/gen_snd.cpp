#include <cstdio>
#include <cstdlib>
#include <string>
#include "genesis.h"
#include "ym3438.h"

void genesis::fm_write(u32 addr, u8 val)
{
	if( addr & 1 )
	{
		fmsynth.data(val);
	} else {
		fmsynth.ctrl((addr>>1)&1, val);
	}
	//fm_run();
	//OPN2_Write(&synth, addr&3, val);
}

u8 genesis::fm_read()
{
	return fmsynth.stat();
	//fm_run();
	//return OPN2_Read(&synth, 0x4000);
}

void genesis::fm_run(u64 cc)
{
	for(u64 i = 0; i < cc; ++i) { fmsynth.cycle(); }
	//while( fm_stamp < stamp )
	//{
		//fm_stamp += 36;
		//s16 buf[2];
		//OPN2_Clock(&synth, &buf[0]);
		//fm_total += buf[0];
		//fm_count += 1;
		//fm_out += (buf[0]+buf[1]) / 512.f;
	//}
}


















