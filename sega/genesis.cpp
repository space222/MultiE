#include <print>
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include "Settings.h"
#include "genesis.h"
extern console* sys;

#define Z80_RUNNING ((z80_busreq&BIT(8)) && (z80_reset&BIT(8)))

void genesis::write(u32 addr, u32 val, int size)
{
	addr &= 0xffFFff;
	if( addr < 0x400000 ) 
	{
		if( ROM.size() <= 2*1024*1024 && addr > 0x200000 && addr <= 0x20FFFF )
		{
			printf("Gen: save ram write $%X = $%X\n", addr, val);
			save[(addr&0xffff)>>1] = val;
		}
		return;
	}
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
		vdp_latch = false;
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
	
	if( addr >= 0xA04000 && addr < 0xA05000 )
	{
		fm_write(addr, val);
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
	
	if( addr == 0xA15100 || addr == 0xA15101 )
	{
		printf("ADEN =$%X\n", val);
		if( !ADEN ) 
		{
			ADEN = val&1;
			if( !ADEN ) return;
			/*
			// was going to HLE the 32X boot, but maybe that should wait
			u32 cartaddr = __builtin_bswap32(*(u32*)&ROM[0x3D4]);
			u32 ramaddr  = __builtin_bswap32(*(u32*)&ROM[0x3D8]);
			u32 bytelen =  __builtin_bswap32(*(u32*)&ROM[0x3DC]);
			printf("cartaddr = $%X, ramaddr = $%X, len = %i\n", cartaddr, ramaddr, bytelen);
			for(u32 i = 0; i < bytelen; ++i) sdram[ramaddr+i] = ROM[cartaddr+i];
			cpu32x[0].pc = __builtin_bswap32(*(u32*)&ROM[0x3E0]);
			cpu32x[1].pc = __builtin_bswap32(*(u32*)&ROM[0x3E4]);
			cpu32x[0].vbr= __builtin_bswap32(*(u32*)&ROM[0x3E8]);
			cpu32x[1].vbr= __builtin_bswap32(*(u32*)&ROM[0x3EC]);
			cpu32x[0].gbr = cpu32x[1].gbr = 0x20004000;
			*(u32*)&comms[0] = __builtin_bswap32(0x4D5F4F4B);
			*(u32*)&comms[4] = __builtin_bswap32(0x535F4F4B);
			*/
		}
		if( !RES  ) RES = val&2;
		if( size == 16 ) FM = val&0x8000;
		return;
	}
	
	if( ADEN )
	{
		write32x(addr, val, size);
		return;
	}
	
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
	if( ADEN && addr < 0x400000 ) return read32x(addr, size);	
	if( addr < ROM.size() ) return __builtin_bswap16(*(u16*)&ROM[addr]);
	if( ROM.size() <= 2*1024*1024 && addr > 0x200000 && addr <= 0x20FFFF )
	{
		return save[(addr&0xffff)>>1];
	}
	if( addr < 0x400000 ) return 0;
	if( addr >= 0xe00000 ) return __builtin_bswap16(*(u16*)&RAM[addr&0xffff]);
	
	if( addr >= 0xA00000 && addr < 0xA02000 )
	{
		//ZRAM[addr&0x1fff] ^= 0xff;
		return __builtin_bswap16(*(u16*)&ZRAM[addr&0x1fff]);
	}
	
	if( addr >= 0xA04000 && addr < 0xA05000 )
	{
		return fm_read();
	}
	
	// Chaotix on 32X needs to see bit 6 set, everything else requires unset to detect NTSC
	if( addr == 0xA10000 ) return (pal ? BIT(6):0)|(domestic ?0:BIT(7)); //(pal ? 0xc0 : 0x80);
	if( addr == 0xA1000C ) return 0;
	
	if( addr == 0xA10008 ) return pad1_ctrl;
	if( addr == 0xA1000A ) return pad2_ctrl;
	if( addr == 0xA10002 )
	{
		return getpad1();
	}
	
	if( addr == 0xA10004 ) return getpad2();
	
	if( addr == 0xC0'0004 || addr == 0xC0'0006 ) 
	{
		//printf("VDP stat\n");
		vdp_latch = false;
		return vdp_stat;
	}
	
	if( addr == 0xC0'0000 || addr == 0xC0'0002 )
	{
		vdp_latch = false;
		return vdp_read();
	}
	
	if( addr == 0xC00008 )
	{
		std::println("HV counter read");
		//exit(1);
		return 0;
	}
	
	if( addr == 0xA11100 )
	{
		return z80_busreq;	
	}
	
	if( addr == 0xA11200 )
	{
		return z80_reset;
	}
	
	if( addr == 0xA130EC ) return ('M'<<8)|'A';
	if( addr == 0xA130EE ) return ('R'<<8)|'S';
	if( addr == 0xA15100 ) return 0x80|RES|ADEN;
	if( ADEN ) return read32x(addr, size);

	printf("%X: read%i <$%X\n", cpu.pc-2, size, addr);
	return 0;
}

void genesis::z80_write(u16 addr, u8 val)
{
	if( addr < 0x4000 ) { ZRAM[addr&0x1fff] = val; return; }
	if( addr < 0x6000 ) { fm_write(addr, val); return; }
	//{ printf("FM: Write $%X = $%X\n", addr, val); return; } //todo: fm.write(addr&3, val);
	if( addr == 0x6000 )
	{ // bank reg
		z80_bank &= 0xFF8000;
		z80_bank >>= 1;
		z80_bank |= ((val&1)<<23);
		z80_bank &= 0xFF8000;
		//printf("z80_bank = $%X\n", z80_bank);
		return;
	}
	if( addr == 0x7F11 ) { psg.out(val); return; }
	if( addr >= 0x8000 )
	{
		write(z80_bank|(addr&0x7fff), val, 8);
		return;
	}
	printf("z80 write $%X = $%X\n", addr, val);
}

u8 genesis::z80_read(u16 addr)
{
	//printf("z80 read $%X\n",addr);
	if( addr < 0x4000 ) return ZRAM[addr&0x1fff];
	if( addr < 0x5000 ) { return fm_read(); }//todo: return fm.status
	if( addr == 0x6000 )
	{ // bank reg. looks like reading resets?
		//z80_bank = 0;
		return 0xff;
	}
	if( addr >= 0x8000 )
	{
		u32 a = z80_bank|(addr&0x7fff);
		//if( a >= 0xff0000 ) return RAM[a&0xffff];
		//if( a < ROM.size() ) return ROM[a];
		//return 0;
		u16 v = read(a&~1, 16);
		a ^= 1;
		return v >> ((a&1)*8);
	}
	return 0;
}

void genesis::run_frame()
{
	pad_get_keys();

	vdp_width = (vreg[12]&1) ? 320 : 256;
	vdp_stat = 0;
	fb_ctrl &= ~0x8000;
	
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
			fm_run();
			while( psg_stamp*15 < stamp )
			{
				psg_stamp += 1;
				u8 t = psg.clock(1);
				sample_cycles += 1;
				if( sample_cycles >= 81 )
				{
					sample_cycles -= 81;
					float sm = ((t/60.f)*2 - 1);
					float fm = fm_out/float(fm_count);
					sm = (sm/5.f) + fm; //(fm*2 - 1);
					fm_count = fm_total = fm_out = 0;
					
					sm = std::clamp(sm, -1.f, 1.f);
					audio_add(sm,sm);
				}
			}
			
			// run the 32X
			if( ADEN && RES )
			{
				for(u32 i = 0; i < cpu.icycles*3; ++i)
				{
					cpu32x[0].step();
					cpu32x[1].step();
				}
			}
		}
		last_target = target;
		
		if( line < 224 )
		{
			draw_line(line);
			vdp_hcnt -= 1;
			if( vdp_hcnt == 0 )
			{
				vdp_hcnt = vreg[0xA];
				if( (vreg[0]&BIT(4)) && cpu.pending_irq == 0  ) cpu.pending_irq = 4;
			}
		} else {
			vdp_hcnt = vreg[0xA];
		}
		if( line == 223 ) 
		{
			if( (vreg[1]&BIT(5))  ) { cpu.pending_irq = 6; spu.irq_line = 1; }
			vdp_stat |= 8;
			fb_ctrl |= 0x8000;
			fb_ctrl &= ~1;
			fb_ctrl |= (fb_ctrl_fsnext&1);
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
	
	OPN2_Reset(&synth);
	OPN2_SetChipType(3);
	fm_stamp = 0;
	fm_count = 0;
	fm_total = 0;
	fm_out = 0;
	
	ADEN = bank9 = RES = bmpmode = fb_ctrl = 0;
	RV = 0;
	*(u32*)&vecrom[0] = 0;
	for(u32 i = 0; i < 63; ++i)
	{
		*(u32*)&vecrom[(i+1)*4] = __builtin_bswap32(0x880200+i*6);
	}
	cpu32x[0].memread = [](u32 a, int s)->u32 { return dynamic_cast<genesis*>(sys)->sh2master_read(a,s); };
	cpu32x[0].memwrite = [](u32 a, u32 v, int s) { dynamic_cast<genesis*>(sys)->sh2master_write(a,v,s); };
	cpu32x[1].memread = [](u32 a, int s)->u32 { return dynamic_cast<genesis*>(sys)->sh2slave_read(a,s); };
	cpu32x[1].memwrite = [](u32 a, u32 v, int s) { dynamic_cast<genesis*>(sys)->sh2slave_write(a,v,s); };
	cpu32x[0].reset();
	cpu32x[1].reset();
	
	cpu32x[0].pc = __builtin_bswap32(*(u32*)&bios32xM[0]);
	cpu32x[1].pc = __builtin_bswap32(*(u32*)&bios32xS[0]);
	printf("32X SH2 PCs = $%X, $%X\n", cpu32x[0].pc, cpu32x[1].pc);
	cpu32x[0].r[15] = __builtin_bswap32(*(u32*)&bios32xM[4]);
	cpu32x[1].r[15] = __builtin_bswap32(*(u32*)&bios32xS[4]);
	
	return;
}

bool genesis::loadROM(const std::string fname)
{
	FILE* fp = fopen("./bios/32X_BIOS1.bin", "rb");
	if( ! fp ) 
	{
		printf("Cant find 32X SH2 bootroms\n");
		return false;
	}
	fseek(fp, 0, SEEK_END);
	auto fsz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	[[maybe_unused]] int u = fread(bios32xM, 1, fsz, fp);
	fclose(fp);

	fp = fopen("./bios/32X_BIOS2.bin", "rb");
	if( ! fp ) 
	{
		printf("Cant find 32X SH2 bootroms\n");
		return false;
	}
	fseek(fp, 0, SEEK_END);
	fsz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	[[maybe_unused]] int u2 = fread(bios32xS, 1, fsz, fp);
	fclose(fp);

	fp = fopen(fname.c_str(), "rb");
	if(!fp) return false;
	
	fseek(fp, 0, SEEK_END);
	fsz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	ROM.resize(fsz);
	[[maybe_unused]] int unu = fread(ROM.data(), 1, fsz, fp);
	fclose(fp);
	
	pal = domestic = false;
	
	if( ROM[0x1F0] == 'U' ||
	    ROM[0x1F1] == 'U' ||
	    ROM[0x1F2] == 'U' )
	{
		pal = domestic = false;
	} else if( ROM[0x1F0] == 'J' ||
	    	   ROM[0x1F1] == 'J' ||
	   	   ROM[0x1F2] == 'J' )
	{
		pal = false;
		domestic = true;
	} else if( ROM[0x1F0] == 'E' ||
	    	   ROM[0x1F1] == 'E' ||
	    	   ROM[0x1F2] == 'E' ) {
	 	pal = true;
	 	domestic = false;	    	   
	}
	
	return true;
}


