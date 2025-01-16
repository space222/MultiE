#include <cstdio>
#include <cstdlib>
#include "n64_rsp.h"

typedef void (*rsp_instr)(n64_rsp&, u32);
#define INSTR [](n64_rsp& rsp, u32 opc)
#define VUWC2 u32 vt = (opc>>16)&0x1f; u32 base = (opc>>21)&0x1f; u32 e = (opc>>7)&15; s32 offs = s8(opc<<1)>>1
#define VUFC2 u32 rt = (opc>>16)&0x1f; u32 vs = (opc>>11)&0x1f; u32 e = (opc>>7)&15
#define VOPP u32 e=(opc>>21)&15; u32 vt=(opc>>16)&0x1f; u32 vs=(opc>>11)&0x1f; u32 vd=(opc>>6)&0x1f

u32 broadcast[] = {
	0,1,2,3,4,5,6,7,
	0,1,2,3,4,5,6,7,
	0,0,2,2,4,4,6,6,
	1,1,3,3,5,5,7,7,
	0,0,0,0,4,4,4,4,
	1,1,1,1,5,5,5,5,
	2,2,2,2,6,6,6,6,
	3,3,3,3,7,7,7,7,
	0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,
	3,3,3,3,3,3,3,3,
	4,4,4,4,4,4,4,4,
	5,5,5,5,5,5,5,5,
	6,6,6,6,6,6,6,6,
	7,7,7,7,7,7,7,7 
};
#define BC(b) broadcast[((e<<3)+(b))]
#define VCO_BIT(b) ((rsp.VCO>>(b))&1)

s16 clamp_signed(s32 accum)
{
	if( accum < -32768 ) return -32768;
	if( accum > 32767 ) return 32767;
	return accum;
}

u16 clamp_unsigned(s32 accum)
{
	if( accum < 0 ) return 0;
	if( accum > 32767 ) return 65535;
	return accum;
}

u16 clamp_unsigned_x(s64 accum)
{
	u16 ACCHI = accum>>32;
	u16 ACCMD = accum>>16;
	u16 ACCLO = accum;
	if( (ACCHI==0xffff&&(ACCMD&BIT(15))) || (ACCHI==0&&!(ACCMD&BIT(15))) ) return ACCLO;
	if( ACCHI & BIT(15) ) return 0;
	return 65535;
}

rsp_instr rsp_cop2(n64_rsp& rsp, u32 opcode)
{
	if( !(opcode & BIT(25)) )
	{
		switch( (opcode>>21) & 0xf )
		{
		case 0: // MFC2
			return INSTR {
				VUFC2;
				s16 r = rsp.v[vs].b(e)<<8;
				r |= rsp.v[vs].b(e+1);
				rsp.r[rt] = r;
			};
		case 2: // CFC2
			return INSTR {
				VUFC2;
				switch( vs & 3 )
				{
				case 0:	rsp.r[rt] = s16(rsp.VCO); break;
				case 1: rsp.r[rt] = s16(rsp.VCC); break;
				case 2:
				case 3: rsp.r[rt] = rsp.VCE; break;
				default: break;
				}
			};
		case 4: // MTC2
			return INSTR {
				VUFC2;
				rsp.v[vs].b(e) = rsp.r[rt]>>8;
				if( e != 15 ) rsp.v[vs].b(e+1) = rsp.r[rt];
			};
		case 6: // CTC2
			return INSTR {
				VUFC2;
				switch( vs & 3 )
				{
				case 0:	rsp.VCO = rsp.r[rt]; break;
				case 1: rsp.VCC = rsp.r[rt]; break;
				case 2: 
				case 3: rsp.VCE = rsp.r[rt] & 0xff; break;
				default: break;
				}
			};
		default:
			printf("RSP: Unimpl cop2 ctrl opcode = $%X\n", (opcode>>21)&0xf);
			exit(1);
			return INSTR {};
		}	
	}
	
	switch( opcode & 0x3F )
	{
	case 0x00: // VMULF
		return INSTR {
			VOPP;
			vreg res;
			for(u32 i = 0; i < 8; ++i)
			{
				s64 prod = rsp.v[vs].sw(i);
				prod *= rsp.v[vt].sw(BC(i));
				prod *= 2;
				rsp.a[i] = prod + 0x8000;
				res.w(i) = clamp_signed(rsp.a[i]>>16);
			}		
			rsp.v[vd] = res;
		};		
	case 0x01: // VMULU
		return INSTR {
			VOPP;
			vreg res;
			for(u32 i = 0; i < 8; ++i)
			{
				s64 prod = rsp.v[vs].sw(i);
				prod *= rsp.v[vt].sw(BC(i));
				prod *= 2;
				rsp.a[i] = prod + 0x8000;
				res.w(i) = clamp_unsigned(rsp.a[i]>>16);
			}		
			rsp.v[vd] = res;
		};
	case 0x04: // VMUDL
		return INSTR {
			VOPP;
			vreg res;
			for(u32 i = 0; i < 8; ++i)
			{
				u32 prod = rsp.v[vs].w(i);
				prod *= rsp.v[vt].w(BC(i));
				rsp.a[i] = (prod>>16)&0xffff;
				res.w(i) = clamp_unsigned_x(rsp.a[i]);
			}		
			rsp.v[vd] = res;
		};		
	
	case 0x05: // VMUDM
		return INSTR {
			VOPP;
			vreg res;
			for(u32 i = 0; i < 8; ++i)
			{
				s64 prod = s64(rsp.v[vs].sw(i)) * u64(rsp.v[vt].w(BC(i)));
				rsp.a[i] = s64(prod) & 0xffffFFFFffffull;
				res.w(i) = clamp_signed(rsp.a[i]>>16);
			}		
			rsp.v[vd] = res;
		};	
	case 0x06: // VMUDN
		return INSTR {
			VOPP;
			vreg res;
			for(u32 i = 0; i < 8; ++i)
			{
				s32 prod = rsp.v[vs].w(i);
				prod *= rsp.v[vt].sw(BC(i));
				rsp.a[i] = prod;
				res.w(i) = clamp_unsigned_x(rsp.a[i]);
			}
			rsp.v[vd] = res;	
		};	
	case 0x07: // VMUDH
		return INSTR {
			VOPP;
			vreg res;
			for(u32 i = 0; i < 8; ++i)
			{
				s32 prod = rsp.v[vs].sw(i);
				prod *= rsp.v[vt].sw(BC(i));
				rsp.a[i] = u64(prod)<<16;
				res.w(i) = clamp_signed(rsp.a[i]>>16);
			}
			rsp.v[vd] = res;
		};		
	case 0x08: // VMACF
		return INSTR {
			VOPP;
			vreg res;
			for(u32 i = 0; i < 8; ++i)
			{
				s64 prod = rsp.v[vs].sw(i);
				prod *= rsp.v[vt].sw(BC(i));
				prod *= 2;
				rsp.a[i] &= 0xffffFFFFffffull;
				rsp.a[i] += prod&0xffffFFFFffffull;
				res.w(i) = clamp_signed(rsp.a[i]>>16);
			}		
			rsp.v[vd] = res;
		};
	case 0x09: // VMACU
		return INSTR {
			VOPP;
			vreg res;
			for(u32 i = 0; i < 8; ++i)
			{
				s64 prod = rsp.v[vs].sw(i);
				prod *= rsp.v[vt].sw(BC(i));
				prod *= 2;
				rsp.a[i] &= 0xffffFFFFffffull;
				rsp.a[i] += prod;
				res.w(i) = clamp_unsigned(rsp.a[i]>>16);
			}		
			rsp.v[vd] = res;
		};
	case 0x0C: // VMADL
		return INSTR {
			VOPP;
			vreg res;
			for(u32 i = 0; i < 8; ++i)
			{
				u32 prod = rsp.v[vs].w(i);
				prod *= rsp.v[vt].w(BC(i));
				rsp.a[i] += (prod>>16)&0xffff;
				res.w(i) = clamp_unsigned_x(rsp.a[i]);
			}		
			rsp.v[vd] = res;
		};
	case 0x0D: // VMADM
		return INSTR {
			VOPP;
			vreg res;
			for(u32 i = 0; i < 8; ++i)
			{
				s64 prod = rsp.v[vs].sw(i);
				prod *= rsp.v[vt].w(BC(i));
				rsp.a[i] &= 0xffffFFFFffffull;
				rsp.a[i] += prod & 0xffffFFFFffffull;
				res.w(i) = clamp_signed(rsp.a[i]>>16);
			}
			rsp.v[vd] = res;
		};	
	case 0x0E: // VMADN
		return INSTR {
			VOPP;
			vreg res;
			for(u32 i = 0; i < 8; ++i)
			{
				s64 prod = rsp.v[vs].w(i);
				prod *= rsp.v[vt].sw(BC(i));
				rsp.a[i] += prod;
				res.w(i) = clamp_unsigned_x(rsp.a[i]);
			}
			rsp.v[vd] = res;
		};
	case 0x0F: // VMADH
		return INSTR {
			VOPP;
			vreg res;
			for(u32 i = 0; i < 8; ++i)
			{
				s32 prod = rsp.v[vs].sw(i);
				prod *= rsp.v[vt].sw(BC(i));
				rsp.a[i] += u64(prod)<<16;
				res.w(i) = clamp_signed(rsp.a[i]>>16);
			}		
			rsp.v[vd] = res;
		};	
	case 0x10: // VADD
		return INSTR {
			VOPP;
			vreg res;
			for(u32 i = 0; i < 8; ++i)
			{
				s32 r = rsp.v[vs].sw(i);
				r += rsp.v[vt].sw(BC(i));
				r += VCO_BIT(i);
				rsp.a[i] &= ~0xffffull;
				rsp.a[i] |= r&0xffff;
				res.sw(i) = clamp_signed(r);			
			}
			rsp.VCO = 0;
			rsp.v[vd] = res;
		};
	case 0x11: // VSUB
		return INSTR {
			VOPP;
			vreg res;
			for(u32 i = 0; i < 8; ++i)
			{
				s32 r = rsp.v[vs].sw(i);
				r -= rsp.v[vt].sw(BC(i));
				r -= VCO_BIT(i);
				rsp.a[i] &= ~0xffffull;
				rsp.a[i] |= r&0xffff;
				res.sw(i) = clamp_signed(r);			
			}
			rsp.VCO = 0;
			rsp.v[vd] = res;
		};
		
	case 0x14: // VADDC
		return INSTR {
			VOPP;
			vreg res;
			rsp.VCO = 0;
			for(u32 i = 0; i < 8; ++i)
			{
				u32 add = rsp.v[vs].w(i) + rsp.v[vt].w(BC(i));
				rsp.a[i] &= ~0xffffull;
				rsp.a[i] |= add & 0xffff;
				res.w(i) = add;
				rsp.VCO |= (add&BIT(16))?(1<<i):0;
			}
			rsp.v[vd] = res;		
		};
	case 0x15: // VSUBC
		return INSTR {
			VOPP;
			vreg res;
			rsp.VCO = 0;
			for(u32 i = 0; i < 8; ++i)
			{
				u32 add = rsp.v[vs].w(i) - rsp.v[vt].w(BC(i));
				rsp.a[i] &= ~0xffffull;
				rsp.a[i] |= add & 0xffff;
				res.w(i) = add;
				rsp.VCO |= (add&BIT(16))?(1<<i):0;
				rsp.VCO |= (add&0x1ffff)?(1<<(8+i)):0;
			}
			rsp.v[vd] = res;		
		};	
	case 0x1D: // VSAR
		return INSTR {
			VOPP;
			for(u32 i = 0; i < 8; ++i)
			{
				if( e == 9 ) { rsp.v[vd].w(i) = rsp.a[i]>>16; }
				else if( e == 10 ) { rsp.v[vd].w(i) = rsp.a[i]; }
				else { rsp.v[vd].w(i) = rsp.a[i]>>32; }
			}		
		};
	case 0x20: // VLT
		return INSTR {
			VOPP;
			vreg res;
			rsp.VCC = 0;
			for(u32 i = 0; i < 8; ++i)
			{
				u32 egl = rsp.v[vs].w(i) == rsp.v[vt].w(BC(i));
				u32 neg = ((rsp.VCO&BIT(i+8)) && (rsp.VCO&BIT(i))) && egl;
				rsp.VCC |= (neg || (rsp.v[vs].sw(i) < rsp.v[vt].sw(BC(i))) ) ? (1<<i) : 0;
				rsp.a[i] &= ~0xffffull;
				rsp.a[i] |= res.w(i) = ((rsp.VCC&BIT(i))? rsp.v[vs].w(i) : rsp.v[vt].w(BC(i)));
			}
			rsp.VCC &= 0xff;
			rsp.VCO = 0;
			rsp.v[vd] = res;
		};
	case 0x21: // VEQ
		return INSTR {
			VOPP;
			vreg res;
			rsp.VCC = 0;
			for(u32 i = 0; i < 8; ++i)
			{
				rsp.VCC |= (!(rsp.VCO&BIT(i+8)) && (rsp.v[vs].w(i) == rsp.v[vt].w(BC(i))) ) ? (1<<i) : 0;
				rsp.a[i] &= ~0xffffull;
				rsp.a[i] |= res.w(i) = ((rsp.VCC&BIT(i))? rsp.v[vs].w(i) : rsp.v[vt].w(BC(i)));
			}
			rsp.VCC &= 0xff;
			rsp.VCO = 0;
			rsp.v[vd] = res;
		};
	case 0x22: // VNE
		return INSTR {
			VOPP;
			vreg res;
			rsp.VCC = 0;
			for(u32 i = 0; i < 8; ++i)
			{
				rsp.VCC |= ((rsp.VCO&BIT(i+8)) || (rsp.v[vs].w(i) != rsp.v[vt].w(BC(i))) ) ? (1<<i) : 0;
				rsp.a[i] &= ~0xffffull;
				rsp.a[i] |= res.w(i) = ((rsp.VCC&BIT(i))? rsp.v[vs].w(i) : rsp.v[vt].w(BC(i)));
			}
			rsp.VCC &= 0xff;
			rsp.VCO = 0;
			rsp.v[vd] = res;
		};	
	case 0x23: // VGE
		return INSTR {
			VOPP;
			vreg res;
			rsp.VCC = 0;
			for(u32 i = 0; i < 8; ++i)
			{
				u32 egl = rsp.v[vs].w(i) == rsp.v[vt].w(BC(i));
				u32 neg = !((rsp.VCO&BIT(i+8)) && (rsp.VCO&BIT(i))) && egl;
				rsp.VCC |= (neg || (rsp.v[vs].sw(i) > rsp.v[vt].sw(BC(i))) ) ? (1<<i) : 0;
				rsp.a[i] &= ~0xffffull;
				rsp.a[i] |= res.w(i) = ((rsp.VCC&BIT(i))? rsp.v[vs].w(i) : rsp.v[vt].w(BC(i)));
			}
			rsp.VCC &= 0xff;
			rsp.VCO = 0;
			rsp.v[vd] = res;
		};
	case 0x25: // VCH  //todo: not correct
		return INSTR {
			VOPP;
			vreg res;
			rsp.VCC = rsp.VCE = rsp.VCO = 0;
			for(u32 i = 0; i < 8; ++i)
			{
				rsp.VCO |= (rsp.v[vs].w(i) ^ rsp.v[vt].w(BC(i)) & BIT(15)) ? (1<<i) : 0;
				s16 vtabs = (rsp.VCO&BIT(i)) ? -rsp.v[vt].sw(BC(i)) : rsp.v[vt].sw(BC(i));
				rsp.VCE |= ((rsp.VCO&BIT(i)) && (rsp.v[vs].sw(i) == (-rsp.v[vt].sw(BC(i)) - 1))) ? (1<<i):0;
				rsp.VCO |= (!(rsp.VCE&BIT(i)) && (rsp.v[vs].sw(i) != vtabs)) ? (1<<(i+8)) : 0;
				rsp.VCC |= (rsp.v[vs].sw(i) <= -rsp.v[vt].sw(BC(i))) ? (1<<i):0;
				rsp.VCC |= (rsp.v[vs].sw(i) >= rsp.v[vt].sw(BC(i))) ? (1<<(i+8)):0;
				u16 clip = (rsp.VCO&BIT(i)) ? (rsp.VCC&BIT(i)) : (rsp.VCC&BIT(i+8));
				rsp.a[i] &= ~0xffffull;
				rsp.a[i] |= res.w(i) = (clip ? u16(vtabs) : rsp.v[vs].w(i));
			}
			rsp.v[vd] = res;
		};
	
	case 0x27: // VMRG
		return INSTR {
			VOPP;
			vreg res;
			for(u32 i = 0; i < 8; ++i)
			{
				rsp.a[i] &= ~0xffffull;
				rsp.a[i] |= res.w(i) = (rsp.VCC&BIT(i)) ? rsp.v[vs].w(i) : rsp.v[vt].w(BC(i));
			}
			rsp.v[vd] = res;
			rsp.VCO = 0; // ??
		};
		
	case 0x28: // VAND
		return INSTR {
			VOPP;
			vreg res;
			for(u32 i = 0; i < 8; ++i)
			{
				rsp.a[i] &= ~0xffffull;
				rsp.a[i] |= res.w(i) = rsp.v[vs].w(i) & rsp.v[vt].w(BC(i));
			}
			rsp.v[vd] = res;
		};
	case 0x29: // VNAND
		return INSTR {
			VOPP;
			vreg res;
			for(u32 i = 0; i < 8; ++i)
			{
				rsp.a[i] &= ~0xffffull;
				rsp.a[i] |= res.w(i) = ~(rsp.v[vs].w(i) & rsp.v[vt].w(BC(i)));
			}
			rsp.v[vd] = res;
		};
	case 0x2A: // VOR
		return INSTR {
			VOPP;
			vreg res;
			for(u32 i = 0; i < 8; ++i)
			{
				rsp.a[i] &= ~0xffffull;
				rsp.a[i] |= res.w(i) = rsp.v[vs].w(i) | rsp.v[vt].w(BC(i));
			}
			rsp.v[vd] = res;
		};
	case 0x2B: // VNOR
		return INSTR {
			VOPP;
			vreg res;
			for(u32 i = 0; i < 8; ++i)
			{
				rsp.a[i] &= ~0xffffull;
				rsp.a[i] |= res.w(i) = ~(rsp.v[vs].w(i) | rsp.v[vt].w(BC(i)));
			}
			rsp.v[vd] = res;
		};
	case 0x2C: // VXOR
		return INSTR {
			VOPP;
			vreg res;
			for(u32 i = 0; i < 8; ++i)
			{
				rsp.a[i] &= ~0xffffull;
				rsp.a[i] |= res.w(i) = rsp.v[vs].w(i) ^ rsp.v[vt].w(BC(i));
			}
			rsp.v[vd] = res;
		};
	case 0x2D: // VNXOR
		return INSTR {
			VOPP;
			vreg res;
			for(u32 i = 0; i < 8; ++i)
			{
				rsp.a[i] &= ~0xffffull;
				rsp.a[i] |= res.w(i) = ~(rsp.v[vs].w(i) ^ rsp.v[vt].w(BC(i)));
			}
			rsp.v[vd] = res;
		};
		
	case 0x32: // VRCPH  //TODO
		return INSTR {
			VOPP;		
		};
	case 0x37: return INSTR {}; // VNOP??
	default:
		printf("RSP: unimpl cop2 opcode = $%X\n", opcode&0x3F);
		//exit(1);
		return INSTR {};
	}
	return INSTR {};
}

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
			u32 addr = rsp.r[base] + offs*2;
			rsp.v[vt].b(e) = rsp.DMEM[addr&0xfff];
			if( e != 15 ) rsp.v[vt].b(e+1) = rsp.DMEM[(addr+1)&0xfff];
		};
	case 2: // LLV
		return INSTR {
			VUWC2;
			u32 addr = rsp.r[base] + offs*4;
			for(u32 i = 0; i < 4 && e < 16; ++i, ++e, ++addr) rsp.v[vt].b(e) = rsp.DMEM[addr&0xfff];
		};
	case 3: // LDV
		return INSTR {
			VUWC2;
			u32 addr = rsp.r[base] + offs*8;
			for(u32 i = 0; i < 8 && e < 16; ++i, ++e, ++addr) rsp.v[vt].b(e) = rsp.DMEM[addr&0xfff];
		};
	case 4: // LQV
		return INSTR {
			VUWC2;
			u32 addr = rsp.r[base] + offs*16;
			u32 end = addr | 15;
			do {
				rsp.v[vt].b(e++) = rsp.DMEM[addr&0xfff];
				addr += 1;
			} while( addr <= end && e <= 15 );
		};
	case 5: // LRV
		return INSTR {
			VUWC2;
			u32 addr = ((rsp.r[base] + offs*16)) & 0xfff;
			u32 len = addr & 15;
			addr &= 0xff0;
			e += (0x10 - len) & 0xf; // wtf happens to the element in lrv?
			for(u32 i = 0; i < len && e < 16; ++i, ++addr)
			{
				rsp.v[vt].b(e++) = rsp.DMEM[addr&0xfff];
			}
		};
	case 6: // LPV
		return INSTR {
			VUWC2;
			u32 addr = (rsp.r[base] + offs*8);
			u32 offset = addr&7;
			offset -= e; // wtf?
			addr &= 0xff8;
			for(u32 i = 0; i < 8; ++i, ++offset)
			{
				rsp.v[vt].w(i) = rsp.DMEM[(addr+(offset&15))&0xfff]<<8;
				// Rasky's docs say wrap at 8byte boundary, but only 16byte passes tests
			}
		};		
	case 7: // LUV
		return INSTR {
			VUWC2;
			u32 addr = (rsp.r[base] + offs*8);
			u32 offset = addr&7;
			offset -= e; // wtf?
			addr &= 0xff8;
			for(u32 i = 0; i < 8; ++i, ++offset)
			{
				rsp.v[vt].w(i) = rsp.DMEM[(addr+(offset&15))&0xfff]<<7;
				// Rasky's docs say wrap at 8byte boundary, but only 16byte passes tests
			}
		};
	
	case 10: return INSTR {}; // LWV doesn't do anything
	
	default: printf("RSP LWC2: fme unimpl opcode $%X\n", (opcode>>11)&0x1f);
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
			u32 addr = rsp.r[base] + offs*2;
			rsp.DMEM[addr&0xfff] = rsp.v[vt].b(e); 
			rsp.DMEM[(addr+1)&0xfff] = rsp.v[vt].b(e+1);
		};
	case 2: // SLV
		return INSTR {
			VUWC2;
			u32 addr = rsp.r[base] + offs*4;
			rsp.DMEM[addr&0xfff] = rsp.v[vt].b(e); 
			rsp.DMEM[(addr+1)&0xfff] = rsp.v[vt].b(e+1);
			rsp.DMEM[(addr+2)&0xfff] = rsp.v[vt].b(e+2);
			rsp.DMEM[(addr+3)&0xfff] = rsp.v[vt].b(e+3);
		};
	case 3: // SDV
		return INSTR {
			VUWC2;
			u32 addr = rsp.r[base] + offs*8;
			rsp.DMEM[addr&0xfff] = rsp.v[vt].b(e); 
			rsp.DMEM[(addr+1)&0xfff] = rsp.v[vt].b(e+1);
			rsp.DMEM[(addr+2)&0xfff] = rsp.v[vt].b(e+2);
			rsp.DMEM[(addr+3)&0xfff] = rsp.v[vt].b(e+3);
			rsp.DMEM[(addr+4)&0xfff] = rsp.v[vt].b(e+4);
			rsp.DMEM[(addr+5)&0xfff] = rsp.v[vt].b(e+5);
			rsp.DMEM[(addr+6)&0xfff] = rsp.v[vt].b(e+6);
			rsp.DMEM[(addr+7)&0xfff] = rsp.v[vt].b(e+7);
		};
	case 4: // SQV
		return INSTR {
			VUWC2;
			u32 addr = rsp.r[base] + offs*16;
			u32 end = addr | 15;
			do {
				rsp.DMEM[addr&0xfff] = rsp.v[vt].b(e++);
				addr += 1;
			} while( addr <= end );
		};
	case 5: // SRV
		return INSTR {
			VUWC2;
			u32 addr = ((rsp.r[base] + offs*16)) & 0xfff;
			u32 len = addr & 15;
			addr &= 0xff0;
			e += (0x10 - len); // wtf happens to the element in lrv?
			for(u32 i = 0; i < len; ++i, ++addr)
			{
				rsp.DMEM[addr&0xfff] = rsp.v[vt].b(e++);
			}
		};
	case 6: // SPV
		return INSTR {
			VUWC2;
			u32 addr = (rsp.r[base] + offs*8);
			for(u32 i = 0; i < 8; ++i, ++addr)
			{
				if( (e&0xf) < 8 )
				{
					rsp.DMEM[addr&0xfff] = rsp.v[vt].w(e++)>>8;
				} else {
					rsp.DMEM[addr&0xfff] = rsp.v[vt].w(e++)>>7;
				}
				// Rasky's docs say wrap addr, but only not wrapping passes tests
			}
		};
	case 7: // SUV
		return INSTR {
			VUWC2;
			u32 addr = (rsp.r[base] + offs*8);
			for(u32 i = 0; i < 8; ++i, ++addr)
			{
				if( (e&0xf) >= 8 )
				{
					rsp.DMEM[addr&0xfff] = rsp.v[vt].w(e++)>>8;
				} else {
					rsp.DMEM[addr&0xfff] = rsp.v[vt].w(e++)>>7;
				}
				// Rasky's docs say wrap addr, but only not wrapping passes tests
			}
		};
		
	
	default: printf("RSP SWC2: unimpl opcode $%X\n", (opcode>>11)&0x1f);
		 //exit(1);
		 return INSTR {};
	}
}






