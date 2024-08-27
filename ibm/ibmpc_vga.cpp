#include <bit>
#include "ibmpc.h"

#define VGA_READ_MAP (vga.reg3CE[4]&3)
#define VGA_MISC_GFX vga.reg3CE[6]
#define VGA_COLOR_COMPARE vga.reg3CE[2]
#define VGA_COLOR_DONT_CARE vga.reg3CE[7]
#define VGA_GFX_MODE vga.reg3CE[5]
#define VGA_BIT_MASK vga.reg3CE[8]
#define VGA_DATA_ROTATE vga.reg3CE[3]
#define VGA_SETRESET_EN vga.reg3CE[1]
#define VGA_SET_RESET vga.reg3CE[0]

#define VGA_MAP_MASK vga.reg3C4[2]

void ibmpc::vga_port_write(u16 port, u16 val, int size)
{
	//printf("VGA port $%X = $%X\n", port, val);
		
	switch( port )
	{
	case 0x3C0:
		vga.index3C0 = (val&0xff);
		if( size == 16 )
		{
			vga.reg3C0[vga.index3C0&0x1F] = val>>8;
		}
		return;
	case 0x3C1: //todo: this is wrong, 3C0 uses a index/val flipflop
		vga.reg3C0[vga.index3C0] = val;
		return;
	case 0x3C2: vga.misc_output = val; return;
	case 0x3C4:
		vga.index3C4 = (val&0xff);
		if( size == 16 )
		{
			vga.reg3C4[vga.index3C4&0x1F] = val>>8;
		}
		return;
	case 0x3C5:
		vga.reg3C4[vga.index3C4] = val;
		return;
	case 0x3C7:
		vga.dac_read_index = (val&0xff)*3;
		vga.dac_state = 0;
		return;
	case 0x3C8:
		vga.dac_write_index = (val&0xff)*3;
		vga.dac_state = 3;
		if( size == 16 )
		{
			vga.dac[vga.dac_write_index++] = val>>8;
			if( vga.dac_write_index == 0x300 ) vga.dac_write_index = 0;
		}
		return;
	case 0x3C9:
		vga.dac[vga.dac_write_index++] = val;
		if( vga.dac_write_index == 0x300 ) vga.dac_write_index = 0;
		return;
	case 0x3CE: 
		vga.index3CE = val&0xff; 
		if( size == 16 ) 
		{
			printf("setting16 3CE reg #%i = $%X\n", vga.index3CE, val);
			vga.reg3CE[vga.index3CE] = val>>8;
		}
		return;
	case 0x3CF:
		printf("setting 3CE reg #%i = $%X\n", vga.index3CE, val);
		vga.reg3CE[vga.index3CE] = val;
		return;	
	}
}

u8 ibmpc::vga_port_read(u16 port)
{
	if( port == 0x3C0 )
	{
		return vga.index3C0;
	}
	if( port == 0x3C1 )
	{
		return vga.reg3C0[vga.index3C0];
	}
	if( port == 0x3C4 )
	{
		return vga.index3C4;
	}
	if( port == 0x3C5 )
	{
		return vga.reg3C4[vga.index3C4];
	}
	if( port == 0x3CF ) return vga.reg3CE[vga.index3CE];
	if( port == 0x3C7 )
	{
		return vga.dac_state;	
	}
	if( port == 0x3C8 )
	{
		return vga.dac_write_index/3;	
	}
	if( port == 0x3C9 )
	{
		u8 r = vga.dac[vga.dac_read_index++];
		if( vga.dac_read_index >= 0x300 ) vga.dac_read_index = 0;
		return r;
	}
	if( port == 0x3CC ) return vga.misc_output;
	return 0;
}

u8 ibmpc::vga_read_vram(u32 addr)
{
	if( VGA_MISC_GFX & 1 )
	{
		vga.rdlatch[0] = gfx[(addr&0xffff)];
		vga.rdlatch[1] = gfx[0x10000 + (addr&0xffff)];
		vga.rdlatch[2] = gfx[0x20000 + (addr&0xffff)];
		vga.rdlatch[3] = gfx[0x30000 + (addr&0xffff)];
		if( VGA_GFX_MODE & 8 )
		{
			const u8 dc = 0xf & ~VGA_COLOR_DONT_CARE;
			const u8 cc = (dc | VGA_COLOR_COMPARE) & 0xf;
			u8 comp = 0;
			for(int i = 7; i >= 0; --i)
			{
				const u8 p0 = (vga.rdlatch[0]>>i)&1;
				const u8 p1 = (vga.rdlatch[1]>>i)&1;
				const u8 p2 = (vga.rdlatch[2]>>i)&1;
				const u8 p3 = (vga.rdlatch[3]>>i)&1;
				const u8 c = p0|(p1<<1)|(p2<<2)|(p3<<3);
				comp |= ((dc | c) == cc) ? (1u<<i) : 0;
			}
			return comp;
		}
		return gfx[VGA_READ_MAP*0x10000 + (addr&0xffff)];
	}
	return gfx[addr&0x7fff];
}

void ibmpc::vga_write_vram(u32 addr, u8 val)
{
	if( VGA_MISC_GFX & 1 )
	{
		const u32 write_mode = VGA_GFX_MODE & 3;
		const u32 rotate_cnt = VGA_DATA_ROTATE & 7;
		const u32 operation = (VGA_DATA_ROTATE>>3)&3;
		u8 data[4];
		if( write_mode == 0 )
		{
			val = std::rotr(val, rotate_cnt);
			data[0] = data[1] = data[2] = data[3] = val;
			if( VGA_SETRESET_EN & 1 ) data[0] = (VGA_SET_RESET & 1) ? 0xff : 0;
			if( VGA_SETRESET_EN & 2 ) data[1] = (VGA_SET_RESET & 2) ? 0xff : 0;
			if( VGA_SETRESET_EN & 4 ) data[2] = (VGA_SET_RESET & 4) ? 0xff : 0;
			if( VGA_SETRESET_EN & 8 ) data[3] = (VGA_SET_RESET & 8) ? 0xff : 0;
			switch( operation )
			{
			case 0: break;
			case 1: data[0]&=vga.rdlatch[0]; data[1]&=vga.rdlatch[1]; data[2]&=vga.rdlatch[2]; data[3]&=vga.rdlatch[3]; break;
			case 2: data[0]|=vga.rdlatch[0]; data[1]|=vga.rdlatch[1]; data[2]|=vga.rdlatch[2]; data[3]|=vga.rdlatch[3]; break;
			case 3: data[0]^=vga.rdlatch[0]; data[1]^=vga.rdlatch[1]; data[2]^=vga.rdlatch[2]; data[3]^=vga.rdlatch[3]; break;
			}	
			data[0] &= VGA_BIT_MASK;
			data[0] |= vga.rdlatch[0] & ~VGA_BIT_MASK;
			data[1] &= VGA_BIT_MASK;
			data[1] |= vga.rdlatch[1] & ~VGA_BIT_MASK;
			data[2] &= VGA_BIT_MASK;
			data[2] |= vga.rdlatch[2] & ~VGA_BIT_MASK;
			data[3] &= VGA_BIT_MASK;
			data[3] |= vga.rdlatch[3] & ~VGA_BIT_MASK;		
		} else if( write_mode == 1 ) {
			data[0] = vga.rdlatch[0];
			data[1] = vga.rdlatch[1];
			data[2] = vga.rdlatch[2];
			data[3] = vga.rdlatch[3];
		} else if( write_mode == 2 ) {
			data[0] = (val&1)?0xff:0;
			data[1] = (val&2)?0xff:0;
			data[2] = (val&4)?0xff:0;
			data[3] = (val&8)?0xff:0;
			switch( operation )
			{
			case 0: break;
			case 1: data[0]&=vga.rdlatch[0]; data[1]&=vga.rdlatch[1]; data[2]&=vga.rdlatch[2]; data[3]&=vga.rdlatch[3]; break;
			case 2: data[0]|=vga.rdlatch[0]; data[1]|=vga.rdlatch[1]; data[2]|=vga.rdlatch[2]; data[3]|=vga.rdlatch[3]; break;
			case 3: data[0]^=vga.rdlatch[0]; data[1]^=vga.rdlatch[1]; data[2]^=vga.rdlatch[2]; data[3]^=vga.rdlatch[3]; break;
			}		
			data[0] &= VGA_BIT_MASK;
			data[0] |= vga.rdlatch[0] & ~VGA_BIT_MASK;
			data[1] &= VGA_BIT_MASK;
			data[1] |= vga.rdlatch[1] & ~VGA_BIT_MASK;
			data[2] &= VGA_BIT_MASK;
			data[2] |= vga.rdlatch[2] & ~VGA_BIT_MASK;
			data[3] &= VGA_BIT_MASK;
			data[3] |= vga.rdlatch[3] & ~VGA_BIT_MASK;			
		} else {
			data[0] = (VGA_SET_RESET&1)?0xff:0;
			data[1] = (VGA_SET_RESET&2)?0xff:0;
			data[2] = (VGA_SET_RESET&4)?0xff:0;
			data[3] = (VGA_SET_RESET&8)?0xff:0;
			val = std::rotr(val, rotate_cnt) & VGA_BIT_MASK;
			data[0] &= val;
			data[0] |= vga.rdlatch[0] & ~val;
			data[1] &= val;
			data[1] |= vga.rdlatch[1] & ~val;
			data[2] &= val;
			data[2] |= vga.rdlatch[2] & ~val;
			data[3] &= val;
			data[3] |= vga.rdlatch[3] & ~val;			
		}	
		if( VGA_MAP_MASK & 1 ) gfx[addr&0xffff] = data[0];
		if( VGA_MAP_MASK & 2 ) gfx[0x10000 + (addr&0xffff)] = data[1];
		if( VGA_MAP_MASK & 4 ) gfx[0x20000 + (addr&0xffff)] = data[2];
		if( VGA_MAP_MASK & 8 ) gfx[0x30000 + (addr&0xffff)] = data[3];
		return;
	}
	// currently not supporting fancy operations in text mode
	if( VGA_READ_MAP == 2 ) printf("loading font\n");
	gfx[addr&0x7fff] = val;	
}


