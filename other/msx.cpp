#include <cctype>
#include "msx.h"
extern console* sys;
char unshift(char pc);

void msx::write(u16 addr, u8 v)
{
	//printf("MSX: write $%X = $%X\n", addr, v);

	u8 top2 = addr>>14;
	if( slots[top2] == 3 ) ram[addr] = v;
}

void msx::out(u16 p, u8 v)
{
	p &= 0xff;
	if( p == 0xA8 )
	{
		//printf("Slots = $%X\n", v);
		slotreg = v;
		slots[0] = v&3;
		slots[1] = (v>>2)&3;
		slots[2] = (v>>4)&3;
		slots[3] = (v>>6);
		return;
	}
	if( p == 0xA0 ) { psg.set(v); return; }
	if( p == 0xA1 ) { psg.write(v); return; }
	if( p == 0x99 )
	{
		vdp.ctrl(v);
		return;
	}
	if( p == 0x98 )
	{
		vdp.data(v);
		return;
	}
	if( p == 0xAB )
	{
		p = 0xAA;
		u8 bit = (v>>1)&7;
		u8 nb = v&1;
		v = ppi_c;
		if( nb )
		{
			v |= (1u<<bit);
		} else {
			v &= ~(1u<<bit);
		}
		// fallthrough and write to ppi_c	
	}
	if( p == 0xAA ) 
	{
		u8 old_c = ppi_c;
		kbrow = v&0xf;
		ppi_c = v;
		
		if( (old_c&0x10) && !(ppi_c&0x10) )
		{
			base_cas_stamp = stamp;
		}		
		if( !casrd && !(ppi_c&0x10) )
		{
			tap.emplace_back(stamp-base_cas_stamp, (ppi_c&BIT(5))?0x80:0);
			base_cas_stamp = stamp;
		}
		return;
	}
	
	
	printf("MSX: out $%X = $%X\n", p, v);
	
}

u8 msx::in(u16 p)
{
	p &= 0xff;
	if( p == 0xA8 ) return slotreg;
	if( p == 0xAA ) return ppi_c;
	if( p == 0xA2 )
	{
		if( psg.index == 14 )
		{
			auto keys = SDL_GetKeyboardState(nullptr);
			u8 v = 0x7f;
			if( keys[SDL_SCANCODE_UP] ) v ^= BIT(0);
			if( keys[SDL_SCANCODE_DOWN] ) v ^= BIT(1);
			if( keys[SDL_SCANCODE_LEFT] ) v ^= BIT(2);
			if( keys[SDL_SCANCODE_RIGHT] ) v ^= BIT(3);
			if( casrd && caspos<tap.size() && !(ppi_c&0x10) && stamp - base_cas_stamp >= tap[caspos].first )
			{
				base_cas_stamp = stamp;
				caspos += 1;
			}
			if( casrd ) v |= tap[caspos].second;
			return v;
		}
		return psg.read();
	}
	if( p == 0x99 ) 
	{
		vdp.vdp_ctrl_latch = false; 
		u8 val = vdp.vdp_ctrl_stat;
		vdp.vdp_ctrl_stat &= ~0x80;
		cpu.irq_line = 0;
		return val;
	}
	if( p == 0x98 )
	{
		vdp.vdp_ctrl_latch = false;
		u8 val = vdp.rdbuf;
		vdp.rdbuf = vdp.vram[vdp.vdp_addr&0x3fff];
		vdp.vdp_addr+=1;	
		return val;
	}
	if( p == 0xA9 )
	{
		auto keys = SDL_GetKeyboardState(nullptr);
		u8 v = 0xff;
		char cc = paste[pastepos];
		char pc = unshift(cc);
		shifted = (pc!=cc);
		if( kbrow == 0 )
		{
			if( keys[SDL_SCANCODE_7] || pc=='7' ) v ^= BIT(7);
			if( keys[SDL_SCANCODE_6] || pc=='6' ) v ^= BIT(6);
			if( keys[SDL_SCANCODE_5] || pc=='5' ) v ^= BIT(5);
			if( keys[SDL_SCANCODE_4] || pc=='4' ) v ^= BIT(4);
			if( keys[SDL_SCANCODE_3] || pc=='3' ) v ^= BIT(3);
			if( keys[SDL_SCANCODE_2] || pc=='2' ) v ^= BIT(2);
			if( keys[SDL_SCANCODE_1] || pc=='1' ) v ^= BIT(1);
			if( keys[SDL_SCANCODE_0] || pc=='0' ) v ^= BIT(0);
			return v;
		}
		if( kbrow == 1 )
		{
			if( keys[SDL_SCANCODE_SEMICOLON] || pc==';' ) v ^= BIT(7);
			if( keys[SDL_SCANCODE_RIGHTBRACKET] || pc==']' ) v ^= BIT(6);
			if( keys[SDL_SCANCODE_LEFTBRACKET] || pc=='[' ) v ^= BIT(5);
			if( keys[SDL_SCANCODE_BACKSLASH] || pc=='\\' ) v ^= BIT(4);
			if( keys[SDL_SCANCODE_EQUALS] || pc=='=' ) v ^= BIT(3);
			if( keys[SDL_SCANCODE_MINUS] || pc=='-' ) v ^= BIT(2);
			if( keys[SDL_SCANCODE_9] || pc=='9' ) v ^= BIT(1);
			if( keys[SDL_SCANCODE_8] || pc=='8' ) v ^= BIT(0);
			return v;
		}
		if( kbrow == 2 )
		{
			if( keys[SDL_SCANCODE_B] || pc=='b' ) v ^= BIT(7);
			if( keys[SDL_SCANCODE_A] || pc=='a' ) v ^= BIT(6);
			if( keys[SDL_SCANCODE_SLASH] || pc=='/' ) v ^= BIT(4);
			if( keys[SDL_SCANCODE_PERIOD] || pc=='.' ) v ^= BIT(3);
			if( keys[SDL_SCANCODE_COMMA] || pc==',' ) v ^= BIT(2);
			if( keys[SDL_SCANCODE_GRAVE] || pc=='`' ) v ^= BIT(1);
			if( keys[SDL_SCANCODE_APOSTROPHE] || pc=='\'' ) v ^= BIT(0);
			return v;
		}
		if( kbrow == 3 )
		{
			if( keys[SDL_SCANCODE_J] || pc=='j' ) v ^= BIT(7);
			if( keys[SDL_SCANCODE_I] || pc=='i' ) v ^= BIT(6);
			if( keys[SDL_SCANCODE_H] || pc=='h' ) v ^= BIT(5);
			if( keys[SDL_SCANCODE_G] || pc=='g' ) v ^= BIT(4);
			if( keys[SDL_SCANCODE_F] || pc=='f' ) v ^= BIT(3);
			if( keys[SDL_SCANCODE_E] || pc=='e' ) v ^= BIT(2);
			if( keys[SDL_SCANCODE_D] || pc=='d' ) v ^= BIT(1);
			if( keys[SDL_SCANCODE_C] || pc=='c' ) v ^= BIT(0);
			return v;
		}
		if( kbrow == 4 )
		{
			if( keys[SDL_SCANCODE_R] || pc=='r' ) v ^= BIT(7);
			if( keys[SDL_SCANCODE_Q] || pc=='q' ) v ^= BIT(6);
			if( keys[SDL_SCANCODE_P] || pc=='p' ) v ^= BIT(5);
			if( keys[SDL_SCANCODE_O] || pc=='o' ) v ^= BIT(4);
			if( keys[SDL_SCANCODE_N] || pc=='n' ) v ^= BIT(3);
			if( keys[SDL_SCANCODE_M] || pc=='m' ) v ^= BIT(2);
			if( keys[SDL_SCANCODE_L] || pc=='l' ) v ^= BIT(1);
			if( keys[SDL_SCANCODE_K] || pc=='k' ) v ^= BIT(0);
			return v;
		}
		if( kbrow == 5 )
		{
			if( keys[SDL_SCANCODE_Z] || pc=='z' ) v ^= BIT(7);
			if( keys[SDL_SCANCODE_Y] || pc=='y' ) v ^= BIT(6);
			if( keys[SDL_SCANCODE_X] || pc=='x' ) v ^= BIT(5);
			if( keys[SDL_SCANCODE_W] || pc=='w' ) v ^= BIT(4);
			if( keys[SDL_SCANCODE_V] || pc=='v' ) v ^= BIT(3);
			if( keys[SDL_SCANCODE_U] || pc=='u' ) v ^= BIT(2);
			if( keys[SDL_SCANCODE_T] || pc=='t' ) v ^= BIT(1);
			if( keys[SDL_SCANCODE_S] || pc=='s' ) v ^= BIT(0);
			return v;
		}
		if( kbrow == 6 )
		{
			if( keys[SDL_SCANCODE_F3] ) v ^= BIT(7);
			if( keys[SDL_SCANCODE_F2] ) v ^= BIT(6);
			if( keys[SDL_SCANCODE_F1] ) v ^= BIT(5);
			//if( keys[SDL_SCANCODE_O] ) v ^= BIT(4);
			if( keys[SDL_SCANCODE_CAPSLOCK] ) v ^= BIT(3);
			//if( keys[SDL_SCANCODE_M] ) v ^= BIT(2);
			if( keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL] ) v ^= BIT(1);
			if( keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT] || shifted ) v ^= BIT(0);
			return v;
		}
		if( kbrow == 7 )
		{
			if( keys[SDL_SCANCODE_RETURN] || pc=='\n' ) v ^= BIT(7);
			//if( keys[SDL_SCANCODE_F2] ) v ^= BIT(6);
			if( keys[SDL_SCANCODE_BACKSPACE] ) v ^= BIT(5);
			//if( keys[SDL_SCANCODE_O] ) v ^= BIT(4);
			if( keys[SDL_SCANCODE_TAB] || pc=='\t' ) v ^= BIT(3);
			if( keys[SDL_SCANCODE_ESCAPE] ) v ^= BIT(2);
			if( keys[SDL_SCANCODE_F5] ) v ^= BIT(1);
			if( keys[SDL_SCANCODE_F4] ) v ^= BIT(0);
			return v;
		}
		if( kbrow == 8 )
		{
			if( keys[SDL_SCANCODE_RIGHT] ) v ^= BIT(7);
			if( keys[SDL_SCANCODE_DOWN] ) v ^= BIT(6);
			if( keys[SDL_SCANCODE_UP] ) v ^= BIT(5);
			if( keys[SDL_SCANCODE_LEFT] ) v ^= BIT(4);
			if( keys[SDL_SCANCODE_DELETE] ) v ^= BIT(3);
			if( keys[SDL_SCANCODE_INSERT] ) v ^= BIT(2);
			if( keys[SDL_SCANCODE_HOME] || keys[SDL_SCANCODE_RCTRL] ) v ^= BIT(1);
			if( keys[SDL_SCANCODE_SPACE] || pc==' ' ) v ^= BIT(0);
			return v;
		}
		return 0xff;
	}
	printf("MSX: in $%X\n", p);
	return 0;
}

u8 msx::read(u16 addr)
{
	u8 top2 = addr>>14;
	if( slots[top2] == 0 ) return bios[addr&0x7fff];
	if( slots[top2] == 1 || slots[top2] == 2 ) return rom[addr];
	//printf("MSX read <$%X\n", addr);
	return ram[addr];
}

void msx::reset()
{
	vdp.reset();
	psg.reset();
	//memset(&cpu, 0, sizeof(cpu));
	cpu.write8 = [](u16 a, u8 v) { dynamic_cast<msx*>(sys)->write(a,v); };
	cpu.out8 = [](u16 a, u8 v) { dynamic_cast<msx*>(sys)->out(a,v); };
	cpu.in8 = [](u16 a)->u8 { return dynamic_cast<msx*>(sys)->in(a); };
	cpu.read8 = [](u16 a)->u8 { return dynamic_cast<msx*>(sys)->read(a); };
	stamp = last_target = sample_cycles = 0;
	slotreg = 0b11110000;
	slots[0] = slots[1] = 0;
	slots[2] = slots[3] = 3;
	casrd = true;
	caspos = 0;
	pastepos = 0;
	shifted = false;
	pfr = 4;
	cpu.pc = 0;
	cpu.sp = 0xdffe;
	cpu.irq_line = 0;
	cpu.iff1 = cpu.iff2 = false;
	cpu.halted = false;
}

bool msx::loadROM(const std::string fname) 
{
	FILE* fp = fopen("./bios/MSX.ROM", "rb");	
	if( !fp )
	{
		printf("Unable to open MSX.ROM\n");
		return false;
	}
	[[maybe_unused]] int unu = fread(bios, 1, 32*1024, fp);
	fclose(fp);
	
	fp = fopen(fname.c_str(), "rb");
	if( !fp )
	{
		printf("Unable to load '%s'\n", fname.c_str());
		return false;
	}
	fseek(fp, 0, SEEK_END);
	auto fsz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	unu = fread(rom, 1, 64*1024, fp);
	fclose(fp);
	
	if( fsz <= 8*1024 )
	{
		u8* dest = rom+8*1024;
		for(u32 i = 1; i < 8; ++i, dest+=8*1024) memcpy(dest, rom, 8*1024);
	} else if( fsz <= 16*1024 ) {
		u8* dest = rom+16*1024;
		for(u32 i = 1; i < 3; ++i, dest+=16*1024) memcpy(dest, rom, 16*1024);
	} else if( fsz <= 32*1024 ) {
		memcpy(rom+32*1024, rom, 32*1024);
	}
	return true;
}

bool again = false;

void msx::run_frame()
{
	for(u32 line = 0; line < 262; ++line)
	{
		//set_vcount(line);	
		u64 target = last_target + 227;
		while( stamp < target )
		{
			u64 c = cpu.step();
			stamp += c;
			u8 snd = psg.cycles(c);
			sample_cycles += c;
			if( sample_cycles >= 81 )
			{
				sample_cycles -= 81;
				float sm = ((snd/45.f)*2 - 1);
				audio_add(sm,sm);
			}
		}
		last_target = target;
		if( line < 192 )
		{
			vdp.draw_scanline(line);
		} else if( line == 192 ) {
			vdp.vdp_ctrl_stat |= 0x80;
			if( (vdp.vdp_regs[1] & BIT(5)) )
			{
				cpu.irq_line = 1;			
			}
		}
	}
	
	if( !paste.empty() )
	{
		pfr -= 1;
		if( pfr == 0 )
		{
			pastepos += 1;
			if( pastepos >= paste.size() )
			{
				pastepos = 0;
				paste.clear();
			} else if( paste[pastepos] == paste[pastepos-1] ) {
				// if two characters in a row are the same
				// the key is never released.
				// so insert a dummy non-printable at the previous character
				// there'll be a frame without a new keypress, but not a big deal
				// this probably has a corner case at end of input, but most of the time
				// that should be something like a visible character + new line
				paste[pastepos-1] = 1;
				pastepos-=1;
			}
			pfr = 4;
		}
	}
}

msx::msx()
{
	setVsync(0);
	FILE* fp = fopen("out.tap", "rb");
	if(!fp) return;
	size_t sz = 0;
	[[maybe_unused]] int unu = fread(&sz, 1, sizeof(sz), fp);
	tap.resize(sz);
	unu = fread(tap.data(), 1, sz*sizeof(tap[0]), fp);
	fclose(fp);
}

msx::~msx()
{
	FILE* fp = fopen("out.tap", "wb");
	size_t sz = tap.size();
	fwrite(&sz, 1, sizeof(sz), fp);
	fwrite(tap.data(), 1, sizeof(tap[0])*tap.size(), fp);
	fclose(fp);
}

char unshift(char pc)
{
	switch( pc )
	{
	case '~': return '`';
	case '!': return '1';
	case '@': return '2';
	case '#': return '3';
	case '$': return '4';
	case '%': return '5';
	case '^': return '6';
	case '&': return '7';
	case '*': return '8';
	case '(': return '9';	
	case ')': return '0';
	case '_': return '-';
	case '+': return '=';
	case ':': return ';';
	case '"': return '\'';
	case '{': return '[';
	case '}': return ']';
	case '|': return '\\';
	case '?': return '/';
	case '<': return ',';
	case '>': return '.';
	}
	return std::tolower(pc);
}

