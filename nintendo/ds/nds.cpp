#include <print>
#include <deque>
#include "Settings.h"
#include "util.h"
#include "nds.h"

std::deque<u32> last10;

extern u16 toggle;

bool enditall = false;
bool last10out = false;
void nds::run_frame()
{
	keystate = keys();
	
	frame_complete = false;
	while( !frame_complete )
	{
		if( arm9.halted && arm7.halted )
		{
			arm9.stamp = sched.next_stamp();
			sched.run_event();
		} else {
			if( !arm9.halted )
			{
				arm9.step();
			} else {
				arm9.stamp += 1;
			}
			if( !arm7.halted && (arm9.stamp&1) )
			{
				arm7.step();
			}
			while( sched.next_stamp() <= arm9.stamp )
			{
				sched.run_event();
			}
		}
	}
		
	memcpy(fbuf, vram, 256*192*2);
}


void nds::reset()
{
	//arm7.reset();
	//arm9.reset();
	arm9.dtcm.base = 0x800000;
	arm9.dtcm.size = 0x4000;
	wramcnt = 3;
	
	ipc.ipcsync7 = ipc.ipcsync9 = 0;
	ipc.fifocnt7.v = ipc.fifocnt9.v = 0x101;
	ipc.last_q2arm9 = ipc.last_q2arm7 = 0;
	disp.scanline = 0;
	arm7.stamp = arm9.stamp = 0;
	arm7.halted = arm9.halted = false;
	sched.reset();
	sched.add_event(1536, EVENT_HBLANK_START);
	sched.add_event(1605, EVENT_NEXT_SCANLINE);
}

bool nds::loadROM(std::string fname)
{
	std::vector<std::string> imap = Settings::get<std::vector<std::string>>("nds", "player1");
	setPlayerInputMap(1, imap);

	arm7.read = [&](u32 a, int sz, ARM_CYCLE ct)-> u32 { return arm7_read(a, sz, ct); };
	arm7.write = [&](u32 a, u32 v, int sz, ARM_CYCLE ct) { arm7_write(a, v, sz, ct); };
	arm9.read = [&](u32 a, int sz, ARM_CYCLE ct)-> u32 { return arm9_read(a, sz, ct); };
	arm9.write = [&](u32 a, u32 v, int sz, ARM_CYCLE ct) { arm9_write(a, v, sz, ct); };
	arm9.inst_fetch = [&](u32 a, int sz, ARM_CYCLE ct) { return arm9_fetch(a, sz, ct); };
	
	if( !freadall(arm7_bios, fopen("./bios/nds7.bios", "rb"), 16_KB) )
	{
		std::println("Need ./bios/nds7.bios");
		return false;
	}
	
	if( !freadall(arm9_bios, fopen("./bios/nds9.bios", "rb"), 4_KB) )
	{
		std::println("Need ./bios/nds9.bios");
		return false;
	}
	
	FILE* fp = fopen(fname.c_str(), "rb");
	fseek(fp, 0, SEEK_END);
	auto fsz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	ROM.resize(fsz);
	[[maybe_unused]] int unu = fread(ROM.data(), 1, fsz, fp);
	fclose(fp);
	///*
	u32 rom_offset = *(u32*)&ROM[0x20];
	u32 ram_addr = *(u32*)&ROM[0x28];
	u32 size = *(u32*)&ROM[0x2C];
	for(u32 i = 0; i < size; ++i, ++ram_addr, ++rom_offset)
	{
		arm9_write(ram_addr, ROM[rom_offset], 8, ARM_CYCLE::X);
	}
	arm9.reset();
	arm9.r[15] = *(u32*)&ROM[0x24];
	arm9.flushp();
	
	rom_offset = *(u32*)&ROM[0x30];
	ram_addr = *(u32*)&ROM[0x38];
	size = *(u32*)&ROM[0x3C];
	for(u32 i = 0; i < size; ++i, ++ram_addr, ++rom_offset)
	{
		arm7_write(ram_addr, ROM[rom_offset], 8, ARM_CYCLE::X);
	}
	arm7.reset();
	arm7.r[15] = *(u32*)&ROM[0x34];
	arm7.flushp();
	//*/
	
	
	return true;
}

u32 nds::keys()
{
	u16 val = 0x3ff;
	if( getInputState(1, 4) ) val ^= BIT(9);
	if( getInputState(1, 5) ) val ^= BIT(8);
	if( getInputState(1, 7) ) val ^= BIT(7);
	if( getInputState(1, 6) ) val ^= BIT(6);
	if( getInputState(1, 8) ) val ^= BIT(5);
	if( getInputState(1, 9) ) val ^= BIT(4);
	if( getInputState(1, 3) ) val ^= BIT(3);
	if( getInputState(1, 2) ) val ^= BIT(2);
	if( getInputState(1, 1) ) val ^= BIT(1);
	if( getInputState(1, 0) ) val ^= BIT(0);
	return val;
}

void nds::dsmath_div()
{
	dsmath.divcnt &= 3;
	u32 mode = dsmath.divcnt&3;
	if( mode == 3 ) mode = 1;
	dsmath.divcnt |= ((dsmath.div_den==0) ? BIT(14):0);
	
	switch( mode )
	{
	case 0: // 32/32=32
		if( u32(dsmath.div_den) == 0 )
		{
			dsmath.div_quot = ( (s32(dsmath.div_num) < 0) ? 0xFFFFffff00000001ull : 0xffffFFFFull );
			dsmath.div_rem = s32(dsmath.div_num);
		} else if( u32(dsmath.div_num) == 0x80000000u && s32(dsmath.div_den) == -1 ) {
			dsmath.div_quot = 0x80000000u;
			dsmath.div_rem = 0;
		} else {
			dsmath.div_quot = s64( s32(dsmath.div_num) / s32(dsmath.div_den) );
			dsmath.div_rem  = s64( s32(dsmath.div_num) % s32(dsmath.div_den) );
		}
		break;
	case 1: // 64/32=64
		if( u32(dsmath.div_den) == 0 )
		{
			dsmath.div_quot = s64(dsmath.div_num) < 0 ? 1:-1;
			dsmath.div_rem = dsmath.div_num;
		} else if( dsmath.div_num == BITL(63) && s32(dsmath.div_den) == -1 ) {
			dsmath.div_quot = BITL(63);
			dsmath.div_rem = 0;
		} else {
			dsmath.div_quot = s64(dsmath.div_num) / s32(dsmath.div_den);
			dsmath.div_rem  = s64(dsmath.div_num) % s32(dsmath.div_den);
		}
		break;
	case 2: // 64/64=64
		if( dsmath.div_den == 0 )
		{
			dsmath.div_quot = s64(dsmath.div_num) < 0 ? 1:-1;
			dsmath.div_rem = dsmath.div_num;
		} else if( dsmath.div_num == BITL(63) && s64(dsmath.div_den) == -1 ) {
			dsmath.div_quot = BITL(63);
			dsmath.div_rem = 0;
		} else {
			dsmath.div_quot = s64(dsmath.div_num) / s64(dsmath.div_den);
			dsmath.div_rem  = s64(dsmath.div_num) % s64(dsmath.div_den);
		}		
		break;	
	}
}

void nds::dsmath_sqrt()
{
	dsmath.sqrtcnt &= 1;
	if( dsmath.sqrtcnt & 1 )
	{
		dsmath.sqrt_res = sqrtl(dsmath.sqrt_param);
	} else {
		dsmath.sqrt_res = sqrtl(u32(dsmath.sqrt_param));
	}
}

void nds::event(u64 oldstamp, u32 code)
{
	if( code == EVENT_HBLANK_START )
	{
		disp.stat |= DISPSTAT_HBLANK_FLAG;
		if( disp.stat & DISPSTAT_HBLANK_IRQ_EN )
		{
			arm9_raise_irq(IRQ_HBLANK_BIT);
			arm7_raise_irq(IRQ_HBLANK_BIT);
		}
		sched.add_event(oldstamp + 1605, EVENT_HBLANK_START);
		return;
	}
	if( code == EVENT_NEXT_SCANLINE )
	{
		disp.stat &= ~7;
		//draw_scanline();
		disp.scanline += 1;
		if( disp.scanline >= 263 )
		{
			disp.scanline = 0;
			frame_complete = true;
		}
		if( disp.scanline == 192 && (disp.stat & DISPSTAT_VBLANK_IRQ_EN) )
		{
			arm9_raise_irq(IRQ_VBLANK_BIT);
			arm7_raise_irq(IRQ_VBLANK_BIT);
		}
		if( disp.scanline >= 192 )
		{
			disp.stat |= DISPSTAT_VBLANK_FLAG;
		}
		u32 vmatch = (disp.stat>>8)|((disp.stat&0x80)<<1);
		if( (disp.stat & DISPSTAT_VCOUNT_IRQ_EN) && vmatch == disp.scanline )
		{
			arm9_raise_irq(IRQ_VCOUNT_BIT);
			arm7_raise_irq(IRQ_VCOUNT_BIT);		
		}
		sched.add_event(oldstamp + 1605, EVENT_NEXT_SCANLINE);
		return;
	}
}










