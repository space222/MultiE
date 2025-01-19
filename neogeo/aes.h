#pragma once
#include "console.h"

class AES : public console
{
public:
	void run_frame() override;
	void reset() override;
	bool loadROM(const std::string) override;
	
	u32 fb_width() override { return 320; }
	u32 fb_height() override { return 240; }
	u8* framebuffer() override { return (u8*)&fbuf[0]; }

	u32 fbuf[320*240];
};

