#include "apple2e.h"

static u32 a2pal[16] = {
	0, 0x8a2140, 0x3c22a5, 0xc847e4, 0x07653e,
	0x7b7e80, 0x308fe3, 0xb9a9fd, 0x3b5107, 0xc77028,
	0x7b7e80, 0xf39ac2, 0x2fb81f, 0xb9d060, 0x6ee1c0,
	0xffffff
};

void apple2e::key_down(int sc)
{
	if( sc == SDL_SCANCODE_F10 )
	{
		char* str = SDL_GetClipboardText();
		paste = str;
		SDL_free(str);
		if( paste.empty() ) return;
		if( paste.back() == '\n' ) 
		{
			paste.back() = '\r';
		} else if( paste.back() != '\r' ) {
			paste.push_back('\r');
		}
		key_last = paste[0];
		if( key_last == '\n' ) key_last = '\r';
		paste.erase(paste.begin());
		key_strobe = 0x80;
		return;
	}
	if( sc == SDL_SCANCODE_LSHIFT || sc == SDL_SCANCODE_LCTRL || 
		sc == SDL_SCANCODE_RSHIFT || sc == SDL_SCANCODE_RCTRL ) return;
	if( key_strobe == 0 ) 
	{
		auto keys = SDL_GetKeyboardState(nullptr);
		bool shift = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
		//bool ctrl = keys[SDL_SCANCODE_LCTRL];
		switch( sc )
		{
		case SDL_SCANCODE_UP: key_last = 11; break;
		case SDL_SCANCODE_DOWN: key_last = 10; break;
		case SDL_SCANCODE_LEFT: key_last = 8; break;
		case SDL_SCANCODE_RIGHT: key_last = 0x15; break;
		case SDL_SCANCODE_ESCAPE: key_last = 0x1B; break;
		
		case SDL_SCANCODE_RETURN: key_last = '\r'; break;
		case SDL_SCANCODE_SPACE: key_last = 32; break;
		case SDL_SCANCODE_APOSTROPHE: key_last = (shift ? '"' : '\''); break;
		case SDL_SCANCODE_SLASH: key_last = (shift ? '?' : '/'); break;
		case SDL_SCANCODE_COMMA: key_last = (shift ? '<' : ','); break;
		case SDL_SCANCODE_PERIOD: key_last = (shift ? '>' : '.'); break;
		case SDL_SCANCODE_EQUALS: key_last = (shift ? '+' : '='); break;
		case SDL_SCANCODE_MINUS: key_last = (shift ? '_' : '-'); break;
		
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
	bool inverse = !(c&0x80);
	c &= 0x7f;
	c *= 8;
	
	for(int y = py; y < py+8; ++y)
	{
		u8 b = font[c+(y&7)];
		for(int x = 0; x < 7; ++x)
		{
			u32 bit = 7^(x+1);
			fbuf[y*fb_width() + px + x] = ((((b>>bit)&1)?0xffffff00 : 0)) ^ (inverse ? 0xffFFff00:0);
		}
	}
}

u32 hires_px(u8 hb, u8 pair)
{
	pair &= 3;
	if( pair == 0 ) return 0;
	if( pair == 3 ) return 0xffFFff00;
	if( hb )
	{
		if( pair == 1 ) return 0x0ab1ff00;
		return 0xff4e0600;	
	}
	if( pair == 1 ) return 0xff00ff00;
	return 0x05ff0300;
}

static u32 segoffs[24] = {
	0x2000, 0x2080, 0x2100, 0x2180, 0x2200, 0x2280, 0x2300,
	0x2380, 0x2028, 0x20A8, 0x2128, 0x21A8, 0x2228, 0x22A8,
	0x2328, 0x23A8, 0x2050, 0x20D0, 0x2150, 0x21D0, 0x2250,
	0x22D0, 0x2350, 0x23D0
};

void apple2e::run_frame()
{
	for(u32 i = 0; i < 17062; ++i)
	{
		//if( c6502.pc == 0x801 ) { printf("running bootsector\n"); exit(1); }
		cycle();
		floppy_clock();
		sample_cycles += 1;
		if( sample_cycles >= 23 )
		{
			sample_cycles = 0;
			float out = ((snd_toggle&1) ? 0.9f : 0);
			audio_add(out, out);
		}
	}

	if( text_mode )
	{
		for(u32 y = 0; y < 8; ++y) 
		{
			for(u32 x = 0; x < 40; ++x) 
			{
				drawtile(x*7, y*8, ram[0x400+(y*128)+x]);
			}
		}
		for(u32 y = 0; y < 8; ++y) 
		{
			for(u32 x = 0; x < 40; ++x) 
			{
				drawtile(x*7, (y+8)*8, ram[0x428+(y*128)+x]);
			}
		}
		for(u32 y = 0; y < 8; ++y) 
		{
			for(u32 x = 0; x < 40; ++x) 
			{
				drawtile(x*7, (y+16)*8, ram[0x450+(y*128)+x]);
			}
		}
		return;
	}
	
	if( !hires )
	{
		for(u32 y = 0; y < 32; ++y)
		{
			for(u32 x = 0; x < 40; ++x)
			{
				u8 b = ram[0x400+((y>>1)*128)+x] >> ((y&1)?4:0);
				u32 px = a2pal[b&15]<<8;
				for(u32 i = 0; i < 7; ++i)
				{
					fbuf[(y*4+0)*fb_width() + (x*7) + i] = 
					fbuf[(y*4+1)*fb_width() + (x*7) + i] =
					fbuf[(y*4+2)*fb_width() + (x*7) + i] = 
					fbuf[(y*4+3)*fb_width() + (x*7) + i] = px;
				}
			}
		}
		for(u32 y = 0; y < 32; ++y)
		{
			for(u32 x = 0; x < 40; ++x)
			{
				u8 b = ram[0x428+((y>>1)*128)+x] >> ((y&1)?4:0);
				u32 px = a2pal[b&15]<<8;
				for(u32 i = 0; i < 7; ++i)
				{
					fbuf[64*fb_width() + (y*4+0)*fb_width() + (x*7) + i] = 
					fbuf[64*fb_width() + (y*4+1)*fb_width() + (x*7) + i] =
					fbuf[64*fb_width() + (y*4+2)*fb_width() + (x*7) + i] = 
					fbuf[64*fb_width() + (y*4+3)*fb_width() + (x*7) + i] = px;
				}
			}
		}
		for(u32 y = 0; y < 32; ++y)
		{
			for(u32 x = 0; x < 40; ++x)
			{
				u8 b = ram[0x450+((y>>1)*128)+x] >> ((y&1)?4:0);
				u32 px = a2pal[b&15]<<8;
				for(u32 i = 0; i < 7; ++i)
				{
					fbuf[128*fb_width() + (y*4+0)*fb_width() + (x*7) + i] = 
					fbuf[128*fb_width() + (y*4+1)*fb_width() + (x*7) + i] =
					fbuf[128*fb_width() + (y*4+2)*fb_width() + (x*7) + i] = 
					fbuf[128*fb_width() + (y*4+3)*fb_width() + (x*7) + i] = px;
				}
			}
		}			
		if( mixed_mode )
		{
			for(u32 y = 4; y < 8; ++y) 
			{
				for(u32 x = 0; x < 40; ++x) 
				{
					drawtile(x*7, (y+16)*8, ram[0x450+(y*128)+x]&0x7f);
				}
			}		
		}
		return;
	}
	
	if( hires )
	{      
		// monochrone for now		
		const u32 segs = (24 - (mixed_mode?4:0));
		u32 page = (page2 ? 0x2000 : 0);
		for(u32 s = 0; s < segs; ++s)
		{
			for(u32 y = 0; y < 8; ++y)
			{
				const u32 lineoffs = (s*8+y)*fb_width();
				for(u32 x = 0; x < 40; ++x)
				{
					u8 b = ram[page + segoffs[s] + y*1024 + x];
					for(u32 i = 0; i < 7; ++i) fbuf[lineoffs + x*7 + i] = ( ((b>>i)&1)?0xffFFff00:0 );
				}
			}
		}	
	
		if( mixed_mode )
		{
			for(u32 y = 4; y < 8; ++y) 
			{
				for(u32 x = 0; x < 40; ++x) 
				{
					drawtile(x*7, (y+16)*8, ram[0x450+(y*128)+x]&0x7f);
				}
			}		
		}
		return;
	}	
}

bool apple2e::loadROM(const std::string fname)
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
	
	fp = fopen("./bios/a2disk.rom", "rb");
	if(!fp)
	{
		printf("Unable to load a2disk.rom\n");
		return false;
	}
	unu = fread(disk, 1, 256, fp);
	fclose(fp);
	
	drive[0].floppy = std::unique_ptr<wozfile>(wozfile::loadWOZ(fname));
	if( !drive[0].floppy )
	{
		printf("Unable to load WOZ '%s'\n", fname.c_str());
		return 1;
	}
	
	return true;
}

void apple2e::floppy_clock()
{
	if( drive[0].motor_cycles )
	{
		drive[0].motor_cycles -= 1;
		if( !drive[0].motor_cycles )
		{
			drive[0].motorOn = !drive[0].motorOn;
			if( drive[0].motorOn == false ) { printf("floppy motor off!\n"); exit(1); }
		}		
	}
	if( drive[1].motor_cycles )
	{
		drive[1].motor_cycles -= 1;
		if( !drive[1].motor_cycles ) drive[1].motorOn = !drive[1].motorOn;
	}

	if( ! drive[curdrive].motorOn ) return;
	if( ! drive[curdrive].floppy ) return;
	
	floppy_cycles -= 1;
	if( floppy_cycles == 0 )
	{
		iwm_shifter <<= 1;
		iwm_shifter |= drive[curdrive].floppy->getBit(drive[curdrive].track, drive[curdrive].bit);
		drive[curdrive].bit += 1;		
		if( iwm_shifter & 0x80 )
		{
			iwm_data_rd = iwm_shifter;
			iwm_shifter = 0;
			floppy_cycles = 8;
		} else {
			floppy_cycles = 4;
		}
	}

	return;
}

u8 apple2e::floppy_access(u16 addr, u8 v, bool read)
{
	u8 oldswitches = iwm_switches;	
	iwm_switches &= ~BIT( (addr>>1)&7 );
	if( addr & 1 ) iwm_switches |= BIT( (addr>>1)&7 );
	curdrive = 0;// ((iwm_switches&BIT(5))?1:0);
	
	if( (oldswitches & BIT(4)) != (iwm_switches&BIT(4)) )
	{
		//drive[curdrive].motor_cycles = 300000; //todo: calc how many cycles for motor state change
		drive[curdrive].motorOn = (iwm_switches&BIT(4));
	}
	
	if( (iwm_switches&0xd0) == 0xd0 && !read )
	{
		printf("floppy: mode set to $%X\n", v);
	}

	if( !drive[curdrive].floppy ) return 0;
	
	u32 newphase = std::countr_zero(u8((iwm_switches&~(1<<drive[curdrive].phase))|16));
	if( newphase != 4 )
	{
		if( addr != 0xc0ec ) printf("floppy access $%X\n", addr);
		
		if( newphase == ((drive[curdrive].phase-1)&3) )
		{
			if( drive[curdrive].track ) { drive[curdrive].track -= 2; drive[curdrive].bit = 0; }
			printf("$%X: floppy track = %i\n", c6502.pc, drive[curdrive].track);
			drive[curdrive].phase = newphase;
			//todo: make sure current bit is in semi-right place on new track
		} else if( newphase == ((drive[curdrive].phase+1)&3) ) {
			if( drive[curdrive].track < 159 ) { drive[curdrive].track += 2;  drive[curdrive].bit = 0; }
			printf("$%X: floppy track = %i\n", c6502.pc, drive[curdrive].track);
			drive[curdrive].phase = newphase;
		}	
	}
	
	if( addr == 0xc0ec )
	{
		return std::exchange(iwm_data_rd, 0);
	}
	return 0;
}

u8 apple2e::langcard(u16 addr, bool)
{
	bank2 = ( (addr&BIT(3)) ? 0xd000 : 0xc000 );
	lc_read_ff = ((addr&3)==3) || ((addr&3)==0);

	if( addr & 1 )
	{
		if( lc_prewrite ) lc_write_ff = false;
	} else {
		lc_write_ff = true;
	}
	
	lc_prewrite = addr&2; // address line 1 ??

	return 0;
}

u8 apple2e::io_access(u16 addr, u8 v, bool read)
{
	u16 upper = addr & ~1;
	
	if( addr >= 0xc0e0 && addr < 0xc0f0 ) return floppy_access(addr, v, read);	
	if( addr >= 0xc080 && addr < 0xc090 ) return langcard(addr, read);
	
	if( upper == 0xC050 )
	{
		text_mode = addr&1;
		return 0;
	}
	if( upper == 0xC052 )
	{
		mixed_mode = addr&1;
		return 0;
	}
	if( upper == 0xC054 )
	{
		page2 = addr&1;
		return 0;
	}
	if( upper == 0xC056 )
	{
		hires = addr&1;
		return 0;
	}
	if( addr == 0xc030 ) 
	{ 
		snd_toggle ^= 1; 
		return 0; 
	}
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
	if( addr == 0xc000 && read ) 
	{
		return key_last|key_strobe;
	}
	if( upper == 0xc000 )
	{
		store80 = addr&1;
		return 0;
	}
	if( upper == 0xc002 )
	{
		ramrd = addr&1;
		return 0;
	}
	if( upper == 0xc004 )
	{
		ramwrt = addr&1;
		return 0;
	}
	if( upper == 0xc008 )
	{
		altzp = addr&1;
		return 0;
	}
	if( addr >= 0xc011 && addr <= 0xc01f ) { printf("a2e: status io <$%X\n", addr); exit(1); }
	if( addr < 0xc100 ) printf("a2e: io access $%X\n", addr);
	return 0;
}
	
u8 apple2e::read(coru6502&, u16 addr)
{
	//printf("a2e: <$%X\n", addr);
	if( addr < 0x200 ) return altzp ? aux[addr] : ram[addr];
	if( addr < 0x400 ) return ramrd ? aux[addr] : ram[addr];
	if( addr < 0x800 ) return ((store80 && page2) || (!store80 && ramrd)) ? aux[addr] : ram[addr];
	if( addr < 0x2000) return ramrd ? aux[addr] : ram[addr];
	if( addr < 0x4000) return ((store80&& page2 &&hires)||(!store80&&ramrd))? aux[addr]:ram[addr];
	if( addr < 0xc000) return ramrd ? aux[addr] : ram[addr];
	
	if( addr >= 0xc600 && addr <= 0xc6ff ) return disk[addr&0xff];
	if( addr < 0xd000 ) 
	{				
		//printf("a2e:$%X: io <$%X\n", c6502.pc, addr);
		return io_access(addr, 0, true);
	}
	
	if( lc_read_ff )
	{
		if( addr < 0xe000 ) 
			return altzp ? aux[bank2 + (addr&0xfff)] : ram[bank2 + (addr&0xfff)];
		return altzp ? aux[addr] : ram[addr];	
	}
	
	if( addr < 0xf800 ) return basic[addr - 0xd000];
	//printf("a2e: <$%X\n", addr);
	return bios[addr&0x7ff];
}

void apple2e::write(coru6502&, u16 addr, u8 v)
{
	//printf("a2e: w $%X = $%X\n", addr, v);
	if( addr < 0x200 ) { altzp ? (aux[addr]=v) : (ram[addr]=v); return; }
	if( addr < 0x400 ) { ramwrt? (aux[addr]=v) : (ram[addr]=v); return; }
	if( addr < 0x800 ) 
	{
		((store80 && page2) || (!store80 && ramwrt)) ? (aux[addr]=v) : (ram[addr]=v); return;
	}
	if( addr < 0x2000) { ramwrt ? (aux[addr]=v) : (ram[addr]=v); return; }
	if( addr < 0x4000) { ((store80&& page2 &&hires)||(!store80&&ramwrt))? (aux[addr]=v):(ram[addr]=v); return; }
	if( addr < 0xc000) { ramwrt ? (aux[addr]=v) : (ram[addr]=v); return; }
	if( addr < 0xd000 ) 
	{
		io_access(addr, v, false);
		return; 
	}
	
	if( !lc_write_ff )
	{
		if( addr < 0xe000 )
		{
			if( altzp ) { aux[bank2 + (addr&0xfff)] = v; return; }
			ram[bank2 + (addr&0xfff)] = v;
			return;
		}
		if( altzp )
		{
			aux[addr] = v;
		} else {
			ram[addr] = v;
		}
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
	snd_toggle = 0;
	sample_cycles = 0;
	
	hires = false;
	text_mode = true;
	mixed_mode = true;
	ramrd = ramwrt = store80 = false;
	altzp = false;
	
	frame_cnt = 0;
	
	lc_read_ff = lc_write_ff = lc_prewrite = false;
	bank2 = 0xd000;
	
	iwm_switches = iwm_data_rd = iwm_shifter = 0;
	floppy_cycles = 8;
	drive[0].motorOn = drive[1].motorOn = false;
	drive[0].motor_cycles = drive[1].motor_cycles = 0;
	drive[0].bit = drive[1].bit = 0;
	drive[0].phase = drive[1].phase = 0;
	
	//paste = "REM use f10 to paste into here\r";
}






