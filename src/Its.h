#ifndef its_h
#define its_h

#include <windows.h>
#include <stdio.h>
#include <memory>
#include "avisynth.h"
#include "common.h"
#include "tpr.h"
#include "drawfont.h"
#include "vfrmap.h"
#include "ItsCut.h"

#define READ_BUF_SIZE				512

#define MAX_FILTERS					64
#define MAX_FILTERNAME_LENGTH		(255+1)
#define MAX_ALIAS					64
#define MAX_ALIAS_LENGTH			(63+1)
#define MAX_SELECTEVERY_PATTERNS	32
#define MAX_CHAPTERS				100
#define MAX_CHAPTER_LENGTH			(63+1)

typedef struct {
	char		fps;
	bool		debug;
	int			posx;
	int			posy;
	const char	*chapterfile;
	const char	*outputfile;
	int			opt;
} PARAMS;

typedef struct {
	int		priority;
	enum { nums=8 };
	enum { FPS24=0, FPS30=1, FPS60=2, FPS48=3, FPS20=4, FPS10=5, FPS12=6, FPS15=7 };
	enum { FPS24_COEFF=10, FPS30_COEFF=8, FPS60_COEFF=4, FPS48_COEFF=5,
		   FPS20_COEFF=12, FPS10_COEFF=24, FPS12_COEFF=20, FPS15_COEFF=16, FPS_DIV_COEFF=2 };
	static char *str(int index) {
		switch(index) {
			case FPS24: return "24";
			case FPS30: return "30";
			case FPS60: return "60";
			case FPS48: return "48";
			case FPS20: return "20";
			case FPS10: return "10";
			case FPS12: return "12";
			case FPS15: return "15";
			default: return "30";
		}
	}
	static char *item(int index) {
		switch(index) {
			case FPS24: return "[24]";
			case FPS30: return "[30]";
			case FPS60: return "[60]";
			case FPS48: return "[48]";
			case FPS20: return "[20]";
			case FPS10: return "[10]";
			case FPS12: return "[12]";
			case FPS15: return "[15]";
			default: return "[30]";
		}
	}
	int  fps2index(int fps) {
		switch(fps) {
			case 24: return FPS24;
			case 30: return FPS30;
			case 60: return FPS60;
			case 48: return FPS48;
			case 20: return FPS20;
			case 10: return FPS10;
			case 12: return FPS12;
			case 15: return FPS15;
			default: return FPS30;
		}
	}
	int  index2fps(int fps) {
		switch(fps) {
			case FPS24: return 24;
			case FPS30: return 30;
			case FPS60: return 60;
			case FPS48: return 48;
			case FPS20: return 20;
			case FPS10: return 10;
			case FPS12: return 12;
			case FPS15: return 15;
			default: return -1;
		}
	}
	int  coeff2fps(int fps) {
		switch(fps) {
			case FPS24_COEFF: return 24;
			case FPS30_COEFF: return 30;
			case FPS60_COEFF: return 60;
			case FPS48_COEFF: return 48;
			case FPS20_COEFF: return 20;
			case FPS10_COEFF: return 10;
			case FPS12_COEFF: return 12;
			case FPS15_COEFF: return 15;
			default: return -1;
		}
	}
	bool checkfps(int fps) {
		switch(fps) {
			case 24:
			case 30:
			case 60:
			case 48:
			case 20:
			case 10:
			case 12:
			case 15:
			case 0:
			case -1: return true;
			default: return false;
		}
	}
	int  fps2coeff(int fps) {
		switch(fps) {
			case 24: return FPS24_COEFF;
			case 30: return FPS30_COEFF;
			case 60: return FPS60_COEFF;
			case 48: return FPS48_COEFF;
			case 20: return FPS20_COEFF;
			case 10: return FPS10_COEFF;
			case 12: return FPS12_COEFF;
			case 15: return FPS15_COEFF;
			case -1: return FPS_DIV_COEFF;
			case 0:  return 0;
			default:
				if((fps & 0xf0)==0x80) {
					return fps & 0x0f;
				}
				return FPS30_COEFF;
		 }
	 }
	int  search2index(char*&p, char*(*F)(int)) {
		int index=-1;
		Skip_Space(p);
		for(int i=0; i<nums; i++) {
			if(_memicmp(p, F(i), strlen(F(i)))==0) {
				index = i;
				p += strlen(F(i));
				Skip_Space(p);
				break;
			}
		}
		return index;
	}
	int search2fps(char*&p, char*(*F)(int)) {
		int index=-1;
		Skip_Space(p);
		int i;
		for(i=0; i<nums; i++) {
			if(_memicmp(p, F(i), strlen(F(i)))==0) {
				index = i;
				p += strlen(F(i));
				Skip_Space(p);
				break;
			}
		}
		return index2fps(i);
	}
} FPS;

typedef struct {
	char	*path;
//	char	path_buffer[_MAX_PATH];
	char	drive[_MAX_DRIVE];
	char	dir[_MAX_DIR];
	char	fname[_MAX_FNAME];
	char	ext[_MAX_EXT];
} PATH;

typedef struct {
	int		start;
	int		frame;
	BYTE	fps;
	BYTE	attrib; // 0:none  1:set 2:copy 4:keyframe 8:chapter 0x80:delete
	BYTE	deint_type;
	BYTE	filter_id;
	BYTE	alias_id;
	BYTE	clip_offs;
	BYTE	adjust;
	BYTE	post;
} MAP;
#define ATTRIB_SET       1
#define ATTRIB_COPY      2
#define ATTRIB_KEYFRAME  4
#define ATTRIB_CHAPTER   8
#define ATTRIB_DELETE   -128

typedef struct {
	ULONGLONG time;
	int		refer;
	int		delta;
	BYTE	attrib;
	BYTE	fps;	//=count  ,this is not equal Map.fps
} OUTPUT;

typedef struct {
	int		num_filters;
	PClip	clip[MAX_FILTERS][5];
	PClip	post[MAX_FILTERS];
	int		start[MAX_FILTERS];
	char	name[MAX_FILTERS][MAX_FILTERNAME_LENGTH];
	bool	vfr[MAX_FILTERS];
	BYTE	request[MAX_FILTERS];
} FILTERS;

typedef struct {
	int		num_alias;
	char	alias[MAX_ALIAS][MAX_ALIAS_LENGTH];
	char	string[FPS::nums][MAX_ALIAS][MAX_FILTERNAME_LENGTH]; // 24,30
	char	default_string[FPS::nums][MAX_FILTERNAME_LENGTH];
	char	default_command_type[FPS::nums];
} ALIAS;

typedef struct {
	int		frame;
	char	text[MAX_CHAPTER_LENGTH];
} CHAPTER;

typedef struct {
	int			num_chapters;
	char		language[16];
	char		charset[16];
	char		utf8_bom;
	ULONGLONG	endtime;
	CHAPTER		c[MAX_CHAPTERS];
} CHAPTERS;


class Its : public GenericVideoFilter {
private:
	Its *self;
	PVideoFrame (__stdcall Its::*pGetFrame)(int n, IScriptEnvironment* env);
	void		(Its::*pDispDebug)(int n, IScriptEnvironment* env, PVideoFrame &vf, int frame_no);
	PClip			dw_clip;
	char			msg[256];
	PARAMS			Params;
	FPS				Fps;
	MAP				*Map;
	OUTPUT			*Out;
	FILTERS			*Filters;
	ALIAS			*Alias;
	CHAPTERS		*Chapters;
	PATH			*Path;
	std::unique_ptr<VfrInfoMap> m_VfrInfoMap;
	VfrMap			*m_VfrMap;
	VfrOut			*m_VfrOut;
	ULONGLONG		dst_count;
	int				chap_count;
	int				err_lineno;
	int				err_frameno;
	int				org_num_frames;
	int				double_num_frames;
	int				target_num_frames;
	int				offset;
	int				frame_pre;
	int				frame_cur;
	int				frame_over;
	char			mode_pattern_origin;
	char			mode_pattern_shift;
	bool			mode_fps_adjust;
	bool			mode_keyframes_grouping;
	bool			POST_require_flag;
	bool			in_keyframes_flag;

public:
	static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
		if(user_data == NULL) {
			static Its *self = new Its(args, env);
			if(self->POST_require_flag==false)		return self;
			
			env->AddFunction("Its", "c", Its::Create, (void*)self);
			const AVSValue xx[] = { self };
			return  env->Invoke("Its", AVSValue(xx,1)).AsClip();
		}
		return new Its(args, env, user_data);
	}
	Its(AVSValue _args, IScriptEnvironment* env);
	Its(AVSValue _args, IScriptEnvironment* env, void *user_data);
	~Its();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) {return (this->*pGetFrame)(n, env);}
	char *GetName() const	{return "Its"; }
	int __stdcall SetCacheHints(int cachehints, int frame_range)
	{
		switch (cachehints)
		{
		case CACHE_GET_MTMODE:
			return MT_SERIALIZED;
		default:
			return 0;
		}
	}

private:
	PVideoFrame __stdcall GetFrame30fps(int n, IScriptEnvironment* env);
	PVideoFrame __stdcall GetFrame120fps(int n, IScriptEnvironment* env);
	PVideoFrame __stdcall GetFrame120fps_itvfr(int n, IScriptEnvironment* env);
	PVideoFrame __stdcall GetFrame24fps(int n, IScriptEnvironment* env);
	PVideoFrame __stdcall GetFramePostFilter(int n, IScriptEnvironment* env);
	void		PostFilter(PVideoFrame& clip, int n, IScriptEnvironment* env);
	void		ReadFrame(int n, IScriptEnvironment* env);
	int			FileRead(const char *filepath, IScriptEnvironment* env);
	int			GetDigit(char *&p);
	int			Skip_Comma(char *&p);
	int			Parse_Line(char *&p);
	int			Check_Filter(char *&name, bool add_filter);
	int			Define_Alias(char *&p);
	int			Define_Default_Alias(char *&p);
	int			Define_Offset(char *&p);
	int			Define_Mode(char *&p);
	int			Set_SelectEvery(int start, int end, int fps_type, int cycle, char *pattern);
	int			Set_Filter(int start, int end, int fps_type, char *&p,  char rightbracket);
	int			Set_Command(int start, int end, int fps_type, char *&p);
	int			Set_Pattern1(int start, int end, int fps_type, char *&p);
	int			Set_Pattern2(int start, int end, int fps_type, char *&p);
	int			Set_Post(int start, int end, int fps_type, char *&p);
	int			Check_Command(char *&p, char &a, char &b);
	int			Search_Alias(char *&p, char *&ptr, int fps_type);
	int			Set_Alias(int start, int end, int fps_type, char *&p);
	int			Set_Default_Alias(int start, int end, int fps_type, char *&p);
	int			Check_Bracket(char *&p, char &bracket);
	int			Set_Alias_Sub(int start, int end, int fps_type, char *ptr, int alias_id);
	void		Set_MapInfo_Sub(int start, int end, int fps_type, int filter_id, int alias_id);
	int			Skip_Time(char *&p);
	bool		IsDigit(char *p);
	bool		IsBit(char *p);
	void		DispDebug(int n, IScriptEnvironment* env, PVideoFrame &vf, int frame_no);
	void		DispDebugSub(PVideoFrame &vf, int x, int y, int n, int frame_no, int type);
	void		DispDebugNULL(int n, IScriptEnvironment* env, PVideoFrame &vf, int frame_no) {}
	void		DrawString(PVideoFrame &vf, char *string, int x, int y) {
					if(vi.IsYV12())       DispStringYV12(vf, string, x, y, false);
					else if(vi.IsYUY2())  DispStringYUY2(vf, string, x, y);
					else if(vi.IsRGB32()) DispStringRGB32(vf, string, x, y);
					else if(vi.IsRGB24()) DispStringRGB24(vf, string, x, y);
				}
	bool		Check_Delimiter(char *&p);
	bool		Check_Delimiter(char p);
	bool		Check_Comment(char *&p);
	bool		IsFilter(int i) {return (Map[i].filter_id>0 && Map[i].filter_id<=MAX_FILTERS) ? true : false;}
	bool		IsTPR(int i) {return (Map[i].filter_id==MAX_FILTERS+1) ? true : false;}
	void		Copy_TPRAttrib(int start, int end, TPR *Tpr);
	void		Set_StartAttrib(int start, int end);
	int			calc_delta(int i, ULONGLONG dst_count);
	int			AdjustFPSTiming(int i, int& n, ULONGLONG& dst_count);
	int			CreateOutMap(void);
	int			CreateStrippedOutMap(void);
	int			PreScanMap(void);
	int			Define_KeyFrames_Enter(char *&p);
	int			Set_KeyFrames(char *&p);
	int			Set_Chapter(char *&p, int start_frame);
	int			WriteChapterFile(const char *savefile);
	int			WriteChapterOGGFile(FILE *hFile, int hh, int mm, int ss, int tick, int frame, int index);
	int			WriteChapterXMLFile(FILE *hFile, int hh, int mm, int ss, int tick, int frame, int index);
	int			WriteTimecodesFile(const char *timecodesfile, const char *chapterfile);
	int			Calc_AdjustMaxFrames(int start, int end, int fps_type);
	void		Set_Attrib(int n, BYTE a) { Map[n].attrib |= a; }
	void		Reset_Attrib(int n, BYTE a) { Map[n].attrib &= ~a; }
	bool		IsAttrib(int n, BYTE a) { return !!(Map[n].attrib & a); }
	void		OutputError(IScriptEnvironment*env, int rc,
	                           const char*msg1, const char*msg2, const char*msg3);
};


#endif //its_h
