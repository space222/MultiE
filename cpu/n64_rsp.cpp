#include <cstdio>
#include <cstdlib>
#include "n64_rsp.h"

#define JTYPE u32 target = ((opc<<2)&0x0fffFFFFu)
#define RTYPE u32 d = (opc>>11)&0x1f; u32 s = (opc>>21)&0x1f; u32 t = (opc>>16)&0x1f; u32 sa=(opc>>6)&0x1f
#define ITYPE u32 t = (opc>>16)&0x1f; u32 s = (opc>>21)&0x1f; u16 imm16 = opc
#define LINK (cpu.nnpc&0xffc)
#define DS_REL_ADDR (cpu.npc + (s32(s16(imm16))<<2))

typedef void (*rsp_instr)(n64_rsp&, u32);
#define INSTR [](n64_rsp& cpu, u32 opc)

rsp_instr rsp_lwc2(n64_rsp&, u32);
rsp_instr rsp_swc2(n64_rsp&, u32);
rsp_instr rsp_cop2(n64_rsp&, u32);

rsp_instr rsp_special(n64_rsp&, u32 opcode)
{
	switch( opcode & 0x3F )
	{
	case 0x00: return INSTR { RTYPE; cpu.r[d] = (cpu.r[t] << sa); }; // SLL
	case 0x02: return INSTR { RTYPE; cpu.r[d] = (cpu.r[t] >> sa); }; // SRL
	case 0x03: return INSTR { RTYPE; cpu.r[d] = (s32(cpu.r[t]) >> sa); }; // SRA
	case 0x04: return INSTR { RTYPE; cpu.r[d] = (cpu.r[t] << (cpu.r[s]&0x1F)); }; // SLLV
	
	case 0x06: return INSTR { RTYPE; cpu.r[d] = (cpu.r[t] >> (cpu.r[s]&0x1F)); }; // SRLV
	case 0x07: return INSTR { RTYPE; cpu.r[d] = (s32(cpu.r[t]) >> (cpu.r[s]&0x1F)); }; // SRAV
	
	case 0x08: return INSTR { RTYPE; cpu.branch(cpu.r[s]); }; // JR
	case 0x09: return INSTR { RTYPE; u32 temp = cpu.r[s]; cpu.r[d] = LINK; cpu.branch(temp); }; // JALR
	
	case 0x0D: return INSTR { /*printf("rsp: break\n");*/ cpu.broke(); }; // BREAK
	
	case 0x20: return INSTR { RTYPE; cpu.r[d] = (cpu.r[t]) + (cpu.r[s]); };  // ADD
	case 0x21: return INSTR { RTYPE; cpu.r[d] = (cpu.r[t]) + (cpu.r[s]); }; // ADDU
	case 0x22: return INSTR { RTYPE; cpu.r[d] = (cpu.r[s]) - (cpu.r[t]); }; // SUB	
	case 0x23: return INSTR { RTYPE; cpu.r[d] = (cpu.r[s]) - (cpu.r[t]); }; // SUBU
	case 0x24: return INSTR { RTYPE; cpu.r[d] = cpu.r[t] & cpu.r[s]; }; // AND
	case 0x25: return INSTR { RTYPE; cpu.r[d] = cpu.r[t] | cpu.r[s]; }; // OR
	case 0x26: return INSTR { RTYPE; cpu.r[d] = cpu.r[t] ^ cpu.r[s]; }; // XOR
	case 0x27: return INSTR { RTYPE; cpu.r[d] = ~(cpu.r[t] | cpu.r[s]); }; // NOR

	case 0x2A: return INSTR { RTYPE; cpu.r[d] = s32(cpu.r[s]) < s32(cpu.r[t]); }; // SLT
	case 0x2B: return INSTR { RTYPE; cpu.r[d] = cpu.r[s] < cpu.r[t]; }; // SLTU
	
	default: printf("RSP: Unimpl special opcode = $%X\n", opcode&0x3F); exit(1); return INSTR {};
	}
}

rsp_instr rsp_regimm(n64_rsp&, u32 opcode)
{
	switch( (opcode>>16) & 0x1F )
	{
	case 0x00: return INSTR { ITYPE; if( s32(cpu.r[s]) < 0 ) { cpu.branch(DS_REL_ADDR); } }; // BLTZ
	case 0x01: return INSTR { ITYPE; if( s32(cpu.r[s]) >= 0 ) { cpu.branch(DS_REL_ADDR); } }; // BGEZ
	case 0x10: return INSTR { ITYPE; u32 temp = LINK; if( s32(cpu.r[s]) < 0 ) { cpu.branch(DS_REL_ADDR); } cpu.r[31] = temp; }; // BLTZAL
	case 0x11: return INSTR { ITYPE; u32 temp = LINK; if( s32(cpu.r[s]) >= 0 ) { cpu.branch(DS_REL_ADDR); } cpu.r[31] = temp; }; // BGEZAL
	default: printf("RSP: unimpl regimm opc $%X\n", (opcode>>16) & 0x1F); exit(1);
	}
	return nullptr;
}

rsp_instr rsp_regular(n64_rsp& proc, u32 opcode)
{
	switch( opcode>>26 )
	{
	case 0x02: return INSTR { JTYPE; cpu.branch(target); }; // J
	case 0x03: return INSTR { JTYPE; cpu.r[31] = LINK; cpu.branch(target); }; // JAL
	case 0x04: return INSTR { ITYPE; if( cpu.r[s] == cpu.r[t] ) cpu.branch(DS_REL_ADDR); }; // BEQ
	case 0x05: return INSTR { ITYPE; if( cpu.r[s] != cpu.r[t] ) cpu.branch(DS_REL_ADDR); }; // BNE
	case 0x06: return INSTR { ITYPE; if( s32(cpu.r[s]) <= 0 ) cpu.branch(DS_REL_ADDR); }; // BLEZ
	case 0x07: return INSTR { ITYPE; if( s32(cpu.r[s]) > 0 ) cpu.branch(DS_REL_ADDR); }; // BGTZ
	case 0x08: return INSTR { ITYPE; cpu.r[t] = (cpu.r[s] + s16(imm16)); }; // ADDI		
	case 0x09: return INSTR { ITYPE; cpu.r[t] = (cpu.r[s] + s16(imm16)); }; // ADDIU
	case 0x0A: return INSTR { ITYPE; cpu.r[t] = s32(cpu.r[s]) < s32(s16(imm16)); }; // SLTI
	case 0x0B: return INSTR { ITYPE; cpu.r[t] = cpu.r[s] < u32(s16(imm16)); }; // SLTIU
	case 0x0C: return INSTR { ITYPE; cpu.r[t] = cpu.r[s] & imm16; }; // ANDI
	case 0x0D: return INSTR { ITYPE; cpu.r[t] = cpu.r[s] | u32(imm16); }; // ORI
	case 0x0E: return INSTR { ITYPE; cpu.r[t] = cpu.r[s] ^ imm16; }; // XORI
	case 0x0F: return INSTR { ITYPE; cpu.r[t] = imm16<<16; }; // LUI
	case 0x10: // COP0 instructions
		switch( (opcode>>21) & 0x1F )
		{
		case 0: // MFC0
			return INSTR { 
				RTYPE; 
				if( d < 8 ) 
				{ 
					cpu.r[t] = cpu.sp_read(d<<2); 
				} else {
					cpu.r[t] = cpu.dp_read((d-8)<<2);
				}
			};
		case 4: // MTC0
			return INSTR { 
				RTYPE; 
				if( d < 8 )
				{
					cpu.sp_write(d<<2, cpu.r[t]);
				} else {
					cpu.dp_write((d-8)<<2, cpu.r[t]);
				}
			};
		default:
			printf("RSP: Unimpl COP0 opcode = $%X\n", opcode);
			exit(1);
		}
	case 0x11: return INSTR {}; // COP1 / FPU todo
	case 0x12: return rsp_cop2(proc, opcode); // vector ops
	case 0x13: return INSTR {}; // COP3??		

	case 0x20: return INSTR { ITYPE; u8 res = cpu.DMEM[(cpu.r[s]+s16(imm16))&0xfff]; cpu.r[t] = s8(res); }; // LB	
	case 0x21:  // LH
		return INSTR { 
			ITYPE; 
			u32 addr = (cpu.r[s]+s16(imm16))&0xfff;
			u16 res = cpu.DMEM[addr]<<8;
			res |= cpu.DMEM[(addr+1)&0xfff];
			cpu.r[t] = s16(res); 
		};
	case 0x23: // LW
		return INSTR { 
			ITYPE; 
			u32 addr = (cpu.r[s]+s16(imm16))&0xfff;
			u32 res = cpu.DMEM[addr]<<24;
			res |= cpu.DMEM[(addr+1)&0xfff]<<16;
			res |= cpu.DMEM[(addr+2)&0xfff]<<8;
			res |= cpu.DMEM[(addr+3)&0xfff];
			cpu.r[t] = res; 
		};
	case 0x24: return INSTR { ITYPE; u8 res = cpu.DMEM[(cpu.r[s]+s16(imm16))&0xfff]; cpu.r[t] = u8(res); }; // LBU	
	case 0x25: // LHU
		return INSTR { 
			ITYPE; 
			u32 addr = (cpu.r[s]+s16(imm16))&0xfff;
			u16 res = cpu.DMEM[addr]<<8;
			res |= cpu.DMEM[(addr+1)&0xfff];
			cpu.r[t] = u16(res); 
		};
	case 0x27: // LWU (same as LW)
		return INSTR { 
			ITYPE; 
			u32 addr = (cpu.r[s]+s16(imm16))&0xfff;
			u32 res = cpu.DMEM[addr]<<24;
			res |= cpu.DMEM[(addr+1)&0xfff]<<16;
			res |= cpu.DMEM[(addr+2)&0xfff]<<8;
			res |= cpu.DMEM[(addr+3)&0xfff];
			cpu.r[t] = res; 
		};
	case 0x28: return INSTR { ITYPE; cpu.DMEM[(cpu.r[s] + s16(imm16))&0xfff] = cpu.r[t]; }; // SB
	case 0x29: // SH
		return INSTR { 
			ITYPE; 
			u32 addr = (cpu.r[s]+s16(imm16))&0xfff;
			cpu.DMEM[addr] = cpu.r[t]>>8;
			cpu.DMEM[(addr+1)&0xfff] = cpu.r[t];
		};
	
	case 0x2B: // SW
		return INSTR { 
			ITYPE; 
			u32 addr = (cpu.r[s]+s16(imm16))&0xfff;
			cpu.DMEM[addr] = cpu.r[t]>>24;
			cpu.DMEM[(addr+1)&0xfff] = cpu.r[t]>>16;
			cpu.DMEM[(addr+2)&0xfff] = cpu.r[t]>>8;
			cpu.DMEM[(addr+3)&0xfff] = cpu.r[t];
		};
		
	case 0x32: return rsp_lwc2(proc, opcode); // LWC2
		
	case 0x3A: return rsp_swc2(proc, opcode); // SWC2

	default: printf("RSP: unimpl regular opc $%X\n", opcode>>26); exit(1);
	}
}

void n64_rsp::step()
{
	r[0] = 0;
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








