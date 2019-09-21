#include "its.h"

const AVS_Linkage *AVS_linkage = 0;

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(IScriptEnvironment* env, const AVS_Linkage* const vectors)
{
    AVS_linkage = vectors;
    env->AddFunction("Its", "c[def]s[tpr]s[fps]i[debug]b[posx]i[posy]i[chapter]s[output]s[opt]i", Its::Create, 0);
    env->AddFunction("ItsCut", "c[timecodes]s", ItsCut::Create, 0);
    return "`Its' IvTc/deint Switcher by kiraru2002";
}

//------------------------------------------------------------------------
#define TMP_SYNTH_VAR "_tmp_its_post_clip"
Its::Its(AVSValue _args, IScriptEnvironment* env, void *user_data)
 : GenericVideoFilter(_args[0].AsClip()), self((Its*)user_data) {
	if(user_data==NULL) {
		env->ThrowError("%s:(post) init error.", GetName());
	}
	if(!env->SetVar(TMP_SYNTH_VAR, _args[0].AsClip())) {
		env->ThrowError("%s:(post) SetVar failed.", GetName());
	}
	
	pGetFrame = &Its::GetFramePostFilter;
	
	for(int i=0; i<self->Filters->num_filters; i++) {
		if(self->Filters->request[i] & (1<<5)) {
			sprintf(msg, "%s.%s", TMP_SYNTH_VAR, self->Filters->name[i]);
			AVSValue arg = msg;
			try{
				self->Filters->post[i] = env->Invoke("Eval", AVSValue(&arg,1)).AsClip();
			}
			catch(IScriptEnvironment::NotFound) {
				env->ThrowError("%s:(post) NotFound. <%s>", GetName(), msg);
			}
			catch(AvisynthError ae) {
				env->ThrowError("%s:(post) Invoke failed. <%s>\n(%s)", GetName(), msg, ae.msg);
			}
		}
	}
}
//------------------------------------------------------------------------


Its::Its(AVSValue _args, IScriptEnvironment* env)
 : GenericVideoFilter(_args[0].AsClip()), self(NULL),
   m_VfrMap(NULL), m_VfrOut(NULL), Map(NULL), Out(NULL), Filters(NULL), Path(NULL), Alias(NULL)
   ,in_keyframes_flag(false), frame_pre(-1), frame_cur(-1)
{
	int i;
	int rc;

	Params.fps         = _args[3].AsInt(0);
	Params.debug       = _args[4].AsBool(false);
	Params.posx        = _args[5].AsInt(0);
	Params.posy        = _args[6].AsInt(0);
	Params.chapterfile = _args[7].AsString("");
	Params.outputfile  = _args[8].AsString("");
	Params.opt         = _args[9].AsInt(0);
	
	if(!Fps.checkfps(Params.fps))	env->ThrowError("%s: param error.(fps)", GetName());
	if(strlen(_args[1].AsString(""))==0 && strlen(_args[2].AsString(""))==0)
		env->ThrowError("%s: param error.(no def)", GetName());
	if(Params.posx<0)								Params.posx = 0;
	if(vi.width>200 && Params.posx>vi.width-200)	Params.posx = vi.width-200;
	if(Params.posy<0)								Params.posy = 0;
	if(vi.height>48 && Params.posy>vi.height-48)	Params.posy = vi.height-48;
	
	org_num_frames = vi.num_frames;
	double_num_frames = org_num_frames * 2;

	switch(Params.fps) {
		case 24:	
			target_num_frames = org_num_frames * 4 / 5;
			vi.SetFPS(vi.fps_numerator * 4, vi.fps_denominator * 5);
			Fps.priority = 24;
			break;
		case 60:
			target_num_frames = org_num_frames * 2;
			vi.SetFPS(vi.fps_numerator * 2, vi.fps_denominator);
			Fps.priority = 30;
			break;
		case 30:
			Fps.priority = 30;
			target_num_frames = org_num_frames;
			break;
		case 0:
		default:
			Fps.priority = 30;
			target_num_frames = org_num_frames * 2;
			break;
	}
	
	if(    ((Map=new MAP [double_num_frames])==NULL)
	    || ((Out=new OUTPUT [target_num_frames])==NULL)
	    || ((Filters=new FILTERS)==NULL)
	    || ((Path=new PATH)==NULL)
	    || ((Alias=new ALIAS)==NULL)
	    || ((Chapters=new CHAPTERS)==NULL)
	  )
		env->ThrowError("%s: not enough memory.", GetName());
	
	memset(Map, 0, double_num_frames * sizeof(MAP));
	memset(Out, 0, target_num_frames * sizeof(OUTPUT));
	memset(Filters, 0, sizeof(FILTERS));
	memset(Alias, 0, sizeof(ALIAS));
	memset(Chapters, 0, sizeof(CHAPTERS));
	offset = 0;
	mode_pattern_origin   = 0;
	mode_pattern_shift    = 0;
	mode_fps_adjust       = false;
	mode_keyframes_grouping = false;
	
	//---- .tpr project file ---
	Path->path = (char *)_args[2].AsString("");
	//_splitpath( Path->path, Path->drive, Path->dir, Path->fname, Path->ext );
	if(strlen(Path->path)>0) {
		TPR Tpr(double_num_frames);

		rc = Tpr.FileRead(Path->path);
		if(rc==-3)	env->ThrowError("%s: read error(%s)", GetName(), Path->path);
		if(rc==-2)	env->ThrowError("%s: open error(%s)", GetName(), Path->path);
		if(rc==-1)	env->ThrowError("%s: not enough memory", GetName());
		if (rc<0)	env->ThrowError("%s: .tpr file format error(%d).", GetName(), rc);
		Copy_TPRAttrib(0, org_num_frames, &Tpr);
	}
	
	Path->path = (char *)_args[1].AsString("");
	if(strlen(Path->path)>0) {
		_splitpath( Path->path, Path->drive, Path->dir, Path->fname, Path->ext );
		rc = FileRead(Path->path, env);
		if(rc<0) OutputError(env, rc, Path->fname, Path->ext, NULL);
	}
	
//	child->SetCacheHints(CACHE_NOTHING, 0);
//	env->SetMemoryMax(64);
	
	const AVSValue dw_args[] = { child };
	try{ 
		dw_clip = env->Invoke("DoubleWeave", AVSValue(dw_args, 1)).AsClip();
	}
	catch(IScriptEnvironment::NotFound) {
		env->ThrowError("%s: Invoke failed. - DoubleWeave", GetName());
	}
	catch(AvisynthError ae) {
		env->ThrowError("%s: Invoke failed. (%s)", GetName(), ae.msg);
	}

	for(i=0; i<Filters->num_filters; i++) {
		if((Filters->request[i] & 1)==0)  continue;
		const char* name = Filters->name[i];
		AVSValue arg = name;
		try{
			Filters->clip[i] = env->Invoke("Eval", AVSValue(&arg,1)).AsClip();
		}
		catch(IScriptEnvironment::NotFound) {
			env->ThrowError("%s: NotFound. <%s>", GetName(), name);
		}
		catch(AvisynthError ae) {
			env->ThrowError("%s: Invoke failed. <%s>\n(%s)", GetName(), name, ae.msg);
		}
		VfrOut vfr;
		if(!vfr.Err())  { Filters->vfr[i] = true; }
	}
	POST_require_flag = false;
	for(i=0; i<Filters->num_filters; i++) {
		if(Filters->request[i] & (1<<5)) {
			POST_require_flag = true;
		}
	}

	PreScanMap();
	rc = CreateStrippedOutMap();
	if(rc<0) OutputError(env, rc, NULL, NULL, NULL);
	vi.num_frames = rc;

	if(Params.fps<=0) {
		m_VfrInfoMap = std::unique_ptr<VfrInfoMap>(new VfrInfoMap(vi.num_frames, 0));
		m_VfrOut = new VfrOut;
		if(m_VfrOut->Err()) {
			delete m_VfrOut; m_VfrOut=NULL;
			Params.opt = 0;
		}
		if(m_VfrInfoMap->Err()) env->ThrowError("%s: Not Allocated Shared Memory.", GetName());
		m_VfrInfoMap->Enter(0,INFINITE);
		m_VfrInfoMap->SetSubVersion(1);
		m_VfrInfoMap->SetDenominator((BYTE)Fps.fps2coeff(-1));
		m_VfrInfoMap->Release(0);
	}

	if(Params.opt==0) {
		if(strlen(Params.chapterfile) > 0) {
			rc = WriteChapterFile(Params.chapterfile);
			if(rc<0) OutputError(env, rc, NULL, NULL, NULL);
		}
		if(strlen(Params.outputfile) > 0) {
			rc = WriteTimecodesFile(Params.outputfile, Params.chapterfile);
			if(rc<0) OutputError(env, rc, NULL, NULL, NULL);
		}
	}

	if(Params.fps>0) {
		pGetFrame = &Its::GetFrame30fps;
	} else {
		if(Params.opt==0) {
			pGetFrame = &Its::GetFrame120fps;
		} else {
			pGetFrame = &Its::GetFrame120fps_itvfr;
		}
	}
	pDispDebug = (Params.debug) ? &Its::DispDebug : &Its::DispDebugNULL;

	dst_count  = 0;
	chap_count = 0;
	frame_over = vi.num_frames-1;
}

Its::~Its() {
	if(self==NULL) {
		delete Chapters;
		delete Alias;
		delete Filters;
		delete [] Out;
		delete [] Map;
		if(m_VfrMap) { delete m_VfrMap; m_VfrMap=NULL; }
		if(m_VfrOut) { delete m_VfrOut; m_VfrOut=NULL; }
	}
}

int Its::PreScanMap(void) {
	int i;
	bool shift_chapter = false;
	int  start     = -1;
	int  fps       = -1;
	BYTE filter_id = -1;
	BYTE adjust    = -1;


	for (i=0; i<double_num_frames; i++) {
		if(IsAttrib(i, ATTRIB_SET|ATTRIB_COPY)) {
			if( (start!=Map[i].start) || (fps!=Map[i].fps) || (filter_id!=Map[i].filter_id)
			     || (adjust!=Map[i].adjust) )
			{
				if(Map[i].adjust)	Map[i].adjust |= 2;
				start     = Map[i].start;
				fps       = Map[i].fps;
				filter_id = Map[i].filter_id;
				adjust    = Map[i].adjust & 1;
			}
			
			if(shift_chapter) {
				Set_Attrib(i,ATTRIB_CHAPTER|ATTRIB_KEYFRAME);
				shift_chapter=false;
			}
		} else {
			if(IsAttrib(i, ATTRIB_CHAPTER)) {
				shift_chapter = true;
			}
		}
	}
	return 0;
}

int Its::CreateStrippedOutMap(void) {
	int				rc;
	int				n = 0;
	int				copy=0;
	bool			adjust_flag = false;
	double				duration = 0;
	dst_count  = 0;
	chap_count = 0;
	
	for(int i=0; i<double_num_frames; i++) {
		if(IsAttrib(i,ATTRIB_SET|ATTRIB_COPY)) {
			// selected frame
				duration = Fps.fps2coeff(Map[i].fps);
				double frame_time = duration * (Map[i].frame_start + Map[i].frame);
				if (frame_time < dst_count) {
					// 前のフレームの表示時間より前なのでスキップ
					Reset_Attrib(i, ATTRIB_SET | ATTRIB_COPY | ATTRIB_DELETE);
					continue;
				}
				/*
				if(adjust_flag || (Map[i].adjust & 2)) {
					delta=calc_delta(i,dst_count);
					if(delta<0) {
						rc = AdjustFPSTiming(i, n, dst_count);
						if(rc<0) { err_frameno = i>>2; return rc; }
						dst_count -= delta;
					}
					if(delta>0) {
						adjust_flag = true;
						Reset_Attrib(i, ATTRIB_SET|ATTRIB_COPY|ATTRIB_DELETE);
						continue;
					}
					adjust_flag = false;
				}
				*/
				if(IsAttrib(i,ATTRIB_SET)) {
					Out[n].refer  = i;
					copy = i;
				} else {
					Out[n].refer  = copy;
				}
				Out[n].attrib = Map[i].attrib;
				Out[n].fps    = Fps.fps2coeff(Map[i].fps);
				Out[n].delta  = calc_delta(i, dst_count);
				Out[n].time   = (frame_time * (1000/4/FPS::FPS_DIV_COEFF) * vi.fps_denominator ) / vi.fps_numerator;
				dst_count = frame_time;
				if(IsAttrib(i, ATTRIB_CHAPTER)) {
					Chapters->c[chap_count].frame = n;
					++chap_count;
				}
				++n;
		}
	}
	dst_count += duration;
	Chapters->endtime = (dst_count * (1000/4/FPS::FPS_DIV_COEFF) * vi.fps_denominator ) / vi.fps_numerator;
	return n;
}

double Its::calc_delta(int i, double dst_count) {
	return dst_count - i * Fps.fps2coeff(60);
}

int Its::AdjustFPSTiming(int i, int& n, double& dst_count) {
	double delta = calc_delta(i, dst_count);
	
	if(n<=0) return 0;
	double count = Out[n-1].fps - delta;
	if( count > 255) return ITS_ERR_ADJUST;
	Out[n-1].fps = (BYTE)count;
	return 0;
}

int Its::CreateOutMap(void) {
	return 0;
}

int Its::WriteChapterFile(const char *savefile) {
	FILE *hFile;
	int i;
	int rc;
	int hh,mm,ss,tick;
	BYTE chapter = 0;
	int end;
	
	if(Params.opt==0) {
		end = vi.num_frames;
	} else {
		end = frame_over;
	}
	hFile = fopen(savefile, "w");
	if(hFile) {
		for(i=0; i<end; i++) {
			if(!(Out[i].attrib & (ATTRIB_SET|ATTRIB_COPY)) || !(Out[i].attrib & ATTRIB_CHAPTER)) continue;
			hh = (int)(Out[i].time / 1000) / 3600;
			mm = (int)(Out[i].time / 1000) / 60 % 60;
			ss = (int)(Out[i].time / 1000) % 60;
			tick = (int)Out[i].time % 1000;
			if(Chapters->language[0]==0) {
				if((rc = WriteChapterOGGFile(hFile, hh, mm, ss, tick, i, chapter)) < 0) return rc;
			} else {
				if((rc = WriteChapterXMLFile(hFile, hh, mm, ss, tick, i, chapter)) < 0) return rc;
			}
			++chapter;
		}
		fclose(hFile);
	} else {
		return ITS_ERR_CHAPTER_OPENFILE;
	}
	return 0;
}

int Its::WriteChapterOGGFile(FILE *hFile, int hh, int mm, int ss, int tick, int frame, int index) {
	if(index==0 && Chapters->utf8_bom) {
		if(fprintf(hFile, "%c%c%c", (char)0xEF, (char)0xBB, (char)0xBF) < 0)  return ITS_ERR_CHAPTER_WRITE;
	}
	if(fprintf(hFile, "CHAPTER%02d=%02d:%02d:%02d.%03d\nCHAPTER%02dNAME=%s\n",
	            index, hh, mm, ss, tick,
	            index, Chapters->c[index].text) < 0) {
		return ITS_ERR_CHAPTER_WRITE;
	}
	return 0;
}
int Its::WriteChapterXMLFile(FILE *hFile, int hh, int mm, int ss, int tick, int frame, int index) {
	double endtime;
	int hh2, mm2, ss2, tick2;

	if(index==0) {
		if(Chapters->utf8_bom) {
			if(fprintf(hFile, "%c%c%c", (char)0xEF, (char)0xBB, (char)0xBF) < 0)  return ITS_ERR_CHAPTER_WRITE;
		}
		if(fprintf(hFile, "<?xml version=\"1.0\" encoding=\"%s\"?>\n"
		                  "<!-- <!DOCTYPE Tags SYSTEM \"matroskatags.dtd\"> -->\n\n"
		                  "<Chapters>\n"
		                  "  <EditionEntry>\n"
		                , Chapters->charset
		          ) < 0)
			return ITS_ERR_CHAPTER_WRITE;
	}
	if(index>=Chapters->num_chapters-1) {
		endtime = Chapters->endtime;
	} else {
		endtime = Out[Chapters->c[index+1].frame].time-1;
	}
	hh2 = (int)(endtime / 1000) / 3600;
	mm2 = (int)(endtime / 1000) / 60 % 60;
	ss2 = (int)(endtime / 1000) % 60;
	tick2 = (int)endtime % 1000;

	if(fprintf(hFile, "    <ChapterAtom>\n"
	                  "      <ChapterDisplay>\n"
	                  "        <ChapterString>%s</ChapterString>\n"
	                  "        <ChapterLanguage>%s</ChapterLanguage>\n"
	                  "      </ChapterDisplay>\n"
	                  "      <ChapterFlagHidden>0</ChapterFlagHidden>\n"
	                  "      <ChapterFlagEnabled>1</ChapterFlagEnabled>\n"
	                  "      <ChapterTimeStart>%02d:%02d:%02d.%03d</ChapterTimeStart>\n"
	                  "      <ChapterTimeEnd>%02d:%02d:%02d.%03d</ChapterTimeEnd>\n"
	                  "    </ChapterAtom>\n"
	                  , Chapters->c[index].text
	                  , Chapters->language
	                  , hh, mm, ss, tick
	                  , hh2, mm2, ss2, tick2
	           ) < 0) {
		return ITS_ERR_CHAPTER_WRITE;
	}
	if(index==Chapters->num_chapters-1) {
		if(fprintf(hFile, 
		                  "    <EditionFlagDefault>0</EditionFlagDefault>\n"
		                  "    <EditionFlagHidden>0</EditionFlagHidden>\n"
		                  "  </EditionEntry>\n"
		                  "</Chapters>\n"
		          ) < 0)
			return ITS_ERR_CHAPTER_WRITE;
	}
	return 0;
}


int Its::WriteTimecodesFile(const char *timecodesfile, const char *chapterfile) {
	FILE *hFile;
	char pathbuff[MAX_PATH];
	int i;
	int end;

	if(Params.opt==0) {
		end = vi.num_frames;
	} else {
		end = frame_over;
	}
	if(*timecodesfile=='.') { 
		_splitpath( chapterfile, Path->drive, Path->dir, Path->fname, Path->ext );
		_makepath( pathbuff, Path->drive, Path->dir, Path->fname, timecodesfile );
		hFile = fopen(pathbuff, "w");
	} else {
		hFile = fopen(timecodesfile, "w");
	}
	if(hFile==NULL) return ITS_ERR_TIMECODES_OPENFILE;
	if(fprintf(hFile, "# timecode format v2\n")<0) {
		return ITS_ERR_TIMECODES_WRITE;
	}
	for(i=0; i<end; i++) {
		if(!(Out[i].attrib & (ATTRIB_SET|ATTRIB_COPY))) continue;
		if(fprintf(hFile, "%I64d\n", (__int64)Out[i].time) < 0) {
			return ITS_ERR_TIMECODES_WRITE;
		}
	}
	fclose(hFile);
	return 0;
}

