#include <cstdio>
#include <cstdlib>
#include "n64_rsp.h"

extern u16 rcp_table[]; // at bottom
extern u16 rsq_table[];
typedef void (*rsp_instr)(n64_rsp&, u32);
#define INSTR [](n64_rsp& rsp, u32 opc) static
#define VUWC2 u32 vt = (opc>>16)&0x1f; u32 base = (opc>>21)&0x1f; u32 e = (opc>>7)&15; s32 offs = s8(opc<<1)>>1
#define VUFC2 u32 rt = (opc>>16)&0x1f; u32 vs = (opc>>11)&0x1f; u32 e = (opc>>7)&15
#define VOPP u32 e=(opc>>21)&15; u32 vt=(opc>>16)&0x1f; u32 vs=(opc>>11)&0x1f; u32 vd=(opc>>6)&0x1f
#define VSING u32 e = (opc>>21)&15; u32 vt = (opc>>16)&0x1f; u32 vd_elem = (opc>>11)&15; u32 vd = (opc>>6)&0x1f

#define VCO_BIT (rsp.VCO & BIT(i))
#define VCO_HI_BIT (rsp.VCO & BIT(i+8))
#define VCC_BIT (rsp.VCC & BIT(i))
#define VCC_HI_BIT (rsp.VCC & BIT(i+8))
#define VCE_BIT (rsp.VCE & BIT(i))

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

u32 rcp(s32 sinput)
{ // copied from dillonb/n64
    // One's complement absolute value, xor with the sign bit to invert all bits if the sign bit is set
    s32 mask = sinput >> 31;
    s32 input = sinput ^ mask;
    if (sinput > INT16_MIN) {
        input -= mask;
    }
    if (input == 0) {
        return 0x7FFFFFFF;
    } else if (sinput == INT16_MIN) {
        return 0xFFFF0000;
    }

    u32 shift = __builtin_clz(input);
    u64 dinput = (u64)input;
    u32 index = ((dinput << shift) & 0x7FC00000) >> 22;

    s32 result = rcp_table[index];
    result = (0x10000 | result) << 14;
    result = (result >> (31 - shift)) ^ mask;
    return result;
}

u32 rsq(u32 input) 
{// copied from dillonb/n64
    if (input == 0) {
        return 0x7FFFFFFF;
    } else if (input == 0xFFFF8000) {
        return 0xFFFF0000;
    } else if (input > 0xFFFF8000) {
        input--;
    }

    s32 sinput = input;
    s32 mask = sinput >> 31;
    input ^= mask;

    int shift = __builtin_clz(input) + 1;

    int index = (((input << shift) >> 24) | ((shift & 1) << 8));
    u32 rom = (((u32)rsq_table[index]) << 14);
    int r_shift = ((32 - shift) >> 1);
    u32 result = (0x40000000 | rom) >> r_shift;

    return result ^ mask;
}



rsp_instr rsp_cop2(n64_rsp&, u32 opcode)
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
	case 0x02: // VRNDP
		return INSTR {
			VOPP;
			vreg res;
			for(u32 i = 0; i < 8; ++i)
			{
				s64 prod = rsp.v[vt].sw(BC(i));
				if( vs & 1 ) prod <<= 16;
				//rsp.a[i] &= 0xffffFFFFffffull;
				//prod &= 0xffffFFFFffffull;
				if( !(rsp.a[i] & BITL(47)) ) rsp.a[i] += prod;
				res.w(i) = clamp_signed(rsp.a[i]>>16);
			}	
			rsp.v[vd] = res;
		};
	case 0x03: // VMULQ
		return INSTR {
			VOPP;
			vreg res;
			for(u32 i = 0; i < 8; ++i)
			{
				s64 prod = rsp.v[vs].sw(i);
				prod *= rsp.v[vt].sw(BC(i));
				if( prod & BIT(31) ) prod += 0x1F;
				rsp.a[i] = prod<<16;
				rsp.a[i] &= 0xffffFFFFffffull;
				res.w(i) = clamp_signed(prod>>1) & 0xfff0;
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
	case 0x0A: // VRNDN
		return INSTR {
			VOPP;
			vreg res;
			for(u32 i = 0; i < 8; ++i)
			{
				s64 prod = rsp.v[vt].sw(BC(i));
				if( vs & 1 ) prod <<= 16;
				if( (rsp.a[i] & BITL(47)) ) rsp.a[i] += prod;
				res.w(i) = clamp_signed(rsp.a[i]>>16);
			}	
			rsp.v[vd] = res;
		};
	case 0x0B: // VMACQ
		return INSTR {
			VOPP;
			vreg res;
			for(u32 i = 0; i < 8; ++i)
			{
				s32 prod = rsp.a[i]>>16;
				if( prod < -0x20 && ((prod & 0x20) == 0))
				{
				    prod += 0x20;
				} else if (prod > 0x20 && (prod & 0x20) == 0) {
				    prod -= 0x20;
				}
				rsp.a[i] &= 0xffffull;
				rsp.a[i] |= u64(u32(prod))<<16;
				res.w(i) = clamp_signed(prod>>1)&0xfff0;
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
				r += VCO_BIT ? 1 : 0;
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
				r -= VCO_BIT ? 1 : 0;
				rsp.a[i] &= ~0xffffull;
				rsp.a[i] |= r&0xffff;
				res.sw(i) = clamp_signed(r);			
			}
			rsp.VCO = 0;
			rsp.v[vd] = res;
		};
		
	case 0x13: // VABS
		return INSTR {
			VOPP;
			vreg res;
			for(u32 i = 0; i < 8; ++i)
			{
				rsp.a[i] &= ~0xffffull;
				
				s16 abs = rsp.v[vs].sw(i);
				if( abs > 0 )
				{
					rsp.a[i] |= res.w(i) = rsp.v[vt].w(BC(i));
				} else if( abs < 0 ) {
					if( rsp.v[vt].w(BC(i)) == 0x8000 )
					{
						res.w(i) = 0x7fff;
						rsp.a[i] |= 0x8000;
					} else {
						res.w(i) = -rsp.v[vt].sw(BC(i));
						rsp.a[i] |= res.w(i);
					}
				} else {
					res.w(i) = 0;		
				}			
			}
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
	
	case 0x12: // VSUT
	case 0x16: // VADDB
	case 0x17: // VSUBB
	case 0x18: // VACCB
	case 0x19: // VSUCB
	case 0x1A: // VSAD
	case 0x1B: // VSAC
	case 0x1C: // VSUM
	case 30: // V30	
	case 31: // V31
	case 46: // V46
	case 47: // V47
	case 59: // V59
	case 0x38: // VEXTT
	case 0x39: // VEXTQ
	case 0x3A: // VEXTN
	case 0x3C: // VINST
	case 0x3D: // VINSQ
	case 0x3E: // VINSN
		return INSTR {
			VOPP;
			for(u32 i = 0; i < 8; ++i)
			{
				rsp.a[i] &= ~0xffffull;
				rsp.a[i] |= (rsp.v[vs].w(i) + rsp.v[vt].w(BC(i)))&0xffff;
			}
			for(u32 i = 0; i < 8; ++i) rsp.v[vd].w(i) = 0;
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
	case 0x24: // VCL
		return INSTR {
			VOPP;
			vreg res;
			for(u32 i = 0; i < 8; ++i)
			{
				/*if( !VCO_BIT && !VCO_HI_BIT )
				{
					rsp.VCC &= ~BIT(i+8);
					rsp.VCC |= ((rsp.v[vs].w(i) >= rsp.v[vt].w(BC(i))) ? BIT(i+8) : 0);
				}
				if( VCO_BIT && !VCO_HI_BIT )
				{
					bool lte = (rsp.v[vs].w(i) <= -rsp.v[vt].w(BC(i)));
					bool eql = (rsp.v[vs].w(i) == -rsp.v[vt].w(BC(i)));
					rsp.VCC &= ~BIT(i);
					rsp.VCC |= (((rsp.VCE&BIT(i))?lte:eql)?BIT(i):0);	
				}
				bool clip = (VCO_BIT ? VCC_BIT : VCC_HI_BIT);
				u16 vtabs = (VCO_BIT ? -rsp.v[vt].sw(BC(i)) : rsp.v[vt].sw(BC(i)));
				rsp.a[i] &= ~0xffffull;
				rsp.a[i] |= res.w(i) = (clip ? vtabs : rsp.v[vs].w(i));
				*/
				u16 vs_element = rsp.v[vs].w(i);// vs->elements[i];
				u16 vte_element = rsp.v[vt].w(BC(i)); // vte.elements[i];
				rsp.a[i] &= ~0xffffull;
				
				if( VCO_BIT ) //N64RSP.vco.l.elements[i]) 
				{
					if( VCO_HI_BIT ) //N64RSP.vco.h.elements[i]) 
					{
						rsp.a[i] |= u16(VCC_BIT ? -vte_element : vs_element);
					} else {
						u16 clamped_sum = vs_element + vte_element;
						bool overflow = (vs_element + vte_element) != clamped_sum;
						rsp.VCC &= ~BIT(i);
						if( VCE_BIT ) {
							rsp.VCC |= (!clamped_sum || !overflow) ? BIT(i) : 0;
							rsp.a[i] |= u16(VCC_BIT ? -vte_element : vs_element);
						} else {
							rsp.VCC |= (!clamped_sum && !overflow) ? BIT(i) : 0;
							rsp.a[i] |= u16(VCC_BIT ? -vte_element : vs_element);
						}
					}
				} else {
					if( VCO_HI_BIT ) //N64RSP.vco.h.elements[i]) 
					{
						rsp.a[i] |= u16(VCC_HI_BIT ? vte_element : vs_element);
					} else {
						rsp.VCC &= ~BIT(i+8);
						rsp.VCC |= ((s32)vs_element - (s32)vte_element >= 0) ? BIT(i+8) : 0;
						rsp.a[i] |= u16(VCC_HI_BIT ? vte_element : vs_element);
					}
				}   
				res.w(i) = rsp.a[i] & 0xffff;
			}
			rsp.VCE = rsp.VCO = 0;		
			rsp.v[vd] = res;		
		};		
	case 0x25: // VCH  //todo: not correct
		return INSTR {
			VOPP;
			vreg res;
			rsp.VCC = rsp.VCE = rsp.VCO = 0;
			for(u32 i = 0; i < 8; ++i)
			{
				/*rsp.VCO |= (((rsp.v[vs].sw(i) ^ rsp.v[vt].sw(BC(i)))<0) ? BIT(i) : 0);
				s16 vtabs = (VCO_BIT ? -rsp.v[vt].sw(BC(i)) : rsp.v[vt].sw(BC(i)));
				rsp.VCE |= ((VCO_BIT && (rsp.v[vs].sw(i) == (-rsp.v[vt].sw(BC(i)) - 1)))?BIT(i):0);
				rsp.VCO |= ((!VCE_BIT && (rsp.v[vs].sw(i) != vtabs))?BIT(i+8):0);
				rsp.VCC |= ((rsp.v[vs].sw(i) <= -rsp.v[vt].sw(BC(i)))?BIT(i):0);
				rsp.VCC |= ((rsp.v[vs].sw(i) >= rsp.v[vt].sw(BC(i)))?BIT(i+8):0);
				bool clip = (VCO_BIT ? VCC_BIT : VCC_HI_BIT);
				rsp.a[i] &= ~0xffffull;
				rsp.a[i] |= res.w(i) = u16(clip ? vtabs : rsp.v[vs].w(i));*/
				s16 vs_element = rsp.v[vs].sw(i);// vs->signed_elements[i];
				s16 vte_element = rsp.v[vt].sw(BC(i)); //vte.signed_elements[i];

				rsp.a[i] &= ~0xffffull;

				if ((vs_element ^ vte_element) < 0) {
				    s16 result = vs_element + vte_element;

				    rsp.a[i] |= u16(result <= 0 ? -vte_element : vs_element);
				    rsp.VCC |= (result <= 0) ? BIT(i) : 0;
				    rsp.VCC |= (vte_element < 0) ? BIT(i+8) : 0;
				    rsp.VCO |= BIT(i);
				    rsp.VCO |= (result != 0 && (u16)vs_element != ((u16)vte_element ^ 0xFFFF)) ? BIT(i+8) : 0;
					rsp.VCE |= (result == -1) ? BIT(i) : 0;				    
				} else {
				    s16 result = vs_element - vte_element;
				    rsp.a[i] |= u16(result >= 0 ? vte_element : vs_element);
				    rsp.VCC |= (vte_element < 0) ? BIT(i) : 0;
				    rsp.VCC |= (result >= 0) ? BIT(i+8) : 0;
				    rsp.VCO |= (result != 0 && (u16)vs_element != ((u16)vte_element ^ 0xFFFF)) ? BIT(i+8) : 0;
				}

				res.w(i) = rsp.a[i] & 0xffff;
			}
			rsp.v[vd] = res;
		};
	case 0x26: // VCR
		return INSTR {
			VOPP;
			vreg res;
			rsp.VCC = 0;
			for(u32 i = 0; i < 8; ++i)
			{
				/*s16 s = rsp.v[vs].w(i);
				s16 t = rsp.v[vt].w(BC(i));
				u32 sign = (s ^ t) & BIT(15);
				u32 nt = t ^ (sign ? 0xffff : 0);
				bool ge = s >= t;
				rsp.VCC |= ge ? BIT(i+8) : 0;
				bool le = s <= -t;
				rsp.VCC |= le ? BIT(i) : 0;
				u16 result = (sign ? le : ge) ? nt : s;
				rsp.a[i] &= ~0xffffull;
				rsp.a[i] |= result;
				res.w(i) = result;*/
				u16 vs_element = rsp.v[vs].w(i); //vs->elements[i];
				u16 vte_element = rsp.v[vt].w(BC(i)); //vte.elements[i];

				bool sign_different = (0x8000 & (vs_element ^ vte_element)) == 0x8000;

				// If vte and vs have different signs, make this negative vte
				u16 vt_abs = sign_different ? ~vte_element : vte_element;

				// Compare using one's complement
				bool gte = (s16)vte_element <= (s16)(sign_different ? 0xFFFF : vs_element);
				bool lte = (((sign_different ? vs_element : 0) + vte_element) & 0x8000) == 0x8000;

				// If the sign is different, check LTE, otherwise, check GTE.
				bool check = sign_different ? lte : gte;
				u16 result = check ? vt_abs : vs_element;

				rsp.a[i] &= ~0xffffull;
				rsp.a[i] |= result;
				res.w(i) = result;
	
				rsp.VCC |= gte ? BIT(i+8) : 0;
				rsp.VCC |= lte ? BIT(i) : 0;
				//N64RSP.vcc.h.elements[i] = FLAGREG_BOOL(gte);
				//N64RSP.vcc.l.elements[i] = FLAGREG_BOOL(lte);
			}
			rsp.v[vd] = res;
			rsp.VCE = rsp.VCO = 0;
			/* // from discord: https://discord.com/channels/465585922579103744/600463718924681232/754456372669579336
			let zero = _mm_setzero_si128();
			let s = self.vector(i.vs()).get_m128();
			let t = self.vector(i.vt()).get_m128_e(i.e());

			let sign = _mm_srai_epi16(_mm_xor_si128(s, t), 15);
			let nt = _mm_xor_si128(t, sign);
				
			let ge = _mm_cmpge_epi16(s, t);
			let le = _mm_cmple_epi16(s, _mm_sub_epi16(zero, t));

			let result = _mm_blendv_epi8(s, nt, _mm_blendv_epi8(ge, le, sign));

			self.acc_l.set_m128(result);
			self.vector(i.vd()).set_m128(result);

			self.vlt.set_m128(_mm_srli_epi16(ge, 15));
			self.vsc.set_m128(_mm_srli_epi16(le, 15));
			self.vne.set_m128(zero);
			self.vct.set_m128(zero);
			self.vce.set_m128(zero);
			*/		
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
		
	case 0x30: // VRCP
		return INSTR {
			VSING;
			for(u32 i = 0; i < 8; ++i)
			{
				rsp.a[i] &= ~0xffffull;
				rsp.a[i] |= rsp.v[vt].w(BC(i));
			}
			u32 result = rcp(rsp.v[vt].sw(BC(7&e)));
			rsp.v[vd].w(7&vd_elem) = result;
			rsp.DIV_OUT = result>>16;
			rsp.divinloaded = false;			
		};
	case 0x31: // VRCPL
		return INSTR {
			VSING;
			for(u32 i = 0; i < 8; ++i)
			{
				rsp.a[i] &= ~0xffffull;
				rsp.a[i] |= rsp.v[vt].w(BC(i));
			}
			u32 result = rsp.divinloaded ? rcp((rsp.DIV_IN<<16)|rsp.v[vt].w(BC(7&e))) :
						       rcp(rsp.v[vt].sw(BC(7&e)));
			rsp.v[vd].w(7&vd_elem) = result;
			rsp.DIV_OUT = result>>16;
			rsp.divinloaded = false;		
		};
		
	case 0x33: // VMOV
		return INSTR {
			VSING;
			for(u32 i = 0; i < 8; ++i) 
			{
				rsp.a[i] &= ~0xffffull;
				rsp.a[i] |= rsp.v[vt].w(BC(i));
			}
			rsp.v[vd].w(7&vd_elem) = rsp.v[vt].w(BC(7&vd_elem));
		};
		
	case 0x32: // VRCPH
	case 0x36: // VRSQH
		return INSTR {
			VSING;
			for(u32 i = 0; i < 8; ++i) 
			{
				rsp.a[i] &= ~0xffffull;
				rsp.a[i] |= rsp.v[vt].w(BC(i));
			}
			rsp.DIV_IN = rsp.v[vt].w(BC(7&e));
			rsp.divinloaded = true;
			rsp.v[vd].w(7&vd_elem) = rsp.DIV_OUT;
		};
		
	case 0x34: // VRSQ
		return INSTR {
			VSING;
			for(u32 i = 0; i < 8; ++i) 
			{
				rsp.a[i] &= ~0xffffull;
				rsp.a[i] |= rsp.v[vt].w(BC(i));
			}
			u32 result = rsq(rsp.v[vt].sw(BC(7&e)));
			rsp.v[vd].w(7&vd_elem) = result;
			rsp.DIV_OUT = result>>16;
			rsp.divinloaded = false;
		};
	case 0x35: // VRSQL
		return INSTR {
			VSING;
			for(u32 i = 0; i < 8; ++i)
			{
				rsp.a[i] &= ~0xffffull;
				rsp.a[i] |= rsp.v[vt].w(BC(i));
			}
			u32 result = rsp.divinloaded ? rsq((rsp.DIV_IN<<16)|rsp.v[vt].w(BC(7&e))) :
						       rsq(rsp.v[vt].sw(BC(7&e)));
			rsp.v[vd].w(7&vd_elem) = result;
			rsp.DIV_OUT = result>>16;
			rsp.divinloaded = false;		
		};	
	case 0x3F: return INSTR {}; // VNULL??
	case 0x37: return INSTR {}; // VNOP??
	default:
		printf("RSP: unimpl cop2 opcode = $%X\n", opcode&0x3F);
		//exit(1);
		return INSTR {};
	}
	return INSTR {};
}

rsp_instr rsp_lwc2(n64_rsp&, u32 opcode)
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
	case 10: return INSTR {}; // LWV doesn't do anything
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
	
	case 0xB: return INSTR {}; // LTV unimpl
	
	default: printf("RSP LWC2: fme unimpl opcode $%X\n", (opcode>>11)&0x1f);
		 //exit(1);
		 return INSTR {};
	}
}

rsp_instr rsp_swc2(n64_rsp&, u32 opcode)
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
		
	case 0xA: // SWV
		return INSTR {
			VUWC2;
			u32 addr = (rsp.r[base] + offs*16); // 16?
			for(u32 i = 0; i < 16; ++i, ++addr)
			{
				rsp.DMEM[addr&0xfff] = rsp.v[vt].b((e++)&0xf);
			}		
		};
	case 0xB: return INSTR {}; // stv unimpl.
	
	default: printf("RSP SWC2: unimpl opcode $%X\n", (opcode>>11)&0x1f);
		 //exit(1);
		 return INSTR {};
	}
}





u16 rcp_table[] = {
0xffff, 0xff00, 0xfe01, 0xfd04, 0xfc07, 0xfb0c, 0xfa11, 0xf918, 0xf81f, 0xf727, 0xf631, 0xf53b, 0xf446, 0xf352, 0xf25f, 0xf16d,
0xf07c, 0xef8b, 0xee9c, 0xedae, 0xecc0, 0xebd3, 0xeae8, 0xe9fd, 0xe913, 0xe829, 0xe741, 0xe65a, 0xe573, 0xe48d, 0xe3a9, 0xe2c5,
0xe1e1, 0xe0ff, 0xe01e, 0xdf3d, 0xde5d, 0xdd7e, 0xdca0, 0xdbc2, 0xdae6, 0xda0a, 0xd92f, 0xd854, 0xd77b, 0xd6a2, 0xd5ca, 0xd4f3,
0xd41d, 0xd347, 0xd272, 0xd19e, 0xd0cb, 0xcff8, 0xcf26, 0xce55, 0xcd85, 0xccb5, 0xcbe6, 0xcb18, 0xca4b, 0xc97e, 0xc8b2, 0xc7e7,
0xc71c, 0xc652, 0xc589, 0xc4c0, 0xc3f8, 0xc331, 0xc26b, 0xc1a5, 0xc0e0, 0xc01c, 0xbf58, 0xbe95, 0xbdd2, 0xbd10, 0xbc4f, 0xbb8f,
0xbacf, 0xba10, 0xb951, 0xb894, 0xb7d6, 0xb71a, 0xb65e, 0xb5a2, 0xb4e8, 0xb42e, 0xb374, 0xb2bb, 0xb203, 0xb14b, 0xb094, 0xafde,
0xaf28, 0xae73, 0xadbe, 0xad0a, 0xac57, 0xaba4, 0xaaf1, 0xaa40, 0xa98e, 0xa8de, 0xa82e, 0xa77e, 0xa6d0, 0xa621, 0xa574, 0xa4c6,
0xa41a, 0xa36e, 0xa2c2, 0xa217, 0xa16d, 0xa0c3, 0xa01a, 0x9f71, 0x9ec8, 0x9e21, 0x9d79, 0x9cd3, 0x9c2d, 0x9b87, 0x9ae2, 0x9a3d,
0x9999, 0x98f6, 0x9852, 0x97b0, 0x970e, 0x966c, 0x95cb, 0x952b, 0x948b, 0x93eb, 0x934c, 0x92ad, 0x920f, 0x9172, 0x90d4, 0x9038,
0x8f9c, 0x8f00, 0x8e65, 0x8dca, 0x8d30, 0x8c96, 0x8bfc, 0x8b64, 0x8acb, 0x8a33, 0x899c, 0x8904, 0x886e, 0x87d8, 0x8742, 0x86ad,
0x8618, 0x8583, 0x84f0, 0x845c, 0x83c9, 0x8336, 0x82a4, 0x8212, 0x8181, 0x80f0, 0x8060, 0x7fd0, 0x7f40, 0x7eb1, 0x7e22, 0x7d93,
0x7d05, 0x7c78, 0x7beb, 0x7b5e, 0x7ad2, 0x7a46, 0x79ba, 0x792f, 0x78a4, 0x781a, 0x7790, 0x7706, 0x767d, 0x75f5, 0x756c, 0x74e4,
0x745d, 0x73d5, 0x734f, 0x72c8, 0x7242, 0x71bc, 0x7137, 0x70b2, 0x702e, 0x6fa9, 0x6f26, 0x6ea2, 0x6e1f, 0x6d9c, 0x6d1a, 0x6c98,
0x6c16, 0x6b95, 0x6b14, 0x6a94, 0x6a13, 0x6993, 0x6914, 0x6895, 0x6816, 0x6798, 0x6719, 0x669c, 0x661e, 0x65a1, 0x6524, 0x64a8,
0x642c, 0x63b0, 0x6335, 0x62ba, 0x623f, 0x61c5, 0x614b, 0x60d1, 0x6058, 0x5fdf, 0x5f66, 0x5eed, 0x5e75, 0x5dfd, 0x5d86, 0x5d0f,
0x5c98, 0x5c22, 0x5bab, 0x5b35, 0x5ac0, 0x5a4b, 0x59d6, 0x5961, 0x58ed, 0x5879, 0x5805, 0x5791, 0x571e, 0x56ac, 0x5639, 0x55c7,
0x5555, 0x54e3, 0x5472, 0x5401, 0x5390, 0x5320, 0x52af, 0x5240, 0x51d0, 0x5161, 0x50f2, 0x5083, 0x5015, 0x4fa6, 0x4f38, 0x4ecb,
0x4e5e, 0x4df1, 0x4d84, 0x4d17, 0x4cab, 0x4c3f, 0x4bd3, 0x4b68, 0x4afd, 0x4a92, 0x4a27, 0x49bd, 0x4953, 0x48e9, 0x4880, 0x4817,
0x47ae, 0x4745, 0x46dc, 0x4674, 0x460c, 0x45a5, 0x453d, 0x44d6, 0x446f, 0x4408, 0x43a2, 0x433c, 0x42d6, 0x4270, 0x420b, 0x41a6,
0x4141, 0x40dc, 0x4078, 0x4014, 0x3fb0, 0x3f4c, 0x3ee8, 0x3e85, 0x3e22, 0x3dc0, 0x3d5d, 0x3cfb, 0x3c99, 0x3c37, 0x3bd6, 0x3b74,
0x3b13, 0x3ab2, 0x3a52, 0x39f1, 0x3991, 0x3931, 0x38d2, 0x3872, 0x3813, 0x37b4, 0x3755, 0x36f7, 0x3698, 0x363a, 0x35dc, 0x357f,
0x3521, 0x34c4, 0x3467, 0x340a, 0x33ae, 0x3351, 0x32f5, 0x3299, 0x323e, 0x31e2, 0x3187, 0x312c, 0x30d1, 0x3076, 0x301c, 0x2fc2,
0x2f68, 0x2f0e, 0x2eb4, 0x2e5b, 0x2e02, 0x2da9, 0x2d50, 0x2cf8, 0x2c9f, 0x2c47, 0x2bef, 0x2b97, 0x2b40, 0x2ae8, 0x2a91, 0x2a3a,
0x29e4, 0x298d, 0x2937, 0x28e0, 0x288b, 0x2835, 0x27df, 0x278a, 0x2735, 0x26e0, 0x268b, 0x2636, 0x25e2, 0x258d, 0x2539, 0x24e5,
0x2492, 0x243e, 0x23eb, 0x2398, 0x2345, 0x22f2, 0x22a0, 0x224d, 0x21fb, 0x21a9, 0x2157, 0x2105, 0x20b4, 0x2063, 0x2012, 0x1fc1,
0x1f70, 0x1f1f, 0x1ecf, 0x1e7f, 0x1e2e, 0x1ddf, 0x1d8f, 0x1d3f, 0x1cf0, 0x1ca1, 0x1c52, 0x1c03, 0x1bb4, 0x1b66, 0x1b17, 0x1ac9,
0x1a7b, 0x1a2d, 0x19e0, 0x1992, 0x1945, 0x18f8, 0x18ab, 0x185e, 0x1811, 0x17c4, 0x1778, 0x172c, 0x16e0, 0x1694, 0x1648, 0x15fd,
0x15b1, 0x1566, 0x151b, 0x14d0, 0x1485, 0x143b, 0x13f0, 0x13a6, 0x135c, 0x1312, 0x12c8, 0x127f, 0x1235, 0x11ec, 0x11a3, 0x1159,
0x1111, 0x10c8, 0x107f, 0x1037, 0x0fef, 0x0fa6, 0x0f5e, 0x0f17, 0x0ecf, 0x0e87, 0x0e40, 0x0df9, 0x0db2, 0x0d6b, 0x0d24, 0x0cdd,
0x0c97, 0x0c50, 0x0c0a, 0x0bc4, 0x0b7e, 0x0b38, 0x0af2, 0x0aad, 0x0a68, 0x0a22, 0x09dd, 0x0998, 0x0953, 0x090f, 0x08ca, 0x0886,
0x0842, 0x07fd, 0x07b9, 0x0776, 0x0732, 0x06ee, 0x06ab, 0x0668, 0x0624, 0x05e1, 0x059e, 0x055c, 0x0519, 0x04d6, 0x0494, 0x0452,
0x0410, 0x03ce, 0x038c, 0x034a, 0x0309, 0x02c7, 0x0286, 0x0245, 0x0204, 0x01c3, 0x0182, 0x0141, 0x0101, 0x00c0, 0x0080, 0x0040
};

u16 rsq_table[] = {
0xffff, 0xff00, 0xfe02, 0xfd06, 0xfc0b, 0xfb12, 0xfa1a, 0xf923, 0xf82e, 0xf73b, 0xf648, 0xf557, 0xf467, 0xf379, 0xf28c, 0xf1a0,
0xf0b6, 0xefcd, 0xeee5, 0xedff, 0xed19, 0xec35, 0xeb52, 0xea71, 0xe990, 0xe8b1, 0xe7d3, 0xe6f6, 0xe61b, 0xe540, 0xe467, 0xe38e,
0xe2b7, 0xe1e1, 0xe10d, 0xe039, 0xdf66, 0xde94, 0xddc4, 0xdcf4, 0xdc26, 0xdb59, 0xda8c, 0xd9c1, 0xd8f7, 0xd82d, 0xd765, 0xd69e,
0xd5d7, 0xd512, 0xd44e, 0xd38a, 0xd2c8, 0xd206, 0xd146, 0xd086, 0xcfc7, 0xcf0a, 0xce4d, 0xcd91, 0xccd6, 0xcc1b, 0xcb62, 0xcaa9,
0xc9f2, 0xc93b, 0xc885, 0xc7d0, 0xc71c, 0xc669, 0xc5b6, 0xc504, 0xc453, 0xc3a3, 0xc2f4, 0xc245, 0xc198, 0xc0eb, 0xc03f, 0xbf93,
0xbee9, 0xbe3f, 0xbd96, 0xbced, 0xbc46, 0xbb9f, 0xbaf8, 0xba53, 0xb9ae, 0xb90a, 0xb867, 0xb7c5, 0xb723, 0xb681, 0xb5e1, 0xb541,
0xb4a2, 0xb404, 0xb366, 0xb2c9, 0xb22c, 0xb191, 0xb0f5, 0xb05b, 0xafc1, 0xaf28, 0xae8f, 0xadf7, 0xad60, 0xacc9, 0xac33, 0xab9e,
0xab09, 0xaa75, 0xa9e1, 0xa94e, 0xa8bc, 0xa82a, 0xa799, 0xa708, 0xa678, 0xa5e8, 0xa559, 0xa4cb, 0xa43d, 0xa3b0, 0xa323, 0xa297,
0xa20b, 0xa180, 0xa0f6, 0xa06c, 0x9fe2, 0x9f59, 0x9ed1, 0x9e49, 0x9dc2, 0x9d3b, 0x9cb4, 0x9c2f, 0x9ba9, 0x9b25, 0x9aa0, 0x9a1c,
0x9999, 0x9916, 0x9894, 0x9812, 0x9791, 0x9710, 0x968f, 0x960f, 0x9590, 0x9511, 0x9492, 0x9414, 0x9397, 0x931a, 0x929d, 0x9221,
0x91a5, 0x9129, 0x90af, 0x9034, 0x8fba, 0x8f40, 0x8ec7, 0x8e4f, 0x8dd6, 0x8d5e, 0x8ce7, 0x8c70, 0x8bf9, 0x8b83, 0x8b0d, 0x8a98,
0x8a23, 0x89ae, 0x893a, 0x88c6, 0x8853, 0x87e0, 0x876d, 0x86fb, 0x8689, 0x8618, 0x85a7, 0x8536, 0x84c6, 0x8456, 0x83e7, 0x8377,
0x8309, 0x829a, 0x822c, 0x81bf, 0x8151, 0x80e4, 0x8078, 0x800c, 0x7fa0, 0x7f34, 0x7ec9, 0x7e5e, 0x7df4, 0x7d8a, 0x7d20, 0x7cb6,
0x7c4d, 0x7be5, 0x7b7c, 0x7b14, 0x7aac, 0x7a45, 0x79de, 0x7977, 0x7911, 0x78ab, 0x7845, 0x77df, 0x777a, 0x7715, 0x76b1, 0x764d,
0x75e9, 0x7585, 0x7522, 0x74bf, 0x745d, 0x73fa, 0x7398, 0x7337, 0x72d5, 0x7274, 0x7213, 0x71b3, 0x7152, 0x70f2, 0x7093, 0x7033,
0x6fd4, 0x6f76, 0x6f17, 0x6eb9, 0x6e5b, 0x6dfd, 0x6da0, 0x6d43, 0x6ce6, 0x6c8a, 0x6c2d, 0x6bd1, 0x6b76, 0x6b1a, 0x6abf, 0x6a64,
0x6a09, 0x6955, 0x68a1, 0x67ef, 0x673e, 0x668d, 0x65de, 0x6530, 0x6482, 0x63d6, 0x632b, 0x6280, 0x61d7, 0x612e, 0x6087, 0x5fe0,
0x5f3a, 0x5e95, 0x5df1, 0x5d4e, 0x5cac, 0x5c0b, 0x5b6b, 0x5acb, 0x5a2c, 0x598f, 0x58f2, 0x5855, 0x57ba, 0x5720, 0x5686, 0x55ed,
0x5555, 0x54be, 0x5427, 0x5391, 0x52fc, 0x5268, 0x51d5, 0x5142, 0x50b0, 0x501f, 0x4f8e, 0x4efe, 0x4e6f, 0x4de1, 0x4d53, 0x4cc6,
0x4c3a, 0x4baf, 0x4b24, 0x4a9a, 0x4a10, 0x4987, 0x48ff, 0x4878, 0x47f1, 0x476b, 0x46e5, 0x4660, 0x45dc, 0x4558, 0x44d5, 0x4453,
0x43d1, 0x434f, 0x42cf, 0x424f, 0x41cf, 0x4151, 0x40d2, 0x4055, 0x3fd8, 0x3f5b, 0x3edf, 0x3e64, 0x3de9, 0x3d6e, 0x3cf5, 0x3c7c,
0x3c03, 0x3b8b, 0x3b13, 0x3a9c, 0x3a26, 0x39b0, 0x393a, 0x38c5, 0x3851, 0x37dd, 0x3769, 0x36f6, 0x3684, 0x3612, 0x35a0, 0x352f,
0x34bf, 0x344f, 0x33df, 0x3370, 0x3302, 0x3293, 0x3226, 0x31b9, 0x314c, 0x30df, 0x3074, 0x3008, 0x2f9d, 0x2f33, 0x2ec8, 0x2e5f,
0x2df6, 0x2d8d, 0x2d24, 0x2cbc, 0x2c55, 0x2bee, 0x2b87, 0x2b21, 0x2abb, 0x2a55, 0x29f0, 0x298b, 0x2927, 0x28c3, 0x2860, 0x27fd,
0x279a, 0x2738, 0x26d6, 0x2674, 0x2613, 0x25b2, 0x2552, 0x24f2, 0x2492, 0x2432, 0x23d3, 0x2375, 0x2317, 0x22b9, 0x225b, 0x21fe,
0x21a1, 0x2145, 0x20e8, 0x208d, 0x2031, 0x1fd6, 0x1f7b, 0x1f21, 0x1ec7, 0x1e6d, 0x1e13, 0x1dba, 0x1d61, 0x1d09, 0x1cb1, 0x1c59,
0x1c01, 0x1baa, 0x1b53, 0x1afc, 0x1aa6, 0x1a50, 0x19fa, 0x19a5, 0x1950, 0x18fb, 0x18a7, 0x1853, 0x17ff, 0x17ab, 0x1758, 0x1705,
0x16b2, 0x1660, 0x160d, 0x15bc, 0x156a, 0x1519, 0x14c8, 0x1477, 0x1426, 0x13d6, 0x1386, 0x1337, 0x12e7, 0x1298, 0x1249, 0x11fb,
0x11ac, 0x115e, 0x1111, 0x10c3, 0x1076, 0x1029, 0x0fdc, 0x0f8f, 0x0f43, 0x0ef7, 0x0eab, 0x0e60, 0x0e15, 0x0dca, 0x0d7f, 0x0d34,
0x0cea, 0x0ca0, 0x0c56, 0x0c0c, 0x0bc3, 0x0b7a, 0x0b31, 0x0ae8, 0x0aa0, 0x0a58, 0x0a10, 0x09c8, 0x0981, 0x0939, 0x08f2, 0x08ab,
0x0865, 0x081e, 0x07d8, 0x0792, 0x074d, 0x0707, 0x06c2, 0x067d, 0x0638, 0x05f3, 0x05af, 0x056a, 0x0526, 0x04e2, 0x049f, 0x045b,
0x0418, 0x03d5, 0x0392, 0x0350, 0x030d, 0x02cb, 0x0289, 0x0247, 0x0206, 0x01c4, 0x0183, 0x0142, 0x0101, 0x00c0, 0x0080, 0x0040
};


