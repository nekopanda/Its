#include "its.h"

int Its::FileRead(const char *filepath, IScriptEnvironment* env) {
	FILE	*fh;
	int		rc;
	char	*buf, *ptr;
	if( (buf = new char [READ_BUF_SIZE]) == NULL) return -1;

	if( (fh = fopen(filepath, "r")) == NULL ) {
		delete [] buf;
		return -2;
	}
	err_lineno = 0;
	while(!feof(fh)) {
		if( fgets(buf, READ_BUF_SIZE-1, fh) == NULL) {
			if(!feof(fh))	rc = -3;
			break;
		}
		ptr = buf;
		if(err_lineno==0) {
			if(buf[0]==(char)0xEF && buf[1]==(char)0xBB && buf[2]==(char)0xBF) {
				ptr = buf+3;	//Delete UTF-8 BOM
				Chapters->utf8_bom = 1;
				memcpy(Chapters->charset, "UTF-8", 5);
			} else {
				memcpy(Chapters->charset, "SHIFT_JIS", 9);
			}
		}
		rc = Parse_Line(ptr);
		if(rc<0 || rc==999) break;
	}
	fclose(fh);
	delete [] buf;
	return rc;
}

int Its::GetDigit(char *&p) {
	int n=0;
	Skip_Space(p);
	if(*p<'0' || *p>'9')	return -1;
	while(*p>='0' && *p<='9') {
		n = n*10 + *p++ - '0';
	}
	Skip_Space(p);
	return n;
}

int Its::Skip_Comma(char *&p) {
	Skip_Space(p);
	if( *p != ',' ) return -1;
	p++;
	Skip_Space(p);
	return 0;
}

bool Its::Check_Delimiter(char *&p) {
	return Check_Delimiter(*p);
}
bool Its::Check_Delimiter(char c) {
	return (c==NULL || c==LF || c=='#' || c==' ' || c=='\t') ? true : false;
}

bool Its::Check_Comment(char *&p) {
	Skip_Space(p);
	return (*p==NULL || *p==LF || *p=='#') ? true : false;
}

int Its::Parse_Line(char *&p) {
	int fps_type;
	int start_frame, end_frame;
	int rc = 0;
	
	err_lineno++;
	Skip_Space(p);
	if(*p == '#' || *p==LF || *p==NULL) return 0;
	if(in_keyframes_flag) {return Set_KeyFrames(p);}
	if (!_memicmp(p, "set"     , 3))		{ p+=3; return Define_Alias(p); }
	if (!_memicmp(p, "default" , 7))		{ p+=7; return Define_Default_Alias(p); }
	if (!_memicmp(p, "offset"  , 6))		{ p+=6; return Define_Offset(p); }
	if (!_memicmp(p, "mode"    , 4))		{ p+=4; return Define_Mode(p); }
	if (!_memicmp(p, "end"     , 3))		{ p+=3; return Check_Delimiter(p) ? 999 : -999; }
	if (!_memicmp(p, "keyframes" , 9))		{ p+=9; return Define_KeyFrames_Enter(p); }

	fps_type = Fps.priority;
	
	start_frame = GetDigit(p);
	if(start_frame<0)									return ITS_ERR_FRAME_NO;
	end_frame = start_frame = start_frame + offset;
	if(start_frame <0 || start_frame > org_num_frames)	return ITS_ERR_FRAME_NO;
	while(rc==0) {
		char leftbracket = 0, rightbracket = 0;
		Skip_Space(p);
		switch(*p++) {
		case 0:
		case LF:
		case '#':
			if(!mode_keyframes_grouping && (start_frame==end_frame)) {//keyframe
				Set_Attrib(start_frame * 2, ATTRIB_KEYFRAME);
				rc = 0;
			} else {
				rc=Set_Default_Alias(start_frame, end_frame, fps_type, p);
			}
			break;
		case '-':
			end_frame = GetDigit(p);
			if(end_frame<0)									end_frame = org_num_frames-1;
			end_frame += offset;
			if(end_frame<0 || end_frame>=org_num_frames)	end_frame = org_num_frames-1;
			continue;
		case ':':
			continue;
		case '[': // [24],[30],[60],[48],[20],[10],[12],[15],[25]
			Skip_Space(p);
			fps_type = Fps.search2fps(p, Fps.str);
			if(fps_type>0) {
				Skip_Space(p);
				if(*p++ != ']') rc=ITS_ERR_FPS;
			} else {
				rc = ITS_ERR_FPS;
			}
			continue;
		case '(': // (10,0,2,5,7)
			rc = Set_Pattern1(start_frame, end_frame, fps_type, p);
			if(rc>=0) rc=1;
			break;
		case '<': // <1010010100>
			rc = Set_Pattern2(start_frame, end_frame, fps_type, p);
			if(rc>=0) rc=1;
			break;
		case '\'':
			rc = Set_Filter(start_frame, end_frame, fps_type, p, '\'');
			if(rc>=0) rc=1;
			break;
		case '"':
			rc = Set_Filter(start_frame, end_frame, fps_type, p, '"' );
			if(rc>=0) rc=1;
			break;
		case '`':
			rc = Set_Filter(start_frame, end_frame, fps_type, p, '\'');
			if(rc>=0) rc=1;
			break;
		case '{':
			rc = Set_Filter(start_frame, end_frame, fps_type, p, '}' );
			if(rc>=0) rc=1;
			break;
		default:
			p--;
			if(Skip_Time(p)>=0) continue;
			
			rc=Set_Alias(start_frame, end_frame, fps_type, p);
			if(rc<0) {
				rc = Set_Command(start_frame, end_frame, fps_type, p);
				if(rc==-101) rc=-11;
			}
			break;
		}
		break;
	}
	return rc;
}

bool Its::IsDigit(char *p) { return (*p>='0' && *p<='9') ? true : false; }
bool Its::IsBit(char *p) { return (*p=='0' || *p=='1') ? true : false; }

int Its::Set_Pattern1(int start, int end, int fps_type, char *&p) {
	int rc = 0;
	char pattern[MAX_SELECTEVERY_PATTERNS] = { 0 };
	int cycle;

	Skip_Space(p);
	cycle = GetDigit(p);
	if(cycle < 0) { rc=-1002;}
	else {
		while(rc==0) {
			int index;
			Skip_Comma(p);
			if( (index=GetDigit(p)) < 0)	rc=-1;
			if(index>=0 && index<MAX_SELECTEVERY_PATTERNS) {
				pattern[index] = 1;
			} else {
				rc = -1;
			}
		}
		if(*p++ != ')')	{
			rc = -1002;
		} else {
			Set_SelectEvery(start, end, fps_type, cycle, pattern);
			rc = 1;
		}
	}
	return rc;
}

int Its::Set_Pattern2(int start, int end, int fps_type, char *&p) {
	int rc;
	char pattern[MAX_SELECTEVERY_PATTERNS] = { 0 };
	int cycle;

	Skip_Space(p);
	cycle = 0;
	while(IsBit(p) && cycle < MAX_SELECTEVERY_PATTERNS-2) {
		pattern[cycle] = *p++ - '0';
		cycle++;
	}
	if(*p++ != '>') {
		rc = -1002;
	} else {
		Set_SelectEvery(start, end, fps_type, cycle, pattern);
		rc = 1;
	}
	return rc;
}

int Its::Calc_AdjustMaxFrames(int start, int end, int fps_type) {
	int max_frames = (end-start)*2;
	switch(fps_type) {
		case 10:
			max_frames = (end-start)/3;
			break;
		case 12:
			max_frames = (end-start)*2/5;
			break;
		case 15:
			max_frames = (end-start)/2;
			break;
		case 24:
			max_frames = (end-start)*4/5;
			break;
		case 25:
			max_frames = (end - start) * 5 / 6;
			break;
		case 30:
			max_frames = end-start;
			break;
		case 48:
			max_frames = (end-start)*4/5 * 2;
			break;
	}
	return max_frames;
}

int Its::Set_SelectEvery(int start, int end, int fps_type, int cycle, char *pattern) {
	int cur_cycle = 0;
	int frame = 0;
	int i;
	++end;
	int frame_start = Calc_AdjustMaxFrames(0, start, fps_type);
	int max_frame = Calc_AdjustMaxFrames(start, end, fps_type);
	
	start += start;
	end += end;

	switch(mode_pattern_origin) {
		case 1:  cur_cycle = start              % cycle; break;
		case 2:  cur_cycle = (start - offset*2) % cycle; break;
		default: break;
	}
	cur_cycle += mode_pattern_shift;
	while(cur_cycle<0) {cur_cycle += cycle;}
	cur_cycle %= cycle;
	
	for(i=start; i<end; i++) {
		Map[i].start      = start;
		Map[i].frame_start = frame_start;
		Map[i].frame      = frame;
		Map[i].fps        = fps_type;
		Map[i].deint_type = 0;
		Map[i].filter_id  = -1;
		Map[i].alias_id   = 0;
		Map[i].adjust     = (mode_fps_adjust) ? 1 : 0;
		
		if(pattern[cur_cycle]==0 || frame >= max_frame) {
			Reset_Attrib(i, ATTRIB_SET|ATTRIB_COPY|ATTRIB_DELETE);
		} else {
			Set_Attrib(i, pattern[cur_cycle]);
		}
		
		if(IsAttrib(i, ATTRIB_SET|ATTRIB_COPY)) {
			frame++;
		}
		
		if(++cur_cycle >= cycle) {cur_cycle = 0;}		// ++cur_cycle %= cycle;
	}
	return 0;
}

int Its::Set_Filter(int start, int end, int fps_type, char *&p, char rightbracket) {
	char *ptr = p;
	
	while((*ptr != rightbracket) && (*ptr != LF)) { ptr++; }
	*ptr = 0;
	int rc = Check_Filter(p , true);
	if(rc>0) {
		Set_MapInfo_Sub(start, end, fps_type, rc, 0);
	}
	return rc;
}

int Its::Check_Filter(char *&name, bool add_filter) {
	int i;
	int rc = -11;
	Skip_Space(name);
	int n=strlen(name);
	while(n>0 && (name[n-1]==' ' || name[n-1]=='\t')) {
		name[n-1] = 0;
		n--;
	}
	if(n>0) {
		for(i=0; i<Filters->num_filters; i++) {
			if (!lstrcmpi(name, Filters->name[i])) { //hit!!
				rc = i+1;
				break;
			}
		}
		if(rc < 0 && add_filter) {
			if(Filters->num_filters < MAX_FILTERS) {
				strncpy(Filters->name[Filters->num_filters++], name, MAX_FILTERNAME_LENGTH);
				rc = Filters->num_filters;
			} else {
				rc = -10;
			}
		}
	}
	return rc;
}


int Its::Skip_Time(char *&p) {
	char TIME_FMT[] = { "99:99:99" };
	char *ptr=p;
	
	for(int i=0; i<(int)strlen(TIME_FMT); i++) {
		if(TIME_FMT[i]=='9') {
			if(!IsDigit(ptr)) {return -50;}
		}
		if(TIME_FMT[i]==':') {
			if(*ptr != ':') {return -50;}
		}
		ptr++;
	}
	p = ptr;
	return 0;
}

int Its::Check_Command(char *&p, char &a, char &b) {
	const char *CMDSET[] = { "tpr", "(", "<", "post", "chapter", "on_a", "on_b", "on", "off_a", "off_b", "off"
								, "dup_a", "dup_b", "dup", "null_a", "null_b", "null" };
	int rc = -101;
	int loop = 1;
	
	a = b = 0;
	Skip_Space(p);
	while(loop) {
		loop = 0;
		for(int i=0; i<sizeof(CMDSET)/sizeof(CMDSET[0]); i++) {
			int len=strlen(CMDSET[i]);
			if(_memicmp(p, CMDSET[i], len)==0) {
				p += len;
				if(i==0) {
					Skip_Space(p);
					if(*p=='<') {
						rc = i;
						p++; Skip_Space(p);
						switch(*p) {
							case '\'': p++; a = '\''; break;
							case '"' : p++; a = '"';  break;
							case '`' : p++; a = '\''; break;
							case '{' : p++; a = '}';  break;
							default  :      a = '>'; break;
						}
					}
				} else if(i<=4) {
					Skip_Space(p);
					rc= i;
				} else {
					if(*p!=' ' && *p!='\t' && *p!=',' && *p!=LF && *p!=NULL && *p!='#') {rc=-101; break;}
						Skip_Comma(p);
					rc = i;
					loop = 1;
					switch(i) {
						case 5:  case 7:  a=ATTRIB_SET; break;
						case 6:           b=ATTRIB_SET; break;
						case 8:  case 10: a=0; break;
						case 9:           b=0; break;
						case 11: case 13: a=ATTRIB_COPY; break;
						case 12:          b=ATTRIB_COPY; break;
						case 14: case 16: a=ATTRIB_DELETE; break;
						case 15:          b=ATTRIB_DELETE; break;
						default: rc=-101; loop = 0; break;
					}
					if(*p==LF || *p==NULL || *p=='#')		break;
				}
			}
		}
	}
	return rc;
}

void Its::Copy_TPRAttrib(int start, int end, TPR *Tpr) {
	int i;
	int frame = 0;
	++end;
	int frame_start = Calc_AdjustMaxFrames(0, start, Tpr->Info.fps);
	int max_frame = Calc_AdjustMaxFrames(start, end, Tpr->Info.fps);
	
	start += start;
	end += end;
	for(i=start; i<end; i++) {
		Map[i].start      = start;
		Map[i].frame_start = frame_start;
		Map[i].frame      = frame;
		Map[i].fps        = Tpr->Info.fps;
		Map[i].deint_type = Tpr->Map[i].deint_type;
		Map[i].filter_id  = MAX_FILTERS+1;
		Map[i].alias_id   = 0;
		Map[i].adjust     = (mode_fps_adjust) ? 1 : 0;
		if(Tpr->Map[i].attrib==0 || frame >= max_frame) {
			Reset_Attrib(i, ATTRIB_SET|ATTRIB_COPY);
		} else {
			Set_Attrib(i, Tpr->Map[i].attrib);
		}
		
		if(IsAttrib(i, ATTRIB_SET|ATTRIB_COPY)) {
			frame++;
		}
	}
}

int Its::Set_Command(int start, int end, int fps_type, char *&p) {
	int rc;
	char a = 0, b = 0;
	char pattern[4];
	
	Skip_Space(p);
	switch(rc=Check_Command(p, a, b)) {
		case 0://tpr
			{
				char *ptr = p;
				while((*ptr != a) && (*ptr != LF) && (*ptr != 0)) { ptr++; }
				*ptr = 0;
				if(strlen(p)>0) {
					TPR Tpr(double_num_frames);
					rc = Tpr.FileRead(p);
					if(rc<0) {
						rc -= 2000;
					} else {
						Copy_TPRAttrib(start, end, &Tpr);
						rc = 0;
					}
				}
			}
			break;
		case 1://(
			rc = (Set_Pattern1(start, end, fps_type, p)<0) ? -102 : 0;
			break;
		case 2://<
			rc = (Set_Pattern2(start, end, fps_type, p)<0) ? -103 : 0;
			break;
		case 3://post=
			rc = (Set_Post(start, end, fps_type, p)<0) ? -104 : 0;
			break;
		case 4://chapter=
			if(start==end) {
				if(*p++=='=') rc=Set_Chapter(p, start);
				else rc=ITS_ERR_CHAPTER;
			}
			break;
		default:
			if(rc>0) {
				pattern[0] = a;
				pattern[1] = b;
				Set_SelectEvery(start, end, fps_type, 2, pattern);
			}
			break;
	}
	return rc;
}

int Its::Set_Post(int start, int end, int fps_type, char *&p) {
	char bracket;
	int  len;
	int  rc = -104;
	int  i;
	char *ptr;
	
	if(*p=='=') {
		p++; Skip_Space(p);
		len = Check_Bracket(p, bracket);
		p[len] = NULL;
		if(bracket==0) {
			if(Search_Alias(p, ptr, fps_type)<0) {ptr = p;}
		} else {ptr = p;}
		if((rc=Check_Filter(ptr, true)) > 0) {
			start += start;
			++end += end;
			for(i=start; i<end; i++) { Map[i].post = rc; } // set filter_id
			Filters->request[rc-1] |= (1<<5);
		}
	}
	return rc;
}

int Its::Search_Alias(char *&p, char *&ptr, int fps_type) {
	int rc = -111;

	Skip_Space(p);
	for(int i=0; i<Alias->num_alias; i++) {
		if(_memicmp(p, Alias->alias[i], strlen(Alias->alias[i]))==0) {
			char c = p[strlen(Alias->alias[i])];
			if(Check_Delimiter(c)) {
				rc = i;
//				p += strlen(Alias->alias[i]);
//				Skip_Space(p);
				ptr = Alias->string[Fps.fps2index(fps_type)][i];
				break;
			}
		}
	}
	return rc;
}

int Its::Set_Alias(int start, int end, int fps_type, char *&p) {
	int rc;
	char *ptr;
	
	rc = Search_Alias(p, ptr, fps_type);
	if(rc>=0) {
		rc = Set_Alias_Sub(start, end, fps_type, ptr, rc+1);
	} else {
		rc = -110;
	}
	return rc;
}

int Its::Set_Alias_Sub(int start, int end, int fps_type, char *ptr, int alias_id) {
	int rc;
	int i;

	rc = Set_Command(start, end, fps_type, ptr);
	if(rc>=0) {
		// command or TPR
		start += start;
		++end += end;
		for(i=start; i<end; i++) { Map[i].alias_id = alias_id; }
	} else if(rc<0) {
		// filter
		rc = Check_Filter(ptr, true);
		if(rc>0) {
			Set_MapInfo_Sub(start, end, fps_type, rc, alias_id);
		}
	}
	return rc;
}

void Its::Set_MapInfo_Sub(int start, int end, int fps_type, int filter_id, int alias_id) {
	int frame   = 0;
	int count   = 0;
	BYTE a      = ATTRIB_SET;
	BYTE b      = (fps_type == 60) ? ATTRIB_SET : 0;
	++end;
	int frame_start = Calc_AdjustMaxFrames(0, start, fps_type);
	int max_frame = Calc_AdjustMaxFrames(start, end, fps_type);
	
	if(filter_id>0 && filter_id<=MAX_FILTERS) {
		Filters->request[filter_id - 1] |= 1;
	}

	start += start;
	end += end;

	for(int i=start; i<end; i++) {
		Map[i].start      = start;
		Map[i].filter_id  = filter_id;
		Map[i].alias_id   = alias_id;
		Map[i].fps        = fps_type;
		Map[i].deint_type = 0;
		Map[i].frame_start = frame_start;
		Map[i].frame      = frame;
		Map[i].adjust     = (mode_fps_adjust) ? 1 : 0;
		
		switch(fps_type) {
		case 24:
			if((count>>1)%5==4 || count&1 || frame>=max_frame) {
				Reset_Attrib(i, ATTRIB_SET|ATTRIB_COPY);
			} else {
				Set_Attrib(i, ATTRIB_SET);
			}
			break;
		case 25:
			if ((count >> 1) % 6 == 5 || count & 1 || frame >= max_frame) {
				Reset_Attrib(i, ATTRIB_SET | ATTRIB_COPY);
			}
			else {
				Set_Attrib(i, ATTRIB_SET);
			}
			break;
		case 48:
			if(count%5==4 || frame>=max_frame) {
				Reset_Attrib(i, ATTRIB_SET|ATTRIB_COPY);
			} else {
				Set_Attrib(i, ATTRIB_SET);
			}
			break;
		default:
			if(frame >= max_frame) {
				Reset_Attrib(i, ATTRIB_SET|ATTRIB_COPY);
			} else {
				if((count&1)==0) Set_Attrib(i, a);
				else             Set_Attrib(i, b);
			}
			break;
		}

		if(IsAttrib(i, ATTRIB_SET|ATTRIB_COPY)) {
			frame++;
		}
		++count;
	}
}

int Its::Set_Default_Alias(int start, int end, int fps_type, char *&p) {
	int rc = -120;
	int i;
	int n;
	char *ptr;
	char *str = NULL;
	
	Skip_Space(p);
	ptr=Alias->default_string[Fps.fps2index(fps_type)];
	if(Alias->default_command_type[Fps.fps2index(fps_type)]<0) {
		for(i=0; i<Alias->num_alias; i++) {
			if(_stricmp(ptr, Alias->alias[i])==0) {
				n = i;
				str = Alias->string[Fps.fps2index(fps_type)][i];
				break;
			}
		}
	} else {
		str = ptr;
		n = -1;
	}
	
	if(str!=NULL && strlen(str)) {
		rc = Set_Alias_Sub(start, end, fps_type, str, n+1);
	}
	return rc;
}


int Its::Define_Alias(char *&p) {
	char	*ptr;
	int		n,m;
	char	bracket;
	int		fps_index;
	int		i;
	
	fps_index = Fps.search2index(p, Fps.item);
	if(fps_index<0) fps_index = (Fps.priority==24) ? FPS::FPS24 : FPS::FPS30;
	
	if((ptr=strchr(p, '='))==NULL)		return -20;
	n = ptr - 1 - p;
	while(n>=0 && (p[n]==' ' || p[n]=='\t') ) { p[n--]=0; }
	if(n<=0)							return -20;
	ptr++;
	m = Check_Bracket(ptr, bracket);
	if(n<0 || m<0 || n>=MAX_ALIAS_LENGTH || m>=MAX_FILTERNAME_LENGTH)	return -20;
	
	for(i=0; i<Alias->num_alias; i++) {
		if(_stricmp(p, Alias->alias[i])==0)	break;
	}
	if(i >= MAX_ALIAS)	return -20;
	ptr[m] = NULL;
	memcpy(Alias->string[fps_index][i], ptr, m+1);
	if(i == Alias->num_alias) {
		memcpy(Alias->alias[i]  , p, n+1);
		Alias->num_alias++;
	}
	return 0;
}

int Its::Define_Default_Alias(char *&p) {
	int  fps_index;
	int  m;
	int  rc=0;
	char bracket;
	char a,b; //dummy
	
	if((fps_index=Fps.search2index(p, Fps.str))>=0) {
		if(*p=='=') {
			p++;
			if((m=Check_Bracket(p, bracket))<=0) return -30;
			if(bracket!=0) {
				Alias->default_command_type[fps_index] = -1;
			} else {
				char *ptr = p;
				int cmd_no = Check_Command(ptr, a, b);
				Alias->default_command_type[fps_index] = cmd_no;
			}
			if(m>=MAX_FILTERNAME_LENGTH) {
				return -30;
			} else {
				p[m] = NULL;
				memcpy(Alias->default_string[fps_index], p, m+1);
			}
		} else {
			return -30;
		}
	} else {
		return -30;
	}
	return rc;
}

int Its::Check_Bracket(char *&p, char &bracket) {
	int len = -1;
	
	Skip_Space(p);
	switch(*p) {
		case '`':
		case '\'':	p++; bracket='\''; break;
		case '"':	p++; bracket='"'; break;
		case '{':	p++; bracket='}'; break;
		default:	bracket=0; break;
	}
	
	if(bracket!=0) {
		char *ptr;
		if((ptr=strchr(p,bracket))!=NULL) {
			len = ptr - p;
		} else {
			return -1;
		}
	} else {
		len = 0;
		while(p[len]!=LF && p[len]!='#' && p[len]!=NULL)  { len++; }
		while(len>0 && (p[len-1]==' ' || p[len-1]=='\t')) { len--; }
	}
	return len;
}

int Its::Define_Offset(char *&p) {
	bool minus = false;
	
	Skip_Space(p);
	if(*p=='-') {
		p++; Skip_Space(p);
		minus = true;
	}
	offset = GetDigit(p);
	if(offset<0) return -40;
	if(minus)	offset = -offset;
	return 0;
}

#define MODE_PATTERN_ORIGIN		"pattern_origin"
#define MODE_PATTERN_SHIFT		"pattern_shift"
#define MODE_FPS_PRIORITY		"fps_priority"
#define MODE_FPS_ADJUST			"fps_adjust"
#define MODE_KEYFRAMES_GROUPING	"keyframes_grouping"
#define MODE_CHAPTER_LANGUAGE	"chapter_language"
#define MODE_CHAPTER_CHARSET	"chapter_charset"

int Its::Define_Mode(char *&p) {
	int rc=-50;
	char bracket;
	int m;
	int i;
	const char *Mode_Cmd[] = {
								 MODE_PATTERN_ORIGIN,
								 MODE_PATTERN_SHIFT,
								 MODE_FPS_PRIORITY,
								 MODE_FPS_ADJUST,
								 MODE_KEYFRAMES_GROUPING,
								 MODE_CHAPTER_LANGUAGE,
								 MODE_CHAPTER_CHARSET,
							};
	
	Skip_Space(p);
	for(i=0; i<sizeof(Mode_Cmd)/sizeof(Mode_Cmd[0]); i++) {
		if(!_memicmp(p, Mode_Cmd[i], strlen(Mode_Cmd[i]))) {
			p += strlen(Mode_Cmd[i]);
			Skip_Space(p);
			if(*p++=='=') {
				Skip_Space(p);
				switch(i) {
					case 0: //pattern_origin
						if(!_memicmp(p, "current", 7)) {p+=7; mode_pattern_origin = 0; rc=0;}
						if(!_memicmp(p, "top"    , 3)) {p+=3; mode_pattern_origin = 1; rc=0;}
						if(!_memicmp(p, "offset" , 6)) {p+=6; mode_pattern_origin = 2; rc=0;}
						break;
					case 1: //pattern_shift
						if(*p=='-') {p++;Skip_Space(p); if((mode_pattern_shift=-GetDigit(p))<=0) rc=0;}
						else        {                   if((mode_pattern_shift= GetDigit(p))>=0) rc=0;}
						break;
					case 2: //fps_priority
						Fps.priority = Fps.search2fps(p, Fps.str);
						if(Fps.priority>0) rc=0;
						break;
					case 3: //fps_adjust
						if(!_memicmp(p, "on",  2)) {p+=2; mode_fps_adjust = true; rc=0;}
						if(!_memicmp(p, "off", 3)) {p+=3; mode_fps_adjust = false; rc=0;}
						break;
					case 4: //keyframes_grouping
						if(!_memicmp(p, "on",  2)) {p+=2; mode_keyframes_grouping = true; rc=0;}
						if(!_memicmp(p, "off", 3)) {p+=3; mode_keyframes_grouping = false; rc=0;}
						break;
					case 5: //chapter_language
						m=Check_Bracket(p, bracket);
						if(m<0 || m>=16) rc = ITS_ERR_CHAPTER_MODE_TOO_LENGTH;
						else {
							rc = 0;
							p[m] = 0;
							memcpy(Chapters->language, p, m+1);
							p += m;
						}
						break;
					case 6: //chapter_charset
						m=Check_Bracket(p, bracket);
						if(m<0 || m>=16) rc = ITS_ERR_CHAPTER_MODE_TOO_LENGTH;
						else {
							rc = 0;
							p[m] = 0;
							memcpy(Chapters->charset, p, m+1);
							p += m;
						}
						break;
				}
				if(rc==0) {
					if(!Check_Delimiter(p)) {rc=-50;}
				}
			}
			break;
		}
	}
	return rc;
}


int Its::Define_KeyFrames_Enter(char *&p) {
	Skip_Space(p);
	switch(*p) {
		case '{': in_keyframes_flag = true; return 0;
		default: break;
	}
	return ITS_ERR_KEYFRAMES;
}

int Its::Set_KeyFrames(char *&p) {
	int start_frame;

	if (*p=='}') {
		in_keyframes_flag = false;
		return 0;
	}
	start_frame=GetDigit(p);
	if(start_frame < 0 || start_frame >= org_num_frames) return ITS_ERR_KEYFRAMES;
	Set_Attrib(start_frame * 2, ATTRIB_KEYFRAME);
	switch(*p) {
	case '}':
		in_keyframes_flag = false;
		break;
	case 0:
	case LF:
	case '#':
		if(!Check_Comment(p)) return ITS_ERR_KEYFRAMES;
		break;
	default:
		return Set_Chapter(p, start_frame);
	}
	return 0;
}

int Its::Set_Chapter(char *&p, int start_frame) {
	int m;
	int i;
	char bracket;
	LONG frame = (LONG)start_frame * 2;

	for(i=Chapters->num_chapters-1; i>=0; i--) {
		if(frame > Chapters->c[i].frame) break;
	}
	m=Check_Bracket(p, bracket);
	if(m<0 || m>=MAX_CHAPTER_LENGTH) return ITS_ERR_CHAPTER;
	p[m] = 0;
	i++;
	if(Chapters->num_chapters>0 && Chapters->c[i].frame==frame) {
		memcpy(Chapters->c[i].text, p, m+1);
	} else {
		if(Chapters->num_chapters >= MAX_CHAPTERS) return ITS_ERR_CHAPTERS_OVER;
		if(i<Chapters->num_chapters) {
			memmove( (void*)&Chapters->c[i+1], (void*)&Chapters->c[i], sizeof(CHAPTER)*(Chapters->num_chapters-i));
		}
		memcpy(Chapters->c[i].text, p, m+1);
		Chapters->c[i].frame = frame;
		Set_Attrib(frame, ATTRIB_CHAPTER);
		++Chapters->num_chapters;
	}
	return 0;
}
