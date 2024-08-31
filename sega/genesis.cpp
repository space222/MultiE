#include <cstdlib>
#include <cstdio>
#include "genesis.h"
extern console* sys;

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
	
	if( addr == 0xA10000 ) return 0x80; //todo: detect rom region
	if( addr == 0xA10008 || addr == 0xA1000C ) return 0;
	
	if( addr == 0xC0'0004 ) return 0; //vdp_stat;

	printf("%X: read%i <$%X\n", cpu.pc-2, size, addr);
	//exit(1);
	
	return 0;
}

void genesis::run_frame()
{
	vdp_width = (vreg[12]&1) ? 320 : 256;
	vdp_stat = 0;
	for(u32 line = 0; line < 262; ++line)
	{
		u64 target = last_target + 3360;
		while( stamp < target )
		{
			cpu.step();
			u64 mc = cpu.icycles * 7;
			stamp += mc;
		}
		last_target = target;
		
		if( line < 224 ) draw_line(line);
		if( line == 223 ) 
		{
			if( (vreg[1]&BIT(5)) ) cpu.pending_irq = 6;
			vdp_stat |= 0x80;
		}
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

void genesis::reset()
{
	memset(&spu, 0, sizeof(spu));
	memset(&cpu, 0, sizeof(cpu));
	spu.reset();
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
	
	stamp = last_target = 0;
	vdp_cd = vdp_addr = 0;
	vdp_latch = fill_pending = false;
	vdp_width = 320;
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
	
	return true;
}


