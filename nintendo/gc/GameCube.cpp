#include <cstdio>
#include <print>
#include "GameCube.h"

void GameCube::run_frame()
{
	vi.dpv = 1;
	while( vi.dpv < 264 )
	{
		for(u32 i = 0; i < 4; ++i)
		{
			if( vi.di[i].b.e && vi.di[i].b.vpos == vi.dpv )
			{
				vi.di[i].b.i = 1;
				setINTSR(INTSR_VI_BIT);
				std::println("VI di{} irq: vpos = {}, dpv = {}", i, (u32)vi.di[i].b.vpos, vi.dpv);
			}
		}
	
		for(u32 i = 0; i < 31000; ++i)
		{
			cpu.step();
		}
		vi.dpv += 1;
	}
	u32* temp = fbuf;
	
	for(u32 y = 0; y < 480; ++y)
	{
		for(u32 x = 0; x < 320; ++x)
		{
			u8 Cr = mem1[0x104000 + (y*320*4) + x*4 + 3];
			u8 Y2 = mem1[0x104000 + (y*320*4) + x*4 + 2];
			u8 Cb = mem1[0x104000 + (y*320*4) + x*4 + 1];		
			u8 Y1 = mem1[0x104000 + (y*320*4) + x*4 + 0];		
			
			{
				s32 R = Y1 + (Cr - 128.) * 1.402;
				s32 G = Y1 + (Cb - 128.) * -0.34414 + (Cr - 128.) * -0.71414;
				s32 B = Y1 + (Cb - 128.) * 1.772;
				R = std::clamp(R, 0, 255);
				G = std::clamp(G, 0, 255);
				B = std::clamp(B, 0, 255);
				*temp = (R<<24)|(G<<16)|(B<<8);
				temp++;
			}
			{
				s32 R = Y2 + (Cr - 128.) * 1.402;
				s32 G = Y2 + (Cb - 128.) * -0.34414 + (Cr - 128.) * -0.71414;
				s32 B = Y2 + (Cb - 128.) * 1.772;
				R = std::clamp(R, 0, 255);
				G = std::clamp(G, 0, 255);
				B = std::clamp(B, 0, 255);
				*temp = (R<<24)|(G<<16)|(B<<8);
				temp++;
			}
		}	
	}
	//memcpy(fbuf, mem1 + 0x104000, 640*480*2);
}

void GameCube::reset()
{
	debug_type = 0;
}

bool GameCube::loadROM(std::string fname)
{
	if( fname.ends_with("dol") || fname.ends_with("DOL") )
	{
		FILE* fp = fopen(fname.c_str(), "rb");
		if( !fp ) return false;
		fseek(fp, 0, SEEK_END);
		auto fsz = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		
		std::vector<u8> file(fsz);
		[[maybe_unused]] int unu = fread(file.data(), 1, fsz, fp);
		fclose(fp);
		
		for(u32 i = 0; i < 7; ++i)
		{
			u32 offset = __builtin_bswap32(*(u32*)&file[i*4]);
			u32 addr = __builtin_bswap32(*(u32*)&file[0x48 + i*4]);
			u32 size = __builtin_bswap32(*(u32*)&file[0x90 + i*4]);
			if( size == 0 ) continue;
			
			addr &= 0x1fffFFFF;
			for(u32 n = 0; n < size; ++n)
			{
				mem1[addr+n] = file[offset+n];
			}
			std::println("text offset {:X}, addr {:X}, size {:X}", offset, addr, size);
		}
		
		for(u32 i = 0; i < 11; ++i)
		{
			u32 offset = __builtin_bswap32(*(u32*)&file[0x1C + i*4]);
			u32 addr = __builtin_bswap32(*(u32*)&file[0x48 + 7*4 + i*4]);
			u32 size = __builtin_bswap32(*(u32*)&file[0x90 + 7*4 + i*4]);
			if( size == 0 ) continue;
			addr &= 0x1fffFFFF;
			for(u32 n = 0; n < size; ++n)
			{
				mem1[addr+n] = file[offset+n];
			}
			std::println("data offset {:X}, addr {:X}, size {:X}", offset, addr, size);
		}
		
		u32 bssaddr = __builtin_bswap32(*(u32*)&file[0xD8]);
		u32 bsssize = __builtin_bswap32(*(u32*)&file[0xDC]);
		if( bssaddr && bsssize )
		{
			bssaddr &= 0x1fffFFFFu;
			for(u32 i = 0; i < bsssize; ++i) mem1[bssaddr+i] = 0;
		}
		
		cpu.pc = __builtin_bswap32(*(u32*)&file[0xE0]);
		std::println("entry point = ${:X}", cpu.pc);
		
		cpu.fetcher = [&](u32 a)->u32 { return fetch(a); };
		cpu.reader = [&](u32 a, int s)->u32 { return read(a, s); };
		cpu.writer = [&](u32 a, u32 v, int s) { write(a,v,s); };
			
		return true;	
	}
	return false;
}

u32 GameCube::fetch(u32 addr)
{
	//std::println("fetch ${:X}", addr);
	addr &= 0x1fffFFFF;
	if( addr < 24*1024*1024 )
	{
		return __builtin_bswap32(*(u32*)&mem1[addr]);
	}
	
	std::println("Unable to fetch from ${:X}", addr);
	return 0;
}

u32 GameCube::read(u32 addr, int size)
{
	if( (addr >> 24) == 0xcc )
	{
		if( addr == 0xcc00202c ) return vi.dpv;
		if( addr == 0xcc002030 ) return vi.di[0].v;
		if( addr == 0xcc002034 ) return vi.di[1].v;
		if( addr == 0xcc002038 ) return vi.di[2].v;
		if( addr == 0xcc00203c ) return vi.di[3].v;
		
		if( addr == 0xcc006404 || addr == 0xcc006408 ) return 0x3fffFFFFu; // cont.1 btns
		
		if( addr == 0xcc003000 )
		{
			std::println("PI INTSR reads ${:X}", pi.INTSR);
			return pi.INTSR;
		}
		
		if( addr == 0xcc003004 ) return pi.INTMR;
		
		std::println("${:X}: read{} ${:X}", cpu.pc-4, size, addr);
		return 0;
	}
	if( (addr&0x1fffFFFFu) < 24*1024*1024 )
	{
		addr &= 0x1fffFFFFu;
		if( size == 8 ) return mem1[addr];
		if( size == 16) return __builtin_bswap16(*(u16*)&mem1[addr]);
		return __builtin_bswap32(*(u32*)&mem1[addr]);
	}
	std::println("${:X}: read{} ${:X}", cpu.pc-4, size, addr);
	return 0xcafe;
}

void GameCube::write(u32 addr, u32 v, int size)
{
	if( (addr >> 24) == 0xcc )
	{
		if( addr == 0xcc003004 ) { pi.INTMR = v; std::println("INTMR = ${:X}", v); return; }
		if( addr == 0xcc002030 ) { vi.di[0].v = v; std::println("di0.vpos = {}", (int)vi.di[0].b.vpos); return; }
		if( addr == 0xcc002034 ) { vi.di[1].v = v; std::println("di1.vpos = {}", (int)vi.di[1].b.vpos);return; }
		if( addr == 0xcc002038 ) { vi.di[2].v = v; std::println("di2.vpos = {}", (int)vi.di[2].b.vpos);return; }
		if( addr == 0xcc00203c ) { vi.di[3].v = v; std::println("di3.vpos = {}", (int)vi.di[3].b.vpos);return; }
		
		if( addr == 0xcc00302c )
		{ // hw revision register, hopefully writing to it is simply ignored and I can use for debug output
			if( debug_type == 0 )
			{
				if( v & 0x80000000u )
				{
					std::putchar(v&0xff);
					return;
				}
				debug_type = v >> 24;
				return;
			}
			if( debug_type == 1 )
			{
				std::printf("%i", (int)v);
			} else if( debug_type == 2 ) {
				std::printf("%u", (unsigned int)v);
			} else if( debug_type == 3 ) {
				float a = 0;
				memcpy(&a, &v, 4);
				std::printf("%f", a);
			}
			debug_type = 0;
			return;
		}
		return;
	}
	if( (addr&0x1fffFFFFu) < 24*1024*1024 )
	{
		addr &= 0x1fffFFFFu;
		if( size == 8 ) mem1[addr] = v;
		else if( size == 16 ) *(u16*)&mem1[addr] = __builtin_bswap16(u16(v));
		else *(u32*)&mem1[addr] = __builtin_bswap32(v);
		return;
	}
	std::println("write{} ${:X} = ${:X}", size, addr, v);
}

void GameCube::setINTSR(u32 b)
{
	pi.INTSR |= b;
	cpu.irq_line = ( pi.INTSR & pi.INTMR & 0x3fff );
}

