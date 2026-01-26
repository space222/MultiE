#include <cstdio>
#include <cstdlib>
//#include <print>
#include "r3000.h"

#define LOAD(R, V) ld_r[1] = (R); ld_v[1] = (V)
#define DEST(R) if( (R) == ld_r[0] ) ld_r[0] = ld_v[0] = 0

#define S ((opc>>21)&0x1F)
#define T ((opc>>16)&0x1F)
#define D ((opc>>11)&0x1F)
#define imm5 ((opc>>6)&0x1F)
#define imm16 (opc&0xffff)
#define simm16 s16(u16(opc&0xffff))

void r3000::exec(u32 opc)
{
	u32 temp = 0;
	
	if( (opc>>26) == 0 )
	{ // special
		switch( opc & 0x3F )
		{
		case 0: r[D] = r[T] << imm5; DEST(D); break;
		case 2: r[D] = r[T] >> imm5; DEST(D); break;
		case 3: r[D] = s32(r[T]) >> imm5; DEST(D); break;
		case 4: r[D] = r[T] << (r[S]&0x1F); DEST(D); break;
		case 6: r[D] = r[T] >> (r[S]&0x1F); DEST(D); break;
		case 7: r[D] = s32(r[T]) >> (r[S]&0x1F); DEST(D); break;
		case 8: nnpc = r[S]; in_delay = 2; break;
		case 9: temp = nnpc; nnpc = r[S]; r[D] = temp; DEST(D); in_delay = 2; break;
		case 0x0C:
			//printf("$%X: SYSCALL!\n", pc); 
			c[13] &= ~0xff;
			c[13] |= 8<<2;
			c[12] = (c[12]&~0xff)|((c[12]&0xf)<<2);
			c[14] = pc;
			npc = 0x80000080;
			nnpc = npc + 4;			
			break;
		case 0x0D: printf("BREAK!\n"); break;// exit(1); break; 
		case 0x10: r[D] = hi; DEST(D); break;
		case 0x11: hi = r[S]; break;
		case 0x12: r[D] = lo; DEST(D); break;
		case 0x13: lo = r[S]; break;
		case 0x18: { s64 a = s32(r[S]); a *= s64(s32(r[T])); hi = a>>32; lo = a; } break;
		case 0x19: { u64 a = r[S]; a *= u64(r[T]); hi = a>>32; lo = a; } break;
		case 0x1A:
			if( r[T] == 0 )
			{
				lo = ( s32(r[S]) >= 0 ) ? 0xffffFFFFu : 1;
				hi = r[S];
			} else if( r[T] == 0xffffFFFFu && r[S] == 0x80000000u ) {
				hi = 0;
				lo = 0x80000000u;
			} else {
				lo = s32(r[S])/s32(r[T]);
				hi = s32(r[S])%s32(r[T]);
			}
			break;
		case 0x1B:
			if( r[T] == 0 )
			{
				hi = r[S];
				lo = 0xffffFFFFu;			
			} else {
				lo = r[S]/r[T];
				hi = r[S]%r[T];
			}
			break;
		case 0x20: r[D] = r[S] + r[T]; DEST(D); break;
		case 0x21: r[D] = r[S] + r[T]; DEST(D); break;
		case 0x22: r[D] = r[S] - r[T]; DEST(D); break;
		case 0x23: r[D] = r[S] - r[T]; DEST(D); break;
		case 0x24: r[D] = r[S] & r[T]; DEST(D); break;
		case 0x25: r[D] = r[S] | r[T]; DEST(D); break;
		case 0x26: r[D] = r[S] ^ r[T]; DEST(D); break;
		case 0x27: r[D] = ~(r[S] | r[T]); DEST(D); break;
		case 0x2A: r[D] = s32(r[S]) < s32(r[T]); DEST(D); break;
		case 0x2B: r[D] = r[S] < r[T]; DEST(D); break;
		
		default:
			printf("Unimpl special opc = $%X\n", opc&0x3F);
			exit(1);		
		}
		return;
	}
	
	if( (opc>>26) == 1 )
	{  // BcondZ
		temp = nnpc;
		if( T & 1 )
		{
			if( s32(r[S]) >= 0 ) { nnpc = npc + (simm16<<2); in_delay = 2; }		
		} else {
			if( s32(r[S]) <  0 ) { nnpc = npc + (simm16<<2); in_delay = 2; }
		}
		if( (T & 0x1e) == 0x10 )
		{
			r[31] = temp;
			DEST(31);
		}
		return;
	}
	
	switch( opc>>26 )
	{
	case 2: nnpc = (npc&0xf0000000u)|((opc&0x3FFffffu)<<2); in_delay = 2; break;
	case 3: temp = nnpc; nnpc = (npc&0xf0000000u)|((opc&0x3FFffffu)<<2); r[31] = temp; DEST(31); in_delay = 2; break;
	case 4: if( r[S] == r[T] ) { nnpc = npc + (simm16<<2); in_delay = 2; } break;
	case 5: if( r[S] != r[T] ) { nnpc = npc + (simm16<<2); in_delay = 2; } break;
	case 6: if( s32(r[S]) <= 0 ) { nnpc = npc + (simm16<<2); in_delay = 2; } break;
	case 7: if( s32(r[S]) > 0 ) { nnpc = npc + (simm16<<2); in_delay = 2; } break;
	case 8: r[T] = r[S] + simm16; DEST(T); break;
	case 9: r[T] = r[S] + simm16; DEST(T); break;
	case 0x0A: r[T] = s32(r[S]) < simm16; DEST(T); break;
	case 0x0B: r[T] = r[S] < u32(simm16); DEST(T); break;
	case 0x0C: r[T] = r[S] & imm16; DEST(T); break;
	case 0x0D: r[T] = r[S] | imm16; DEST(T); break;
	case 0x0E: r[T] = r[S] ^ imm16; DEST(T); break;
	case 0x0F: r[T] = imm16 << 16; DEST(T); break;

	case 0x10: cop0(opc); break;
	case 0x11: printf("%X: COP1!\n", pc); exit(1); break;
	case 0x12: gte_exec(opc); break;
	case 0x13: printf("%X: COP3!\n", pc); exit(1); break;
	
	case 0x20: LOAD(T, (s32)(s8)read(r[S]+simm16, 8)); DEST(T); break;
	case 0x21: LOAD(T, (s32)(s16)read(r[S]+simm16, 16)); DEST(T); break;
	case 0x23: LOAD(T, read(r[S]+simm16, 32)); DEST(T); break;
	case 0x24: LOAD(T, read(r[S]+simm16, 8)); DEST(T); break;
	case 0x25: LOAD(T, read(r[S]+simm16, 16)); DEST(T); break;
	case 0x28: write(r[S]+simm16, r[T], 8); break;
	case 0x29: write(r[S]+simm16, r[T], 16); break;
	case 0x2B: write(r[S]+simm16, r[T], 32); break;
	
	case 0x22:{ // lwl
		u32* v = (ld_r[0] == T) ? &ld_v[0] : &r[T];
		temp = read((r[S]+simm16)&~3, 32);
		switch((r[S]+simm16)&3)
		{
		case 0: temp <<= 24; temp |= *v & 0xffFFff; break;
		case 1: temp <<= 16; temp |= *v & 0xffff; break;
		case 2: temp <<= 8;  temp |= *v & 0xff; break;
		case 3: break;
		}
		LOAD(T, temp);
		DEST(T);
		}break;
	case 0x26:{ // lwr
		u32* v = (ld_r[0] == T) ? &ld_v[0] : &r[T];
		temp = read((r[S]+simm16)&~3, 32);
		switch((r[S]+simm16)&3)
		{
		case 0: break;
		case 1: temp >>= 8; temp |= *v & 0xff000000; break;
		case 2: temp >>= 16; temp |= *v & 0xffff0000; break;
		case 3: temp >>= 24; temp |= *v & 0xffFFff00; break;
		}
		LOAD(T, temp);
		DEST(T);
		}break;	
		
	case 0x2A: // swl
		temp = read((r[S]+simm16)&~3, 32);
		switch((r[S]+simm16)&3)
		{
		case 0: write((r[S]+simm16)&~3, (temp&~0xff)|(r[T]>>24), 32); break;
		case 1: write((r[S]+simm16)&~3, (temp&~0xffFF)|(r[T]>>16), 32); break;
		case 2: write((r[S]+simm16)&~3, (temp&~0xffFFff)|(r[T]>>8), 32); break;
		case 3: write((r[S]+simm16)&~3, r[T], 32); break;
		}		
		break;
	case 0x2E: // swr
		temp = read((r[S]+simm16)&~3, 32);
		switch((r[S]+simm16)&3)
		{
		case 0: write((r[S]+simm16)&~3, r[T], 32); break;
		case 1: write((r[S]+simm16)&~3, (temp&~0xffFFff00)|(r[T]<<8), 32); break;
		case 2: write((r[S]+simm16)&~3, (temp&~0xffFF0000)|(r[T]<<16), 32); break;
		case 3: write((r[S]+simm16)&~3, (temp&~0xff000000)|(r[T]<<24), 32); break;
		}		
		break;
	
	case 0x30: c[T] = read(r[S]+simm16, 32); break;
	case 0x31:
	case 0x33: break;
	case 0x32: gte_load_d(T, read(r[S]+simm16, 32)); break;
	
	case 0x38:
		write(r[S]+simm16, c[T], 32);
		break;
	case 0x39:
	case 0x3B: break;
	case 0x3A: write(r[S]+simm16, gte_get_d(T), 32); break;
	default:
		printf("Unimpl regular opcode = $%X\n", opc>>26);
		exit(1);
	}
	return;
}

void r3000::cop0(u32 opc)
{
	//printf("COP0!: opc = $%X, ", opc);
	if( S & 0x10 )
	{
		//printf("RFE\n");
		c[12] = (c[12]&~0xf)|((c[12]>>2)&0xf);
		return;
	}
	
	switch( S )
	{
	case 0:{
		//LOAD(T, c[D]);
		//DEST(T); 
		r[T] = c[D];
		//printf("r%i = c%i\n", T, D); 
		}break;
	case 4:
		if( D != 13 )
		{
			c[D] = r[T];
		}
		//printf("c%i = $%X\n", D, r[T]);
		break;
	default:
		printf("Unimpl cop0 opcode = $%X\n", opc);
		exit(1);
	}	
}

void r3000::step()
{
	u32 opc = read(pc&~3, 32);
	//std::println("opc = ${:X}", opc);
	
	if( ((pc&0x1ffffff) == 0x00000A0 || (pc&0x1ffffff) == 0x00000B0) )
	{
		if( r[9] == 0x3D || r[9] == 0x3C )
		{
			fputc(r[4],stderr);
		}	
	}
	
	exec(opc);
	if( in_delay )
	{
		in_delay -= 1;
	}
	
	pc = npc;
	npc = nnpc;
	nnpc += 4;

	advance_load();
	r[0] = 0;
}

void r3000::advance_load()
{
	r[ld_r[0]] = ld_v[0];
	ld_r[0] = ld_r[1];
	ld_v[0] = ld_v[1];
	ld_r[1] = ld_v[1] = 0;
}

void r3000::reset()
{
	pc = 0xBFC00000u;
	for(int i = 0; i < 32; ++i) 
	{
		r[i] = c[i] = 0;
	}
	npc = pc + 4;
	nnpc = pc + 8;
	in_delay = 0;
	ld_r[0] = ld_r[1] = 0;
	ld_v[0] = ld_v[1] = 0;
}



