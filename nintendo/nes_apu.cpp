#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <SDL.h>
#include "nes.h"
#include "Settings.h"

extern SDL_AudioDeviceID audio_dev;

const u32 NES_SAMPLES_PER_SDL_SAMPLE = 40;
const u32 SDL_SAMPLES_PER_FRAME = 735;

static u8 duty[] = { 0, 1, 0, 0, 0, 0, 0, 0,
	             0, 1, 1, 0, 0, 0, 0, 0,
	             0, 1, 1, 1, 1, 0, 0, 0,
	             1, 0, 0, 1, 1, 1, 1, 1 };
	             
static u8 triduty[] = { 
15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
};

static u16 lencnt[] = {
10,254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

static u16 noisetim[] = {
4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};

static u16 dmcrate[] = {
428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106,  84,  72,  54
};

static std::vector<float> nes_samples;

void nes::apu_clock_env_lincnt()
{
	if( ! ( apu_regs[0] & 0x10 ) )
	{
		if( p1_env_start )
		{
			p1_env_start = 0;
			pulse1_env_vol = 15;
			pulse1_env_div = apu_regs[0]&0xf;
			pulse1_env_div++;
		} else {
			if( pulse1_env_div == 0 )
			{
				if( pulse1_env_vol )
				{
					pulse1_env_vol--;
				} else {
					if( apu_regs[0]&0x20 )
					{
						pulse1_env_vol = 15;
						pulse1_env_div = apu_regs[0]&0xf;
						pulse1_env_div++;
					}
				}
			} else {
				pulse1_env_div--;
			}
		}		
	}
	
	if( ! ( apu_regs[4] & 0x10 ) )
	{
		if( p2_env_start )
		{
			p2_env_start = 0;
			pulse2_env_vol = 15;
			pulse2_env_div = apu_regs[4]&0xf;
			pulse2_env_div++;
		} else {
			if( pulse2_env_div == 0 )
			{
				if( pulse2_env_vol )
				{
					pulse2_env_vol--;
				} else {
					if( apu_regs[4]&0x20 ) 
					{
						pulse2_env_vol = 15;
						pulse2_env_div = apu_regs[4]&0xf;
						pulse2_env_div++;
					}
				}
			} else {
				pulse2_env_div--;
			}
		}
	}
	
	if( ! ( apu_regs[0xc] & 0x10 ) )
	{
		if( noise_env_start )
		{
			noise_env_start = 0;
			noise_env_vol = 15;
			noise_env_div = apu_regs[0xc]&0xf;
			noise_env_div++;
		} else {
			if( noise_env_div == 0 )
			{
				if( noise_env_vol )
				{
					noise_env_vol--;
				} else {
					if( apu_regs[0xc]&0x20 ) 
					{
						noise_env_vol = 15;
						noise_env_div = apu_regs[0xc]&0xf;
						noise_env_div++;
					}
				}
			} else {
				noise_env_div--;
			}
		}
	}
	
	if( tri_lin_reload )
	{
		tri_lin = apu_regs[8] & 0x7f;
		if( !(apu_regs[8] & 0x80) ) tri_lin_reload = 0;
	} else if( tri_lin ) tri_lin--;	
}

void nes::apu_clock_len_sweep()
{
	if( (apu_regs[0x15]&1) && !( apu_regs[0]&0x20 ) )
	{
		if( pulse1_len ) pulse1_len--;
	}
	if( (apu_regs[0x15]&2) && !( apu_regs[4]&0x20 ) )
	{
		if( pulse2_len ) pulse2_len--;
	}
	if( (apu_regs[0x15]&4) && !( apu_regs[8]&0x80 ) )
	{
		if( tri_len ) tri_len--;
	}
	if( (apu_regs[0x15]&8) && !( apu_regs[0xC]&0x20 ) )
	{
		if( noise_len ) noise_len--;
	}
	
	if( (apu_regs[1] & 0x80) && (apu_regs[1]&7) )
	{
		if( p1_sweep_reload )
		{
			p1_sweep_reload = 0;
			p1_sweep_div = (apu_regs[1]>>4)&7;
		} else if( p1_sweep_div == 0 ) {
			p1_sweep_div = (apu_regs[1]>>4)&7;
			u16 timer = ((apu_regs[3]&7)<<8)|apu_regs[2];
			s16 val = timer >> (apu_regs[1]&7);
			if( apu_regs[1] & 8 ) val = -val - 1;
			timer += val;
			if( timer & 0x8000 ) timer = 0;
			apu_regs[3] &= ~7;
			apu_regs[3] |= (timer>>8)&7;
			apu_regs[2] = timer;
		} else {
			p1_sweep_div--;
		}
	}
	
	if( (apu_regs[5] & 0x80) && (apu_regs[5]&7)  )
	{
		if( p2_sweep_reload )
		{
			p2_sweep_reload = 0;
			p2_sweep_div = (apu_regs[5]>>4)&7;
		} else if( p2_sweep_div == 0 ) {
			p2_sweep_div = (apu_regs[5]>>4)&7;
			u16 timer = ((apu_regs[7]&7)<<8)|apu_regs[6];
			s16 val = timer >> (apu_regs[5]&7);
			if( apu_regs[5] & 8 ) val = -val;
			timer += val;
			if( timer & 0x8000 ) timer = 0;
			apu_regs[7] &= ~7;
			apu_regs[7] |= (timer>>8)&7;
			apu_regs[6] = timer;
		} else {
			p2_sweep_div--;
		}	
	}
}

void nes::apu_clock()
{
	apu_stamp++;
	if( apu_4017 & 0x80 )
	{
		if( apu_stamp == 7457 ) {
			apu_clock_env_lincnt();	
		} else if( apu_stamp == 14913 ) {
			apu_clock_env_lincnt();
			apu_clock_len_sweep();
		} else if( apu_stamp == 22371 ) {
			apu_clock_env_lincnt();		
		} else if( apu_stamp == 29829 ) {
			; // nothing clocked here
		} else if( apu_stamp == 37281 ) {
			apu_clock_env_lincnt();
			apu_clock_len_sweep();
		//} else if( apu_stamp >= 37282 ) {
			apu_stamp = 0;
		}
	} else {
		if( apu_stamp == 7457 ) {
			apu_clock_env_lincnt();
		} else if( apu_stamp == 14913 ) {
			apu_clock_env_lincnt();
			apu_clock_len_sweep();
		} else if( apu_stamp == 22371 ) {
			apu_clock_env_lincnt();
		} else if( apu_stamp == 29828 ) {
			; // nothing clocked here
		} else if( apu_stamp == 29829 ) {
			apu_clock_env_lincnt();
			apu_clock_len_sweep();
		//} else if( apu_stamp >= 29830 ) {
			apu_stamp = 0;
		}
	}
	
	if( apu_stamp & 1 )
	{
		if( pulse1_timer == 0 )
		{
			pulse1_seq = (pulse1_seq+1)&7;
			pulse1_timer = ((apu_regs[3]&7)<<8)|apu_regs[2];
			pulse1_timer++;
		} else {
			pulse1_timer--;
		}
		pulse1_vol = (apu_regs[0]&0x10) ? (apu_regs[0]&0xf) : pulse1_env_vol;
		if( duty[(apu_regs[0]>>6)*8 + pulse1_seq] == 0 ) pulse1_vol = 0;
		
		if( pulse2_timer == 0 )
		{
			pulse2_seq = (pulse2_seq+1)&7;
			pulse2_timer = ((apu_regs[7]&7)<<8)|apu_regs[6];
			pulse2_timer++;
		} else {
			pulse2_timer--;
		}
		pulse2_vol = (apu_regs[4]&0x10) ? (apu_regs[4]&0xf) : pulse2_env_vol;
		if( duty[(apu_regs[4]>>6)*8 + pulse2_seq] == 0 ) pulse2_vol = 0;
		
		if( noise_timer == 0 )
		{
			noise_timer = noisetim[apu_regs[0xE]&0xf];
			u16 fb = (noise_shift&1)^((apu_regs[0xE]&0x80)?!!(noise_shift&0x40):!!(noise_shift&2));
			noise_shift >>= 1;
			noise_shift |= fb<<14;
		} else {
			noise_timer--;
		}
	}
	
	if( 1 )
	{
		if( tri_timer == 0 )
		{
			tri_seq = (tri_seq+1)&0x1F;
			tri_timer = ((apu_regs[0xB]&7)<<8)|apu_regs[0xA];
		} else {
			tri_timer--;
		}
		
		if( dmc_len )
		{
			if( dmc_timer == 0 )
			{
				dmc_timer = dmcrate[apu_regs[0x10]&0xf];
				dmc_bindex++;
				if( dmc_bindex == 8 )
				{
					dmc_byte = read(dmc_cur_addr++);
					dmc_bindex = 0;
					dmc_len--;
				}
				if( (dmc_byte&1) && apu_regs[0x11] <= 125 ) apu_regs[0x11]+=2;
				else if( apu_regs[0x11] >= 2 ) apu_regs[0x11]-=2;
				dmc_byte>>=1;
			} else {
				dmc_timer--;
			}
		}
	}
	
	u8 p1=0, p2=0, tri=0, noi=0, dmc=apu_regs[0x11]&0x7f;
	if( (apu_regs[0x15] & 1) && pulse1_len ) { p1 = pulse1_vol; }
	if( (apu_regs[0x15] & 2) && pulse2_len ) { p2 = pulse2_vol; }
	if( (apu_regs[0x15] & 4) && tri_len && tri_lin ) { tri = triduty[tri_seq]; }
	if( (apu_regs[0x15] & 8) && noise_len ) 
	{ 
		noi = (noise_shift&1)?0:((apu_regs[0xc]&0x10)?(apu_regs[0xc]&0xf):noise_env_vol);
	}
	
	float pulse_out = 0;
	if( p1 || p2 ) pulse_out = 95.88f / ((8128.f / (p1 + p2)) + 100.f);
	float tnd_out = 0;
	if( tri || noi || dmc ) tnd_out = 159.79f / ((1.f / ((tri / 8227.f) + (noi / 12241.f) + (dmc / 22638.f)))+100.f);
	
	nes_samples.push_back(pulse_out + tnd_out);
	
	if( nes_samples.size() >= 40 )
	{
		float total = 0;
		for(int i = 0; i < 40; ++i) total += nes_samples[i];
		nes_samples.clear();
		
		float sm = ((total / 40.f)*2 - 1);
		audio_add(sm, sm);
	}
}

void nes::apu_write(u16 addr, u8 val)
{
	addr &= 0x1F;
	apu_regs[addr] = val;

	if( addr == 3 )
	{
		pulse1_seq = 0;
		pulse1_timer = (apu_regs[3]&7)<<8;
		pulse1_timer |= apu_regs[2];
		pulse1_timer++;
		pulse1_len = lencnt[apu_regs[3]>>3];
		p1_env_start = 1;
		return;
	}
	if( addr == 7 )
	{
		pulse2_seq = 0;
		pulse2_timer = (apu_regs[7]&7)<<8;
		pulse2_timer |= apu_regs[6];
		pulse2_timer++;
		pulse2_len = lencnt[apu_regs[7]>>3];
		p2_env_start = 1;
		return;
	}
	if( addr == 0xA )
	{
		//tri_timer = ((apu_regs[0xB]&7)<<8)|apu_regs[0xA];
		return;
	}
	if( addr == 0xB )
	{
		tri_timer = ((apu_regs[0xB]&7)<<8)|apu_regs[0xA];
		tri_len = lencnt[apu_regs[0xB]>>3];
		tri_lin_reload = 1;
		return;
	}
	if( addr == 0xE )
	{
		noise_timer = noisetim[val&0xf];
		return;
	}
	if( addr == 0xF )
	{
		noise_len = lencnt[val>>3];
		noise_env_start = 1;
		return;
	}
	if( addr == 0x15 )
	{
		val ^= 0xff;
		if( val & 1 ) pulse1_len = 0;
		if( val & 2 ) pulse2_len = 0;
		if( val & 4 ) tri_len = tri_lin = 0;
		if( val & 8 ) noise_len = 0;
		
		if( val & 0x10 )
		{
			dmc_len = 0;
		} else {
			dmc_len = apu_regs[0x13]*16 + 1;
			dmc_bindex = 7;
			dmc_cur_addr = apu_regs[0x12]*64 + 0xc000;
			dmc_timer = dmcrate[apu_regs[0x10]&0xf];
		}
		return;
	}
	if( addr == 1 ) { p1_sweep_reload = 1; return; }
	if( addr == 5 ) { p2_sweep_reload = 1; return; }
}

void nes::apu_reset()
{
	nes_samples.reserve(40);
}


