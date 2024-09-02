#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <filesystem>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <SDL.h>
#include "Settings.h"
#include "psx.h"
extern console* sys;
extern SDL_AudioDeviceID audio_dev;

u32 sized_read(u8* data, u32 addr, int size)
{
	u32 t = 0;
	switch( size )
	{
	case 8: t = data[addr]; break;
	case 16: memcpy(&t, (data+addr), 2); break;
	case 32: memcpy(&t, (data+addr), 4); break;
	default:
		printf("crap read size = %ibits\n", size);
		exit(1);
	}
	return t;
}

void sized_write(u8* data, u32 addr, u32 val, int size)
{
	switch( size )
	{
	case 8: data[addr] = val; break;
	case 16: memcpy((data+addr), &val, 2); break;
	case 32: /*printf("addr$%X = $%X\n", addr, val);*/ memcpy((data+addr), &val, 4); break;
	default:
		printf("crap write size = %ibits\n", size);
		exit(1);
	}
	return;
}

//static bool once = false;

void psx::dma_run_channel(u32 ch)
{
	//printf("DMA: Running channel %i\n", ch);
	if( ch == 2 )
	{
		if( ((dchan[2].chcr>>9)&3) == 2 )
		{
			u32 address = dchan[2].madr&0x1ffffc;
			while(true)
			{
				u32 header = *(u32*)&RAM[address];
 				u32 len = header>>24;
 				for(u32 i = 0; i < len; ++i) gpu_gp0(*(u32*)&RAM[address+4*(i+1)]);
 				u32 next_address = header&0xffFFff;
 				if( next_address & (1u<<23) ) break;
 				address = next_address;
 			}
 			dchan[2].madr = address;
		} else {
			if( dchan[2].chcr & 2 )
			{
				printf("DMA: Channel 2 increment neg!\n");
				exit(1);
			}
			u32 len = (dchan[2].bcr>>16)*((dchan[2].bcr&0xffff));
			u32 start = dchan[2].madr&0x1ffffc;
			if( (dchan[2].chcr&1) == 0 )
			{
				for(u32 i = 0; i < len; ++i) *(u32*)&RAM[start + (i*4)] = gpuread();
			} else {
				//printf("DMA start $%X, len = $%X bytes\n", start, len*4);
				for(u32 i = 0; i < len; ++i) gpu_gp0(*(u32*)&RAM[start + (i*4)]);
			}
			dchan[2].madr = start + len;
		}
		
		if( (DICR & ((1u<<23)|(1u<<(16+2)))) == ((1u<<23)|(1u<<(16+2)))  ) 
		{
			DICR |= (1u<<(24+2));
			u32 old31 = DICR>>31;
			DICR |= ( (DICR&(1u<<15)) || ((DICR&(1u<<23))&&(((DICR>>16)&(DICR>>24)&0x7f))) ) ? (1u<<31) : 0;
			if( old31 == 0 && (DICR>>31) )
			{
				//printf("DMA: IRQ chan 2\n");
				I_STAT |= IRQ_DMA;
			}
		}
		return;
	}
	if( ch == 6 )
	{
		u32 addr = dchan[6].madr & 0x1ffffc;
		u32 len = dchan[6].bcr&0xffff;
		if( len == 0 ) len = 0x10000;
		//printf("chan6 len = $%X\n", len);
		
		/*
		for(u32 i = 0; i < len; ++i) 
		{
			//printf("setting $%X to $%X\n", addr - (i*4), addr - (i*4) - 4);
			*(u32*)&RAM[addr - (i*4)] = addr - (i*4) - 4;
		}
		*(u32*)&RAM[addr - len*4] = 0x00ffFFFF;
		*/
		u32 address = addr;
		u32 dma_len = len;
		for(u32 i = 0; i < dma_len; ++i)
		{
 			u32 header = (i == dma_len - 1) ? 0xFFFFFF : (address - 4);
			*(u32*)&RAM[address] = header;
    			address -= 4;
		}
		dchan[6].madr = addr;
		//printf("set $%X to Fs\n", addr - len*4);
		
		if( (DICR & ((1u<<23)|(1u<<(16+6)))) == ((1u<<23)|(1u<<(16+6))) ) 
		{
			DICR |= (1u<<(24+6));
			u32 old31 = DICR>>31;
			DICR |= ( (DICR&(1u<<15)) || ((DICR&(1u<<23))&&(((DICR>>16)&(DICR>>24)&0x7f))) ) ? (1u<<31) : 0;
			if( old31 == 0 && (DICR>>31) )
			{
				I_STAT |= IRQ_DMA;
				//printf("DMA: IRQ chan 6\n");
			}
		}
		return;
	}
	if( ch == 4 )
	{
		if( (dchan[4].chcr&1) == 0 )
		{
			printf("SPU SRAM Read DMA not supported\n");
			//exit(1);
		} else {
			u32 len = (dchan[4].bcr>>16)*((dchan[4].bcr&0xffff));
			u32 addr = dchan[4].madr & 0x1ffffc;
			
			for(u32 i = 0; i < len; ++i)
			{
				u32 word = *(u32*)&RAM[addr + (i*4)];
				spu_write_addr &= 0x7ffff;
				SRAM[spu_write_addr++] = word;
				spu_write_addr &= 0x7ffff;
				SRAM[spu_write_addr++] = word>>8;
				spu_write_addr &= 0x7ffff;
				SRAM[spu_write_addr++] = word>>16;
				spu_write_addr &= 0x7ffff;
				SRAM[spu_write_addr++] = word>>24;
				spu_write_addr &= 0x7ffff;
			}
			dchan[4].madr = addr + len;
		}
		
		if( (DICR & ((1u<<23)|(1u<<(16+4)))) == ((1u<<23)|(1u<<(16+4))) ) 
		{
			DICR |= (1u<<(24+4));
			u32 old31 = DICR>>31;
			DICR |= ( (DICR&(1u<<15)) || ((DICR&(1u<<23))&&(((DICR>>16)&(DICR>>24)&0x7f))) ) ? (1u<<31) : 0;
			if( old31 == 0 && (DICR>>31) )
			{
				I_STAT |= IRQ_DMA;
				//printf("DMA: IRQ chan 4\n");
			}
		}			
		return;
	}
	if( ch == 3 )
	{
		if( (dchan[3].chcr&1) == 1 )
		{
			printf("CD: Attempt to write to CD?\n");
			exit(1);
		}
		
		u32 len = /*(dchan[3].bcr>>16)*/1*((dchan[3].bcr&0xffff));
		u32 addr = dchan[3].madr & 0x1ffffc;
		printf("DMA: CD read <$%X, len %i bytes\n", addr, len*4);
		
		if( curloc.LBA >= 150 )
		{
			for(u32 i = 0; i < len; ++i)
			{
				*(u32*)&RAM[addr + (i*4)] = *(u32*)&CDDATA[((curloc.LBA-150)*0x930) + ((cd_mode&0x20)?0x0C:0x18) + cd_offset];
				cd_offset += 4;
			}
		}
		dchan[3].madr = addr + len;
		
		if( (DICR & ((1u<<23)|(1u<<(16+3)))) == ((1u<<23)|(1u<<(16+3))) ) 
		{
			DICR |= (1u<<(24+3));
			u32 old31 = DICR>>31;
			DICR |= ( (DICR&(1u<<15)) || ((DICR&(1u<<23))&&(((DICR>>16)&(DICR>>24)&0x7f))) ) ? (1u<<31) : 0;
			if( old31 == 0 && (DICR>>31) )
			{
				I_STAT |= IRQ_DMA;
				//printf("DMA: IRQ chan 3\n");
			}
		}			
		return;
	}
	if( ch == 0 )
	{
		// mdec in
		u32 bs = (dchan[0].bcr>>16);
		if( !bs ) bs = 1;
		u32 len = bs*((dchan[0].bcr&0xffff));
		u32 addr = dchan[0].madr & 0x1ffffc;
		for(u32 i = 0; i < len; ++i)
		{
			mdec_cmd(*(u32*)&RAM[addr + (i*4)]);
		}
		dchan[0].madr = addr + len;
		
		//todo: make this non-instant
		if( (DICR & ((1u<<23)|(1u<<(16+0)))) == ((1u<<23)|(1u<<(16+0))) ) 
		{
			DICR |= (1u<<(24+0));
			u32 old31 = DICR>>31;
			DICR |= ( (DICR&(1u<<15)) || ((DICR&(1u<<23))&&(((DICR>>16)&(DICR>>24)&0x7f))) ) ? (1u<<31) : 0;
			if( old31 == 0 && (DICR>>31) )
			{
				I_STAT |= IRQ_DMA;
				//printf("DMA: IRQ chan 0\n");
			}
		}			
		return;
	}
	if( ch == 1 )
	{
		// mdec out
		u32 bs = (dchan[1].bcr>>16);
		if( !bs ) bs = 1;
		u32 len = bs*((dchan[1].bcr&0xffff));
		u32 addr = dchan[1].madr & 0x1ffffc;
		
		for(u32 i = 0; i < len; ++i)
		{
			*(u32*)&RAM[addr + (i*4)] = mdec_read(0x1F801820, 32);
		}
		dchan[1].madr = addr + len;
		//printf("DMA: MDECout has %i bytes left\n", (int)mdec_out.size());
		//todo: make this non-instant
		if( (DICR & ((1u<<23)|(1u<<(16+1)))) == ((1u<<23)|(1u<<(16+1))) ) 
		{
			DICR |= (1u<<(24+1));
			u32 old31 = DICR>>31;
			DICR |= ( (DICR&(1u<<15)) || ((DICR&(1u<<23))&&(((DICR>>16)&(DICR>>24)&0x7f))) ) ? (1u<<31) : 0;
			if( old31 == 0 && (DICR>>31) )
			{
				I_STAT |= IRQ_DMA;
				//printf("DMA: IRQ chan 0\n");
			}
		}			
		return;
	}
	printf("Unimpl DMA Channel = %i\n", ch);
	exit(1);
}

void psx::run_frame()
{
	for(u32 line = 0; line < 263; ++line)
	{
		if( line == 240 )
		{
			I_STAT |= IRQ_VBLANK | IRQ_SPU;
			if( (gpu_dispmode&0x24) != 0x24 ) gpustat &= ~(1u<<31);
		}
		if( (gpu_dispmode&0x24) != 0x24 && line < 240 ) gpustat ^= (1u<<31);
		for(u32 i = 0; i < 1074; ++i) 
		{
			cpu.step();
			u32 nextopc = psx::read(cpu.pc, 32);
			if( (I_STAT & I_MASK & 0x7FF) )
			{
				cpu.c[13] |= (1u<<10);
			} else {
				cpu.c[13] &= ~(1u<<10);
			}
			if( (nextopc & 0xFE000000) != 0x4A000000 )
			{
				if( (cpu.c[13]&BIT(10)) && (cpu.c[12] & (1u<<10)) && (cpu.c[12]&1) && !cpu.in_delay )
				{
					cpu.c[14] = cpu.pc;
					cpu.pc = 0x80000080;
					cpu.npc = cpu.pc + 4;
					cpu.nnpc = cpu.npc + 4;
					cpu.c[12] = ((cpu.c[12]&~0xff)|((cpu.c[12]&0xf)<<2));
					cpu.c[13] &= ~0xff;
					if( I_STAT & 4 ) printf("CD: IRQ\n");
					cpu.advance_load();
				}
			}
			stamp += 2;
			if( ((t0_mode>>8)&3) == 0 || ((t0_mode>>8)&3) == 2 ) tick_timer_zero(2);
			if( ((t1_mode>>8)&3) == 0 || ((t1_mode>>8)&3) == 2 ) tick_timer_one(2);
			tick_timer_two(2);
			
			spu_cycles += 2;
			if( spu_cycles >= 768 )
			{
				tick_spu();
				spu_cycles -= 768;
				float sm = (Settings::mute ? 0 : spu_out);
				audio_add(sm, sm);
				/*spu_samples.push_back(Settings::mute ? 0 : spu_out); // (spu_ctrl&0xc000)? spu_out : 0 );
				if( spu_samples.size() >= 735 )
				{
					SDL_QueueAudio(audio_dev, spu_samples.data(), spu_samples.size()*4);				
					spu_samples.clear();
				}*/
			}
			
			while( sched.next_stamp() <= stamp )
			{
				sched.run_event();
			}
		}
		if( ((t1_mode>>8)&1) )
		{       // if timer 1 is set to inc on hblank
			tick_timer_one(1);
		}
	}
	gpu_field = (gpu_field ? 0 : 1);
	gpustat &= ~(1u<<31);
	if( (gpu_dispmode&0x24) == 0x24 && gpu_field ) 
	{
		gpustat |= 1u<<31;
	}
	//while( SDL_GetQueuedAudioSize(audio_dev) > 1800*4 );
	
	//return;
	//if( ! (gpu_dispmode&0x10) ) return;
	
	static u32 horires[] = { 256, 320, 512, 640 };
	gpu_xres = (gpu_dispmode&0x40) ? 368 : horires[gpu_dispmode&3];
	gpu_yres = 240;
	if( (gpu_dispmode&0x24) == 0x24 ) gpu_yres = 480;
	u32 xstart = (gpu_dispstart&0x7ff);
	u32 ystart = (gpu_dispstart>>10)&0x3ff;
	//printf("disp start = %i, %i, res(%i, %i)\n", xstart, ystart, gpu_xres, gpu_yres);
	u32 x1 = gpu_hdisp&0xfff;
	u32 x2 = (gpu_hdisp>>12)&0xfff;
	u32 y1 = gpu_vdisp&0x3ff;
	u32 y2 = (gpu_vdisp>>10)&0x3ff;
	if( y2 > y1 )
	{
		gpu_yres = y2 - y1;
	} else {
		gpu_yres = y1 - y2;
	}
	if( (gpu_dispmode&0x24) == 0x24 ) gpu_yres <<= 1;
	//printf("disp values x$%X,$%X , y$%X,$%X\n", x1,x2, y1,y2);
	
	if( gpu_dispmode & 0x10 )
	{
		fbuf.resize(3*gpu_xres*gpu_yres);
		for(u32 y = 0; y < gpu_yres; ++y)
		{
			memcpy(&fbuf[y*gpu_xres*3], &VRAM[(y+ystart)*512 + xstart], 3*gpu_xres);
		}
	} else {
		fbuf.resize(2*gpu_xres*gpu_yres);
		for(u32 y = 0; y < gpu_yres; ++y)
		{
			memcpy(&fbuf[y*gpu_xres*2], &VRAM[(y+ystart)*1024 + xstart], 2*gpu_xres);
		}		
	}

}

void psx::write(u32 addr, u32 val, int size)
{
	if( addr >= 0xfffe0000u )
	{
		//printf("cache ctrl write%i $%X\n", size, addr);
		return;
	}
	//u32 seg = addr >> 30;
	addr &= 0x1fffFFFF;

	if( addr < 8*1024*1024 )
	{
		if( ! (cpu.c[12] & 0x10000) ) sized_write(RAM.data(), addr&0x1fffff, val, size);
		return;
	}
	if( addr >= 0x1F800000 && addr < 0x1F800400 )
	{
		sized_write(scratch, addr&0x3ff, val, size);
		return;
	}
	
	if( addr == 0x1F801100 ) { t0_val = val&0xffff; return; }
	if( addr == 0x1F801110 ) { t1_val = val&0xffff; return; }
	if( addr == 0x1F801120 ) { t2_val = val&0xffff; return; }
	if( addr == 0x1F801104 ) 
	{ 
		t0_mode = (t0_mode&(7u<<10))|(val&~(7u<<10)); 
		t0_mode |= (1u<<10);
		t0_val = 0; 
		printf("TIM0 Mode = $%X\n", val); 
		return; 
	}
	if( addr == 0x1F801114 ) 
	{ 
		t1_mode = (t1_mode&(7u<<10))|(val&~(7u<<10)); 
		t1_mode |= (1u<<10);
		t1_val = 0; 
		printf("TIM1 Mode = $%X\n", val); 
		return; 
	}
	if( addr == 0x1F801124 ) 
	{ 
		t2_mode = (t2_mode&(7u<<10))|(val&~(7u<<10)); 
		t2_mode |= (1u<<10);
		t2_val = 0; 
		printf("TIM2 Mode = $%X\n", val); 
		return; 
	}
	if( addr == 0x1F801108 ) { t0_target = val&0xffff; return; }
	if( addr == 0x1F801118 ) { t1_target = val&0xffff; return; }
	if( addr == 0x1F801128 ) { t2_target = val&0xffff; return; }
	if (addr == 0x1F801074 ) { I_MASK = val; return; }

	if( addr == 0x1F801810 ) { gpu_gp0(val); return; }
	if( addr == 0x1F801814 ) { gpu_gp1(val); return; }
	
	if( addr == 0x1F801070 )
	{
		I_STAT &= val;
		return;
	}
	
	if( addr >= 0x1F801080 && addr < 0x1F8010F0 )
	{
		u32 chan = (addr>>4) & 7;
		u32 reg = (addr >> 2)& 3;
		//printf("$%X (ch%i, reg%i) = $%X\n", addr, chan, reg, val);
		switch(reg)
		{
		case 3: break;
		case 2: dchan[chan].chcr = val; 
			if( val & (1u<<24) ) 
			{ 
				dma_run_channel(chan); 
				dchan[chan].chcr &= ~(1u<<24);
			}
			break;
		case 1: dchan[chan].bcr = val; break;
		case 0: dchan[chan].madr = val; break;
		}
		return;
	}
	
	if( addr == 0x1F8010F0 ) { DPCR = val; return; }
	
	if( addr == 0x1F8010F4 )
	{
		DICR = (DICR&(1u<<31)) | ((DICR&~val&0x7f000000u)) | (val&0xffFFff);
		DICR &= ~(1u<<31);
		DICR |= ( (DICR&(1u<<15)) || ((DICR&(1u<<23))&&(((DICR>>16)&(DICR>>24)&0x7f))) ) ? (1u<<31) : 0;
		return;
	}
	
	if( addr >= 0x1F801C00 && addr < 0x1F801E60 )
	{
		spu_write(addr, val, size);
		return;
	}
		
	if( addr == 0x1F801800 )
	{
		cd_index = val&3;
		return;
	}
	if( addr == 0x1F801801 )
	{
		cd_write_1801(val);
		return;
	}
	if( addr == 0x1F801802 )
	{
		if( cd_index == 0 )
		{
			cd_param.push_back(val);
			//printf("cd param = $%X\n", val);
		} else if( cd_index == 1 ) {
			cd_ie = val;
			if( !(cd_ie&7) ) 
			{
				printf("CD: IE = $%X\n", val);
				//exit(1);
			}
		}
		return;
	}
	if( addr == 0x1F801803 )
	{
		cd_write_1803(val);
		return;
	}
	
	if( addr >= 0x1F801040 && addr < 0x1F801060 )
	{
		pad_write(addr, val);
		return;
	}
	
	if( addr >= 0x1F801820 && addr <= 0x1F801828 )
	{
		mdec_write(addr, val, size);
		return;
	}
	//printf("$%X = $%X, %ibits\n", addr, val, size);	
}

u32 psx::read(u32 addr, int size)
{
	if( addr >= 0xfffe0000u )
	{
		//printf("cache ctrl read%i $%X\n", size, addr);
		return 0;
	}
	//u32 seg = addr >> 30;
	addr &= 0x1fffFFFF;

	if( addr < 8*1024*1024 )
	{
		return sized_read(RAM.data(), addr&0x1fffff, size);
	}
	
	if( addr >= 0x1F800000 && addr < 0x1F800400 )
	{
		return sized_read(scratch, addr&0x3ff, size);
	}
	
	if( addr >= 0x1FC00000 && addr < 0x1FC80000 )
	{
		return sized_read(BIOS.data(), addr&0x7ffff, size);
	}
	
	if( !exp1.empty() && addr >= 0x1F000000u && addr < (0x1F000000u+exp1.size()) )
	{
		return sized_read(exp1.data(), addr-0x1f000000u, size);
	}
	
	//printf("read $%X, %ibits\n", addr, size);

	
	if( addr == 0x1F801810 ) return gpuread();
	if( addr == 0x1F801814 ) return gpustat; // ^= 1u<<31;
	
	if( addr == 0x1F801070 ) return I_STAT;
	if (addr == 0x1F801074 ) return I_MASK;
		
	if( addr == 0x1F801100 ) return t0_val;
	if( addr == 0x1F801110 ) return t1_val;
	if( addr == 0x1F801120 ) return t2_val;
	if( addr == 0x1F801104 ) { t0_mode &= ~(3u<<11); return t0_mode; }
	if( addr == 0x1F801114 ) { t1_mode &= ~(3u<<11); return t1_mode; }
	if( addr == 0x1F801124 ) { t2_mode &= ~(3u<<11); return t2_mode; }
	if( addr == 0x1F801108 ) return t0_target;
	if( addr == 0x1F801118 ) return t1_target;
	if( addr == 0x1F801128 ) return t2_target;
	
	if( addr == 0x1F8010F0 ) return DPCR;
	if( addr == 0x1F8010F4 ) return DICR;
	
	if( addr >= 0x1F801080 && addr < 0x1F8010F0 )
	{
		u32 chan = (addr>>4)&3;
		addr >>= 2;
		switch(addr)
		{
		case 3: break;
		case 2: return dchan[chan].chcr;
		case 1: return dchan[chan].bcr;
		case 0: return dchan[chan].madr;
		}
		return 0;
	}

	if( addr >= 0x1F801C00 && addr < 0x1F801E60 )
	{
		return spu_read(addr, size);
	}
	
	if( addr >= 0x1F801040 && addr < 0x1F801060 ) return pad_read(addr);
	
	if( addr >= 0x1F801820 && addr <= 0x1F801828 )
	{
		return mdec_read(addr, size);
	}
	
	if( addr == 0x1F020018 || addr == 0x1F020010 || addr == 0x1F060000 )
	{
		return 0xff;  // Caetla expects a non-zero value otherwise it boots direct-to-game
	}

	
	if( addr == 0x1F801800 ) 
	{	// reading 1800 always get status/index
		u8 v = (cd_async_active==6?0x40:0) |
				(cd_rcomplete ? 0 : 0x20) | (cd_param.empty() ? 8 : 0) | (cd_param.size()==16 ? 0 : 0x10);
		return v|cd_index;
	}
	if( addr == 0x1F801801 )
	{	// reading 1801 is always reponse q
		u8 r = cd_resp[cd_rpos];
		cd_rpos = (cd_rpos-1)&0xf;
		if( cd_rpos == 15 ) 
		{
			cd_rcomplete = 1;
			if( cd_if == 0 && cd_rcomplete && !cd_irqq.empty() )
			{
				dispatch_cd_irq(cd_irqq.front());
				cd_irqq.erase(cd_irqq.begin());
			}
		}
		//printf("cd resp, got $%X, rpos at %i\n", r, cd_rpos);
		return r;
	}
	if( addr == 0x1F801802 )
	{	// reading 1802 is always data q
		//would be cd data fifo. most thing use DMA (hopefully?)?
		printf("CD: Data fifo usage\n");
		if( curloc.LBA < 150 ) return 0;
		return CDDATA[((curloc.LBA-150)*0x930) + ((cd_mode&0x20)?0x0C:0x18) + (cd_offset++)];		
	}
	
	if( addr == 0x1F801803 )
	{	// 1803 is the only cd reg indexed on read
		if( cd_index & 1 ) return cd_if;
		return cd_ie;
	}
	

	
	return 0;
}

bool psx::loadROM(std::string fname)
{
	if( CDDATA ) munmap(CDDATA, CDDATA_size);
	
	std::string s = Settings::get<std::string>("psx", "bios");
	if( s.empty() ) s = "./bios/PSX.BIOS";
	
	FILE* fp = fopen(s.c_str(), "rb");
	if( ! fp )
	{
		printf("Unable to load BIOS\n");
		exit(1);
	}
	
	fseek(fp, 0, SEEK_END);
	auto fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	BIOS.resize(fsize);

	[[maybe_unused]] int unu = fread(BIOS.data(), 1, fsize, fp);
	fclose(fp);
	
	if( std::filesystem::path(fname).filename() == "open.lid" )
	{
		cd_lid_open = true;
		return true;
	}
	cd_lid_open = false;
	
	std::string expname = Settings::get<std::string>("psx", "exp1");
	if( !expname.empty() )
	{
		FILE* ep = fopen(expname.c_str(), "rb");
		if( ep )
		{
			fseek(ep, 0, SEEK_END);
			exp1.resize(ftell(ep));
			fseek(ep, 0, SEEK_SET);
			unu = fread(exp1.data(), 1, exp1.size(), ep);
			fclose(ep);	
		} else {
			printf("PSX: Failed to load exp1 rom\n");
			exit(1);
		}
	}
	
	fp = fopen(fname.c_str(), "rb");
	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	char a = fgetc(fp);
	char b = fgetc(fp);
	if( a == 'P' && b == 'S' )
	{
		while( cpu.pc != 0x80030000 ) cpu.step();
		printf("PSX: kernel loaded\n");
		fseek(fp, 0, SEEK_SET);
		exe.resize(fsize);
		unu = fread(exe.data(), 1, fsize, fp);
		fclose(fp);
		
		reset_exe();
		
		std::string cdname = Settings::get<std::string>("psx", "cd");
		if( !cdname.empty() )
		{
			int fd = open(fname.c_str(), O_RDONLY);
			if(fd == -1)
			{
				return false;
			}

			struct stat sb;
			fstat(fd, &sb);
			CDDATA_size = sb.st_size;
			CDDATA =(u8*) mmap(nullptr, (CDDATA_size+4095)&~0xfff, PROT_READ,
		               MAP_PRIVATE, fd, 0);
		        close(fd);
		}
				
		return true;
	} else {
		fclose(fp);
		
		int fd = open(fname.c_str(), O_RDONLY);
	        if(fd == -1)
	        {
	        	return false;
	        }

		struct stat sb;
		fstat(fd, &sb);
		CDDATA_size = sb.st_size;
		CDDATA =(u8*) mmap(nullptr, (CDDATA_size+4095)&~0xfff, PROT_READ,
                       MAP_PRIVATE, fd, 0);
                close(fd);
	}

	return true;
}

void psx_write(u32 a, u32 v, int s) { dynamic_cast<psx*>(sys)->write(a, v, s); }
u32 psx_read(u32 a, int s) { return dynamic_cast<psx*>(sys)->read(a, s); }

psx::psx() : sched(this)
{
	VRAM.resize(512*1024);
	RAM.resize(2*1024*1024 + 65535);
	SRAM.resize(512*1024);
	spu_samples.reserve(770);
	cpu.read = psx_read;
	cpu.write = psx_write;
	gpustat = 7<<26;
	cd_rpos = 0;
	cd_rcomplete = 1;
	spu_out = 0;
	slot[0].flag = slot[1].flag = 8;
	CDDATA = nullptr;
	
	for(u32 i = 0; i < 24; ++i) svoice[i].curaddr = svoice[i].start = svoice[i].curnibble = 0;
	spu_kon = spu_koff = 0;
	
	gpu_texmask_x = gpu_texmask_y = gpu_texoffs_x = gpu_texoffs_y = 0;
	
	cpu.reset();
	setVsync(0);
}

/*
static u8 zigzag[] = {
0 ,1 ,5 ,6 ,14,15,27,28,
2 ,4 ,7 ,13,16,26,29,42,
3 ,8 ,12,17,25,30,41,43,
9 ,11,18,24,31,40,44,53,
10,19,23,32,39,45,52,54,
20,22,33,38,46,51,55,60,
21,34,37,47,50,56,59,61,
35,36,48,49,57,58,62,63
};
*/

void psx::reset()
{
	if( exe.empty() ) 
	{
		cpu.reset();
	} else {
		reset_exe();
	}
	
	gpu_dispmode = 0;
	gpu_read_active = false;
	slot[0].pos = slot[1].pos = 0;
	pad_stat = 0;
	spu_cycles = 0;
	spu_samples.clear();
	stamp = 0;
	last_clut_value = 0xffff;
	area_x1 = area_y1 = 0;
	area_x2 = 1024;
	area_y2 = 512;
	t2_div = 0;
	t2_val = t1_val = t0_val = 0;
	t2_mode = t1_mode = t0_mode = 0;
	I_MASK = 0xffff;
	//for(u32 i = 0; i < 64; ++i) zagzig[zigzag[i]]=i;
}

void psx::reset_exe()
{
	memcpy(&cpu.pc, &exe[0x10], 4);
	printf("PSX: EXE start at $%X\n", cpu.pc);
	cpu.npc = cpu.pc + 4;
	cpu.nnpc = cpu.npc + 4;
	memcpy(&cpu.r[28], &exe[0x14], 4);
	u32 temp = 0;
	memcpy(&temp, &exe[0x30], 4);
	if( temp ) cpu.r[29] = temp;
	u32 start = 0;
	memcpy(&start, &exe[0x18], 4);
	u32 len = 0;
	memcpy(&len, &exe[0x1C], 4);
	for(u32 i = 0; i < len; ++i) RAM[(start&0x1fffff)+i] = exe[0x800 + i];
}

bool psx::load_media(int card, std::string fname)
{
	if( card >= 2 ) return false;
	
	if( card==0 ) mc0_fname = fname;
	FILE* fp = fopen(fname.c_str(), "rb");
	if( !fp ) return false;
	
	[[maybe_unused]] int unu = fread(card ? memcard1 : memcard0, 1, 128*1024, fp);
	fclose(fp);	
	return true;
}

psx::~psx() 
{
	setVsync(1);
	FILE* fp = fopen(mc0_fname.c_str(), "wb");
	if( !fp ) return;
	[[maybe_unused]] int unu = fwrite(memcard0, 1, 128*1024, fp);
	fclose(fp);
}

static std::array<std::string, 8> memcards = { "Memcard #0", "Memcard #1" };
std::array<std::string, 8>& psx::media_names() { return memcards; }


