#include <iostream>
#include <string>
#include <array>
#include "6502.h"

// A semi-cycle accurate 6502
// Things not yet supported:
//	- BRK (including interaction between it and irq/nmi at the same time)
//	- CLI/SEI/PLP delay
//	- probably missed some cases where r/w cycle should happen

static void setNZ(c6502& cpu, u8 val)
{
	cpu.P.b.Z = (val==0)?1:0;
	cpu.P.b.N = (val&0x80)?1:0;
}

static u8 adc(c6502& cpu, u8 v1, u8 v2)
{
	if( cpu.allow_decimal && cpu.P.b.D )
	{
		printf("Decimal mode not supported!\n");
		exit(1);
		return 0;
	}
	
	u16 res = v1;
	res += v2;
	res += cpu.P.b.C;
	cpu.P.b.V = ((res ^ v1) & (res ^ v2) & 0x80)?1:0;
	cpu.P.b.C = (res>>8);
	setNZ(cpu, res&0xff);
	return res;
}

static u8 sbc(c6502& cpu, u8 v1, u8 v2)
{
	if( cpu.allow_decimal && cpu.P.b.D )
	{
	
		return 0;
	}
	
	return adc(cpu, v1, v2^0xff);
}

static void cmp(c6502& cpu, u8 v1, u8 v2)
{
	u16 res = v1;
	res += v2^0xff;
	res += 1;
	cpu.P.b.C = (res>>8);
	setNZ(cpu, res&0xff);
}

static void cmp(c6502& cpu, u8 val)
{
	cmp(cpu, cpu.A, val);
}

static u8 adc(c6502& cpu, u8 val) { return adc(cpu, cpu.A, val); }
static u8 sbc(c6502& cpu, u8 val) { return sbc(cpu, cpu.A, val); }

static bool nop(c6502& cpu) { return cpu.cycle!=2; }
static bool sed(c6502& cpu) { cpu.P.b.D = 1; return cpu.cycle!=2; }
static bool sec(c6502& cpu) { cpu.P.b.C = 1; return cpu.cycle!=2; }
static bool sei(c6502& cpu) { cpu.P.b.I = 1; return cpu.cycle!=2; }
static bool cld(c6502& cpu) { cpu.P.b.D = 0; return cpu.cycle!=2; }
static bool clc(c6502& cpu) { cpu.P.b.C = 0; return cpu.cycle!=2; }
static bool cli(c6502& cpu) { cpu.P.b.I = 0; return cpu.cycle!=2; }
static bool clv(c6502& cpu) { cpu.P.b.V = 0; return cpu.cycle!=2; }
static bool tax(c6502& cpu) 
{
	if( cpu.cycle == 2 ) setNZ(cpu,cpu.X=cpu.A); 
	return cpu.cycle!=2; 
}
static bool tay(c6502& cpu) 
{ 
	if( cpu.cycle == 2 ) setNZ(cpu,cpu.Y=cpu.A); 
	return cpu.cycle!=2; 
}
static bool txa(c6502& cpu) 
{ 
	if( cpu.cycle == 2 ) setNZ(cpu,cpu.A=cpu.X); 
	return cpu.cycle!=2; 
}
static bool tya(c6502& cpu) 
{ 
	if( cpu.cycle == 2 ) setNZ(cpu,cpu.A=cpu.Y); 
	return cpu.cycle!=2; 
}
static bool tsx(c6502& cpu) 
{ 
	if( cpu.cycle == 2 ) setNZ(cpu,cpu.X=cpu.S); 
	return cpu.cycle!=2; 
}
static bool txs(c6502& cpu) { cpu.S = cpu.X; return cpu.cycle!=2; }
static bool inx(c6502& cpu) 
{ 
	if( cpu.cycle == 2 ) setNZ(cpu,++cpu.X); 
	return cpu.cycle!=2; 
}
static bool iny(c6502& cpu) 
{ 
	if( cpu.cycle == 2 ) setNZ(cpu,++cpu.Y); 
	return cpu.cycle!=2; 
}
static bool dex(c6502& cpu) 
{ 
	if( cpu.cycle == 2 ) setNZ(cpu,--cpu.X); 
	return cpu.cycle!=2; 
}
static bool dey(c6502& cpu) 
{ 
	if( cpu.cycle == 2 ) setNZ(cpu,--cpu.Y); 
	return cpu.cycle!=2; 
}
static bool jmp_abs(c6502& cpu) 
{ 
	if( cpu.cycle == 3 )
	{
		u16 temp = cpu.read(cpu.PC);
		temp |= cpu.read(cpu.PC+1)<<8;
		cpu.PC = temp;
	}
	return cpu.cycle!=3; 
}
static bool jmp_ind(c6502& cpu) 
{ 
	if( cpu.cycle == 5 )
	{
		u16 oldpc = cpu.PC;
		u16 temp = cpu.read(cpu.PC);
		temp |= cpu.read(cpu.PC+1)<<8;
		cpu.PC = cpu.read(temp)|(cpu.read((temp&0xff00)|((temp+1)&0xff))<<8);
		//printf("$%X: jmp ($%04X) = $%04X\n", oldpc, temp, cpu.PC);
	}
	return cpu.cycle!=5; 
}
static bool jsr(c6502& cpu)
{
	if( cpu.cycle == 6 )
	{
		//u16 oldpc = cpu.PC-1;
		u16 temp = cpu.read(cpu.PC++);
		temp |= cpu.read(cpu.PC) << 8;
		cpu.write(0x100+cpu.S, cpu.PC>>8);
		cpu.S--;
		cpu.write(0x100+cpu.S, cpu.PC);
		cpu.S--;
		cpu.PC = temp;
		//printf("jsr from $%X to $%X\n", oldpc, cpu.PC);
	}
	return cpu.cycle!=6;
}
static bool rts(c6502& cpu)
{
	if( cpu.cycle == 6 )
	{
		cpu.S++;
		u16 temp = cpu.read(0x100+cpu.S);
		cpu.S++;
		temp |= cpu.read(0x100+cpu.S)<<8;
		cpu.PC = temp + 1;
		//printf("rts to $%X\n", cpu.PC);
	}
	return cpu.cycle!=6;
}
static bool rti(c6502& cpu)
{
	if( cpu.cycle == 6 )
	{
		cpu.S++;
		cpu.P.r = cpu.read(0x100+cpu.S);
		cpu.P.b.B = 0;
		cpu.P.b.pad = 1;
		cpu.S++;
		u16 temp = cpu.read(0x100+cpu.S);
		cpu.S++;
		temp |= cpu.read(0x100+cpu.S)<<8;
		cpu.PC = temp;
		//printf("RTI to $%04X\n", cpu.PC);
	}
	return cpu.cycle!=6;
}
static bool pla(c6502& cpu)
{
	if( cpu.cycle == 4 )
	{
		cpu.S++;
		cpu.A = cpu.read(0x100+cpu.S);
		setNZ(cpu,cpu.A);
	}
	return cpu.cycle!=4;
}
static bool plp(c6502& cpu)
{
	if( cpu.cycle == 4 )
	{
		cpu.S++;
		cpu.P.r = cpu.read(0x100+cpu.S);
		cpu.P.b.B = 0;
		cpu.P.b.pad = 1;
	}
	return cpu.cycle!=4;
}
static bool pha(c6502& cpu)
{
	if( cpu.cycle == 3 )
	{
		cpu.write(0x100+cpu.S, cpu.A);
		cpu.S--;
	}
	return cpu.cycle!=3;
}
static bool php(c6502& cpu)
{
	if( cpu.cycle == 3 )
	{
		cpu.write(0x100+cpu.S, cpu.P.r|0x10);
		cpu.S--;
	}
	return cpu.cycle!=3;
}
static bool bit_zp(c6502& cpu)
{
	if( cpu.cycle == 3 )
	{
		u8 t = cpu.read(cpu.read(cpu.PC++));
		cpu.P.b.Z = ((t&cpu.A)==0)?1:0;
		cpu.P.b.V = (t&0x40)?1:0;
		cpu.P.b.N = (t&0x80)?1:0;
	}
	return cpu.cycle!=3;
}
static bool bit_abs(c6502& cpu)
{
	if( cpu.cycle == 4 )
	{
		u16 addr = cpu.read(cpu.PC++);
		addr |= cpu.read(cpu.PC++)<<8;
		u8 t = cpu.read(addr);
		//printf("BIT read $%X = $%X\n", addr, t);
		cpu.P.b.Z = ((t&cpu.A)==0)?1:0;
		cpu.P.b.V = (t&0x40)?1:0;
		cpu.P.b.N = (t&0x80)?1:0;
	}
	return cpu.cycle!=4;
}
static bool bcc(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t8 = cpu.read(cpu.PC++); return cpu.P.b.C == 0;
	case 3: cpu.t16 = cpu.PC + (s8)cpu.t8;
		std::swap(cpu.t16, cpu.PC);
		return ((cpu.t16&0xff00)!=(cpu.PC&0xff00));
	case 4: return false;
	}
	return false;
}
static bool bcs(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t8 = cpu.read(cpu.PC++); return cpu.P.b.C != 0;
	case 3: cpu.t16 = cpu.PC + (s8)cpu.t8;
		std::swap(cpu.t16, cpu.PC);
		return ((cpu.t16&0xff00)!=(cpu.PC&0xff00));
	case 4: return false;
	}
	return false;
}
static bool bvc(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t8 = cpu.read(cpu.PC++); return cpu.P.b.V == 0;
	case 3: cpu.t16 = cpu.PC + (s8)cpu.t8;
		std::swap(cpu.t16, cpu.PC);
		return ((cpu.t16&0xff00)!=(cpu.PC&0xff00));
	case 4: return false;
	}
	return false;
}
static bool bvs(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t8 = cpu.read(cpu.PC++); return cpu.P.b.V != 0;
	case 3: cpu.t16 = cpu.PC + (s8)cpu.t8;
		std::swap(cpu.t16, cpu.PC);
		return ((cpu.t16&0xff00)!=(cpu.PC&0xff00));
	case 4: return false;
	}
	return false;
}
static bool bne(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t8 = cpu.read(cpu.PC++); return cpu.P.b.Z == 0;
	case 3: cpu.t16 = cpu.PC + (s8)cpu.t8;
		std::swap(cpu.t16, cpu.PC);
		return ((cpu.t16&0xff00)!=(cpu.PC&0xff00));
	case 4: return false;
	}
	return false;
}
static bool beq(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t8 = cpu.read(cpu.PC++); return cpu.P.b.Z != 0;
	case 3: cpu.t16 = cpu.PC + (s8)cpu.t8;
		std::swap(cpu.t16, cpu.PC);
		return ((cpu.t16&0xff00)!=(cpu.PC&0xff00));
	case 4: return false;
	}
	return false;
}
static bool bpl(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t8 = cpu.read(cpu.PC++); /*printf("$%X: bpl would be to $%X\n", cpu.PC-1, cpu.PC+(s8)cpu.t8);*/ return cpu.P.b.N == 0;
	case 3: cpu.t16 = cpu.PC + (s8)cpu.t8;
		std::swap(cpu.t16, cpu.PC);
		//printf("$%X: bpl to $%X\n", cpu.t16, cpu.PC);
		return ((cpu.t16&0xff00)!=(cpu.PC&0xff00));
	case 4: return false;
	}
	return false;
}
static bool bmi(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t8 = cpu.read(cpu.PC++); return cpu.P.b.N != 0;
	case 3: cpu.t16 = cpu.PC + (s8)cpu.t8;
		std::swap(cpu.t16, cpu.PC);
		return ((cpu.t16&0xff00)!=(cpu.PC&0xff00));
	case 4: return false;
	}
	return false;
}
static bool lda_imm(c6502& cpu)
{
	if( cpu.cycle == 2 ) setNZ(cpu, cpu.A = cpu.read(cpu.PC++));
	return cpu.cycle!=2;
}
static bool lda_zp(c6502& cpu)
{
	if( cpu.cycle == 3 ) setNZ(cpu, cpu.A = cpu.read(cpu.read(cpu.PC++)));
	return cpu.cycle!=3;
}
static bool lda_zpx(c6502& cpu)
{
	if( cpu.cycle == 2 ) cpu.t8 = cpu.read(cpu.PC++);
	else if( cpu.cycle == 3 ) cpu.read(cpu.t8);
	else if( cpu.cycle == 4 ) setNZ(cpu, cpu.A = cpu.read(0xff&(cpu.X+cpu.t8)));
	return cpu.cycle!=4;
}
static bool lda_abs(c6502& cpu)
{
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		setNZ(cpu, cpu.A = cpu.read(cpu.t16));
	}
	return cpu.cycle!=4;
}
static bool lda_abs_x(c6502& cpu)
{
	if( cpu.cycle <= 3 ) return true;
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		cpu.A = cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.X)&0xff));
		setNZ(cpu, cpu.A);
		return ((cpu.t16&0xff00)|((cpu.t16+cpu.X)&0xff)) != (cpu.t16+cpu.X);
	}
	if( cpu.cycle == 5 )
	{
		cpu.A = cpu.read(cpu.t16 + cpu.X);
		setNZ(cpu, cpu.A);
	}
	return false;
}
static bool lda_abs_y(c6502& cpu)
{
	if( cpu.cycle <= 3 ) return true;
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		cpu.A = cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.Y)&0xff));
		setNZ(cpu, cpu.A);
		return ((cpu.t16&0xff00)|((cpu.t16+cpu.Y)&0xff)) != (cpu.t16+cpu.Y);
	}
	if( cpu.cycle == 5 )
	{
		cpu.A = cpu.read(cpu.t16 + cpu.Y);
		setNZ(cpu, cpu.A);
	}
	return false;
}
static bool lda_ind_x(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t8 = cpu.read(cpu.PC++); return true;
	case 3: cpu.read(cpu.t8); cpu.t8 += cpu.X; return true;
	case 4: cpu.t16 = cpu.read(cpu.t8); return true;
	case 5: cpu.t16 |= cpu.read(0xff&(cpu.t8+1))<<8; return true;
	case 6: setNZ(cpu, cpu.A=cpu.read(cpu.t16)); return false;	
	}
	return false;
}
static bool lda_ind_y(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t8 = cpu.read(cpu.PC++); return true;
	case 3: cpu.t16 = cpu.read(cpu.t8); return true;
	case 4: cpu.t16 |= cpu.read(0xff&(cpu.t8+1))<<8; return true;
	case 5: cpu.t8 = cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.Y)&0xff)); 
		if( (cpu.t16&0xff00) != ((cpu.t16+cpu.Y)&0xff00) ) return true;
		else setNZ(cpu, cpu.A=cpu.t8);
		return false;
	case 6: setNZ(cpu, cpu.A = cpu.read(cpu.t16+cpu.Y)); return false;
	}
	return false;
}
static bool and_imm(c6502& cpu)
{
	if( cpu.cycle == 2 ) setNZ(cpu, cpu.A &= cpu.read(cpu.PC++));
	return cpu.cycle!=2;
}
static bool and_zp(c6502& cpu)
{
	if( cpu.cycle == 3 ) setNZ(cpu, cpu.A &= cpu.read(cpu.read(cpu.PC++)));
	return cpu.cycle!=3;
}
static bool and_zpx(c6502& cpu)
{
	if( cpu.cycle == 2 ) cpu.t8 = cpu.read(cpu.PC++);
	else if( cpu.cycle == 3 ) cpu.read(cpu.t8);
	else if( cpu.cycle == 4 ) setNZ(cpu, cpu.A &= cpu.read(0xff&(cpu.X+cpu.t8)));
	return cpu.cycle!=4;
}
static bool and_abs(c6502& cpu)
{
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		setNZ(cpu, cpu.A &= cpu.read(cpu.t16));
	}
	return cpu.cycle!=4;
}
static bool and_abs_x(c6502& cpu)
{
	if( cpu.cycle <= 3 ) return true;
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		if( ((cpu.t16&0xff00)|((cpu.t16+cpu.X)&0xff)) != (cpu.t16+cpu.X) )
		{
			cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.X)&0xff));
			return true;
		}
		cpu.A &= cpu.read(cpu.t16+cpu.X);
		setNZ(cpu, cpu.A);
		return false;
	}
	if( cpu.cycle == 5 )
	{
		cpu.A &= cpu.read(cpu.t16 + cpu.X);
		setNZ(cpu, cpu.A);
	}
	return false;
}
static bool and_abs_y(c6502& cpu)
{
	if( cpu.cycle <= 3 ) return true;
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		if( ((cpu.t16&0xff00)|((cpu.t16+cpu.Y)&0xff)) != (cpu.t16+cpu.Y) )
		{
			cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.Y)&0xff));
			return true;
		}
		cpu.A &= cpu.read(cpu.t16+cpu.Y);
		setNZ(cpu, cpu.A);
		return false;
	}
	if( cpu.cycle == 5 )
	{
		cpu.A &= cpu.read(cpu.t16 + cpu.Y);
		setNZ(cpu, cpu.A);
	}
	return false;
}
static bool and_ind_x(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t8 = cpu.read(cpu.PC++); return true;
	case 3: cpu.read(cpu.t8); cpu.t8 += cpu.X; return true;
	case 4: cpu.t16 = cpu.read(cpu.t8); return true;
	case 5: cpu.t16 |= cpu.read(0xff&(cpu.t8+1))<<8; return true;
	case 6: setNZ(cpu, cpu.A&=cpu.read(cpu.t16)); return false;	
	}
	return false;
}
static bool and_ind_y(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t8 = cpu.read(cpu.PC++); return true;
	case 3: cpu.t16 = cpu.read(cpu.t8); return true;
	case 4: cpu.t16 |= cpu.read(0xff&(cpu.t8+1))<<8; return true;
	case 5: cpu.t8 = cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.Y)&0xff)); 
		if( (cpu.t16&0xff00) != ((cpu.t16+cpu.Y)&0xff00) ) return true;
		else setNZ(cpu, cpu.A&=cpu.t8);
		return false;
	case 6: setNZ(cpu, cpu.A &= cpu.read(cpu.t16+cpu.Y)); return false;
	}
	return false;
}
static bool or_imm(c6502& cpu)
{
	if( cpu.cycle == 2 ) setNZ(cpu, cpu.A |= cpu.read(cpu.PC++));
	return cpu.cycle!=2;
}
static bool or_zp(c6502& cpu)
{
	if( cpu.cycle == 3 ) setNZ(cpu, cpu.A |= cpu.read(cpu.read(cpu.PC++)));
	return cpu.cycle!=3;
}
static bool or_zpx(c6502& cpu)
{
	if( cpu.cycle == 2 ) cpu.t8 = cpu.read(cpu.PC++);
	else if( cpu.cycle == 3 ) cpu.read(cpu.t8);
	else if( cpu.cycle == 4 ) setNZ(cpu, cpu.A |= cpu.read(0xff&(cpu.X+cpu.t8)));
	return cpu.cycle!=4;
}
static bool or_abs(c6502& cpu)
{
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		setNZ(cpu, cpu.A |= cpu.read(cpu.t16));
	}
	return cpu.cycle!=4;
}
static bool or_abs_x(c6502& cpu)
{
	if( cpu.cycle <= 3 ) return true;
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		if( ((cpu.t16&0xff00)|((cpu.t16+cpu.X)&0xff)) != (cpu.t16+cpu.X) )
		{
			cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.X)&0xff));
			return true;
		}
		cpu.A |= cpu.read(cpu.t16+cpu.X);
		setNZ(cpu, cpu.A);
		return false;
	}
	if( cpu.cycle == 5 )
	{
		cpu.A |= cpu.read(cpu.t16 + cpu.X);
		setNZ(cpu, cpu.A);
	}
	return false;
}
static bool or_abs_y(c6502& cpu)
{
	if( cpu.cycle <= 3 ) return true;
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		if( ((cpu.t16&0xff00)|((cpu.t16+cpu.Y)&0xff)) != (cpu.t16+cpu.Y) )
		{
			cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.Y)&0xff));
			return true;
		}
		cpu.A |= cpu.read(cpu.t16+cpu.Y);
		setNZ(cpu, cpu.A);
		return false;
	}
	if( cpu.cycle == 5 )
	{
		cpu.A |= cpu.read(cpu.t16 + cpu.Y);
		setNZ(cpu, cpu.A);
	}
	return false;
}
static bool or_ind_x(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t8 = cpu.read(cpu.PC++); return true;
	case 3: cpu.read(cpu.t8); cpu.t8 += cpu.X; return true;
	case 4: cpu.t16 = cpu.read(cpu.t8); return true;
	case 5: cpu.t16 |= cpu.read(0xff&(cpu.t8+1))<<8; return true;
	case 6: setNZ(cpu, cpu.A|=cpu.read(cpu.t16)); return false;	
	}
	return false;
}
static bool or_ind_y(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t8 = cpu.read(cpu.PC++); return true;
	case 3: cpu.t16 = cpu.read(cpu.t8); return true;
	case 4: cpu.t16 |= cpu.read(0xff&(cpu.t8+1))<<8; return true;
	case 5: cpu.t8 = cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.Y)&0xff)); 
		if( (cpu.t16&0xff00) != ((cpu.t16+cpu.Y)&0xff00) ) return true;
		else setNZ(cpu, cpu.A|=cpu.t8);
		return false;
	case 6: setNZ(cpu, cpu.A |= cpu.read(cpu.t16+cpu.Y)); return false;
	}
	return false;
}
static bool eor_imm(c6502& cpu)
{
	if( cpu.cycle == 2 ) setNZ(cpu, cpu.A ^= cpu.read(cpu.PC++));
	return cpu.cycle!=2;
}
static bool eor_zp(c6502& cpu)
{
	if( cpu.cycle == 3 ) setNZ(cpu, cpu.A ^= cpu.read(cpu.read(cpu.PC++)));
	return cpu.cycle!=3;
}
static bool eor_zpx(c6502& cpu)
{
	if( cpu.cycle == 2 ) cpu.t8 = cpu.read(cpu.PC++);
	else if( cpu.cycle == 3 ) cpu.read(cpu.t8);
	else if( cpu.cycle == 4 ) setNZ(cpu, cpu.A ^= cpu.read(0xff&(cpu.X+cpu.t8)));
	return cpu.cycle!=4;
}
static bool eor_abs(c6502& cpu)
{
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		setNZ(cpu, cpu.A ^= cpu.read(cpu.t16));
	}
	return cpu.cycle!=4;
}
static bool eor_abs_x(c6502& cpu)
{
	if( cpu.cycle <= 3 ) return true;
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		if( ((cpu.t16&0xff00)|((cpu.t16+cpu.X)&0xff)) != (cpu.t16+cpu.X) )
		{
			cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.X)&0xff));
			return true;
		}
		cpu.A ^= cpu.read(cpu.t16+cpu.X);
		setNZ(cpu, cpu.A);
		return false;
	}
	if( cpu.cycle == 5 )
	{
		cpu.A ^= cpu.read(cpu.t16 + cpu.X);
		setNZ(cpu, cpu.A);
	}
	return false;
}
static bool eor_abs_y(c6502& cpu)
{
	if( cpu.cycle <= 3 ) return true;
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		if( ((cpu.t16&0xff00)|((cpu.t16+cpu.Y)&0xff)) != (cpu.t16+cpu.Y) )
		{
			cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.Y)&0xff));
			return true;
		}
		cpu.A ^= cpu.read(cpu.t16+cpu.Y);
		setNZ(cpu, cpu.A);
		return false;
	}
	if( cpu.cycle == 5 )
	{
		cpu.A ^= cpu.read(cpu.t16 + cpu.Y);
		setNZ(cpu, cpu.A);
	}
	return false;
}
static bool eor_ind_x(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t8 = cpu.read(cpu.PC++); return true;
	case 3: cpu.read(cpu.t8); cpu.t8 += cpu.X; return true;
	case 4: cpu.t16 = cpu.read(cpu.t8); return true;
	case 5: cpu.t16 |= cpu.read(0xff&(cpu.t8+1))<<8; return true;
	case 6: setNZ(cpu, cpu.A^=cpu.read(cpu.t16)); return false;	
	}
	return false;
}
static bool eor_ind_y(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t8 = cpu.read(cpu.PC++); return true;
	case 3: cpu.t16 = cpu.read(cpu.t8); return true;
	case 4: cpu.t16 |= cpu.read(0xff&(cpu.t8+1))<<8; return true;
	case 5: cpu.t8 = cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.Y)&0xff)); 
		if( (cpu.t16&0xff00) != ((cpu.t16+cpu.Y)&0xff00) ) return true;
		else setNZ(cpu, cpu.A^=cpu.t8);
		return false;
	case 6: setNZ(cpu, cpu.A ^= cpu.read(cpu.t16+cpu.Y)); return false;
	}
	return false;
}
static bool sty_zp(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t8 = cpu.read(cpu.PC++); return true;
	case 3: cpu.write(cpu.t8, cpu.Y); return false;	
	}
	return false;
}
static bool sty_zp_x(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t8 = cpu.read(cpu.PC++); return true;
	case 3: cpu.read(cpu.t8); cpu.t8 += cpu.X; return true;
	case 4: cpu.write(cpu.t8, cpu.Y); return false;	
	}
	return false;
}
static bool sty_abs(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t16 = cpu.read(cpu.PC++); return true;
	case 3: cpu.t16 |= cpu.read(cpu.PC++)<<8; return true;
	case 4: cpu.write(cpu.t16, cpu.Y); return false;	
	}
	return false;
}
static bool stx_zp(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t8 = cpu.read(cpu.PC++); return true;
	case 3: cpu.write(cpu.t8, cpu.X); return false;	
	}
	return false;
}
static bool stx_zp_y(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t8 = cpu.read(cpu.PC++); return true;
	case 3: cpu.read(cpu.t8); cpu.t8 += cpu.Y; return true;
	case 4: cpu.write(cpu.t8, cpu.X); return false;	
	}
	return false;
}
static bool stx_abs(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t16 = cpu.read(cpu.PC++); return true;
	case 3: cpu.t16 |= cpu.read(cpu.PC++)<<8; return true;
	case 4: cpu.write(cpu.t16, cpu.X); return false;	
	}
	return false;
}
static bool ldx_imm(c6502& cpu)
{
	if( cpu.cycle == 2 ) setNZ(cpu, cpu.X = cpu.read(cpu.PC++));
	return cpu.cycle!=2;
}
static bool ldx_zp(c6502& cpu)
{
	if( cpu.cycle == 3 ) setNZ(cpu, cpu.X = cpu.read(cpu.read(cpu.PC++)));
	return cpu.cycle!=3;
}
static bool ldx_zpy(c6502& cpu)
{
	if( cpu.cycle == 2 ) cpu.t8 = cpu.read(cpu.PC++);
	else if( cpu.cycle == 3 ) cpu.read(cpu.t8);
	else if( cpu.cycle == 4 ) setNZ(cpu, cpu.X = cpu.read(0xff&(cpu.Y+cpu.t8)));
	return cpu.cycle!=4;
}
static bool ldx_abs(c6502& cpu)
{
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		setNZ(cpu, cpu.X = cpu.read(cpu.t16));
	}
	return cpu.cycle!=4;
}
static bool ldx_abs_y(c6502& cpu)
{
	if( cpu.cycle <= 3 ) return true;
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		cpu.X = cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.Y)&0xff));
		setNZ(cpu, cpu.X);
		return ((cpu.t16&0xff00)|((cpu.t16+cpu.Y)&0xff)) != (cpu.t16+cpu.Y);
	}
	if( cpu.cycle == 5 )
	{
		cpu.X = cpu.read(cpu.t16 + cpu.Y);
		setNZ(cpu, cpu.X);
	}
	return false;
}
static bool ldy_imm(c6502& cpu)
{
	if( cpu.cycle == 2 ) setNZ(cpu, cpu.Y = cpu.read(cpu.PC++));
	return cpu.cycle!=2;
}
static bool ldy_zp(c6502& cpu)
{
	if( cpu.cycle == 3 ) setNZ(cpu, cpu.Y = cpu.read(cpu.read(cpu.PC++)));
	return cpu.cycle!=3;
}
static bool ldy_zpx(c6502& cpu)
{
	if( cpu.cycle == 2 ) cpu.t8 = cpu.read(cpu.PC++);
	else if( cpu.cycle == 3 ) cpu.read(cpu.t8);
	else if( cpu.cycle == 4 ) setNZ(cpu, cpu.Y = cpu.read(0xff&(cpu.X+cpu.t8)));
	return cpu.cycle!=4;
}
static bool ldy_abs(c6502& cpu)
{
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		setNZ(cpu, cpu.Y = cpu.read(cpu.t16));
	}
	return cpu.cycle!=4;
}
static bool ldy_abs_x(c6502& cpu)
{
	if( cpu.cycle <= 3 ) return true;
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		cpu.Y = cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.X)&0xff));
		setNZ(cpu, cpu.Y);
		return ((cpu.t16&0xff00)|((cpu.t16+cpu.X)&0xff)) != (cpu.t16+cpu.X);
	}
	if( cpu.cycle == 5 )
	{
		cpu.Y = cpu.read(cpu.t16 + cpu.X);
		setNZ(cpu, cpu.Y);
	}
	return false;
}
static bool sta_zp(c6502& cpu)
{
	if( cpu.cycle == 3 ) cpu.write(cpu.read(cpu.PC++), cpu.A);
	return cpu.cycle!=3;
}
static bool sta_zpx(c6502& cpu)
{
	if( cpu.cycle == 2 ) cpu.t8 = cpu.read(cpu.PC++);
	else if( cpu.cycle == 3 ) cpu.read(cpu.t8);
	else if( cpu.cycle == 4 ) cpu.write(0xff&(cpu.t8+cpu.X), cpu.A);
	return cpu.cycle!=4;
}
static bool sta_abs(c6502& cpu)
{
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		cpu.write(cpu.t16, cpu.A);
	}
	return cpu.cycle!=4;
}
static bool sta_abs_x(c6502& cpu)
{
	if( cpu.cycle <= 3 ) return true;
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.X)&0xff));
		return true;
	}
	if( cpu.cycle == 5 )
	{
		cpu.write(cpu.t16+cpu.X, cpu.A);
	}
	return false;
}
static bool sta_abs_y(c6502& cpu)
{
	if( cpu.cycle <= 3 ) return true;
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.Y)&0xff));
		return true;
	}
	if( cpu.cycle == 5 )
	{
		cpu.write(cpu.t16+cpu.Y, cpu.A);
	}
	return false;
}
static bool sta_ind_x(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t8 = cpu.read(cpu.PC++); return true;
	case 3: cpu.read(cpu.t8); cpu.t8 += cpu.X; return true;
	case 4: cpu.t16 = cpu.read(cpu.t8); return true;
	case 5: cpu.t16 |= cpu.read(0xff&(cpu.t8+1))<<8; return true;
	case 6: cpu.write(cpu.t16, cpu.A); return false;	
	}
	return false;
}
static bool sta_ind_y(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t8 = cpu.read(cpu.PC++); return true;
	case 3: cpu.t16 = cpu.read(cpu.t8); return true;
	case 4: cpu.t16 |= cpu.read(0xff&(cpu.t8+1))<<8; return true;
	case 5: cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.Y)&0xff)); return true;
	case 6: cpu.write(cpu.t16+cpu.Y, cpu.A); return false;
	}
	return false;
}
static bool dec_zp(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t16 = cpu.read(cpu.PC++); return true;
	case 3: cpu.t8 = cpu.read(cpu.t16); return true;
	case 4: cpu.write(cpu.t16, cpu.t8); cpu.t8--; setNZ(cpu, cpu.t8); return true;
	case 5: cpu.write(cpu.t16, cpu.t8); return false;
	}
	return false;
}
static bool dec_zp_x(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t16 = cpu.read(cpu.PC++); return true;
	case 3: cpu.read(cpu.t16); cpu.t16 = (cpu.t16+cpu.X)&0xff; return true;
	case 4: cpu.t8 = cpu.read(cpu.t16); return true;
	case 5: cpu.write(cpu.t16, cpu.t8); cpu.t8--; setNZ(cpu, cpu.t8); return true;
	case 6: cpu.write(cpu.t16, cpu.t8); return false;
	}
	return false;
}
static bool dec_abs(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t16 = cpu.read(cpu.PC++); return true;
	case 3: cpu.t16 |= cpu.read(cpu.PC++)<<8; return true;
	case 4: cpu.t8 = cpu.read(cpu.t16); return true;
	case 5: cpu.write(cpu.t16, cpu.t8); cpu.t8--; setNZ(cpu, cpu.t8); return true;
	case 6: cpu.write(cpu.t16, cpu.t8); return false;	
	}	
	return false;
}
static bool dec_abs_x(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t16 = cpu.read(cpu.PC++); return true;
	case 3: cpu.t16 |= cpu.read(cpu.PC++)<<8; return true;
	case 4: cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.X)&0xff)); cpu.t16 += cpu.X; return true;
	case 5: cpu.t8 = cpu.read(cpu.t16); return true;
	case 6: cpu.write(cpu.t16, cpu.t8); cpu.t8--; setNZ(cpu, cpu.t8); return true;
	case 7: cpu.write(cpu.t16, cpu.t8); return false;	
	}
	return false;
}
static bool inc_zp(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t16 = cpu.read(cpu.PC++); return true;
	case 3: cpu.t8 = cpu.read(cpu.t16); return true;
	case 4: cpu.write(cpu.t16, cpu.t8); cpu.t8++; setNZ(cpu, cpu.t8); return true;
	case 5: cpu.write(cpu.t16, cpu.t8); return false;
	}
	return false;
}
static bool inc_zp_x(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t16 = cpu.read(cpu.PC++); return true;
	case 3: cpu.read(cpu.t16); cpu.t16 = (cpu.t16+cpu.X)&0xff; return true;
	case 4: cpu.t8 = cpu.read(cpu.t16); return true;
	case 5: cpu.write(cpu.t16, cpu.t8); cpu.t8++; setNZ(cpu, cpu.t8); return true;
	case 6: cpu.write(cpu.t16, cpu.t8); return false;
	}
	return false;
}
static bool inc_abs(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t16 = cpu.read(cpu.PC++); return true;
	case 3: cpu.t16 |= cpu.read(cpu.PC++)<<8; return true;
	case 4: cpu.t8 = cpu.read(cpu.t16); return true;
	case 5: cpu.write(cpu.t16, cpu.t8); cpu.t8++; setNZ(cpu, cpu.t8); return true;
	case 6: cpu.write(cpu.t16, cpu.t8); return false;	
	}
	return false;
}
static bool inc_abs_x(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t16 = cpu.read(cpu.PC++); return true;
	case 3: cpu.t16 |= cpu.read(cpu.PC++)<<8; return true;
	case 4: cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.X)&0xff)); cpu.t16 += cpu.X; return true;
	case 5: cpu.t8 = cpu.read(cpu.t16); return true;
	case 6: cpu.write(cpu.t16, cpu.t8); cpu.t8++; setNZ(cpu, cpu.t8); return true;
	case 7: cpu.write(cpu.t16, cpu.t8); return false;	
	}
	return false;
}
static bool adc_imm(c6502& cpu)
{
	if( cpu.cycle == 2 ) cpu.A = adc(cpu, cpu.read(cpu.PC++));
	return cpu.cycle!=2;
}
static bool adc_zp(c6502& cpu)
{
	if( cpu.cycle == 3 ) cpu.A = adc(cpu, cpu.read(cpu.read(cpu.PC++)));
	return cpu.cycle!=3;
}
static bool adc_zpx(c6502& cpu)
{
	if( cpu.cycle == 2 ) cpu.t8 = cpu.read(cpu.PC++);
	else if( cpu.cycle == 3 ) cpu.read(cpu.t8);
	else if( cpu.cycle == 4 ) cpu.A = adc(cpu, cpu.read(0xff&(cpu.X+cpu.t8)));
	return cpu.cycle!=4;
}
static bool adc_abs(c6502& cpu)
{
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		cpu.A = adc(cpu, cpu.read(cpu.t16));
	}
	return cpu.cycle!=4;
}
static bool adc_abs_x(c6502& cpu)
{
	if( cpu.cycle <= 3 ) return true;
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		if( ((cpu.t16&0xff00)|((cpu.t16+cpu.X)&0xff)) != (cpu.t16+cpu.X) )
		{
			cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.X)&0xff));
			return true;
		}
		cpu.A = adc(cpu, cpu.read(cpu.t16 + cpu.X));
		return false;
	}
	if( cpu.cycle == 5 )
	{
		cpu.A = adc(cpu, cpu.read(cpu.t16 + cpu.X));
	}
	return false;
}
static bool adc_abs_y(c6502& cpu)
{
	if( cpu.cycle <= 3 ) return true;
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		if( ((cpu.t16&0xff00)|((cpu.t16+cpu.Y)&0xff)) != (cpu.t16+cpu.Y) )
		{
			cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.Y)&0xff));
			return true;
		}
		cpu.A = adc(cpu, cpu.read(cpu.t16 + cpu.Y));
		return false;
	}
	if( cpu.cycle == 5 )
	{
		cpu.A = adc(cpu, cpu.read(cpu.t16 + cpu.Y));
	}
	return false;
}
static bool adc_ind_x(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t8 = cpu.read(cpu.PC++); return true;
	case 3: cpu.read(cpu.t8); cpu.t8 += cpu.X; return true;
	case 4: cpu.t16 = cpu.read(cpu.t8); return true;
	case 5: cpu.t16 |= cpu.read(0xff&(cpu.t8+1))<<8; return true;
	case 6: cpu.A = adc(cpu, cpu.read(cpu.t16)); return false;	
	}
	return false;
}
static bool adc_ind_y(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t8 = cpu.read(cpu.PC++); return true;
	case 3: cpu.t16 = cpu.read(cpu.t8); return true;
	case 4: cpu.t16 |= cpu.read(0xff&(cpu.t8+1))<<8; return true;
	case 5: cpu.t8 = cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.Y)&0xff)); 
		if( (cpu.t16&0xff00) != ((cpu.t16+cpu.Y)&0xff00) ) return true;
		else cpu.A = adc(cpu, cpu.t8);
		return false;
	case 6: cpu.A = adc(cpu, cpu.read(cpu.t16+cpu.Y)); return false;
	}
	return false;
}
static bool sbc_imm(c6502& cpu)
{
	if( cpu.cycle == 2 ) cpu.A = sbc(cpu, cpu.read(cpu.PC++));
	return cpu.cycle!=2;
}
static bool sbc_zp(c6502& cpu)
{
	if( cpu.cycle == 3 ) cpu.A = sbc(cpu, cpu.read(cpu.read(cpu.PC++)));
	return cpu.cycle!=3;
}
static bool sbc_zpx(c6502& cpu)
{
	if( cpu.cycle == 2 ) cpu.t8 = cpu.read(cpu.PC++);
	else if( cpu.cycle == 3 ) cpu.read(cpu.t8);
	else if( cpu.cycle == 4 ) cpu.A = sbc(cpu, cpu.read(0xff&(cpu.X+cpu.t8)));
	return cpu.cycle!=4;
}
static bool sbc_abs(c6502& cpu)
{
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		cpu.A = sbc(cpu, cpu.read(cpu.t16));
	}
	return cpu.cycle!=4;
}
static bool sbc_abs_x(c6502& cpu)
{
	if( cpu.cycle <= 3 ) return true;
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		if( ((cpu.t16&0xff00)|((cpu.t16+cpu.X)&0xff)) != (cpu.t16+cpu.X) )
		{
			cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.X)&0xff));
			return true;
		}
		cpu.A = sbc(cpu, cpu.read(cpu.t16 + cpu.X));
		return false;
	}
	if( cpu.cycle == 5 )
	{
		cpu.A = sbc(cpu, cpu.read(cpu.t16 + cpu.X));
	}
	return false;
}
static bool sbc_abs_y(c6502& cpu)
{
	if( cpu.cycle <= 3 ) return true;
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		if( ((cpu.t16&0xff00)|((cpu.t16+cpu.Y)&0xff)) != (cpu.t16+cpu.Y) )
		{
			cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.Y)&0xff));
			return true;
		}
		cpu.A = sbc(cpu, cpu.read(cpu.t16 + cpu.Y));
		return false;
	}
	if( cpu.cycle == 5 )
	{
		cpu.A = sbc(cpu, cpu.read(cpu.t16 + cpu.Y));
	}
	return false;
}
static bool sbc_ind_x(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t8 = cpu.read(cpu.PC++); return true;
	case 3: cpu.read(cpu.t8); cpu.t8 += cpu.X; return true;
	case 4: cpu.t16 = cpu.read(cpu.t8); return true;
	case 5: cpu.t16 |= cpu.read(0xff&(cpu.t8+1))<<8; return true;
	case 6: cpu.A = sbc(cpu, cpu.read(cpu.t16)); return false;	
	}
	return false;
}
static bool sbc_ind_y(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t8 = cpu.read(cpu.PC++); return true;
	case 3: cpu.t16 = cpu.read(cpu.t8); return true;
	case 4: cpu.t16 |= cpu.read(0xff&(cpu.t8+1))<<8; return true;
	case 5: cpu.t8 = cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.Y)&0xff)); 
		if( (cpu.t16&0xff00) != ((cpu.t16+cpu.Y)&0xff00) ) return true;
		else cpu.A = sbc(cpu, cpu.t8);
		return false;
	case 6: cpu.A = sbc(cpu, cpu.read(cpu.t16+cpu.Y)); return false;
	}
	return false;
}
static bool cmp_imm(c6502& cpu)
{
	if( cpu.cycle == 2 ) cmp(cpu, cpu.read(cpu.PC++));
	return cpu.cycle!=2;
}
static bool cmp_zp(c6502& cpu)
{
	if( cpu.cycle == 3 ) cmp(cpu, cpu.read(cpu.read(cpu.PC++)));
	return cpu.cycle!=3;
}
static bool cmp_zpx(c6502& cpu)
{
	if( cpu.cycle == 2 ) cpu.t8 = cpu.read(cpu.PC++);
	else if( cpu.cycle == 3 ) cpu.read(cpu.t8);
	else if( cpu.cycle == 4 ) cmp(cpu, cpu.read(0xff&(cpu.X+cpu.t8)));
	return cpu.cycle!=4;
}
static bool cmp_abs(c6502& cpu)
{
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		cmp(cpu, cpu.read(cpu.t16));
	}
	return cpu.cycle!=4;
}
static bool cmp_abs_x(c6502& cpu)
{
	if( cpu.cycle <= 3 ) return true;
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		if( ((cpu.t16&0xff00)|((cpu.t16+cpu.X)&0xff)) != (cpu.t16+cpu.X) )
		{
			cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.X)&0xff));
			return true;
		}
		cmp(cpu, cpu.read(cpu.t16 + cpu.X));
		return false;
	}
	if( cpu.cycle == 5 )
	{
		cmp(cpu, cpu.read(cpu.t16 + cpu.X));
	}
	return false;
}
static bool cmp_abs_y(c6502& cpu)
{
	if( cpu.cycle <= 3 ) return true;
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		if( ((cpu.t16&0xff00)|((cpu.t16+cpu.Y)&0xff)) != (cpu.t16+cpu.Y) )
		{
			cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.Y)&0xff));
			return true;
		}
		cmp(cpu, cpu.read(cpu.t16 + cpu.Y));
		return false;
	}
	if( cpu.cycle == 5 )
	{
		cmp(cpu, cpu.read(cpu.t16 + cpu.Y));
	}
	return false;
}
static bool cmp_ind_x(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t8 = cpu.read(cpu.PC++); return true;
	case 3: cpu.read(cpu.t8); cpu.t8 += cpu.X; return true;
	case 4: cpu.t16 = cpu.read(cpu.t8); return true;
	case 5: cpu.t16 |= cpu.read(0xff&(cpu.t8+1))<<8; return true;
	case 6: cmp(cpu, cpu.read(cpu.t16)); return false;	
	}
	return false;
}
static bool cmp_ind_y(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t8 = cpu.read(cpu.PC++); return true;
	case 3: cpu.t16 = cpu.read(cpu.t8); return true;
	case 4: cpu.t16 |= cpu.read(0xff&(cpu.t8+1))<<8; return true;
	case 5: cpu.t8 = cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.Y)&0xff)); 
		if( (cpu.t16&0xff00) != ((cpu.t16+cpu.Y)&0xff00) ) return true;
		else cmp(cpu, cpu.t8);
		return false;
	case 6: cmp(cpu, cpu.read(cpu.t16+cpu.Y)); return false;
	}
	return false;
}
static bool cpx_imm(c6502& cpu)
{
	if( cpu.cycle == 2 ) cmp(cpu, cpu.X, cpu.read(cpu.PC++));
	return cpu.cycle!=2;
}
static bool cpx_zp(c6502& cpu)
{
	if( cpu.cycle == 3 ) cmp(cpu, cpu.X, cpu.read(cpu.read(cpu.PC++)));
	return cpu.cycle!=3;
}
static bool cpx_abs(c6502& cpu)
{
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		cmp(cpu, cpu.X, cpu.read(cpu.t16));
	}
	return cpu.cycle!=4;
}
static bool cpy_imm(c6502& cpu)
{
	if( cpu.cycle == 2 ) cmp(cpu, cpu.Y, cpu.read(cpu.PC++));
	return cpu.cycle!=2;
}
static bool cpy_zp(c6502& cpu)
{
	if( cpu.cycle == 3 ) cmp(cpu, cpu.Y, cpu.read(cpu.read(cpu.PC++)));
	return cpu.cycle!=3;
}
static bool cpy_abs(c6502& cpu)
{
	if( cpu.cycle == 4 )
	{
		cpu.t16 = cpu.read(cpu.PC++);
		cpu.t16 |= cpu.read(cpu.PC++)<<8;
		cmp(cpu, cpu.Y, cpu.read(cpu.t16));
	}
	return cpu.cycle!=4;
}
static bool asl_a(c6502& cpu)
{
	if( cpu.cycle == 2 )
	{
		cpu.P.b.C = (cpu.A>>7);
		cpu.A <<= 1;
		setNZ(cpu, cpu.A);
	}
	return cpu.cycle!=2;
}
static bool asl_zp(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t16 = cpu.read(cpu.PC++); return true;
	case 3: cpu.t8 = cpu.read(cpu.t16); return true;
	case 4: cpu.write(cpu.t16, cpu.t8); cpu.P.b.C = (cpu.t8>>7); cpu.t8<<=1; setNZ(cpu, cpu.t8); return true;
	case 5: cpu.write(cpu.t16, cpu.t8); return false;
	}
	return false;
}
static bool asl_zp_x(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t16 = cpu.read(cpu.PC++); return true;
	case 3: cpu.read(cpu.t16); cpu.t16 = (cpu.t16+cpu.X)&0xff; return true;
	case 4: cpu.t8 = cpu.read(cpu.t16); return true;
	case 5: cpu.write(cpu.t16, cpu.t8); cpu.P.b.C = (cpu.t8>>7); cpu.t8<<=1; setNZ(cpu, cpu.t8); return true;
	case 6: cpu.write(cpu.t16, cpu.t8); return false;
	}
	return false;
}
static bool asl_abs(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t16 = cpu.read(cpu.PC++); return true;
	case 3: cpu.t16 |= cpu.read(cpu.PC++)<<8; return true;
	case 4: cpu.t8 = cpu.read(cpu.t16); return true;
	case 5: cpu.write(cpu.t16, cpu.t8); cpu.P.b.C = (cpu.t8>>7); cpu.t8<<=1; setNZ(cpu, cpu.t8); return true;
	case 6: cpu.write(cpu.t16, cpu.t8); return false;	
	}
	return false;
}
static bool asl_abs_x(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t16 = cpu.read(cpu.PC++); return true;
	case 3: cpu.t16 |= cpu.read(cpu.PC++)<<8; return true;
	case 4: cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.X)&0xff)); cpu.t16 += cpu.X; return true;
	case 5: cpu.t8 = cpu.read(cpu.t16); return true;
	case 6: cpu.write(cpu.t16, cpu.t8); cpu.P.b.C = (cpu.t8>>7); cpu.t8<<=1; setNZ(cpu, cpu.t8); return true;
	case 7: cpu.write(cpu.t16, cpu.t8); return false;	
	}
	return false;
}
static bool lsr_a(c6502& cpu)
{
	if( cpu.cycle == 2 )
	{
		cpu.P.b.C = (cpu.A&1);
		cpu.A >>= 1;
		setNZ(cpu, cpu.A);
	}
	return cpu.cycle!=2;
}
static bool lsr_zp(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t16 = cpu.read(cpu.PC++); return true;
	case 3: cpu.t8 = cpu.read(cpu.t16); return true;
	case 4: cpu.write(cpu.t16, cpu.t8); cpu.P.b.C = (cpu.t8&1); cpu.t8>>=1; setNZ(cpu, cpu.t8); return true;
	case 5: cpu.write(cpu.t16, cpu.t8); return false;
	}
	return false;
}
static bool lsr_zp_x(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t16 = cpu.read(cpu.PC++); return true;
	case 3: cpu.read(cpu.t16); cpu.t16 = (cpu.t16+cpu.X)&0xff; return true;
	case 4: cpu.t8 = cpu.read(cpu.t16); return true;
	case 5: cpu.write(cpu.t16, cpu.t8); cpu.P.b.C = (cpu.t8&1); cpu.t8>>=1; setNZ(cpu, cpu.t8); return true;
	case 6: cpu.write(cpu.t16, cpu.t8); return false;
	}
	return false;
}
static bool lsr_abs(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t16 = cpu.read(cpu.PC++); return true;
	case 3: cpu.t16 |= cpu.read(cpu.PC++)<<8; return true;
	case 4: cpu.t8 = cpu.read(cpu.t16); return true;
	case 5: cpu.write(cpu.t16, cpu.t8); cpu.P.b.C = (cpu.t8&1); cpu.t8>>=1; setNZ(cpu, cpu.t8); return true;
	case 6: cpu.write(cpu.t16, cpu.t8); return false;	
	}
	return false;
}
static bool lsr_abs_x(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t16 = cpu.read(cpu.PC++); return true;
	case 3: cpu.t16 |= cpu.read(cpu.PC++)<<8; return true;
	case 4: cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.X)&0xff)); cpu.t16 += cpu.X; return true;
	case 5: cpu.t8 = cpu.read(cpu.t16); return true;
	case 6: cpu.write(cpu.t16, cpu.t8); cpu.P.b.C = (cpu.t8&1); cpu.t8>>=1; setNZ(cpu, cpu.t8); return true;
	case 7: cpu.write(cpu.t16, cpu.t8); return false;	
	}
	return false;
}
static bool ror_a(c6502& cpu)
{
	if( cpu.cycle == 2 )
	{
		u8 oldC = cpu.P.b.C;
		cpu.P.b.C = (cpu.A&1);
		cpu.A >>= 1;
		cpu.A |= oldC<<7;
		setNZ(cpu, cpu.A);
	}
	return cpu.cycle!=2;
}
static bool ror_zp(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t16 = cpu.read(cpu.PC++); return true;
	case 3: cpu.t8 = cpu.read(cpu.t16); return true;
	case 4:{
		cpu.write(cpu.t16, cpu.t8);
		u8 oldC = cpu.P.b.C;
		cpu.P.b.C = (cpu.t8&1); 
		cpu.t8>>=1; 
		cpu.t8 |= oldC<<7;
		setNZ(cpu, cpu.t8); 
		}return true;
	case 5: cpu.write(cpu.t16, cpu.t8); return false;
	}
	return false;
}
static bool ror_zp_x(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t16 = cpu.read(cpu.PC++); return true;
	case 3: cpu.read(cpu.t16); cpu.t16 = (cpu.t16+cpu.X)&0xff; return true;
	case 4: cpu.t8 = cpu.read(cpu.t16); return true;
	case 5:{
		cpu.write(cpu.t16, cpu.t8); 
		u8 oldC = cpu.P.b.C;
		cpu.P.b.C = (cpu.t8&1); 
		cpu.t8>>=1; 
		cpu.t8 |= oldC<<7;
		setNZ(cpu, cpu.t8); 
		}return true;
	case 6: cpu.write(cpu.t16, cpu.t8); return false;
	}
	return false;
}
static bool ror_abs(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t16 = cpu.read(cpu.PC++); return true;
	case 3: cpu.t16 |= cpu.read(cpu.PC++)<<8; return true;
	case 4: cpu.t8 = cpu.read(cpu.t16); return true;
	case 5:{
		cpu.write(cpu.t16, cpu.t8); 
		u8 oldC = cpu.P.b.C;
		cpu.P.b.C = (cpu.t8&1); 
		cpu.t8>>=1; 
		cpu.t8 |= oldC<<7;
		setNZ(cpu, cpu.t8); 
		}return true;
	case 6: cpu.write(cpu.t16, cpu.t8); return false;	
	}
	return false;
}
static bool ror_abs_x(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t16 = cpu.read(cpu.PC++); return true;
	case 3: cpu.t16 |= cpu.read(cpu.PC++)<<8; return true;
	case 4: cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.X)&0xff)); cpu.t16 += cpu.X; return true;
	case 5: cpu.t8 = cpu.read(cpu.t16); return true;
	case 6: {
		cpu.write(cpu.t16, cpu.t8); 
		u8 oldC = cpu.P.b.C;
		cpu.P.b.C = (cpu.t8&1); 
		cpu.t8>>=1; 
		cpu.t8 |= oldC<<7;
		setNZ(cpu, cpu.t8); 
		}return true;
	case 7: cpu.write(cpu.t16, cpu.t8); return false;	
	}
	return false;
}
static bool rol_a(c6502& cpu)
{
	if( cpu.cycle == 2 )
	{
		u8 oldC = cpu.P.b.C;
		cpu.P.b.C = (cpu.A>>7);
		cpu.A <<= 1;
		cpu.A |= oldC;
		setNZ(cpu, cpu.A);
	}
	return cpu.cycle!=2;
}
static bool rol_zp(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t16 = cpu.read(cpu.PC++); return true;
	case 3: cpu.t8 = cpu.read(cpu.t16); return true;
	case 4:{
		cpu.write(cpu.t16, cpu.t8);
		u8 oldC = cpu.P.b.C;
		cpu.P.b.C = (cpu.t8>>7);
		cpu.t8 <<= 1;
		cpu.t8 |= oldC;
		setNZ(cpu, cpu.t8); 
		}return true;
	case 5: cpu.write(cpu.t16, cpu.t8); return false;
	}
	return false;
}
static bool rol_zp_x(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t16 = cpu.read(cpu.PC++); return true;
	case 3: cpu.read(cpu.t16); cpu.t16 = (cpu.t16+cpu.X)&0xff; return true;
	case 4: cpu.t8 = cpu.read(cpu.t16); return true;
	case 5:{
		cpu.write(cpu.t16, cpu.t8); 
		u8 oldC = cpu.P.b.C;
		cpu.P.b.C = (cpu.t8>>7);
		cpu.t8 <<= 1;
		cpu.t8 |= oldC;
		setNZ(cpu, cpu.t8); 
		}return true;
	case 6: cpu.write(cpu.t16, cpu.t8); return false;
	}
	return false;
}
static bool rol_abs(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t16 = cpu.read(cpu.PC++); return true;
	case 3: cpu.t16 |= cpu.read(cpu.PC++)<<8; return true;
	case 4: cpu.t8 = cpu.read(cpu.t16); return true;
	case 5:{
		cpu.write(cpu.t16, cpu.t8); 
		u8 oldC = cpu.P.b.C;
		cpu.P.b.C = (cpu.t8>>7);
		cpu.t8 <<= 1;
		cpu.t8 |= oldC;
		setNZ(cpu, cpu.t8); 
		}return true;
	case 6: cpu.write(cpu.t16, cpu.t8); return false;	
	}
	return false;
}
static bool rol_abs_x(c6502& cpu)
{
	switch( cpu.cycle )
	{
	case 1: return true;
	case 2: cpu.t16 = cpu.read(cpu.PC++); return true;
	case 3: cpu.t16 |= cpu.read(cpu.PC++)<<8; return true;
	case 4: cpu.read((cpu.t16&0xff00)|((cpu.t16+cpu.X)&0xff)); cpu.t16 += cpu.X; return true;
	case 5: cpu.t8 = cpu.read(cpu.t16); return true;
	case 6: {
		cpu.write(cpu.t16, cpu.t8); 
		u8 oldC = cpu.P.b.C;
		cpu.P.b.C = (cpu.t8>>7);
		cpu.t8 <<= 1;
		cpu.t8 |= oldC;
		setNZ(cpu, cpu.t8); 
		}return true;
	case 7: cpu.write(cpu.t16, cpu.t8); return false;	
	}
	return false;
}

static bool nmi(c6502& cpu)
{
	if( cpu.cycle == 7 )
	{
		cpu.write(0x100+cpu.S, cpu.PC>>8); cpu.S--;
		cpu.write(0x100+cpu.S, cpu.PC);    cpu.S--;
		cpu.write(0x100+cpu.S, cpu.P.r&~0x10); cpu.S--;
		cpu.PC = cpu.read(0xFFFA);
		cpu.PC |= cpu.read(0xFFFB)<<8;
		cpu.P.b.I = 1;
	}
	return cpu.cycle!=7;
}

static bool irq(c6502& cpu)
{
	if( cpu.cycle == 7 )
	{
		cpu.write(0x100+cpu.S, cpu.PC>>8); cpu.S--;
		cpu.write(0x100+cpu.S, cpu.PC);    cpu.S--;
		cpu.write(0x100+cpu.S, cpu.P.r&~0x10); cpu.S--;
		cpu.PC = cpu.read(0xFFFE);
		cpu.PC |= cpu.read(0xFFFF)<<8;
		cpu.P.b.I = 1;
	}
	return cpu.cycle!=7;
}

typedef bool (*nesop)(c6502&);

#define OPC_IRQ   0x100
#define OPC_NMI   0x101

std::array<nesop, 259> nes_opcode_table = [](){
	std::array<nesop, 259> t;
	for(u32 i = 0; i < t.size(); ++i) t[i] = 0;
	
	t[OPC_NMI] = nmi;
	t[OPC_IRQ] = irq;
	
	t[0xEA] = nop;
	t[0x90] = bcc;
	t[0xB0] = bcs;
	t[0xF0] = beq;
	t[0x24] = bit_zp;
	t[0x2C] = bit_abs;
	t[0x30] = bmi;
	t[0xD0] = bne;
	t[0x10] = bpl;
	t[0x50] = bvc;
	t[0x70] = bvs;
	t[0x18] = clc;
	t[0xD8] = cld;
	t[0x58] = cli;
	t[0xB8] = clv;
	t[0x20] = jsr;
	t[0x48] = pha;
	t[0x08] = php;
	t[0x68] = pla;
	t[0x28] = plp;
	t[0x40] = rti;
	t[0x60] = rts;
	t[0x38] = sec;
	t[0xF8] = sed;
	t[0x78] = sei;
	t[0xAA] = tax;
	t[0xA8] = tay;
	t[0xBA] = tsx;
	t[0x8A] = txa;
	t[0x9A] = txs;
	t[0x98] = tya;
	
	t[0x69] = adc_imm;
	t[0x65] = adc_zp;
	t[0x75] = adc_zpx;
	t[0x6D] = adc_abs;
	t[0x7D] = adc_abs_x;
	t[0x79] = adc_abs_y;
	t[0x61] = adc_ind_x;
	t[0x71] = adc_ind_y;

	t[0x29] = and_imm;
	t[0x25] = and_zp;
	t[0x35] = and_zpx;
	t[0x2D] = and_abs;
	t[0x3D] = and_abs_x;
	t[0x39] = and_abs_y;
	t[0x21] = and_ind_x;
	t[0x31] = and_ind_y;
	
	t[0xC9] = cmp_imm;
	t[0xC5] = cmp_zp;
	t[0xD5] = cmp_zpx;
	t[0xCD] = cmp_abs;
	t[0xDD] = cmp_abs_x;
	t[0xD9] = cmp_abs_y;
	t[0xC1] = cmp_ind_x;
	t[0xD1] = cmp_ind_y;
	
	t[0xE0] = cpx_imm;
	t[0xE4] = cpx_zp;
	t[0xEC] = cpx_abs;
	
	t[0xC0] = cpy_imm;
	t[0xC4] = cpy_zp;
	t[0xCC] = cpy_abs;
	
	t[0xC6] = dec_zp;
	t[0xD6] = dec_zp_x;
	t[0xCE] = dec_abs;
	t[0xDE] = dec_abs_x;
	
	t[0xCA] = dex;
	t[0x88] = dey;
	
	t[0x49] = eor_imm;
	t[0x45] = eor_zp;
	t[0x55] = eor_zpx;
	t[0x4D] = eor_abs;
	t[0x5D] = eor_abs_x;
	t[0x59] = eor_abs_y;
	t[0x41] = eor_ind_x;
	t[0x51] = eor_ind_y;
	
	t[0xE6] = inc_zp;
	t[0xF6] = inc_zp_x;
	t[0xEE] = inc_abs;
	t[0xFE] = inc_abs_x;
	
	t[0xE8] = inx;
	t[0xC8] = iny;
	
	t[0x4C] = jmp_abs;
	t[0x6C] = jmp_ind;
	
	t[0xA9] = lda_imm;
	t[0xA5] = lda_zp;
	t[0xB5] = lda_zpx;
	t[0xAD] = lda_abs;
	t[0xBD] = lda_abs_x;
	t[0xB9] = lda_abs_y;
	t[0xA1] = lda_ind_x;
	t[0xB1] = lda_ind_y;
	
	t[0xA2] = ldx_imm;
	t[0xA6] = ldx_zp;
	t[0xB6] = ldx_zpy;
	t[0xAE] = ldx_abs;
	t[0xBE] = ldx_abs_y;
	
	t[0xA0] = ldy_imm;
	t[0xA4] = ldy_zp;
	t[0xB4] = ldy_zpx;
	t[0xAC] = ldy_abs;
	t[0xBC] = ldy_abs_x;
	
	t[0x09] = or_imm;
	t[0x05] = or_zp;
	t[0x15] = or_zpx;
	t[0x0D] = or_abs;
	t[0x1D] = or_abs_x;
	t[0x19] = or_abs_y;
	t[0x01] = or_ind_x;
	t[0x11] = or_ind_y;
	
	t[0xE9] = sbc_imm;
	t[0xE5] = sbc_zp;
	t[0xF5] = sbc_zpx;
	t[0xED] = sbc_abs;
	t[0xFD] = sbc_abs_x;
	t[0xF9] = sbc_abs_y;
	t[0xE1] = sbc_ind_x;
	t[0xF1] = sbc_ind_y;
	
	t[0x85] = sta_zp;
	t[0x95] = sta_zpx;
	t[0x8D] = sta_abs;
	t[0x9D] = sta_abs_x;
	t[0x99] = sta_abs_y;
	t[0x81] = sta_ind_x;
	t[0x91] = sta_ind_y;
	
	t[0x86] = stx_zp;
	t[0x96] = stx_zp_y;
	t[0x8E] = stx_abs;
	
	t[0x84] = sty_zp;
	t[0x94] = sty_zp_x;
	t[0x8C] = sty_abs;
	
	t[0x0A] = asl_a;
	t[0x06] = asl_zp;
	t[0x16] = asl_zp_x;
	t[0x0E] = asl_abs;
	t[0x1E] = asl_abs_x;
	
	t[0x4A] = lsr_a;
	t[0x46] = lsr_zp;
	t[0x56] = lsr_zp_x;
	t[0x4E] = lsr_abs;
	t[0x5E] = lsr_abs_x;
	
	t[0x2A] = rol_a;
	t[0x26] = rol_zp;
	t[0x36] = rol_zp_x;
	t[0x2E] = rol_abs;
	t[0x3E] = rol_abs_x;
	
	t[0x6A] = ror_a;
	t[0x66] = ror_zp;
	t[0x76] = ror_zp_x;
	t[0x6E] = ror_abs;
	t[0x7E] = ror_abs_x;
	return t;
}();

u8 testram[0x10000];

void test_write8(u16 addr, u8 val)
{
	testram[addr] = val;
}

u8 test_read8(u16 addr)
{
	return testram[addr];
}

bool c6502::step()
{
	c6502& cpu = *this;
	cpu.stamp++;
	if( cpu.cycle == 1 )
	{
		//printf("cpu.PC = $%X\n", cpu.PC);
		if( nmi_edge )
		{
			cpu.opc = OPC_NMI;
			nmi_edge = false;
		} else if( irq_assert && cpu.P.b.I==0 ) {
			cpu.opc = OPC_IRQ;
			//printf("%li: IRQ accepted\n", cpu.stamp);
		} else {
			cpu.opc = cpu.read(cpu.PC++);
		}
		if( !nes_opcode_table[cpu.opc] )
		{
			printf("unimpl. opc = $%02X at PC=$%04X\n", cpu.opc, cpu.PC-1);
			exit(1);
			return false;
		}
	} else {
		if( !nes_opcode_table[cpu.opc](cpu) )
		{
			cpu.cycle = 1;
			return false;
		}
	}
	cpu.cycle++;
	return true;
}

void c6502::nmi()
{
	nmi_edge = true;
}

void c6502::reset()
{	// treated as a power-on
	PC = read(0xFFFC);
	PC |= read(0xFFFD)<<8;
	//printf("Reset to $%X\n", PC);
	P.r = 0x34;
	A = X = Y = 0;
	S = 0xFD;
	cycle = 1;
	stamp = 0;
	nmi_edge = irq_assert = false;
}

