#include <print>
#include "6809.h"

u64 c6809::step10()
{
	u8 opc = read(pc++);
	
	switch( opc )
	{
	case 0x9F: // STY direct
		ea = (DP<<8)|read(pc++);
		write16(ea, Y);
		setNZ16(Y);
		F.b.V = 0;
		return 6;
	case 0xAF: // STY indexed
		cycles = 6;
		lea();
		write16(ea, Y);
		setNZ16(Y);
		F.b.V = 0;
		return cycles;
	case 0xBF: // STY expanded
		ea = read16(pc);
		pc += 2;
		write16(ea, Y);
		setNZ16(Y);
		F.b.V = 0;
		return 7;
	case 0xDF: // STS direct
		ea = (DP<<8)|read(pc++);
		write16(ea, S);
		setNZ16(S);
		F.b.V = 0;
		return 6;
	case 0xEF: // STS indexed
		cycles = 6;
		lea();
		write16(ea, S);
		setNZ16(S);
		F.b.V = 0;
		return cycles;
	case 0xFF: // STS expanded
		ea = read16(pc);
		pc += 2;
		write16(ea, S);
		setNZ16(S);
		F.b.V = 0;
		return 7;
	case 0xCE: // LDS imm
		S = read16(pc);
		pc+=2;
		setNZ16(S);
		F.b.V = 0;
		return 4;
	case 0xDE: // LDS direct
		S = direct16(read(pc++));
		setNZ16(S);
		F.b.V = 0;
		return 6;
	case 0xEE: // LDS indexed
		cycles = 6;
		lea();
		S = read16(ea);
		setNZ16(S);
		F.b.V = 0;		
		return cycles;
	case 0xFE: // LDS expanded
		ea = read16(pc);
		pc += 2;
		S = read16(ea);
		setNZ16(S);
		F.b.V = 0;
		return 7;
	case 0x8E: // LDY imm
		Y = read16(pc);
		pc+=2;
		setNZ16(Y);
		F.b.V = 0;
		return 4;
	case 0x9E: // LDY direct
		Y = direct16(read(pc++));
		setNZ16(Y);
		F.b.V = 0;
		return 6;
	case 0xAE: // LDY indexed
		cycles = 6;
		lea();
		Y = read16(ea);
		setNZ16(Y);
		F.b.V = 0;		
		return cycles;
	case 0xBE: // LDY expanded
		ea = read16(pc);
		pc += 2;
		Y = read16(ea);
		setNZ16(Y);
		F.b.V = 0;
		return 7;
	case 0x8C: // CMPY imm
		add16(Y, read16(pc)^0xffff, 1);
		pc += 2;
		F.b.C ^= 1;
		return 5;
	case 0x9C: // CMPY direct
		add16(Y, direct16(read(pc++))^0xffff, 1);
		F.b.C ^= 1;
		return 7;
	case 0xAC: // CMPY indexed
		cycles = 7;
		lea();
		add16(Y, read16(ea)^0xffff, 1);
		F.b.C ^= 1;
		return cycles;
	case 0xBC: // CMPY expanded
		ea = read16(pc);
		pc += 2;
		add16(Y, read16(ea)^0xffff, 1);
		F.b.C ^= 1;
		return 10;
	case 0x83: // CMPD imm
		add16(D(), read16(pc)^0xffff, 1);
		pc += 2;
		F.b.C ^= 1;
		return 5;
	case 0x93: // CMPD direct
		add16(D(), direct16(read(pc++))^0xffff, 1);
		F.b.C ^= 1;
		return 7;
	case 0xA3: // CMPD indexed
		cycles = 7;
		lea();
		add16(D(), read16(ea)^0xffff, 1);
		F.b.C ^= 1;
		return cycles;
	case 0xB3: // CMPD expanded
		ea = read16(pc);
		pc += 2;
		add16(D(), read16(ea)^0xffff, 1);
		F.b.C ^= 1;
		return 10;
	case 0x20: // LBRA //todo: ??
		ea = read16(pc);
		pc += ea + 2;
		return 6;
	case 0x21: // LBRN
		read16(pc); pc+=2;
		return 5; //todo: long branch cycles
	case 0x22: // LBHI
		ea = read16(pc);
		pc += 2;
		if( F.b.Z == 0 && F.b.C == 0 )
		{
			pc += ea;
			return 5;
		}
		return 6;
	case 0x23: // LBLS
		ea = read16(pc);
		pc += 2;
		if( F.b.Z == 1 || F.b.C == 1 )
		{
			pc += ea;
			return 5;
		}
		return 6;
	case 0x24: // LBCC
		ea = read16(pc);
		pc += 2;
		if( F.b.C == 0 )
		{
			pc += ea;
			return 5;
		}
		return 6;
	case 0x25: // LBCS
		ea = read16(pc);
		pc += 2;
		if( F.b.C == 1 )
		{
			pc += ea;
			return 5;
		}
		return 6;
	case 0x26: // LBNE
		ea = read16(pc);
		pc += 2;
		if( F.b.Z == 0 )
		{
			pc += ea;
			return 5;
		}
		return 6;
	case 0x27: // LBEQ
		ea = read16(pc);
		pc += 2;
		if( F.b.Z == 1 )
		{
			pc += ea;
			return 5;
		}
		return 6;
	case 0x28: // LBVC
		ea = read16(pc);
		pc += 2;
		if( F.b.V == 0 )
		{
			pc += ea;
			return 5;
		}
		return 6;
	case 0x29: // LBVS
		ea = read16(pc);
		pc += 2;
		if( F.b.V == 1 )
		{
			pc += ea;
			return 5;
		}
		return 6;
	case 0x2A: // LBPL
		ea = read16(pc);
		pc += 2;
		if( F.b.N == 0 )
		{
			pc += ea;
			return 5;
		}
		return 6;
	case 0x2B: // LBMI
		ea = read16(pc);
		pc += 2;
		if( F.b.N == 1 )
		{
			pc += ea;
			return 5;
		}
		return 6;
	case 0x2C: // LBGE
		ea = read16(pc);
		pc += 2;
		if( F.b.N == F.b.V )
		{
			pc += ea;
			return 5;
		}
		return 6;
	case 0x2D: // LBLT
		ea = read16(pc);
		pc += 2;
		if( F.b.N != F.b.V )
		{
			pc += ea;
			return 5;
		}
		return 6;
	case 0x2E: // LBGT
		ea = read16(pc);
		pc += 2;
		if( F.b.N == F.b.V && F.b.Z == 0 )
		{
			pc += ea;
			return 5;
		}
		return 6;
	case 0x2F: // LBLE
		ea = read16(pc);
		pc += 2;
		if( F.b.N != F.b.V || F.b.Z == 1 )
		{
			pc += ea;
			return 5;
		}
		return 6;
	default:
		std::println("6809: Unimpl step10 opc = ${:X}", opc);
		exit(1);
	}
	
	return 1;	
}

u64 c6809::step11()
{
	u8 opc = read(pc++);
	
	switch( opc )
	{
	case 0x8C: // CMPS imm
		add16(S, read16(pc)^0xffff, 1);
		pc += 2;
		F.b.C ^= 1;
		return 5;
	case 0x9C: // CMPS direct
		add16(S, direct16(read(pc++))^0xffff, 1);
		F.b.C ^= 1;
		return 7;
	case 0xAC: // CMPS indexed
		cycles = 7;
		lea();
		add16(S, read16(ea)^0xffff, 1);
		F.b.C ^= 1;
		return cycles;
	case 0xBC: // CMPS expanded
		ea = read16(pc);
		pc += 2;
		add16(S, read16(ea)^0xffff, 1);
		F.b.C ^= 1;
		return 10;

	case 0x83: // CMPU imm
		add16(U, read16(pc)^0xffff, 1);
		pc += 2;
		F.b.C ^= 1;
		return 5;
	case 0x93: // CMPU direct
		add16(U, direct16(read(pc++))^0xffff, 1);
		F.b.C ^= 1;
		return 7;
	case 0xA3: // CMPU indexed
		cycles = 7;
		lea();
		add16(U, read16(ea)^0xffff, 1);
		F.b.C ^= 1;
		return cycles;
	case 0xB3: // CMPU expanded
		ea = read16(pc);
		pc += 2;
		add16(U, read16(ea)^0xffff, 1);
		F.b.C ^= 1;
		return 10;
	
	default:
		std::println("6809: Unimpl step11 opc = ${:X}", opc);
		exit(1);
	}
	
	return 1;
}

u64 c6809::step()
{
	u8 opc = read(pc++);
	//	std::println("${:X}: opc ${:X}", /*(pc-1 < 0x5000) ? (0xc03f + ((pc-1)&0x1fff)) : */pc-1, opc);
	u8 t = 0;
	
	switch( opc )
	{
	case 0x10: // prefixed opcode
		return step10();
	case 0x11: // prefixed opcode
		return step11();
	case 0x16: // LBRA //todo: ??
		ea = read16(pc);
		pc += ea + 2;
		return 5;
	case 0x17: // LBSR //todo: ??
		ea = read16(pc);
		pc += 2;
		push16(pc);
		pc += ea;
		return 9;


	case 0x39: // RTS
		pc = pop16();
		return 5;
	case 0xB5: // BITA extended
		ea = read16(pc);
		pc += 2;
		setNZ(A&read(ea));
		F.b.V = 0;
		return 5;
	case 0xF5: // BITB extended
		ea = read16(pc);
		pc += 2;
		setNZ(B&read(ea));
		F.b.V = 0;
		return 5;
	case 0xA5: // BITA indexed
		cycles = 4;
		lea();
		setNZ(A&read(ea));
		F.b.V = 0;
		return cycles;
	case 0xE5: // BITB indexed
		cycles = 4;
		lea();
		setNZ(B&read(ea));
		F.b.V = 0;
		return cycles;
	case 0x03: // COM direct
		ea = (DP<<8)|read(pc++);
		t = read(ea);
		t = ~t;
		write(ea, t);
		F.b.C = 1;
		F.b.V = 0;
		setNZ(t);		
		return 6;
	case 0x63: // COM indexed
		cycles = 6;
		lea();
		t = read(ea);
		t = ~t;
		write(ea, t);
		F.b.C = 1;
		F.b.V = 0;
		setNZ(t);		
		return cycles;
	case 0x73: // COM extended
		ea = read16(pc);
		pc += 2;
		t = read(ea);
		t = ~t;
		write(ea, t);
		F.b.C = 1;
		F.b.V = 0;
		setNZ(t);		
		return 7;
	case 0xC3: // ADDD imm
		ea = read16(pc);
		pc += 2;
		D(add16(D(), ea, 0));
		return 4;
	case 0xD3: // ADDD direct
		ea = read(pc++);
		D(add16(D(), direct16(ea), 0));
		return 6;
	case 0xE3: // ADDD indexed
		cycles = 6;
		lea();
		D(add16(D(), read16(ea), 0));
		return cycles;
	case 0xF3: // ADDD extended
		ea = read16(pc);
		pc += 2;
		D(add16(D(), read16(ea), 0));
		return 7;
	case 0xCB: // ADDB imm
		B = add(B, read(pc++), 0);
		return 2;
	case 0xDB: // ADDB direct
		ea = (DP<<8)|read(pc++);
		B = add(B, read(ea), 0);
		return 4;
	case 0xEB: // ADDB indexed
		cycles = 4;
		lea();
		B = add(B, read(ea), 0);
		return cycles;
	case 0xFB: // ADDB extended
		ea = read16(pc);
		pc += 2;
		B = add(B, read(ea), 0);		
		return 5;
	case 0x8B: // ADDA imm
		A = add(A, read(pc++), 0);
		return 2;
	case 0x9B: // ADDA direct
		ea = (DP<<8)|read(pc++);
		A = add(A, read(ea), 0);
		return 4;
	case 0xAB: // ADCA indexed
		cycles = 4;
		lea();
		A = add(A, read(ea), 0);
		return cycles;
	case 0xBB: // ADDA extended
		ea = read16(pc);
		pc += 2;
		A = add(A, read(ea), 0);		
		return 5;
	case 0xC9: // ADCB imm
		B = add(B, read(pc++), F.b.C);
		return 2;
	case 0xD9: // ADCB direct
		ea = (DP<<8)|read(pc++);
		B = add(B, read(ea), F.b.C);
		return 4;
	case 0xE9: // ADCB indexed
		cycles = 4;
		lea();
		B = add(B, read(ea), F.b.C);
		return cycles;
	case 0xF9: // ADCB extended
		ea = read16(pc);
		pc += 2;
		B = add(B, read(ea), F.b.C);		
		return 5;
	case 0x89: // ADCA imm
		A = add(A, read(pc++), F.b.C);
		return 2;
	case 0x99: // ADCA direct
		ea = (DP<<8)|read(pc++);
		A = add(A, read(ea), F.b.C);
		return 4;
	case 0xA9: // ADCA indexed
		cycles = 4;
		lea();
		A = add(A, read(ea), F.b.C);
		return cycles;
	case 0xB9: // ADCA extended
		ea = read16(pc);
		pc += 2;
		A = add(A, read(ea), F.b.C);		
		return 5;
	case 0x44: // LSRA
		F.b.C = A&1;
		A >>= 1;		
		setNZ(A);
		return 2;
	case 0x54: // LSRB
		F.b.C = B&1;
		B >>= 1;		
		setNZ(B);
		return 2;
	case 0x04: // LSR direct
		ea = (DP<<8)|read(pc++);
		t = read(ea);
		F.b.C = t&1;
		t >>= 1;		
		write(ea, t);
		setNZ(t);
		return 6;
	case 0x64: // LSR indexed
		cycles = 6;
		lea();
		t = read(ea);
		F.b.C = t&1;
		t >>= 1;		
		write(ea, t);
		setNZ(t);
		return cycles;
	case 0x74: // LSR extended
		ea = read16(pc);
		pc += 2;
		t = read(ea);
		F.b.C = t&1;
		t >>= 1;		
		write(ea, t);
		setNZ(t);
		return 7;
	case 0x48: // LSLA
		F.b.C = A>>7;
		A <<= 1;
		setNZ(A);
		F.b.V = F.b.N ^ F.b.C;
		return 2;
	case 0x58: // LSLB
		F.b.C = B>>7;
		B <<= 1;
		setNZ(B);
		F.b.V = F.b.N ^ F.b.C;
		return 2;
	case 0x08: // LSL direct
		ea = (DP<<8)|read(pc++);
		t = read(ea);
		F.b.C = t>>7;
		t <<= 1;
		write(ea, t);
		setNZ(t);
		F.b.V = F.b.N ^ F.b.C;
		return 6;
	case 0x68: // LSL indexed
		cycles = 6;
		lea();
		t = read(ea);
		F.b.C = t>>7;
		t <<= 1;		
		write(ea, t);
		setNZ(t);
		F.b.V = F.b.N ^ F.b.C;
		return cycles;
	case 0x78: // LSL extended
		ea = read16(pc);
		pc += 2;
		t = read(ea);
		F.b.C = t>>7;
		t <<= 1;		
		write(ea, t);
		setNZ(t);
		F.b.V = F.b.N ^ F.b.C;
		return 7;
	case 0x47: // ASRA
		F.b.C = A&1;
		A = s8(A) >> 1;
		setNZ(A);
		return 2;
	case 0x57: // ASRB
		F.b.C = B&1;
		B = s8(B) >> 1;
		setNZ(B);
		return 2;
	case 0x07: // ASR direct
		ea = (DP<<8)|read(pc++);
		t = read(ea);
		F.b.C = t&1;
		t = s8(t) >> 1;
		write(ea, t);
		setNZ(t);
		return 6;
	case 0x67: // ASR indexed
		cycles = 6;
		lea();
		t = read(ea);
		F.b.C = t&1;
		t = s8(t) >> 1;
		write(ea, t);
		setNZ(t);
		return cycles;
	case 0x77: // ASR extended
		ea = read16(pc);
		pc += 2;
		t = read(ea);
		F.b.C = t&1;
		t = s8(t) >> 1;
		write(ea, t);
		setNZ(t);
		return 7;
	case 0x9D: // JSR direct
		ea = (DP<<8)|read(pc++);
		push16(pc);
		pc = ea;
		return 7;
	case 0xAD: // JSR indexed
		cycles = 7;
		lea();
		push16(pc);
		pc = ea;		
		return cycles;
	case 0xBD: // JSR extended
		ea = read16(pc);
		pc += 2;
		push16(pc);
		pc = ea;
		return 8;
	case 0x0E: // JMP direct
		t = read(pc++);
		pc = (DP<<8)|t;
		return 3;
	case 0x6E: // JMP indexed
		cycles = 3;
		lea();
		pc = ea;
		return cycles;
	case 0x7E: // JMP extended
		ea = read16(pc);
		pc = ea;
		return 4;
	case 0xB8: // EORA extended
		ea = read16(pc);
		pc += 2;
		A ^= read(ea);
		setNZ(A);
		F.b.V = 0;
		return 5;
	case 0xF8: // EORB extended
		ea = read16(pc);
		pc += 2;
		B ^= read(ea);
		setNZ(B);
		F.b.V = 0;
		return 5;
	case 0xB4: // ANDA extended
		ea = read16(pc);
		pc += 2;
		A &= read(ea);
		setNZ(A);
		F.b.V = 0;
		return 5;
	case 0xF4: // ANDB extended
		ea = read16(pc);
		pc += 2;
		B &= read(ea);
		setNZ(B);
		F.b.V = 0;
		return 5;
	case 0xBA: // ORA extended
		ea = read16(pc);
		pc += 2;
		A |= read(ea);
		setNZ(A);
		F.b.V = 0;
		return 5;
	case 0xFA: // ORB extended
		ea = read16(pc);
		pc += 2;
		B |= read(ea);
		setNZ(B);
		F.b.V = 0;
		return 5;
	case 0x20: // BRA
		t = read(pc++);
		pc += s8(t);
		return 3;
	case 0x21: // BRN
		read(pc++);
		return 3;
	case 0x22: // BHI
		t = read(pc++);
		if( F.b.Z == 0 && F.b.C == 0 )
		{
			pc += s8(t);
		}
		return 3;
	case 0x23: // BLS
		t = read(pc++);
		if( F.b.Z == 1 || F.b.C == 1 )
		{
			pc += s8(t);
		}
		return 3;
	case 0x24: // BCC
		t = read(pc++);
		if( F.b.C == 0 )
		{
			pc += s8(t);
		}
		return 3;
	case 0x25: // BCS
		t = read(pc++);
		if( F.b.C == 1 )
		{
			pc += s8(t);
		}
		return 3;
	case 0x26: // BNE
		t = read(pc++);
		if( F.b.Z == 0 )
		{
			pc += s8(t);
		}
		return 3;
	case 0x27: // BEQ
		t = read(pc++);
		if( F.b.Z == 1 )
		{
			pc += s8(t);
		}
		return 3;
	case 0x28: // BVC
		t = read(pc++);
		if( F.b.V == 0 )
		{
			pc += s8(t);
		}
		return 3;
	case 0x29: // BVS
		t = read(pc++);
		if( F.b.V == 1 )
		{
			pc += s8(t);
		}
		return 3;
	case 0x2A: // BPL
		t = read(pc++);
		if( F.b.N == 0 )
		{
			pc += s8(t);
		}
		return 3;
	case 0x2B: // BMI
		t = read(pc++);
		if( F.b.N == 1 )
		{
			pc += s8(t);
		}
		return 3;
	case 0x2C: // BGE
		t = read(pc++);
		if( F.b.N == F.b.V )
		{
			pc += s8(t);
		}
		return 3;
	case 0x2D: // BLT
		t = read(pc++);
		if( F.b.N != F.b.V )
		{
			pc += s8(t);
		}
		return 3;
	case 0x2E: // BGT
		t = read(pc++);
		if( F.b.N == F.b.V && F.b.Z == 0 )
		{
			pc += s8(t);
		}
		return 3;
	case 0x2F: // BLE
		t = read(pc++);
		if( F.b.N != F.b.V || F.b.Z == 1 )
		{
			pc += s8(t);
		}
		return 3;
	case 0xA8: // EORA indexed
		cycles = 4;
		lea();
		A ^= read(ea);
		setNZ(A);
		F.b.V = 0;
		return cycles;
	case 0xE8: // EORB indexed
		cycles = 4;
		lea();
		B ^= read(ea);
		setNZ(B);
		F.b.V = 0;
		return cycles;
	case 0xA4: // ANDA indexed
		cycles = 4;
		lea();
		A &= read(ea);
		setNZ(A);
		F.b.V = 0;
		return cycles;
	case 0xE4: // ANDB indexed
		cycles = 4;
		lea();
		B &= read(ea);
		setNZ(B);
		F.b.V = 0;
		return cycles;
	case 0xAA: // ORA indexed
		cycles = 4;
		lea();
		A |= read(ea);
		setNZ(A);
		F.b.V = 0;
		return cycles;
	case 0xEA: // ORB indexed
		cycles = 4;
		lea();
		B |= read(ea);
		setNZ(B);
		F.b.V = 0;
		return cycles;
	case 0xA6: // LDA indexed
		cycles = 4;
		lea();
		A = read(ea);
		setNZ(A);
		F.b.V = 0;
		return cycles;
	case 0xE6: // LDB indexed
		cycles = 4;
		lea();
		B = read(ea);
		setNZ(B);
		F.b.V = 0;
		return cycles;
	case 0xEC: // LDD indexed
		cycles = 5;
		lea();
		D(read16(ea));
		setNZ16(D());
		F.b.V = 0;
		return cycles;
	case 0xEE: // LDU indexed
		cycles = 5;
		lea();
		U = read16(ea);
		setNZ16(U);
		F.b.V = 0;
		return cycles;
	case 0xAE: // LDX indexed
		cycles = 5;
		lea();
		X = read16(ea);
		setNZ16(X);
		F.b.V = 0;
		return cycles;
	case 0x95: // BITA direct
		setNZ(A&direct(read(pc++)));
		F.b.V = 0;
		return 2;
	case 0xD5: // BITB direct
		setNZ(B&direct(read(pc++)));
		F.b.V = 0;
		return 2;	
	case 0x96: // LDA direct
		A = direct(read(pc++));
		setNZ(A);
		F.b.V = 0;
		return 4;
	case 0xD6: // LDB direct
		B = direct(read(pc++));
		setNZ(B);
		F.b.V = 0;
		return 4;
	case 0xDC: // LDD direct
		D(direct16(read(pc++)));
		setNZ16(D());
		F.b.V = 0;
		return 5;
	case 0xDE: // LDU direct
		U = direct16(read(pc++));
		setNZ16(U);
		F.b.V = 0;
		return 5;
	case 0x9E: // LDX direct
		X = direct16(read(pc++));
		setNZ16(X);
		F.b.V = 0;
		return 5;
	case 0x4C: // INCA
		A += 1;
		setNZ(A);
		F.b.V = ((A==0x80) ? 1:0);
		return 2;
	case 0x5C: // INCB
		B += 1;
		setNZ(B);
		F.b.V = ((B==0x80) ? 1:0);
		return 2;
	case 0x4A: // DECA
		A -= 1;
		setNZ(A);
		F.b.V = ((A==0x7F) ? 1:0);
		return 2;
	case 0x5A: // DECB
		B -= 1;
		setNZ(B);
		F.b.V = ((B==0x7F) ? 1:0);
		return 2;	
	case 0x0F: // CLR direct
		direct(read(pc++), 0);
		F.b.N = F.b.V = F.b.C = 0;
		F.b.Z = 1;
		return 6;
	case 0x6F: // CLR indexed
		cycles = 6;
		lea();
		write(ea, 0);
		return cycles;
	case 0x7F: // CLR extended
		ea = read16(pc);
		pc += 2;
		write(ea, 0);
		return 7;
	case 0x7C: // INC extended
		ea = read16(pc);
		pc += 2;
		t = read(ea);
		t += 1;
		setNZ(t);
		F.b.V = ((t==0x80) ? 1:0);
		write(ea, t);
		return 7;
	case 0x6C: // INC indexed
		cycles = 6;
		lea();
		t = read(ea);
		t += 1;
		setNZ(t);
		F.b.V = ((t==0x80) ? 1:0);
		write(ea, t);
		return cycles;
	case 0x7A: // DEC extended
		ea = read16(pc);
		pc += 2;
		t = read(ea);
		t -= 1;
		setNZ(t);
		F.b.V = ((t==0x7F) ? 1:0);
		write(ea, t);
		return 7;
	case 0x6A: // DEC indexed
		cycles = 6;
		lea();
		t = read(ea);
		t -= 1;
		setNZ(t);
		F.b.V = ((t==0x7F) ? 1:0);
		write(ea, t);
		return cycles;
	case 0x0C: // INC direct
		opc = read(pc++);
		t = direct(opc);
		t += 1;
		setNZ(t);
		F.b.V = ((t==0x80)? 1:0);
		direct(opc, t);		
		return 6;
	case 0x0A: // DEC direct
		opc = read(pc++);
		t = direct(opc);
		t -= 1;
		setNZ(t);
		F.b.V = ((t==0x7F)? 1:0);
		direct(opc, t);		
		return 6;
	case 0x9A: // ORA direct
		A |= direct(read(pc++));
		setNZ(A);	
		return 4;
	case 0xDA: // ORB direct
		B |= direct(read(pc++));
		setNZ(B);	
		return 4;
	case 0x98: // EORA direct
		A ^= direct(read(pc++));
		setNZ(A);	
		return 4;
	case 0xD8: // EORB direct
		B ^= direct(read(pc++));
		setNZ(B);	
		return 4;
	case 0x94: // ANDA direct
		A &= direct(read(pc++));
		setNZ(A);	
		return 4;
	case 0xD4: // ANDB direct
		B &= direct(read(pc++));
		setNZ(B);	
		return 4;
	case 0x1A: // ORCC
		F.v |= read(pc++);
		return 3;
	case 0x8A: // ORA
		A |= read(pc++);
		setNZ(A);
		F.b.V = 0;
		return 2;
	case 0xCA: // ORB
		B |= read(pc++);
		setNZ(B);
		F.b.V = 0;
		return 2;	
	case 0x86: // LDA
		A = read(pc++);
		setNZ(A);
		F.b.V = 0;
		return 2;
	case 0xC6: // LDB
		B = read(pc++);
		setNZ(B);
		F.b.V = 0;
		return 2;
	case 0xCC: // LDD
		D(read16(pc));
		pc += 2;
		setNZ16(D());
		F.b.V = 0;
		return 3;
	case 0xCE: // LDU
		U = read16(pc);
		pc += 2;
		setNZ16(U);
		F.b.V = 0;
		return 3;
	case 0x8E: // LDX
		X = read16(pc);
		pc += 2;
		setNZ16(X);
		F.b.V = 0;
		return 3;
	case 0x3A: // ABX
		X += B;
		return 3;
	case 0x84: // ANDA
		A &= read(pc++);
		setNZ(A);
		F.b.V = 0;
		return 2;
	case 0xC4: // ANDB
		B &= read(pc++);
		setNZ(B);
		F.b.V = 0;
		return 2;
	case 0x1C: // ANDCC
		F.v &= read(pc++);
		return 3;
	case 0x85: // BITA
		setNZ(A&read(pc++));
		F.b.V = 0;
		return 2;
	case 0xC5: // BITB
		setNZ(B&read(pc++));
		F.b.V = 0;
		return 2;
	case 0x4F: // CLRA
		A = 0;
		F.b.N = F.b.C = F.b.V = 0;
		F.b.Z = 1;
		return 2;
	case 0x5F: // CLRB
		B = 0;
		F.b.N = F.b.C = F.b.V = 0;
		F.b.Z = 1;
		return 2;	
	case 0x43: // COMA
		A = ~A;
		setNZ(A);
		F.b.V = 0;
		F.b.C = 1;
		return 2;
	case 0x53: // COMB
		B = ~B;
		setNZ(B);
		F.b.V = 0;
		F.b.C = 1;
		return 2;	
	case 0x4D: // TSTA
		setNZ(A);
		F.b.V = 0;
		return 2;
	case 0x5D: // TSTB
		setNZ(B);
		F.b.V = 0;
		return 2;
	case 0x0D: // TST direct
		setNZ(direct(read(pc++)));
		F.b.V = 0;
		return 6;
	case 0x3D: // MUL (todo: check flags [esp. carry])
		D(u16(A)*u16(B));
		F.b.Z = ((D()==0)? 1:0);
		F.b.C = ((D()&0x80)? 1:0); // ??
		return 11;
	case 0x88: // EORA
		A ^= read(pc++);
		setNZ(A);
		F.b.V = 0;
		return 2;
	case 0xC8: // EORB
		B ^= read(pc++);
		setNZ(B);
		F.b.V = 0;
		return 2;
	case 0x1E: // EXG
		{
			u8 V1 = getreg(opc&15);
			u8 V2 = getreg(opc>>4);
			setreg(opc>>4, V1);
			setreg(opc&15, V2);
		}
		return 8;
	case 0x1F: // TFR
		opc = read(pc++);
		setreg(opc&15, getreg(opc>>4));
		return 6;
	case 0x35:
	case 0x37:{ // PULS/PULU
		u8 regs = read(pc++);
		u16& p = ((opc==0x37) ? U:S);
		cycles = 5;
		if( regs & BIT(0) )
		{
			F.v = read(p++);
			cycles+=1;
		}
		if( regs & BIT(1) )
		{
			A = read(p++);
			cycles+=1;
		}
		if( regs & BIT(2) )
		{
			B = read(p++);
			cycles+=1;
		}
		if( regs & BIT(3) )
		{
			DP = read(p++);
			cycles+=1;
		}
		if( regs & BIT(4) )
		{
			X = read(p++);
			X <<= 8;
			X |= read(p++);
			cycles+=2;
		}
		if( regs & BIT(5) )
		{
			Y = read(p++);
			Y <<= 8;
			Y |= read(p++);
			cycles+=2;
		}	
		if( regs & BIT(6) )
		{
			u16 tmp = read(p++);
			tmp <<= 8;
			tmp |= read(p++);
			if( opc == 0x37 )
			{
				S = tmp;
			} else {
				U = tmp;
			}
			cycles+=2;
		}	
		if( regs & BIT(7) )
		{
			pc = read(p++);
			pc <<= 8;
			pc |= read(p++);
			cycles+=1;
		}		
		return cycles;	
	}	
	case 0x34:
	case 0x36:{ // PSHS/PSHU
		u8 regs = read(pc++);
		u16& p = ((opc==0x36) ? U:S);
		cycles = 5;
		if( regs & BIT(7) )
		{
			write(--p, pc);
			write(--p, pc>>8);
			cycles+=2;
		}
		if( regs & BIT(6) )
		{
			write(--p, ((opc==0x36) ? S:U)); // ?
			write(--p, ((opc==0x36) ? (S>>8):(U>>8)));
			cycles+=2;
		}
		if( regs & BIT(5) )
		{
			write(--p, Y);
			write(--p, Y>>8);
			cycles+=2;
		}
		if( regs & BIT(4) )
		{
			write(--p, X);
			write(--p, X>>8);
			cycles+=2;
		}
		if( regs & BIT(3) )
		{
			write(--p, DP);
			cycles+=1;
		}		
		if( regs & BIT(2) )
		{
			write(--p, B);
			cycles+=1;
		}
		if( regs & BIT(1) )
		{
			write(--p, A);
			cycles+=1;
		}	
		if( regs & BIT(0) )
		{
			write(--p, F.v);
			cycles+=1;
		}
		return cycles;
	}
	case 0xB6: // LDA extended
		ea = read16(pc);
		pc += 2;
		A = read(ea);
		setNZ(A);
		F.b.V = 0;
		return 5;
	case 0xF6: // LDB extended
		ea = read16(pc);
		pc += 2;
		B = read(ea);
		setNZ(B);
		F.b.V = 0;
		return 5;
	case 0xFC: // LDD extended
		ea = read16(pc);
		pc += 2;
		D(read16(ea));
		setNZ16(D());
		F.b.V = 0;
		return 6;
	case 0xFE: // LDU extended
		ea = read16(pc);
		pc += 2;
		U = read16(ea);
		setNZ16(U);
		F.b.V = 0;
		return 6;
	case 0xBE: // LDX extended
		ea = read16(pc);
		pc += 2;
		X = read16(ea);
		setNZ16(X);
		F.b.V = 0;
		return 6;
	case 0xB7: // STA extended
		ea = read16(pc);
		pc += 2;
		write(ea, A);
		setNZ(A);
		F.b.V = 0;
		return 5;
	case 0xF7: // STB extended
		ea = read16(pc);
		pc += 2;
		write(ea, B);
		setNZ(B);
		F.b.V = 0;
		return 5;
	case 0xFD: // STD extended
		ea = read16(pc);
		pc += 2;
		write16(ea, D());
		setNZ16(D());
		F.b.V = 0;
		return 6;
	case 0xFF: // STU extended
		ea = read16(pc);
		pc += 2;
		write16(ea, U);
		setNZ16(U);
		F.b.V = 0;
		return 6;
	case 0xBF: // STX extended
		ea = read16(pc);
		pc += 2;
		write16(ea, X);
		setNZ16(X);
		F.b.V = 0;
		return 6;
	case 0xA7: // STA indexed
		cycles = 4;
		lea();
		write(ea, A);
		setNZ(A);
		F.b.V = 0;
		return cycles;
	case 0xE7: // STB indexed
		cycles = 4;
		lea();
		write(ea, B);
		setNZ(B);
		F.b.V = 0;
		return cycles;
	case 0xED: // STD indexed
		cycles = 5;
		lea();
		write16(ea, D());
		setNZ16(D());
		F.b.V = 0;
		return cycles;
	case 0xEF: // STU indexed
		cycles = 5;
		lea();
		write16(ea, U);
		setNZ16(U);
		F.b.V = 0;
		return cycles;
	case 0xAF: // STX indexed
		cycles = 5;
		lea();
		write16(ea, X);
		setNZ16(X);
		F.b.V = 0;
		return cycles;
	case 0x97: // STA direct
		direct(read(pc++), A);
		setNZ(A);  //todo: flags on a store?
		F.b.V = 0;	
		return 4;
	case 0xD7: // STB direct
		direct(read(pc++), B);
		setNZ(B);  //todo: flags on a store?
		F.b.V = 0;	
		return 4;
	case 0xDD: // STD direct
		direct16(read(pc++), D());
		setNZ16(D());  //todo: flags on a store?
		F.b.V = 0;	
		return 5;
	case 0xDF: // STU direct
		direct16(read(pc++), U);
		setNZ16(U);  //todo: flags on a store?
		F.b.V = 0;	
		return 5;
	case 0x9F: // STX direct
		direct16(read(pc++), X);
		setNZ16(X);  //todo: flags on a store?
		F.b.V = 0;	
		return 5;
	case 0x1D: // SEX
		A = ((B&0x80) ? 0xff:0);
		setNZ(B);
		return 2;
	case 0x12: return 2; // NOP
	case 0x30: // LEAX
		cycles = 4;
		lea();
		X = ea;
		F.b.Z = ((X==0) ? 1:0);
		return cycles;
	case 0x31: // LEAY
		cycles = 4;
		lea();
		Y = ea;
		F.b.Z = ((Y==0) ? 1:0);
		return cycles;
	case 0x32: // LEAS
		cycles = 4;
		lea();
		S = ea;
		return cycles;
	case 0x33: // LEAU
		cycles = 4;
		lea();
		U = ea;
		return cycles;
	case 0x80: // SUBA imm
		A = add(A, read(pc++)^0xff, 1);
		F.b.C ^= 1;
		return 2;
	case 0x90: // SUBA direct
		A = add(A, direct(read(pc++))^0xff, 1);
		F.b.C ^= 1;
		return 4;
	case 0xA0: // SUBA indexed
		cycles = 4;
		lea();
		A = add(A, read(ea)^0xff, 1);
		F.b.C ^= 1;		
		return cycles;
	case 0xB0: // SUBA extended
		ea = read16(pc);
		pc += 2;
		A = add(A, read(ea)^0xff, 1);
		F.b.C ^= 1;
		return 5;
	case 0xC0: // SUBB imm
		B = add(B, read(pc++)^0xff, 1);
		F.b.C ^= 1;
		return 2;
	case 0xD0: // SUBB direct
		B = add(B, direct(read(pc++))^0xff, 1);
		F.b.C ^= 1;
		return 4;
	case 0xE0: // SUBB indexed
		cycles = 4;
		lea();
		B = add(B, read(ea)^0xff, 1);
		F.b.C ^= 1;		
		return cycles;
	case 0xF0: // SUBB extended
		ea = read16(pc);
		pc += 2;
		B = add(B, read(ea)^0xff, 1);
		F.b.C ^= 1;
		return 5;
	case 0x83: // SUBD imm
		D(add16(D(), read16(pc)^0xffff, 1));
		pc += 2;
		F.b.C ^= 1;
		return 3;
	case 0x93: // SUBD direct
		D(add16(D(), direct16(read(pc++))^0xffff, 1));
		F.b.C ^= 1;
		return 6;
	case 0xA3: // SUBD indexed
		cycles = 6;
		lea();
		D(add16(D(), read16(ea)^0xffff, 1));
		F.b.C ^= 1;		
		return cycles;
	case 0xB3: // SUBD extended
		ea = read16(pc);
		pc += 2;
		D(add16(D(), read16(ea)^0xffff, 1));
		F.b.C ^= 1;
		return 7;
	case 0x82: // SBCA imm
		A = add(A, read(pc++)^0xff, F.b.C^1);
		F.b.C ^= 1;
		return 2;
	case 0x92: // SBCA direct
		A = add(A, direct(read(pc++))^0xff, F.b.C^1);
		F.b.C ^= 1;
		return 4;
	case 0xA2: // SBCA indexed
		cycles = 4;
		lea();
		A = add(A, read(ea)^0xff, F.b.C^1);
		F.b.C ^= 1;
		return cycles;
	case 0xB2: // SBCA extended
		ea = read16(pc);
		pc += 2;
		A = add(A, read(ea)^0xff, F.b.C^1);
		F.b.C ^= 1;		
		return 5;
	case 0xC2: // SBCB imm
		B = add(B, read(pc++)^0xff, F.b.C^1);
		F.b.C ^= 1;
		return 2;
	case 0xD2: // SBCB direct
		B = add(B, direct(read(pc++))^0xff, F.b.C^1);
		F.b.C ^= 1;
		return 4;
	case 0xE2: // SBCB indexed
		cycles = 4;
		lea();
		B = add(B, read(ea)^0xff, F.b.C^1);
		F.b.C ^= 1;
		return cycles;
	case 0xF2: // SBCB extended
		ea = read16(pc);
		pc += 2;
		B = add(B, read(ea)^0xff, F.b.C^1);
		F.b.C ^= 1;		
		return 5;
	case 0x40: // NEGA
		A = add(0, A^0xff, 1);
		F.b.C ^= 1;
		return 2;
	case 0x50: // NEGB
		B = add(0, B^0xff, 1);
		F.b.C ^= 1;
		return 2;
	case 0x00: // NEG direct
		ea = (DP<<8)|read(pc++);
		t = read(ea);
		t = add(0, t^0xff, 1);
		F.b.C ^= 1;
		write(ea, t);
		return 6;
	case 0x60: // NEG indexed
		cycles = 6;
		lea();
		t = read(ea);
		t = add(0, t^0xff, 1);
		F.b.C ^= 1;
		write(ea, t);
		return cycles;
	case 0x70: // NEG extended
		ea = read16(pc);
		pc += 2;
		t = read(ea);
		t = add(0, t^0xff, 1);
		F.b.C ^= 1;
		write(ea, t);
		return 6;
	case 0x49: // ROLA
		t = F.b.C;
		F.b.C = A>>7;
		A = (A<<1)|t;
		setNZ(A);
		F.b.V = F.b.C ^ F.b.N;
		return 2;
	case 0x59: // ROLB
		t = F.b.C;
		F.b.C = B>>7;
		B = (B<<1)|t;
		setNZ(B);
		F.b.V = F.b.C ^ F.b.N;
		return 2;
	case 0x09: // ROL direct
		ea = (DP<<8)|read(pc++);
		opc = read(ea);
		t = F.b.C;
		F.b.C = opc>>7;
		opc = (opc<<1)|t;
		write(ea, opc);
		setNZ(opc);
		F.b.V = F.b.C ^ F.b.N;
		return 6;
	case 0x69: // ROL indexed
		cycles = 6;
		lea();
		opc = read(ea);
		t = F.b.C;
		F.b.C = opc>>7;
		opc = (opc<<1)|t;
		write(ea, opc);
		setNZ(opc);
		F.b.V = F.b.C ^ F.b.N;
		return cycles;
	case 0x79: // ROL expanded
		ea = read16(pc);
		pc += 2;
		opc = read(ea);
		t = F.b.C;
		F.b.C = opc>>7;
		opc = (opc<<1)|t;
		write(ea, opc);
		setNZ(opc);
		F.b.V = F.b.C ^ F.b.N;
		return 7;
	case 0x46: // RORA
		t = F.b.C<<7;
		F.b.C = A&1;
		A = (A>>1)|t;
		setNZ(A);
		return 2;
	case 0x56: // RORB
		t = F.b.C<<7;
		F.b.C = B&1;
		B = (B>>1)|t;
		setNZ(A);
		return 2;
	case 0x06: // ROR direct
		ea = (DP<<8)|read(pc++);
		opc = read(ea);
		t = F.b.C<<7;
		F.b.C = opc&1;
		opc = (opc>>1)|t;
		write(ea, opc);
		setNZ(A);
		return 6;
	case 0x66: // ROR direct
		cycles = 6;
		lea();
		opc = read(ea);
		t = F.b.C<<7;
		F.b.C = opc&1;
		opc = (opc>>1)|t;
		write(ea, opc);
		setNZ(A);
		return cycles;
	case 0x76: // ROR expanded
		ea = read16(pc);
		pc += 2;
		opc = read(ea);
		t = F.b.C<<7;
		F.b.C = opc&1;
		opc = (opc>>1)|t;
		write(ea, opc);
		setNZ(A);
		return cycles;
	case 0x3B: // RTI
		F.v = pop8();
		if( F.b.E )
		{
			A = pop8();
			B = pop8();
			DP = pop8();
			X = pop16();
			Y = pop16();
			U = pop16();
		}
		pc = pop16();
		return (F.b.E ? 15:6);
	case 0x81: // CMPA imm
		add(A, read(pc++)^0xff, 1);
		F.b.C ^= 1;
		return 2;
	case 0x91: // CMPA direct
		add(A, direct(read(pc++))^0xff, 1);
		F.b.C ^= 1;
		return 4;
	case 0xA1: // CMPA indexed
		cycles = 4;
		lea();
		add(A, read(ea)^0xff, 1);
		F.b.C ^= 1;
		return cycles;
	case 0xB1: // CMPA expanded
		ea = read16(pc);
		pc += 2;
		add(A, read(ea)^0xff, 1);
		F.b.C ^= 1;
		return 5;
	case 0xC1: // CMPB imm
		add(B, read(pc++)^0xff, 1);
		F.b.C ^= 1;
		return 2;
	case 0xD1: // CMPB direct
		add(B, direct(read(pc++))^0xff, 1);
		F.b.C ^= 1;
		return 4;
	case 0xE1: // CMPB indexed
		cycles = 4;
		lea();
		add(B, read(ea)^0xff, 1);
		F.b.C ^= 1;
		return cycles;
	case 0xF1: // CMPB expanded
		ea = read16(pc);
		pc += 2;
		add(B, read(ea)^0xff, 1);
		F.b.C ^= 1;
		return 5;
	case 0x8C: // CMPX imm
		ea = read16(pc);
		add16(X, ea^0xffff, 1);
		pc += 2;
		F.b.C ^= 1;
		return 4;
	case 0x9C: // CMPX direct
		add16(X, direct16(read(pc++))^0xffff, 1);
		F.b.C ^= 1;
		return 6;
	case 0xAC: // CMPX indexed
		cycles = 6;
		lea();
		add16(X, read16(ea)^0xffff, 1);
		F.b.C ^= 1;
		return cycles;
	case 0xBC: // CMPX expanded
		ea = read16(pc);
		pc += 2;
		add16(X, read16(ea)^0xffff, 1);
		F.b.C ^= 1;
		return 7;
	case 0x8D: // BSR
		t = read(pc++);
		push16(pc);
		pc += s8(t);
		return 7;
	case 0x6D: // TST indexed
		cycles = 6;
		lea();
		t = read(ea);
		setNZ(t);
		F.b.V = 0;
		return cycles;
	case 0x7D: // TST extended
		ea = read16(pc);
		pc += 2;
		t = read(ea);
		setNZ(t);
		F.b.V = 0;
		return 7;
	default:
		std::println("6809:${:X}: unimpl opc ${:X}", pc, opc);
		exit(1);
	}
	
	return 1;
}

void c6809::lea()
{
	pb = read(pc++);
	u16* I = nullptr;
	switch( (pb>>5)&3 )
	{
	case 0: I = &X; break;
	case 1: I = &Y; break;
	case 2: I = &U; break;
	case 3: I = &S; break;
	}
	
	if( pb < 0x80 )
	{
		ea = *I;
		ea += s8(pb<<3)>>3;
		cycles += 1;
		return;
	}
	
	switch( pb & 15 )
	{
	case 0: // ,R+
		ea = *I;
		*I += 1;
		cycles += 2;
		return;
	case 1: // ,R++
		ea = *I;
		*I += 2;
		if( pb & BIT(4) )
		{
			ea = read16(ea);
			cycles += 3;
		}
		cycles += 3;
		return;
	case 2: // ,-R
		*I -= 1;
		ea = *I;
		cycles += 2;
		return;
	case 3: // ,--R
		*I -= 2;
		ea = *I;
		if( pb & BIT(4) )
		{
			ea = read16(ea);
			cycles += 3;
		}
		cycles += 3;
		return;
	case 4: // ,R + no offset
		ea = *I;
		if( pb & BIT(4) )
		{
			ea = read16(ea);
			cycles += 3;
		}
		return;
	case 5: // ,R + B offset
		ea = *I;
		ea += s8(B);
		if( pb & BIT(4) )
		{
			ea = read16(ea);
			cycles += 3;
		}
		cycles += 1;
		return;
	case 6: // ,R + A offset
		ea = *I;
		ea += s8(A);
		if( pb & BIT(4) )
		{
			ea = read16(ea);
			cycles += 3;
		}
		cycles += 1;
		return;
	case 8: // ,R + 8bit offset
		ea = *I;
		ea += s8(read(pc++));
		if( pb & BIT(4) )
		{
			ea = read16(ea);
			cycles += 3;
		}
		cycles += 1;
		return;
	case 9: // ,R + 16bit offset
		ea = *I;
		ea += read16(pc);
		pc += 2;
		if( pb & BIT(4) )
		{
			ea = read16(ea);
			cycles += 3;
		}
		cycles += 4;
		return;
	case 11: // ,R + D offset
		ea = *I;
		ea += D();
		if( pb & BIT(4) )
		{
			ea = read16(ea);
			cycles += 3;
		}
		cycles += 4;
		return;
	case 12: // ,PC + 8bit offset
		ea = pc+1;
		ea += s8(read(pc++));
		if( pb & BIT(4) )
		{
			ea = read16(ea);
			cycles += 3;
		}
		cycles += 1;
		return;
	case 13: // ,PC + 16bit offset		
		ea = pc+2;
		ea += read16(pc);
		pc += 2;
		if( pb & BIT(4) )
		{
			ea = read16(ea);
			cycles += 3;
		}
		cycles += 5;
		return;
	case 15: // EA = Addr
		ea = read16(pc);
		pc += 2;
		if( pb & BIT(4) )
		{
			ea = read16(ea);
			cycles += 3;
		}
		cycles += 5;
		//todo: ^ this is a total guess, manual gives
		//   5 cycles for Extended Indirect, but says nothing
		//   about Extended Direct which seems like it should
		//   be a thing based on the encoding. You just normally
		//   wouldn't do that because "Extended" mode opcodes exist.
		return;
	}
	
	std::println("6809:lea(): pb = ${:X}", pb);
	std::println("todo: figure out what happens for unused address mode values");
	exit(1);
}

u8 c6809::add(u8 a, u8 b, u8 c)
{
	u16 res = a;
	res += b;
	res += c;
	setNZ(u8(res));
	F.b.H = ((a&15) + (b&15) + c)>>4;
	F.b.C = res>>8;
	F.b.V = (((a^res) & (b^res) & 0x80)? 1:0);
	return res;
}

u16 c6809::add16(u16 a, u16 b, u16 c)
{
	u32 res = a;
	res += b;
	res += c;
	setNZ16(u16(res));
	F.b.C = res>>16;
	F.b.V = (((a^res) & (b^res) & 0x8000)? 1:0);
	return res;
}












