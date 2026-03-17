#pragma once
#include <cstdio>
#include <cstdlib>
#include <concepts>
#include <coroutine>
#include <utility>
#include <functional>
#include "itypes.h"
#include "Yieldable.h"

class c65816
{
public:
	union reg
	{
		struct __attribute__((packed)) 
		{
			u8 l, h;
		} b;
		u16 v;
	} __attribute__((packed));
	
	union flags
	{
		struct
		{
			unsigned int C : 1;
			unsigned int Z : 1;
			unsigned int I : 1;
			unsigned int D : 1;
			unsigned int X : 1;
			unsigned int M : 1;
			unsigned int V : 1;
			unsigned int N : 1;
		}  __attribute__((packed)) b;
		u8 v;	
	} __attribute__((packed));

	Yieldable run();
			
	void setnz(std::same_as<u8> auto v)
	{
		F.b.Z = (v==0)?1:0;
		F.b.N = (v&0x80)?1:0;
	}
	
	void setnz(std::same_as<reg> auto R)
	{
		F.b.Z = (R.v==0)?1:0;
		F.b.N = (R.v&0x8000)?1:0;
	}
	
	
	u8 add8(u8,u8);
	u16 add16(u16,u16);	
	u8 sub8(u8,u8);
	u16 sub16(u16,u16);	
	
	std::function<u8(u32 a)> bus_read;
	std::function<void(u32 a, u8 v)> bus_write;
	
	void reset()
	{
		
	}
	
	bool do_irqs = false;
	void poll_irqs()
	{
		do_irqs = nmi_line||(F.b.I==0&&irq_line);
	}
	
	bool irq_line=false,nmi_line=false; 
	// irq_line is directly the level-triggered pin. nmi_line here is actually the flip-flop that detected an edge.
	
	bool instr_complete = false;
	
	void fetch()
	{
		instr_complete = true;
		if( do_irqs )
		{
			opc = 0;
		} else {
			opc = bus_read(pb|pc);
		}
	}
	
	flags F;
	u16 pc;
	u32 pb, db;
	u8 opc;
	reg A, X, Y, S, D;
	bool E;
};



