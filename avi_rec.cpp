#include <print>
#include <string>
#include <string_view>
#include <vector>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>
#include "avi_rec.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t s32;
typedef int8_t s8;
typedef int16_t s16;
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef int32_t LONG;
#define PACKED __attribute__((packed))

template<typename T>
void outstruct(FILE* fp, const T& t)
{
	fwrite(&t, 1, sizeof(t), fp);
}

void o32(FILE* fp, u32 v) { fwrite(&v, 1, 4, fp); }
void o16(FILE* fp, u16 v) { fwrite(&v, 1, 2, fp); }
void o8(FILE* fp, u8 v) { fputc(v, fp); }
void ostr(FILE* fp, std::string_view sv) { fwrite(sv.data(), 1, sv.size(), fp); }

struct aviheader 
{
  DWORD  dwMicroSecPerFrame;
  DWORD  dwMaxBytesPerSec;
  DWORD  dwPaddingGranularity;
  DWORD  dwFlags;
  DWORD  dwTotalFrames;
  DWORD  dwInitialFrames;
  DWORD  dwStreams;
  DWORD  dwSuggestedBufferSize;
  DWORD  dwWidth;
  DWORD  dwHeight;
  DWORD  dwReserved[4];
} PACKED;

struct AVIStreamHeader {
  //FOURCC fccType;
  //FOURCC fccHandler;
  DWORD  dwFlags;
  WORD   wPriority;
  WORD   wLanguage;
  DWORD  dwInitialFrames;
  DWORD  dwScale;
  DWORD  dwRate;
  DWORD  dwStart;
  DWORD  dwLength;
  DWORD  dwSuggestedBufferSize;
  DWORD  dwQuality;
  DWORD  dwSampleSize;
  WORD x,y,w,h;
} PACKED;
struct BITMAPINFOHEADER {
  DWORD biSize;
  LONG  biWidth;
  LONG  biHeight;
  WORD  biPlanes;
  WORD  biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  LONG  biXPelsPerMeter;
  LONG  biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} PACKED;
struct WAVEFORMATEX {
  WORD  wFormatTag;
  WORD  nChannels;
  DWORD nSamplesPerSec;
  DWORD nAvgBytesPerSec;
  WORD  nBlockAlign;
  WORD  wBitsPerSample;
  WORD  cbSize;
} PACKED;

void AVI::close() { pop_list(); pop_list(); fclose(fp); }


void AVI::frame(void* data, uint32_t w, uint32_t h, uint32_t format)
{
	//todo: in the future, use w/h/format to possibly center smaller data in the larger video
	if( outbpp == 16 && inbpp == 16 )
	{
		u16* D = (u16*) data;
		for(u32 i = 0; i < width*height; ++i)
		{
			u8 R = (D[i]>>10)&0x1f;
			u8 G = (D[i]>>5)&0x1f;
			u8 B = D[i]&0x1f;
			D[i] = (B<<10)|(G<<5)|R;
		}
	}
	ostr(fp, "00db");
	o32(fp, width*height*(outbpp/8));
	fwrite(data, 1, width*height*(outbpp/8), fp);
}

void AVI::samples(void* data)
{
	ostr(fp, "01wb");
	o32(fp, channels*(audioHz/60)*2);
	fwrite(data, 1, channels*(audioHz/60)*2, fp);
}

bool AVI::init(std::string fname, uint32_t w, uint32_t h, uint32_t informat, uint32_t outformat, uint32_t chan, uint32_t hz)
{
	width = w;
	height = h;
	inbpp = informat;
	outbpp = outformat;
	audioHz = hz;
	channels = chan;
	
	fp = fopen(fname.c_str(), "wb");
	fclose(fp);
	fp = fopen(fname.c_str(), "rb+");
	
	ostr(fp, "RIFF");
	push_size();
	ostr(fp, "AVI ");
	ostr(fp, "LIST");
	push_size();
	ostr(fp, "hdrl");
	ostr(fp, "avih");
	aviheader avih;
	o32(fp, sizeof(avih));
	memset(&avih, 0, sizeof(avih));
	avih.dwMicroSecPerFrame = 16667;
	avih.dwMaxBytesPerSec = width*height*4*60;
	avih.dwStreams = 2;
	avih.dwSuggestedBufferSize = 0xffffFFFFu;
	avih.dwWidth = width;
	avih.dwHeight = height;
	outstruct(fp, avih);
	
	// stream header list
	ostr(fp, "LIST");
	push_size();
	ostr(fp, "strl");

	ostr(fp, "strh");
	AVIStreamHeader vidhead;
	memset(&vidhead, 0, sizeof(vidhead));
	o32(fp, sizeof(vidhead)+8);
	ostr(fp, "vids");
	ostr(fp, "DIB ");
	vidhead.dwScale = 1;
	vidhead.dwRate = 60;
	vidhead.dwSuggestedBufferSize = 0xffffFFFFu;
	vidhead.dwSampleSize = width*height*(outbpp/8);
	vidhead.x = vidhead.y = 0;
	vidhead.w = width;
	vidhead.h = height;
	//vidhead.dwLength = 1;
	outstruct(fp, vidhead);

	ostr(fp, "strf");
	BITMAPINFOHEADER bitmap;
	memset(&bitmap, 0, sizeof(bitmap));
	o32(fp, sizeof(bitmap));
	bitmap.biSize = sizeof(bitmap);
	bitmap.biWidth = width;
	bitmap.biHeight = -height;
	bitmap.biPlanes = 1;
	bitmap.biBitCount = outbpp;
	bitmap.biCompression = 0;
	bitmap.biSizeImage = width*height*(outbpp/8);
	outstruct(fp, bitmap);

	ostr(fp, "strh");
	AVIStreamHeader ahead;
	memset(&ahead, 0, sizeof(ahead));
	o32(fp, sizeof(ahead)+8);
	ostr(fp, "auds");
	o32(fp, 0); // pcm? raw? 1?
	ahead.dwScale = 1;
	ahead.dwRate = 44100;
	ahead.dwSuggestedBufferSize = 0xffffFFFFu;
	ahead.dwSampleSize = 2;
	outstruct(fp, ahead);
	
	ostr(fp, "strf");
	o32(fp, sizeof(WAVEFORMATEX));
	WAVEFORMATEX wave;
	memset(&wave, 0, sizeof(wave));
	wave.wFormatTag = 1;
	wave.nChannels = chan;
	wave.nSamplesPerSec = hz;
	wave.nAvgBytesPerSec = hz*4;
	wave.nBlockAlign = chan*2;
	wave.wBitsPerSample = 16;
	wave.cbSize = sizeof(wave);
	outstruct(fp, wave);
	pop_list(); // list of stream headers
	pop_list(); // main header list
	
	ostr(fp, "LIST");
	push_size();
	ostr(fp, "movi");
	
	return true;
}

void AVI::push_size()
{
	sizestack.push_back(ftell(fp));
	o32(fp, 0);
}

void AVI::pop_list()
{
	u32 p = sizestack.back();
	sizestack.pop_back();
	auto cur = ftell(fp);
	fseek(fp, p, SEEK_SET);
	o32(fp, cur-p-4);
	fseek(fp, 0, SEEK_END);
}

