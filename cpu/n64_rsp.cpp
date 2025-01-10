#include <cstdio>
#include <cstdlib>
#include "n64_rsp.h"

typedef void (*rsp_instr)(n64_rsp&, u32);
#define INSTR [](n64_rsp& rsp, u32 opc)

rsp_instr rsp_special(n64_rsp&, u32 opcode)
{
	switch( opcode & 0x3F )
	{
	case 0x0D: return INSTR { rsp.broke(); };
	default: return INSTR{};
	}
}

rsp_instr rsp_regimm(n64_rsp&, u32 opcode)
{
	return INSTR{};
}

rsp_instr rsp_regular(n64_rsp&, u32 opcode)
{
	return INSTR{};
}

void n64_rsp::step()
{
	u32 opc = __builtin_bswap32(*(u32*)&IMEM[pc&0xffc]);
	if( (opc>>26) == 0 )
	{
		rsp_special(*this, opc)(*this, opc);
	} else if( (opc>>26) == 1 ) {
		rsp_regimm(*this, opc)(*this, opc);
	} else {
		rsp_regular(*this, opc)(*this, opc);
	}
	//todo: break might not run the pipeline after?	
	pc = npc & 0xffc;
	npc = nnpc;
	nnpc += 4;
}








