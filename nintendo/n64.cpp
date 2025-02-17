#include <print>
#include <array>
#include <filesystem>
#include "n64.h"

u32 flash_status = 0x11118001;
u8 sram[32*1024];

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

char viewbuf[0x2000];

u64 n64::read(u32 addr, int size)
{
	if( addr < 8*1024*1024 ) return sized_read(mem.data(), addr, size);
	if( addr >= 0x04000000 && addr < 0x05000000 )
	{
		//std::println("${:X}: RCP Read{} <${:X}", u32(cpu.pc), size, addr);
		if( size == 8 )
		{
			u32 val = read(addr&~3, 32);
			return val >> (8*(3-(addr&3)));
		} else if( size == 16 ) {
			u32 val = read(addr&~3, 32);
			return (addr&2) ? (val&0xffff) : (val>>16);
		}
		if( addr < 0x04040000 )
		{
			if( addr & 0x1000 )
			{
				return __builtin_bswap32(*(u32*)&IMEM[addr&0xfff]);
			}
			return __builtin_bswap32(*(u32*)&DMEM[addr&0xfff]);
		}
		//printf("N64:$%X: r%i <$%X\n", u32(cpu.pc), size, addr);
		if( addr < 0x04100000 ) { return sp_read(addr); }
		if( addr >= 0x04100000 && addr < 0x04200000 ) return dp_read(addr);
		if( addr >= 0x04300000 && addr < 0x04400000 ) return mi_read(addr);
		if( addr >= 0x04400000 && addr < 0x04500000 ) return vi_read(addr);
		if( addr >= 0x04500000 && addr < 0x04600000 ) return ai_read(addr);
		if( addr >= 0x04600000 && addr < 0x04700000 ) return pi_read(addr);
		if( addr == 0x0470000C ) return 0x14;  // RI_SELECT to 0x14 to skip a lot of IPL init
		if( addr >= 0x04800000 && addr < 0x04900000 ) return si_read(addr);
		
		//if( addr == 0x0410000C ) return 0;
		//if( addr == 0x04080000 ) return 0;
		
		return 0;
	}
	
	if( addr == 0x05000508 ) return -1;
		
	if( addr >= 0x10000000 && addr < (0x10000000 + ROM.size()) )
	{
		if( size == 16 )
		{
			return __builtin_bswap32(*(u32*)&ROM[addr-0x10000000]) >> (((addr&2)^2)*8);
		}
		if( size == 32 )
		{
			return __builtin_bswap32(*(u32*)&ROM[addr-0x10000000]);
		}
		return (__builtin_bswap32(*(u32*)&ROM[(addr&~1)-0x10000000]) >> (((addr&3)^3)*8)) & 0xff;
	}
	
	if( addr >= 0x1FC00000 && addr <= 0x1FCFFFFF )
	{
		if( size == 8 )
		{
			u32 val = read(addr&~3, 32);
			return val >> (8*(3-(addr&3)));
		} else if( size == 16 ) {
			u32 val = read(addr&~3, 32);
			return (addr&2) ? (val&0xffff) : (val>>16);
		}
		addr &= 0x7ff;
		if( addr >= 0x7c0 ) return __builtin_bswap32(*(u32*)&pifram[addr&0x3F]);
		if( pif_rom_enabled )
		{
			return __builtin_bswap32(*(u32*)&pifrom[addr]);
		}
		return 0;
	}
	
	if( addr >= 0x03F00000 && addr <= 0x03FFFFFF ) return 0;
	
	if( addr >= 0x13FF0000 && addr < 0x13FF2000 )
	{
		if( size != 32 ) { printf("ISViewer not word!\n"); exit(1); }
		return __builtin_bswap32(*(u32*)&viewbuf[addr-0x13ff0000]);
	}
	
	//if( addr >= 0x0800000 && addr <= 0xF000000 ) return __builtin_bswap32(*(u32*)&sram[addr&0x7fff]);
	
	printf("N64:$%X: r%i <$%X\n", u32(cpu.pc), size, addr);
	//exit(1);
	return 0;
}

void n64::write(u32 addr, u64 v, int size)
{
	if( addr < 8*1024*1024 ) 
	{
		sized_write(mem.data(), addr, v, size); 
		cpu.invalidate(addr); 
		if( size == 64 ) cpu.invalidate(addr+4);
		return;
	}
	if( addr >= 0x04000000 && addr < 0x05000000 )
	{
		if( size == 8 )
		{
			v <<= 8*(3-(addr&3));
			addr &= ~3;
		} else if( size == 16 ) {
			if( !(addr & 2) ) { v <<= 16; }
			addr &= ~3;
		} else if( size == 64 ) {
			v >>= 32;
		}
		if( addr < 0x04040000 )
		{
			if( addr & 0x1000 )
			{
				*(u32*)&IMEM[addr&0xffc] = __builtin_bswap32(u32(v));
				RSP.invalidate(addr);
			} else {
				*(u32*)&DMEM[addr&0xffc] = __builtin_bswap32(u32(v));
			}
			return;
		}
		if( addr < 0x04100000 ) { sp_write(addr, v); return; }
		if( addr >= 0x04100000 && addr < 0x04200000 ) { dp_write(addr, v); return; }
		if( addr >= 0x04300000 && addr < 0x04400000 ) { mi_write(addr, v); return; }
		if( addr >= 0x04400000 && addr < 0x04500000 ) { vi_write(addr, v); return; }
		if( addr >= 0x04500000 && addr < 0x04600000 ) { ai_write(addr, v); return; }
		if( addr >= 0x04600000 && addr < 0x04700000 ) { pi_write(addr, v); return; }
		if( addr >= 0x04800000 && addr < 0x04900000 ) { si_write(addr, v); return; }
		
		printf("N64: Unhandled RCP Write%i $%X = $%X\n", size, addr, u32(v));
		return;
	}
	
	if( addr >= 0x1FC00000 && addr <= 0x1FCFFFFF )
	{
		if( size == 8 )
		{
			v <<= 8*(3-(addr&3));
			addr &= ~3;
		} else if( size == 16 ) {
			if( !(addr & 2) ) { v <<= 16; }
			addr &= ~3;
		} else if( size == 64 ) {
			v >>= 32;
		}
		addr &= 0x7ff;
		if( addr < 0x7c0 ) return;
		printf("N64: pif%i write $%X = $%X\n", size, addr, u32(v));
		*(u32*)&pifram[addr&0x3F] = __builtin_bswap32(u32(v));
		if( (addr&0x3F) == 0x3c ) pif_run();
		return;
	}
	if( addr == 0x13FF0014 )
	{
		u32 len = (v < 0x200) ? v : 0x200;
		for(u32 i = 0; i < len; ++i)
		{
			fputc(viewbuf[0x20+i], stderr);
		}
		return;
	}
	if( addr >= 0x13FF0000 && addr < 0x13FF2000 )
	{
		if( size != 32 ) { printf("ISViewer not word!\n"); exit(1); }
		*(u32*)&viewbuf[addr-0x13ff0000] = __builtin_bswap32(u32(v));
		return;
	}
	
	/*if( addr >= 0x0800000 && addr <= 0xF000000 ) 
	{
		*(u32*)&sram[addr&0x7fff] = __builtin_bswap32(u32(v));
		return;
	}*/
	
	
	printf("$%X: W%i $%X = $%lX\n", u32(cpu.pc), size, addr, v);
}

bool n64::loadROM(const std::string fname)
{
	FILE* fp = fopen(fname.c_str(), "rb");
	if(!fp)
	{
		printf("Unable to open '%s'\n", fname.c_str());
		return false;
	}

	mem.clear();
	mem.resize(8*1024*1024);
	
	fseek(fp, 0, SEEK_END);
	auto fsz = ftell(fp);
	fsz += 0xfffff;
	fsz &= ~0xfffff;
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
	} else if( ROM[0] != 0x80 || ROM[1] != 0x37 || ROM[2] != 0x12 || ROM[3] != 0x40 ) {
		std::println("N64: ROM has unknown byte order. First bytes {:X}{:X}{:X}{:X}", ROM[0], ROM[1], ROM[2], ROM[3]);
	}
	
	//u32 entry = __builtin_bswap32(*(u32*)&ROM[8]);
	//memcpy(mem.data()+(entry&0x7fffff), ROM.data()+0x1000, fsz < 0x101000 ? fsz : 0x100000);
	eeprom_written = false;
	std::string sfname = fname;
	if( auto pos = sfname.rfind('.'); pos != std::string::npos ) sfname.erase(pos);
	sfname += ".eeprom";
	save_file = sfname;
	FILE* eep = fopen(sfname.c_str(), "rb");
	printf("reading <%s>\n", sfname.c_str());
	if( eep )
	{
		printf("reading <%s>\n", sfname.c_str());
		unu = fread(eeprom, 1, 2048, eep);
		fclose(eep);
	}
	
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
	for(u32 line = 0; line < 263; ++line)
	{
		if( (VI_CTRL & 3) > 1 )
		{
			VI_V_CURRENT = (VI_V_CURRENT&1) | (line<<1);
			if( (VI_V_CURRENT>>1) == ((VI_V_INTR&0x3ff)>>1) ) raise_mi_bit(MI_INTR_VI_BIT);
		} else {
			VI_V_CURRENT = 0;
		}
		for(u32 i = 0; i < 5700; ++i)
		{
			cpu.step();
			rspdiv += 1;
			if( rspdiv >= 3 ) rspdiv = 0;
			if( !(SP_STATUS & 1) && (rspdiv != 2) )
			{
				RSP.step();
			}
			//dp_send();
			/*if( pi_cycles_til_irq )
			{
				pi_cycles_til_irq -= 1;
				if( pi_cycles_til_irq == 0 )
				{
					PI_STATUS &= ~3;
					raise_mi_bit(MI_INTR_PI_BIT);
				}
			}
			if( si_cycles_til_irq )
			{
				si_cycles_til_irq -= 1;
				if( si_cycles_til_irq == 0 )
				{
					SI_STATUS &= ~1;
					raise_mi_bit(MI_INTR_SI_BIT);
				}			
			}
			*/
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
						if( ai_buf[0].valid )
						{
							raise_mi_bit(MI_INTR_AI_BIT);
							//printf("N64: AI interrupt 2\n");
						}
					}
					
					ai_L = (L / 32768.f);
					ai_R = (R / 32768.f);				
				}
			}
			ai_output_cycles += 1;
			if( ai_output_cycles >= (93750000/44100) )
			{
				ai_output_cycles = 0; // -= (93750000/44100);
				audio_add(ai_L, ai_R);
			}
		}
	}
	
	if( (VI_CTRL&BIT(6)) && (VI_CTRL&3) ) VI_V_CURRENT ^= 1;
	
	vi_draw_frame();
}

void n64::reset()
{
	cpu.RAM = mem.data();
	mem.clear();
	mem.resize(8*1024*1024);
	
	for(u32 i = 0; i < 0x10; ++i) pi_regs[i] = vi_regs[i] = 0;	
	for(u32 i = 0; i < 1024; ++i) RSP.invalidate(i<<2);

	curwidth = 320;
	curheight = 240;
	curbpp = 16;
	cpu.phys_read = [&](u64 a, int s)->u64 { return read(a, s); };
	cpu.phys_write= [&](u64 a, u64 v, int s) { write(a, v, s); };
	cpu.reset();
	cpu.pc = s32(0xbfc00000); //__builtin_bswap32(*(u32*)&ROM[8]));
	cpu.npc = cpu.pc + 4;
	cpu.nnpc = cpu.npc + 4;
	memset(&mem[0], 0, 0x800000);
	memset(DMEM, 0, 0x1000);
	memset(IMEM, 0, 0x1000);
	
	for(u32 i = 0; i < 0x1FFFFFFF+1; i += 0x1000)
	{
		cpu.readers[i>>12] = get_reader(i);
	}
	
	PI_STATUS = SI_STATUS = VI_CTRL = 0;
	memset(si_regs, 0, 30);
	pif_rom_enabled = true;
	MI_VERSION = 0x02020102;
	MI_INTERRUPT = 0;
	MI_MASK = 0;
	
	RDP.rdp_irq = [&](){ raise_mi_bit(MI_INTR_DP_BIT); DP_STATUS &= ~(BIT(3)|BIT(5)|BIT(7)); }; //|BIT(7) //??
	RDP.rdram = mem.data();
	for(u32 i = 0; i < 8; ++i) dp_regs[i] = 0;
	DP_STATUS = 0x80;
	dp_xfer.valid = false;

	for(u32 i = 0; i < 8; ++i) sp_regs[i] = 0;
	SP_STATUS = 1;
	RSP.broke = [&]() 
		{ 
			if( SP_STATUS & BIT(6) ) 
			{ 
				raise_mi_bit(MI_INTR_SP_BIT); 
			} 
			SP_STATUS |= 3;
		};
	RSP.IMEM = IMEM;
	RSP.DMEM = DMEM;
	RSP.dp_write = [&](u32 a, u32 v) { dp_write(a, v); };
	RSP.sp_write = [&](u32 a, u32 v) { sp_write(a, v); };
	RSP.dp_read = [&](u32 a) -> u32 { return dp_read(a); };
	RSP.sp_read = [&](u32 a) -> u32 { return sp_read(a); };
	rspdiv = 0;
		
	*(u32*)&mem[0x318] = __builtin_bswap32(0x800000);
	*(u32*)&mem[0x3f0] = __builtin_bswap32(0x800000);	
	memset(pifram, 0, 64);
	
	if( do_boot_hle )
	{
		printf("boot HLE not yet supported\n");
		exit(1);
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
	
		pifram[0x24] = 0;
		pifram[0x25] = 4;
		pifram[0x26] = seed;
		pifram[0x27] = seed;
	}
	
	ai_buf[0].valid = false;
	ai_buf[1].valid = false;
	ai_buf[0].length = ai_buf[1].length = 0;
	ai_dma_enabled = false;
	ai_cycles_per_sample = 800;
	ai_cycles = ai_output_cycles = 0;
}

void n64::raise_mi_bit(u32 b)
{
	//if( b == 0 ) printf("MI IRQ raised, intr bit %i\n", b);
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
			//printf("MI: cleared DP irq\n");
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
			MI_MASK &= ~16;
		} else if( pi == 2 ) {
			MI_MASK |= 16;
		}
		if( dp == 1 )
		{
			MI_MASK &= ~32;
		} else if( dp == 2 ) {
			MI_MASK |= 32;
		}
		
		//printf("N64: MI_MASK = $%X, v = $%X\n", MI_MASK, u32(v));
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

n64::~n64()
{
	if( eeprom_written && save_file.size() )
	{
		FILE* fp = fopen(save_file.c_str(), "wb");
		fwrite(eeprom, 1, 2048, fp);
		fclose(fp);
	}
}

std::function<u64(u32, int)> n64::get_reader(u32 phys)
{
	if( phys < 0x800000 ) return [&](u32 a, int s) -> u64 { return sized_read(mem.data(), a, s); };
	if( phys < 0x0400'0000 ) return [](u32, int) -> u64 { return 0; };
	if( phys < 0x0404'0000 ) return [&](u32 a, int) -> u64 
		{ 
			if( a & BIT(12) ) return __builtin_bswap32(*(u32*)&IMEM[a&0xfff]);
			return __builtin_bswap32(*(u32*)&DMEM[a&0xfff]);
		};
	if( phys < 0x040C'0000 ) return [&](u32 a, int) -> u64
		{
			return sp_read(a);
		};
	if( phys < 0x0410'0000 ) return [](u32 a, int s)->u64 { std::println("n64: unmapped access{} rd ${:X}", s, a); return 0; };
	if( phys < 0x0420'0000 ) return [&](u32 a, int) -> u64
		{
			return dp_read(a);
		};
	if( phys < 0x0430'0000 ) return [](u32 a, int) -> u64 { std::println("n64: dp span reg read ${:X}", a); return 0; };
	if( phys < 0x0440'0000 ) return [&](u32 a, int) -> u64 { return mi_read(a); };
	if( phys < 0x0450'0000 ) return [&](u32 a, int) -> u64 { return vi_read(a); };
	if( phys < 0x0460'0000 ) return [&](u32 a, int) -> u64 { return ai_read(a); };
	if( phys < 0x0470'0000 ) return [&](u32 a, int) -> u64 { return pi_read(a); };
	if( phys < 0x0480'0000 ) return [](u32 a, int) -> u64 { if( a == 0x0470000C ) return 0x14; else return 0; };
	if( phys < 0x0490'0000 ) return [&](u32 a, int) -> u64 { return si_read(a); };
	if( phys < 0x0500'0000 ) return [](u32 a, int s) -> u64 { std::println("n64: unmapped access{} rd ${:X}", s, a); return 0; };


	return [&](u32 a, int s) { return read(a, s); };
}


