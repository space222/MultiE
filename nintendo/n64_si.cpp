#include <cstring>
#include "n64.h"

void n64::pif_run()
{
	if( pifram[0x3f] & 0x10 ) { pifram[0x3f] &= ~0x10; pif_rom_enabled = false; pifram[0x3f] &= 0x7f; }
	if( pifram[0x3f] & 0x40 ) { pifram[0x3f] &= ~0x40; pifram[0x3f] &= 0x7f; }
	if( pifram[0x3f] & 0x20 ) { pifram[0x3f] &= ~0x20; pifram[0x3f] |= 0x80; }


	if( !(pifram[0x3f] & 1) ) return;
	pifram[0x3f] &= ~1;
	pifram[0x3f] |= 0x80;
	// from here on it's controller time
	//todo: controller time
	
	int chan = 0;
	
	for(u32 i = 0; i < 63 && chan < 6; ++i)
	{
		pifram[i] |= 0x80;
	}
}

void n64::si_write(u32 addr, u32 v)
{
	addr = (addr&0x1F)>>2;
	if( addr > 6 ) return;
	if( addr == 6 )
	{	// SI_STATUS: writing clears interrupt
		si_regs[5] = 0;
		clear_mi_bit(MI_INTR_SI_BIT);
		printf("N64: SI irq cleared\n");
		return;
	}
	
	si_regs[addr] = v;
	
	if( addr == 1 )
	{	// SI_PIF_AD_RD64B: run the pif and read the results back to ram
		pif_run();
		memcpy(mem.data()+(si_regs[0]&0x7fffff), pifram, 64);
		raise_mi_bit(MI_INTR_SI_BIT);
		si_regs[6] |= BIT(12);
		si_regs[0] += 60;  // "points to last word written to" ?
		return;
	}
	
	if( addr == 4 )
	{	// SI_PIF_AD_WR64B
		memcpy(pifram, mem.data()+(si_regs[0]&0x7fffff), 64);
		raise_mi_bit(MI_INTR_SI_BIT);
		si_regs[0] += 60; // ??
		return;
	}	
}

u32 n64::si_read(u32 addr)
{
	addr = (addr&0x1F)>>2;
	if( addr > 6 ) return 0;
	return si_regs[addr];
}




