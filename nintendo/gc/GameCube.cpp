#include <algorithm>
#include <cstdio>
#include <print>
#include "Settings.h"
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
				//std::println("VI di{} irq: vpos = {}, dpv = {}", i, (u32)vi.di[i].b.vpos, vi.dpv);
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
			u8 Cr = mem1[vi.fbaddr + (y*320*4) + x*4 + 3];
			u8 Y2 = mem1[vi.fbaddr + (y*320*4) + x*4 + 2];
			u8 Cb = mem1[vi.fbaddr + (y*320*4) + x*4 + 1];		
			u8 Y1 = mem1[vi.fbaddr + (y*320*4) + x*4 + 0];		
			
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
	cpu.reset();
	debug_type = 0;
	vi.di[0].v = vi.di[1].v = vi.di[2].v = vi.di[3].v = 0;
	cpu.msr.b.EE = 0;
	
	/* //seeing what kind of stuff Dolphin writes to RAM in order to HLE the bootrom
	// Booted from bootrom. 0xE5207C22 = booted from jtag
  PowerPC::MMU::HostWrite_U32(guard, 0x0D15EA5E, 0x80000020);

  // Physical Memory Size (24MB on retail)
  PowerPC::MMU::HostWrite_U32(guard, memory.GetRamSizeReal(), 0x80000028);

  // Console type - DevKit  (retail ID == 0x00000003) see YAGCD 4.2.1.1.2
  // TODO: determine why some games fail when using a retail ID.
  // (Seem to take different EXI paths, see Ikaruga for example)
  const u32 console_type = static_cast<u32>(Core::ConsoleType::LatestDevkit);
  PowerPC::MMU::HostWrite_U32(guard, console_type, 0x8000002C);

  // Fake the VI Init of the IPL (YAGCD 4.2.1.4)
  PowerPC::MMU::HostWrite_U32(guard, DiscIO::IsNTSC(SConfig::GetInstance().m_region) ? 0 : 1,
                              0x800000CC);

  // ARAM Size. 16MB main + 4/16/32MB external. (retail consoles have no external ARAM)
  PowerPC::MMU::HostWrite_U32(guard, 0x01000000, 0x800000d0);

  PowerPC::MMU::HostWrite_U32(guard, 0x09a7ec80, 0x800000F8);  // Bus Clock Speed
  PowerPC::MMU::HostWrite_U32(guard, 0x1cf7c580, 0x800000FC);  // CPU Clock Speed

  PowerPC::MMU::HostWrite_U32(guard, 0x4c000064, 0x80000300);  // Write default DSI Handler:     rfi
  PowerPC::MMU::HostWrite_U32(guard, 0x4c000064, 0x80000800);  // Write default FPU Handler:     rfi
  PowerPC::MMU::HostWrite_U32(guard, 0x4c000064, 0x80000C00);
	*/
	
}

bool GameCube::loadROM(std::string fname)
{
	cpu.fetcher = [&](u32 a)->u32 { return fetch(a); };
	cpu.reader = [&](u32 a, int s)->u32 { return read(a, s); };
	cpu.writer = [&](u32 a, u32 v, int s) { write(a,v,s); };
	
	cpu.readf = [&](u32 a)->float { float v = 0; u32 b = read(a,32); memcpy(&v,&b,4); return v; };
	cpu.readd = [&](u32 a)->double { return read_double(a); };
	cpu.writef = [&](u32 a, float f) { u32 b = 0; memcpy(&b,&f,4); write(a,b,32); };
	cpu.writed = [&](u32 a, double d) { write_double(a, d); };

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
		return true;	
	}
	
	//std::vector<std::string> imap = Settings::get<std::vector<std::string>>("n64", "player1");
	//for(auto e : imap) std::println("{}", e);
	//setPlayerInputMap(1, imap);
	
	std::string bname = Settings::get<std::string>("gamecube", "bios");
	//std::println("{}", bname);
	
	FILE* fp = fopen(bname.c_str(), "rb");
	if(!fp)
	{
		std::println("Unable to open GC IPL '{}'", bname);
		return false;
	}
	ipl.resize(2097152);
	[[maybe_unused]] int unu = fread(ipl.data(), 1, ipl.size(), fp);
	fclose(fp);
	std::copy(ipl.begin(), ipl.end(), std::back_inserter(ipl_clear));
	descramble(ipl_clear.data()+0x100, ipl_clear.size()-0x100);
	cpu.pc = 0xfff00100u;
	return true;
}

u32 GameCube::fetch(u32 addr)
{
	//std::println("fetch ${:X}", addr);
	if( addr >= 0xfff00000u )
	{
		return __builtin_bswap32(*(u32*)&ipl_clear[addr&0xffff]);
	}
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
	if( ((addr >> 24)&0xf) == 0x0c )
	{
		addr |= 0xc0000000u;
		if( addr == 0xCC00302c ) return 0x246500B1; // hw revision reg
		if( addr == 0xcc00202c ) return vi.dpv;
		if( addr == 0xcc006404 )
		{
			auto keys = SDL_GetKeyboardState(nullptr);
			return (keys[SDL_SCANCODE_S] ? -1 : 0);
		}
		if( addr == 0xcc006408 ) return 0;//0x3fffFFFFu; // cont.1 btns
		
		if( addr >= 0xcc006800u && addr < 0xcc006900u )
		{
			//u32 t = exiregs[(addr>>2)&0x3F];
			//if( addr == 0xcc006834 || addr == 0xcc00680c ) exiregs[(addr&0x7f)>>2] &= ~1;
			//return t;
			return exi_read(addr, size);
		}
		
		
		if( addr >= 0xcc005000u && addr < 0xcc005100u )
		{
			return dsp_io_read(addr, size);
		}
		
		std::println("${:X}: read{} ${:X}", cpu.pc-4, size, addr);
				
		if( addr == 0xcc002030 ) return vi.di[0].v;
		if( addr == 0xcc002034 ) return vi.di[1].v;
		if( addr == 0xcc002038 ) return vi.di[2].v;
		if( addr == 0xcc00203c ) return vi.di[3].v;
		
		
		if( addr == 0xcc003000 )
		{
			std::println("PI INTSR reads ${:X}", pi.INTSR);
			return pi.INTSR;
		}
		
		if( addr == 0xcc003004 ) return pi.INTMR;
		
		
		
		return 0;
	}
	if( (addr >> 24) == 0xe0 )
	{
		std::println("Raccess to locked cache ${:X}", addr);
		exit(1);
		return 0;
	}
	if( (addr&0x1fffFFFFu) < 24*1024*1024 )
	{
		addr &= 0x1fffFFFFu;
		//if( addr >= 0x20 && addr <= 0x30 ) { std::println("read ${:X}", addr); exit(1); }
		if( size == 8 ) return mem1[addr];
		if( size == 16) return __builtin_bswap16(*(u16*)&mem1[addr]);
		return __builtin_bswap32(*(u32*)&mem1[addr]);
	}
	std::println("${:X}: read{} ${:X}", cpu.pc-4, size, addr);
	return 0;
}

void GameCube::write(u32 addr, u32 v, int size)
{
	if( ((addr >> 24)&0xf) == 0x0c )
	{
		addr |= 0xc0000000u;
		if( addr == 0xcc003004 ) { pi.INTMR = v; return; }
		if( addr >= 0xcc002030 && addr <= 0xcc00203c ) 
		{
			u32 direg = (addr>>2)&3;
			vi.di[direg].v &= BIT(31);
			vi.di[direg].v |= v&~BIT(31);
			if( v & BIT(31) ) vi.di[direg].v &= ~BIT(31);
			std::println("di{} = 0x{:X}", direg, v);
			if( !(vi.di[0].b.i || vi.di[1].b.i || vi.di[2].b.i || vi.di[3].b.i) )
			{
				clearINTSR(INTSR_VI_BIT);
			}
			return;
		}
		if( addr == 0xcc00201c ) { vi.fbaddr = v&0x1fffFFFFu; return; }
		if( addr == 0xcc00302c )
		{ // hw revision register, hopefully writing to it is simply ignored and I can use for debug output
			//std::println("debug out = ${:X}", v);
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
		if( addr >= 0xcc005000u && addr < 0xcc005100u )
		{
			dsp_io_write(addr, v, size);
			return;
		}
		if( addr >= 0xcc006800u && addr < 0xcc006900u )
		{
			exi_write(addr, v, size);			
			return;
		}
		std::println("${:X}: write{} ${:X} = ${:X}", cpu.pc-4, size, addr, v);
		return;
	}
	if( (addr >> 24) == 0xe0 )
	{
		std::println("Waccess to locked cache ${:X} = ${:X}", addr, v);
		exit(1);
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
	std::println("${:X}: write{} ${:X} = ${:X}", cpu.pc-4, size, addr, v);
	exit(1);
}

void GameCube::write_double(u32 addr, double d)
{
	if( (addr >> 24) == 0xcc )
	{
		std::println("${:X}: write double to ${:X}", cpu.pc-4, addr);
		return;
	}
	if( (addr&0x1fffFFFFu) < 24*1024*1024 )
	{
		u64 v = 0;
		memcpy(&v,&d,8);
		v = __builtin_bswap64(v);
		memcpy(mem1+(addr&0x1fffFFFFu), &v, 8);
		return;
	}
	std::println("${:X}: write double to ${:X}", cpu.pc-4, addr);
	return;
}

double GameCube::read_double(u32 addr)
{
	if( (addr >> 24) == 0xcc )
	{
		std::println("${:X}: read double from ${:X}", cpu.pc-4, addr);
		return 0;
	}
	if( (addr&0x1fffFFFFu) < 24*1024*1024 )
	{
		u64 u = 0;
		memcpy(&u, mem1+(addr&0x1fffFFFFu), 8);
		u = __builtin_bswap64(u);
		double dd = 0;
		memcpy(&dd, &u, 8);
		return dd;
	}
	std::println("${:X}: read double from ${:X}", cpu.pc-4, addr);
	return 42;
}


void GameCube::setINTSR(u32 b)
{
	pi.INTSR |= b;
	cpu.irq_line = ( pi.INTSR & pi.INTMR & 0x3fff );
}

void GameCube::clearINTSR(u32 b)
{
	pi.INTSR &= ~b;
	cpu.irq_line = ( pi.INTSR & pi.INTMR & 0x3fff );
}

// bootrom descrambler reversed by segher
// Copyright 2008 Segher Boessenkool <segher@kernel.crashing.org>
void GameCube::descramble(u8* data, u32 size)
{
	u8 acc = 0;
	u8 nacc = 0;

	u16 t = 0x2953;
	u16 u = 0xd9c2;
	u16 v = 0x3ff1;

	u8 x = 1;

	for (u32 it = 0; it < size;)
	{
		int t0 = t & 1;
		int t1 = (t >> 1) & 1;
		int u0 = u & 1;
		int u1 = (u >> 1) & 1;
		int v0 = v & 1;

		x ^= t1 ^ v0;
		x ^= (u0 | u1);
		x ^= (t0 ^ u1 ^ v0) & (t0 ^ u0);

		if (t0 == u0)
		{
			v >>= 1;
			if (v0)
				v ^= 0xb3d0;
		}

		if (t0 == 0)
		{
			u >>= 1;
			if (u0)
				u ^= 0xfb10;
		}

		t >>= 1;
		if (t0)
			t ^= 0xa740;

		nacc++;
		acc = 2*acc + x;
		if (nacc == 8)
		{
			data[it++] ^= acc;
			nacc = 0;
		}
	}
}


	

