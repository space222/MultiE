#include "apple2e.h"

void apple2e::key_down(int sc)
{
	if( sc == SDL_SCANCODE_F10 )
	{
		char* str = SDL_GetClipboardText();
		paste = str;
		SDL_free(str);
		if( paste.empty() ) return;
		key_last = paste[0];
		if( key_last == '\n' ) key_last = '\r';
		paste.erase(paste.begin());
		key_strobe = 0x80;
		return;
	}
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

void apple2e::drawtile(int px, int py, int c)
{
	c *= 8;
	
	for(int y = py; y < py+8; ++y)
	{
		u8 b = font[c+(y&7)];
		for(int x = 1; x < 8; ++x)
		{
			u32 bit = 7^x;
			fbuf[y*fb_width() + px + x] = (((b>>bit)&1)?0xffffff00 : 0);
		}
	}
}

void apple2e::run_frame()
{
	for(u32 i = 0; i < 17062; ++i)
	{
		cycle();
	}

	for(u32 y = 0; y < 8; ++y) 
	{
		for(u32 x = 0; x < 40; ++x) 
		{
			drawtile(x*7, y*8, ram[0x400+(y*128)+x]&0x7f);
		}
	}
	for(u32 y = 0; y < 8; ++y) 
	{
		for(u32 x = 0; x < 40; ++x) 
		{
			drawtile(x*7, (y+8)*8, ram[0x428+(y*128)+x]&0x7f);
		}
	}
	for(u32 y = 0; y < 8; ++y) 
	{
		for(u32 x = 0; x < 40; ++x) 
		{
			drawtile(x*7, (y+16)*8, ram[0x450+(y*128)+x]&0x7f);
		}
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
	
	fp = fopen("./bios/apple2font.bin", "rb");
	if(!fp)
	{
		printf("Unable to load apple2font.bin\n");
		return false;
	}
	unu = fread(font, 1, 8*128, fp);
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
		if( addr == 0xc010 ) 
		{ 
			if( paste.size() )
			{
				key_last = paste[0];
				if( key_last == '\n' ) key_last = '\r';
				paste.erase(paste.begin());
				key_strobe = 0x80;
				return 0;
			}
			key_strobe = 0; 
			return 0;
		}

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
		if( addr == 0xc010 ) 
		{
			if( paste.size() )
			{
				key_last = paste[0];
				if( key_last == '\n' ) key_last = '\r';
				paste.erase(paste.begin());
				key_strobe = 0x80;
				return;
			}
			key_strobe = 0; 
			return;
		}
	 	return; 
	}
}

void apple2e::reset()
{
	c6502.reader = [&](coru6502& c, u16 a) -> u8 { return this->read(c, a); };
	c6502.writer = [&](coru6502& c, u16 a, u8 v) { this->write(c, a, v); };
	c6502.cpu_type = CPU_65C02;
	cycle = c6502.run();
	
	for(u32 i = 0; i < 0x10000; ++i) ram[i] = 0xff;
	for(u32 i = 0; i < fb_width()*fb_height(); ++i) fbuf[i] = 0;
	key_last = ' ';
	key_strobe = 0;
}

