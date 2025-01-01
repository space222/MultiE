#include <cstring>
#include "n64.h"

void n64::pif_run()
{

}

void n64::si_write(u32 addr, u32 v)
{
	addr = (addr&0x1F)>>2;
	if( addr > 5 ) return;
	if( addr == 5 )
	{	// SI_STATUS: writing clears interrupt
		si_regs[5] = 0;
		clear_mi_bit(MI_INTR_SI_BIT);
		return;
	}
	
	si_regs[addr] = v;
	
	if( addr == 1 )
	{	// SI_PIF_AD_RD64B: run the pif and read the results back to ram
		pif_run();
		memcpy(mem.data()+(si_regs[0]&0x7fffff), pifram, 64);
		raise_mi_bit(MI_INTR_SI_BIT);
		si_regs[0] += 60;  // "points to last word written to" ?
		return;
	}
	
	if( addr == 3 )
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
	if( addr > 5 ) return 0;
	return si_regs[addr];
}




