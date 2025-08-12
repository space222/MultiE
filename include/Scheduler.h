#pragma once
#include <vector>
#include "itypes.h"
#include "console.h"

class Scheduler
{
public:
	Scheduler(console* c) : cons(c) 
	{
		reset();
	}
	
	struct event
	{
		u64 stamp;
		//sched_handler func;
		u32 code;
	};
	
	std::vector<event> events;
	
	void reset()
	{
		events.clear();
		events.emplace_back(event{0xffffFFFFffffFFFFull, 0});
	}
	
	void add_event(u64 stamp, u32 code);
	void filter_out_event(u32 code);
	u64 next_stamp();
	void run_event();
	
protected:
	console* cons;	
};







