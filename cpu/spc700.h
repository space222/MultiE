#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <coroutine>
#include <utility>
#include "Yieldable.h"
#include "itypes.h"

/*
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
*/

#define PACKED __attribute__((packed))

struct spc700
{
	u16 pc, opc, DP;
	u8 S, A, X, Y;
	bool waiting;
	union {
		struct {
			unsigned int C : 1;
			unsigned int Z : 1;
			unsigned int I : 1;
			unsigned int H : 1;
			unsigned int b : 1;
			unsigned int P : 1;
			unsigned int V : 1;
			unsigned int N : 1;
		} PACKED b;
		u8 v;
	} PACKED F; // usually called P on the 6502, but easier for me to remember F


	std::function<u8(spc700&, u32)> reader;
	std::function<void(spc700&, u32, u8)> writer;
	
	u8 read(u16 addr) 
	{
		//pba = addr;
		return reader(*this, addr); 
	}
	void write(u16 addr, u8 v) 
	{ 
		//pba = addr;
		writer(*this, addr, v); 
	}
	
	void push(u8 v)
	{
		write(0x100 | S, v);
		S -= 1;
	}
	
	u8 pop()
	{
		S += 1;
		return read(0x100|S);
	}
	
	//void prev() { read(pba); }
	
	void setnz(u8 v)
	{
		F.b.Z = ((v==0)?1:0);
		F.b.N = v>>7;
	}
	
	u8 add(u8 a, u8 b)
	{ //todo: this cpu had an H flag instead of decimal mode
		u16 res = F.b.C;
		res += a;
		res += b;
		setnz(res);
		F.b.H = ((a&0xf)+(b&0xf)+F.b.C)>>4;
		F.b.C = res>>8;
		F.b.V = (((res^a)&(res^b)&0x80)?1:0);
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


