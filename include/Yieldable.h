#pragma once

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

