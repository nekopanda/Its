#include "its.h"

PVideoFrame __stdcall Its::GetFramePostFilter(int n, IScriptEnvironment* env) {
	PClip   post = child;
	if(self->Map[self->Out[n].refer].post>0) {
		post = self->Filters->post[self->Map[self->Out[n].refer].post-1];
	}
	PVideoFrame result = post->GetFrame(n,env);
	return result;
}

PVideoFrame __stdcall Its::GetFrame30fps(int n, IScriptEnvironment* env) {
	PVideoFrame		result;
	int		frame_no;
	
	if(!IsFilter(Out[n].refer)) {
		result = dw_clip->GetFrame(Out[n].refer, env);
		frame_no=Out[n].refer>>1;
	} else {
		int offs = Map[Out[n].refer].clip_offs;;
		if(Map[Out[n].refer].fps==24) {
			frame_no = ((Map[Out[n].refer].start>>1)+5-offs)/5*4-4+Map[Out[n].refer].frame;
		}
		else {
			frame_no = (Out[n].refer>>1)-offs;
		}
		result = Filters->clip[Map[Out[n].refer].filter_id-1][offs]->GetFrame(frame_no, env);
	}
	(this->*pDispDebug)(n, env, result, frame_no);
	return result;
}

PVideoFrame __stdcall Its::GetFrame24fps(int n, IScriptEnvironment* env) {
	return GetFrame30fps(n, env);
}

PVideoFrame __stdcall Its::GetFrame120fps(int n, IScriptEnvironment* env) {
	PVideoFrame		result;
	int				frame_no;
	
	if(!IsFilter(Out[n].refer)) {
		frame_no = Out[n].refer>>1;
		result = dw_clip->GetFrame(Out[n].refer, env);
	} else {
		int offs = Map[Out[n].refer].clip_offs;
		switch(Map[Out[n].refer].fps) {
		case 24:
		case 48:
			frame_no = ((Map[Out[n].refer].start>>1)+5-offs)/5*4-4+Map[Out[n].refer].frame;
			break;
		case 60:
			frame_no = Out[n].refer;
			break;
		default:
			frame_no = (Out[n].refer>>1)-offs;
			break;
		}
		result = Filters->clip[Map[Out[n].refer].filter_id-1][offs]->GetFrame(frame_no, env);
	}

	m_VfrInfoMap->Enter(1,INFINITE);
	m_VfrInfoMap->SetCount(n, Out[n].fps);
	if(Map[Out[n].refer].fps==30 && IsFilter(Out[n].refer)) {
		int c;
		if( Filters->vfr[ Map[Out[n].refer].filter_id-1 ] ) {
			if(m_VfrOut) {
				switch(m_VfrOut->GetStatus(frame_no)) {
					case 'D': c = 0; break;
					case '2': c = 24; break;
					default:  c = 30; break;
				}
			} else {
				c = 30;
			}
			m_VfrInfoMap->SetCount(n, Fps.fps2coeff(c));
		}
	}
	m_VfrInfoMap->CopyFlag(n, Out[n].attrib & (ATTRIB_COPY|ATTRIB_KEYFRAME));
	m_VfrInfoMap->Release(1);
	(this->*pDispDebug)(n, env, result, frame_no);
	return result;
}

PVideoFrame __stdcall Its::GetFrame120fps_itvfr(int n, IScriptEnvironment* env) {
	PVideoFrame		result;
	int				frame_no;
	int				offs = Map[Out[n].refer].clip_offs;
	char			s;
	int				rc;
	
	if(n == frame_pre+1) {
		++frame_pre;
		++frame_cur;
		while(frame_cur < vi.num_frames && Filters->vfr[Map[Out[frame_cur].refer].filter_id-1]) {
			Filters->clip[Map[Out[frame_cur].refer].filter_id-1][offs]->GetFrame(Out[frame_cur].refer>>1, env);
			s = m_VfrOut->GetStatus(Out[frame_cur].refer>>1);
			if(s != 'D') {
				break;
			}
			memset((void*)&Out[frame_cur], 0, sizeof(Out[0]));
			++frame_cur;
		}
		if(frame_cur < vi.num_frames) {
			memcpy(&Out[frame_pre], &Out[frame_cur], sizeof(Out[0]));
		} else {
			memcpy(&Out[n], &Out[vi.num_frames-1], sizeof(Out[0]));
			if(frame_over > n) {
				frame_over = n;
				if(strlen(Params.chapterfile) > 0) {
					rc = WriteChapterFile(Params.chapterfile);
					if(rc<0) OutputError(env, rc, NULL, NULL, NULL);
				}
				if(strlen(Params.outputfile) > 0) {
					rc = WriteTimecodesFile(Params.outputfile, Params.chapterfile);
					if(rc<0) OutputError(env, rc, NULL, NULL, NULL);
				}
			}
		}
		m_VfrInfoMap->Enter(1,INFINITE);
		if(n>0)  dst_count += m_VfrInfoMap->GetCount(n-1);
		m_VfrInfoMap->Release(1);
		Out[n].time = (dst_count * (1000/4/FPS::FPS_DIV_COEFF) * vi.fps_denominator + vi.fps_numerator/2 )
                       / vi.fps_numerator;
	}
	
	if(!IsFilter(Out[n].refer)) {
		frame_no = Out[n].refer>>1;
		result = dw_clip->GetFrame(Out[n].refer, env);
	} else {
		switch(Map[Out[n].refer].fps) {
		case 24:
		case 48:
			frame_no = ((Map[Out[n].refer].start>>1)+5-offs)/5*4-4+Map[Out[n].refer].frame;
			break;
		case 60:
			frame_no = Out[n].refer;
			break;
		default:
			frame_no = (Out[n].refer>>1)-offs;
			break;
		}
		result = Filters->clip[Map[Out[n].refer].filter_id-1][offs]->GetFrame(frame_no, env);
	}

	m_VfrInfoMap->Enter(1,INFINITE);
	if(n >= frame_over) {
		Out[n].attrib = 0;
		m_VfrInfoMap->SetCount(n, 0);
	} else {
		m_VfrInfoMap->SetCount(n, Out[n].fps);
		if(Map[Out[n].refer].fps==30 && IsFilter(Out[n].refer)) {
			int c;
			if( Filters->vfr[ Map[Out[n].refer].filter_id-1 ] ) {
				if(m_VfrOut) {
					switch(s = m_VfrOut->GetStatus(frame_no)) {
						case 'D': c = 0; break;
						case '2': c = 24; break;
						default:  c = 30; break;
					}
				} else {
					c = 30;
				}
				m_VfrInfoMap->SetCount(n, Fps.fps2coeff(c));
			}
		}
	}
	m_VfrInfoMap->CopyFlag(n, Out[n].attrib & (ATTRIB_COPY|ATTRIB_KEYFRAME));
	m_VfrInfoMap->Release(1);
	(this->*pDispDebug)(n, env, result, frame_no);
	return result;
}

void Its::DispDebug(int n, IScriptEnvironment* env, PVideoFrame& result, int frame_no) {
	env->MakeWritable(&result);
	if(!IsFilter(Out[n].refer)) {
		DispDebugSub(result, Params.posx, Params.posy, n, Out[n].refer, 0);
		if(IsTPR(Out[n].refer)) {DispDebugSub(result, Params.posx, Params.posy, n, 0, 2);}
	} else {
		DispDebugSub(result, Params.posx, Params.posy, n, frame_no, 1);
		DispDebugSub(result, Params.posx, Params.posy, n, 0, 2);
	}
	DispDebugSub(result, Params.posx, Params.posy, n, 0, (Params.fps<=0) ? 3 : 4);
	DispDebugSub(result, Params.posx, Params.posy, n, 0, 5);
}

void Its::DispDebugSub(PVideoFrame& pframe, int x, int y, int n, int frame_no, int type) {
	int count, denom, fps;
	int hh,mm,ss,tick;
	int i;

	switch(type) {
	case 0:
		if(Params.fps==24) {
			sprintf(msg, "frame=%d(%d %d %d) %dfps delta=%.2f %s"
				, n
				, Out[n].refer>>1
				, frame_no
				, (Out[n].refer)*4/10
				, Map[Out[n].refer].fps
				, (float)Out[n].delta/Fps.fps2coeff(Map[Out[n].refer].fps)
				, (Out[n].attrib & ATTRIB_KEYFRAME) ? "[K]" : ""
			);
		} else {
			sprintf(msg, "frame=%d(%d %d %d) %dfps delta=%.2f %s"
				, n
				, Out[n].refer>>1
				, Out[n].refer
				, frame_no
				, Map[Out[n].refer].fps
				, (float)Out[n].delta/Fps.fps2coeff(Map[Out[n].refer].fps)
				, (Out[n].attrib & ATTRIB_KEYFRAME) ? "[K]" : ""
			);
		}
		DrawString(pframe, msg, x, y);
		break;
	case 1:
		sprintf(msg, "frame=%d(%d %d %d<+%d>) %dfps delta=%.2f %s"
				, n
				, Out[n].refer>>1
				, Out[n].refer
				, frame_no
				, Map[Out[n].refer].clip_offs
				, Map[Out[n].refer].fps
				, (float)Out[n].delta/Fps.fps2coeff(Map[Out[n].refer].fps)
				, (Out[n].attrib & ATTRIB_KEYFRAME) ? "[K]" : ""
			);
		DrawString(pframe, msg, x, y);
		break;
	case 2:
		if(Map[Out[n].refer].alias_id>0) {
			sprintf(msg, "<%s> [%s]"
					, Alias->alias[Map[Out[n].refer].alias_id-1]
					, IsTPR(Out[n].refer) ? "TMPGEnc" : Filters->name[Map[Out[n].refer].filter_id-1]
			);
			DrawString(pframe, msg, x, y+16);
		} else if(Map[Out[n].refer].alias_id==-1) {
			sprintf(msg, "<%s> [%s]"
					, Alias->default_string[Fps.fps2index(Out[n].fps)]
					, IsTPR(Out[n].refer) ? "TMPGEnc" : Filters->name[Map[Out[n].refer].filter_id-1]
			);
			DrawString(pframe, msg, x, y+16);
		} else {
			sprintf(msg, "[%s]"
					, IsTPR(Out[n].refer) ? "TMPGEnc" : Filters->name[Map[Out[n].refer].filter_id-1]
			);
			DrawString(pframe, msg, x, y+16);
		}
		break;
	case 3:
		m_VfrInfoMap->Enter(0,INFINITE);
		denom = (int)m_VfrInfoMap->GetDenominator();
		m_VfrInfoMap->Release(0);
		m_VfrInfoMap->Enter(1,INFINITE);
		count = (int)m_VfrInfoMap->GetCount(n);
		m_VfrInfoMap->Release(1);
		fps =Fps.coeff2fps(count);
		hh   = (int)(Out[n].time / 1000 / 3600);
		mm   = (int)(Out[n].time / 1000 / 60 % 60);
		ss   = (int)(Out[n].time / 1000 % 60);
		tick = (int)(Out[n].time % 1000);
		sprintf(msg, "<vfrout> [%02d:%02d:%02d.%03d] %d/%d <%dfps> adjust=%d",
					hh, mm, ss, tick,
					count, m_VfrInfoMap->GetDenominator(), fps, Map[Out[n].refer].adjust);
		DrawString(pframe, msg, x, y+32);
		break;
	case 4:
		hh   = (int)(Out[n].time / 1000 / 3600);
		mm   = (int)(Out[n].time / 1000 / 60 % 60);
		ss   = (int)(Out[n].time / 1000 % 60);
		tick = (int)(Out[n].time % 1000);
		sprintf(msg, "<out> [%02d:%02d:%02d.%03d] %dfps", hh, mm, ss, tick, Params.fps);
		DrawString(pframe, msg, x, y+32);
		break;
	case 5:
		if(Out[n].attrib & ATTRIB_CHAPTER) {
			for(i=Chapters->num_chapters-1; i>=0; i--) {
				if(n >= Chapters->c[i].frame) break;
			}
			if(i>=0) {
				sprintf(msg, "<CHAPTER%02d>", i);
				DrawString(pframe, msg, x, y+48);
			}
		}
		break;
	}
}

