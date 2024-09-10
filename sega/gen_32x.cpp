#include <cstdio>
#include "genesis.h"

static u32 sized_read(u8* data, u32 addr, int size)
{
	switch( size )
	{
	case 8: return data[addr];
	case 16: return __builtin_bswap16(*(u16*)&data[addr]);
	default: break;
	}
	return __builtin_bswap32(*(u32*)&data[addr]);
}

u32 genesis::sh2master_read(u32 addr, int size)
{
	if( addr >= 0xc0000000u && addr < 0xc0001000u )
	{
		return sized_read(uppercache, addr-0xc0000000, size);
	}
	addr &= 0x1fffFFFFu;
	if( addr < 0x800 ) return sized_read(bios32xM, addr, size);
	if( addr >= 0x06000000 && addr < 0x06050000 ) return sized_read(sdram, addr-0x06000000, size);
	if( addr >= 0x02000000 && addr < 0x02400000 )
	{
		addr -= 0x02000000;
		if( addr < ROM.size() ) return sized_read(ROM.data(), addr, size);
		return 0;	
	}
	if( addr >= 0x4020 && addr < 0x4030 ) return sized_read(comms, addr-0x4020, size);
	if( addr == 0x410A ) return fb_ctrl;
	printf("SH2m: read%i <$%X\n", size, addr);
	
	
	return 0xcafe;
}

u32 genesis::sh2slave_read(u32 addr, int size)
{
	if( addr >= 0xc0000000u && addr < 0xc0001000u )
	{
		return sized_read(uppercache, addr-0xc0000000, size);
	}
	addr &= 0x1fffFFFFu;
	if( addr < 0x800 ) return sized_read(bios32xS, addr, size);
	if( addr >= 0x06000000 && addr < 0x06050000 ) return sized_read(sdram, addr-0x06000000, size);
	if( addr >= 0x02000000 && addr < 0x02400000 )
	{
		addr -= 0x02000000;
		if( addr < ROM.size() ) return sized_read(ROM.data(), addr, size);
		return 0;	
	}
	if( addr >= 0x4020 && addr < 0x4030 ) return sized_read(comms, addr-0x4020, size);
	if( addr == 0x410A ) return fb_ctrl;
	printf("SH2s: read%i <$%X\n", size, addr);
	
	return 0xbabe;
}

void genesis::sh2master_write(u32 addr, u32 val, int size)
{
	if( addr >= 0xc0000000 && addr < 0xc0001000 )
	{
		addr -= 0xc0000000;
		if( size == 8 ) uppercache[addr] = val;
		else if( size == 16 ) *(u16*)&uppercache[addr] = __builtin_bswap16(u16(val));
		else *(u32*)&uppercache[addr] = __builtin_bswap32(val);
		return;
	}
	addr &= 0x1fffFFFF;
	if( addr >= 0x06000000 && addr < 0x06050000 ) 
	{
		addr -= 0x06000000;
		if( size == 8 ) sdram[addr] = val;
		else if( size == 16 ) *(u16*)&sdram[addr] = __builtin_bswap16(u16(val));
		else *(u32*)&sdram[addr] = __builtin_bswap32(val);
		return;
	}
	if( addr >= 0x4020 && addr < 0x4030 )
	{
		addr -= 0x4020;
		if( size == 8 ) comms[addr] = val;
		else if( size == 16 ) *(u16*)&comms[addr] = __builtin_bswap16(u16(val));
		else *(u32*)&comms[addr] = __builtin_bswap32(val);
		printf("SH2m: comms write%i $%X = $%X\n", size, addr, val);
		return;	
	}
	printf("SH2m: write%i $%X = $%X\n", size, addr, val);
}

void genesis::sh2slave_write(u32 addr, u32 val, int size)
{
	if( addr >= 0xc0000000 && addr < 0xc0001000 )
	{
		addr -= 0xc0000000;
		if( size == 8 ) uppercache[addr] = val;
		else if( size == 16 ) *(u16*)&uppercache[addr] = __builtin_bswap16(u16(val));
		else *(u32*)&uppercache[addr] = __builtin_bswap32(val);
		return;
	}
	addr &= 0x1fffFFFF;
	if( addr >= 0x06000000 && addr < 0x06050000 ) 
	{
		addr -= 0x06000000;
		if( size == 8 ) sdram[addr] = val;
		else if( size == 16 ) *(u16*)&sdram[addr] = __builtin_bswap16(u16(val));
		else *(u32*)&sdram[addr] = __builtin_bswap32(val);
		return;
	}
	if( addr >= 0x4020 && addr < 0x4030 )
	{
		addr -= 0x4020;
		if( size == 8 ) comms[addr] = val;
		else if( size == 16 ) *(u16*)&comms[addr] = __builtin_bswap16(u16(val));
		else *(u32*)&comms[addr] = __builtin_bswap32(val);
		printf("SH2m: comms write%i $%X = $%X\n", size, addr, val);
		return;	
	}
	printf("SH2s: write%i $%X = $%X\n", size, addr, val);
}

u32 genesis::read32x(u32 addr, int size)
{
	if( addr < 0x100 )
	{
		return __builtin_bswap16(*(u16*)&vecrom[addr]);
	}
	
	if( addr < 0x400000 )
	{
		if( RV == 1 && addr < ROM.size() ) return __builtin_bswap16(*(u16*)&ROM[addr]);
		return 0;	
	}
	
	if( addr >= 0x880000 && addr < 0x900000 )
	{
		if( RV == 0 ) return __builtin_bswap16(*(u16*)&ROM[addr-0x880000]);
		printf("failed to read from $880000 cart area\n");
		return 0;
	}
	if( addr >= 0x900000 && addr < 0xA00000 )
	{
		addr -= 0x900000;
		addr += bank9;
		if( RV == 0 && addr < ROM.size() ) return __builtin_bswap16(*(u16*)&ROM[addr]);
		return 0;
	}
	
	if( addr >= 0x840000 && addr < 0x860000 )
	{
		//todo: blocking access important?
		return __builtin_bswap16(*(u16*)&fb32x[(((fb_ctrl&1)^1)*128*1024) + (addr-0x840000)]);
	}
	if( addr >= 0x860000 && addr < 0x880000 )
	{
		//todo: can read fb from overwrite-image area?
		return __builtin_bswap16(*(u16*)&fb32x[(((fb_ctrl&1)^1)*128*1024) + (addr-0x860000)]);
	}
	
	printf("32X-68k:%X: read%i <$%X\n", cpu.pc-2 , size, addr);
	
	
	if( addr == 0xA15180 ) { return bmpmode; }
	if( addr == 0xA1518A ) { return fb_ctrl; }
	if( addr >= 0xA15120 && addr < 0xA15130 ) return sized_read(comms, addr-0xA15120, size);
	
	return 0;
}

void genesis::write32x(u32 addr, u32 val, int size)
{
	if( addr >= 0x840000 && addr < 0x860000 )
	{
		//todo: blocking access important?
		if( size == 16 ) *(u16*)&fb32x[(((fb_ctrl&1)^1)*128*1024) + (addr-0x840000)] = __builtin_bswap16(u16(val));
		else if( size == 8 ) fb32x[(((fb_ctrl&1)^1)*128*1024) + (addr-0x840000)] = val;
		return;
	}
	if( addr >= 0x860000 && addr < 0x880000 )
	{
		if( size == 16 )
		{
			if( u8(val>>8) ) fb32x[(((fb_ctrl&1)^1)*128*1024) + (addr-0x860000)] = val>>8;
			addr += 1;
		}
		if( u8(val) ) fb32x[(((fb_ctrl&1)^1)*128*1024) + (addr-0x860000)] = val;
		return;
	}
	
	if( addr == 0xA15184 ) { autofill_len = (val&0xff)+1; return; }
	if( addr == 0xA15186 ) { autofill_addr = val<<1; return; }
	if( addr == 0xA15188 )
	{
		printf("32X-68k: auto fill @$%X, %i words\n", autofill_addr, autofill_len);
		for(u32 i = 0; i < autofill_len; ++i, autofill_addr+=2) 
		{
			*(u16*)&fb32x[(((fb_ctrl&1)^1)*128*1024) + autofill_addr] = __builtin_bswap16(u16(autofill_val));
		}
		return;
	}
	
	if( addr == 0xA15180 )
	{
		bmpmode = val;
		if( !(bmpmode & 3) ) fb_ctrl = (fb_ctrl&~1) | (fb_ctrl_fsnext&1);
		return;
	}
	if( addr == 0xA1518A || addr == 0xA1518B )
	{
		fb_ctrl_fsnext = val;
		if( !(bmpmode & 3) || (fb_ctrl&0x8000) ) fb_ctrl = (fb_ctrl&~1) | (fb_ctrl_fsnext&1);
		return;
	}
	
	if( addr >= 0xA15120 && addr < 0xA15130 )
	{
		addr -= 0xA15120;
		if( size == 8 ) comms[addr] = val;
		else *(u16*)&comms[addr] = __builtin_bswap16(u16(val));
		return;
	}	
	
	printf("32X-68k: Write%i $%X = $%X\n", size, addr, val);
	return;
}


