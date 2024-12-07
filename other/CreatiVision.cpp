#include "CreatiVision.h"
extern console* sys;

void CreatiVision::write(u16 addr, u8 v)
{
	if( addr < 0x400 ) { ram[addr] = v; return; }
	if( addr == 0x1002 ) { psg.out(v); return; }

	printf("CV:$%X: write $%X = $%X\n", cpu.PC, addr, v);
	if( addr == 0x3000 ) { vdp.data(v); return; }
	if( addr == 0x3001 ) { vdp.ctrl(v); return; }
}

u8 CreatiVision::read(u16 addr)
{
	if( addr < 0x400 ) return ram[addr];
	if( addr == 0x1003 ) return 0xff; // key input?
	if( addr == 0x2000 ) 
	{
		vdp.vdp_ctrl_latch = false;
		u8 val = vdp.rdbuf;
		vdp.rdbuf = vdp.vram[vdp.vdp_addr&0x3fff];
		vdp.vdp_addr+=1;	
		return val;
	}
	if( addr == 0x2001 )
	{
		vdp.vdp_ctrl_latch = false; 
		u8 val = vdp.vdp_ctrl_stat;
		vdp.vdp_ctrl_stat &= ~0x80;
		//cpu.irq_line = 0;
		return val;
	}
	if( addr >= 0xf800 ) 
	{ 
		return bios[addr&0x7ff]; 
	}
	if( addr > (0xc000 - ROM.size()) ) { return ROM[addr - (0xc000-ROM.size())]; }
	return 0;
}

void CreatiVision::run_frame()
{
	for(u32 i = 0; i < 1500; ++i) cpu.step();
	for(u32 i = 0; i < fb_height(); ++i) vdp.draw_scanline(i);
}

void CreatiVision::reset()
{
	cpu.write = [](u16 a, u8 v) { dynamic_cast<CreatiVision*>(sys)->write(a,v); };
	cpu.read = [](u16 a) -> u8 { return dynamic_cast<CreatiVision*>(sys)->read(a); };
	cpu.reset();
	vdp.reset();
	psg.reset();
}

bool CreatiVision::loadROM(const std::string fname)
{
	FILE* fp = fopen("./bios/bioscv.rom", "rb");
	if(!fp)
	{
		printf("CreatiVision: Unable to load bios 'bioscv.rom'\n");
		return false;
	}
	
	[[maybe_unused]] int unu = fread(bios, 1, 2048, fp);
	fclose(fp);
	
	fp = fopen(fname.c_str(), "rb");
	if( !fp )
	{
		printf("CreatiVision: Unable to load ROM '%s'\n", fname.c_str());
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

