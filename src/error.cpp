#include "its.h"

void	Its::OutputError(IScriptEnvironment*env, int rc, const char*msg1, const char*msg2, const char*msg3)
{

	if(rc>=-2000 && rc<-3000) { //TMPGEnc project file error
		if(rc==-2003)	env->ThrowError("%s: .tpr read error(\"%s%s\":line %d)", GetName(), msg1, msg2, err_lineno);
		if(rc==-2002)	env->ThrowError("%s: .tpr open error(\"%s%s\":line %d)", GetName(), msg1, msg2, err_lineno);
		if(rc==-2001)	env->ThrowError("%s: not enough memory", GetName());
		env->ThrowError("%s: .tpr format error(%d).", GetName(), rc);
	}

	if(rc==ITS_ERR_ADJUST)		env->ThrowError("%s: can't fps adjust.(frame %d)", GetName(), err_frameno);

	if(rc==ITS_ERR_KEYFRAMES)
		env->ThrowError("%s: keyframes error.(\"%s%s\":line %d)", GetName(), msg1, msg2, err_lineno);
	if(rc==ITS_ERR_CHAPTERS_OVER)
		env->ThrowError("%s: chapters too many.(\"%s%s\":line %d)", GetName(), msg1, msg2, err_lineno);
	if(rc==ITS_ERR_CHAPTER)
		env->ThrowError("%s: chapter error.(\"%s%s\":line %d)", GetName(), msg1, msg2, err_lineno);
	if(rc==ITS_ERR_CHAPTER_OPENFILE)
		env->ThrowError("%s: chapter file open error.(\"%s%s\")", GetName(), msg1, msg2);
	if(rc==ITS_ERR_CHAPTER_WRITE)
		env->ThrowError("%s: chapter write error.(\"%s%s\")", GetName(), msg1, msg2);
	if(rc==ITS_ERR_CHAPTER_MODE_TOO_LENGTH)
		env->ThrowError("%s: chapter mode too length.(\"%s%s\":line %d)", GetName(), msg1, msg2, err_lineno);

	if(rc==ITS_ERR_TIMECODES_OPENFILE)	env->ThrowError("%s: timecodes file open error.", GetName());
	if(rc==ITS_ERR_TIMECODES_WRITE)		env->ThrowError("%s: timecodes write error.", GetName());

	if(rc==ITS_ERR_FRAME_NO)
		env->ThrowError("%s: invalid frame-no.(\"%s%s\":line %d)",GetName(),msg1,msg2,err_lineno);
	if(rc==ITS_ERR_FPS)
		env->ThrowError("%s: [fps] error.(\"%s%s\":line %d)",GetName(),msg1,msg2,err_lineno);

	if(rc<=-20)	env->ThrowError("%s: format error(\"%s%s\":line %d, err=%d)", GetName(), msg1, msg2, err_lineno, rc);
	if(rc==-11)	env->ThrowError("%s: not found filter alias name.(\"%s%s\":line %d)", GetName(),msg1,msg2,err_lineno);
	if(rc==-10)	env->ThrowError("%s: too many filter(\"%s%s\":line %d)", GetName(), msg1, msg2, err_lineno);
	if(rc==-3)	env->ThrowError("%s: read error(\"%s%s\":line %d)", GetName(), msg1, msg2, err_lineno);
	if(rc==-2)	env->ThrowError("%s: open error(%s)", GetName(), Path->path);
	if(rc==-1)	env->ThrowError("%s: not enough memory", GetName());
	if(rc<0)	env->ThrowError("%s: error(\"%s%s\":line %d, err=%d)", GetName(), msg1, msg2, err_lineno, rc);

}

