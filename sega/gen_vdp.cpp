#include <cstdio>
#include <cstdlib>
#include "genesis.h"

static u32 ntsize[] = { 32, 64, 128, 128 };

void genesis::render_sprite(u32 y, u32 hpos, u32 htiles, u32 vtiles, u16 entry)
{
	if( hpos + (htiles*8) < 128 || hpos >= (vdp_width+128) ) return;
	if( (entry & BIT(12)) ) y = ((vtiles*8)-1) - y;
	u16 tstart = (entry&0x7ff)*32 + (y&7)*4;
	u8 tdata[16];
	for(u32 i = 0; i < htiles; ++i)
	{
		memcpy(&tdata[i*4], &VRAM[tstart + (vtiles*i + (y/8))*32], 4);
	}
	
	for(u32 i = 0; i < htiles*8; ++i)
	{
		if( hpos < 128 || (sprbuf[hpos-128]&0xf) ) { hpos+=1; continue; }
		u32 ind = ((entry&BIT(11))? (htiles*8)-1-i : i);
		u8 t = 0xf & (tdata[ind>>1] >> ((ind&1)?0:4));
		if( t )
		{
			t |= (entry>>9)&0x30;
			t |= (entry>>8)&0x80;
		}
		sprbuf[hpos-128] = t;
		hpos += 1;
		if( hpos-128 >= vdp_width ) break;
	}
}

void genesis::eval_sprites(u32 line)
{
	memset(sprbuf, 0, 320);

	u16 spraddr = vreg[5]&0x7f;
	if( vdp_width == 320 ) spraddr &= ~1;
	spraddr <<= 9;
	
	u32 num = 0;
	u32 pix = 0;
	u32 spr = 0;
	u32 max = 0;
	
	do {
		max += 1;
		u32 vpos = (0x3FF & __builtin_bswap16(*(u16*)&VRAM[spraddr + (spr*8)]));
		u32 vs = (VRAM[spraddr + (spr*8) + 2]&3)+1;
		if( (line+128) >= vpos && (line+128) - vpos < (vs*8) )
		{
			num+=1;
			u16 hpos = (0x1FF & __builtin_bswap16(*(u16*)&VRAM[spraddr + (spr*8) + 6]));
			if( hpos == 0 ) break; //also needs low pri?
			u32 hs = ((VRAM[spraddr + (spr*8) + 2]>>2)&3)+1;
			pix += hs*8;
			u16 entry = __builtin_bswap16(*(u16*)&VRAM[spraddr + (spr*8) + 4]);
			render_sprite((line+128)-vpos, hpos, hs, vs, entry);	
		}
	} while( max < (vdp_width==320 ? 80:64) &&
		 num < (vdp_width==320 ? 20:16) && 
		 pix < vdp_width && (spr=VRAM[spraddr + (spr*8) + 3]&0x7f)  );	
}

bool genesis::vdp_in_window(u32 line, u32 px)
{
	if( vreg[17] & 0x80 )
	{
		if( px >= (vreg[17]&0x1F)*16 ) return true;	
	} else {
		if( px < (vreg[17]&0x1F)*16 ) return true;
	}
	if( vreg[18] & 0x80 )
	{
		if( line >= (vreg[18]&0x1F)*8 ) return true;
	} else {
		if( line < (vreg[18]&0x1F)*8 ) return true;
	}
	return false;
}

void genesis::draw_line(u32 line)
{
	eval_sprites(line);

	u32 ntA = ((vreg[2]>>3)&7) << 13;
	u32 ntB = (vreg[4]&7) << 13;
	u32 ntW = (vreg[3]>>1)&0x1f;
	//if( vdp_width == 320 ) ntW &= ~1;
	ntW <<= 11;
	
	const u32 nt_height= ntsize[(vreg[16]>>4)&3];
	const u32 nt_width = ntsize[vreg[16]&3];
	const u32 vmask = (nt_height*8)-1;
	const u32 hmask = (nt_width*8)-1;
	
	const u32 hs_base = (vreg[0xD]&0x3F)<<10;
	u32 hsA = __builtin_bswap16(*(u16*)&VRAM[hs_base]);
	u32 hsB = __builtin_bswap16(*(u16*)&VRAM[hs_base+2]);
	
	for(u32 px = 0; px < vdp_width; ++px)
	{
		if( (vreg[0xB] & 3) == 3 )
		{
			hsA = __builtin_bswap16(*(u16*)&VRAM[hs_base + line*4]);
			hsB = __builtin_bswap16(*(u16*)&VRAM[hs_base + line*4 + 2]);		
		} else if( (vreg[0xB] & 3) == 2 ) {
			hsA = __builtin_bswap16(*(u16*)&VRAM[hs_base + (line&~7)*4]);
			hsB = __builtin_bswap16(*(u16*)&VRAM[hs_base + (line&~7)*4 + 2]);		
		}	
		hsA &= 0x3ff;
		hsB &= 0x3ff;
		
		u32 vsA = 0, vsB = 0;
		if( vreg[0xB] & BIT(2) )
		{
			vsA = __builtin_bswap16(*(u16*)&VSRAM[(px>>4)*4]);
			vsB = __builtin_bswap16(*(u16*)&VSRAM[(px>>4)*4 + 2]);
		} else {
			vsA = __builtin_bswap16(*(u16*)&VSRAM[0]);
			vsB = __builtin_bswap16(*(u16*)&VSRAM[2]);
		}
		vsA &= 0x3ff;
		vsB &= 0x3ff;
		
		u16 entryA = 0;		
		u8 tA = 0;	
		if( vdp_in_window(line, px) )
		{
			entryA = __builtin_bswap16(*(u16*)&VRAM[ntW + (((line&vmask)/8)*nt_width + ((px&hmask)/8))*2]);
			u32 lXW = ((px&7) ^ ((entryA&BIT(11))?7:0));
			tA = VRAM[(entryA&0x7ff)*32 + ((line&7) ^ ((entryA&BIT(12))?7:0))*4 + (lXW>>1)];
			tA >>= (lXW&1)?0:4;
		} else {
			entryA = __builtin_bswap16(*(u16*)&VRAM[ntA + (((((line+vsA)&vmask)/8)*nt_width) + (((px-hsA)&hmask)/8))*2]);
			u32 lYA = ((line+vsA)&7) ^ ((entryA&BIT(12))?7:0);
			u32 lXA = ((px-hsA)&7) ^ ((entryA&BIT(11))?7:0);
			tA = VRAM[(entryA&0x7ff)*32 + lYA*4 + (lXA>>1)] >> ((lXA&1)?0:4);
		}
		tA &= 0xf;
		
		u16 entryB = __builtin_bswap16(*(u16*)&VRAM[ntB + (((((line+vsB)&vmask)/8)*nt_width) + (((px-hsB)&hmask)/8))*2]);
		u32 lYB = ((line+vsB)&7) ^ ((entryB&BIT(12))?7:0);
		u32 lXB = ((px-hsB)&7) ^ ((entryB&BIT(11))?7:0);
		u8 tB = VRAM[(entryB&0x7ff)*32 + lYB*4 + (lXB>>1)] >> ((lXB&1)?0:4);
		tB &= 0xf;
		
		if( (sprbuf[px] & 0x80) && (sprbuf[px]&0xf) )
		{
			tA = sprbuf[px]&0x3f;
		} else if( tA && (entryA&BIT(15)) ) {
			tA |= (entryA>>9)&0x30;
		} else if( tB && (entryB&BIT(15)) ) {
			tA = tB | ((entryB>>9)&0x30);
		} else if( !(sprbuf[px]&0x80) && (sprbuf[px]&0xf) ) {
			tA = sprbuf[px]&0x3f;
		} else if( tA && !(entryA&BIT(15)) ) {
			tA |= (entryA>>9)&0x30;
		} else if( tB && !(entryB&BIT(15)) ) {
			tA = tB | ((entryB>>9)&0x30);
		} else {
			tA = vreg[7]&0x3F;
		}
			
		u32 c = __builtin_bswap16(*(u16*)&CRAM[tA<<1]);
		fbuf[line*vdp_width + px] = ((c&0xe)<<28)|((c&0xe0)<<16)|((c&0xe00)<<4);
	}
	return;
}

void genesis::vdp_ctrl(u16 val)
{
	if( !vdp_latch && (val&0xC000) == 0x8000 )
	{
		//printf("VDP: regwrite = $%X\n", val);
		u32 r = (val>>8)&0x1f;
		vreg[r] = val;
		vdp_addr &= ~0x3fff;
		vdp_cd &= ~3;
		return;
	}
	
	if( vdp_latch )
	{	// second write
		vdp_addr &= ~0xC000;
		vdp_addr |= (val<<14);
		vdp_cd &= ~0x3c;
		vdp_cd |= (val&0xf0)>>2;
		//printf("%X: VDP: cd = $%X, addr = $%X\n", cpu.pc-2, vdp_cd, vdp_addr);
		if( (vdp_cd & BIT(5)) && (vreg[1]&BIT(4)) )
		{
			if( (vreg[0x17] & 0xc0) == 0x80 )
			{
				fill_pending = true;
			} else if( (vreg[0x17] & 0xc0) == 0xc0 ) {
				vdp_vram2vram();
				vreg[0x14] = vreg[0x13] = 0;
			} else {
				u32 len = (vreg[0x14]<<8)|vreg[0x13];
				if( len == 0 ) len = 0xffff;
				u32 src = 0;
				for(u32 i = 0; i < len; ++i)
				{
					src = (vreg[0x17]<<16)|(vreg[0x16]<<8)|vreg[0x15];
					vdp_data(read(src<<1,16));
					vreg[0x15] += 1;
					vreg[0x16] += ((vreg[0x15]==0)?1:0);
				}
				vreg[0x14] = vreg[0x13] = 0;
			}
		}	
	} else {
		//first write
		vdp_addr &= ~0x3fff;
		vdp_addr |= (val&0x3fff);
		vdp_cd &= ~3;
		vdp_cd |= (val>>14);
	}

	vdp_latch = !vdp_latch;
	//printf("VDP: non-reg write = $%X\n", val);
}

u16 genesis::vdp_read()
{
	u16 index = vdp_addr;
	vdp_addr += vreg[0xf];
	
	switch( vdp_cd & 0xf )
	{
	case 0: return __builtin_bswap16(*(u16*)&VRAM[index&0xfffe]);
	case 8: return __builtin_bswap16(*(u16*)&CRAM[index&0x7e]);
	default:
		printf("VDP: Read with CD = $%X\n", vdp_cd);
		exit(1);	
	}
	return 0;
}

void genesis::vdp_data(u16 val)
{
	if( fill_pending )
	{
		fill_pending = false;
		u32 len = (vreg[0x14]<<8)|vreg[0x13];
		if( len == 0 ) len = 0xffff;
		if( (vdp_cd&0xf) != 1 )
		{
			printf("VDP Fill CD = $%X\n", vdp_cd);
			//exit(1);
		}
		for(u32 i = 0; i < len; ++i) 
		{
			VRAM[vdp_addr] = val;
			vdp_addr += vreg[0xf];
		}
		vreg[0x14] = vreg[0x13] = 0;
		return;
	}
	
	u16 index = vdp_addr;
	vdp_addr += vreg[0xf];
	
	switch( vdp_cd & 0xf )
	{
	case 1:
		VRAM[index] = val>>8;
		VRAM[index^1] = val;	
		break;
	case 3:
		index = (index>>1)&0x3F;
		CRAM[index*2] = val>>8;
		CRAM[index*2 + 1] = val;
		break;
	case 5:
		//todo: check masking
		*(u16*)&VSRAM[index&0x7e] = __builtin_bswap16(val);
		break;
	default:
		printf("GEN: VDP write $%X with cd = $%X\n", val, vdp_cd);
		//exit(1);
	}
	
}

void genesis::vdp_vram2vram()
{
	u32 len = (vreg[0x14]<<8)|vreg[0x13];
	if( len == 0 ) len = 0xffff;
	for(u32 i = 0; i < len; ++i)
	{
		VRAM[vdp_addr] = VRAM[(vreg[0x16]<<8)|vreg[0x15]];
		vreg[0x15] += 1;
		vreg[0x16] += ((vreg[0x15]==0)?1:0);
		vdp_addr += vreg[0xf];
	}
	vreg[0x14] = vreg[0x13] = 0;
}

