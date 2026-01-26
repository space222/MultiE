#include "psx.h"

#define CD_STAT (0x02|(cd_lid_open?0x10:0)|(cd_async_active==3?0x80:0)|(cd_async_active==6?0x20:0))  // 0x12 for open lid, 0x02 normal

u32 bcd2d(u8 b)
{
	u32 v = b>>4;
	v &= 0xf;
	v *= 10;
	v += b&0xf;
	return v;
}

void psx::cd_cmd(u8 c)
{
	printf("CD: CMD $%X\n", c);
	switch( c )
	{
	case 0x1B:
	case 0x06: // ReadN
		cd_active_cmd = 6;
		cd_cmd_state = 0;
		sched.add_event(stamp + 0xc4e1, 0x40);
		break;
		
	case 3: // Play
		cd_active_cmd = c;
		cd_cmd_state = 0;
		sched.add_event(stamp + 0xc4e1, 0x40);
		break;
		
	case 0x07: // MotorOn
		cd_active_cmd = c;
		cd_cmd_state = 0;
		sched.add_event(stamp + 0xc4e1, 0x40);	
		break;
		
	case 0x10: // GetLocL
		cd_active_cmd = c;
		cd_cmd_state = 0;
		sched.add_event(stamp + 0xc4e1, 0x40);
		break;
	case 0x11: // GetLocP
		cd_active_cmd = c;
		cd_cmd_state = 0;
		sched.add_event(stamp + 0xc4e1, 0x40);
		break;
		
	case 0x0D: // SetFilter
		cd_channel = cd_param[0];
		cd_file = cd_param[1];
		cd_active_cmd = c;
		cd_cmd_state = 0;
		sched.add_event(stamp + 0xc4e1, 0x40);
		break;
	
	case 0x08: // Stop
	case 0x09: // Pause
		cd_active_cmd = 9;
		cd_cmd_state = 0;
		sched.add_event(stamp + 0xc4e1, 0x40);
		break;
	case 0x14: // GetTD
	case 0x13: // GetTN
		cd_active_cmd = c;
		cd_cmd_state = 0;
		sched.add_event(stamp + 0xc4e1, 0x40);
		break;
	
	case 0x0B: // Mute
	case 0x0C: // DeMute
		cd_active_cmd = c;
		cd_cmd_state = 0;
		sched.add_event(stamp + 0xc4e1, 0x40);
		break;
		
	case 0x0A: // Init
		cd_active_cmd = c;
		cd_cmd_state = 0;
		cd_mode = 0x20;
		sched.add_event(stamp + 0x13cce, 0x40);
		break;
	
	case 0x01: // GetStat
		cd_active_cmd = c;
		cd_cmd_state = 0;
		sched.add_event(stamp + 0xc4e1, 0x40);
		break;
	case 0x02: // SetLoc
		cd_active_cmd = c;
		cd_cmd_state = 0;
		cd_min = cd_param[0];
		cd_s = cd_param[1];
		cd_sectors = cd_param[2];
		if( cd_min == 0 && cd_s == 1 ) { cd_s = 2; }
		seek_pending = true;
		printf("CD: Setloc ($%x, $%x, $%x)\n", cd_min, cd_s, cd_sectors);
		sched.add_event(stamp + 0xc4e1, 0x40);
		break;
		
	case 0x0E: // SetMode
		cd_active_cmd = c;
		cd_cmd_state = 0;
		sched.add_event(stamp + 0xc4e1, 0x40);
		if( !cd_param.empty() ) cd_mode = cd_param[0];
		printf("setmode = $%X\n", cd_mode);
		break;
	
	case 0x16: // SeekP
	case 0x15: // SeekL
		cd_active_cmd = 0x15;
		cd_cmd_state = 0;
		sched.add_event(stamp + 0xc4e1, 0x40);
		break;
		
	case 0x19: // test
		if( !cd_param.empty() && cd_param[0] == 0x20 )
		{
			cd_active_cmd = c;
			cd_cmd_state = 0;
			sched.add_event(stamp + 0xc4e1, 0x40);
			break;
		}
		if( cd_param.empty() )
		{
			printf("CD: test($19) cmd with empty params\n");
		} else {
			printf("CD: Unimpl test($19) cmd = $%X\n", cd_param[0]);
		}
		exit(1);
		break;
	case 0x1A: // GetID
		cd_active_cmd = c;
		cd_cmd_state = 0;
		sched.add_event(stamp + 0xc4e1, 0x40);
		break;
	default:
		printf("CD: unimpl cmd = $%X\n", c);
		exit(1);
		break;
	}
	cd_param.clear();
}

void psx::cd_write_1801(u8 val)
{
	switch( cd_index & 3 )
	{
	case 0:
		cd_cmd(val);
		break;	
	default:
		printf("CD: write 1801.%i = $%X\n", cd_index&3, val);
		break; //exit(1);
	}
}

void psx::cd_write_1803(u8 val)
{
	switch( cd_index & 3 )
	{
	case 0:
		if( !(val & BIT(7)) )
		{
			//cd_offset = 0;
		}
		break;
	case 1:
		cd_if = 0;
		if( !cd_irqq.empty() /*&& (cd_ie&7)*/ )
		{
			dispatch_cd_irq(cd_irqq.front());
			cd_irqq.erase(cd_irqq.begin());
		}
		break;
	default:
		printf("CD: write 1803.%i = $%X\n", cd_index&3, val);
		break;
	}
}

/*void psx::queue_cd_irq(u32 irq, const std::span<u8>& resp)
{
	cdresult c;
	memcpy(&c.resp[0], &resp[0], resp.size());
	c.irq = irq;
	c.rsize = resp.size();
	if( cd_if == 0 )
	{
		dispatch_cd_irq(c);
	} else {
		cd_irqq.push_back(c);
	}
}*/

void psx::dispatch_cd_irq(const cdresult& R)
{
	memcpy(&cd_resp[0], &R.resp[0], R.rsize);
	cd_rcomplete = 0;
	cd_rpos = R.rsize-1;
	I_STAT |= IRQ_CD; //if( cd_ie & 7 ) 
	cd_if = R.irq;
}

void psx::cd_cmd_event(u64 estamp)
{
	switch( cd_active_cmd )
	{
	case 0x1B:
	case 0x06: // ReadN
			sched.filter_out_event(0x41);
			for(auto iter = cd_irqq.begin(); iter != cd_irqq.end();)
			{
				if( iter->irq == 1 ) iter = cd_irqq.erase(iter);
				else iter++;
			}
		if( seek_pending )
		{
			nextloc.minutes = cd_min;
			nextloc.seconds = cd_s;
			nextloc.sectors = cd_sectors;
			nextloc.calc_lba();
			seek_pending = false;
		}
		queue_cd_irq(3, std::array<u8, 1>{ u8(CD_STAT) });
		cd_async_active = 6;
		cd_active_cmd = 0;
		sched.add_event(estamp + ((cd_mode&0x80)?0x36cd2:0x6e1cd), 0x41);
		break;
		
	case 0x03: // Play
		if( seek_pending )
		{
			nextloc.minutes = cd_min;
			nextloc.seconds = cd_s;
			nextloc.sectors = cd_sectors;
			nextloc.calc_lba();
			seek_pending = false;
		}
		queue_cd_irq(3, std::array<u8, 1>{ u8(CD_STAT) });
		cd_async_active = 3;
		cd_active_cmd = 0;
		sched.add_event(estamp + ((cd_mode&0x80)?0x36cd2:0x6e1cd), 0x41);	
		break;
		
	case 0x09: // Pause
		if( cd_cmd_state == 0 )
		{
			queue_cd_irq(3, std::array<u8, 1>{ u8(CD_STAT) });
			cd_cmd_state = 1;
			cd_async_active = 0;
			sched.filter_out_event(0x41);
			sched.add_event(estamp+(cd_async_active?((cd_mode&0x80)?0x10bd93:0x21181c):0x1df2), 0x40);
			for(auto iter = cd_irqq.begin(); iter != cd_irqq.end();)
			{
				if( iter->irq == 1 ) iter = cd_irqq.erase(iter);
				else iter++;
			}
			break;
		}
		queue_cd_irq(2, std::array<u8, 1>{ u8(CD_STAT) });
		cd_active_cmd = cd_cmd_state = 0;		
		break;
		
	case 0x0A: // Init
		if( cd_cmd_state == 0 )
		{
			queue_cd_irq(3, std::array<u8, 1>{ u8(CD_STAT) });
			cd_cmd_state = 1;
			sched.add_event(estamp+0xc000, 0x40);
			break;
		}
		queue_cd_irq(2, std::array<u8, 1>{ u8(CD_STAT) });
		cd_cmd_state = 0;
		break;
	
	case 0x15: // SeekL
		if( cd_cmd_state == 0 )
		{
			queue_cd_irq(3, std::array<u8, 1>{ u8(CD_STAT) });
			cd_cmd_state = 1;
			sched.add_event(estamp+0xc000, 0x40);
			//todo: a seek cmd executes a pause?	
			sched.filter_out_event(0x41);
			for(auto iter = cd_irqq.begin(); iter != cd_irqq.end();)
			{
				if( iter->irq == 1 ) iter = cd_irqq.erase(iter);
				else iter++;
			}
			if( seek_pending )
			{
				nextloc.minutes = cd_min;
				nextloc.seconds = cd_s;
				nextloc.sectors = cd_sectors;
				nextloc.calc_lba();
				seek_pending = false;
				printf("Seeked to (%x, %x, %x) %i\n", nextloc.minutes, nextloc.seconds, nextloc.sectors, nextloc.LBA);
			}
			break;
		}
		queue_cd_irq(2, std::array<u8, 1>{ u8(CD_STAT) });
		cd_cmd_state = 0;
		break;
		
	case 0x07: // MotorOn
		if( cd_cmd_state == 0 )
		{
			queue_cd_irq(3, std::array<u8, 1>{ u8(CD_STAT) });
			cd_cmd_state = 1;
			sched.add_event(estamp+0xc000, 0x40);
			break;
		}
		queue_cd_irq(2, std::array<u8, 1>{ u8(CD_STAT) });
		cd_cmd_state = 0;
		break;
	
	case 0x1A:
		if( cd_cmd_state == 0 )
		{
			queue_cd_irq(3, std::array<u8, 1>{ u8(CD_STAT) });
			sched.add_event(estamp+0x4a00, 0x40);
			cd_cmd_state = 1;
			break;
		}
		cd_cmd_state = 0;
		queue_cd_irq(2, std::array<u8, 8>{ 0x41, 0x45, 0x43, 0x53, 0, 0x20, 0, 2 });
		break;
	case 0x0E:
	case 0x02:
	case 0x01:
	case 0x0C:
	case 0x0D:
	case 0x0B:
		queue_cd_irq(3, std::array<u8, 1>{ u8(CD_STAT) });
		cd_active_cmd = cd_cmd_state = 0;
		break;
	case 0x14:
	case 0x13:
		queue_cd_irq(3, std::array<u8, 3>{ 1,1,u8(CD_STAT) });
		cd_active_cmd = cd_cmd_state = 0;
		break;
	case 0x19:
		queue_cd_irq(3, std::array<u8, 4>{ 0xc0, 0x19, 0x09, 0x94 });
		cd_active_cmd = cd_cmd_state = 0;
		break;
		
	case 0x10:{
		if( curloc.LBA < 150 )
		{
			queue_cd_irq(3, std::array<u8, 8>{ 0,0,0,0,0,0,0,0 });
			cd_active_cmd = cd_cmd_state = 0;		
			break;
		}
		u8* sect = &CDDATA[(curloc.LBA-150)*0x930 + 0xc];
		queue_cd_irq(3, std::array<u8, 8>{ sect[7], sect[6], sect[5], sect[4], sect[3], sect[2], sect[1], sect[0] });
		cd_active_cmd = cd_cmd_state = 0;
		}break;
			
	case 0x11:
		//queue_cd_irq(3, std::array<u8, 8>{ cd_sectors, cd_s, cd_min, cd_sectors, cd_s, cd_min, 1, 1 });
		queue_cd_irq(3, std::array<u8, 8>{ 8, 0, 0, 2, curloc.sectors, curloc.seconds, curloc.minutes });
		cd_active_cmd = cd_cmd_state = 0;
		break;
		
	
	}

}


/* psx::event is effectively part of the scheduler and is
   defined here, but used for every subsystem
*/
void psx::event(u64 estamp, u32 code)
{
	if( code == 0x40 )
	{
		cd_cmd_event(estamp);
		return;
	}
	
	if( code == 0x41 )
	{
		if( cd_async_active == 0x06 )
		{
			if( !(cd_mode & 0x40) || (CDDATA[(curloc.LBA-150)*0x930 + 18]&0x44)!=0x44 ) 
				queue_cd_irq(1, std::array<u8, 1>{ u8(CD_STAT) });
			curloc = nextloc;
			nextloc.inc();
			if( curloc.LBA >= 150 && (CDDATA[(curloc.LBA-150)*0x930 + 18]&0xe0) == 0xe0 )
			{
				sched.filter_out_event(0x41);
				for(auto iter = cd_irqq.begin(); iter != cd_irqq.end();)
				{
					if( iter->irq == 1 ) iter = cd_irqq.erase(iter);
					else iter++;
				}
			} else {
				sched.add_event(estamp + ((cd_mode&0x80)?0x36cd2:0x6e1cd), 0x41);
			}
			cd_offset = 0;
			return;
		}
		
		if( cd_async_active == 0x03 )
		{
			if( cd_mode&4 ) queue_cd_irq(1, std::array<u8, 8>{ 0,0,0,0,0,0,0,u8(CD_STAT) });
			sched.add_event(estamp + ((cd_mode&0x80)?0x36cd2:0x6e1cd), 0x41);
			curloc = nextloc;
			nextloc.inc();
			cd_offset = 0;
			return;		
		}
		return;
	}

	if( code >= 0x80 )
	{
		pad_event(estamp, code);
		return;
	}

}

