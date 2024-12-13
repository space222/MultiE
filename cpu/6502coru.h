#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <coroutine>
#include <utility>
#include "itypes.h"

struct Yieldable
{
	struct promise_type
	{
		int val;
		std::suspend_always initial_suspend() { return {}; }
		std::suspend_always final_suspend() noexcept(true) { return {}; }
		std::suspend_always return_void() { return {}; }
		std::suspend_always yield_value(int v) { val = v; return {}; }
		
		Yieldable get_return_object() { return { std::coroutine_handle<promise_type>::from_promise(*this) }; }
		
		void unhandled_exception() {}
	};
	
	Yieldable() : handle(nullptr) {}
	Yieldable(std::coroutine_handle<promise_type> h) : handle(h) {}
	std::coroutine_handle<promise_type> handle;

	Yieldable& operator=(Yieldable&& y) { handle = std::exchange(y.handle, nullptr); return *this; }
	//todo: ^ probably will need to ask someone if this makes sense how to handle assignable coroutine wrapper objs
	
	~Yieldable()
	{
		if( handle )
		{
			 handle.destroy();
		}
	}
	
	int operator()() 
	{
		handle(); 
		return handle.promise().val;
	}
};

#define PACKED __attribute__((packed))

enum coru6502_type { CPU_2A03=0, CPU_6502, CPU_65C02, CPU_WDC65C02, CPU_HUC6280 };

struct coru6502
{
	u16 pc;
	u8 s, a, x, y;
	bool irq_line, nmi_line, waiting;
	u32 pba;
	union {
		struct {
			unsigned int C : 1;
			unsigned int Z : 1;
			unsigned int I : 1;
			unsigned int D : 1;
			unsigned int b : 1;
			unsigned int t : 1;
			unsigned int V : 1;
			unsigned int N : 1;
		} PACKED b;
		u8 v;
	} PACKED F; // usually called P on the 6502, but easier for me to remember F


	std::function<u8(coru6502&, u32)> reader;
	std::function<void(coru6502&, u32, u8)> writer;
	u32 cpu_type = CPU_6502;
	
	//todo: HuC6280 MMU regs
	u8 read(u16 addr) 
	{
		pba = addr;
		return reader(*this, addr); 
	}
	void write(u16 addr, u8 v) 
	{ 
		pba = addr;
		writer(*this, addr, v); 
	}
	
	void prev() { read(pba); }
	
	void setnz(u8 v)
	{
		F.b.Z = ((v==0)?1:0);
		F.b.N = v>>7;
	}
	
	u8 add(u8 A, u8 B)
	{
		if( F.b.D && cpu_type!=CPU_2A03 )
		{
			printf("6502coru:todo: decimal mode.\n");
			exit(1);
			return 0;
		}
		u16 res = F.b.C;
		res += A;
		res += B;
		setnz(res);
		F.b.C = res>>8;
		F.b.V = (((res^A)&(res^B)&0x80)?1:0);
		return res;
	}
	
	void cmp(u8 A, u8 B)
	{
		u16 res = 1;
		res += A;
		res += B^0xff;
		F.b.C = ((res>>8)?1:0);
		setnz(res);
	}
	
	Yieldable run(); // call this to grab a coroutine to call to run a cycle
};


