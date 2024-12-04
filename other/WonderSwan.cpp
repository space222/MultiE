#include "WonderSwan.h"

void WonderSwan::write(u32 addr, u8 v)
{
	addr &= 0xffFFff;
	printf("WS: Write $%X = $%X\n", addr, v);
	if( addr < 0x10000 ) { ram[addr] = v; return; }
	if( addr < 0x20000 ) { sram[((sram_bank<<16)+(addr-0x10000))] = v; return; }
	return;
}

u8 WonderSwan::read(u32 addr)
{
	addr &= 0xffFFff;
	//printf("WS: Read <$%X\n", addr);
	if( addr < 0x10000 ) return ram[addr];
	if( addr < 0x20000 ) return sram[(sram_bank<<16)+(addr-0x10000)];
	if( addr < 0x30000 ) return ROM[((rom_bank0<<16)+(addr-0x20000))%ROM.size()];
	if( addr < 0x40000 )
	{
		return ROM[((rom_bank1<<16)+(addr-0x30000))%ROM.size()];
	}
	
	if( (SYSTEM_CTRL1 & 1) && (addr >= 0xfe000) ) return bios[addr-0xfe000];
	
	return ROM[((rom_linear*1024*1024)+addr)%ROM.size()];
}

void WonderSwan::out(u16 port, u16 v, int sz)
{
	printf("WS: Out%i $%X = $%X\n", sz, port, v);
	if( sz == 16 )
	{
		switch( port )
		{
		case 0xBA: eeprom_wr = v; return;
		case 0xBC: eeprom_cmd = v; return;
		default: break;
		}
		out(port, v&0xff, 8);
		out(port+1, v>>8, 8);
		return;
	}
	if( port >= 0x80 && port < 0x9F ) return;
	switch( port )
	{
	case 0x04:
		sprite_table = v;
		break;
	case 0x05:
		sprite_first = v & 0x7f;
		break;
	case 0x06:
		sprite_cnt = v;
		break;
	case 0x07:
		screen_addr = v;
		break;
	case 0x10:
		scr1_x = v;
		break;
	case 0x11:
		scr1_y = v;
		break;
	case 0x12:
		scr2_x = v;
		break;
	case 0x13:
		scr2_y = v;
		break;
	case 0x60:
		SYSTEM_CTRL2 = v;
		break;
	case 0xA0: 
		SYSTEM_CTRL1 &= ~BIT(2); 
		SYSTEM_CTRL1 |= v&BIT(2);
		break;
	case 0xB5:
		KEY_SCAN = v;
		break;
	case 0xBE: 
		eeprom_run(); 
		break;
	case 0xC2:
		rom_bank0 = v;
		break;
	case 0xC3:
		rom_bank1 = v;
		break;
	default: 
		exit(1);
		break;	
	}
}

u16 WonderSwan::in(u16 port, int sz)
{
	printf("WS: In%i <$%X\n", sz, port);
	if( sz == 16 )
	{
		switch( port )
		{
		case 0xBA: return eeprom_rd;
		case 0xBC: return eeprom_cmd;
		default: break;
		}
		u16 r = in(port, 8);
		r |= in(port+1, 8)<<8;
		return r;
	}
	if( port >= 0x80 && port < 0x9F ) return 0;
	switch( port )
	{
	case 0x04: return sprite_table;
	case 0x05: return sprite_first;
	case 0x06: return sprite_cnt;
	case 0x07: return screen_addr;
	
	case 0x10: return scr1_x;
	case 0x11: return scr1_y;
	case 0x12: return scr2_x;
	case 0x13: return scr2_y;
	case 0x60: return SYSTEM_CTRL2;
	case 0xA0: return SYSTEM_CTRL1;
	case 0xB5: return KEY_SCAN&0xf;
	case 0xBE: return eeprom_stat;
	case 0xC2: return rom_bank0;
	case 0xC3: return rom_bank1;

	default: break;
	}
	return 0;
}

void WonderSwan::reset()
{
	cpu.read = [&](u32 addr)->u8 { return read(addr); };
	cpu.write = [&](u32 addr, u8 v) { write(addr, v); };
	cpu.port_in = [&](u16 p, int sz)->u16 { return in(p, sz); };
	cpu.port_out = [&](u16 p, u16 v, int sz) { out(p, v, sz); };
	cpu.reset();
	
	rom_linear = 0xff;
	rom_bank0 = 0xff;
	rom_bank1 = 0xff;
	sram_bank = 0xff;
	SYSTEM_CTRL1 = 0x8B;
	SYSTEM_CTRL2 = 0;
	eeprom_stat = 0x83;
}

void WonderSwan::run_frame()
{
	for(u32 i = 0; i < 150; ++i)
	{
		printf("WS: linIP = $%X\n",  (cpu.start_cs<<4) + cpu.IP);
		cpu.step();
	}
}

bool WonderSwan::loadROM(const std::string fname)
{
	FILE* fp = fopen("./bios/wsbios.bin", "rb");
	if( !fp )
	{
		printf("Unable to load WonderSwan bootrom 'wsbios.bin'\n");
		return false;
	}
	[[maybe_unused]] int unu = fread(bios, 1, 0x2000, fp);
	fclose(fp);
	
	fp = fopen(fname.c_str(), "rb");
	if( !fp )
	{
		printf("Failed to load WonderSwan rom '%s'\n", fname.c_str());
		return false;
	}
	
	fseek(fp, 0, SEEK_END);
	auto fsz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	ROM.resize(fsz);
	unu = fread(ROM.data(), 1, fsz, fp);
	fclose(fp);

	return true;
}

void WonderSwan::eeprom_run()
{
	printf("WS:$%X: EEPROM Cmd = $%X\n", (cpu.start_cs<<4) + cpu.IP, eeprom_cmd);
	u32 c = (eeprom_cmd >> 10) & 3;
	if( c == 0 )
	{
		c = (eeprom_cmd>>8)&3;
		if( c == 3 )
		{
			eeprom_stat &= ~BIT(7);
			return;
		}
		printf("WS: EEPROM subop = %i\n", c);
		exit(1);
	} else if( c == 1 ) {
		exit(1);
	} else if( c == 2 ) {
		u32 waddr = (eeprom_cmd&0x3ff)<<1;
		waddr &= 0x7ff;
		eeprom_rd = *(u16*)&EEPROM[waddr];	
	} else {
		exit(1);
	
	}
}

