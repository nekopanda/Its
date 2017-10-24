//***************************************************************
//***   drowfont  ver 1.01                                    ***
//***          date   : 2004.09.29                            ***
//***          auther : kiraru2002                            ***
//***************************************************************

#ifndef __drawfont_h__
#define __drawfont_h__

#include <windows.h>
#include <stdio.h>
#include "avisynth.h"

#ifndef AVISYNTH_2_5
	#define AVISYNTH_2_5
#endif

//--- function prototype ---
extern void DispStringYV12_Y(PVideoFrame &vf, char *string, int x, int y);
extern void DispStringYV12(PVideoFrame &vf, char *string, int x, int y, bool interlaced);
extern void DispStringYUY2(PVideoFrame &vf, char *string, int x, int y);
extern void DispStringRGB24(PVideoFrame &vf, char *string, int x, int y);
extern void DispStringRGB32(PVideoFrame &vf, char *string, int x, int y);
extern void DispCharYV12_Y(unsigned char *ptr, unsigned char c, int pitch, int limit);
extern void DispCharYV12(PVideoFrame &vf, unsigned char c, int limit, int x, int y, bool interlaced);
extern void DispCharYUY2(unsigned char *ptr, unsigned char c, int pitch, int limit);
extern void DispCharRGB24(unsigned char *ptr, unsigned char c, int pitch, int limit);
extern void DispCharRGB32(unsigned char *ptr, unsigned char c, int pitch, int limit);
//---

#endif //__drawfont_h__
