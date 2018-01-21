#ifndef itscut_h
#define itscut_h

#include <windows.h>
#include <wincon.h>
#include "avisynth.h"
#include "vfrmap.h"


class ItsCut : public GenericVideoFilter {
private:
	VfrInfoOut *vfr;
	char *GetName()	const {return "ItsCut"; }
	int  src_frame;
	int  dst_frame;
	int  end_frame;
	double  *timecode;
	const char *timecodesfilename;
	BOOL enable_timecodes;
	double total_count;
	
	
public:
	static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
		return  new ItsCut(args, env);
	}
	ItsCut(AVSValue _args, IScriptEnvironment* env)
	 : GenericVideoFilter(_args[0].AsClip())
	 , src_frame(-1), dst_frame(-1), end_frame(vi.num_frames-1)
	 , timecode(NULL), enable_timecodes(FALSE), total_count(0)
	 {
		child->SetCacheHints(CACHE_NOTHING, 0);
		vfr = new VfrInfoOut;
		if(vfr->Err()) {
			env->ThrowError("%s: requires using its filter.", GetName());
		}
		timecodesfilename = _args[1].AsString("");
		if(timecodesfilename[0]) enable_timecodes = TRUE;
		if((timecode = new double [vi.num_frames-1])==NULL) {
			env->ThrowError("%s: not enough memory.", GetName());
		}
		vfr->MakeWritable();
	}
	
	~ItsCut() {
		vfr->MakeNoWritable();
		if(vfr) {delete vfr; vfr = NULL;}
		if(timecode) {delete [] timecode; timecode=NULL; }
	}
	
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) {
		double count;
		
		if(dst_frame>=end_frame) {
			vfr->SetCount(n, 0);
			return child->GetFrame(end_frame, env);
		}
		
		if(n == src_frame+1) {
			++src_frame;
			++dst_frame;
			child->GetFrame(dst_frame    , env);
			child->GetFrame(dst_frame + 1, env);
			if(vfr->GetCount(dst_frame + 1) == 0)		end_frame = dst_frame;
			count = vfr->GetCount(dst_frame);
			total_count += count;
			timecode[n] = (total_count * (1000/4/2) * vi.fps_denominator ) / vi.fps_numerator;
//			vfr->SetCount(n, count);
			if(dst_frame >= end_frame) {
				if(enable_timecodes > 0) {
					WriteTimecodesFile(timecodesfilename, n);
				}
				GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
			}
		} else {
			MessageBox(NULL,"no sequential frame access","ItsCut",MB_OK|MB_ICONSTOP);
		}
		return child->GetFrame(dst_frame, env);
	}
	
	int WriteTimecodesFile(const char *fname, int end) {
		int i;
		FILE *hFile;
		hFile = fopen(fname, "w");
		if(hFile==NULL) return -1;
		if(fprintf(hFile, "# timecode format v2\n")<0) {
			return -1;
		}
		for(i=0; i<=end; i++) {
			if(fprintf(hFile, "%I64d\n", (__int64)timecode[i]) < 0) {
				return -1;
			}
		}
		fclose(hFile);
		return 0;
	}

};

#endif //itscut_h
