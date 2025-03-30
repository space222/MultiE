#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <bit>
#include <SFML/Graphics.hpp>
#include "types.h"

u16 mem_read16(u32);
void mem_write16(u32, u16);
void gfx_write16(u32 addr, u16 val);
u16 gfx_read16(u32 addr);

void arm7_dumpregs(arm7&);
u32 getreg(arm7&, u32);

extern arm7 cpu;

extern u8 BIOS[16*1024];
extern u8* ROM;
extern size_t romsize;

u32 io88 = 0; // temporaries to see if I can get thru BIOS
u32 io204 = 0;
u32 halt_mode = 0xff;

u16 rtc_read(u32);
void rtc_write(u32, u16);

u16 dma_read16(u32);
void dma_write16(u32, u16);
void gfx_write16(u32, u16);
u16 gfx_read16(u32);

u16 timers_read16(u32);
void timers_write16(u32, u16);

u8 SRAM[128*1024];
int flash_state = 0;
u8 flash_cmd = 0;
u32 flash_bank = 0;
extern int detected_sram_type;

extern u16 gfxregs[50];

u8 WRAM_lo[0x40000] = {0};
u8 WRAM_hi[0x8000] = {0};
u8 palette[0x400];
u8 OAM[0x400];
u8 VRAM[0x18000] = {0};
u16 KEYS = 0x3ff;
u16 IE = 0;
u16 IF = 0;
u16 IME = 0;
u16 WAITCNT = 0;
bool rtc_active = false;

void mem_write8(u32 addr, u8 val)
{
	//printf("write8 %X = %X\n", addr, val);
	
	if( addr < 0x02000000ul )
	{ // BIOS
		return;	
	}
	if( addr < 0x03000000ul )
	{ // WRAM_lo
		WRAM_lo[addr&0x3FFFF] = val;
		return;	
	}
	if( addr < 0x04000000ul )
	{ // WRAM_hi
		WRAM_hi[addr&0x7fff] = val;
		return;
	}
	if( addr < 0x05000000ul )
	{ // IO
		//todo: a suprising number of games do 8bit writes to IO
		if( addr == 0x04000208 )
		{
			IME = val&1;
			return;
		}
		if( addr == 0x04000202 )
		{   // ok BIOS does some irq 8bit handling
			IF = (IF&0xff00) | (IF & ~(u16)val);
			return;
		}
		if( addr == 0x04000300 )
		{	// BIOS soft reset flag (no idea if need to actually emu)
			return;
		}
		if( addr == 0x04000301 ) 
		{
			//printf("System in HALT Mode = %i\n", val);
			halt_mode = val;
			return;
		}
		if( addr == 0x04000410 )
		{	// BIOS writes 0xFF, gbatek theorizes BIOS bug.
			return;
		}
		if( addr == 0x04000000 )
		{
			u16 dcnt = mem_read16(0x04000000);
			dcnt &= 0xff00;
			dcnt |= val;
			mem_write16(0x04000000, dcnt);
			return;
		}
		if( addr == 0x04000005 )
		{
			u16 dispstat = mem_read16(0x4000004);
			dispstat &= 0xff;
			dispstat |= (val<<8);
			mem_write16(0x4000004, dispstat);
			return;
		}
		if( addr == 0x04000048 )
		{
			gfxregs[0x48>>1] &= 0xff00;
			gfxregs[0x48>>1] |= val;
			return;
		}
		if( addr == 0x04000049 )
		{
			gfxregs[0x48>>1] &= 0xff;
			gfxregs[0x48>>1] |= val<<8;
			return;
		}
		
		if( 0 )//addr < 0x4000050 )
		{
			u32 t = addr&~1;
			u16 C = gfx_read16(t);
			if( addr & 1 )
				C = (C&0xff) | (val<<8);
			else
				C = (C&0xff00) | val;
			gfx_write16(t, C);
			return;
		}
		if( 0 )//addr >= 0x4000100 && addr <= 0x400010F )
		{
			u32 t = addr&~1;
			u16 C = timers_read16(t);
			if( addr & 1 )
				C = (C&0xff) | (val<<8);
			else
				C = (C&0xff00) | val;
			timers_write16(t, val);
			return;
		}
		if( addr > 0x40000A8 && addr != 0x04000120 && addr != 0x04000128 
			&& addr != 0x04000140 )
		{
			printf("IOWrite8 $%X to $%X\n", val, addr);
			//exit(1);
		}
		return;
	}
	if( addr < 0x06000000ul )
	{ // palette
		addr &= 0x3fe;
		u16 V = (val<<8)|val;
		memcpy(palette+addr, &V, 2);
		return;	
	}
	if( addr < 0x07000000ul )
	{ // VRAM
		//TODO: complex behavior depends on Gfx mode
		u16 p = val;
		p = (p<<8)|val;
		addr &= 0x1FFFe;
		if( addr & 0x10000 ) addr &= ~0x8000;
		memcpy(VRAM+addr, &p, 2);
		return;
	}
	if( addr < 0x08000000ul )
	{ // OAM
		return;
	}
	if( addr < 0x10000000ul )
	{  // either ROM or various types of SaveRAM that isn't supported yet
		if( addr >= 0x0e000000 && addr <= 0x0e00FFFF )
		{
			if( (addr == 0xE005555 && val == 0xAA) || (addr == 0xE002AAA && val == 0x55) )
			{
				flash_state++;
				return;
			}
			if( addr == 0xE005555 && flash_state == 2 )
			{
				flash_state = 0;
				flash_cmd = val;
				if( flash_cmd == 0x10 )
				{
					//printf("Flash cleared!\n");
					for(int i = 0; i < 0x20000; ++i) SRAM[i] = 0xff;
					return;
				}
				//printf("Flash cmd = %X\n", val);
				return;
			}
			if( flash_state == 2 && val == 0x30 && (addr&0xfff) == 0 )
			{
				//printf("Page clear!\n");
				flash_state = 0;
				for(int i = 0; i < 0x1000; ++i) SRAM[flash_bank + (addr&0xF000) + i] = 0xff;			
			}
			if( flash_cmd == 0xb0 && addr == 0x0E000000 )
			{
				flash_cmd = 0;
				flash_bank = (val&1)<<16;
				return;
			}
			if( flash_cmd == 0xa0 )
			{
				flash_cmd = 0;
				SRAM[flash_bank | (addr&0xffff)] = val;
				return;
			}
			return;
		}
		printf("SaveRAM Write @$%X = $%X\n", addr, val);
		exit(1);
		return;
	}

	return;
}

void mem_write16(u32 addr, u16 val)
{
	//if( addr & 1 ) printf("write16 %X = %X\n", addr, val);
	
	if( addr < 0x02000000ul )
	{ // BIOS
		return;	
	}
	if( addr < 0x03000000ul )
	{ // WRAM_lo
		memcpy(WRAM_lo+(addr&0x3FFFe), &val, 2);
		return;	
	}
	if( addr < 0x04000000ul )
	{ // WRAM_hi
		memcpy(WRAM_hi+(addr&0x7ffe), &val, 2);
		return;
	}
	if( addr < 0x05000000ul )
	{ // IO
		//TODO: any IO
		if( addr < 0x04000060 )
		{
			gfx_write16(addr, val);
			return;
		}
		//printf("IO Write16 %X to $%X, @$%X\n", val, addr, cpu.regs[15]);
		
		if( addr < 0x040000B0 )
		{  //TODO: sound
			if( addr == 0x04000088 )
			{
				io88 = val;		
			}
			return;
		}
		if( addr < 0x04000100 )
		{
			dma_write16(addr, val);
			return;
		}
		if( addr < 0x04000120 )
		{
			printf("Timer write @$%X = $%X\n", addr, val);
			//timers_write16(addr, val);
			return;
		}
		if( addr < 0x04000134 )
		{  //TODO: keypad interrupt
			return;
		}
		if( addr < 0x04000200 )
		{ // serial2
			return;
		}
		if( addr < 0x04000412 )
		{ // irq / sysctrl
			if( addr == 0x04000200 )
			{
				IE = val;
			} else if( addr == 0x04000204 ) {
				WAITCNT = val;
			} else if( addr == 0x04000202 ) {
				IF &= ~val;
			} else if( addr == 0x04000208 ) {
				IME = val&1;
			}
			return;
		}
		return;
	}
	if( addr < 0x06000000 )
	{ // palette
		memcpy(palette+(addr&0x3fe), &val, 2);
		return;
	}
	if( addr < 0x07000000 )
	{ // VRAM
		addr &= 0x1FFFE;
		if( addr & 0x10000 ) addr &= ~0x8000;
		memcpy(VRAM+addr, &val, 2);
		return;
	}
	if( addr < 0x08000000 )
	{ // OAM
		memcpy(OAM+(addr&0x3fe), &val, 2);
		return;
	}
	if( addr < 0x10000000 )
	{  // either ROM or various types of SaveRAM that isn't supported yet
		if( addr >= 0x080000c4 && addr <= 0x080000c8 )
		{
			if( addr == 0x080000c8 )
			{
				//rtc_active = (val&1);
				return;
			}
			//if( rtc_active ) rtc_write(addr, val);
			return;
		}
		return;
	}
	
	//printf("write16 %X = %X\n", addr, val);
	return;
}

void mem_write32(u32 addr, u32 val)
{
	//printf("write32 %X = %X\n", addr, val);
	
	if( addr < 0x02000000ul )
	{ // BIOS
		return;	
	}
	if( addr < 0x03000000ul )
	{ // WRAM_lo
		memcpy(WRAM_lo+(addr&0x3FFFc), &val, 4);
		return;	
	}
	if( addr < 0x04000000ul )
	{ // WRAM_hi
		memcpy(WRAM_hi+(addr&0x7ffc), &val, 4);
		return;
	}
	if( addr < 0x05000000ul )
	{ // IO
		//printf("IO Write32 %X to $%X\n", val, addr);
		mem_write16(addr, val);
		mem_write16(addr+2, val>>16);
		return;
	}
	if( addr < 0x06000000ul )
	{ // palette
		memcpy(palette+(addr&0x3fc), &val, 4);
		return;	
	}
	if( addr < 0x07000000ul )
	{ // VRAM
		addr &= 0x1FFFC;
		if( addr & 0x10000 ) addr &= ~0x8000;
		memcpy(VRAM+addr, &val, 4);
		return;
	}
	if( addr < 0x08000000ul )
	{ // OAM
		memcpy(OAM+(addr&0x3fc), &val, 4);
		return;
	}
	if( addr < 0x10000000ul )
	{  // either ROM or various types of SaveRAM that isn't supported yet
		return;
	}
	
	printf("write to bogus memory\n");
	arm7_dumpregs(cpu);
	exit(1);
	return;
}

u8 mem_read8(u32 addr)
{
	if( addr < 0x02000000ul )
	{ // BIOS
		if( cpu.regs[15] < 0x4000 ) return BIOS[addr&0x3fff];
		return cpu.prefetch;
	}
	if( addr < 0x03000000ul )
	{ // WRAM_lo
		return WRAM_lo[addr&0x3FFFF];	
	}
	if( addr < 0x04000000ul )
	{ // WRAM_hi
		return WRAM_hi[addr&0x7fff];
	}
	if( addr < 0x05000000ul )
	{ // IO
		if( addr < 0x04000050 )
		{
			u16 val = gfx_read16(addr);
			if( addr & 1 ) return val>>8;
			return val;
		}
		//if( addr == 0x04000130 ) return KEYS;
		//printf("IO Read8 @%X\n", addr);
		//TODO: 8bit reads of most other IO addrs
		return 0;
	}
	if( addr < 0x06000000ul )
	{ // palette
		return palette[addr&0x3ff];
	}
	if( addr < 0x07000000ul )
	{ // VRAM
		if( addr & 0x10000 ) addr &= ~0x8000;
		return VRAM[addr&0x1ffff];
	}
	if( addr < 0x08000000ul )
	{ // OAM
		return OAM[addr&0x3ff];
	}
	if( addr < 0x10000000ul )
	{  // either ROM or various types of SaveRAM that isn't supported yet
		//if( addr >= 0x0d000000 && addr  < 0x0e000000 ) return 1;
		if( addr >= 0x0e000000 && addr <= 0x0e00FFFF )
		{
			if( flash_cmd == 0x90 && addr == 0x0E000000 ) return 0x62;
			if( flash_cmd == 0x90 && addr == 0x0E000001 ) return 0x13;
			return SRAM[flash_bank | (addr&0xffff)];
		}
		addr &= 0x01ffFFFf;
		if( addr > romsize )
		{
			addr %= romsize;
		}
		//printf("ROM Read8 %X\n", addr);
		return ROM[addr];
	}
	
	printf("read8 %X\n", addr);
	return cpu.prefetch;
}

u16 mem_read16(u32 addr)
{
	//printf("READ16 @%X\n", addr);
	u16 temp = 0;
	if( addr < 0x02000000ul )
	{ // BIOS
		if( cpu.regs[15] < 0x4004 )
		{
			memcpy(&temp, BIOS+(addr&0x3ffe), 2);
		} else {
			temp = 0xE3A0;
		}
	} else if( addr < 0x03000000ul ) { // WRAM_lo
		memcpy(&temp, WRAM_lo+(addr&0x3fffe), 2);
	} else if( addr < 0x04000000ul ) { // WRAM_hi
		memcpy(&temp, WRAM_hi+(addr&0x7ffe), 2);
	} else if( addr < 0x05000000ul ) { // IO
		if( addr < 0x04000060 )
		{
			return gfx_read16(addr);
		}
		if( addr < 0x040000B0 )
		{  //TODO: sound
			if( addr == 0x04000088 )
			{
				return io88;
			}
			return 0;
		}
		if( addr < 0x04000100 )
		{
			return dma_read16(addr);
		}
		if( addr < 0x04000120 )
		{  //TODO: timers
			return 0xffff; //timers_read16(addr);
		}
		if( addr < 0x04000134 )
		{  //TODO: keypad interrupt
			if( addr == 0x04000130 ) 
			{
				return KEYS;
			} 
			return 0;
		}
		if( addr < 0x04000200 )
		{ // serial2
			return 0;
		}
		if( addr < 0x04000412 )
		{ // irq / sysctrl
			if( addr == 0x04000200 )
			{
				return IE;
			} else if( addr == 0x04000204 ) {
				return WAITCNT;
			} else if( addr == 0x04000202 ) {
				return IF;
			} else if( addr == 0x04000208 ) {
				return IME;
			}
			return 0;
		}
		return 0;
	} else if( addr < 0x06000000u ) { // palette
		memcpy(&temp, palette+(addr&0x3fe), 2);
	} else if( addr < 0x07000000u ) { // VRAM
		if( addr >= 0x6C00000 ) 
		{
			//arm7_dumpregs(cpu);
			//exit(1);
		}
		addr &= 0x1FFFe;
		if( addr & 0x10000 ) addr &= ~0x8000;
		memcpy(&temp, VRAM+addr, 2);
	} else if( addr < 0x08000000u ) { // OAM
		memcpy(&temp, OAM+(addr&0x3ff), 2);
	} else if( addr < 0x09000000u ) {  // either ROM or various types of SaveRAM that isn't supported yet
		if( rtc_active && addr >= 0x080000c4 && addr <= 0x080000c8 )
		{
			return rtc_read(addr);
		}
		addr &= 0x01ffFFFe;
		if( addr >= romsize )
		{
			addr %= romsize;
			//printf("Error: Read16 (@$%X) from beyond the ROM:\n", 0x08000000 + addr);
			//arm7_dumpregs(cpu);
			//exit(1);
		}
		memcpy(&temp, ROM+(addr), 2);
	} else if( addr < 0x10000000 ) {
		if( addr >= 0x0d000000 && addr  < 0x0e000000 ) return 1;
		addr &= 0x01ffFFFe;
		if( addr >= romsize )
		{
			addr %= romsize;
			//printf("Error: Read16 (@$%X) from beyond the ROM:\n", 0x08000000 + addr);
			//arm7_dumpregs(cpu);
			//exit(1);
		}
		memcpy(&temp, ROM+(addr), 2);	
	} else {
		printf("read from bogus memory\n");
		printf("read16 %X\n", addr);
		arm7_dumpregs(cpu);
		//exit(1);
	}
	
	if( addr & 1 ) temp = (temp<<8)|(temp>>8);	
	return temp;
}

u32 mem_read32(u32 addr)
{
	u32 temp = 0;
	if( addr < 0x02000000 )
	{ // BIOS
		if( cpu.regs[15] < 0x4000 )
		{
			memcpy(&temp, BIOS+(addr&0x3ffc), 4);	
		} else {
			temp = cpu.prefetch;
		}
	} else if( addr < 0x03000000 ) { 
		// WRAM_lo
		memcpy(&temp, WRAM_lo+(addr&0x3fffc), 4);
	} else if( addr < 0x04000000 ) { 
		// WRAM_hi
		memcpy(&temp, WRAM_hi+(addr&0x7ffc), 4);
	} else if( addr < 0x05000000 ) { 
		// IO
		//printf("IO Read32 @$%X\n", addr);
		if( addr < 0x04000060 )
		{
			temp = gfx_read16(addr&~3);
			temp |= gfx_read16((addr&~3)+2)<<16;
		} else if( addr == 0x04000130 ) {
			temp = KEYS;
		} else {
			//printf("IO Read32 @$%X\n", addr);
			temp = mem_read16((addr&~3));
			temp |= mem_read16((addr&~3)+2)<<16;
		}
	} else if( addr < 0x06000000 ) { 
		// palette
		memcpy(&temp, palette+(addr&0x3fc), 4);	
	} else if( addr < 0x07000000 ) { 
		// VRAM
		addr &= 0x1fffc;
		if( addr & 0x10000 ) addr &= ~0x8000;
		memcpy(&temp, VRAM+addr, 4);
	} else if( addr < 0x08000000 ) { 
		// OAM
		memcpy(&temp, OAM+(addr&0x3fc), 4);
	} else if( addr < 0x10000000 ) {  
		// either ROM or various types of SaveRAM that isn't supported yet
		if( (addr&0x1fffffc) >= romsize )
		{
			//printf("Error: Read32 (@$%X) from beyond the ROM:\n", 0x08000000 + addr);
			//arm7_dumpregs(cpu);
			//exit(1);
			addr %= romsize;
		}
		memcpy(&temp, ROM+(addr&0x1fffffc), 4);
	} else {
		printf("read32 %X: ", addr);
		printf("read bogus memory\n");
		arm7_dumpregs(cpu);
		exit(1);
	}
	
	if( addr & 3 ) temp = std::rotr(temp, (addr&3)*8);
	return temp;
}


u32 code_read32(u32 addr)
{
	int res = 0;
		
	if( addr < 0x4000 )
	{
		addr &= ~3;
		memcpy(&res, BIOS+addr, 4);
		return res;	
	}

	if( addr >= 0x08000000 )
	{
		addr &= 0x1fffFFc;
		memcpy(&res, ROM+addr, 4);
		return res;	
	}
	
	if( addr >= 0x02000000 && addr < 0x03000000 )
	{
		memcpy(&res, WRAM_lo+(addr&0x3FFFc), 4);
		return res;
	}
	
	if( addr >= 0x03000000 && addr < 0x04000000 )
	{
		memcpy(&res, WRAM_hi+(addr&0x7ffc), 4);
		return res;
	}
	
	if( addr >= 0x06000000 && addr < 0x07000000 )
	{
		addr &= 0x1ffffc;
		memcpy(&res, VRAM+addr, 4);
		return res;
	}

	printf("Running from non-code area $%X, LR=$%X\n", addr, getreg(cpu, 14));
	exit(1);
	return 0;
}

u16 code_read16(u32 addr)
{
	if( addr < 0x4000 )
	{
		addr &= ~1;
		return (BIOS[addr+1]<<8)|BIOS[addr];	
	}

	if( addr >= 0x08000000 )
	{
		addr &= 0x1fffFFe;
		return (ROM[addr+1]<<8)|ROM[addr];	
	}
	
	if( addr >= 0x02000000 && addr < 0x03000000 )
	{
		addr &= 0x3fffe;
		return (WRAM_lo[addr+1]<<8)|WRAM_lo[addr];
	}
	
	if( addr >= 0x03000000 && addr < 0x04000000 )
	{
		addr &= 0x7ffe;
		return (WRAM_hi[addr+1]<<8)|WRAM_hi[addr];
	}
	
	if( addr >= 0x06000000 && addr < 0x07000000 )
	{
		addr &= 0x1ffffe;
		return (VRAM[addr+1]<<8)|VRAM[addr];
	}

	printf("RunningT from noncode area $%X, LR=$%X\n", addr, getreg(cpu, 14));
	exit(1);
	return 0;
}





