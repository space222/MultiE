#pragma once
#include <functional>
#include "itypes.h"

#define CR(a) ((cpu.cr.v >> (31-((a)&0x1f)))&1)

union cond
{
	struct __attribute__((packed)) {
		unsigned int pad : 28;
		unsigned int SO : 1;
		unsigned int EQ : 1;
		unsigned int GT : 1;
		unsigned int LT : 1;				
	} b;
	u32 v;
} __attribute__((packed));

union mstrg
{
	struct __attribute__((packed)) {
		unsigned int LE : 1;
		unsigned int RI : 1;
		unsigned int pad0 : 2;
		unsigned int DR : 1;
		unsigned int IR : 1;
		unsigned int IP : 1;
		unsigned int pad1 : 1;
		unsigned int FE1 : 1;
		unsigned int BE : 1;
		unsigned int SE : 1;
		unsigned int FE0 : 1;
		unsigned int ME : 1;
		unsigned int FP : 1;
		unsigned int PR : 1;
		unsigned int EE : 1;
		unsigned int ILE : 1;
		unsigned int pad2 : 1;
		unsigned int POW : 1;					
	} b;
	u32 v;
} __attribute__((packed));

union intex
{
	struct __attribute__((packed)) {
		unsigned int strcnt : 7;
		unsigned int pad : 22;
		unsigned int CA : 1;
		unsigned int OV : 1;
		unsigned int SO : 1;	
	} b;
	u32 v;
} __attribute__((packed));

union hid2_t
{
	struct __attribute__((packed)) {
		unsigned int strcnt : 7;
		unsigned int pad : 22;
		unsigned int CA : 1;
		unsigned int OV : 1;
		unsigned int SO : 1;	
	} b;
	u32 v;
} __attribute__((packed));

union hid1_t
{
	struct __attribute__((packed)) {
		unsigned int strcnt : 7;
		unsigned int pad : 22;
		unsigned int CA : 1;
		unsigned int OV : 1;
		unsigned int SO : 1;	
	} b;
	u32 v;
} __attribute__((packed));

union hid0_t
{
	struct __attribute__((packed)) {
		unsigned int strcnt : 7;
		unsigned int pad : 22;
		unsigned int CA : 1;
		unsigned int OV : 1;
		unsigned int SO : 1;	
	} b;
	u32 v;
} __attribute__((packed));

class gekko
{
public:
	void reset();
	void step();
	bool irq_line;

	void crlog(u32 v)
	{
		cr.b.LT = ((s32(v)<0) ? 1 : 0);
		cr.b.GT = ((s32(v)>0) ? 1 : 0);
		cr.b.EQ = ((s32(v)==0) ? 1 : 0);
		cr.b.SO = xer.b.SO;
	}
	
	u32 fetch(u32);
	u32 read(u32, int);
	void write(u32, u32, int);
	
	std::function<u32(u32)> fetcher;
	std::function<u32(u32,int)> reader;
	std::function<void(u32,u32,int)> writer;
	
	u32 read_spr(u32);
	void write_spr(u32, u32);
	
	double f[32];
	double ps1[32];
	
	u32 r[32];
	u32 pc, lr, ctr, srr0, l2cr;
	u32 srr1;
	cond cr;
	mstrg msr;
	intex xer;
	hid0_t hid0;
	hid1_t hid1;
	hid2_t hid2;
};


