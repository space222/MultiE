#pragma once
#include <print>
#include <string>
#include <string_view>
#include <vector>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>

class AVI
{
public:
	bool init(std::string fname, uint32_t w, uint32_t h, uint32_t informat, uint32_t outformat, uint32_t chan, uint32_t hz);
	void frame(void* data, uint32_t w = 0, uint32_t h = 0, uint32_t format = 0);
	void samples(void* data);
	void close();

protected:
	void push_size();
	void pop_list();
	
	uint32_t width, height;
	uint32_t inbpp, outbpp;
	uint32_t audioHz, channels;
	FILE* fp;
	
	std::vector<uint32_t> sizestack;
};




