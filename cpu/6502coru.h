#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <coroutine>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;

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
	
	Yieldable() {}
	Yieldable(std::coroutine_handle<promise_type> h) : handle(h) {}
	std::coroutine_handle<promise_type> handle;
		
	int operator()() 
	{ 
		handle(); 
		return handle.promise().val;
	}
};

#define PACKED __attribute__((packed))

enum c6502_type { ENABLE_65C02 = 1, ENABLE_HuC6280 = 2, DISABLE_DECIMAL = 4 };

struct coru6502
{
	u16 pc;
	u8 s, a, x, y;
	bool irq_line, nmi_line, waiting;
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
	u32 cpu_type = DISABLE_DECIMAL;
	
	//todo: HuC6280 MMU regs
	u8 read(u16 addr) 
	{
		return reader(*this, addr); 
	}
	void write(u16 addr, u8 v) 
	{ 
		writer(*this, addr, v); 
	}
	
	void setnz(u8 v)
	{
		F.b.Z = ((v==0)?1:0);
		F.b.N = v>>7;
	}
	
	u8 add(u8 A, u8 B)
	{
		if( F.b.D && !(cpu_type&DISABLE_DECIMAL) )
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


