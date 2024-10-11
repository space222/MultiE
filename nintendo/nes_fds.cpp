#include "nes.h"

void nes::fds_clock()
{
	if( fds_timer_ctrl&1 )
	{
		if( fds_timer == 0 ) 
		{
			fds_timer_irq = true;
		} else {
			fds_timer -= 1;
			if( fds_timer == 0 )
			{
				if( fds_timer_ctrl & BIT(1) )
				{
					fds_timer = fds_timer_reload;
				} else {
					fds_timer_ctrl &= ~1;
				}			
			}		
		}	
	}

	if( fds_state_clocks != 0 )
	{
		fds_state_clocks -= 1;
		if( fds_state_clocks != 0 ) return;
		fds_rd = floppy[floppy_pos];
		if( !(fds_ctrl&4) ) floppy[floppy_pos] = fds_wr;
		if( (fds_ctrl&0x80) && (fds_io_en&1) ) 
		{
			if( look_for_gap_end && (fds_rd&0x80) ) 
			{
				in_gap = look_for_gap_end = false;
			} else if( !in_gap ) {
				fds_disk_irq = true;
			}
			floppy_pos += 1;
		}
		fds_disk_stat |= 2;
		fds_state_clocks = 150;
		//printf("fds: new floppy byte, pos = %i\n", int(floppy_pos));
	}
}

void nes::fds_reg_write(u16 addr, u8 val)
{
	if( addr == 0x4023 )
	{
		fds_io_en = val&3;
		if( !(val&1) )
		{
			fds_timer_irq = false;
			fds_timer_ctrl &= ~1;
		}
		return;
	}
	if( addr == 0x4020 ) { fds_timer_reload &= 0xff00; fds_timer_reload |= val; return; }
	if( addr == 0x4021 ) { fds_timer_reload &= 0x00ff; fds_timer_reload |= val<<8; return; }
	
	if( addr == 0x4022 )
	{
		fds_timer_ctrl = val;
		if( val & BIT(0) )
		{
			fds_timer = fds_timer_reload;
			printf("fds: timer activated\n");
		} else {
			fds_timer_irq = false;
		}
		return;
	}
	
	if( addr >= 0x4040 && !(fds_io_en&2) ) return;
	if( addr  < 0x4040 && !(fds_io_en&1) ) return;
	if( addr == 0x4024 )
	{
		fds_disk_irq = false;
		fds_wr = val;
		return;
	}
	//printf("FDS: write $%X = $%X\n", addr, val);
	
	if( addr == 0x4025 )
	{
		fds_disk_irq = false;
		if( !(fds_ctrl&2) && (val&2) )
		{
			floppy_pos = 0;
			fds_state = 0;
			//printf("fds: floppy reset\n");
		}
		if( (fds_ctrl&2) && !(val&2) )
		{
			fds_state_clocks = 150;
			//printf("fds: floppy active\n");
		}
		if( !(val & BIT(6)) )
		{
			in_gap = true;
		} else if( in_gap && (val&BIT(6)) ) {
			look_for_gap_end = true;
		}
		fds_ctrl = val;
		return;
	}
	
	
}


u8 nes::fds_reg_read(u16 addr)
{
	if( addr == 0x4030 )
	{
		fds_disk_irq = fds_timer_irq = false;
		return fds_disk_stat;
	}
	if( addr == 0x4031 )
	{
		fds_disk_irq = false;
		fds_disk_stat &= ~2;
		//printf("disk read $%X\n", fds_rd);
		return fds_rd;
	}
	if( addr == 0x4032 )
	{
		fds_disk_irq = false;
		return (fds_ctrl&2);	
	}
	if( addr == 0x4033 ) return 0x80;
	//printf("FDS: read $%X\n", addr);

	if( addr == 0x4016 ) { printf("huh?\n"); exit(1); }
	
	return 0;
}



void nes::load_floppy(std::vector<u8>& file)
{
	// pre-anything-gap
	floppy.insert(std::begin(floppy), 28300/8, 0);
	floppy[floppy.size()-1] = 0x80;
	
	// block 1
	u32 offset = 0;
	for(u32 i = 0; i < 0x38; ++i) 
	{
		floppy.push_back(file[offset++]);
	}
	floppy.push_back(0);
	floppy.push_back(0);  //todo: crc
	// gap before block 2
	floppy.insert(std::end(floppy), 976/8, 0);
	floppy[floppy.size()-1] = 0x80;
	
	// block 2
	floppy.push_back(file[offset++]);
	//u32 num_files = file[offset];
	floppy.push_back(file[offset++]);
	floppy.push_back(0);
	floppy.push_back(0);  //todo: crc
	
	while( offset < 65500 && offset < file.size() )
	{
		// gap before block
		floppy.insert(std::end(floppy), 976/8, 0);
		floppy[floppy.size()-1] = 0x80;
	
		// block 3
		u32 file_size = file[offset + 0xd]|(file[offset+0xe]<<8);
		for(u32 i = 0; i < 0x10; ++i) floppy.push_back(file[offset++]);
		floppy.push_back(0);
		floppy.push_back(0);  //todo: crc
		
		// gap before block
		floppy.insert(std::end(floppy), 976/8, 0);
		floppy[floppy.size()-1] = 0x80;
		
		// block 4 (using <= to include leading block type byte)
		for(u32 i = 0; i <= file_size; ++i) floppy.push_back(file[offset++]);
		floppy.push_back(0);
		floppy.push_back(0);  //todo: crc
	}	
}


