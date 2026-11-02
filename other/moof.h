#pragma once
#include <print>
#include <string>
#include <vector>
#include "itypes.h"

class MOOF
{
public:
	static MOOF* load(const std::string&);

	int sides;
	std::vector<u32> track_offsets[2];
	std::vector<u32> track_lengths[2];

	std::vector<u8> data;
};

