#ifndef common_h
#define common_h

#include <windows.h>

void DebugMsg(const char *fmt,...);

#define		MAX(x,max)				(((x)>(max)) ? (x) : (max))
#define		MIN(x,min)				(((x)<(min)) ? (x) : (min))

#define		Skip_Space(p) { while( *(char*)p==' ' || *(char*)p=='\t') { (*(char**)&p)++; } }

#define LF						'\n'

#define ITS_ERR_FRAME_NO                    -1000
#define ITS_ERR_FPS                         -1001

#define ITS_ERR_KEYFRAMES                   -1900
#define ITS_ERR_CHAPTERS_OVER               -1901
#define ITS_ERR_CHAPTER                     -1902
#define ITS_ERR_CHAPTER_OPENFILE            -1903
#define ITS_ERR_CHAPTER_WRITE               -1904
#define ITS_ERR_TIMECODES_OPENFILE          -1905
#define ITS_ERR_TIMECODES_WRITE             -1906
#define ITS_ERR_CHAPTER_MODE_TOO_LENGTH     -1907

#define ITS_ERR_ADJUST              -1910

#endif //common_h
