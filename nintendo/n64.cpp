#include "n64.h"

static u64 sized_read(u8* data, u32 addr, int size)
{
	if( size == 64 ) return __builtin_bswap64(*(u64*)&data[addr]);
	if( size == 32 ) return __builtin_bswap32(*(u32*)&data[addr]);
	if( size == 16 ) return __builtin_bswap16(*(u16*)&data[addr]);
	return data[addr];
}

static void sized_write(u8* data, u32 addr, u64 v, int size)
{
	if( size == 64 ) { *(u64*)&data[addr] = __builtin_bswap64(v); return; }
	if( size == 32 ) { u32 temp = v; *(u32*)&data[addr] = __builtin_bswap32(temp); return; }
	if( size == 16 ) { u16 temp = v; *(u16*)&data[addr] = __builtin_bswap16(temp); return; }
	data[addr] = v;
}

u64 n64::read(u32 addr, int size)
{
	if( addr < 8*1024*1024 ) return sized_read(mem.data(), addr, size);
	if( addr >= 0x04300000 && addr < 0x04400000 ) return mi_read(addr);
	if( addr >= 0x04400000 && addr < 0x04500000 ) return vi_read(addr);
	if( addr >= 0x04500000 && addr < 0x04600000 ) return ai_read(addr);
	if( addr >= 0x04600000 && addr < 0x04700000 ) return pi_read(addr);
	if( addr >= 0x04800000 && addr < 0x04900000 ) return si_read(addr);
	if( addr >= 0x1FC00000 && addr <= 0x1FCFFFFF )
	{
		addr &= 0x7ff;
		if( addr >= 0x7c0 ) return *(u32*)&pifram[addr&0x3F];
		return *(u32*)&pifrom[addr];
	}
	printf("N64: r%i <$%X\n", size, addr);
	return 0;
}

void n64::write(u32 addr, u64 v, int size)
{
	if( addr < 8*1024*1024 ) { sized_write(mem.data(), addr, v, size); return; }
	if( addr >= 0x04300000 && addr < 0x04400000 ) { mi_write(addr, v); return; }
	if( addr >= 0x04400000 && addr < 0x04500000 ) { vi_write(addr, v); return; }
	if( addr >= 0x04500000 && addr < 0x04600000 ) { ai_write(addr, v); return; }
	if( addr >= 0x04600000 && addr < 0x04700000 ) { pi_write(addr, v); return; }
	if( addr >= 0x04800000 && addr < 0x04900000 ) { si_write(addr, v); return; }
	if( addr >= 0x1FC00000 && addr <= 0x1FCFFFFF )
	{
		addr &= 0x7ff;
		if( addr < 0x7c0 ) return;
		*(u32*)&pifram[addr&0x3F] = v;
		return;
	}
}

bool n64::loadROM(const std::string fname)
{
	FILE* fp = fopen(fname.c_str(), "rb");
	if(!fp)
	{
		printf("Unable to open '%s'\n", fname.c_str());
		return false;
	}

	mem.resize(8*1024*1024);
	
	fseek(fp, 0, SEEK_END);
	auto fsz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	ROM.resize(fsz);
	[[maybe_unused]] int unu = fread(ROM.data(), 1, fsz, fp);
	fclose(fp);
	printf("ROM size = %li bytes\n", fsz);
	u32 entry = __builtin_bswap32(*(u32*)&ROM[8]);
	memcpy(mem.data()+(entry&0x7fffff), ROM.data()+0x1000, fsz < 0x101000 ? fsz : 0x100000);
	
	
	return true;
}

void n64::run_frame()
{
	for(u32 line = 0; line < 262; ++line)
	{
		VI_V_CURRENT = line<<1;
		for(u32 i = 0; i < 5725; ++i)
		{
			cpu.step();
			if( ai_dma_enabled && ai_buf[0].valid )
			{
				ai_cycles += 1;
				if( ai_cycles >= ai_cycles_per_sample )
				{
					ai_cycles = 0;
					s16 L = __builtin_bswap16(*(u16*)&mem[ai_buf[0].ramaddr&0x7fffff]);
					ai_buf[0].ramaddr += 2;
					s16 R = __builtin_bswap16(*(u16*)&mem[ai_buf[0].ramaddr&0x7fffff]);
					ai_buf[0].ramaddr += 2;
					ai_buf[0].length -= 4;
					if( ai_buf[0].length == 0 )
					{
						ai_buf[0] = ai_buf[1];
						ai_buf[1].valid = false;
						if( ai_buf[0].valid ) raise_mi_bit(MI_INTR_AI_BIT);
					}
					
					ai_L = (L / 32768.f);
					ai_R = (R / 32768.f);				
				}
			}
			ai_output_cycles += 1;
			if( ai_output_cycles >= (93750000/44100) )
			{
				ai_output_cycles -= (93750000/44100);
				//audio_add(ai_L, ai_R);
			}
		}
	}
	vi_draw_frame();
}

void n64::reset()
{
	curwidth = 320;
	curheight = 240;
	curbpp = 16;
	cpu.phys_read = [&](u64 a, int s)->u64 { return read(a, s); };
	cpu.phys_write= [&](u64 a, u64 v, int s) { write(a, v, s); };
	cpu.reset();
	cpu.pc = s32(__builtin_bswap32(*(u32*)&ROM[8]));
	cpu.npc = cpu.pc + 4;
	cpu.nnpc = cpu.npc + 4;
	
	PI_STATUS = VI_CTRL = 0;
	pif_rom_enabled = true;
	MI_VERSION = 0x02020102;
	
	ai_buf[0].valid = ai_buf[1].valid = false;
	ai_dma_enabled = false;
	ai_cycles_per_sample = 800;
	ai_cycles = ai_output_cycles = 0;
}

void n64::raise_mi_bit(u32 b)
{
	MI_INTERRUPT |= BIT(b);
	if( MI_INTERRUPT & MI_MASK & 0x3F )
	{
		cpu.CAUSE |= BIT(10);
	} else {
		cpu.CAUSE &= ~BIT(10);
	}
}

void n64::clear_mi_bit(u32 b)
{
	MI_INTERRUPT &= ~BIT(b);
	if( MI_INTERRUPT & MI_MASK & 0x3F )
	{
		cpu.CAUSE |= BIT(10);
	} else {
		cpu.CAUSE &= ~BIT(10);
	}
}
	
void n64::mi_write(u32 r, u32 v)
{
	r = (r&0xf)>>2;
	if( r == 1 || r == 2 ) { return; }  // MI_VERSION and MI_INTERRUPT read-only
	
	if( r == 0 ) 
	{  // MI_MODE. not supporting the memset thing for now, but it also contains an RDP related bit
		if( v & BIT(11) )
		{
			clear_mi_bit(MI_INTR_DP_BIT);
			//todo: clear DP interrupt on DP somewhere?
		}
		return;
	}
	
	if( r == 3 )
	{  // MI_MASK
		u32 sp = v&3;
		u32 si = (v>>2)&3;
		u32 ai = (v>>4)&3;
		u32 vi = (v>>6)&3;
		u32 pi = (v>>8)&3;
		u32 dp = (v>>10)&3;
		
		if( sp == 1 )
		{
			MI_MASK &= ~1;
		} else if( sp == 2 ) {
			MI_MASK |= 1;
		}
		if( si == 1 )
		{
			MI_MASK &= ~2;
		} else if( si == 2 ) {
			MI_MASK |= 2;
		}		
		if( ai == 1 )
		{
			MI_MASK &= ~4;
		} else if( ai == 2 ) {
			MI_MASK |= 4;
		}		
		if( vi == 1 )
		{
			MI_MASK &= ~8;
		} else if( vi == 2 ) {
			MI_MASK |= 8;
		}	
		if( pi == 1 )
		{
			MI_MASK &= ~8;
		} else if( pi == 2 ) {
			MI_MASK |= 8;
		}
		if( dp == 1 )
		{
			MI_MASK &= ~16;
		} else if( dp == 2 ) {
			MI_MASK |= 16;
		}	
		
		if( MI_INTERRUPT & MI_MASK & 0x3F )
		{
			cpu.CAUSE |= BIT(10);
		} else {
			cpu.CAUSE &= ~BIT(10);
		}	
		return;
	}
}

u32 n64::mi_read(u32 addr)
{
	addr = (addr&0xf)>>2;
	return mi_regs[addr];	
}





