#include <vector>
#include <algorithm>
#include <print>
#include "itypes.h"
#include "Scheduler.h"

void Scheduler::add_event(u64 stamp, u32 code)
{
	auto iter = std::find_if(events.begin(), events.end(), [=](auto E)->bool { return E.stamp < stamp; });
	events.insert(iter, event{stamp,code});
}

void Scheduler::filter_out_event(u32 code)
{
	std::erase_if(events, [=](auto& E) { return E.code == code; });
}

u64 Scheduler::next_stamp()
{
	return events.back().stamp;
}

void Scheduler::run_event()
{
	event e = events.back();
	events.pop_back();
	if( e.code ) cons->event(e.stamp, e.code);
}











