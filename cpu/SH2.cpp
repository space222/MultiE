#include <cstdio>
#include <cstdlib>
#include "SH2.h"

#define Rn cpu.r[(opc>>8)&0xf]
#define Rm cpu.r[(opc>>4)&0xf]
#define R0 cpu.r[0]

//extern int cyclenum;

void sh2_nop(SH2&, u16) {}
void sh2_clrt(SH2& cpu, u16) { cpu.sr.b.T = 0; }
void sh2_sett(SH2& cpu, u16) { cpu.sr.b.T = 1; }
void sh2_clrmac(SH2& cpu, u16) { cpu.mach = cpu.macl = 0; }
void sh2_2nm4(SH2& cpu, u16 opc) { u32 v = Rm; Rn -= 1; cpu.write(Rn, v, 8); }
void sh2_2nm5(SH2& cpu, u16 opc) { u32 v = Rm; Rn -= 2; cpu.write(Rn, v, 16); }
void sh2_2nm6(SH2& cpu, u16 opc) { u32 v = Rm; Rn -= 4; cpu.write(Rn, v, 32); }
void sh2_3nmC(SH2& cpu, u16 opc) { Rn += Rm; }
void sh2_rts(SH2& cpu, u16) { /*cyclenum++;*/ cpu.exec(cpu.nextpc); cpu.nextpc = cpu.pr; }
void sh2_rte(SH2& cpu, u16) 
{
	u32 temp = cpu.nextpc;
	cpu.nextpc = cpu.read(cpu.r[15],32); cpu.r[15]+=4; 
	cpu.sr.v = cpu.read(cpu.r[15],32); cpu.r[15]+=4;
	/*cyclenum++;*/ cpu.exec(temp); 
}

void sh2_stc_sr(SH2& cpu, u16 opc) { Rn = cpu.sr.v; }
void sh2_movt(SH2& cpu, u16 opc) { Rn = cpu.sr.b.T; }
void sh2_stc_gbr(SH2& cpu, u16 opc) { Rn = cpu.gbr; }
void sh2_stc_vbr(SH2& cpu, u16 opc) { Rn = cpu.vbr; }
void sh2_sts_mach(SH2& cpu, u16 opc) { Rn = cpu.mach; }
void sh2_sts_macl(SH2& cpu, u16 opc) { Rn = cpu.macl; }
void sh2_sts_pr(SH2& cpu, u16 opc) { Rn = cpu.pr; }

void sh2_bsrf(SH2& cpu, u16 opc) 
{ 
	cpu.pr = cpu.pc+4; 
	u32 temp = cpu.pc + Rn + 4; 
	/*cyclenum++;*/ cpu.exec(cpu.nextpc);
	cpu.nextpc = temp;
}
void sh2_braf(SH2& cpu, u16 opc) { u32 temp = cpu.pc + Rn + 4; /*cyclenum++;*/ cpu.exec(cpu.nextpc); cpu.nextpc = temp; }
void sh2_jsr(SH2& cpu, u16 opc) { u32 temp = Rn; /*cyclenum++;*/ cpu.exec(cpu.nextpc); cpu.pr = cpu.pc+4; cpu.nextpc = temp; }
void sh2_jmp(SH2& cpu, u16 opc) { u32 temp = Rn; /*cyclenum++;*/ cpu.exec(cpu.nextpc); cpu.nextpc = temp; }

void sh2_div0u(SH2& cpu, u16) { cpu.sr.b.M = cpu.sr.b.Q = cpu.sr.b.T = 0; }
void sh2_sleep(SH2& cpu, u16) { /*todo*/ cpu.nextpc -= 2; }

void sh2_0nm4(SH2& cpu, u16 opc) { u8 v = Rm; cpu.write(Rn+R0, v, 8); }
void sh2_0nm5(SH2& cpu, u16 opc) { u16 v = Rm; cpu.write(Rn+R0, v, 16); }
void sh2_0nm6(SH2& cpu, u16 opc) { u32 v = Rm; cpu.write(Rn+R0, v, 32); }
void sh2_mull(SH2& cpu, u16 opc) { cpu.macl = Rm*Rn; }

void sh2_0nmC(SH2& cpu, u16 opc) { Rn = (s32)(s8)cpu.read(Rm+R0, 8); }
void sh2_0nmD(SH2& cpu, u16 opc) { Rn = (s32)(s16)cpu.read(Rm+R0, 16); }
void sh2_0nmE(SH2& cpu, u16 opc) { Rn = cpu.read(Rm+R0, 32); }

void sh2_1nmd(SH2& cpu, u16 opc) { cpu.write(Rn+((opc&0xf)*4), Rm, 32); }

void sh2_2nm0(SH2& cpu, u16 opc) { cpu.write(Rn, Rm, 8); }
void sh2_2nm1(SH2& cpu, u16 opc) { cpu.write(Rn, Rm, 16); }
void sh2_2nm2(SH2& cpu, u16 opc) { cpu.write(Rn, Rm, 32); }

void sh2_tst(SH2& cpu, u16 opc) { cpu.sr.b.T = ((Rn&Rm)==0)?1:0; }
void sh2_and(SH2& cpu, u16 opc) { Rn &= Rm; }
void sh2_xor(SH2& cpu, u16 opc) { Rn ^= Rm; }
void sh2_or(SH2& cpu, u16 opc) { Rn |= Rm; }
void sh2_xtrct(SH2& cpu, u16 opc) { Rn = (Rm<<16)|(Rn>>16); }
void sh2_muluw(SH2& cpu, u16 opc) { u32 a = Rm&0xffff; a *= Rn&0xffff; cpu.macl = a; }
void sh2_mulsw(SH2& cpu, u16 opc) { s32 a = s16(Rm); a *= s16(Rn); cpu.macl = a; }
void sh2_dmulul(SH2& cpu, u16 opc) { u64 a = Rm; a *= Rn; cpu.mach = a>>32; cpu.macl = a; }
void sh2_dmulsl(SH2& cpu, u16 opc) { s64 a = s32(Rm); a *= s32(Rn); cpu.mach = a>>32; cpu.macl = a; }

void sh2_div0s(SH2& cpu, u16 opc) { cpu.sr.b.Q = (Rn>>31); cpu.sr.b.M = (Rm>>31); cpu.sr.b.T = cpu.sr.b.Q ^ cpu.sr.b.M; }
void sh2_div1(SH2& cpu, u16 opc) 
{
	// straight yoink out of the manual
	u32 tmp0;
	u8 old_q,tmp1;
	old_q=cpu.sr.b.Q;
	cpu.sr.b.Q=(unsigned char)((0x80000000u & Rn)!=0);
	Rn<<=1;
	Rn|=(unsigned long)cpu.sr.b.T;
	switch(old_q){
	case 0:switch(cpu.sr.b.M){
	case 0:tmp0=Rn;
	Rn-=Rm;
	tmp1=(Rn>tmp0);
	switch(cpu.sr.b.Q){
	case 0:cpu.sr.b.Q=tmp1;
	break;
	case 1:cpu.sr.b.Q=(unsigned char)(tmp1==0);
	break;
	}
	break;
	case 1:tmp0=Rn;
	Rn+=Rm;
	tmp1=(Rn<tmp0);
	switch(cpu.sr.b.Q){
	case 0:cpu.sr.b.Q=(unsigned char)(tmp1==0);
	break;
	case 1:cpu.sr.b.Q=tmp1;
	break;
	}
	break;
	}
	break;
	case 1:switch(cpu.sr.b.M){
	case 0:tmp0=Rn;
	Rn+=Rm;
	tmp1=(Rn<tmp0);
	switch(cpu.sr.b.Q){
	case 0:cpu.sr.b.Q=tmp1;
	break;
	case 1:cpu.sr.b.Q=(unsigned char)(tmp1==0);
	break;
	}
	break;
	case 1:tmp0=Rn;
	Rn-=Rm;
	tmp1=(Rn>tmp0);
	switch(cpu.sr.b.Q){
	case 0:cpu.sr.b.Q=(unsigned char)(tmp1==0);
	break;
	case 1:cpu.sr.b.Q=tmp1;
	break;
	}
	break;
	}
	break;
	}
	cpu.sr.b.T=(cpu.sr.b.Q==cpu.sr.b.M);
}

void sh2_cmpeq(SH2& cpu, u16 opc) { cpu.sr.b.T = ((Rn==Rm)?1:0); }
void sh2_cmphs(SH2& cpu, u16 opc) { cpu.sr.b.T = ((Rn>=Rm)?1:0); }
void sh2_cmpge(SH2& cpu, u16 opc) { cpu.sr.b.T = ((s32(Rn)>=s32(Rm))?1:0); }
void sh2_cmphi(SH2& cpu, u16 opc) { cpu.sr.b.T = ((Rn>Rm)?1:0); }
void sh2_cmpgt(SH2& cpu, u16 opc) { cpu.sr.b.T = ((s32(Rn)>s32(Rm))?1:0); }
void sh2_cmppz(SH2& cpu, u16 opc) { cpu.sr.b.T = ((s32(Rn)>=0)?1:0); }
void sh2_cmppl(SH2& cpu, u16 opc) { cpu.sr.b.T = ((s32(Rn)>0)?1:0); }

void sh2_sub(SH2& cpu, u16 opc) { Rn -= Rm; }
void sh2_subc(SH2& cpu, u16 opc) 
{ 
	u64 a = Rn;
	a += ~Rm;
	a += cpu.sr.b.T^1;
	Rn = a;
	cpu.sr.b.T = ((a>>32)&1)^1;
}
void sh2_subv(SH2& cpu, u16 opc) 
{ 
	u32 a = Rn-Rm;
	cpu.sr.b.T = (((Rn^a)&(~Rm^a)&(1u<<31))?1:0);
	Rn = a;
}
void sh2_negc(SH2& cpu, u16 opc) 
{ 
	u64 a = 0;
	a += ~Rm;
	a += cpu.sr.b.T^1;
	Rn = a;
	cpu.sr.b.T = ((a>>32)&1)^1;
}
void sh2_addc(SH2& cpu, u16 opc) { u64 a = Rn; a += Rm; a += cpu.sr.b.T; Rn = a; cpu.sr.b.T = a>>32; }
void sh2_addv(SH2& cpu, u16 opc) 
{ 
	u32 a = Rn+Rm;
	cpu.sr.b.T = (((Rn^a)&(Rm^a)&(1u<<31))?1:0);
	Rn = a;
}

void sh2_cmpstr(SH2& cpu, u16 opc) 
{ 
	if( (Rm&0xff) == (Rn&0xff) ||
	    (Rm&0xff00) == (Rn&0xff00) ||
	    (Rm&0xff0000)==(Rn&0xff0000) ||
	    (Rm>>24)==(Rn>>24) )
	{
		cpu.sr.b.T = 1;
	} else {
		cpu.sr.b.T = 0;
	}
}

void sh2_4m26(SH2& cpu, u16 opc) { cpu.pr = cpu.read(Rn, 32); Rn += 4; }
void sh2_4m27(SH2& cpu, u16 opc) { cpu.vbr = cpu.read(Rn, 32); Rn += 4; }

void sh2_4m06(SH2& cpu, u16 opc) { cpu.mach = cpu.read(Rn, 32); Rn += 4; }
void sh2_4m16(SH2& cpu, u16 opc) { cpu.macl = cpu.read(Rn, 32); Rn += 4; }
void sh2_4m17(SH2& cpu, u16 opc) { cpu.gbr = cpu.read(Rn, 32); Rn += 4; }
void sh2_4m07(SH2& cpu, u16 opc) { cpu.sr.v = cpu.read(Rn, 32); Rn += 4; }
void sh2_4m0A(SH2& cpu, u16 opc) { cpu.mach = Rn; }
void sh2_4m1A(SH2& cpu, u16 opc) { cpu.macl = Rn; }
void sh2_4m0E(SH2& cpu, u16 opc) { cpu.sr.v = Rn; }
void sh2_4m1E(SH2& cpu, u16 opc) { cpu.gbr = Rn; }
void sh2_4m2A(SH2& cpu, u16 opc) { cpu.pr = Rn; }
void sh2_4m2E(SH2& cpu, u16 opc) { cpu.vbr = Rn; }

void sh2_shll(SH2& cpu, u16 opc) { cpu.sr.b.T = (Rn>>31); Rn<<=1; }
void sh2_shlr(SH2& cpu, u16 opc) { cpu.sr.b.T = (Rn&1); Rn>>=1; }
void sh2_shar(SH2& cpu, u16 opc) { cpu.sr.b.T = (Rn&1); Rn = s32(Rn)>>1; }
void sh2_shll2(SH2& cpu, u16 opc) { Rn<<=2; }
void sh2_shlr2(SH2& cpu, u16 opc) { Rn>>=2; }
void sh2_shll8(SH2& cpu, u16 opc) { Rn<<=8; }
void sh2_shlr8(SH2& cpu, u16 opc) { Rn>>=8; }
void sh2_shll16(SH2& cpu, u16 opc) { Rn<<=16; }
void sh2_shlr16(SH2& cpu, u16 opc) { Rn>>=16; }
void sh2_4m02(SH2& cpu, u16 opc) { Rn-=4; cpu.write(Rn, cpu.mach, 32); }
void sh2_4m12(SH2& cpu, u16 opc) { Rn-=4; cpu.write(Rn, cpu.macl, 32); }
void sh2_4m22(SH2& cpu, u16 opc) { Rn-=4; cpu.write(Rn, cpu.pr, 32); }
void sh2_4m23(SH2& cpu, u16 opc) { Rn-=4; cpu.write(Rn, cpu.vbr, 32); }
void sh2_4m13(SH2& cpu, u16 opc) { Rn-=4; cpu.write(Rn, cpu.gbr, 32); }
void sh2_4m03(SH2& cpu, u16 opc) { Rn-=4; cpu.write(Rn, cpu.sr.v, 32); }
void sh2_rotl(SH2& cpu, u16 opc) { cpu.sr.b.T = (Rn>>31); Rn=(Rn>>31)|(Rn<<1); }
void sh2_rotr(SH2& cpu, u16 opc) { cpu.sr.b.T = (Rn&1); Rn=(Rn<<31)|(Rn>>1); }
void sh2_dt(SH2& cpu, u16 opc) { Rn -= 1; cpu.sr.b.T = ((Rn==0)?1:0); }

void sh2_tasb(SH2& cpu, u16 opc) 
{ 
	u8 v = cpu.read(Rn, 8);
	if( !v ) { cpu.sr.b.T = 1; cpu.write(Rn, v|0x80, 8); }
	else { cpu.sr.b.T = 0; }
}

void sh2_rotcl(SH2& cpu, u16 opc) { u32 oldT = cpu.sr.b.T; cpu.sr.b.T = (Rn>>31); Rn <<= 1; Rn |= oldT; }
void sh2_rotcr(SH2& cpu, u16 opc) { u32 oldT = cpu.sr.b.T; cpu.sr.b.T = (Rn&1); Rn >>= 1; Rn |= oldT<<31; }

void sh2_5nmd(SH2& cpu, u16 opc) { Rn = cpu.read(Rm+((opc&0xf)*4), 32); }

void sh2_6nm0(SH2& cpu, u16 opc) { Rn = (s32)(s8)cpu.read(Rm, 8); }
void sh2_6nm1(SH2& cpu, u16 opc) { Rn = (s32)(s16)cpu.read(Rm, 16); }
void sh2_6nm2(SH2& cpu, u16 opc) { Rn = cpu.read(Rm, 32); }
void sh2_6nm3(SH2& cpu, u16 opc) { Rn = Rm; }
void sh2_6nm4(SH2& cpu, u16 opc) { u32 v = (s32)(s8)cpu.read(Rm, 8); Rm += 1; Rn = v; }
void sh2_6nm5(SH2& cpu, u16 opc) { u32 v = (s32)(s16)cpu.read(Rm, 16); Rm += 2; Rn = v; }
void sh2_6nm6(SH2& cpu, u16 opc) { u32 v = cpu.read(Rm, 32); Rm += 4; Rn = v; }
void sh2_6nm7(SH2& cpu, u16 opc) { Rn = ~Rm; }
void sh2_6nm8(SH2& cpu, u16 opc) { Rn = (Rm&0xffff0000)|((Rm<<8)&0xff00)|((Rm>>8)&0xff); }
void sh2_6nm9(SH2& cpu, u16 opc) { Rn = (Rm<<16)|(Rm>>16); }
void sh2_6nmB(SH2& cpu, u16 opc) { Rn = 0-Rm; }
void sh2_6nmC(SH2& cpu, u16 opc) { Rn = u8(Rm); }
void sh2_6nmD(SH2& cpu, u16 opc) { Rn = u16(Rm); }
void sh2_6nmE(SH2& cpu, u16 opc) { Rn = (s32)s8(Rm); }
void sh2_6nmF(SH2& cpu, u16 opc) { Rn = (s32)s16(Rm); }
void sh2_7ni(SH2& cpu, u16 opc) { Rn += s8(opc); }

void sh2_80md(SH2& cpu, u16 opc) { cpu.write(Rm+(opc&0xf), R0, 8); }
void sh2_81md(SH2& cpu, u16 opc) { cpu.write(Rm+(opc&0xf)*2, R0, 16); }
void sh2_84md(SH2& cpu, u16 opc) { R0 = (s32)(s8)cpu.read(Rm+(opc&0xf), 8); }
void sh2_85md(SH2& cpu, u16 opc) { R0 = (s32)(s16)cpu.read(Rm+(opc&0xf)*2, 16); }
void sh2_88i(SH2& cpu, u16 opc) { cpu.sr.b.T = ((s32(R0) == (s32)(s8)opc)?1:0); }
void sh2_bt(SH2& cpu, u16 opc) { if( cpu.sr.b.T ) cpu.nextpc = cpu.pc + 4 + 2*(s8)opc; }
void sh2_bf(SH2& cpu, u16 opc) { if( !cpu.sr.b.T ) cpu.nextpc = cpu.pc + 4 + 2*(s8)opc; }
void sh2_bt_s(SH2& cpu, u16 opc) { if( cpu.sr.b.T ) { /*cyclenum++;*/ cpu.exec(cpu.nextpc); cpu.nextpc = cpu.pc + 4 + 2*(s8)opc; } }
void sh2_bf_s(SH2& cpu, u16 opc) { if( !cpu.sr.b.T ) { /*cyclenum++;*/ cpu.exec(cpu.nextpc); cpu.nextpc = cpu.pc + 4 + 2*(s8)opc; } }

void sh2_9nd(SH2& cpu, u16 opc) { Rn = (s32)(s16)cpu.read(cpu.pc + 4 + 2*(opc&0xff), 16); }
void sh2_bra(SH2& cpu, u16 opc) { /*cyclenum++;*/ cpu.exec(cpu.nextpc); cpu.nextpc = cpu.pc + 4 + 2*(s16(opc<<4)>>4); }
void sh2_bsr(SH2& cpu, u16 opc) { /*cyclenum++;*/ cpu.exec(cpu.nextpc); cpu.pr = cpu.pc + 4; cpu.nextpc = cpu.pc + 4 + 2*(s16(opc<<4)>>4); }

void sh2_c0d(SH2& cpu, u16 opc) { cpu.write(cpu.gbr + (opc&0xff), R0, 8); }
void sh2_c1d(SH2& cpu, u16 opc) { cpu.write(cpu.gbr + (opc&0xff)*2, R0, 16); }
void sh2_c2d(SH2& cpu, u16 opc) { cpu.write(cpu.gbr + (opc&0xff)*4, R0, 32); }
void sh2_c4d(SH2& cpu, u16 opc) { R0 = (s32)(s8)cpu.read(cpu.gbr + (opc&0xff), 8); }
void sh2_c5d(SH2& cpu, u16 opc) { R0 = (s32)(s16)cpu.read(cpu.gbr + (opc&0xff)*2, 16); }
void sh2_c6d(SH2& cpu, u16 opc) { R0 = cpu.read(cpu.gbr + (opc&0xff)*4, 32); }
void sh2_c7d(SH2& cpu, u16 opc) { R0 = (((cpu.pc+4)&~3) + (opc&0xff)*4); }
void sh2_c8i(SH2& cpu, u16 opc) { cpu.sr.b.T = ((R0&u8(opc))==0?1:0); }
void sh2_c9i(SH2& cpu, u16 opc) { R0 &= u8(opc); }
void sh2_cAi(SH2& cpu, u16 opc) { R0 ^= u8(opc); }
void sh2_cBi(SH2& cpu, u16 opc) { R0 |= u8(opc); }
void sh2_cCi(SH2& cpu, u16 opc) { cpu.sr.b.T = ((cpu.read(cpu.gbr+R0,8)&u8(opc))==0?1:0); }
void sh2_cDi(SH2& cpu, u16 opc) { u8 v = cpu.read(cpu.gbr+R0,8); v &= u8(opc); cpu.write(cpu.gbr+R0, v, 8); }
void sh2_cEi(SH2& cpu, u16 opc) { u8 v = cpu.read(cpu.gbr+R0,8); v ^= u8(opc); cpu.write(cpu.gbr+R0, v, 8); }
void sh2_cFi(SH2& cpu, u16 opc) { u8 v = cpu.read(cpu.gbr+R0,8); v |= u8(opc); cpu.write(cpu.gbr+R0, v, 8); }

void sh2_dnd(SH2& cpu, u16 opc) { Rn = cpu.read(((cpu.pc+4)&~3) + (opc&0xff)*4, 32); }
void sh2_eni(SH2& cpu, u16 opc) { Rn = (s32)(s8)opc; }

void sh2_trapa(SH2& cpu, u16 opc) 
{
	cpu.r[15]-=4;
	cpu.write(cpu.r[15], cpu.sr.v, 32);
	cpu.r[15]-=4;
	cpu.write(cpu.r[15], cpu.pc+2, 32);
	cpu.nextpc = cpu.read((opc&0xff)*4 + cpu.vbr, 32);
}

typedef void (*sh2opc)(SH2& cpu, u16 opc);

struct sh2instr
{
	u16 mask, res;
	sh2opc func;
};

static sh2instr optable[] = {
	{ 0xf000, 0xE000, sh2_eni },
	{ 0xf000, 0xD000, sh2_dnd },

	{ 0xff00, 0xC000, sh2_c0d },
	{ 0xff00, 0xC100, sh2_c1d },
	{ 0xff00, 0xC200, sh2_c2d },
	{ 0xff00, 0xC300, sh2_trapa },
	{ 0xff00, 0xC400, sh2_c4d },
	{ 0xff00, 0xC500, sh2_c5d },
	{ 0xff00, 0xC600, sh2_c6d },
	{ 0xff00, 0xC700, sh2_c7d },
	{ 0xff00, 0xC800, sh2_c8i },
	{ 0xff00, 0xC900, sh2_c9i },
	{ 0xff00, 0xCA00, sh2_cAi },
	{ 0xff00, 0xCB00, sh2_cBi },
	{ 0xff00, 0xCC00, sh2_cCi },
	{ 0xff00, 0xCD00, sh2_cDi },
	{ 0xff00, 0xCE00, sh2_cEi },
	{ 0xff00, 0xCF00, sh2_cFi },
	
	{ 0xff00, 0x8000, sh2_80md },
	{ 0xff00, 0x8100, sh2_81md },
	{ 0xff00, 0x8400, sh2_84md },
	{ 0xff00, 0x8500, sh2_85md },
	{ 0xff00, 0x8800, sh2_88i },
	{ 0xff00, 0x8900, sh2_bt },
	{ 0xff00, 0x8B00, sh2_bf },
	{ 0xff00, 0x8D00, sh2_bt_s },
	{ 0xff00, 0x8F00, sh2_bf_s },
	{ 0xf000, 0x9000, sh2_9nd },
	{ 0xf000, 0xA000, sh2_bra },
	{ 0xf000, 0xB000, sh2_bsr },
	
	{ 0xf000, 0x7000, sh2_7ni },
	{ 0xf00f, 0x6000, sh2_6nm0 },
	{ 0xf00f, 0x6001, sh2_6nm1 },
	{ 0xf00f, 0x6002, sh2_6nm2 },
	{ 0xf00f, 0x6003, sh2_6nm3 },
	{ 0xf00f, 0x6004, sh2_6nm4 },
	{ 0xf00f, 0x6005, sh2_6nm5 },
	{ 0xf00f, 0x6006, sh2_6nm6 },
	{ 0xf00f, 0x6007, sh2_6nm7 },
	{ 0xf00f, 0x6008, sh2_6nm8 },
	{ 0xf00f, 0x6009, sh2_6nm9 },
	{ 0xf00f, 0x600A, sh2_negc },
	{ 0xf00f, 0x600B, sh2_6nmB },
	{ 0xf00f, 0x600C, sh2_6nmC },
	{ 0xf00f, 0x600D, sh2_6nmD },
	{ 0xf00f, 0x600E, sh2_6nmE },
	{ 0xf00f, 0x600F, sh2_6nmF },
	{ 0xf000, 0x5000, sh2_5nmd },
	{ 0xf0ff, 0x4000, sh2_shll },
	{ 0xf0ff, 0x4001, sh2_shlr },
	{ 0xf0ff, 0x4002, sh2_4m02 },
	{ 0xf0ff, 0x4003, sh2_4m03 },
	{ 0xf0ff, 0x4004, sh2_rotl },
	{ 0xf0ff, 0x4005, sh2_rotr },
	{ 0xf0ff, 0x4006, sh2_4m06 },
	{ 0xf0ff, 0x4007, sh2_4m07 },
	{ 0xf0ff, 0x4008, sh2_shll2 },
	{ 0xf0ff, 0x4009, sh2_shlr2 },
	{ 0xf0ff, 0x400A, sh2_4m0A },
	{ 0xf0ff, 0x400B, sh2_jsr },
	{ 0xf0ff, 0x400E, sh2_4m0E },
	{ 0xf0ff, 0x4010, sh2_dt },
	{ 0xf0ff, 0x4011, sh2_cmppz },
	{ 0xf0ff, 0x4012, sh2_4m12 },
	{ 0xf0ff, 0x4013, sh2_4m13 },
	{ 0xf0ff, 0x4015, sh2_cmppl },
	{ 0xf0ff, 0x4016, sh2_4m16 },
	{ 0xf0ff, 0x4017, sh2_4m17 },
	{ 0xf0ff, 0x4018, sh2_shll8 },
	{ 0xf0ff, 0x4019, sh2_shlr8 },
	{ 0xf0ff, 0x401A, sh2_4m1A },
	{ 0xf0ff, 0x401B, sh2_tasb },
	{ 0xf0ff, 0x401E, sh2_4m1E },
	{ 0xf0ff, 0x4020, sh2_shll },
	{ 0xf0ff, 0x4021, sh2_shar },
	{ 0xf0ff, 0x4022, sh2_4m22 },
	{ 0xf0ff, 0x4023, sh2_4m23 },
	{ 0xf0ff, 0x4024, sh2_rotcl },
	{ 0xf0ff, 0x4025, sh2_rotcr },
	{ 0xf0ff, 0x4026, sh2_4m26 },
	{ 0xf0ff, 0x4027, sh2_4m27 },
	{ 0xf0ff, 0x4028, sh2_shll16 },
	{ 0xf0ff, 0x4029, sh2_shlr16 },
	{ 0xf0ff, 0x402A, sh2_4m2A },
	{ 0xf0ff, 0x402B, sh2_jmp },
	{ 0xf0ff, 0x402E, sh2_4m2E },
	{ 0xf00f, 0x2000, sh2_2nm0 },
	{ 0xf00f, 0x2001, sh2_2nm1 },
	{ 0xf00f, 0x2002, sh2_2nm2 },
	{ 0xf00f, 0x2007, sh2_div0s },
	{ 0xf00f, 0x2008, sh2_tst },
	{ 0xf00f, 0x2009, sh2_and },
	{ 0xf00f, 0x200A, sh2_xor },
	{ 0xf00f, 0x200B, sh2_or },
	{ 0xf00f, 0x200C, sh2_cmpstr },
	{ 0xf00f, 0x200D, sh2_xtrct },
	{ 0xf00f, 0x200E, sh2_muluw },
	{ 0xf00f, 0x200F, sh2_mulsw },
	{ 0xf00f, 0x3000, sh2_cmpeq },
	{ 0xf00f, 0x3002, sh2_cmphs },
	{ 0xf00f, 0x3003, sh2_cmpge },
	{ 0xf00f, 0x3004, sh2_div1 },
	{ 0xf00f, 0x3005, sh2_dmulul },
	{ 0xf00f, 0x3006, sh2_cmphi },
	{ 0xf00f, 0x3007, sh2_cmpgt },
	{ 0xf00f, 0x3008, sh2_sub },
	{ 0xf00f, 0x300A, sh2_subc },
	{ 0xf00f, 0x300B, sh2_subv },
	{ 0xf00f, 0x300D, sh2_dmulsl },
	{ 0xf00f, 0x300E, sh2_addc },
	{ 0xf00f, 0x300F, sh2_addv },
	{ 0xf000, 0x1000, sh2_1nmd },
	{ 0xf00f, 0x000C, sh2_0nmC },
	{ 0xf00f, 0x000D, sh2_0nmD },
	{ 0xf00f, 0x000E, sh2_0nmE },
	{ 0xf00f, 0x0007, sh2_mull },
	{ 0xf00f, 0x0004, sh2_0nm4 },
	{ 0xf00f, 0x0005, sh2_0nm5 },
	{ 0xf00f, 0x0006, sh2_0nm6 },
	{ 0xf0ff, 0x0003, sh2_bsrf },
	{ 0xf0ff, 0x0023, sh2_braf },
	{ 0xf0ff, 0x0012, sh2_stc_gbr },
	{ 0xf0ff, 0x0022, sh2_stc_vbr },
	{ 0xf0ff, 0x0002, sh2_stc_sr },
	{ 0xf0ff, 0x0029, sh2_movt },
	{ 0xf0ff, 0x002A, sh2_sts_pr },
	{ 0xffff, 0x0008, sh2_clrt },
	{ 0xf0ff, 0x000A, sh2_sts_mach },
	{ 0xf0ff, 0x001A, sh2_sts_macl },
	{ 0xffff, 0x0018, sh2_sett },
	{ 0xffff, 0x0028, sh2_clrmac },
	{ 0xffff, 0x0019, sh2_div0u },
	{ 0xffff, 0x002B, sh2_rte },
	{ 0xffff, 0x001B, sh2_sleep },
	{ 0xffff, 0x0009, sh2_nop },
	{ 0xffff, 0x000B, sh2_rts },
	{ 0xf00f, 0x2004, sh2_2nm4 },
	{ 0xf00f, 0x2005, sh2_2nm5 },
	{ 0xf00f, 0x2006, sh2_2nm6 },
	{ 0xf00f, 0x300C, sh2_3nmC },
};

void SH2::reset()
{
	pc = 0;
}

u64 SH2::step()
{
	nextpc = pc+2;
	u64 c = exec(pc);
	pc = nextpc;
	return c;
}

u64 SH2::exec(u32 addr)
{
	u16 opc = memread(addr, 16); //, true);
	for(u32 i = 0; i < sizeof(optable)/sizeof(sh2instr); ++i)
	{
		if( (opc & optable[i].mask) == optable[i].res )
		{
			optable[i].func(*this, opc);
			return 1;
		}
	}
	
	printf("SH2: Unimpl opc = $%04X\n", opc);
	exit(1);	
	return 1;
}

u32 SH2::read(u32 addr, int size)
{
	if( addr >= 0xFFFFfe00 )
	{
		printf("SH2: internal reg access rd%i <$%X\n", size, addr);
		//exit(1);
		return 0;
	}
	return memread(addr,size); //,false);
}

void SH2::write(u32 addr, u32 val, int size)
{
	if( size == 8 ) val &= 0xff;
	else if( size == 16 ) val &= 0xffff;

	if( addr >= 0xFFFFfe00 )
	{
		printf("SH2: internal reg access wr%i $%X = $%X\n", size, addr, val);
		//exit(1);
		return;
	}

	memwrite(addr, val, size);
}

