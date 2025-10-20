#include <print>
#include <cstring>
#include <cmath>
#include "sh4.h"

struct sh4_instr { u16 mask,res; std::function<void(sh4&,u16)> func; std::string name, decode; };
#define INSTR [](sh4& cpu, u16 opc) 
#define Rn cpu.r[(opc>>8)&15]
#define Rm cpu.r[(opc>>4)&15]
#define R0 cpu.r[0]
#define NeqM ( ((opc>>8)&15) == ((opc>>4)&15) )
#define srT cpu.sr.b.T
//#define PRIV if( cpu.sr.b.MD==0 ) return cpu.priv()
#define PRIV
//extern u32 cyc;
//#define DELAY cyc+=1; cpu.exec(cpu.fetch(cpu.pc))
#define DELAY cpu.exec(cpu.fetch(cpu.pc))
#define FDCHK if( cpu.sr.b.FD==1 ) return cpu.fpu_disabled()
#define FRn cpu.fpu.f[(((opc>>8)&15)^1)+(cpu.fpctrl.b.FR<<4)]
#define FRm cpu.fpu.f[(((opc>>4)&15)^1)+(cpu.fpctrl.b.FR<<4)]
#define DRn cpu.fpu.d[((opc>>9)&7)+(cpu.fpctrl.b.FR<<3)]
#define DRm cpu.fpu.d[((opc>>5)&7)+(cpu.fpctrl.b.FR<<3)]
#define XRn cpu.fpu.d[((opc>>9)&7)+(cpu.fpctrl.b.FR<<3)^8]
#define XRm cpu.fpu.d[((opc>>5)&7)+(cpu.fpctrl.b.FR<<3)^8]

sh4_instr sh4_opcodes[] = {
	// integer movs
{ 0xF000, 0xE000, INSTR { Rn = s8(opc); }, "mov #imm, Rn", "1110nnnniiiiiiii" },
{ 0xF000, 0x9000, INSTR { Rn = (s16)cpu.read(u8(opc)*2+cpu.pc+4, 16); }, "mov.w @(disp8,pc), Rn", "1001nnnndddddddd"},
{ 0xF000, 0xD000, INSTR { Rn = cpu.read(u8(opc)*4+(cpu.pc&~3)+4, 32); }, "mov.l @(disp8,pc), Rn", "1101nnnndddddddd"},
{ 0xF00F, 0x6003, INSTR { Rn = Rm; }, "mov Rm,Rn", "0110nnnnmmmm0011" },
{ 0xF00F, 0x2000, INSTR { cpu.write(Rn, Rm, 8); }, "mov.b Rm,@Rn", "0010nnnnmmmm0000" },
{ 0xF00F, 0x2001, INSTR { cpu.write(Rn, Rm, 16); }, "mov.w Rm,@Rn", "0010nnnnmmmm0001" },
{ 0xF00F, 0x2002, INSTR { cpu.write(Rn, Rm, 32); }, "mov.l Rm,@Rn", "0010nnnnmmmm0010" },
{ 0xF00F, 0x6000, INSTR { Rn=(s8)cpu.read(Rm,8); }, "mov.b @Rm,Rn", "0110nnnnmmmm0000" },
{ 0xF00F, 0x6001, INSTR { Rn=(s16)cpu.read(Rm,16); }, "mov.w @Rm,Rn", "0110nnnnmmmm0001" },
{ 0xF00F, 0x6002, INSTR { Rn= cpu.read(Rm,32); }, "mov.l @Rm,Rn", "0110nnnnmmmm0010" },
{ 0xF00F, 0x2004, INSTR { cpu.write(Rn-1,Rm,8); Rn-=1; }, "mov.b Rm,@-Rn", "0010nnnnmmmm0100"},
{ 0xF00F, 0x2005, INSTR { cpu.write(Rn-2,Rm,16); Rn-=2; }, "mov.w Rm,@-Rn", "0010nnnnmmmm0101"},
{ 0xF00F, 0x2006, INSTR { cpu.write(Rn-4,Rm,32); Rn-=4; }, "mov.l Rm,@-Rn", "0010nnnnmmmm0110"},
{ 0xF00F, 0x6004, INSTR { Rn =(s8)cpu.read(Rm,8); if(!NeqM) Rm += 1; }, "mov.b @Rm+,Rn", "0110nnnnmmmm0100"},
{ 0xF00F, 0x6005, INSTR { Rn =(s16)cpu.read(Rm,16); if(!NeqM) Rm += 2; }, "mov.w @Rm+,Rn", "0110nnnnmmmm0101"},
{ 0xF00F, 0x6006, INSTR { Rn = cpu.read(Rm,32); if(!NeqM) Rm += 4; }, "mov.l @Rm+,Rn", "0110nnnnmmmm0110"},
{ 0xFF00, 0x8000, INSTR { cpu.write(Rm+(opc&15), R0, 8); }, "mov.b R0,@(disp,Rn)", "10000000nnnndddd"},
{ 0xFF00, 0x8100, INSTR { cpu.write(Rm+(opc&15)*2, R0, 16); }, "mov.w R0,@(disp,Rn)", "10000001nnnndddd"},
{ 0xF000, 0x1000, INSTR { cpu.write(Rn+(opc&15)*4, Rm, 32); }, "mov.l Rm,@(disp,Rn)", "0001nnnnmmmmdddd"},
{ 0xFF00, 0x8400, INSTR { cpu.r[0]=(s8)cpu.read(Rm+(opc&15),8); }, "mov.b @(disp,Rm),R0", "10000100mmmmdddd"},
{ 0xFF00, 0x8500, INSTR { cpu.r[0]=(s16)cpu.read(Rm+(opc&15)*2,16); }, "mov.w @(disp,Rm),R0", "10000101mmmmdddd"},
{ 0xF000, 0x5000, INSTR { Rn = cpu.read((opc&15)*4+Rm,32); }, "mov.l @(disp,Rm),Rn", "0101nnnnmmmmdddd"},
{ 0xF00F, 0x0004, INSTR { cpu.write(R0+Rn, Rm, 8); }, "mov.b Rm,@(R0,Rn)", "0000nnnnmmmm0100" },
{ 0xF00F, 0x0005, INSTR { cpu.write(R0+Rn, Rm, 16); }, "mov.w Rm,@(R0,Rn)", "0000nnnnmmmm0101" },
{ 0xF00F, 0x0006, INSTR { cpu.write(R0+Rn, Rm, 32); }, "mov.l Rm,@(R0,Rn)", "0000nnnnmmmm0110" },
{ 0xF00F, 0x000C, INSTR { Rn=(s8)cpu.read(R0+Rm,8); }, "mov.b @(R0,Rm),Rn", "0000nnnnmmmm1100" },
{ 0xF00F, 0x000D, INSTR { Rn=(s16)cpu.read(R0+Rm,16); }, "mov.w @(R0,Rm),Rn", "0000nnnnmmmm1101" },
{ 0xF00F, 0x000E, INSTR { Rn= cpu.read(R0+Rm,32); }, "mov.l @(R0,Rm),Rn", "0000nnnnmmmm1110" },
{ 0xFF00, 0xC000, INSTR { cpu.write(u8(opc)+cpu.GBR, R0, 8); }, "mov.b R0,@(disp,GBR)", "11000000dddddddd"},
{ 0xFF00, 0xC100, INSTR { cpu.write(u8(opc)*2+cpu.GBR, R0,16); }, "mov.w R0,@(disp,GBR)", "11000001dddddddd"},
{ 0xFF00, 0xC200, INSTR { cpu.write(u8(opc)*4+cpu.GBR, R0,32); }, "mov.l R0,@(disp,GBR)", "11000010dddddddd"},
{ 0xFF00, 0xC400, INSTR { cpu.r[0]=(s8)cpu.read(u8(opc)+cpu.GBR,8); }, "mov.b @(disp,GBR),R0", "11000100dddddddd"},
{ 0xFF00, 0xC500, INSTR { cpu.r[0]=(s16)cpu.read(u8(opc)*2+cpu.GBR,16); }, "mov.w @(disp,GBR),R0", "11000101dddddddd"},
{ 0xFF00, 0xC600, INSTR { cpu.r[0]= cpu.read(u8(opc)*4+cpu.GBR,32); }, "mov.l @(disp,GBR),R0", "11000110dddddddd"},
{ 0xFF00, 0xC700, INSTR { cpu.r[0] = u8(opc)*4 + (cpu.pc&~3) + 4; }, "mova @(disp,PC),R0", "11000111dddddddd"},
{ 0xF0FF, 0x0029, INSTR { Rn = srT; }, "movt Rn", "0000nnnn00101001" },
{ 0xF00F, 0x6008, INSTR { Rn = (Rm&0xFFFF0000)|((Rm>>8)&0x00ff)|((Rm<<8)&0xff00); }, "swap.b Rm,Rn", "0110nnnnmmmm1000"},
{ 0xF00F, 0x6009, INSTR { Rn = (Rm<<16)|(Rm>>16); }, "swap.w Rm,Rn", "0110nnnnmmmm1001"},
{ 0xF00F, 0x200D, INSTR { Rn = (Rm<<16)|(Rn>>16); }, "xtrct Rm,Rn", "0010nnnnmmmm1101" },

	// integer arithmetic

{ 0xF00F, 0x300C, INSTR { Rn += Rm; }, "add Rm,Rn", "0011nnnnmmmm1100" },
{ 0xF000, 0x7000, INSTR { Rn += s8(opc); }, "add #imm,Rn", "0111nnnniiiiiiii" },
{ 0xF00F, 0x300E, INSTR { u64 t = Rn; t += Rm; t+=srT; srT=(t>>32); Rn=t; }, "addc Rm,Rn", "0011nnnnmmmm1110" },
{0xF00F,0x300F,INSTR{u64 t=Rn; t+=Rm; srT=((t^Rn)&(t^Rm)&BIT(31))?1:0; Rn=t; },"addv Rm,Rn","0011nnnnmmmm1111" },
{ 0xFF00, 0x8800, INSTR { srT = (cpu.r[0]==u32(s8(opc))); }, "cmp/eq #imm,R0", "10001000iiiiiiii" },
{ 0xF00F, 0x3000, INSTR { srT = (Rn==Rm); }, "cmp/eq Rm,Rn", "0011nnnnmmmm0000" },
{ 0xF00F, 0x3002, INSTR { srT = Rn>=Rm; }, "cmp/hs Rm,Rn", "0011nnnnmmmm0010" },
{ 0xF00F, 0x3003, INSTR { srT = s32(Rn)>=s32(Rm); }, "cmp/ge Rm,Rn", "0011nnnnmmmm0011" },
{ 0xF00F, 0x3006, INSTR { srT = Rn>Rm; }, "cmp/hi Rm,Rn", "0011nnnnmmmm0110" },
{ 0xF00F, 0x3007, INSTR { srT = s32(Rn)>s32(Rm); }, "cmp/gt Rm,Rn", "0011nnnnmmmm0111" },
{ 0xF0FF, 0x4011, INSTR { srT = s32(Rn)>=0; }, "cmp/pz Rn", "0100nnnn00010001" },
{ 0xF0FF, 0x4015, INSTR { srT = s32(Rn)>0; }, "cmp/pl Rn", "0100nnnn00010101" },
{ 0xFFFF, 0x0019, INSTR { cpu.sr.b.M = cpu.sr.b.Q = cpu.sr.b.T = 0; }, "div0u", "0000000000011001"},
{ 0xF0FF, 0x4010, INSTR { Rn -= 1; srT = ((Rn==0)?1:0); }, "dt Rn", "0100nnnn00010000" },
{ 0xF00F, 0x600E, INSTR { Rn = s8(Rm); }, "exts.b Rm,Rn", "0110nnnnmmmm1110" },
{ 0xF00F, 0x600F, INSTR { Rn = s16(Rm); }, "exts.w Rm,Rn", "0110nnnnmmmm1111" },
{ 0xF00F, 0x600C, INSTR { Rn = u8(Rm); }, "extu.b Rm,Rn", "0110nnnnmmmm1100" },
{ 0xF00F, 0x600D, INSTR { Rn = u16(Rm); }, "extu.w Rm,Rn", "0110nnnnmmmm1101" },
{ 0xF00F, 0x3008, INSTR { Rn -= Rm; }, "sub Rm,Rn", "0011nnnnmmmm1000" },
{ 0xF00F, 0x200E, INSTR { cpu.MACL = u16(Rn); cpu.MACL *= u16(Rm); }, "mulu.w Rm,Rn", "0010nnnnmmmm1110" },
{ 0xF00F, 0x200F, INSTR { cpu.MACL = (s32(s16(Rn)) * s32(s16(Rm))); }, "muls.w Rm,Rn", "0010nnnnmmmm1111"},
{ 0xF00F, 0x0007, INSTR { cpu.MACL = Rm * Rn; }, "mul.l Rm,Rn", "0000nnnnmmmm0111" },
{ 0xF00F, 0x2007, INSTR { cpu.sr.b.Q = Rn>>31; cpu.sr.b.M = Rm>>31; srT = cpu.sr.b.M^cpu.sr.b.Q; }, "div0s Rm,Rn", "0010nnnnmmmm0111"},
{ 0xF00F, 0x200C, INSTR { srT = ((Rn&0xff)==(Rm&0xff))||((Rn&0xff00)==(Rm&0xff00))
			||((Rn&0xff0000)==(Rm&0xff0000))||((Rn&0xff000000u)==(Rm&0xff000000u)); }, "cmp/str Rm,Rn","0010nnnnmmmm1100"},
{ 0xF00F, 0x3005, INSTR { u64 a = Rn; a *= Rm; cpu.MACH = a>>32; cpu.MACL = a; }, "dmulu.l Rm,Rn", "0011nnnnmmmm0101" },
{ 0xF00F, 0x300A, INSTR { u32 tmp0=Rn; u32 tmp1=Rn-Rm; Rn=tmp1-srT; srT=(tmp0<tmp1||tmp1<Rn); }, "subc Rm,Rn", "0011nnnnmmmm1010" },
{ 0xF00F, 0x300B, INSTR { u32 t = Rn-Rm; srT = ((t^Rn)&(t^(~Rm))&BIT(31))?1:0; Rn=t; }, "subv Rm,Rn", "0011nnnnmmmm1011" },
{ 0xF00F, 0x300D, INSTR { s64 a =s32(Rn); a*=s32(Rm); cpu.MACH=a>>32; cpu.MACL=a; }, "dmuls.l Rm,Rn", "0011nnnnmmmm1101"},
{ 0xF00F, 0x600A, INSTR { u32 temp=0-Rm; Rn=temp-srT; srT=(0<temp||temp<Rn); }, "negc Rm,Rn", "0110nnnnmmmm1010" },
{ 0xF00F, 0x600B, INSTR { Rn = 0 - Rm; }, "neg Rm,Rn", "0110nnnnmmmm1011" },
{ 0xF00F, 0x3004, INSTR { cpu.div1((opc>>4)&15, (opc>>8)&15); }, "div1 Rm,Rn", "0011nnnnmmmm0100"},

	// logic ops
	
{ 0xF00F, 0x2009, INSTR { Rn &= Rm; }, "and Rm,Rn", "0010nnnnmmmm1001" },
{ 0xFF00, 0xC900, INSTR { R0 &=u8(opc); }, "and #imm,R0", "11001001iiiiiiii"},
{ 0xFF00, 0xCD00,INSTR{ cpu.write(R0+cpu.GBR, u8(opc)&cpu.read(R0+cpu.GBR,8), 8); }, "and.b #imm,@(R0,GBR)","11001101iiiiiiii" },
{ 0xF00F, 0x6007, INSTR { Rn = ~Rm; }, "not Rm,Rn", "0110nnnnmmmm0111"},
{ 0xF00F, 0x200B, INSTR { Rn |= Rm; }, "or Rm,Rn", "0010nnnnmmmm1011" },
{ 0xFF00, 0xCB00, INSTR { R0 |= u8(opc); }, "or #imm,R0", "11001011iiiiiiii" },
{ 0xFF00, 0xCF00, INSTR { cpu.write(R0+cpu.GBR, u8(opc)|cpu.read(R0+cpu.GBR,8), 8); }, "or.b #imm,@(R0,GBR)","11001111iiiiiiii"},
{ 0xF0FF, 0x401B, INSTR { u8 t=cpu.read(Rn,8); srT=(t==0); cpu.write(Rn,t|0x80,8); }, "tas.b @Rn", "0100nnnn00011011"},
{ 0xF00F, 0x2008, INSTR { srT = ((Rn&Rm)==0); }, "tst Rm,Rn", "0010nnnnmmmm1000" },
{ 0xFF00, 0xC800, INSTR { srT = ((R0&u8(opc))==0); }, "tst #imm,R0", "11001000iiiiiiii" },
{ 0xFF00, 0xCC00, INSTR { srT = ((u8(opc)&cpu.read(R0+cpu.GBR,8))==0); }, "tst.b #imm,@(R0,GBR)", "11001100iiiiiiii"},
{ 0xF00F, 0x200A, INSTR { Rn ^= Rm; }, "xor Rm,Rn", "0010nnnnmmmm1010" },
{ 0xFF00, 0xCA00, INSTR { R0 ^= u8(opc); }, "xor #imm,R0", "11001010iiiiiiii" },
{ 0xFF00, 0xCE00,INSTR { cpu.write(R0+cpu.GBR, u8(opc)^cpu.read(R0+cpu.GBR,8), 8); }, "xor.b #imm,@(R0,GBR)","11001110iiiiiiii"},

	// shifts
	
{ 0xF0FF, 0x4004, INSTR { srT = Rn>>31; Rn = (Rn<<1)|(Rn>>31); }, "rotl Rn", "0100nnnn0000100" },
{ 0xF0FF, 0x4005, INSTR { srT = Rn&1; Rn = (Rn>>1)|(Rn<<31); }, "rotr Rn", "0100nnnn00000101" },
{ 0xF0FF, 0x4024, INSTR { u32 t = srT; srT = Rn>>31; Rn = (Rn<<1)|t; }, "rotcl Rn", "0100nnnn00100100"},
{ 0xF0FF, 0x4025, INSTR { u32 t = srT<<31; srT = Rn&1; Rn = (Rn>>1)|t; }, "rotcr Rn", "0100nnnn00100101"},
{ 0xF0FF, 0x4020, INSTR { srT=Rn>>31; Rn<<=1; }, "shal Rn", "0100nnnn00100000" },
{ 0xF0FF, 0x4021, INSTR { srT=Rn&1; Rn=s32(Rn)>>1; }, "shar Rn", "0100nnnn00100001" },
{ 0xF0FF, 0x4000, INSTR { srT=Rn>>31; Rn<<=1; }, "shll Rn", "0100nnnn00000000" },
{ 0xF0FF, 0x4001, INSTR { srT=Rn&1; Rn>>=1; }, "shlr", "0100nnnn00000001" },
{ 0xF0FF, 0x4008, INSTR { Rn <<= 2; }, "shll2 Rn", "0100nnnn00001000" },
{ 0xF0FF, 0x4009, INSTR { Rn >>= 2; }, "shlr2 Rn", "0100nnnn00001001" },
{ 0xF0FF, 0x4018, INSTR { Rn <<= 8; }, "shll8 Rn", "0100nnnn00011000" },
{ 0xF0FF, 0x4019, INSTR { Rn >>= 8; }, "shlr8 Rn", "0100nnnn00011001" },
{ 0xF0FF, 0x4028, INSTR { Rn <<=16; }, "shll16 Rn","0100nnnn00101000" },
{ 0xF0FF, 0x4029, INSTR { Rn >>=16; }, "shlr16 Rn","0100nnnn00101001" },
{ 0xF00F, 0x400D, INSTR {
			if( s32(Rm) >= 0 )
			{
				Rn <<= (Rm&0x1F);
			} else {
				u32 a = ((0x1F&~Rm)+1);
				if( a >= 32 ) 
				{ 
					Rn = 0; 
				} else {
					Rn >>= a;
				}
			}
		}, "shld Rm,Rn", "0100nnnnmmmm1101" },
{ 0xF00F, 0x400C, INSTR {
			if( s32(Rm) >= 0 )
			{
				Rn <<= (Rm&0x1F);
			} else {
				u32 a = ((0x1F&~Rm)+1);
				if( a >= 32 ) 
				{ 
					Rn = (Rn&BIT(31))? 0xffffFFFFu:0; 
				} else {
					Rn = s32(Rn) >> a;
				}
			}
		}, "shld Rm,Rn", "0100nnnnmmmm1101" },

	// branches
	
{ 0xFF00, 0x8B00, INSTR { if( srT == 0 ) { cpu.pc += s8(opc)*2 + 2; }}, "bf label", "10001011dddddddd"},
{ 0xFF00, 0x8F00, INSTR{if(srT==0){ cpu.pc += 2; DELAY; cpu.pc += s8(opc)*2; }}, "bf/s label", "10001111dddddddd" },
{ 0xFF00, 0x8900, INSTR { if( srT == 1 ) { cpu.pc += s8(opc)*2 + 2; }}, "bt label", "10001001dddddddd"},
{ 0xFF00, 0x8D00, INSTR{if(srT==1){ cpu.pc += 2; DELAY; cpu.pc += s8(opc)*2; }}, "bt/s label", "10001101dddddddd" },
{ 0xF000, 0xA000, INSTR { cpu.pc += 2; DELAY; cpu.pc +=(s16(opc<<4)>>3); }, "bra label","1010dddddddddddd"},
{ 0xF0FF, 0x0023, INSTR { cpu.pc += 2; u32 t = Rn; DELAY; cpu.pc += t; }, "braf Rn", "0000nnnn00100011" },
{ 0xF000, 0xB000,INSTR{ cpu.pc += 2; DELAY; cpu.PR=cpu.pc+2; cpu.pc+=(s16(opc<<4)>>3); },"bsr label","1011dddddddddddd"},
{ 0xF0FF, 0x0003, INSTR { cpu.PR=cpu.pc+4; cpu.pc+=2; u32 t=Rn; DELAY; cpu.pc+=t; }, "bsrf Rn","0000nnnn00000011"},
{ 0xF0FF, 0x402B, INSTR { cpu.pc+=2; u32 t=Rn; DELAY; cpu.pc=t-2; }, "jmp @Rn", "0100nnnn00101011"},
{ 0xF0FF, 0x400B, INSTR { cpu.pc+=2; u32 t=Rn; DELAY; cpu.PR=cpu.pc+2; cpu.pc=t-2; }, "jsr @Rn", "0100nnnn00101011"},
{ 0xFFFF, 0x000B, INSTR { cpu.pc+=2; u32 t=cpu.PR; DELAY; cpu.pc=t-2; }, "rts", "0000000000001011" },
{ 0xFFFF, 0x002B, INSTR { PRIV; cpu.pc+=2; u32 t=cpu.SPC; u32 m=cpu.SSR; DELAY; cpu.pc=t-2; cpu.setSR(m); }, "rte", "0000000000101011" },

	// system control
	
{ 0xFFFF, 0x001B, INSTR { cpu.sleeping = true; }, "sleep", "0000000000011011" },
{ 0xFFFF, 0x0028, INSTR { cpu.MACH = cpu.MACL = 0; }, "clrmac", "0000000000101000" },
{ 0xFFFF, 0x0048, INSTR { cpu.sr.b.S = 0; }, "clrs", "0000000001001000" },
{ 0xFFFF, 0x0008, INSTR { srT = 0; }, "clrt", "0000000000001000" },
{ 0xF0FF, 0x400E, INSTR { PRIV; cpu.setSR(Rn); }, "ldc Rn,SR", "0100nnnn00001110" },
{ 0xF0FF, 0x401E, INSTR { cpu.GBR = Rn; }, "ldc Rn,GBR", "0100nnnn00011110" },
{ 0xF0FF, 0x402E, INSTR { PRIV; cpu.VBR = Rn; }, "ldc Rn,VBR", "0100nnnn00101110" },
{ 0xF0FF, 0x403E, INSTR { PRIV; cpu.SSR = Rn; }, "ldc Rn,SSR", "0100nnnn00111110" },
{ 0xF0FF, 0x404E, INSTR { PRIV; cpu.SPC = Rn; }, "ldc Rn,SPC", "0100nnnn01001110" },
{ 0xF0FF, 0x40FA, INSTR { PRIV; cpu.DBR = Rn; }, "ldc Rn,DBR", "0100nnnn11111010" },
{ 0xF0FF, 0x4007, INSTR { PRIV; u32 t = cpu.read(Rn,32); Rn+=4; cpu.setSR(t); }, "ldc.l @Rn+,SR", "0100nnnn00000111"},
{ 0xF0FF, 0x4017, INSTR { cpu.GBR = cpu.read(Rn,32); Rn+=4; }, "ldc.l, @Rn+,GBR", "0100nnnn00010111"},
{ 0xF0FF, 0x4027, INSTR { PRIV; cpu.VBR = cpu.read(Rn,32); Rn+=4; }, "ldc.l, @Rn+,VBR", "0100nnnn00100111"},
{ 0xF0FF, 0x4037, INSTR { PRIV; cpu.SSR = cpu.read(Rn,32); Rn+=4; }, "ldc.l, @Rn+,SSR", "0100nnnn00110111"},
{ 0xF0FF, 0x4047, INSTR { PRIV; cpu.SPC = cpu.read(Rn,32); Rn+=4; }, "ldc.l, @Rn+,SPC", "0100nnnn01000111"},
{ 0xF0FF, 0x40F6, INSTR { PRIV; cpu.DBR = cpu.read(Rn,32); Rn+=4; }, "ldc.l, @Rn+,DBR", "0100nnnn11110110"},
{ 0xF0FF, 0x400A, INSTR { cpu.MACH = Rn; }, "lds Rn,MACH", "0100nnnn00001010" },
{ 0xF0FF, 0x401A, INSTR { cpu.MACL = Rn; }, "lds Rn,MACL", "0100nnnn00011010" },
{ 0xF0FF, 0x402A, INSTR { cpu.PR = Rn; }, "lds Rn,PR", "0100nnnn00101010" },
{ 0xF0FF, 0x4006, INSTR { cpu.MACH = cpu.read(Rn,32); Rn+=4; }, "lds.l @Rm+,MACH", "0100nnnn00000110" },
{ 0xF0FF, 0x4016, INSTR { cpu.MACL = cpu.read(Rn,32); Rn+=4; }, "lds.l @Rm+,MACL", "0100nnnn00010110" },
{ 0xF0FF, 0x4026, INSTR { cpu.PR = cpu.read(Rn,32); Rn+=4; }, "lds.l @Rm+,PR", "0100nnnn00100110" },
{ 0xFFFF, 0x0009, INSTR {}, "nop", "0000000000001001" },
{ 0xFFFF, 0x0058, INSTR { cpu.sr.b.S = 1; }, "sets", "0000000001011000" },
{ 0xFFFF, 0x0018, INSTR { srT = 1; }, "sett", "0000000000011000" },
{ 0xF0FF, 0x0002, INSTR { PRIV; Rn=cpu.sr.v; }, "stc SR,Rn", "0000nnnn00000010" },
{ 0xF0FF, 0x0012, INSTR { Rn=cpu.GBR; }, "stc GBR,Rn", "0000nnnn00010010" },
{ 0xF0FF, 0x0022, INSTR { PRIV; Rn=cpu.VBR; }, "stc VBR,Rn", "0000nnnn00100010" },
{ 0xF0FF, 0x0032, INSTR { PRIV; Rn=cpu.SSR; }, "stc SSR,Rn", "0000nnnn00110010" },
{ 0xF0FF, 0x0042, INSTR { PRIV; Rn=cpu.SPC; }, "stc SPC,Rn", "0000nnnn01000010" },
{ 0xF0FF, 0x003A, INSTR { PRIV; Rn=cpu.SGR; }, "stc SGR,Rn", "0000nnnn00111010" },
{ 0xF0FF, 0x00FA, INSTR { PRIV; Rn=cpu.DBR; }, "stc DBR,Rn", "0000nnnn11111010" },
{ 0xF0FF, 0x4003, INSTR { PRIV; Rn-=4; cpu.write(Rn, cpu.sr.v, 32); }, "stc.l SR,@-Rn", "0100nnnn00000011"},
{ 0xF0FF, 0x4013, INSTR { Rn-=4; cpu.write(Rn, cpu.GBR, 32); }, "stc.l GBR,@-Rn", "0100nnnn00010011"},
{ 0xF0FF, 0x4023, INSTR { PRIV; Rn-=4; cpu.write(Rn, cpu.VBR, 32); }, "stc.l VBR,@-Rn", "0100nnnn00100011"},
{ 0xF0FF, 0x4033, INSTR { PRIV; Rn-=4; cpu.write(Rn, cpu.SSR, 32); }, "stc.l SSR,@-Rn", "0100nnnn00110011"},
{ 0xF0FF, 0x4043, INSTR { PRIV; Rn-=4; cpu.write(Rn, cpu.SPC, 32); }, "stc.l SPC,@-Rn", "0100nnnn01000011"},
{ 0xF0FF, 0x4032, INSTR { PRIV; Rn-=4; cpu.write(Rn, cpu.SGR, 32); }, "stc.l SGR,@-Rn", "0100nnnn00110010"},
{ 0xF0FF, 0x40F2, INSTR { PRIV; Rn-=4; cpu.write(Rn, cpu.DBR, 32); }, "stc.l DBR,@-Rn", "0100nnnn11110010"},
{ 0xF0FF, 0x000A, INSTR { Rn=cpu.MACH; }, "sts MACH,Rn", "0000nnnn00001010" },
{ 0xF0FF, 0x001A, INSTR { Rn=cpu.MACL; }, "sts MACL,Rn", "0000nnnn00011010" },
{ 0xF0FF, 0x002A, INSTR { Rn=cpu.PR; }, "sts PR,Rn", "0000nnnn00101010" },
{ 0xF0FF, 0x4002, INSTR { PRIV; Rn-=4; cpu.write(Rn, cpu.MACH, 32); }, "stc.l MACH,@-Rn", "0100nnnn00000010"},
{ 0xF0FF, 0x4012, INSTR { PRIV; Rn-=4; cpu.write(Rn, cpu.MACL, 32); }, "stc.l MACL,@-Rn", "0100nnnn00010010"},
{ 0xF0FF, 0x4022, INSTR { PRIV; Rn-=4; cpu.write(Rn, cpu.PR, 32); }, "stc.l PR,@-Rn", "0100nnnn00100010"},
{ 0xF08F, 0x0082, INSTR { PRIV; Rn = cpu.rbank[(opc>>4)&7]; }, "stc Rm_BANK,Rn", "0000nnnn1mmm0010" },
{ 0xF08F, 0x4083, INSTR { PRIV; Rn-=4; cpu.write(Rn, cpu.rbank[(opc>>4)&7], 32); }, "stc.l Rm_BANK,@-Rn","0100nnnn1mmm0011"},
{ 0xF08F, 0x408E, INSTR { PRIV; cpu.rbank[(opc>>4)&7] = Rn; }, "ldc Rn,Rm_BANK", "0100nnnn1mmm1110" },
{ 0xF08F, 0x4087, INSTR { PRIV; cpu.rbank[(opc>>4)&7] = cpu.read(Rn, 32); Rn += 4; }, "ldc.l @Rn+,Rm_BANK", "0100nnnn1mmm0111"},
{ 0xFF00, 0xC300, INSTR {
		
		cpu.SPC = cpu.pc+2; 
		cpu.SSR = cpu.sr.v;
		cpu.TRA = u8(opc)<<2; 
		cpu.EXPEVT = 0x160;
		cpu.pc = cpu.VBR + 0x100 - 2;
		auto f = cpu.sr;
		f.b.MD = f.b.RB = f.b.BL = 1;
		cpu.setSR(f.v);
		//std::println("${:X}: trapa ${:X}", cpu.SPC-2, cpu.TRA>>2);
	}, "trapa #imm", "11000011iiiiiiii" },

	// fr/fschg
{ 0xFFFF, 0xFBFD, INSTR { cpu.fpctrl.b.FR ^= 1; }, "frchg", "1111101111111101" },
{ 0xFFFF, 0xF3FD, INSTR { cpu.fpctrl.b.SZ ^= 1; }, "fschg", "1111001111111101" },

	// Float ctrl
	
{ 0xF0FF, 0x406A, INSTR { cpu.fpctrl.v = Rn; }, "lds Rn,FPSCR", "0100nnnn01101010" },
{ 0xF0FF, 0x405A, INSTR { cpu.FPUL = Rn; }, "lds Rn,FPUL", "0100nnnn01011010" },
{ 0xF0FF, 0x4066, INSTR { cpu.fpctrl.v = cpu.read(Rn,32); Rn+=4; }, "lds.l @Rn+,FPSCR", "0100nnnn01100110" },
{ 0xF0FF, 0x4056, INSTR { cpu.FPUL = cpu.read(Rn,32); Rn+=4; }, "lds.l @Rn+,FPSCR", "0100nnnn01010110" },
{ 0xF0FF, 0x006A, INSTR { Rn = cpu.fpctrl.v; }, "sts FPSCR,Rn", "0000nnnn01101010" },
{ 0xF0FF, 0x005A, INSTR { Rn = cpu.FPUL; }, "sts FPUL,Rn", "0000nnnn01011010" },
{ 0xF0FF, 0x4062, INSTR { Rn-=4; cpu.write(Rn, cpu.fpctrl.v, 32); }, "sts.l FPSCR,@-Rn", "0100nnnn01100010" },
{ 0xF0FF, 0x4052, INSTR { Rn-=4; cpu.write(Rn, cpu.FPUL, 32); }, "sts.l FPUL,@-Rn", "0100nnnn01010010" },

	// cache
	
{ 0xF0FF, 0x0083, INSTR { cpu.pref(Rn); }, "pref @Rn", "0000nnnn10000011" }, // ??
{ 0xF0FF, 0x0093, INSTR { }, "ocbi @Rn", "0000nnnn10010011" }, // ??
{ 0xF0FF, 0x00A3, INSTR { }, "ocbp @Rn", "0000nnnn10100011" }, // ??
{ 0xF0FF, 0x00B3, INSTR { }, "ocbwb @Rn", "0000nnnn10110011" }, //??
{ 0xF0FF, 0x00C3, INSTR { cpu.write(Rn, R0, 32); }, "movca.l R0,@Rn", "0000nnnn10110011" }, //??
{ 0xFFFF, 0x0038, INSTR { }, "ldtlb", "0000000000111011" }, //??

	// float moves

{ 0xF00F, 0xF006, INSTR {
		if( cpu.fpctrl.b.SZ == 0 )
		{
			FRn = std::bit_cast<float>((u32)cpu.read(Rm+R0,32));
		} else {
			u64 a = cpu.read(Rm+R0,64);
			a = (a>>32)|(a<<32);
			memcpy((opc&BIT(8))?&XRn:&DRn, &a, 8);
		}
	}, "fmov.s @(R0,Rm),FRn", "1111nnnnmmmm0110" },
{ 0xF00F, 0xF007, INSTR {
		if( cpu.fpctrl.b.SZ == 0 )
		{
			cpu.write(Rn+R0,std::bit_cast<u32>(FRm),32);
		} else {
			u64 a = std::bit_cast<u64>((opc&BIT(4))?XRm:DRm);
			cpu.write(Rn+R0,(a<<32)|(a>>32),64);
		}
	}, "fmov.s FRm,@(R0,Rn)", "1111nnnnmmmm0111" },	
{ 0xF00F, 0xF008, INSTR {
		if( cpu.fpctrl.b.SZ == 0 )
		{
			FRn = std::bit_cast<float>((u32)cpu.read(Rm,32));
		} else {
			u64 a = cpu.read(Rm,64);
			a = (a<<32)|(a>>32);
			memcpy((opc&BIT(8))?&XRn:&DRn, &a, 8);
		}
	}, "fmov.s @Rm,FRn", "1111nnnnmmmm1001" },	
{ 0xF00F, 0xF009, INSTR {
		if( cpu.fpctrl.b.SZ == 0 )
		{
			FRn = std::bit_cast<float>((u32)cpu.read(Rm,32));
			Rm += 4;
		} else {
			u64 a = cpu.read(Rm,64);
			a = (a<<32)|(a>>32);
			memcpy((opc&BIT(8))?&XRn:&DRn, &a, 8);
			Rm += 8;
		}
	}, "fmov.s @Rm+,FRn", "1111nnnnmmmm1001" },
{ 0xF00F, 0xF00A, INSTR {
		if( cpu.fpctrl.b.SZ == 0 )
		{
			cpu.write(Rn, std::bit_cast<u32>(FRm), 32);
		} else {
			u64 a = std::bit_cast<u64>((opc&BIT(4))?XRm:DRm);
			cpu.write(Rn, (a<<32)|(a>>32), 64);
		}
	}, "fmov.s FRm,@Rn", "1111nnnnmmmm1010" },
{ 0xF00F, 0xF00B, INSTR {
		if( cpu.fpctrl.b.SZ == 0 )
		{
			Rn -= 4;
			cpu.write(Rn, std::bit_cast<u32>(FRm), 32);
		} else {
			Rn -= 8;
			u64 a = std::bit_cast<u64>((opc&BIT(4))?XRm:DRm);
			cpu.write(Rn, (a<<32)|(a>>32), 64);
		}
	}, "fmov.s FRm,@-Rn", "1111nnnnmmmm1011" },
{ 0xF00F, 0xF00C, INSTR {
		if( cpu.fpctrl.b.SZ == 0 )
		{
			FRn = FRm;
		} else {
			if( opc & BIT(8) )
			{
				XRn = ((opc&BIT(4))?XRm:DRm);
			} else {
				DRn = ((opc&BIT(4))?XRm:DRm);
			}
		}
	}, "fmov.s FRm,FRn", "1111nnnnmmmm1100" },
{ 0xF0FF, 0xF03D, INSTR {
		if( cpu.fpctrl.b.PR == 0 )
		{
			cpu.FPUL = (s32) trunc(FRn);
		} else {
			cpu.FPUL = (s32) trunc(DRn);
		}
		
	}, "ftrc FRn,FPUL", "1111nnnn00111101" },
{ 0xF0FF, 0xF02D, INSTR {
		if( cpu.fpctrl.b.PR == 0 )
		{
			FRn = (s32)cpu.FPUL;
		} else {
			DRn = (s32)cpu.FPUL;
		}
	}, "float FPUL,FRn", "1111nnnn00101101" },
{ 0xF1FF, 0xF0FD, INSTR {
		s32 fraction = cpu.FPUL&0xffff;
		float a = 2*M_PI*float(fraction) / 0x10000;
		u32 n = ((opc>>8)&15)&0xE;
		cpu.fpu.f[(cpu.fpctrl.b.FR<<4)+n+1] = sin(a);
		cpu.fpu.f[(cpu.fpctrl.b.FR<<4)+n] = cos(a);
	}, "fcsa FPUL,DRn", "1111nnn011111101" },
{ 0xF0FF, 0xF08D, INSTR { FRn = 0; }, "fldi0 FRn", "1111nnnn10001101" },
{ 0xF0FF, 0xF09D, INSTR { FRn = 1.0f; }, "fldi1 FRn", "1111nnnn10011101" },
{ 0xF0FF, 0xF0BD, INSTR { if( cpu.fpctrl.b.PR==0 ) { /*todo exception*/ return; } cpu.FPUL = std::bit_cast<u32>((float)DRn); }, "fcnvds DRn,FPUL", "1111nnnn10111101" },
{ 0xF0FF, 0xF01D, INSTR { cpu.FPUL = std::bit_cast<s32>(FRn); }, "flds FRn,FPUL", "1111nnnn00011101"},
{ 0xF0FF, 0xF00D, INSTR { FRn = std::bit_cast<float>(cpu.FPUL);},"fsts FPUL,FRn", "1111nnnn00001101"},


	// float arithmetic
	
{ 0xF00F, 0xF000, INSTR { if( cpu.fpctrl.b.PR == 0 ) { FRn += FRm; } else { DRn += DRm; }}, "fadd FRm,FRn", "1111nnnnmmmm0000" },
{ 0xF00F, 0xF001, INSTR { if( cpu.fpctrl.b.PR == 0 ) { FRn -= FRm; } else { DRn -= DRm; }}, "fsub FRm,FRn", "1111nnnnmmmm0001" },
{ 0xF00F, 0xF002, INSTR { if( cpu.fpctrl.b.PR == 0 ) { FRn *= FRm; } else { DRn *= DRm; }}, "fmul FRm,FRn", "1111nnnnmmmm0010" },
{ 0xF00F, 0xF003, INSTR { if( cpu.fpctrl.b.PR == 0 ) { FRn /= FRm; } else { DRn /= DRm; }}, "fdiv FRm,FRn", "1111nnnnmmmm0011" },
{ 0xF00F, 0xF004, INSTR { if( cpu.fpctrl.b.PR == 0 ) { srT = FRn==FRm; } else { srT =DRn==DRm; }}, "fcmp/eq FRm,FRn", "1111nnnnmmmm0100" },
{ 0xF00F, 0xF005, INSTR { if( cpu.fpctrl.b.PR == 0 ) { srT = FRn > FRm; }else{ srT = DRn > DRm; }},"fcmp/gt FRm,FRn","1111nnnnmmmm0101" },
{ 0xF0FF, 0xF05D, INSTR { FRn = std::bit_cast<float>(std::bit_cast<s32>(FRn)&~BIT(31)); }, "fabs FRn", "1111nnnn01011101"},
{ 0xF0FF, 0xF04D, INSTR { FRn = std::bit_cast<float>(std::bit_cast<s32>(FRn)^BIT(31)); }, "fneg FRn", "1111nnnn01001101"},
{ 0xF0FF, 0xF06D, INSTR { if( cpu.fpctrl.b.PR == 0 ){ FRn = std::sqrt(FRn); } else { DRn = std::sqrt(DRn); }},"fsqrt FRn","1111nnnn01101101"},
{ 0xF00F, 0xF00E, INSTR { FRn += cpu.fpu.f[(cpu.fpctrl.b.FR<<4)+1] * FRm; }, "fmac FR0,FRm,FRn", "1111nnnnmmmm1110" },
{ 0xF3FF, 0xF1FD, INSTR { cpu.ftrv((opc>>8)&0xc); }, "ftrv XMTRX,FVn", "1111nn0111111101" },
{ 0xF0FF, 0xF0ED, INSTR { cpu.fipr((opc>>8)&0xc, ((opc>>8)&3)<<2); }, "fipr FVm,FVn", "1111nnmm11101101" },
{ 0xF1AD, 0xF0AD, INSTR { if( cpu.fpctrl.b.PR == 0 ) { /*todo exception*/return; } DRn = (double)std::bit_cast<float>(cpu.FPUL); }, "fcnvsd FPUL,DRn", "1111nnn010101101" },
{ 0xF0FF, 0xF07D, INSTR { FRn = 1.0f / std::sqrt(FRn); }, "fsrra FRn", "1111nnnn01111101" },

};

void sh4::exec(u16 opc)
{
	opcache[opc](*this, opc);
/*	for(auto& I : opcodes)
	{
		if( (opc & I.mask) == I.res )
		{
			I.func(*this, opc);
			return;
		}
	}
	std::println("${:X}: unimpl opc ${:04X}", pc, opc);
	exit(1);
*/
}

void sh4::step()
{
	u16 opc = fetch(pc);
	exec(opc);
	pc += 2;
}

void sh4::setSR(u32 v)
{
	sh4_sr_t n, o;
	n.v = v;
	o.v = sr.v;
	sr.v = v;
	sr.b.pad1 = sr.b.pad2 = sr.b.pad5 = sr.b.pad12 = 0;
	
	if( o.b.MD != n.b.MD )
	{ // mode switch
		if( n.b.MD == 0 && o.b.RB == 1 )
		{  // entering user mode, and old priv mode was swapped
			swapregs();
			sr.b.RB = 0;
			return;
		}
		if( n.b.MD == 1 && n.b.RB == 1  )
		{  // entering priv mode, and bank1 is/should be active
			swapregs();
			return;
		}
		if( n.b.MD == 0 ) sr.b.RB = 0;
		return;
	}
	
	if( n.b.MD == 1 && (n.b.RB != o.b.RB) )
	{  // staying in priv mode and RB changed
		swapregs();
		return;
	}
	
	if( n.b.MD == 0 ) sr.b.RB = 0;
}

void sh4::swapregs()
{
	for(u32 i = 0; i < 8; ++i)
	{
		std::swap(r[i], rbank[i]);
	}
}

void sh4::priv()
{
	std::println("PRIV!");
	pc = VBR + 0x100 - 2;
}

void sh4::fipr(u32 a, u32 b)
{
	a += (fpctrl.b.FR<<4);
	b += (fpctrl.b.FR<<4);	
	double R = 0;
	for(u32 i = 0; i < 4; ++i) R += double(fpu.f[a+i])*double(fpu.f[b+i]);
	fpu.f[((a+3)^1)] = R;
}

void sh4::ftrv(u32 n)
{
	std::println("FTRV unimpl");
	exit(1);
	float v[4] = { fpu.f[n+1], fpu.f[n+0], fpu.f[n+3], fpu.f[n+2] };
}

void sh4::div1(u32 m, u32 n)
{ // from MAME. only one that passes the SSTs
	const u32 SH_T = 0x00000001;
	const u32 SH_S = 0x00000002;
	const u32 SH_Q = 0x00000100;
	const u32 SH_M = 0x00000200;

	uint32_t old_q = sr.v & SH_Q;
	if (0x80000000 & r[n])
		sr.v |= SH_Q;
	else
		sr.v &= ~SH_Q;

	r[n] = (r[n] << 1) | (sr.v & SH_T);

	if (!old_q)
	{
		if (!(sr.v & SH_M))
		{
			uint32_t tmp = r[n];
			r[n] -= r[m];
			if (!(sr.v & SH_Q))
				if (r[n] > tmp)
					sr.v |= SH_Q;
				else
					sr.v &= ~SH_Q;
			else
				if (r[n] > tmp)
					sr.v &= ~SH_Q;
				else
					sr.v |= SH_Q;
		}
		else
		{
			uint32_t tmp = r[n];
			r[n] += r[m];
			if (!(sr.v & SH_Q))
			{
				if (r[n] < tmp)
					sr.v &= ~SH_Q;
				else
					sr.v |= SH_Q;
			}
			else
			{
				if (r[n] < tmp)
					sr.v |= SH_Q;
				else
					sr.v &= ~SH_Q;
			}
		}
	}
	else
	{
		if (!(sr.v & SH_M))
		{
			uint32_t tmp = r[n];
			r[n] += r[m];
			if (!(sr.v & SH_Q))
				if (r[n] < tmp)
					sr.v |= SH_Q;
				else
					sr.v &= ~SH_Q;
			else
				if (r[n] < tmp)
					sr.v &= ~SH_Q;
				else
					sr.v |= SH_Q;
		}
		else
		{
			uint32_t tmp = r[n];
			r[n] -= r[m];
			if (!(sr.v & SH_Q))
				if (r[n] > tmp)
					sr.v &= ~SH_Q;
				else
					sr.v |= SH_Q;
			else
				if (r[n] > tmp)
					sr.v |= SH_Q;
				else
					sr.v &= ~SH_Q;
		}
	}

	uint32_t tmp = (sr.v & (SH_Q | SH_M));
	if (tmp == 0 || tmp == 0x300) /* if Q == M set T else clear T */
		sr.v |= SH_T;
	else
		sr.v &= ~SH_T;
}

void sh4::init()
{
	for(u32 Op = 0; Op <= 0xffff; ++Op)
	{
		opcache[Op] = [&](sh4&, u16 opc) { std::println("${:X}: Unimpl opc ${:X}", pc, opc); exit(1); };
		for(const auto& t : sh4_opcodes)
		{
			if( (t.mask & Op) == t.res )
			{
				opcache[Op] = t.func;
				break;
			}
		}	
	}
}

