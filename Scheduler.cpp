#include <vector>
#include "itypes.h"
#include "Scheduler.h"

void Scheduler::add_event(u64 stamp, u32 code)
{
	if( events.empty() || stamp < events.back().stamp )
	{
		events.emplace_back(stamp, code);
	} else {
		for(u32 i = 0; i < events.size(); ++i)
		{
			if( events[i].stamp < stamp )
			{
				events.insert(events.begin()+i, {stamp,code});
				return;
			}	
		}
	}
	return;
}

void Scheduler::filter_out_event(u32 code)
{
	for(u32 i = 0; i < events.size(); ++i)
	{
		if( events[i].code == code )
		{
			events[i].code = 0;
			//return
		}
	}
	return;
}

u64 Scheduler::next_stamp()
{
	//if( events.empty() ) return SCHED_NO_EVENT;
	return events.back().stamp;
}

void Scheduler::run_event()
{
	event e = events.back();
	events.pop_back();
	if( e.code ) cons->event(e.stamp, e.code);
}











