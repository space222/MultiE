#include <print>
#include <string>
#include "snes.h"
//this file implements the DSP-1 expansion coprocessor, for snes audio see snd.cpp

u32 snes::dspn_P(u32 opcode)
{
	switch( (opcode>>20) & 3 )
	{
	case 0: return dspn.ram[dspn.DP];
	case 1: return dspn.srcval;
	case 2: return (s16(dspn.K) * s16(dspn.L) * 2)>>16;
	default: return s16(dspn.K) * s16(dspn.L) * 2;
	}
}

void snes::dspn_exec()
{
	u32 opcode = dspnPROM[dspn.PC];
	dspn.PC += 1;
	
	//std::println("dsp1:${:X}: opc = ${:X}", dspn.PC-1, opcode);
	
	if( ((opcode>>22)&3) == 3 )
	{  // load immediate
		dspn_dst(opcode, (opcode>>6)&0xffff);
		return;
	}
	
	if( ((opcode>>22)&3) == 2 )
	{ // jumps
		u32 addr = (opcode>>2) & 0x7ff;
		switch( (opcode>>13) & 0x1ff )
		{
		case 0x100: dspn.PC = addr; break;
		case 0x140: dspn_push(dspn.PC); dspn.PC = addr; break;
		case 0x80: if( dspn.Fa.b.C == 0 ) dspn.PC = addr; break;
		case 0x82: if( dspn.Fa.b.C == 1 ) dspn.PC = addr; break;
		case 0x84: if( dspn.Fb.b.C == 0 ) dspn.PC = addr; break;
		case 0x86: if( dspn.Fb.b.C == 1 ) dspn.PC = addr; break;
		case 0x88: if( dspn.Fa.b.Z == 0 ) dspn.PC = addr; break;
		case 0x8A: if( dspn.Fa.b.Z == 1 ) dspn.PC = addr; break;
		case 0x8C: if( dspn.Fb.b.Z == 0 ) dspn.PC = addr; break;
		case 0x8E: if( dspn.Fb.b.Z == 1 ) dspn.PC = addr; break;
		case 0x90: if( dspn.Fa.b.OV0 == 0 ) dspn.PC = addr; break;
		case 0x92: if( dspn.Fa.b.OV0 == 1 ) dspn.PC = addr; break;
		case 0x94: if( dspn.Fb.b.OV0 == 0 ) dspn.PC = addr; break;
		case 0x96: if( dspn.Fb.b.OV0 == 1 ) dspn.PC = addr; break;
		case 0x98: if( dspn.Fa.b.OV1 == 0 ) dspn.PC = addr; break;
		case 0x9A: if( dspn.Fa.b.OV1 == 1 ) dspn.PC = addr; break;
		case 0x9C: if( dspn.Fb.b.OV1 == 0 ) dspn.PC = addr; break;
		case 0x9E: if( dspn.Fb.b.OV1 == 1 ) dspn.PC = addr; break;
		case 0xA0: if( dspn.Fa.b.S0 == 0 ) dspn.PC = addr; break;
		case 0xA2: if( dspn.Fa.b.S0 == 1 ) dspn.PC = addr; break;
		case 0xA4: if( dspn.Fb.b.S0 == 0 ) dspn.PC = addr; break;
		case 0xA6: if( dspn.Fb.b.S0 == 1 ) dspn.PC = addr; break;
		case 0xA8: if( dspn.Fa.b.S1 == 0 ) dspn.PC = addr; break;
		case 0xAA: if( dspn.Fa.b.S1 == 1 ) dspn.PC = addr; break;
		case 0xAC: if( dspn.Fb.b.S1 == 0 ) dspn.PC = addr; break;
		case 0xAE: if( dspn.Fb.b.S1 == 1 ) dspn.PC = addr; break;
		case 0xB0: if( (dspn.DP&15) == 0 ) dspn.PC = addr; break;
		case 0xB1: if( (dspn.DP&15) != 0 ) dspn.PC = addr; break;
		case 0xB2: if( (dspn.DP&15) == 15 ) dspn.PC = addr; break;
		case 0xB3: if( (dspn.DP&15) != 15 ) dspn.PC = addr; break;

		case 0xBC: if( dspn.SR.b.RQM == 0 ) dspn.PC = addr; break;
		case 0xBE: if( dspn.SR.b.RQM == 1 ) dspn.PC = addr; break;
		default:
			std::println("DSPn: Unimpl jmp ${:X}", (opcode>>13) & 0x1ff);
			exit(1);
		}
		//std::println("jmp to ${:X}", dspn.PC);
		return;
	}
	
	auto& F = (opcode&BIT(15)) ? dspn.Fb : dspn.Fa;
	auto& Acc = (opcode&BIT(15)) ? dspn.AccB : dspn.AccA;
	u32 othC = (opcode&BIT(15)) ? dspn.Fa.b.C : dspn.Fb.b.C;
	dspn.srcval = dspn_src(opcode>>4);
	// alu ops
	switch( (opcode>>16) & 15 )
	{
	case 0: break; // nop
	case 1: { // or
		Acc |= dspn_P(opcode); 
		F.b.Z = Acc==0;
		F.b.S0 = F.b.S1 = Acc>>15;
		F.b.C = F.b.OV0 = F.b.OV1 = 0;
		} break;
	case 2: { // and
		Acc &= dspn_P(opcode); 
		F.b.Z = Acc==0;
		F.b.S0 = F.b.S1 = Acc>>15;
		F.b.C = F.b.OV0 = F.b.OV1 = 0;
		} break;
	case 3: { // xor
		Acc ^= dspn_P(opcode); 
		F.b.Z = Acc==0;
		F.b.S0 = F.b.S1 = Acc>>15;
		F.b.C = F.b.OV0 = F.b.OV1 = 0;
		} break;
	case 4: { // sub
		u32 res = Acc;
		u16 p = dspn_P(opcode)^0xffff; 
		res += p;
		res += 1;
		F.b.C = res>>16;
		F.b.S0 = res>>15;
		F.b.Z = ((res&0xffff)==0);
		F.b.OV0 = ((res^p)&(res^Acc))>>15;
		if( F.b.OV0 )
		{
			F.b.S1 = F.b.S0;
			F.b.OV1 ^= 1;		
		}
		} break;
	case 5: { // add
		u32 res = Acc;
		u16 p = dspn_P(opcode); 
		res += p;
		F.b.C = res>>16;
		F.b.S0 = res>>15;
		F.b.Z = ((res&0xffff)==0);
		F.b.OV0 = ((res^p)&(res^Acc))>>15;
		if( F.b.OV0 )
		{
			F.b.S1 = F.b.S0;
			F.b.OV1 ^= 1;		
		}
		} break;
	case 6: { // sbb
		u32 res = Acc;
		u16 p = dspn_P(opcode)^0xffff; 
		res += p;
		res += 1^othC;
		F.b.C = 1^(res>>16);
		F.b.S0 = res>>15;
		F.b.Z = ((res&0xffff)==0);
		F.b.OV0 = ((res^p)&(res^Acc))>>15;
		if( F.b.OV0 )
		{
			F.b.S1 = F.b.S0;
			F.b.OV1 ^= 1;		
		}
		} break;
	case 7: { // adc
		u32 res = Acc;
		u16 p = dspn_P(opcode); 
		res += p;
		res += othC;
		F.b.C = (res>>16);
		F.b.S0 = res>>15;
		F.b.Z = ((res&0xffff)==0);
		F.b.OV0 = ((res^p)&(res^Acc))>>15;
		if( F.b.OV0 )
		{
			F.b.S1 = F.b.S0;
			F.b.OV1 ^= 1;		
		}
		} break;
	case 8: // dec
		F.b.C = Acc==0;
		F.b.OV0 = Acc==0x8000;
		Acc -= 1;
		F.b.Z = Acc==0;		
		F.b.S0 = Acc>>15;
		if( F.b.OV0 )
		{
			F.b.S1 = F.b.S0;
			F.b.OV1 ^= 1;
		}	
		break;	
	case 9: // inc
		F.b.C = Acc==0xffff;
		F.b.OV0 = Acc==0x7fff;
		Acc += 1;
		F.b.Z = Acc==0;		
		F.b.S0 = Acc>>15;
		if( F.b.OV0 )
		{
			F.b.S1 = F.b.S0;
			F.b.OV1 ^= 1;
		}	
		break;
	case 0xA: { // not
		Acc ^= 0xffff; 
		F.b.Z = Acc==0;
		F.b.S0 = F.b.S1 = Acc>>15;
		F.b.C = F.b.OV0 = F.b.OV1 = 0;
		} break;
	case 0xB: { // sar1
		F.b.C = Acc&1;
		Acc = s16(Acc)>>1; 
		F.b.Z = Acc==0;
		F.b.S0 = F.b.S1 = Acc>>15;
		F.b.OV0 = F.b.OV1 = 0;
		} break;
	case 0xC: { // rcl1
		F.b.C = Acc>>15;
		Acc = (Acc<<1)|othC;
		F.b.Z = Acc==0;
		F.b.S0 = F.b.S1 = Acc>>15;
		F.b.OV0 = F.b.OV1 = 0;
		} break;
	case 0xD: { // sll2
		F.b.C = 0; //??
		Acc = (Acc<<2)|3; 
		F.b.Z = Acc==0;
		F.b.S0 = F.b.S1 = Acc>>15;
		F.b.OV0 = F.b.OV1 = 0;
		} break;
	case 0xE: { // sll4
		F.b.C = 0; //??
		Acc = (Acc<<4)|15; 
		F.b.Z = Acc==0;
		F.b.S0 = F.b.S1 = Acc>>15;
		F.b.OV0 = F.b.OV1 = 0;
		} break;
	case 0xF: { // xchg
		F.b.C = 0;
		Acc = (Acc<<8)|(Acc>>8); 
		F.b.Z = Acc==0;
		F.b.S0 = F.b.S1 = Acc>>15;
		F.b.OV0 = F.b.OV1 = 0;
		} break;
	default:
		std::println("DSPn: Unimpl alu ${:X}", (opcode>>16) & 15);
		exit(1);
	}
	
	//does this stuff just happen "at the same time"?
	dspn_dst(opcode, dspn.srcval);
	
	u32 dpl = dspn.DP&15;
	switch( (opcode>>13)&3 )
	{
	case 0: break;
	case 1: dpl+=1; break;
	case 2: dpl-=1; break;
	case 3: dpl=0; break;
	}
	dspn.DP ^= (((opcode>>9)&15)<<4);
	dspn.DP &= 0xf0;
	dspn.DP |= dpl&15;
	
	if( opcode & BIT(8) ) 
	{
		dspn.RP -= 1;
		dspn.RP &= 0x7ff;
	}
	
	if( opcode & BIT(22) )
	{
		dspn.PC = dspn_pop();
	}
}

void snes::dspn_dst(u32 r, u32 v)
{
	switch( r & 15 )
	{
	case 0: return;
	case 1: dspn.AccA = v; break;
	case 2: dspn.AccB = v; break;
	case 3: dspn.TR = v; break;
	case 4: dspn.DP = v; break;
	case 5: dspn.RP = v; break;
	case 6: dspn.DR = v; dspn.SR.b.RQM = 1; break;
	case 7: dspn.SR.v = (v&~0x9000)|(dspn.SR.v&0x9000); break;
	//case 8: break;
	//case 9: break;
	case 0xA: dspn.K = v; break;
	case 0xB: dspn.K = v; dspn.L = dspnDROM[dspn.RP&0x3ff]; break;
	case 0xC: dspn.L = v; dspn.K = dspn.ram[(dspn.DP | 0x40)&0xff]; break;
	case 0xD: dspn.L = v; break;
	case 0xE: dspn.TRB = v; break;
	case 0xF: dspn.ram[dspn.DP&0xff] = v; break;
	default:
		std::println("dspn_dst error {}", r&15);
		exit(1);	
	}
}

u32 snes::dspn_src(u32 r)
{
	switch( r & 15 )
	{
	case 0: return dspn.TRB;
	case 1: return dspn.AccA;
	case 2: return dspn.AccB;
	case 3: return dspn.TR;
	case 4: return dspn.DP;
	case 5: return dspn.RP;
	case 6: return dspnDROM[dspn.RP&0x3ff];
	case 7: return 0x8000-dspn.Fa.b.S1;
	case 8: dspn.SR.b.RQM = 1; return dspn.DR;
	case 9: return dspn.DR;
	case 0xA: return dspn.SR.v;
	//case 0xB: return 0; // 0Bh   SIM  (SI serial MSB first)
	//case 0xC: return 0; // 0Ch   SIL  (SI serial LSB first)
	case 0xD: return dspn.K;
	case 0xE: return dspn.L;
	case 0xF: return dspn.ram[dspn.DP&0xff];
	default:
		std::println("dspn_src error {}", r&15);
		exit(1);	
	}
}

u8 snes::dspn_data_rd()
{
	if( dspn.SR.b.DRC )
	{
		dspn.SR.b.RQM = 0;
		return dspn.DR;
	}
	
	if( dspn.SR.b.DRS == 0 )
	{
		dspn.SR.b.DRS = 1;
		return dspn.DR;
	} else {
		dspn.SR.b.RQM = dspn.SR.b.DRS = 0;
		return dspn.DR >> 8;
	}
}

void snes::dspn_data_wr(u8 v)
{
	if( dspn.SR.b.DRC )
	{
		dspn.DR = v;
		dspn.SR.b.RQM = 0;
		return;
	}
	
	if( dspn.SR.b.DRS == 0 )
	{
		dspn.DR &= 0xff00;
		dspn.DR |= v;
		dspn.SR.b.DRS = 1;
	} else {
		dspn.DR &= 0x00ff;
		dspn.DR |= v<<8;
		dspn.SR.b.DRS = dspn.SR.b.RQM = 0;
	}
}

u8 snes::dspn_stat_rd() { return dspn.SR.v>>8; }
void snes::dspn_stat_wr(u8 /*v*/)
{
	return;  // can 65816 set SR?
}

void snes::dspn_run()
{
	while( dspn.stamp < master_stamp )
	{
		dspn_exec();
		dspn.stamp += 2;
	}
}

