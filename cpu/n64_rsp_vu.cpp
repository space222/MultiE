#include <cstdio>
#include <cstdlib>
#include "n64_rsp.h"

typedef void (*rsp_instr)(n64_rsp&, u32);
#define INSTR [](n64_rsp& rsp, u32 opc)
#define VUWC2 u32 vt = (opc>>16)&0x1f; u32 base = (opc>>21)&0x1f; u32 e = (opc>>7)&15; s32 offs = s8(opc<<1)>>1

rsp_instr rsp_lwc2(n64_rsp& rsp, u32 opcode)
{
	switch( (opcode>>11) & 0x1f )
	{
	case 0: // LBV
		return INSTR {
			VUWC2;
			u32 addr = rsp.r[base] + offs;
			rsp.v[vt].b(e) = rsp.DMEM[addr&0xfff];
		};
	case 1: // LSV
		return INSTR {
			VUWC2;
			u32 addr = rsp.r[base] + offs;
			rsp.v[vt].b(e) = rsp.DMEM[addr&0xfff];
			if( e != 15 ) rsp.v[vt].b(e+1) = rsp.DMEM[(addr+1)&0xfff];
		};	
	default: printf("RSP LWC2: unimpl opcode $%X\n", (opcode>>11)&0x1f);
		 //exit(1);
		 return INSTR {};
	}
}

rsp_instr rsp_swc2(n64_rsp& rsp, u32 opcode)
{
	switch( (opcode>>11) & 0x1f )
	{
	case 0: // SBV
		return INSTR {
			VUWC2;
			u32 addr = rsp.r[base] + offs;
			rsp.DMEM[addr&0xfff] = rsp.v[vt].b(e);
		};
	case 1: // SSV
		return INSTR {
			VUWC2;
			u32 addr = rsp.r[base] + offs;
			rsp.DMEM[addr&0xfff] = rsp.v[vt].b(e); 
			rsp.DMEM[(addr+1)&0xfff] = rsp.v[vt].b(e+1);
		};
	default: printf("RSP SWC2: unimpl opcode $%X\n", (opcode>>11)&0x1f);
		 //exit(1);
		 return INSTR {};
	}
}
