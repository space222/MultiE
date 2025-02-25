#include <print>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "VR4300.h"

#define JTYPE u64 target = ((opc<<2)&0x0fffFFFFu) + (cpu.npc&~0x0fff'ffffull)
#define RTYPE [[maybe_unused]]u32 d = (opc>>11)&0x1f; \
		[[maybe_unused]]u32 s = (opc>>21)&0x1f; [[maybe_unused]]u32 t = (opc>>16)&0x1f; [[maybe_unused]]u32 sa=(opc>>6)&0x1f
#define ITYPE [[maybe_unused]]u32 t = (opc>>16)&0x1f; [[maybe_unused]]u32 s = (opc>>21)&0x1f; \
		[[maybe_unused]]u16 imm16 = opc
#define INSTR [](VR4300& cpu, u32 opc)
#define OVERFLOW32(r, a, b) (((r)^(a))&((r)^(b))&BIT(31))
#define OVERFLOW64(r, a, b) (((r)^(a))&((r)^(b))&BITL(63))
#define LINK cpu.nnpc
#define DS_REL_ADDR (cpu.npc + (s64(s16(imm16))<<2))
#define LIKELY cpu.npc = cpu.nnpc; cpu.nnpc += 4;

typedef void(*vr4300_instr) (VR4300&, u32);
static vr4300_instr vr4300_icache[0x800000>>2] = {0};

vr4300_instr cop1(VR4300&, u32);

vr4300_instr decode_special(VR4300& proc, u32 opcode)
{
	switch( opcode & 0x3F )
	{
	case 0x00: return INSTR { RTYPE; cpu.r[d] = s32(cpu.r[t] << sa); }; // SLL
	case 0x02: return INSTR { RTYPE; cpu.r[d] = s32(u32(cpu.r[t]) >> sa); }; // SRL
	case 0x03: return INSTR { RTYPE; cpu.r[d] = s32(cpu.r[t] >> sa); }; // SRA
	case 0x04: return INSTR { RTYPE; cpu.r[d] = s32(cpu.r[t] << (cpu.r[s]&0x1F)); }; // SLLV
	
	case 0x06: return INSTR { RTYPE; cpu.r[d] = s32(u32(cpu.r[t]) >> (cpu.r[s]&0x1F)); }; // SRLV
	case 0x07: return INSTR { RTYPE; cpu.r[d] = s32(cpu.r[t] >> (cpu.r[s]&0x1F)); }; // SRAV
	
	case 0x08: return INSTR { RTYPE; cpu.branch(cpu.r[s]); }; // JR
	case 0x09: return INSTR { RTYPE; u64 temp = cpu.r[s]; cpu.r[d] = LINK; cpu.branch(temp); }; // JALR
	
	case 0x0C: return [](VR4300& cpu, u32) { cpu.exception(8); }; // SYSCALL
	case 0x0D: return [](VR4300& cpu, u32) { cpu.exception(9); }; // BREAK
	
	case 0x0F: return [](VR4300&, u32) {}; // SYNC
	case 0x10: return INSTR { RTYPE; cpu.r[d] = cpu.hi; }; // MFHI
	case 0x11: return INSTR { RTYPE; cpu.hi = cpu.r[s]; }; // MTHI
	case 0x12: return INSTR { RTYPE; cpu.r[d] = cpu.lo; }; // MFLO
	case 0x13: return INSTR { RTYPE; cpu.lo = cpu.r[s]; }; // MTLO
	case 0x14: return INSTR { RTYPE; cpu.r[d] = cpu.r[t] << (cpu.r[s]&0x3F); }; // DSLLV

	case 0x16: return INSTR { RTYPE; cpu.r[d] = cpu.r[t] >> (cpu.r[s]&0x3F); }; // DSRLV	
	case 0x17: return INSTR { RTYPE; cpu.r[d] = s64(cpu.r[t]) >> (cpu.r[s]&0x3F); }; // DSRAV
	case 0x18: // MULT
		return INSTR
		{
			ITYPE;
			s64 a = s32(cpu.r[s]);
			a *= s32(cpu.r[t]); //s64(cpu.r[t]<<30)>>30;
			cpu.hi = s32(u32(a>>32));
			cpu.lo = s32(u32(a));
		};
	case 0x19: // MULTU
		return INSTR
		{
			ITYPE;
			u64 a = u32(cpu.r[s]);
			a *= u32(cpu.r[t]);
			cpu.hi = s32(a>>32);
			cpu.lo = s32(a);
		};
	
	/* // from a quick search on discord for "div overflow"
	DIV:
	+number by 0: LO = -1, HI = rs
	-number by 0: LO = 1, HI = rs
	-0x80000000 by -1: LO = -0x80000000, HI = 0

	DIVU:
	by 0: LO = -1, HI = rs
	*/
	
	case 0x1A: // DIV
		return INSTR
		{
			ITYPE;
			s32 a = cpu.r[s];
			s32 b = cpu.r[t];
			if( b == 0 )
			{
				cpu.lo = (a&BIT(31))?1:-1;
				cpu.hi = cpu.r[s];
			} else if( u32(cpu.r[s]) == 0x80000000u && u32(cpu.r[t]) == 0xffffFFFFu) {
				cpu.lo = a;
				cpu.hi = 0;
			} else {
				cpu.lo = a / b;
				cpu.hi = a % b;
			}		
		};	
	case 0x1B: // DIVU
		return INSTR
		{
			ITYPE;
			u32 a = cpu.r[s];
			u32 b = cpu.r[t];
			if( b == 0 )
			{
				cpu.lo = -1;
				cpu.hi = cpu.r[s];
			} else {
				cpu.lo = s32(a / b);
				cpu.hi = s32(a % b);
			}		
		};
	case 0x1C: return INSTR { RTYPE; __int128_t a = s64(cpu.r[t]); a *= s64(cpu.r[s]); cpu.hi = u64(a>>64); cpu.lo = a; }; // DMULT
	case 0x1D: return INSTR { RTYPE; __uint128_t a = cpu.r[t]; a *= cpu.r[s]; cpu.hi = u64(a>>64); cpu.lo = a; }; // DMULTU
	case 0x1E: // DDIV
		return INSTR 
		{
			ITYPE;
			s64 a = cpu.r[s];
			s64 b = cpu.r[t];
			if( b == 0 )
			{
				cpu.lo = (a<0)?1:-1;
				cpu.hi = cpu.r[s];
			} else if( cpu.r[s] == 0x8000'0000'0000'0000ull && cpu.r[t] == 0xffffFFFFffffFFFFull ) {
				cpu.hi = 0;
				cpu.lo = a;
			} else {
				cpu.lo = a / b;
				cpu.hi = a % b;			
			}
		};
	case 0x1F: // DDIVU
		return INSTR 
		{
			ITYPE;
			u64 a = cpu.r[s];
			u64 b = cpu.r[t];
			if( b == 0 )
			{
				cpu.lo = -1;
				cpu.hi = cpu.r[s];
			} else {
				cpu.lo = a / b;
				cpu.hi = a % b;			
			}		
		};
	case 0x20: // ADD
		return INSTR { 
			RTYPE; 
			s32 v = s32(cpu.r[t])+s32(cpu.r[s]);
			if( OVERFLOW32(v, s32(cpu.r[t]), s32(cpu.r[s])) ) 
			{ 
				cpu.overflow(); 
			} else { 
				cpu.r[d] = v;
			}
		};
	case 0x21: return INSTR { RTYPE; cpu.r[d] = s32(cpu.r[t]) + s32(cpu.r[s]); }; // ADDU
	case 0x22: // SUB
		return INSTR { 
			RTYPE; 
			s32 v = s32(cpu.r[s]) - s32(cpu.r[t]); 
			if( OVERFLOW32(v, u32(cpu.r[s]), ~u32(cpu.r[t])) ) 
			{ 
				cpu.overflow(); 
			} else { 
				cpu.r[d] = v;
			}
		};
	case 0x23: return INSTR { RTYPE; cpu.r[d] = s32(cpu.r[s]) - s32(cpu.r[t]); }; // SUBU
	case 0x24: return INSTR { RTYPE; cpu.r[d] = cpu.r[t] & cpu.r[s]; }; // AND
	case 0x25: return INSTR { RTYPE; cpu.r[d] = cpu.r[t] | cpu.r[s]; }; // OR
	case 0x26: return INSTR { RTYPE; cpu.r[d] = cpu.r[t] ^ cpu.r[s]; }; // XOR
	case 0x27: return INSTR { RTYPE; cpu.r[d] = ~(cpu.r[t] | cpu.r[s]); }; // NOR

	case 0x2A: return INSTR { RTYPE; cpu.r[d] = s64(cpu.r[s]) < s64(cpu.r[t]); }; // SLT
	case 0x2B: return INSTR { RTYPE; cpu.r[d] = cpu.r[s] < cpu.r[t]; }; // SLTU
	case 0x2C: // DADD
		return INSTR { 
			RTYPE; 
			u64 v = cpu.r[s] + cpu.r[t];
			if( OVERFLOW64(v, cpu.r[s], cpu.r[t]) )
			{
				cpu.overflow();
			} else {
				cpu.r[d] = v;
			}	
		};
	case 0x2D: return INSTR { RTYPE; cpu.r[d] = cpu.r[t] + cpu.r[s]; }; // DADDU
	case 0x2E: // DSUB
		return INSTR { 
			RTYPE; 
			u64 v = cpu.r[s] - cpu.r[t];
			if( OVERFLOW64(v, cpu.r[s], ~cpu.r[t]) )
			{
				cpu.overflow();
			} else {
				cpu.r[d] = v; 
			}
		};
	case 0x2F: return INSTR { RTYPE; cpu.r[d] = cpu.r[s] - cpu.r[t]; }; // DSUBU
	
	case 0x30: return INSTR { RTYPE; if( s64(cpu.r[s]) >= s64(cpu.r[t]) ) { cpu.exception(13); } }; // TGE
	case 0x31: return INSTR { RTYPE; if( cpu.r[s] >= cpu.r[t] ) { cpu.exception(13); } }; // TGEU
	case 0x32: return INSTR { RTYPE; if( s64(cpu.r[s]) < s64(cpu.r[t]) ) { cpu.exception(13); } }; // TLT
	case 0x33: return INSTR { RTYPE; if( cpu.r[s] < cpu.r[t] ) { cpu.exception(13); } }; // TLTU
	case 0x34: return INSTR { RTYPE; if( cpu.r[s] == cpu.r[t] ) { cpu.exception(13); } }; // TEQ

	case 0x36: return INSTR { RTYPE; if( cpu.r[s] != cpu.r[t] ) { cpu.exception(13); } }; // TNE

	case 0x38: return INSTR { RTYPE; cpu.r[d] = cpu.r[t] << sa; }; // DSLL
	
	case 0x3A: return INSTR { RTYPE; cpu.r[d] = cpu.r[t] >> sa; }; // DSRL
	case 0x3B: return INSTR { RTYPE; cpu.r[d] = s64(cpu.r[t]) >> sa; }; // DSRA
	case 0x3C: return INSTR { RTYPE; cpu.r[d] = cpu.r[t] << (sa+32); }; // DSLL32
	
	case 0x3E: return INSTR { RTYPE; cpu.r[d] = cpu.r[t] >> (sa+32); }; // DSRL32
	case 0x3F: return INSTR { RTYPE; cpu.r[d] = s64(cpu.r[t]) >> (sa+32); }; // DSRA32
	default: printf("$%lX: Unimpl special opcode = $%X\n", proc.pc, (opcode & 0x3F)); exit(1);
	}
	return nullptr;
}

vr4300_instr decode_regimm(VR4300&, u32 opcode)
{
	switch( (opcode>>16) & 0x1F )
	{
	case 0x00: return INSTR { ITYPE; if( s64(cpu.r[s]) < 0 ) { cpu.branch(DS_REL_ADDR); } }; // BLTZ
	case 0x01: return INSTR { ITYPE; if( s64(cpu.r[s]) >= 0 ) { cpu.branch(DS_REL_ADDR); } }; // BGEZ
	case 0x02: // BLTZL
		return INSTR 
		{
			ITYPE;
			if( s64(cpu.r[s]) < 0 )
			{
				cpu.branch(DS_REL_ADDR);
			} else {
				LIKELY;
			}
		};
	case 0x03: // BGEZL
		return INSTR 
		{
			ITYPE;
			if( s64(cpu.r[s]) >= 0 )
			{
				cpu.branch(DS_REL_ADDR);
			} else {
				LIKELY;
			}
		};

	case 0x08: return INSTR { ITYPE; if( s64(cpu.r[s]) >= s64(s16(imm16)) ) { cpu.exception(13); } }; // TGEI
	case 0x09: return INSTR { ITYPE; if( cpu.r[s] >= (u64)s64(s16(imm16)) ) { cpu.exception(13); } }; // TGEIU
	case 0x0A: return INSTR { ITYPE; if( s64(cpu.r[s]) < s64(s16(imm16)) ) { cpu.exception(13); } }; // TLTI
	case 0x0B: return INSTR { ITYPE; if( cpu.r[s] < (u64)s64(s16(imm16)) ) { cpu.exception(13); } }; // TLTIU
	case 0x0C: return INSTR { ITYPE; if( s64(cpu.r[s]) == s64(s16(imm16)) ) { cpu.exception(13); } }; // TEQI

	case 0x0E: return INSTR { ITYPE; if( s64(cpu.r[s]) != s64(s16(imm16)) ) { cpu.exception(13); } }; // TNEI
		
	case 0x10: return INSTR { ITYPE; u64 temp = LINK; if( s64(cpu.r[s]) < 0 ) { cpu.branch(DS_REL_ADDR); } cpu.r[31] = temp; }; // BLTZAL
	case 0x11: return INSTR { ITYPE; u64 temp = LINK; if( s64(cpu.r[s]) >= 0 ) { cpu.branch(DS_REL_ADDR); } cpu.r[31] = temp; }; // BGEZAL
	case 0x12: // BLTZALL
		return INSTR 
		{
			ITYPE;
			u64 temp = LINK;
			if( s64(cpu.r[s]) < 0 )
			{
				cpu.branch(DS_REL_ADDR);
			} else {
				LIKELY;
			}
			cpu.r[31] = temp;		
		};
	case 0x13: // BGEZALL
		return INSTR 
		{
			ITYPE;
			u64 temp = LINK;
			if( s64(cpu.r[s]) >= 0 )
			{
				cpu.branch(DS_REL_ADDR);
			} else {
				LIKELY;
			}
			cpu.r[31] = temp;		
		};
	default: printf("VR4300: unimpl regimm opc $%X\n", (opcode>>16) & 0x1F); exit(1);
	}
	return nullptr;
}

vr4300_instr decode_regular(VR4300& proc, u32 opcode)
{
	switch( opcode>>26 )
	{
	case 0x02:  // J
		return INSTR 
		{ 
			JTYPE;
			if( u32(target) == u32(cpu.pc) )
			{
				cpu.in_infinite_loop = (cpu.read(cpu.pc+4, 32) == 0);
			}
			cpu.branch(target); 
		};
	case 0x03: return INSTR { JTYPE; cpu.r[31] = LINK; cpu.branch(target); }; // JAL
	case 0x04: // BEQ
		return INSTR 
		{
			ITYPE; 
			cpu.ndelay = true;
			if( s == 0 && t == 0 && cpu.read(cpu.pc+4, 32) == 0 ) cpu.in_infinite_loop = true;
			if( cpu.r[s] == cpu.r[t] ) cpu.branch(DS_REL_ADDR); 
		};
	case 0x05: return INSTR { ITYPE; cpu.ndelay = true; if( cpu.r[s] != cpu.r[t] ) cpu.branch(DS_REL_ADDR); }; // BNE
	case 0x06: return INSTR { ITYPE; cpu.ndelay = true; if( s64(cpu.r[s]) <= 0 ) cpu.branch(DS_REL_ADDR); }; // BLEZ
	case 0x07: return INSTR { ITYPE; cpu.ndelay = true; if( s64(cpu.r[s]) > 0 ) cpu.branch(DS_REL_ADDR); }; // BGTZ
	case 0x08: // ADDI
		return INSTR 
		{ 
			ITYPE; 
			s32 temp = s16(imm16); 
			s32 v = s32(cpu.r[s]) + s16(imm16); 
			if( OVERFLOW32(v, u32(cpu.r[s]), temp) )
			{
				cpu.overflow();
			} else {
				cpu.r[t] = v;
			}			
		};
	case 0x09: return INSTR { ITYPE; cpu.r[t] = s32(cpu.r[s]) + s16(imm16); }; // ADDIU
	case 0x0A: return INSTR { ITYPE; cpu.r[t] = s64(cpu.r[s]) < s64(s16(imm16)); }; // SLTI
	case 0x0B: return INSTR { ITYPE; cpu.r[t] = cpu.r[s] < u64(s16(imm16)); }; // SLTIU
	case 0x0C: return INSTR { ITYPE; cpu.r[t] = cpu.r[s] & u64(u16(imm16)); }; // ANDI
	case 0x0D: return INSTR { ITYPE; cpu.r[t] = cpu.r[s] | u64(u16(imm16)); }; // ORI
	case 0x0E: return INSTR { ITYPE; cpu.r[t] = cpu.r[s] ^ u64(u16(imm16)); }; // XORI
	case 0x0F: return INSTR { ITYPE; cpu.r[t] = s32(imm16<<16); }; // LUI
	case 0x10: // COP0 instructions
		switch( (opcode>>21) & 0x1F )
		{
		case 0: return INSTR { RTYPE; cpu.r[t] = s32(u32(cpu.c0_read32(d))); }; // MFC0
		case 1: return INSTR { RTYPE; cpu.r[t] = cpu.c0_read64(d); }; // DMFC0
		case 4: return INSTR { RTYPE; cpu.c0_write32(d, cpu.r[t]); }; // MTC0
		case 5: return INSTR { RTYPE; cpu.c0_write64(d, cpu.r[t]); }; // DMTC0	
		default:
			if( opcode == 0b01000010000000000000000000011000 )
			{  // ERET
				return [](VR4300& cpu, u32) 
				{
					cpu.LLbit = false;
					if( cpu.STATUS & cpu.STATUS_ERL )
					{
						cpu.npc = s32(cpu.ErrorEPC);
					} else {
						cpu.npc = s32(cpu.EPC);
					}
					//printf("ERET to $%lX\n", cpu.EPC);
					cpu.nnpc = cpu.npc + 4;
					cpu.STATUS &= ~(cpu.STATUS_ERL|cpu.STATUS_EXL);
				};
			}
			if( opcode == 0b0100'0010'0000'0000'0000'0000'0000'1000 )
			{  // TLBP
				return [](VR4300& cpu, u32)
				{
					//std::println("TLBP");	
					u32 p = 0;
					if(int ent = cpu.tlb_search(cpu.ENTRY_HI&~0xff, p); ent >= 0)
					{
						cpu.INDEX = ent;
					} else {
						cpu.INDEX |= BIT(31);
					}
				};			
			}
			if( opcode == 0b0100'0010'0000'0000'0000'0000'0000'0001 )
			{  // TLBR
				return [](VR4300& cpu, u32)
				{
					//std::println("TLBR");	
					auto& en = cpu.tlb[cpu.INDEX&0x1F];
					cpu.PAGE_MASK = en.mask;
					cpu.ENTRY_HI = en.vpn | en.asid;
					cpu.ENTRY_LO0 = en.page0 | en.G | en.C0 | en.D0 | en.V0;
					cpu.ENTRY_LO1 = en.page1 | en.G | en.C1 | en.D1 | en.V1;			
				};
			}
			if( opcode == 0b0100'0010'0000'0000'0000'0000'0000'0010 )
			{  // TLBWI
				return [](VR4300& cpu, u32)
				{
					auto& en = cpu.tlb[cpu.INDEX&0x1F];  // masked to 32?
					en.mask = cpu.PAGE_MASK & 0xFFE000;
					en.asid = cpu.ENTRY_HI & 0xff;
					en.vpn  = cpu.ENTRY_HI & ~0x1fff;
					en.page0 = cpu.ENTRY_LO0 & 0x1fffffc0;
					en.page1 = cpu.ENTRY_LO1 & 0x1fffffc0;
					en.G = cpu.ENTRY_LO0 & cpu.ENTRY_LO1 & 1;
					en.C0 = cpu.ENTRY_LO0 & 0x38;
					en.C1 = cpu.ENTRY_LO1 & 0x38;
					en.D0 = cpu.ENTRY_LO0 & 4;
					en.D1 = cpu.ENTRY_LO1 & 4; 
					en.V0 = cpu.ENTRY_LO0 & 2;
					en.V1 = cpu.ENTRY_LO1 & 2;
					/*if( en.V0 )
					{
						std::println("${:X} even translates to ${:X}", en.vpn, en.page0<<6);
					}
					if( en.V1 )
					{
						std::println("${:X} odd translates to ${:X}", en.vpn, en.page1<<6);
					}*/
				};			
			}
			if( opcode == 0b0100'0010'0000'0000'0000'0000'0000'0110 )
			{  // TLBWR
				return [](VR4300& cpu, u32)
				{
					//std::println("TLBWR");
					auto& en = cpu.tlb[cpu.RANDOM&0x1F];  // masked to 32?
					en.mask = cpu.PAGE_MASK & 0xFFE000;
					en.asid = cpu.ENTRY_HI & 0xff;
					en.vpn  = cpu.ENTRY_HI & ~0x1fff;
					en.page0 = cpu.ENTRY_LO0 & 0x1fffffc0;
					en.page1 = cpu.ENTRY_LO1 & 0x1fffffc0;
					en.G = cpu.ENTRY_LO0 & cpu.ENTRY_LO1 & 1;
					en.C0 = cpu.ENTRY_LO0 & 0x38;
					en.C1 = cpu.ENTRY_LO1 & 0x38;
					en.D0 = cpu.ENTRY_LO0 & 4;
					en.D1 = cpu.ENTRY_LO1 & 4; 
					en.V0 = cpu.ENTRY_LO0 & 2;
					en.V1 = cpu.ENTRY_LO1 & 2; 
				};			
			}
			printf("VR4300: Unimpl COP0 opcode = $%X\n", opcode);
			//return [](VR4300&, u32) {};
			exit(1);
		}
	case 0x11: return cop1(proc, opcode); // COP1 / FPU todo
	case 0x12: return [](VR4300& cpu, u32) { if( cpu.COPUnusable(2) ) return; }; // COP2??		
	case 0x13: return [](VR4300& cpu, u32) { cpu.exception(10); }; // COP3??		
	
	case 0x14:  // BEQL 
		return INSTR {
			ITYPE;
			if( cpu.r[s] == cpu.r[t] ) 
			{ 
				cpu.branch(DS_REL_ADDR); 
			} else { 
				LIKELY;
			}
		};
	case 0x15:  // BNEL 
		return INSTR {
			ITYPE;
			if( cpu.r[s] != cpu.r[t] ) 
			{ 
				cpu.branch(DS_REL_ADDR); 
			} else { 
				LIKELY;
			}
		};
	case 0x16: // BLEZL
		return INSTR {
			ITYPE;
			if( s64(cpu.r[s]) <= 0 )
			{
				cpu.branch(DS_REL_ADDR);
			} else {
				LIKELY;
			}
		};
	case 0x17: // BGTZL
		return INSTR {
			ITYPE;
			if( s64(cpu.r[s]) > 0 )
			{
				cpu.branch(DS_REL_ADDR);
			} else {
				LIKELY;
			}
		};
	case 0x18:  // DADDI
		return INSTR 
		{ 
			ITYPE; 
			u64 temp = s64(s16(imm16)); 
			u64 v = cpu.r[s] + temp; 
			if( OVERFLOW64(v, cpu.r[s], temp) )
			{
				cpu.overflow();
			} else {
				cpu.r[t] = v;
			}			
		};
	case 0x19: return INSTR { ITYPE; cpu.r[t] = cpu.r[s] + s16(imm16); }; // DADDIU
	
	case 0x1A: // LDL
		return INSTR
		{
			ITYPE;
			const u64 addr = cpu.r[s] + s16(imm16);
			auto res = cpu.read(addr&~7, 64);
			if( ! res )
			{	//todo: address for exception might need to be the full unaligned value
				return;
			}
			const u32 shift = (addr&7)<<3;
			cpu.r[t] &= (1ull<<shift)-1;
			cpu.r[t] |= res<<shift;
		};
	case 0x1B: // LDR
		return INSTR
		{
			ITYPE;
			const u64 addr = cpu.r[s] + s16(imm16);
			auto res = cpu.read(addr&~7, 64);
			if( ! res )
			{	//todo: address for exception might need to be the full unaligned value
				return;
			}
			cpu.r[t] &= (~((1ull<<(((addr&7))*8))-1))<<8;
			cpu.r[t] |= res>>((7-(addr&7))<<3);
		};
	
	case 0x20: return INSTR { ITYPE; auto res = cpu.read(cpu.r[s] + s16(imm16), 8); if( !res ) return; cpu.r[t] = s8(res); }; // LB	
	case 0x21: return INSTR { ITYPE; auto res = cpu.read(cpu.r[s] + s16(imm16), 16); if( !res ) return; cpu.r[t] = s16(res); }; // LH
	case 0x22: // LWL
		return INSTR
		{
			ITYPE;
			const u64 addr = cpu.r[s] + s16(imm16);
			auto res = cpu.read(addr&~3, 32);
			if( ! res )
			{	//todo: address for exception might need to be the full unaligned value
				return;
			}
			const u32 shift = (addr&3)<<3;
			cpu.r[t] &= (1ull<<shift)-1;
			cpu.r[t] |= res<<shift;
			cpu.r[t] = s32(cpu.r[t]);
		};	
	case 0x23: return INSTR { ITYPE; auto res = cpu.read(cpu.r[s]+s16(imm16), 32); if(!res) { return; } cpu.r[t] = s32(res); }; // LW
	case 0x24: return INSTR { ITYPE; auto res = cpu.read(cpu.r[s] + s16(imm16), 8); if( !res ) return; cpu.r[t] = u8(res); }; // LBU	
	case 0x25: return INSTR { ITYPE; auto res = cpu.read(cpu.r[s] + s16(imm16), 16); if( !res ) return; cpu.r[t] = u16(res); }; // LHU
	case 0x26: // LWR
		return INSTR
		{
			ITYPE;
			const u64 addr = cpu.r[s] + s16(imm16);
			auto res = cpu.read(addr&~3, 32);
			if( ! res )
			{	//todo: address for exception might need to be the full unaligned value
				return;
			}
			cpu.r[t] &= ~((1ull<<(((addr&3)+1)*8))-1);
			cpu.r[t] |= res>>((3-(addr&3))<<3);
			cpu.r[t] = s32(cpu.r[t]);
		};
	case 0x27: return INSTR { ITYPE; auto res = cpu.read(cpu.r[s] + s16(imm16), 32); if( !res ) return; cpu.r[t] = u32(res); }; // LWU
	
	case 0x28: return INSTR { ITYPE; cpu.write(cpu.r[s] + s16(imm16), cpu.r[t], 8); }; // SB
	case 0x29: return INSTR { ITYPE; cpu.write(cpu.r[s] + s16(imm16), cpu.r[t], 16); }; // SH
	case 0x2A: // SWL
		//todo: there's only 1 actual bus transaction in these
		//	will need to do a more raw, tlb translation + physical access here
		return INSTR
		{
			ITYPE;
			u64 addr = cpu.r[s] + s16(imm16);
			auto mem = cpu.read(addr&~3, 32);
			if( !mem ) return;
			cpu.write(addr&~3, (u32(cpu.r[t]) >> ((addr&3)*8)) | (u32(mem) & ~((1ull<<(32-((addr&3)*8)))-1)), 32);
		};
	case 0x2B: return INSTR { ITYPE; cpu.write(cpu.r[s] + s16(imm16), cpu.r[t], 32); }; // SW
	case 0x2C: // SDL
		//todo: there's only 1 actual bus transaction in these
		//	will need to do a more raw, tlb translation + physical access here
		return INSTR
		{
			ITYPE;
			u64 addr = cpu.r[s] + s16(imm16);
			auto mem = cpu.read(addr&~7, 64);
			if( !mem ) return;
			const u64 mask = (addr&7) ? ~((1ull<<(64-((addr&7)*8)))-1) : 0;
			cpu.write(addr&~7, ((cpu.r[t] >> ((addr&7)*8))) | (mem & mask), 64);
		};
	case 0x2D: // SDR
		//todo: there's only 1 actual bus transaction in these
		//	will need to do a more raw, tlb translation + physical access here
		return INSTR
		{
			ITYPE;
			u64 addr = cpu.r[s] + s16(imm16);
			auto mem = cpu.read(addr&~7, 64);
			if( !mem ) return;
			const u32 shift = (56-((addr&7)*8));
			cpu.write(addr&~7, (mem&((1ull<<shift)-1)) | (cpu.r[t]<<shift), 64);
		};
	case 0x2E: // SWR
		//todo: there's only 1 actual bus transaction in these
		//	will need to do a more raw, tlb translation + physical access here
		return INSTR
		{
			ITYPE;
			u64 addr = cpu.r[s] + s16(imm16);
			auto mem = cpu.read(addr&~3, 32);
			if( !mem ) return;
			const u32 shift = (24-((addr&3)*8));
			cpu.write(addr&~3, (mem&((1ull<<shift)-1)) | (u32(cpu.r[t])<<shift), 32);
		};
	case 0x2F: return [](VR4300&, u32) { /*printf("VR4300: cache instruction\n");*/ }; // CACHE not implemented
	case 0x30: // LL
		return INSTR
		{
			ITYPE;
			auto res = cpu.read(cpu.r[s]+s16(imm16), 32);
			if( !res ) { return; }
			cpu.LL_ADDR = (cpu.r[s] + s16(imm16)) & 0x1fffFFFF;
			cpu.LL_ADDR >>= 4;
			cpu.LLbit = true;
			cpu.r[t] = s32(res);			
		};

	case 0x31: // LWC1
		return INSTR {
			ITYPE;
			if( cpu.COPUnusable(1) ) return;
			auto res = cpu.read(cpu.r[s]+s16(imm16), 32);
			if( !res ) { return; }
			u32 a = res;
			if( !(cpu.STATUS & BIT(26)) )
			{
				t = ((t&~1)<<3) + ((t&1)?4:0);
			} else { 
				t <<= 3;
			}
			memcpy(&cpu.f[t], &a, 4);
		};

	case 0x34: // LLD
		return INSTR
		{
			ITYPE;
			auto res = cpu.read(cpu.r[s]+s16(imm16), 64);
			if( !res ) { return; }
			cpu.LL_ADDR = (cpu.r[s] + s16(imm16)) & 0x1fffFFFF;
			cpu.LL_ADDR >>= 4;
			cpu.LLbit = true;
			cpu.r[t] = res;			
		};
	case 0x35: // LDC1
		return INSTR {
			ITYPE;
			if( cpu.COPUnusable(1) ) return;
			auto res = cpu.read(cpu.r[s]+s16(imm16), 64);
			if( !res ) { return; }
			u64 a = res;
			if( !(cpu.STATUS & BIT(26)) ) t &= ~1;
			memcpy(&cpu.f[t<<3], &a, 8);
		};
	case 0x37: // LD
		return INSTR 
		{ 
			ITYPE; 
			auto res = cpu.read(cpu.r[s] + s16(imm16), 64); 
			if( !res ) return; 
			cpu.r[t] = res; 
		};
	case 0x38: // SC
		return INSTR
		{
			ITYPE;
			if( cpu.LLbit )
			{
				cpu.write(cpu.r[s] + s16(imm16), cpu.r[t], 32);
				cpu.r[t] = 1;			
			} else {
				cpu.r[t] = 0;
			}		
		};
	case 0x39: // SWC1
		return INSTR {
			ITYPE;
			if( cpu.COPUnusable(1) ) return;
			u32 a = 0;
			if( !(cpu.STATUS & BIT(26)) )
			{
				t = ((t&~1)<<3) + ((t&1)?4:0);
			} else { 
				t <<= 3;
			}
			memcpy(&a, &cpu.f[t], 4);
			cpu.write(cpu.r[s] + s16(imm16), a, 32);
		};
	case 0x3C: // SCD
		return INSTR
		{
			ITYPE;
			if( cpu.LLbit )
			{
				cpu.write(cpu.r[s] + s16(imm16), cpu.r[t], 64);
				cpu.r[t] = 1;			
			} else {
				cpu.r[t] = 0;
			}		
		};
	case 0x3D: // SDC1
		return INSTR {
			ITYPE;
			if( cpu.COPUnusable(1) ) return;
			u64 a = 0;
			if( !(cpu.STATUS & BIT(26)) ) t &= ~1;
			memcpy(&a, &cpu.f[t<<3], 8);
			cpu.write(cpu.r[s] + s16(imm16), a, 64);
		};
	
	case 0x3F: return INSTR { ITYPE; cpu.write(cpu.r[s] + s16(imm16), cpu.r[t], 64); }; // SD
	default: printf("VR4300: unimpl regular opc $%X\n", opcode>>26); exit(1);
	}
	return nullptr;
}

int countdiv = 0;

void VR4300::step()
{
	r[0] = 0;
	delay = ndelay;
	ndelay = false;
	in_infinite_loop = false;
	BusResult opc;
	
	{
		u32 rd = (RANDOM&0x1f)-1;
		rd &= 0x1f;
		if( rd == WIRED ) rd = 31;
		RANDOM = (RANDOM&0x20)|rd;
	}
	if( ((STATUS&7)==1) && (STATUS & CAUSE & 0xff00) )
	{       // exception() will set npc
		//printf("interrupt: mask=$%X, intr=$%X\n", mimask, miirq);
		exception(0);
	} else {
		//std::println(stderr, "${:X}", u32(pc));
		if( u32(pc) >= 0x80000000u && u32(pc) < 0x80800000u )
		{
			if( pc & 3 )
			{
				BADVADDR = pc;
				CONTEXT &= ~0x7fFFf0;
				CONTEXT |= (u32(pc)>>9);
				CONTEXT &= ~15;
				XCONTEXT = (pc>>9);
				XCONTEXT &= 0x1fffffff0ull;
				address_error(false);
			} else {
				vr4300_icache[(pc&0x7ffffc)>>2](*this, __builtin_bswap32(*(u32*)&RAM[pc&0x7ffffc]));		
			}
		} else 
		if( !(opc = read(pc, 32)) ) {
			// if an exception happened on opcode fetch, exception() will already have been called
			// nothing else to be done here other than skip to pc pipeline advance
			//printf("Exception reading opcode from $%X\n", u32(pc));
			//exit(1);
		} else {
			if( (opc >> 26) == 0 )
			{
				decode_special(*this, opc)(*this, opc);
			} else if( (opc >> 26) == 1 ) {
				decode_regimm(*this, opc)(*this, opc);	
			} else {
				decode_regular(*this, opc)(*this, opc);
			}
		}
		countdiv ^= 1;
		if( countdiv & 1 ) 
		{
			COUNT = (COUNT+1)&0xffffFFFFull;
		}
		if( u32(COUNT) == u32(COMPARE) )
		{
			CAUSE |= BIT(15);
		}	
	}
	//if( u32(pc) >= 0x80300000 && u32(pc) < 0x80400000 )
	//{
	//	printf("pc = $%X\n", u32(pc));
	//}
	
	pc = npc;
	npc = nnpc;
	nnpc += 4;
}

void VR4300::branch(u64 target)
{
	nnpc = target;
	ndelay = true;
}

void VR4300::overflow()
{
	printf("overflow\n");
	//exit(1);
	exception(12);
}

void VR4300::address_error(bool write)
{
	printf("$%X: ae\n", u32(pc));
	//exit(1);
	exception(write ? 5 : 4);
}

void VR4300::exception(u32 ec, u32 vector)
{
	CAUSE &= ~(BIT(31)|0xff);
	if( delay )
	{
		CAUSE |= BIT(31);
		EPC = pc - 4;
		delay = ndelay = false;
	} else {
		EPC = pc;
	}
	CAUSE |= ec<<2;
	STATUS |= STATUS_EXL;
	
	if( ec != 11 ) CAUSE &= ~(BIT(28)|BIT(29));
	
	//printf("Exception! code = %i\n", ec);
	//printf("EPC = $%lX\n", EPC);
	//printf("CAUSE = $%X\n", u32(CAUSE));
	npc = s32(vector);
	nnpc = npc + 4;
}

void VR4300::reset()
{
	for(u32 i = 0; i < 32; ++i) c[i] = r[i] = 0;
	STATUS = 0x34000000;
	cop1_half_mode = !(STATUS & BIT(26));
	c[15] = 0x00000B22;
	RANDOM = 31;
	CONFIG = 0x7006e463;
	//COMPARE = 0x1fff0000;
	FCSR = 0;
	fpu_cond = false;
	
	pc = 0xbfc00000;
	npc = pc + 4;
	nnpc = npc + 4;
	delay = ndelay = LLbit = false;
	
	for(u32 i = 0; i < 0x800000; i+=4)
	{
		invalidate(i);
	}
	
	in_infinite_loop = false;
}

void VR4300::invalidate(u32 pa)
{
	vr4300_icache[(pa&0x7fffff)>>2] = 
		[](VR4300& cpu, u32 opc) 
		{ 
			vr4300_instr INS = nullptr;
			if( (opc >> 26) == 0 )
			{
				INS = decode_special(cpu, opc);
			} else if( (opc >> 26) == 1 ) {
				INS = decode_regimm(cpu, opc);
			} else {
				INS = decode_regular(cpu, opc);
			}
			vr4300_icache[(cpu.pc&0x7fffff)>>2] = INS;
			INS(cpu, opc);
		};
}

BusResult VR4300::read(u64 addr, int size)
{
	if( (size == 32 && (addr&3)) || (size == 16 && (addr&1)) || (size == 64 && (addr&7)) ) 
	{
		BADVADDR = addr;
		CONTEXT &= ~0x7fFFf0;
		CONTEXT |= (u32(addr)>>9);
		CONTEXT &= ~15;
		XCONTEXT = (addr>>9);
		XCONTEXT &= 0x1fffffff0ull;
		address_error(false); 
		return BusResult::exception(1); 
	}
	if( u32(addr) < 0x8000'0000u || u32(addr) >= 0xc000'0000u )
	{
		u32 p = 0;
		if( int ent = tlb_search(addr, p); ent >= 0 )
		{
			addr = p;
		} else {
			BADVADDR = addr;
			CONTEXT &= ~0x7fFFf0;
			CONTEXT |= (u32(addr)>>9);
			CONTEXT &= ~15;
			XCONTEXT = (addr>>9);
			XCONTEXT &= 0x1fffffff0ull;
			ENTRY_HI = (ENTRY_HI&0xff) | (addr&~0xfff);
			exception(2, (ent==-1) ? 0x80000000u : 0x80000180u);
			return BusResult::exception(1); 
		}
		//addr -= 0x3C70000; // saves the mario64 head from crashing. really just need a tlb.
	}
	u32 phys_addr = addr&0x1FFFffff;
	return readers[phys_addr>>12](phys_addr, size);
	//return phys_read(phys_addr, size);
}

BusResult VR4300::write(u64 addr, u64 v, int size)
{
	if( (size == 32 && (addr&3)) || (size == 16 && (addr&1)) || (size == 64 && (addr&7)) ) 
	{
		BADVADDR = addr;
		CONTEXT &= ~0x7fFFf0;
		CONTEXT |= (u32(addr)>>9);
		CONTEXT &= ~15;
		XCONTEXT = (addr>>9);
		XCONTEXT &= 0x1fffffff0ull;
		address_error(true); 
		return BusResult::exception(1); 
	}
	if( u32(addr) < 0x8000'0000u || u32(addr) >= 0xc000'0000u )
	{
		u32 p = 0;
		if( int ent = tlb_search(addr, p); ent >= 0 )
		{
			addr = p;
		} else {
			BADVADDR = addr;
			CONTEXT &= ~0x7fFFf0;
			CONTEXT |= (u32(addr)>>9);
			CONTEXT &= ~15;
			XCONTEXT = (addr>>9);
			XCONTEXT &= 0x1fffffff0ull;
			ENTRY_HI = (ENTRY_HI&0xff) | (addr&~0xfff);
			exception(3, (ent==-1) ? 0x80000000u : 0x80000180u);
			return BusResult::exception(1); 
		}
		//addr -= 0x3C70000; // saves the mario64 head from crashing. really just need a tlb.
	}
	u32 phys_addr = addr&0x1FFFffff;
	phys_write(phys_addr, v, size);
	return 0;
}

u32 VR4300::c0_read32(u32 reg)
{
	//fprintf(stderr, "cop0 read32 c%i = $%lX\n", reg, s64(s32(c[reg])));
	u64 val = c0_read64(reg);
	//if( reg == 9 ) printf("mfc0 count(%i) = $%X\n", reg, u32(val));
	return val;
}

u64 VR4300::c0_read64(u32 reg)
{
	//fprintf(stderr, "cop0 read64 c%i = $%lX\n", reg, c[reg]);
	switch( reg )
	{
	case 9: return u32(COUNT);
	case 11: return u32(COMPARE);
	default: break;
	}
	return c[reg];
}

// c0_write32 is implemented as the full 64bits for now. 
// according to discord posts MTC0 is the same as DMTC0
// but mFc0 is not the same as DmFc0.
void VR4300::c0_write64(u32 reg, u64 v)
{
	//fprintf(stderr, "cop0 write32 c%i = $%X\n", reg, u32(v));
	switch( reg )
	{
	case 2: ENTRY_LO0 = v & 0x3fffffff; return;
	case 3: ENTRY_LO1 = v & 0x3fffffff; return;
	case 5: PAGE_MASK = v & (0xfff<<13); return;
	case 10: ENTRY_HI = v & ~0x1f00; ENTRY_HI &= 0xFFffffFFFFull; return;
	
	//failed: a == b expected, but a=0x345678ffffe0ff b=0x78ffffe0ff. EntryHi was written as 0x12345678ffffffff


	case 0: INDEX = v&0x8000003Fu; return;
	case 4: CONTEXT &= ((1ull<<23)-1); CONTEXT |= v&~((1ull<<23)-1); CONTEXT &= ~15; return;
	
	case 6: WIRED = v & 0x3F; RANDOM = 31; return;
	case 9: COUNT = u32(v); return;
	case 11: CAUSE &= ~BIT(15); COMPARE = u32(v); return;
	case 12: STATUS = u32(v); STATUS &= ~BITL(19); cop1_half_mode = !(STATUS & BIT(26)); return;
	case 13: CAUSE &= ~(BIT(8)|BIT(9)); CAUSE |= v & (BIT(8)|BIT(9)); return;
	
	case 16: CONFIG = v; CONFIG = (CONFIG & 0x7F00800F) | 0x70066460; return;
	
	case 17: LL_ADDR = u32(v); return;
	
	case 20: v&=~0x1ffffFFFFull; XCONTEXT&=0x1ffffFFFFull; XCONTEXT|=v; return;
	
	
	case 26: c[26] = v&0xff; return;
	
	case  1: return;  // Random is read-only
	case  8: return;  // badvaddr is read-only
	case 15: return;  // PRid is read-only
	case 27: return;  // CacheError is read-only
	default: break;
	}
	c[reg] = v;
	return;
}

void VR4300::c0_write32(u32 reg, u64 v)
{
	//fprintf(stderr, "cop0 write64 c%i = $%lX\n", reg, v);
	c0_write64(reg, s32(u32(v)));
}

bool VR4300::COPUnusable(u32 cop)
{
	if( STATUS & BIT(28+cop) ) return false;
	exception(11);
	CAUSE &= ~(BIT(28)|BIT(29));
	CAUSE |= cop<<28;
	return true;
}

int VR4300::tlb_search(u64 virt, u32& phys)
{
	for(int i = 0; i < 32; ++i)
	{
		auto& E = tlb[i];
		const u32 M = ~(E.mask|0x1fff);
		if( (virt & M) != E.vpn ) continue;
		if( !E.G && ((ENTRY_HI&0xff) != E.asid) ) continue;
		const u32 odd = virt & (1<<(std::countr_zero(M)-1));
		if( odd )
		{
			if( !E.V1 ) return -2;
			phys = E.page1;
		} else {
			if( !E.V0 ) return -2;
			phys = E.page0;
		}
		phys <<= 6;
		phys &= M>>1;
		phys |= (virt & ~(M>>1));
		//std::println("translated v${:X} to p${:X}", virt, phys);
		return i;
	}
	return -1;
}

