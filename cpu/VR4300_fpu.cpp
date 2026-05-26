#include <print>
#include <climits>
#include <cfloat>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cfenv>
#include "VR4300.h"
#define JTYPE u64 target = ((opc<<2)&0x0fffFFFFu) + (cpu.npc&~0x0fff'ffffull)
#define RTYPE [[maybe_unused]]u32 d = (opc>>11)&0x1f; [[maybe_unused]]u32 s = (opc>>21)&0x1f; \
		[[maybe_unused]]u32 t = (opc>>16)&0x1f; [[maybe_unused]]u32 sa=(opc>>6)&0x1f
#define ITYPE [[maybe_unused]]u32 t = (opc>>16)&0x1f; [[maybe_unused]]u32 s = (opc>>21)&0x1f; [[maybe_unused]]u16 imm16 = opc
#define INSTR [](VR4300& cpu, u32 opc) static
#define OVERFLOW32(r, a, b) (((r)^(a))&((r)^(b))&BIT(31))
#define OVERFLOW64(r, a, b) (((r)^(a))&((r)^(b))&BITL(63))
#define LINK cpu.nnpc
#define DS_REL_ADDR (cpu.npc + (s32(s16(imm16))<<2))
#define LIKELY cpu.npc = cpu.nnpc; cpu.nnpc += 4;
#define FLTYPE u32 fs = (opc>>11)&0x1f; u32 rt = (opc>>16)&0x1f
#define FITYPE  if( cpu.COPUnusable(1) ) return; \
		feclearexcept(FE_ALL_EXCEPT); \
		[[maybe_unused]]u32 fs = (opc>>11)&0x1f; \
		[[maybe_unused]]u32 fd = (opc>>6)&0x1f;  \
		[[maybe_unused]] u32 ft = (opc>>16)&0x1f; \
		if( !(cpu.STATUS & BIT(26)) ) fs &= ~1; \
		cpu.FCSR &= ~0x3f000
#define instr return [](VR4300& cpu, u32 opc)
#define instr_nop return [](VR4300&, u32) {}

typedef void(*vr4300_instr) (VR4300&, u32);
vr4300_instr cop1_word(VR4300&, u32);
vr4300_instr cop1_long(VR4300&, u32);

u32 fnan = 0x7FBFFFFFu;
u64 dnan = 0x7FF7FFFFFFFFFFFFull;
float fmaxint = 2147483648.f;
using std::fpclassify;
using std::feclearexcept;
using std::fetestexcept;

bool VR4300::isQNaN_f(float a)
{
	u32 v;
	memcpy(&v, &a, 4);
	return (fpclassify(a)==FP_NAN) && (v & BIT(22));
}

bool VR4300::isQNaN_d(double a)
{
	const u64 mask = 0x7FF8000000000000ull;
	u64 v;
	memcpy(&v, &a, 8);
	return /*(fpclassify(a)==FP_NAN) &&*/ ((v & mask)==mask); //BITL(51));
}

#define COP1CHECK if( cpu.COPUnusable(1) ) return
#define SA ((opc>>6)&0x1f)
#define D ((opc>>11)&0x1F)
#define T ((opc>>16)&0x1F)
#define S ((opc>>21)&0x1F)
#define FD *(float*)&cpu.f[SA*8]
#define FS *(float*)&cpu.f[(cpu.STATUS&BIT(26))?D*8:((D&~1)*8)] 
#define FT *(float*)&cpu.f[T*8]
#define DD *(double*)&cpu.f[SA*8]
#define DS *(double*)&cpu.f[(cpu.STATUS&BIT(26))?D*8:((D&~1)*8)]
#define DT *(double*)&cpu.f[T*8]

static vr4300_instr decode_fpu_float(VR4300&,u32 opcode)
{
	switch( opcode&0x3F )
	{
	case 0: instr { COP1CHECK; FD = FS + FT; };
	case 1: instr { COP1CHECK; FD = FS - FT; };
	case 2: instr { COP1CHECK; FD = FS * FT; };
	case 3: instr { COP1CHECK; FD = FS / FT; };
	case 4: instr { COP1CHECK; FD = sqrt(FS); };
	case 5: instr { COP1CHECK; FD = fabs(FS); };
	case 6: instr { COP1CHECK; FD = FS; };
	case 7: instr { COP1CHECK; FD = -FS; };
	case 8: instr { COP1CHECK; DD = std::bit_cast<double>((s64)round(FS)); };
	case 9: instr { COP1CHECK; DD = std::bit_cast<double>((s64)trunc(FS)); };
	case 0xA: instr { COP1CHECK; DD = std::bit_cast<double>((s64)ceil(FS)); };
	case 0xB: instr { COP1CHECK; DD = std::bit_cast<double>((s64)floor(FS)); };

	case 0xC: instr { COP1CHECK; FD = std::bit_cast<float>((s32)round(FS)); };
	case 0xD: instr { COP1CHECK; FD = std::bit_cast<float>((s32)trunc(FS)); };
	case 0xE: instr { COP1CHECK; FD = std::bit_cast<float>((s32)ceil(FS)); };
	case 0xF: instr { COP1CHECK; FD = std::bit_cast<float>((s32)floor(FS)); };

	case 0x21: instr { COP1CHECK; DD = FS; }; // convert to double
	case 0x24: instr { COP1CHECK; FD = std::bit_cast<float>((s32)FS); }; // convert to word
	case 0x25: instr { COP1CHECK; DD = std::bit_cast<double>((s64)FS); }; // convert to long
	case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37: 
	case 0x38: case 0x39: case 0x3A: case 0x3B: case 0x3C: case 0x3D: case 0x3E: case 0x3F: 
	instr {
		COP1CHECK;
		bool eq = (opc&BIT(1)) && (FS==FT);
		bool lt = (opc&BIT(2)) && (FS <FT);
		cpu.fpu_cond = (eq || lt);
		cpu.FCSR &= ~BIT(23);
		if( cpu.fpu_cond ) cpu.FCSR |= BIT(23);
	};
	default: std::println("Unimpl fpu opc = ${:X}", opcode&0x3F); instr_nop;
	}
}

static vr4300_instr decode_fpu_double(VR4300&,u32 opcode)
{
	switch( opcode&0x3F )
	{
	case 0: instr { COP1CHECK; DD = DS + DT; };
	case 1: instr { COP1CHECK; DD = DS - DT; };
	case 2: instr { COP1CHECK; DD = DS * DT; };
	case 3: instr { COP1CHECK; DD = DS / DT; };
	case 4: instr { COP1CHECK; DD = sqrt(DS); };
	case 5: instr { COP1CHECK; DD = fabs(DS); };
	case 6: instr { COP1CHECK; DD = DS; };
	case 7: instr { COP1CHECK; DD = -DS; };

	case 8: instr { COP1CHECK; DD = std::bit_cast<double>((s64)round(DS)); };
	case 9: instr { COP1CHECK; DD = std::bit_cast<double>((s64)trunc(DS)); };
	case 0xA: instr { COP1CHECK; DD = std::bit_cast<double>((s64)ceil(DS)); };
	case 0xB: instr { COP1CHECK; DD = std::bit_cast<double>((s64)floor(DS)); };

	case 0xC: instr { COP1CHECK; FD = std::bit_cast<float>((s32)round(DS)); };
	case 0xD: instr { COP1CHECK; FD = std::bit_cast<float>((s32)trunc(DS)); };
	case 0xE: instr { COP1CHECK; FD = std::bit_cast<float>((s32)ceil(DS)); };
	case 0xF: instr { COP1CHECK; FD = std::bit_cast<float>((s32)floor(DS)); };
	case 0x20: instr { COP1CHECK; FD = DS; }; // convert to float
	case 0x24: instr { COP1CHECK; FD = std::bit_cast<float>((s32)DS); }; // convert to word
	case 0x25: instr { COP1CHECK; DD = std::bit_cast<double>((s64)DS); }; // convert to long
	case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37: 
	case 0x38: case 0x39: case 0x3A: case 0x3B: case 0x3C: case 0x3D: case 0x3E: case 0x3F: 
	instr {
		COP1CHECK;
		bool eq = (opc&BIT(1)) && (DS==DT);
		bool lt = (opc&BIT(2)) && (DS <DT);
		cpu.fpu_cond = (eq || lt);
		cpu.FCSR &= ~BIT(23);
		if( cpu.fpu_cond ) cpu.FCSR |= BIT(23);
	};
	default: std::println("Unimpl fpu double opc = ${:X}", opcode&0x3F); instr_nop;
	}
}

static vr4300_instr decode_fpu_word(VR4300&,u32 opcode)
{
	switch( opcode&0x3F )
	{
	case 0x20: instr { COP1CHECK; FD = std::bit_cast<s32>(FS); }; // convert to float
	case 0x21: instr { COP1CHECK; DD = std::bit_cast<s32>(FS); }; // convert to double
	}
	instr_nop;
}

static vr4300_instr decode_fpu_long(VR4300&,u32 opcode)
{
	switch( opcode&0x3F )
	{
	case 0x20: instr { COP1CHECK; FD = std::bit_cast<s64>(DS); }; // convert to float
	case 0x21: instr { COP1CHECK; DD = std::bit_cast<s64>(DS);  }; // convert to double	
	}
	instr_nop;
}

vr4300_instr cop1_d(VR4300&, u32 opcode)
{
	if( (opcode & 0x3F) >= 0x30 )
	{	// All the compares
		return INSTR {
			FITYPE;
			double a, b;
			memcpy(&a, &cpu.f[fs<<3], 8);
			memcpy(&b, &cpu.f[ft<<3], 8);
			if( (cpu.isQNaN_d(a) || cpu.isQNaN_d(b)) && cpu.signal_fpu(cpu.FPU_INVALID) ) { return; }
			bool isNAN = (fpclassify(a)==FP_NAN||fpclassify(b)==FP_NAN);
			if( isNAN && (opc & 8) && cpu.signal_fpu(cpu.FPU_INVALID) ) return;
			bool res = (opc & 1) && isNAN;
			res = res || ((opc & 2) && (a==b));
			res = res || ((opc & 4) && (a <b));
			cpu.fpu_cond = res;
			cpu.FCSR &= ~BIT(23);
			if( res ) cpu.FCSR |= BIT(23);
		};
	}

	switch( opcode & 0x3F )
	{
	case 0: // ADD.D
		return INSTR {
			FITYPE;
			double a, b, c;
			memcpy(&a, &cpu.f[fs<<3], 8);
			memcpy(&b, &cpu.f[ft<<3], 8);
			if( fpclassify(a) == FP_SUBNORMAL || fpclassify(b) == FP_SUBNORMAL ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;	
			} else if( fpclassify(a) == FP_NAN || fpclassify(b) == FP_NAN ) {
				if( (fpclassify(a) == FP_NAN && !cpu.isQNaN_d(a)) || (fpclassify(b) == FP_NAN && !cpu.isQNaN_d(b)) )
				{
					cpu.signal_fpu(cpu.FPU_UNIMPL);
					return;
				}
				if( cpu.signal_fpu(cpu.FPU_INVALID) ) return;
				memcpy(&c, &dnan, 8);
			} else if( fpclassify(a) == FP_INFINITE && fpclassify(b) == FP_INFINITE ) {
				if( cpu.signal_fpu(cpu.FPU_INVALID) ) return;
				memcpy(&c, &dnan, 8);
			} else {
				c = a + b;
				if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
				if( fetestexcept(FE_UNDERFLOW) )
				{
					if( !(cpu.FCSR & BIT(24)) ) { cpu.signal_fpu(cpu.FPU_UNIMPL); return; }
					if( cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
					//c = 0;
				}
				if( fetestexcept(FE_INEXACT) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}
			memcpy(&cpu.f[fd<<3], &c, 8);
		};	
	case 1: // SUB.D
		return INSTR {
			FITYPE;
			double a, b, c;
			memcpy(&a, &cpu.f[fs<<3], 8);
			memcpy(&b, &cpu.f[ft<<3], 8);
			if( fpclassify(a) == FP_SUBNORMAL || fpclassify(b) == FP_SUBNORMAL ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;	
			} else if( fpclassify(a) == FP_NAN || fpclassify(b) == FP_NAN ) {
				if( (fpclassify(a) == FP_NAN && !cpu.isQNaN_d(a)) || (fpclassify(b) == FP_NAN && !cpu.isQNaN_d(b)) )
				{
					cpu.signal_fpu(cpu.FPU_UNIMPL);
					return;
				}
				if( cpu.signal_fpu(cpu.FPU_INVALID) ) return;
				memcpy(&c, &dnan, 8);
			} else if( fpclassify(a) == FP_INFINITE && fpclassify(b) == FP_INFINITE ) {
				if( cpu.signal_fpu(cpu.FPU_INVALID) ) return;
				memcpy(&c, &dnan, 8);
			} else {
				c = a - b;
				if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
				if( fetestexcept(FE_UNDERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
				if( fetestexcept(FE_INEXACT) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}
			memcpy(&cpu.f[fd<<3], &c, 8);
		};
	case 2: // MUL.D
		return INSTR {
			FITYPE;
			double a, b, c;
			memcpy(&a, &cpu.f[fs<<3], 8);
			memcpy(&b, &cpu.f[ft<<3], 8);
			if( fpclassify(a) == FP_SUBNORMAL || fpclassify(b) == FP_SUBNORMAL ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;	
			} else if( fpclassify(a) == FP_NAN || fpclassify(b) == FP_NAN ) {
				if( (fpclassify(a) == FP_NAN && !cpu.isQNaN_d(a)) || (fpclassify(b) == FP_NAN && !cpu.isQNaN_d(b)) )
				{
					cpu.signal_fpu(cpu.FPU_UNIMPL);
					return;
				}
				if( cpu.signal_fpu(cpu.FPU_INVALID) ) return;
				memcpy(&c, &dnan, 8);
			} else if( fpclassify(a) == FP_INFINITE && fpclassify(b) == FP_INFINITE ) {
				c = std::numeric_limits<double>::infinity();
				if( std::signbit(a) ^ std::signbit(b) ) c = -c;
			} else {
				c = a * b;
				if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
				if( fetestexcept(FE_UNDERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
				if( fetestexcept(FE_INEXACT) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}
			memcpy(&cpu.f[fd<<3], &c, 8);
		};
	case 3: // DIV.D
		return INSTR {
			FITYPE;
			double a, b, c;
			memcpy(&a, &cpu.f[fs<<3], 8);
			memcpy(&b, &cpu.f[ft<<3], 8);
			if( fpclassify(a) == FP_SUBNORMAL || fpclassify(b) == FP_SUBNORMAL ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;	
			} else if( fpclassify(a) == FP_NAN || fpclassify(b) == FP_NAN ) {
				if( (fpclassify(a) == FP_NAN && !cpu.isQNaN_d(a)) || (fpclassify(b) == FP_NAN && !cpu.isQNaN_d(b)) )
				{
					cpu.signal_fpu(cpu.FPU_UNIMPL);
					return;
				}
				if( cpu.signal_fpu(cpu.FPU_INVALID) ) return;
				memcpy(&c, &dnan, 8);
			} else if( fpclassify(a) == FP_INFINITE && fpclassify(b) == FP_INFINITE ) {
				if( cpu.signal_fpu(cpu.FPU_INVALID) ) return;
				memcpy(&c, &dnan, 8);
			} else {
				if( b == 0 ) 
				{ 
					if( a == 0 ) 
					{
						if( cpu.signal_fpu(cpu.FPU_INVALID) ) return;
						memcpy(&c, &dnan, 8);
					} else {
						if( cpu.signal_fpu(cpu.FPU_DIVZERO) ) return;
						c = std::numeric_limits<double>::infinity();
						if( std::signbit(a) ^ std::signbit(b) ) c = -c;
					}
				} else {
					c = a / b;
					if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
					if( fetestexcept(FE_UNDERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
					if( fetestexcept(FE_INEXACT) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
				}
			}
			memcpy(&cpu.f[fd<<3], &c, 8);
		};
	case 4: // SQRT.D
		return INSTR {
			FITYPE;
			double a, c;
			memcpy(&a, &cpu.f[fs<<3], 8);
			if( fpclassify(a) == FP_SUBNORMAL ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;
			} else if( fpclassify(a) == FP_NAN ) {
				if( !cpu.isQNaN_d(a) )
				{
					cpu.signal_fpu(cpu.FPU_UNIMPL);
					return;
				}
				if( cpu.signal_fpu(cpu.FPU_INVALID) ) return;
				memcpy(&c, &dnan, 8);
			} else if( a < 0 ) {
				if( cpu.signal_fpu(cpu.FPU_INVALID) ) return;
				memcpy(&c, &dnan, 8);
			} else {
				c = sqrt(a);
				if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
				if( fetestexcept(FE_UNDERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
				if( fetestexcept(FE_INEXACT) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}	
			memcpy(&cpu.f[fd<<3], &c, 8);
		};
	case 5: // ABS.D
		return INSTR {
			FITYPE;
			double a, c;
			memcpy(&a, &cpu.f[fs<<3], 8);
			if( fpclassify(a) == FP_SUBNORMAL ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;
			} else if( fpclassify(a) == FP_NAN ) {
				if( !cpu.isQNaN_d(a) )
				{
					cpu.signal_fpu(cpu.FPU_UNIMPL);
					return;
				}
				if( cpu.signal_fpu(cpu.FPU_INVALID) ) return;
				memcpy(&c, &dnan, 8);
			} else {
				c = fabs(a);
				if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
				if( fetestexcept(FE_UNDERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
				if( fetestexcept(FE_INEXACT) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}
			memcpy(&cpu.f[fd<<3], &c, 8);
		};
	case 6: // MOV.D
		return INSTR {
			auto temp = cpu.FCSR;
			FITYPE;
			cpu.FCSR = temp;
			memcpy(&cpu.f[fd<<3], &cpu.f[fs<<3], 8);
		};
	case 7: // NEG.D
		return INSTR {
			FITYPE;
			double a, c;
			memcpy(&a, &cpu.f[fs<<3], 8);
			if( fpclassify(a) == FP_SUBNORMAL ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;
			} else if( fpclassify(a) == FP_NAN ) {
				if( !cpu.isQNaN_d(a) )
				{
					cpu.signal_fpu(cpu.FPU_UNIMPL);
					return;
				}
				if( cpu.signal_fpu(cpu.FPU_INVALID) ) return;
				memcpy(&c, &dnan, 8);
			} else {
				c = -a;
				if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
				if( fetestexcept(FE_UNDERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
				if( fetestexcept(FE_INEXACT) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}
			memcpy(&cpu.f[fd<<3], &c, 8);
		};
	case 8: // ROUND.L.D
		return INSTR {
			FITYPE;
			double a;
			s64 c;
			memcpy(&a, &cpu.f[fs<<3], 8);
			if( fpclassify(a) == FP_SUBNORMAL || fpclassify(a) == FP_NAN || fpclassify(a) == FP_INFINITE ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;
			} else {
				if( a > INT64_MAX || a < INT64_MIN ) { cpu.signal_fpu(cpu.FPU_UNIMPL); return; }
				int old = fegetround();
				fesetround(FE_TONEAREST);
				c = lrint(a);
				fesetround(old);
				//c = llround(a);
				if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
				if( fetestexcept(FE_UNDERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
				if( a != double(c) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}
			memcpy(&cpu.f[fd<<3], &c, 8);
		};
	case 9: // TRUNC.L.D
		return INSTR {
			FITYPE;
			double a;
			s64 c;
			memcpy(&a, &cpu.f[fs<<3], 8);
			if( fpclassify(a) == FP_SUBNORMAL || fpclassify(a) == FP_NAN || fpclassify(a) == FP_INFINITE ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;
			} else {
				if( a > INT64_MAX || a < INT64_MIN ) { cpu.signal_fpu(cpu.FPU_UNIMPL); return; }
				c = trunc(a);
				if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
				if( fetestexcept(FE_UNDERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
				if( a != double(c) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}
			memcpy(&cpu.f[fd<<3], &c, 8);
		};
	case 10: // CEIL.L.D
		return INSTR {
			FITYPE;
			double a;
			s64 c;
			memcpy(&a, &cpu.f[fs<<3], 8);
			if( fpclassify(a) == FP_SUBNORMAL || fpclassify(a) == FP_NAN || fpclassify(a) == FP_INFINITE ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;
			} else {
				if( a > INT64_MAX || a < INT64_MIN ) { cpu.signal_fpu(cpu.FPU_UNIMPL); return; }
				c = ceil(a);
				if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
				if( fetestexcept(FE_UNDERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
				if( a != double(c) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}
			memcpy(&cpu.f[fd<<3], &c, 8);
		};
	case 11: // FLOOR.L.D
		return INSTR {
			FITYPE;
			double a;
			s64 c;
			memcpy(&a, &cpu.f[fs<<3], 8);
			if( fpclassify(a) == FP_SUBNORMAL || fpclassify(a) == FP_NAN || fpclassify(a) == FP_INFINITE ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;
			} else {
				if( a > INT64_MAX || a < INT64_MIN ) { cpu.signal_fpu(cpu.FPU_UNIMPL); return; }
				c = floor(a);
				if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
				if( fetestexcept(FE_UNDERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
				if( a != double(c) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}
			memcpy(&cpu.f[fd<<3], &c, 8);
		};
	case 12: // ROUND.W.D
		return INSTR {
			FITYPE;
			double a;
			s32 c;
			memcpy(&a, &cpu.f[fs<<3], 8);
			if( fpclassify(a) == FP_SUBNORMAL || fpclassify(a) == FP_NAN || fpclassify(a) == FP_INFINITE ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;
			} else {
				if( a >= fmaxint || a < INT_MIN ) { cpu.signal_fpu(cpu.FPU_UNIMPL); return; }
				int old = fegetround();
				fesetround(FE_TONEAREST);
				c = lrint(a);
				fesetround(old);
				if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
				if( fetestexcept(FE_UNDERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
				if( a != double(c) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}
			memcpy(&cpu.f[fd<<3], &c, 4);
			memset(&cpu.f[(fd<<3)+4], 0, 4);
		};
	case 13: // TRUNC.W.D
		return INSTR {
			FITYPE;
			double a;
			s32 c;
			memcpy(&a, &cpu.f[fs<<3], 8);
			if( fpclassify(a) == FP_SUBNORMAL || fpclassify(a) == FP_NAN || fpclassify(a) == FP_INFINITE ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;
			} else {
				if( a >= fmaxint || a < INT_MIN ) { cpu.signal_fpu(cpu.FPU_UNIMPL); return; }
				c = trunc(a);
				if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
				if( fetestexcept(FE_UNDERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
				if( a != double(c) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}
			memcpy(&cpu.f[fd<<3], &c, 4);
			memset(&cpu.f[(fd<<3)+4], 0, 4);
		};
	case 14: // CEIL.W.D
		return INSTR {
			FITYPE;
			double a;
			s32 c;
			memcpy(&a, &cpu.f[fs<<3], 8);
			if( fpclassify(a) == FP_SUBNORMAL || fpclassify(a) == FP_NAN || fpclassify(a) == FP_INFINITE ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;
			} else {
				if( a >= fmaxint || a < INT_MIN ) { cpu.signal_fpu(cpu.FPU_UNIMPL); return; }
				c = ceil(a);
				if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
				if( fetestexcept(FE_UNDERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
				if( a != double(c) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}
			memcpy(&cpu.f[fd<<3], &c, 4);
			memset(&cpu.f[(fd<<3)+4], 0, 4);
		};
	case 15: // FLOOR.W.D
		return INSTR {
			FITYPE;
			double a;
			s32 c;
			memcpy(&a, &cpu.f[fs<<3], 8);
			if( fpclassify(a) == FP_SUBNORMAL || fpclassify(a) == FP_NAN || fpclassify(a) == FP_INFINITE ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;
			} else {
				if( a >= fmaxint || a < INT_MIN ) { cpu.signal_fpu(cpu.FPU_UNIMPL); return; }
				c = floor(a);
				if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
				if( fetestexcept(FE_UNDERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
				if( a != double(c) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}
			memcpy(&cpu.f[fd<<3], &c, 4);
			memset(&cpu.f[(fd<<3)+4], 0, 4);
		};

	case 0x20: // CVT.S.D
		return INSTR {
			FITYPE;
			double a;
			float b;
			memcpy(&a, &cpu.f[fs<<3], 8);
			if( fpclassify(a) == FP_SUBNORMAL ) 
			{ 
				cpu.signal_fpu(cpu.FPU_UNIMPL); 
				return; 
			}
			else if( fpclassify(a) == FP_NAN )
			{
				if( !cpu.isQNaN_d(a) ) { cpu.signal_fpu(cpu.FPU_UNIMPL); return; }
				if( cpu.signal_fpu(cpu.FPU_INVALID) ) return;
				memcpy(&cpu.f[fd<<3], &fnan, 4);
				return;
			} else if( a > FLT_MAX && !std::isinf(a) ) {
				if( cpu.signal_fpu(cpu.FPU_OVERFLOW|cpu.FPU_INEXACT) ) return;
				b = std::numeric_limits<float>::infinity();
			} else if( a < -FLT_MAX && !std::isinf(a) ) {
				if( cpu.signal_fpu(cpu.FPU_UNDERFLOW|cpu.FPU_INEXACT) ) return;
				b = -std::numeric_limits<float>::infinity();
			} else {
				b = a;
				if( fetestexcept(FE_INEXACT) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}
			memcpy(&cpu.f[fd<<3], &b, 4);
			memset(&cpu.f[(fd<<3)+4], 0, 4);
		};

	case 0x24: // CVT.W.D
		return INSTR {
			FITYPE;
			double a;
			memcpy(&a, &cpu.f[fs<<3], 8);
			if( fpclassify(a) == FP_SUBNORMAL || fpclassify(a) == FP_NAN || fpclassify(a) == FP_INFINITE )
			{ 
				cpu.signal_fpu(cpu.FPU_UNIMPL); 
				return; 
			}
			if( a >= fmaxint || a < INT_MIN ) { cpu.signal_fpu(cpu.FPU_UNIMPL); return; }
			s32 b = std::lrint(a);
			if( fetestexcept(FE_INVALID) && cpu.signal_fpu(cpu.FPU_INVALID) ) return;
			if( fetestexcept(FE_INEXACT) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			memcpy(&cpu.f[fd<<3], &b, 4);
			memset(&cpu.f[(fd<<3)+4], 0, 4);	
		};
	case 0x25: // CVT.L.D
		return INSTR {
			FITYPE;
			double a;
			memcpy(&a, &cpu.f[fs<<3], 8);
			if( fpclassify(a) == FP_SUBNORMAL || fpclassify(a) == FP_NAN || fpclassify(a) == FP_INFINITE )
			{ 
				cpu.signal_fpu(cpu.FPU_UNIMPL); 
				return; 
			}
			if( a > INT64_MAX || a < INT64_MIN ) { cpu.signal_fpu(cpu.FPU_UNIMPL); return; }
			s64 b = std::lrint(a);
			if( fetestexcept(FE_INVALID) && cpu.signal_fpu(cpu.FPU_INVALID) ) return;
			if( fetestexcept(FE_INEXACT) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			memcpy(&cpu.f[fd<<3], &b, 8);		
		};
	default: return [](VR4300& cpu, u32) { if( cpu.COPUnusable(1) ) return; cpu.FCSR &= ~0x3f000; cpu.signal_fpu(cpu.FPU_UNIMPL); };
	}
}

vr4300_instr cop1_s(VR4300&, u32 opcode)
{
	if( (opcode & 0x3F) >= 0x30 )
	{	// All the compares
		return INSTR {
			FITYPE;
			float a, b;
			memcpy(&a, &cpu.f[fs<<3], 4);
			memcpy(&b, &cpu.f[ft<<3], 4);
			if( (cpu.isQNaN_f(a) || cpu.isQNaN_f(b)) && cpu.signal_fpu(cpu.FPU_INVALID) ) { return; }
			bool isNAN = (fpclassify(a)==FP_NAN||fpclassify(b)==FP_NAN);
			bool res = ((opc & 1) && isNAN);
			if( isNAN && (opc & 8) && cpu.signal_fpu(cpu.FPU_INVALID) ) return;
			res = res || ((opc & 2) && (a == b));
			res = res || ((opc & 4) && (a < b));
			cpu.fpu_cond = res;
			cpu.FCSR &= ~BIT(23);
			if( res ) cpu.FCSR |= BIT(23);
		};
	}

	switch( opcode & 0x3F )
	{
	case 0: // ADD.S
		return INSTR {
			FITYPE;
			float a, b, c;
			memcpy(&a, &cpu.f[fs<<3], 4);
			memcpy(&b, &cpu.f[ft<<3], 4);
			if( fpclassify(a) == FP_SUBNORMAL || fpclassify(b) == FP_SUBNORMAL ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;	
			} else if( fpclassify(a) == FP_NAN || fpclassify(b) == FP_NAN ) {
				if( (fpclassify(a) == FP_NAN && !cpu.isQNaN_f(a)) || (fpclassify(b) == FP_NAN && !cpu.isQNaN_f(b)) )
				{
					cpu.signal_fpu(cpu.FPU_UNIMPL);
					return;
				}
				if( cpu.signal_fpu(cpu.FPU_INVALID) ) return;
				memcpy(&c, &fnan, 4);
			} else if( fpclassify(a) == FP_INFINITE && fpclassify(b) == FP_INFINITE ) {
				if( cpu.signal_fpu(cpu.FPU_INVALID) ) return;
				memcpy(&c, &fnan, 4);
			} else {
				c = a + b;
				if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
				if( fetestexcept(FE_UNDERFLOW) )
				{
					if( !(cpu.FCSR & BIT(24)) ) { cpu.signal_fpu(cpu.FPU_UNIMPL); return; }
					if( cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
					//c = 0;
				}
				if( fetestexcept(FE_INEXACT) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}		
			memcpy(&cpu.f[fd<<3], &c, 4);
			memset(&cpu.f[(fd<<3)+4], 0, 4);
		};
	case 1: // SUB.S
		return INSTR {
			FITYPE;
			float a, b, c;
			memcpy(&a, &cpu.f[fs<<3], 4);
			memcpy(&b, &cpu.f[ft<<3], 4);
			if( fpclassify(a) == FP_SUBNORMAL || fpclassify(b) == FP_SUBNORMAL ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;	
			} else if( fpclassify(a) == FP_NAN || fpclassify(b) == FP_NAN ) {
				if( (fpclassify(a) == FP_NAN && !cpu.isQNaN_f(a)) || (fpclassify(b) == FP_NAN && !cpu.isQNaN_f(b)) )
				{
					cpu.signal_fpu(cpu.FPU_UNIMPL);
					return;
				}
				if( cpu.signal_fpu(cpu.FPU_INVALID) ) return;
				memcpy(&c, &fnan, 4);
			} else if( fpclassify(a) == FP_INFINITE && fpclassify(b) == FP_INFINITE ) {
				if( cpu.signal_fpu(cpu.FPU_INVALID) ) return;
				memcpy(&c, &fnan, 4);
			} else {
				c = a - b;
				if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
				if( fetestexcept(FE_UNDERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
				if( fetestexcept(FE_INEXACT) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}
			memcpy(&cpu.f[fd<<3], &c, 4);
			memset(&cpu.f[(fd<<3)+4], 0, 4);
		};
	case 2: // MUL.S
		return INSTR {
			FITYPE;
			float a, b, c;
			memcpy(&a, &cpu.f[fs<<3], 4);
			memcpy(&b, &cpu.f[ft<<3], 4);
			if( fpclassify(a) == FP_SUBNORMAL || fpclassify(b) == FP_SUBNORMAL ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;	
			} else if( fpclassify(a) == FP_NAN || fpclassify(b) == FP_NAN ) {
				if( (fpclassify(a) == FP_NAN && !cpu.isQNaN_f(a)) || (fpclassify(b) == FP_NAN && !cpu.isQNaN_f(b)) )
				{
					cpu.signal_fpu(cpu.FPU_UNIMPL);
					return;
				}
				if( cpu.signal_fpu(cpu.FPU_INVALID) ) return;
				memcpy(&c, &fnan, 4);
			} else if( fpclassify(a) == FP_INFINITE && fpclassify(b) == FP_INFINITE ) {
				c = std::numeric_limits<float>::infinity();
				if( std::signbit(a) ^ std::signbit(b) ) c = -c;
			} else {
				c = a * b;
				if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
				if( fetestexcept(FE_UNDERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
				if( fetestexcept(FE_INEXACT) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}
			memcpy(&cpu.f[fd<<3], &c, 4);
			memset(&cpu.f[(fd<<3)+4], 0, 4);
		};
	case 3: // DIV.S
		return INSTR {
			FITYPE;
			float a, b, c;
			memcpy(&a, &cpu.f[fs<<3], 4);
			memcpy(&b, &cpu.f[ft<<3], 4);
			if( fpclassify(a) == FP_SUBNORMAL || fpclassify(b) == FP_SUBNORMAL ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;	
			} else if( fpclassify(a) == FP_NAN || fpclassify(b) == FP_NAN ) {
				if( (fpclassify(a) == FP_NAN && !cpu.isQNaN_f(a)) || (fpclassify(b) == FP_NAN && !cpu.isQNaN_f(b)) )
				{
					cpu.signal_fpu(cpu.FPU_UNIMPL);
					return;
				}
				if( cpu.signal_fpu(cpu.FPU_INVALID) ) return;
				memcpy(&c, &fnan, 4);
			} else if( fpclassify(a) == FP_INFINITE && fpclassify(b) == FP_INFINITE ) {
				if( cpu.signal_fpu(cpu.FPU_INVALID) ) return;
				memcpy(&c, &fnan, 4);
			} else {
				if( b == 0 ) 
				{ 
					if( a == 0 ) 
					{
						if( cpu.signal_fpu(cpu.FPU_INVALID) ) return;
						memcpy(&c, &fnan, 4);
					} else {
						if( cpu.signal_fpu(cpu.FPU_DIVZERO) ) return;
						c = std::numeric_limits<float>::infinity();
						if( std::signbit(a) ^ std::signbit(b) ) c = -c;
					}
				} else {
					c = a / b;
					if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
					if( fetestexcept(FE_UNDERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
					if( fetestexcept(FE_INEXACT) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
				}
			}
			memcpy(&cpu.f[fd<<3], &c, 4);
			memset(&cpu.f[(fd<<3)+4], 0, 4);
		};
	case 4: // SQRT.S
		return INSTR {
			FITYPE;
			float a, c;
			memcpy(&a, &cpu.f[fs<<3], 4);
			if( fpclassify(a) == FP_SUBNORMAL ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;	
			} else if( fpclassify(a) == FP_NAN ) {
				if( !cpu.isQNaN_f(a) )
				{
					cpu.signal_fpu(cpu.FPU_UNIMPL);
					return;
				}
				if( cpu.signal_fpu(cpu.FPU_INVALID) ) return;
				memcpy(&c, &fnan, 4);
			} else if( a < 0 ) {
				if( cpu.signal_fpu(cpu.FPU_INVALID) ) return;
				memcpy(&c, &fnan, 4);
			} else {
				c = sqrt(a);
				if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
				if( fetestexcept(FE_UNDERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
				if( fetestexcept(FE_INEXACT) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}
			memcpy(&cpu.f[fd<<3], &c, 4);
			memset(&cpu.f[(fd<<3)+4], 0, 4);
		};
	case 5: // ABS.S
		return INSTR {
			FITYPE;
			float a, c;
			memcpy(&a, &cpu.f[fs<<3], 4);
			if( fpclassify(a) == FP_SUBNORMAL ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;	
			} else if( fpclassify(a) == FP_NAN ) {
				if( !cpu.isQNaN_f(a) )
				{
					cpu.signal_fpu(cpu.FPU_UNIMPL);
					return;
				}
				if( cpu.signal_fpu(cpu.FPU_INVALID) ) return;
				memcpy(&c, &fnan, 4);
			} else {
				c = fabs(a);
				if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
				if( fetestexcept(FE_UNDERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
				if( fetestexcept(FE_INEXACT) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}
			memcpy(&cpu.f[fd<<3], &c, 4);
			memset(&cpu.f[(fd<<3)+4], 0, 4);
		};
	case 6: // MOV.S
		return INSTR {
			auto temp = cpu.FCSR;
			FITYPE;
			cpu.FCSR = temp;
			memcpy(&cpu.f[fd<<3], &cpu.f[fs<<3], 8);
		};
	case 7: // NEG.S
		return INSTR {
			FITYPE;
			float a, c;
			memcpy(&a, &cpu.f[fs<<3], 4);
			if( fpclassify(a) == FP_SUBNORMAL ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;	
			} else if( fpclassify(a) == FP_NAN ) {
				if( !cpu.isQNaN_f(a) )
				{
					cpu.signal_fpu(cpu.FPU_UNIMPL);
					return;
				}
				if( cpu.signal_fpu(cpu.FPU_INVALID) ) return;
				memcpy(&c, &fnan, 4);
			} else {
				c = -a;
				if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
				if( fetestexcept(FE_UNDERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
				if( fetestexcept(FE_INEXACT) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}
			memcpy(&cpu.f[fd<<3], &c, 4);
			memset(&cpu.f[(fd<<3)+4], 0, 4);
		};
	case 8: // ROUND.L.S
		return INSTR {
			FITYPE;
			float a;
			s64 c;
			memcpy(&a, &cpu.f[fs<<3], 4);
			if( fpclassify(a) == FP_SUBNORMAL || fpclassify(a) == FP_NAN || fpclassify(a) == FP_INFINITE ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;
			} else {
				if( a > double(INT64_MAX) || a < double(INT64_MIN) ) { cpu.signal_fpu(cpu.FPU_UNIMPL); return; }
				int old = fegetround();
				fesetround(FE_TONEAREST);
				c = lrint(a);
				fesetround(old);
				if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
				if( fetestexcept(FE_UNDERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
				if( a != float(c) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}
			memcpy(&cpu.f[fd<<3], &c, 8);
		};
	case 9: // TRUNC.L.S
		return INSTR {
			FITYPE;
			float a;
			s64 c;
			memcpy(&a, &cpu.f[fs<<3], 4);
			if( fpclassify(a) == FP_SUBNORMAL || fpclassify(a) == FP_NAN || fpclassify(a) == FP_INFINITE ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;
			} else {
				if( a > double(INT64_MAX) || a < double(INT64_MIN) ) { cpu.signal_fpu(cpu.FPU_UNIMPL); return; }
				c = trunc(a);
				if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
				if( fetestexcept(FE_UNDERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
				if( a != float(c) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}
			memcpy(&cpu.f[fd<<3], &c, 8);
		};
	case 10: // CEIL.L.S
		return INSTR {
			FITYPE;
			float a;
			s64 c;
			memcpy(&a, &cpu.f[fs<<3], 4);
			if( fpclassify(a) == FP_SUBNORMAL || fpclassify(a) == FP_NAN || fpclassify(a) == FP_INFINITE ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;
			} else {
				if( a > double(INT64_MAX) || a < double(INT64_MIN) ) { cpu.signal_fpu(cpu.FPU_UNIMPL); return; }
				c = ceil(a);
				if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
				if( fetestexcept(FE_UNDERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
				if( a != float(c) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}
			memcpy(&cpu.f[fd<<3], &c, 8);
		};
	case 11: // FLOOR.L.S
		return INSTR {
			FITYPE;
			float a;
			s64 c;
			memcpy(&a, &cpu.f[fs<<3], 4);
			if( fpclassify(a) == FP_SUBNORMAL || fpclassify(a) == FP_NAN || fpclassify(a) == FP_INFINITE ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;
			} else {
				if( a > double(INT64_MAX) || a < double(INT64_MIN) ) { cpu.signal_fpu(cpu.FPU_UNIMPL); return; }
				c = floor(a);
				if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
				if( fetestexcept(FE_UNDERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
				if( a != float(c) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}
			memcpy(&cpu.f[fd<<3], &c, 8);
		};
	case 12: // ROUND.W.S
		return INSTR {
			FITYPE;
			float a;
			s32 c;
			memcpy(&a, &cpu.f[fs<<3], 4);
			if( fpclassify(a) == FP_SUBNORMAL || fpclassify(a) == FP_NAN || fpclassify(a) == FP_INFINITE ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;
			} else {
				if( a >= fmaxint || a < INT_MIN ) { cpu.signal_fpu(cpu.FPU_UNIMPL); return; }
				int old = fegetround();
				fesetround(FE_TONEAREST);
				c = lrint(a);
				fesetround(old);
				
				//c = nearbyint(a);
				if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
				if( fetestexcept(FE_UNDERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
				if( a != float(c) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}
			memcpy(&cpu.f[fd<<3], &c, 4);
			memset(&cpu.f[(fd<<3)+4], 0, 4);
		};
	case 13: // TRUNC.W.S
		return INSTR {
			FITYPE;
			float a;
			s32 c;
			memcpy(&a, &cpu.f[fs<<3], 4);
			if( fpclassify(a) == FP_SUBNORMAL || fpclassify(a) == FP_NAN || fpclassify(a) == FP_INFINITE ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;
			} else {
				if( a >= fmaxint || a < INT_MIN ) { cpu.signal_fpu(cpu.FPU_UNIMPL); return; }
				c = trunc(a);
				if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
				if( fetestexcept(FE_UNDERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
				if( a != float(c) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}
			memcpy(&cpu.f[fd<<3], &c, 4);
			memset(&cpu.f[(fd<<3)+4], 0, 4);
		};
	case 14: // CEIL.W.S
		return INSTR {
			FITYPE;
			float a;
			s32 c;
			memcpy(&a, &cpu.f[fs<<3], 4);
			if( fpclassify(a) == FP_SUBNORMAL || fpclassify(a) == FP_NAN || fpclassify(a) == FP_INFINITE ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;
			} else {
				if( a >= fmaxint || a < INT_MIN ) { cpu.signal_fpu(cpu.FPU_UNIMPL); return; }
				c = ceil(a);
				if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
				if( fetestexcept(FE_UNDERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
				if( a != float(c) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}
			memcpy(&cpu.f[fd<<3], &c, 4);
			memset(&cpu.f[(fd<<3)+4], 0, 4);
		};
	case 15: // FLOOR.W.S
		return INSTR {
			FITYPE;
			float a;
			s32 c;
			memcpy(&a, &cpu.f[fs<<3], 4);
			if( fpclassify(a) == FP_SUBNORMAL || fpclassify(a) == FP_NAN || fpclassify(a) == FP_INFINITE ) 
			{
				cpu.signal_fpu(cpu.FPU_UNIMPL);
				return;
			} else {
				if( a >= fmaxint || a < INT_MIN ) { cpu.signal_fpu(cpu.FPU_UNIMPL); return; }
				c = floor(a);
				if( fetestexcept(FE_OVERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_OVERFLOW) ) return;
				if( fetestexcept(FE_UNDERFLOW) && cpu.signal_fpu(cpu.FPU_INEXACT|cpu.FPU_UNDERFLOW) ) return;
				if( a != float(c) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			}
			memcpy(&cpu.f[fd<<3], &c, 4);
			memset(&cpu.f[(fd<<3)+4], 0, 4);
		};
		
	case 0x21: // CVT.D.S
		return INSTR {
			FITYPE;
			float a;
			memcpy(&a, &cpu.f[fs<<3], 4);
			if( fpclassify(a) == FP_SUBNORMAL ) //|| fpclassify(a) == FP_INFINITE ) 
			{ 
				cpu.signal_fpu(cpu.FPU_UNIMPL); return; 
			} else if( fpclassify(a) == FP_NAN ) {
				if( !cpu.isQNaN_f(a) ) { cpu.signal_fpu(cpu.FPU_UNIMPL); return; }
				if( cpu.signal_fpu(cpu.FPU_INVALID) ) return;
				memcpy(&cpu.f[fd<<3], &dnan, 8);
				return;
			}
			double b = a;
			if( fetestexcept(FE_INEXACT) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			memcpy(&cpu.f[fd<<3], &b, 8);		
		};
		
	case 0x24: // CVT.W.S
		return INSTR {
			FITYPE;
			float a;
			memcpy(&a, &cpu.f[fs<<3], 4);
			if( fpclassify(a) == FP_SUBNORMAL || fpclassify(a) == FP_NAN || fpclassify(a) == FP_INFINITE )
			{ 
				cpu.signal_fpu(cpu.FPU_UNIMPL); 
				return; 
			}
			if( a >= fmaxint || a < INT_MIN ) { cpu.signal_fpu(cpu.FPU_UNIMPL); return; }
			s32 b = std::lrint(a);
			if( fetestexcept(FE_INEXACT) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			if( fetestexcept(FE_INVALID) && cpu.signal_fpu(cpu.FPU_INVALID) ) return;
			memcpy(&cpu.f[fd<<3], &b, 4);		
			memset(&cpu.f[(fd<<3)+4], 0, 4);
		};
	case 0x25: // CVT.L.S
		return INSTR {
			FITYPE;
			float a;
			memcpy(&a, &cpu.f[fs<<3], 4);
			if( fpclassify(a) == FP_SUBNORMAL || fpclassify(a) == FP_NAN || fpclassify(a) == FP_INFINITE )
			{ 
				cpu.signal_fpu(cpu.FPU_UNIMPL); 
				return; 
			}
			if( a > double(INT64_MAX) || a < double(INT64_MIN) ) { cpu.signal_fpu(cpu.FPU_UNIMPL); return; }
			s64 b = std::lrint(a);
			if( fetestexcept(FE_INEXACT) && cpu.signal_fpu(cpu.FPU_INEXACT) ) return;
			if( fetestexcept(FE_INVALID) && cpu.signal_fpu(cpu.FPU_INVALID) ) return;
			memcpy(&cpu.f[fd<<3], &b, 8);		
		};
	default: return [](VR4300& cpu, u32) { if( cpu.COPUnusable(1) ) return; cpu.FCSR &= ~0x3f000; cpu.signal_fpu(cpu.FPU_UNIMPL); };
	}
}

vr4300_instr cop1(VR4300& proc, u32 opcode)
{
	switch( (opcode>>21) & 0x1F )
	{
	case 0: // MFC1
		return INSTR {
			FLTYPE;
			if( cpu.COPUnusable(1) ) return;
			if( !(cpu.STATUS & BIT(26)) )
			{
				fs = ((fs&~1)<<3) + ((fs&1)?4:0);
			} else { 
				fs <<= 3;
			}
			memcpy(&cpu.r[rt], &cpu.f[fs], 4);
			cpu.r[rt] = s32(cpu.r[rt]);
		};
	case 1: // DMFC1
		return INSTR {
			FLTYPE;
			if( cpu.COPUnusable(1) ) return;
			if( !(cpu.STATUS & BIT(26)) ) fs &= ~1;
			memcpy(&cpu.r[rt], &cpu.f[fs<<3], 8);
		};
	case 2: // CFC1
		return INSTR {
			FLTYPE;
			if( cpu.COPUnusable(1) ) return;
			if( fs == 0 )
			{
				cpu.r[rt] = 0xa00;
			} else {
				cpu.r[rt] = s32(cpu.FCSR);
			}
		};
		
	case 4: // MTC1
		return INSTR {
			FLTYPE;
			if( cpu.COPUnusable(1) ) return;
			if( !(cpu.STATUS & BIT(26)) )
			{
				fs = ((fs&~1)<<3) + ((fs&1)?4:0);
			} else {
				fs <<= 3;
			}
			memcpy(&cpu.f[fs], &cpu.r[rt], 4);
		};
	case 5: // DMTC1
		return INSTR {
			FLTYPE;
			if( cpu.COPUnusable(1) ) return;
			if( !(cpu.STATUS & BIT(26)) ) fs &= ~1;
			memcpy(&cpu.f[fs<<3], &cpu.r[rt], 8);
		};
	case 6: // CTC1
		return INSTR {
			FLTYPE;
			if( cpu.COPUnusable(1) ) return;
			if( fs != 0 )
			{
				//u32 oldrnd = cpu.FCSR & 3;
				cpu.FCSR = cpu.r[rt] & 0x183ffffu;
				if( ((cpu.FCSR>>7) & (cpu.FCSR>>12) & 0x1F) || (cpu.FCSR & BIT(17)) )
				{
					cpu.exception(15);
				}
				if( true ) //oldrnd != (cpu.FCSR&3) )
				{
					switch( cpu.FCSR&3 )
					{
					case 0: fesetround(FE_TONEAREST); break;
					case 1: fesetround(FE_TOWARDZERO); break;
					case 2: fesetround(FE_UPWARD); break;
					case 3: fesetround(FE_DOWNWARD); break;
					}
				}
				cpu.fpu_cond = cpu.FCSR & BIT(23);
			}
		};
	case 8: // BC
		return INSTR {
			ITYPE;
			if( cpu.COPUnusable(1) ) return;
			switch( (opc>>16)&0x1f )
			{
			case 0: // BCF
				cpu.ndelay = true;
				if( !cpu.fpu_cond )
				{
					cpu.branch(DS_REL_ADDR);
				}
				break;
			case 1: // BCT
				cpu.ndelay = true;
				if( cpu.fpu_cond )
				{
					cpu.branch(DS_REL_ADDR);
				}
				break;
			case 2: // BCF Likely
				if( !cpu.fpu_cond )
				{
					cpu.branch(DS_REL_ADDR);
				} else {
					LIKELY;
				}
				break;
			case 3: // BCT Likely
				if( cpu.fpu_cond )
				{
					cpu.branch(DS_REL_ADDR);
				} else {
					LIKELY;
				}
				break;
			default: 
				printf("cop1_bc unimpl opcode = $%X\n", (opc>>16)&0x1f);
				break;
			}
		};
		
	case 0x10: return decode_fpu_float(proc, opcode); //return cop1_s(proc, opcode);
	case 0x11: return decode_fpu_double(proc, opcode);//return cop1_d(proc, opcode);
	case 0x14: return decode_fpu_word(proc, opcode);//return cop1_word(proc, opcode);
	case 0x15: return decode_fpu_long(proc, opcode);//return cop1_long(proc, opcode);
	default:
		//printf("VR4300: Unimpl or undef cop1 opcode = $%X\n", opcode);
		return [](VR4300& cpu, u32) { if( cpu.COPUnusable(1) ) return; cpu.FCSR &= ~0x3f000; cpu.signal_fpu(cpu.FPU_UNIMPL); };
	}
	return nullptr;
}



vr4300_instr cop1_word(VR4300&, u32 opcode)
{
	switch( opcode & 0x3F )
	{
	case 0x20: // CVT.S
		return INSTR {
			FITYPE;
			s32 a;
			memcpy(&a, &cpu.f[fs<<3], 4);
			float b = a;
			if( a != s32(b) && cpu.signal_fpu(cpu.FPU_INEXACT) )
			{
				return;
			}
			memcpy(&cpu.f[fd<<3], &b, 4);
			memset(&cpu.f[(fd<<3)+4], 0, 4);
		};
	case 0x21: // CVT.D
		return INSTR {
			FITYPE;
			s32 a;
			memcpy(&a, &cpu.f[fs<<3], 4);
			double b = a;
			if( a != s32(b) && cpu.signal_fpu(cpu.FPU_INEXACT) )
			{
				return;
			}
			memcpy(&cpu.f[fd<<3], &b, 8);
		};
	default: 
		fprintf(stderr, "cop1_word unimpl opcode = $%X\n", opcode & 0x3F);
		return [](VR4300& cpu, u32) { if( cpu.COPUnusable(1) ) return; cpu.FCSR &= ~0x3f000; cpu.signal_fpu(cpu.FPU_UNIMPL); };
	}
}

vr4300_instr cop1_long(VR4300&, u32 opcode)
{
	switch( opcode & 0x3F )
	{
	case 0x20: // CVT.S
		return INSTR {
			FITYPE;
			s64 a;
			memcpy(&a, &cpu.f[fs<<3], 8);
			if( a > FLT_MAX || a < -FLT_MAX ) { cpu.signal_fpu(cpu.FPU_UNIMPL); return; }
			float b = a;
			if( a != s64(b) && cpu.signal_fpu(cpu.FPU_INEXACT) )
			{
				return;
			}
			memcpy(&cpu.f[fd<<3], &b, 4);
			memset(&cpu.f[(fd<<3)+4], 0, 4);
		};
	case 0x21: // CVT.D
		return INSTR {
			FITYPE;
			s64 a;
			memcpy(&a, &cpu.f[fs<<3], 8);
			if( a > DBL_MAX || a < -DBL_MAX ) { cpu.signal_fpu(cpu.FPU_UNIMPL); return; }
			double b = a;
			if( a != s64(b) && cpu.signal_fpu(cpu.FPU_INEXACT) )
			{
				return;
			}
			memcpy(&cpu.f[fd<<3], &b, 8);
		};	
	default: 
		fprintf(stderr, "cop1_long unimpl opcode = $%X\n", opcode & 0x3F);
		return [](VR4300& cpu, u32) { if( cpu.COPUnusable(1) ) return; cpu.FCSR &= ~0x3f000; cpu.signal_fpu(cpu.FPU_UNIMPL); };
	}
}


