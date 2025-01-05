#include <array>
#include "n64.h"

std::array<u32, 256> crc_table = []() {
	std::array<u32, 256> crct;
	u32 crc, poly;
	s32 i, j;

	poly = 0xEDB88320;
	for (i = 0; i < 256; i++) {
		crc = i;
		for (j = 8; j > 0; j--) {
			if (crc & 1) crc = (crc >> 1) ^ poly;
			else crc >>= 1;
		}
		crct[i] = crc;
	}
	return crct;
}();

u32 crc32(u8 *data, int len) 
{
	u32 crc = ~0;
	int i;
	for (i = 0; i < len; i++) 
	{
		crc = (crc >> 8) ^ crc_table[(crc ^ data[i]) & 0xFF];
	}
	return ~crc;
}

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
	if( addr >= 0x04000000 && addr < 0x04001000 )
	{
		return __builtin_bswap32(*(u32*)&DMEM[addr&0xfff]);
	}
	if( addr >= 0x04001000 && addr < 0x04002000 )
	{
		return __builtin_bswap32(*(u32*)&IMEM[addr&0xfff]);
	}
	if( addr >= 0x04300000 && addr < 0x04400000 ) return mi_read(addr);
	if( addr >= 0x04400000 && addr < 0x04500000 ) return vi_read(addr);
	if( addr >= 0x04500000 && addr < 0x04600000 ) return ai_read(addr);
	if( addr >= 0x04600000 && addr < 0x04700000 ) return pi_read(addr);
	if( addr >= 0x04800000 && addr < 0x04900000 ) return si_read(addr);
	
	if( addr == 0x04040010 ) return 1;
	if( addr == 0x04040018 ) return 0;
	if( addr == 0x0410000C ) return 0;
	if( addr == 0x0470000C ) return 0x14;
	if( addr == 0x04080000 ) return 0;
	
	if( addr >= 0x10000000 && addr < (0x10000000 + ROM.size()) )
	{
		return __builtin_bswap32(*(u32*)&ROM[addr-0x10000000]);
	}
	
	if( addr >= 0x1FC00000 && addr <= 0x1FCFFFFF )
	{
		addr &= 0x7ff;
		if( addr >= 0x7c0 ) return __builtin_bswap32(*(u32*)&pifram[addr&0x3F]);
		if( pif_rom_enabled )
		{
			return __builtin_bswap32(*(u32*)&pifrom[addr]);
		}
		return 0;
	}
	
	printf("N64:$%X: r%i <$%X\n", u32(cpu.pc), size, addr);
	exit(1);
	return 0;
}

void n64::write(u32 addr, u64 v, int size)
{
	if( addr < 8*1024*1024 ) { sized_write(mem.data(), addr, v, size); return; }
	if( addr >= 0x04000000 && addr < 0x04001000 )
	{
		*(u32*)&DMEM[addr&0xfff] = __builtin_bswap32(u32(v));
		return;
	}
	if( addr >= 0x04001000 && addr < 0x04002000 )
	{
		*(u32*)&IMEM[addr&0xfff] = __builtin_bswap32(u32(v));
		return;
	}
	if( addr >= 0x04300000 && addr < 0x04400000 ) { mi_write(addr, v); return; }
	if( addr >= 0x04400000 && addr < 0x04500000 ) { vi_write(addr, v); return; }
	if( addr >= 0x04500000 && addr < 0x04600000 ) { ai_write(addr, v); return; }
	if( addr >= 0x04600000 && addr < 0x04700000 ) { pi_write(addr, v); return; }
	if( addr >= 0x04800000 && addr < 0x04900000 ) { si_write(addr, v); return; }
	if( addr >= 0x1FC00000 && addr <= 0x1FCFFFFF )
	{
		addr &= 0x7ff;
		if( addr < 0x7c0 ) return;
		printf("N64: pif%i write $%X = $%X\n", size, addr, u32(v));
		*(u32*)&pifram[addr&0x3F] = __builtin_bswap32(u32(v));
		if( (addr&0x3F) == 0x3c ) pif_run();
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
	
	if( ROM[0] == 0x40 && ROM[1] == 0x12 && ROM[2] == 0x37 && ROM[3] == 0x80 )
	{
		printf("N64: ROM has incorrect endianness (32bswap)\n");
		for(u32 i = 0; i < ROM.size(); i+=4)
		{
			*(u32*)&ROM[i] = __builtin_bswap32(*(u32*)&ROM[i]);
		}
	} else if( ROM[0] == 0x12 && ROM[1] == 0x40 && ROM[2] == 0x80 && ROM[3] == 0x37 ) {
		printf("N64: ROM has incorrect endianness (wordswap)\n");
		for(u32 i = 0; i < ROM.size(); i+=4)
		{
			*(u32*)&ROM[i] = (*(u32*)&ROM[i]>>16)|(*(u32*)&ROM[i]<<16);
		}	
	} else if( ROM[0] == 0x37 && ROM[1] == 0x80 && ROM[2] == 0x40 && ROM[3] == 0x12 ) {
		printf("N64: ROM has incorrect endianness (16bswap)\n");
		for(u32 i = 0; i < ROM.size(); i+=2)
		{
			*(u16*)&ROM[i] = __builtin_bswap16(*(u16*)&ROM[i]);
		}	
	}
	
	//u32 entry = __builtin_bswap32(*(u32*)&ROM[8]);
	//memcpy(mem.data()+(entry&0x7fffff), ROM.data()+0x1000, fsz < 0x101000 ? fsz : 0x100000);
	
	fp = fopen("./bios/pifdata.bin", "rb");
	if( !fp )
	{
		do_boot_hle = true;
		printf("N64: Will attempt to HLE IPL1&2.\nPut 'pifdata.bin' in the bios folder.\n");
		return true;
	}
	unu = fread(pifrom, 1, 0x7c0, fp);
	fclose(fp);
	do_boot_hle = false;
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
				audio_add(ai_L, ai_R);
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
	cpu.pc = s32(0xbfc00000); //__builtin_bswap32(*(u32*)&ROM[8]));
	cpu.npc = cpu.pc + 4;
	cpu.nnpc = cpu.npc + 4;
	
	PI_STATUS = VI_CTRL = 0;
	pif_rom_enabled = true;
	MI_VERSION = 0x02020102;
	
	if( do_boot_hle )
	{
	
	} else {
		u8 seed = 0x3F;
		u32 iplcrc = crc32(ROM.data()+0x40, 0x1000-0x40);
		switch( iplcrc )
		{
		case 0x6170A4A1:
		case 0x90BB6CB5:
			seed = 0x3F;
			break;
		case 0x0B050EE0: seed = 0x78; break;
		case 0x98BC2C86: seed = 0x91; break;
		case 0xACC8580A: seed = 0x85; break;
		default:
			printf("N64: Found IPL3 with crc32 = $%X\n", iplcrc);
			break;
		}
	
		pifram[0x26] = seed;
		pifram[0x27] = seed;
	}
	
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





