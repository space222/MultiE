#include "apple2e.h"

void apple2e::key_down(int sc)
{
	if( sc == SDL_SCANCODE_LSHIFT || sc == SDL_SCANCODE_LCTRL ) return;
	if( key_strobe == 0 ) 
	{
		auto keys = SDL_GetKeyboardState(nullptr);
		bool shift = keys[SDL_SCANCODE_LSHIFT];
		bool ctrl = keys[SDL_SCANCODE_LCTRL];
		switch( sc )
		{
		case SDL_SCANCODE_RETURN: key_last = '\r'; break;
		case SDL_SCANCODE_SPACE: key_last = 32; break;
		case SDL_SCANCODE_APOSTROPHE: key_last = (shift ? '"' : '\''); break;
		
		
		case SDL_SCANCODE_0: key_last = (shift ? ')' : '0'); break;
		case SDL_SCANCODE_1: key_last = (shift ? '!' : '1'); break;
		case SDL_SCANCODE_2: key_last = (shift ? '@' : '2'); break;
		case SDL_SCANCODE_3: key_last = (shift ? '#' : '3'); break;
		case SDL_SCANCODE_4: key_last = (shift ? '$' : '4'); break;
		case SDL_SCANCODE_5: key_last = (shift ? '%' : '5'); break;
		case SDL_SCANCODE_6: key_last = (shift ? '^' : '6'); break;
		case SDL_SCANCODE_7: key_last = (shift ? '&' : '7'); break;
		case SDL_SCANCODE_8: key_last = (shift ? '*' : '8'); break;
		case SDL_SCANCODE_9: key_last = (shift ? '(' : '9'); break;
		
		default:
			key_last = SDL_GetKeyName(SDL_GetKeyFromScancode((SDL_Scancode)sc))[0];
			break;
		}
	}
	key_strobe = 0x80;
}

void apple2e::run_frame()
{
	for(u32 i = 0; i < 17062; ++i)
	{
		cycle();
	}
	printf("\033[0;0H");
	for(u32 y = 0; y < 24; ++y) 
	{
		for(u32 x = 0; x < 40; ++x)
		{
			u32 addr = 0;
			if( y < 8 ) addr = 0x400+(y*128);
			else if( y < 16 ) addr = 0x428+((y-8)*128);
			else addr = 0x450+((y-16)*128);
			putchar(ram[addr+x]&0x7f);
		}
		putchar('\n');
	}
}

bool apple2e::loadROM(const std::string)
{
	FILE* fp = fopen("./bios/a2p_monitor.bin", "rb");
	if(!fp)
	{
		printf("Unable to load monitor\n");
		return false;
	}
	
	[[maybe_unused]] int unu = fread(bios, 1, 2048, fp);
	fclose(fp);
	
	fp = fopen("./bios/applebasic.bin", "rb");
	if(!fp)
	{
		printf("Unable to load applebasic.bin\n");
		return false;
	}
	
	unu = fread(basic, 1, 10*1024, fp);
	fclose(fp);
	
	return true;
}
	
u8 apple2e::read(coru6502&, u16 addr)
{
	if( addr < 0xc000 ) return ram[addr];
	if( addr < 0xd000 ) 
	{ //todo: io 
		if( addr == 0xc000 ) return key_last|key_strobe;
		if( addr == 0xc030 ) return 0;
		printf("a2e:$%X: io <$%X\n", c6502.pc, addr);
		if( addr == 0xc010 ) { key_strobe = 0; return 0; }

	}
	if( addr < 0xf800 ) return basic[addr - 0xd000];
	//printf("a2e: <$%X\n", addr);
	return bios[addr&0x7ff];
}

void apple2e::write(coru6502&, u16 addr, u8 v)
{
	if( addr < 0xc000 ) { ram[addr] = v; return; }
	if( addr < 0xd000 ) 
	{ //todo: io
		printf("a2e: IO $%X = $%X\n", addr, v); 
		if( addr == 0xc010 ) { key_strobe = 0; exit(1); return; }
	 	return; 
	}
}

void apple2e::reset()
{
	c6502.reader = [&](coru6502& c, u16 a) -> u8 { return this->read(c, a); };
	c6502.writer = [&](coru6502& c, u16 a, u8 v) { this->write(c, a, v); };
	c6502.cpu_type = CPU_65C02;
	cycle = c6502.run();
	//cycle();
	//c6502.pc = 0xe000;
	for(u32 i = 0; i < 0x10000; ++i) ram[i] = 0xff;
	key_last = ' ';
	key_strobe = 0;
}

