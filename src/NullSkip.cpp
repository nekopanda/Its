#include <windows.h>
#include <stdio.h>
#include "avisynth.h"
#include "common.h"
#include "vfrmap.h"

class NullSkip : public GenericVideoFilter {
private:
	VfrOut	*m_VfrOut;
	PClip	filter_clip;
	int		pre_frameno;
	
	char *GetName()	const {return "NullSkip"; }
	
public:
	static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
		return  new NullSkip(args, env);
	}
	NullSkip(AVSValue _args, IScriptEnvironment* env)
	 : GenericVideoFilter(_args[0].AsClip()) {
		
		pre_frameno = 0;
		child->SetCacheHints(CACHE_NOTHING, 0);
		
		m_VfrOut = new VfrOut;
		if(m_VfrOut->Err()) {
			env->ThrowError("%s: no filter of vfrout.", GetName());
		}
		
		try{
			filter_clip = env->Invoke("Eval", AVSValue(&_args[1],1)).AsClip();
		}
		catch(IScriptEnvironment::NotFound) {
			env->ThrowError("%s:NotFound. <%s>", GetName(), _args[1].AsString());
		}
		catch(AvisynthError ae) {
			env->ThrowError("%s:Invoke failed.<%s>\n(%s)", GetName(), _args[1].AsString(), ae.msg);
		}
		
		VideoInfo filter_vi = filter_clip->GetVideoInfo();
		vi.width = filter_vi.width;
		vi.height = filter_vi.height;
		vi.pixel_type = filter_vi.pixel_type;
		vi.image_type = filter_vi.image_type;
	}
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

PVideoFrame __stdcall NullSkip::GetFrame(int n, IScriptEnvironment* env) {
	PVideoFrame result = child->GetFrame(n, env);
	if(n>0 && m_VfrOut->GetStatus(n)=='D') {
		return filter_clip->GetFrame(n-1, env);
	}
	return filter_clip->GetFrame(n, env);
}

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env) {
    env->AddFunction("NullSkip", "c[filter]s", NullSkip::Create, 0);
    return "`NullSkip' vfrout support filter by kiraru2002";
}
