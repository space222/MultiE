#include <string>
#include <cstdio>
#include <cstdlib>
#include "macplus.h"
extern console* sys;
extern u8 byte2gcr[]; //at bottom


bool dolog = false;

void macplus::run_frame()
{
	via_pc &= ~1;
	for(u32 line=0; line<342+28; ++line)
	{
		u64 target = last_target + 352;
		while( stamp < target ) 
		{ 
			printf("pc = $%X\n", cpu.pc);
			cpu.step(); 
			stamp += cpu.icycles; 
		}
		last_target = target;
		if( line == 342 )
		{
			if( via_ie & 2 )
			{
				cpu.pending_irq = 1;
				via_if = 0x82;
				//printf("IRQ IF = $%X, IE = $%X\n", via_if, via_ie);
			}
			via_pc |= 1;
			if( (via_ie & 0x20) && !dolog ) { dolog= true; cpu.pending_irq = 1; via_if |= 0xa0; }
		}
	}
	
	for(u32 y = 0; y < 342; ++y)
	{
		for(u32 x = 0; x < 512; ++x)
		{
			u8 v = ram[((via_a&BIT(6))?0x3F2700:0x3FA700) + (y*(512/8)) + (x/8)];
			v >>= 7^(x&7);
			fbuf[512*y + x] = (v&1) ? 0 : 0xffFFff00;
		}
	}
}

void macplus::write(u32 addr, u32 v, int size)
{
	addr &= 0xffFFff;
	if( addr < 0x400000 )
	{
		if( size == 16 )
		{
			ram[addr] = v>>8;
			addr += 1;
		}
		if( addr == 0x168 || addr == 0x16a ) printf("ticks set to $%X\n", v);
		if( addr < 0x102 && u16(v) == 0x1B12 ) printf("VBLANK VECTOR: $%X = $%X\n", addr, v);
		ram[addr] = v;
		return;
	}
	
	if( addr == 0xEFE1FE ) { via_b = v; return; }
	if( addr == 0xEFFFFE ) { via_a = v; return; }
	if( addr == 0xEFFDFE ) { via_ie = v; printf("VIA_IE = $%X\n", v); return; }
	if( addr == 0xEFF9FE ) { via_pc = v; return; }
	
	printf("macplus: w%i $%X = $%X\n", size, addr, v);
}

u32 macplus::read(u32 addr, int size)
{
	addr &= 0xffFFff;
	if( cpu.pc >= 0x401a70 && cpu.pc <= 0x401a80 && addr < 0x400 ) printf("$%X: read%i <$%X\n", cpu.pc, size, addr);
	if( size != 16 )
	{
		printf("non16bit read!\n");
		exit(1);	
	}
	if( addr < 0x400000 ) return __builtin_bswap16(*(u16*)&ram[addr]);
	if( addr < 0x500000 ) return __builtin_bswap16(*(u16*)&rom[addr&0x1ffff]);

	//todo: go back to 8 and 16bit read/writes. 
	// I messed myself up trying to get clever in converting everything to 16bit
	// in the cpu.read/write handlers
	if( addr == 0xEFE1FE ) return (via_b|BIT(6));
	if( addr == 0xEFFFFE ) return via_a;
	if( addr == 0xEFFBFE ) { return via_if<<8; }
	if( addr == 0xEFFDFE ) { return via_ie<<8; }
	if( addr == 0xEFF9FE ) return via_pc<<8;
	if( addr == 0xDFFDFE ) return 0x1f;
	
	printf("MAC+:%06X: read%i <$%X\n", cpu.pc, size, addr);
	return 0xfffff;
	//exit(1);
}


void macplus::reset()
{
	memset(&cpu, 0, sizeof(cpu));
	cpu.read_code16 = cpu.mem_read16 = [](u32 a)->u16 { return dynamic_cast<macplus*>(sys)->read(a, 16); };
	cpu.mem_read8 = [](u32 a) -> u8 
	{ 
		return 0xff & (dynamic_cast<macplus*>(sys)->read(a&~1, 16)>>(((a&1)^1)*8));
	};
	cpu.mem_read32= [](u32 a) -> u32
		{ return (dynamic_cast<macplus*>(sys)->read(a, 16)<<16)|dynamic_cast<macplus*>(sys)->read(a+2,16); };
	cpu.intack = []{};
	cpu.mem_write8= [](u32 a, u8 v) { dynamic_cast<macplus*>(sys)->write(a, v, 8); };
	cpu.mem_write16= [](u32 a, u16 v) { dynamic_cast<macplus*>(sys)->write(a, v, 16); };
	cpu.mem_write32= [](u32 a, u32 v) 
		{ 
			dynamic_cast<macplus*>(sys)->write(a, v>>16, 16);
			dynamic_cast<macplus*>(sys)->write(a+2, v&0xffff, 16);
		};

	cpu.sr.raw = 0x2700;
	cpu.pc = cpu.mem_read32(0x400004);
	cpu.r[15] = cpu.r[16] = cpu.mem_read32(0x400000);
	
	via_a = via_b = via_if = 0;
	via_pc = 0;
	via_ie = 0x80;
	
	stamp = last_target = 0;
}

bool macplus::loadROM(const std::string)
{
	FILE* fp = fopen("./bios/macplus.rom", "rb");
	if(!fp)
	{
		printf("Unable to load macplus.rom\n");
		return false;
	}
	
	[[maybe_unused]] int unu = fread(rom, 1, 512*1024, fp);
	fclose(fp);
	return true;
}

u8 byte2gcr[64] =
{
0x96, 0x97, 0x9A, 0x9B, 0x9D, 0x9E, 0x9F, 0xA6, 0xA7, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB2, 0xB3,
0xB4, 0xB5, 0xB6, 0xB7, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xCB, 0xCD, 0xCE, 0xCF, 0xD3,
0xD6, 0xD7, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 0xE5, 0xE6, 0xE7, 0xE9, 0xEA, 0xEB, 0xEC,
0xED, 0xEE, 0xEF, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};


