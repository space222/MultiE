#include <bit>
#include "virtualboy.h"

static u32 sized_read(u8* data, u32 addr, int sz)
{
	if( sz == 8 ) return data[addr];
	if( sz == 16 ) return *(u16*)&data[addr&~1];
	return *(u32*)&data[addr&~3];
}

static void sized_write(u8* data, u32 addr, u32 v, int sz)
{
	if( sz == 8 ) { data[addr] = v; }
	else if( sz == 16 ) { *(u16*)&data[addr&~1] = v; }
	else { *(u32*)&data[addr&~3] = v; }
}

void virtualboy::write_miscio(u32 addr, u32 v, int sz)
{
	printf("IO Write%i $%X = $%X\n", sz, addr, v);
}

u32 virtualboy::read_miscio(u32 addr, int sz)
{
	if( addr == 0x02000010 ) return 0xff;
	if( addr == 0x02000014 ) return 0xff;
	printf("IO Read%i <$%X\n", sz, addr);
	return 0;
}

static u16 dispstat = 0x4c;
static u16 rotit = 1;

u32 virtualboy::read(u32 addr, int sz)
{
	addr &= 0x07FFFFFF;
	if( addr >= 0x07000000 )
	{
		addr -= 0x07000000;
		if( addr >= ROM.size() ) addr %= ROM.size();
		return sized_read(ROM.data(), addr, sz);
	}
	if( addr >= 0x06000000 )
	{
		printf("vb: use of cart ram <$%X\n", addr);
		//exit(1);
		return 0;
	}
	printf("vb: read%i <$%X\n", sz, addr);
	if( addr >= 0x05000000 )
	{
		return sized_read(ram, addr&0xffff, sz);	
	}
	if( addr >= 0x04000000 )
	{
		printf("vb: use of cart exp hw <$%X\n", addr);
		exit(1);
		return 0;
	}
	if( addr >= 0x03000000 )
	{
		printf("vb: use of unmapped space <$%X\n", addr);
		exit(1);
		return 0;
	}
	if( addr >= 0x02000000 )
	{
		return read_miscio(addr, sz);	
	}
	if( addr >= 0x01000000 )
	{
		return 0; //todo: sound
	}
	
	// under this line is all VIP
	addr &= 0x7FFFF;
	if( addr < 0x00040000 ) return sized_read(vram, addr, sz);
	if( addr < 0x5F800 ) return 0; // unmapped space or unused mmio
	if( addr >= 0x7E000 ) return sized_read(&vram[(addr-0x7E000) + 0x1E000], 0, sz);
	if( addr >= 0x7C000 ) return sized_read(&vram[(addr-0x7C000) + 0x16000], 0, sz);
	if( addr >= 0x7A000 ) return sized_read(&vram[(addr-0x7A000) + 0x0E000], 0, sz);
	if( addr >= 0x78000 ) return sized_read(&vram[(addr-0x78000) + 0x06000], 0, sz);
	
	if( sz == 32 )
	{
		addr &= ~3;
		u32 res = read(addr, 16);
		res |= read(addr+2, 16)<<16;
		return res;
	}
	if( sz == 8 )
	{
		u16 v = read(addr&~1, 16);
		return v>>((addr&1)?8:0);	
	}
	
	if( addr == 0x5F800 ) return INTPND;
	if( addr == 0x5F802 ) return INTENB;
	if( addr == 0x5F820 ) 
	{
		dispstat ^= 0x80;
		return 0x40|dispstat;
	}
	if( addr == 0x5F840 ) return rotit = std::rotr(rotit, 1);
	return 0;
}

void virtualboy::write(u32 addr, u32 v, int sz)
{
	addr &= 0x7FFFFFF;
	if( addr >= 0x07000000 ) return;
	if( addr >= 0x06000000 )
	{
		printf("vb: use of cart ram $%X = $%X\n", addr, v);
		//exit(1);
		return;
	}
	printf("vb: write%i $%X = $%X\n", sz, addr, v);
	if( addr >= 0x05000000 )
	{
		sized_write(ram, addr&0xffff, v, sz);
		return;
	}
	if( addr >= 0x04000000 )
	{
		printf("vb: use of cart exp hw $%X = $%X\n", addr, v);
		exit(1);
		return;
	}
	if( addr >= 0x03000000 )
	{
		printf("vb: use of unmapped space $%X = $%X\n", addr, v);
		exit(1);
		return;
	}
	if( addr >= 0x02000000 )
	{
		write_miscio(addr, v, sz);
		return;
	}
	if( addr >= 0x01000000 )
	{
		return; //todo: snd
	}
	// under this line is all VIP
	addr &= 0x7FFFF;
	if( addr < 0x00040000 ) { sized_write(vram, addr, v, sz); return; }
	if( addr < 0x5F800 ) return; // unmapped space or unused mmio
	if( addr >= 0x7E000 ) { sized_write(&vram[(addr-0x7E000) + 0x1E000], 0, v, sz); return; }
	if( addr >= 0x7C000 ) { sized_write(&vram[(addr-0x7C000) + 0x16000], 0, v, sz); return; }
	if( addr >= 0x7A000 ) { sized_write(&vram[(addr-0x7A000) + 0x0E000], 0, v, sz); return; }
	if( addr >= 0x78000 ) { sized_write(&vram[(addr-0x78000) + 0x06000], 0, v, sz); return; }
	
	if( addr == 0x5F804 )
	{
		printf("INTCLR = $%X\n", v);
		INTPND &= ~v;
		return;
	}
	if( addr == 0x5F802 )
	{
		printf("INTENB = $%X\n", v);
		INTENB = v;
	}
	
}

void virtualboy::run_frame()
{
	dispstat ^= 0x3c;

	u32 cycles_in_frame = 0;
	while( cycles_in_frame < 50000 ) //333333 )
	{
		if( INTPND & INTENB ) cpu.irq(4, 0xFE40, 0xffffFE40);
	
		if( !cpu.halted ) cycles_in_frame += cpu.step();
		else break;
		//else { printf("HLT\n"); exit(1); }
	}
	
	for(u32 x = 0; x < 384; ++x)
	{
		for(u32 y = 0; y < 224; y += 4)
		{
			u8 L = vram[0 + ((dispstat&BIT(4))?0x8000:0) + x*(256/4) + (y/4)];
			u8 R = vram[0x10000 + ((dispstat&BIT(4))?0x8000:0) + x*(256/4) + (y/4)];
			fbuf[y*384 + x] = (L&3)<<30;
			fbuf[(y+1)*384 + x] = (L&0xc)<<28;
			fbuf[(y+2)*384 + x] = (L&0x30)<<26;
			fbuf[(y+3)*384 + x] = (L&0xc0)<<24;
			fbuf[y*384 + x] |= (R&3)<<14;
			fbuf[(y+1)*384 + x] |= (R&0xc)<<12;
			fbuf[(y+2)*384 + x] |= (R&0x30)<<10;
			fbuf[(y+3)*384 + x] |= (R&0xc0)<<8;
		}
	}
	
	INTPND |= BIT(14)|6;
}

bool virtualboy::loadROM(const std::string fname)
{
	FILE* fp = fopen(fname.c_str(), "rb");
	if( !fp ) return false;
	
	fseek(fp, 0, SEEK_END);
	auto fsz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	ROM.resize(fsz);
	[[maybe_unused]] int u = fread(ROM.data(), 1, fsz, fp);
	fclose(fp);
	return true;
}

void virtualboy::reset()
{
	cpu.read = [&](u32 addr, int sz) { return read(addr,sz); };
	cpu.write= [&](u32 addr, u32 v, int sz) { write(addr,v,sz); };
	cpu.reset();
	INTPND = INTENB = 0;
	for(u32 i = 0; i < 256*1024; ++i) vram[i] = 0;
}

