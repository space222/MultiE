#include <cstdio>
#include "genesis.h"
#include "util.h"

u32 genesis::sh2master_read(u32 addr, int size)
{
	
	
	return 0xcafe;
}

u32 genesis::sh2slave_read(u32 addr, int size)
{
	
	return 0xbabe;
}

void genesis::sh2master_write(u32 addr, u32 val, int size)
{
	
	printf("SH2m: write%i $%X = $%X\n", size, addr, val);
}

void genesis::sh2slave_write(u32 addr, u32 val, int size)
{
	
	printf("SH2s: write%i $%X = $%X\n", size, addr, val);
}

u32 genesis::read32x(u32 addr, int size)
{
	
	printf("32X-68k:%X: read%i <$%X\n", cpu.pc-2 , size, addr);

	
	return 0xfc;
}

void genesis::write32x(u32 addr, u32 val, int size)
{
	
	
	printf("32X-68k: Write%i $%X = $%X\n", size, addr, val);
	return;
}


