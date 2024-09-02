#include <cstdlib>
#include <cstdio>
#include "Settings.h"
#include "genesis.h"
extern console* sys;

#define Z80_RUNNING ((z80_busreq&BIT(8)) && (z80_reset&BIT(8)))

void genesis::write(u32 addr, u32 val, int size)
{
	addr &= 0xffFFff;
	if( addr < 0x400000 ) return;
	if( addr >= 0xe00000 )
	{
		if( size == 8 )
		{
			RAM[addr&0xffff] = val;
		} else if( size == 16 ) {
			u16 v = val;
			*(u16*)&RAM[addr&0xffff] = __builtin_bswap16(v);
		}
		return;
	}
	
	if( addr == 0xc0'0000 || addr == 0xc0'0002 )
	{
		if( size == 8 )
		{
			val = (val<<8)|(val&0xff);
		}
		vdp_data(val);
		return;
	}
	
	if( addr == 0xc0'0004 || addr == 0xc0'0006 )
	{
		vdp_ctrl(val);
		return;
	}
	
	if( addr >= 0xc0'0011 && addr < 0xc00018 )
	{
		psg.out(val);
		return;
	}
	
	if( addr >= 0xA00000 && addr < 0xA02000 )
	{
		ZRAM[addr&0x1fff] = val;
		return;
	}
	
	if( addr == 0xA10008 )
	{
		pcycle = 0;
		pad1_data = PAD_DATA_DEFAULT;
		pad1_ctrl = val;
		return;
	}
	if( addr == 0xA1000A )
	{
		pcycle2 = 0;
		pad2_data = PAD_DATA_DEFAULT;
		pad2_ctrl = val;
		return;
	}
	
	if( addr == 0xA10003 )
	{
		if( (pad1_data^val)&0x40 )
		{
			pcycle++;
			if( pcycle == 9 ) pcycle = 0;
		}
		pad1_data = val;
		return;
	}
	if( addr == 0xA10005 )
	{
		if( (pad2_data^val)&0x40 )
		{
			pcycle2++;
			if( pcycle2 == 9 ) pcycle2 = 0;
		}
		pad2_data = val;
		return;
	}
	
	if( addr == 0xA11100 )
	{
		if( size == 8 ) 
		{
			z80_busreq = val<<8;
		} else {
			z80_busreq = val;
		}
		z80_busreq ^= 0x100;
		return;
	}
	
	if( addr == 0xA11200 )
	{
		if( size == 8 ) 
		{
			z80_reset = val<<8;
		} else {
			z80_reset = val;
		}
		spu.pc = 0;
		return;
	}
	
	if( addr == 0xA10009 || addr == 0xA1000B ) return;
	
	printf("Write%i $%X = $%X\n", size, addr, val);
	//exit(1);
}

u32 genesis::read(u32 addr, int size)
{
	if( size != 16 )
	{
		printf("gen read size != 16, got %i\n", size);
		exit(1);
	}
	addr &= 0xffFFff;
	if( addr < ROM.size() ) return __builtin_bswap16(*(u16*)&ROM[addr]);
	if( addr < 0x400000 ) return 0;
	if( addr >= 0xe00000 ) return __builtin_bswap16(*(u16*)&RAM[addr&0xffff]);
	
	if( addr >= 0xA00000 && addr < 0xA02000 )
	{
		//ZRAM[addr&0x1fff] ^= 0xff;
		return __builtin_bswap16(*(u16*)&ZRAM[addr&0x1fff]);
	}
	
	if( addr == 0xA10000 ) return (pal ? 0xc0 : 0x80);
	if( addr == 0xA1000C ) return 0;
	
	if( addr == 0xA10008 ) return pad1_ctrl;
	if( addr == 0xA1000A ) return 0;
	if( addr == 0xA10002 )
	{
		return getpad1();
	}
	
	if( addr == 0xA10004 ) return getpad2();
	
	if( addr == 0xC0'0004 || addr == 0xC0'0006 ) 
	{
		//printf("VDP stat\n");
		return vdp_stat;
	}
	
	if( addr == 0xC0'0000 || addr == 0xC0'0002 )
	{
		return vdp_read();
	}
	
	if( addr == 0xA11100 )
	{
		return z80_busreq;	
	}
	
	if( addr == 0xA11200 )
	{
		return z80_reset;
	}
	
	printf("%X: read%i <$%X\n", cpu.pc-2, size, addr);

	//exit(1);
	
	return 0;
}

void genesis::z80_write(u16 addr, u8 val)
{
	if( addr < 0x4000 ) { ZRAM[addr&0x1fff] = val; return; }
	if( addr < 0x6000 ) return; //{ printf("FM: Write $%X = $%X\n", addr, val); return; } //todo: fm.write(addr&1, val);
	if( addr == 0x6000 )
	{ // bank reg
		z80_bank &= 0xFF8000;
		z80_bank >>= 1;
		z80_bank |= ((val&1)<<23);
		z80_bank &= 0xFF8000;
		//printf("z80_bank = $%X\n", z80_bank);
		return;
	}
	printf("z80 write $%X = $%X\n", addr, val);
	if( addr == 0x7F11 ) { psg.out(val); return; }
	if( addr >= 0x8000 )
	{
		write(z80_bank|(addr&0x7fff), val, 8);
		return;
	}
}

u8 genesis::z80_read(u16 addr)
{
	//printf("z80 read $%X\n",addr);
	if( addr < 0x4000 ) return ZRAM[addr&0x1fff];
	if( addr < 0x5000 ) { return 0; }//todo: return fm.status
	if( addr == 0x6000 )
	{ // bank reg. looks like reading resets?
		//z80_bank = 0;
		return 0xff;
	}
	if( addr >= 0x8000 )
	{
		u32 a = z80_bank|(addr&0x7fff);
		if( a >= 0xff0000 ) return RAM[a&0xffff];
		if( a < ROM.size() ) return ROM[a];
		return 0;
	}
	return 0;
}

void genesis::run_frame()
{
	pad_get_keys();

	vdp_width = (vreg[12]&1) ? 320 : 256;
	vdp_stat = 0;
	for(u32 line = 0; line < 262; ++line)
	{
		u64 target = last_target + 3420; // 3360;
		while( stamp < target )
		{
			// run the 68k
			cpu.step();
			u64 mc = cpu.icycles * 7;
			stamp += mc;
			// run the z80
			if( Z80_RUNNING )
			{
				while( spu_stamp < stamp )
				{
					spu_stamp += spu.step() * 15;
				}
			} else {
				spu_stamp = stamp;
			}
			// run the psg
			while( psg_stamp*15 < stamp )
			{
				psg_stamp += 1;
				u8 t = psg.clock(1);
				sample_cycles += 1;
				if( sample_cycles >= 81 )
				{
					sample_cycles -= 81;
					float sm = Settings::mute ? 0 : ((t/60.f)*2 - 1);
					audio_add(sm,sm);
				}
			}			
		}
		last_target = target;
		
		if( line < 224 ) draw_line(line);
		if( line == 223 ) 
		{
			if( (vreg[1]&0x60)  ) { cpu.pending_irq = 6; spu.irq_line = 1; }
			vdp_stat |= 8;
		}
		if( line == 224 ) { spu.irq_line = 0; }
	}
}

void genwrite8(u32 addr, u8 val) { dynamic_cast<genesis*>(sys)->write(addr, val, 8); }
void genwrite16(u32 addr, u16 val) { dynamic_cast<genesis*>(sys)->write(addr, val, 16); }
void genwrite32(u32 addr, u32 val)
{
	dynamic_cast<genesis*>(sys)->write(addr, val>>16, 16);
	dynamic_cast<genesis*>(sys)->write(addr+2, val&0xffff, 16);
}

u8 genread8(u32 addr)
{
	return 0xff & (dynamic_cast<genesis*>(sys)->read(addr&~1, 16) >> (((addr&1)^1)*8));
}

u16 genread16(u32 addr) 
{
	//printf("genread16 $%X\n", addr); 
	return dynamic_cast<genesis*>(sys)->read(addr, 16); 
}

u32 genread32(u32 addr)
{
	u32 res = dynamic_cast<genesis*>(sys)->read(addr, 16)<<16;
	res |= dynamic_cast<genesis*>(sys)->read(addr+2, 16);
	return res;
}

u8 z80read(u16 addr) { return dynamic_cast<genesis*>(sys)->z80_read(addr); }
void z80write(u16 addr, u8 v) { dynamic_cast<genesis*>(sys)->z80_write(addr,v); }

void genesis::reset()
{
	memset(&spu, 0, sizeof(spu));
	memset(&cpu, 0, sizeof(cpu));
	memset(&vreg, 0, 0x20);
	
	spu.reset();
	spu.sp = 0x1ff0;
	if( !ROM.empty() )
	{
		cpu.r[15] = *(u32*)&ROM[0];
		cpu.r[16] = cpu.r[15] = __builtin_bswap32(cpu.r[15]);
		cpu.pc = *(u32*)&ROM[4];
		cpu.pc = __builtin_bswap32(cpu.pc);
		cpu.sr.raw = 0x2700;	
	}
	
	cpu.mem_read8 = genread8;
	cpu.read_code16 = cpu.mem_read16 = genread16;
	cpu.mem_read32 = genread32;
	cpu.mem_write8 = genwrite8;
	cpu.mem_write16 = genwrite16;
	cpu.mem_write32 = genwrite32;
	cpu.intack = []{};
	
	spu.write = z80write;
	spu.read = z80read;
	spu.in = [](u16)->u8{ return 0xff; };
	spu.out = [](u16,u8){};
	
	key1 = key2 = key3 = 0;
	pad1_ctrl = pad2_ctrl = 0;
	pcycle = pcycle2 = 0;
	pad1_data = pad2_data = PAD_DATA_DEFAULT;
	
	stamp = spu_stamp = last_target = 0;
	vdp_cd = vdp_addr = 0;
	vdp_latch = fill_pending = false;
	vdp_width = 320;
	
	sample_cycles = 0;
	z80_reset = 0x100;
	z80_busreq = 0x100;
	psg_stamp = 0;
	return;
}

bool genesis::loadROM(const std::string fname)
{
	FILE* fp = fopen(fname.c_str(), "rb");
	if(!fp) return false;
	
	fseek(fp, 0, SEEK_END);
	auto fsz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	ROM.resize(fsz);
	[[maybe_unused]] int unu = fread(ROM.data(), 1, fsz, fp);
	fclose(fp);
	
	if( ROM[0x1F0] == 'U' ||
	    ROM[0x1F1] == 'U' ||
	    ROM[0x1F2] == 'U' )
	{
		pal = false;
	} else if( ROM[0x1F0] == 'J' || ROM[0x1F0] == 'E' ||
	    	   ROM[0x1F1] == 'J' || ROM[0x1F1] == 'E' ||
	   	   ROM[0x1F2] == 'J' || ROM[0x1F2] == 'E' )
	{
		pal = true;
	}
	
	return true;
}


