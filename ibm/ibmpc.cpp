#include <cstdio>
#include <cstdlib>
#include <bit>
#include "ibmpc.h"

extern console* sys;
extern uint64_t ibmpc_font[128];

u8 dmaregs[0x10];

u8 ibmpc::read(u32 addr)
{
	if( addr < 0xA0000 ) return RAM[addr];
	if( addr >= 0xb8000 && addr < 0xc0000 ) return vga_read_vram(addr);
	//if( addr >= 0xffff0 ) printf("addr = $%X, res = $%X\n", addr, addr - (0x100000-ROM.size()));
	if( addr >= (0x100000 - ROM.size()) ) return ROM[addr - (0x100000-ROM.size())];
	//printf("<$%X\n", addr); //, u32(0x100000 - ROM.size()));
	return 0;
}

void ibmpc::write(u32 addr, u8 v)
{
	if( addr < 0xA0000 )
	{
		RAM[addr] = v;
		return;
	}
	if( addr >= 0xb0000 && addr < 0xb8000 ) 
	{
		vga_write_vram(addr, v);
		return;	
	}
	if( addr >= 0xb8000 && addr < 0xc0000 ) 
	{
		vga_write_vram(addr, v);
		return;
	}
	printf("$%X = $%X\n", addr, v);
}

u8 d = 0;

u16 ibmpc::port_in(u16 port, int size)
{
	printf("in%i $%X\n", size, port);
	
	if( port < 0x10 ) return dmaregs[port];
	
	if( port == 0x20 || port == 0x21 ) return pic_read(port);
	if( port >= 0x40 && port <= 0x43 ) return pit_read(port);
	if( port == 0x3DA )
	{
		d ^= 0x9;
		return d;
	}
	
	if( port == 0x3FA ) return 0;
	if( port == 0x2FA || port == 0x3EA || port == 0x2EA ) return 0xffff;
		
	if( port == 0x3BA || port == 0x3B4 ) return vga_port_read(port);
	if( port >= 0x3C0 && port < 0x3F0 ) return vga_port_read(port);
	
	
	if( port >= 0x3F0 && port < 0x400 ) return fdc_read(port);
	
	if( port == 0x62 )
	{
		if( PPI_B & 8 ) return 0x2;
		return 0xc;
	}
	
	if( port == 0x61 )
	{
		u8 res = PPI_B;
		PPI_B ^= 0x10;
		return res;
	}
	
	if( port == 0x201 ) return 0xff;
	return 0xff;
}

void ibmpc::port_out(u16 port, u16 val, int size)
{
	//*((u16*)&RAM[0x400]) = 0x3F8;
	//*((u16*)&RAM[0x411]) &= ~0xe;
	
	if( port < 0x10 )
	{
		dmaregs[port] = val;
		return;
	}
	printf("out%i $%X = $%X\n", size, port, val);
	if( port == 0x20 || port == 0x21 )
	{
		pic_write(port, val);
		/*if( val == 0x20 )
		{
			if( cpu.pic.isr ) cpu.pic.isr &= ~(1<<std::countr_zero(cpu.pic.isr));
		}*/
		return;
	}
	
	if( port == 0x61 )
	{
		PPI_B = val;
		return;
	}

	if( port >= 0x40 && port <= 0x43 ) 
	{
		printf("pit $%X= $%X\n", port, val);
		pit_write(port, val);
		return;
	}
	
	if( port == 0x3BA || port == 0x3B4 || (port >= 0x3C0 && port < 0x3F0) )
	{
		vga_port_write(port, val, size);
		return;
	}
	if( port >= 0x3F0 && port < 0x400 ) 
	{
		fdc_write(port,val);
		return;
	}
	
	
}

void ibmpc::reset()
{
	cpu.reset();
	//ROM[0xFfffe - (0x100000 - ROM.size())] = 0xFC;
	
	vga.index3CE = vga.index3D4 = vga.index3C4 = 0;
	vga.index3C0 = -1;
}

void ibmpc::run_frame()
{
	for(int i = 0; i < 70000; ++i)
	{
		cpu.step();
		for(int b = 0; b < 7; ++b)
		{
			pitcycle();
		}
	}
	
	for(u32 y = 0; y < 480; ++y)
	{
		for(u32 x = 0; x < 640; ++x)
		{
			u32 ch = gfx[((y/8)*80 + (x/8))*2];
			if( ch & 0x80 ) ch &= 0x7f;
			u8 b = ((u8*)ibmpc_font)[ch*8 + ((y&7)^7)];
			b >>= (x&7)^7;
			b &= 1;
			fbuf[y*720 + x] = 
			fbuf[y*720 + x + 1] =
			//fbuf[(y+1)*720 + x] = 
			//fbuf[(y+1)*720 + x + 1] =
			b ? 0xffffff00 : 0;
		}
	}
}

bool ibmpc::loadROM(std::string fname)
{
	FILE* fp = fopen(fname.c_str(), "rb");
	if( ! fp ) return false;
	
	fseek(fp, 0, SEEK_END);
	auto fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	ROM.resize(fsize);
	[[maybe_unused]] int unu = fread(ROM.data(), 1, fsize, fp);
	fclose(fp);
	return true;
}

u8 ibmpc_read(u32 addr) { return dynamic_cast<ibmpc*>(sys)->read(addr); }
void ibmpc_write(u32 addr, u8 v) { dynamic_cast<ibmpc*>(sys)->write(addr, v); }
u16 ibmpc_in(u16 port, int size) { return dynamic_cast<ibmpc*>(sys)->port_in(port, size); }
void ibmpc_out(u16 port, u16 val, int size) { dynamic_cast<ibmpc*>(sys)->port_out(port, val, size); }

ibmpc::ibmpc()
{
	cpu.read = ibmpc_read;
	cpu.write = ibmpc_write;
	cpu.port_in = ibmpc_in;
	cpu.port_out = ibmpc_out;
	RAM.resize(0xA0000);
	fbuf.resize(720*480);
	gfx.resize(256*1024);
	pitchan[0].gate = pitchan[1].gate = 1;
	pitchan[2].gate = 0;
	pitdiv = 8;
}

static std::array<std::string, 8> pc_media = { "Floppy A", "Floppy B" };

std::array<std::string, 8>& ibmpc::media_names()
{
	return pc_media;
}

uint64_t ibmpc_font[128] = {
	0x7E7E7E7E7E7E0000,	/* NUL */
	0x7E7E7E7E7E7E0000,	/* SOH */
	0x7E7E7E7E7E7E0000,	/* STX */
	0x7E7E7E7E7E7E0000,	/* ETX */
	0x7E7E7E7E7E7E0000,	/* EOT */
	0x7E7E7E7E7E7E0000,	/* ENQ */
	0x7E7E7E7E7E7E0000,	/* ACK */
	0x7E7E7E7E7E7E0000,	/* BEL */
	0x7E7E7E7E7E7E0000,	/* BS */
	0x0,			/* TAB */
	0x7E7E7E7E7E7E0000,	/* LF */
	0x7E7E7E7E7E7E0000,	/* VT */
	0x7E7E7E7E7E7E0000,	/* FF */
	0x7E7E7E7E7E7E0000,	/* CR */
	0x7E7E7E7E7E7E0000,	/* SO */
	0x7E7E7E7E7E7E0000,	/* SI */
	0x7E7E7E7E7E7E0000,	/* DLE */
	0x7E7E7E7E7E7E0000,	/* DC1 */
	0x7E7E7E7E7E7E0000,	/* DC2 */
	0x7E7E7E7E7E7E0000,	/* DC3 */
	0x7E7E7E7E7E7E0000,	/* DC4 */
	0x7E7E7E7E7E7E0000,	/* NAK */
	0x7E7E7E7E7E7E0000,	/* SYN */
	0x7E7E7E7E7E7E0000,	/* ETB */
	0x7E7E7E7E7E7E0000,	/* CAN */
	0x7E7E7E7E7E7E0000,	/* EM */
	0x7E7E7E7E7E7E0000,	/* SUB */
	0x7E7E7E7E7E7E0000,	/* ESC */
	0x7E7E7E7E7E7E0000,	/* FS */
	0x7E7E7E7E7E7E0000,	/* GS */
	0x7E7E7E7E7E7E0000,	/* RS */
	0x7E7E7E7E7E7E0000,	/* US */
	0x0,			/* (space) */
	0x808080800080000,	/* ! */
	0x2828000000000000,	/* " */
	0x287C287C280000,	/* # */
	0x81E281C0A3C0800,	/* $ */
	0x6094681629060000,	/* % */
	0x1C20201926190000,	/* & */
	0x808000000000000,	/* ' */
	0x810202010080000,	/* ( */
	0x1008040408100000,	/* ) */
	0x2A1C3E1C2A000000,	/* * */
	0x8083E08080000,	/* + */
	0x81000,		/* , */
	0x3C00000000,		/* - */
	0x80000,		/* . */
	0x204081020400000,	/* / */
	0x1824424224180000,	/* 0 */
	0x8180808081C0000,	/* 1 */
	0x3C420418207E0000,	/* 2 */
	0x3C420418423C0000,	/* 3 */
	0x81828487C080000,	/* 4 */
	0x7E407C02423C0000,	/* 5 */
	0x3C407C42423C0000,	/* 6 */
	0x7E04081020400000,	/* 7 */
	0x3C423C42423C0000,	/* 8 */
	0x3C42423E023C0000,	/* 9 */
	0x80000080000,		/* : */
	0x80000081000,		/* ; */
	0x6186018060000,	/* < */
	0x7E007E000000,		/* = */
	0x60180618600000,	/* > */
	0x3844041800100000,	/* ? */
	0x3C449C945C201C,	/* @ */
	0x1818243C42420000,	/* A */
	0x7844784444780000,	/* B */
	0x3844808044380000,	/* C */
	0x7844444444780000,	/* D */
	0x7C407840407C0000,	/* E */
	0x7C40784040400000,	/* F */
	0x3844809C44380000,	/* G */
	0x42427E4242420000,	/* H */
	0x3E080808083E0000,	/* I */
	0x1C04040444380000,	/* J */
	0x4448507048440000,	/* K */
	0x40404040407E0000,	/* L */
	0x4163554941410000,	/* M */
	0x4262524A46420000,	/* N */
	0x1C222222221C0000,	/* O */
	0x7844784040400000,	/* P */
	0x1C222222221C0200,	/* Q */
	0x7844785048440000,	/* R */
	0x1C22100C221C0000,	/* S */
	0x7F08080808080000,	/* T */
	0x42424242423C0000,	/* U */
	0x8142422424180000,	/* V */
	0x4141495563410000,	/* W */
	0x4224181824420000,	/* X */
	0x4122140808080000,	/* Y */
	0x7E040810207E0000,	/* Z */
	0x3820202020380000,	/* [ */
	0x4020100804020000,	/* \ */
	0x3808080808380000,	/* ] */
	0x1028000000000000,	/* ^ */
	0x7E0000,		/* _ */
	0x1008000000000000,	/* ` */
	0x3C023E463A0000,	/* a */
	0x40407C42625C0000,	/* b */
	0x1C20201C0000,		/* c */
	0x2023E42463A0000,	/* d */
	0x3C427E403C0000,	/* e */
	0x18103810100000,	/* f */
	0x344C44340438,		/* g */
	0x2020382424240000,	/* h */
	0x800080808080000,	/* i */
	0x800180808080870,	/* j */
	0x20202428302C0000,	/* k */
	0x1010101010180000,	/* l */
	0x665A42420000,		/* m */
	0x2E3222220000,		/* n */
	0x3C42423C0000,		/* o */
	0x5C62427C4040,		/* p */
	0x3A46423E0202,		/* q */
	0x2C3220200000,		/* r */
	0x1C201804380000,	/* s */
	0x103C1010180000,	/* t */
	0x2222261A0000,		/* u */
	0x424224180000,		/* v */
	0x81815A660000,		/* w */
	0x422418660000,		/* x */
	0x422214081060,		/* y */
	0x3C08103C0000,		/* z */
	0x1C103030101C0000,	/* { */
	0x808080808080800,	/* | */
	0x38080C0C08380000,	/* } */
	0x324C000000,		/* ~ */
	0x7E7E7E7E7E7E0000	/* DEL */
};


