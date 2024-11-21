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
	else if( sz == 16 ) { *(u16*)&data[addr] = v; }
	else { *(u32*)&data[addr] = v; }
}

static u16 dispstat = 0x40;

u32 virtualboy::read(u32 addr, int sz)
{
	addr &= 0x7FFFFFF;
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
	
	if( addr < 0x00040000 )
	{
		return sized_read(vram, addr, sz);
	}
	printf("vb: read%i <$%X\n", sz, addr);
	if( addr == 0x5F820 ) 
	{
		dispstat ^= 0x3c;
		return 0x40|dispstat;
	}
	if( addr == 0x02000010 ) return 2;
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
	
	if( addr < 0x00040000 )
	{
		sized_write(vram, addr, v, sz);
		return;
	}
	printf("vb: write%i $%X = $%X\n", sz, addr, v);
}

void virtualboy::run_frame()
{
	u32 cycles_in_frame = 0;
	while( cycles_in_frame < 500 ) //333333 )
	{
		if( !cpu.halted ) cycles_in_frame += cpu.step();
		else break;
		//if( cpu.halted ) { printf("HLT\n"); exit(1); }
	}
	
	for(u32 x = 0; x < 384; ++x)
	{
		for(u32 y = 0; y < 224; y += 4)
		{
			u8 b = vram[0 + x*(224/4) + (y/4)];
			fbuf[y*384 + x] = (b&3)<<30;
			fbuf[(y+1)*384 + x] = (b&0xc)<<28;
			fbuf[(y+2)*384 + x] = (b&0x30)<<26;
			fbuf[(y+3)*384 + x] = (b&0xc0)<<24;
		}
	}
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
}

