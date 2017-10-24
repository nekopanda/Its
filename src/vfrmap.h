//
// vfrmap.h
//
#ifndef vfrmap_h
#define vfrmap_h

#include <windows.h>
#include <stdio.h>
#include <process.h>

class VfrMap;
class VfrOut;
class VfrMap2;
class VfrOut2;

// フィルタプラグイン用(itvfr)
class VfrMap
{
	HANDLE	m_hMemMap;
	char	*m_MemView;
	int		m_frame_offset;

public:
	VfrMap(int frames) {
		char MemFile[100];
		int	size = sizeof(char) * (frames+10);

		_snprintf(MemFile,100,"vfrout_%d",_getpid());
		m_hMemMap = CreateFileMapping((HANDLE)0xffffffff,NULL,PAGE_READWRITE,0,size,MemFile);
		m_MemView = (char*) MapViewOfFile(m_hMemMap,FILE_MAP_ALL_ACCESS,0,0,0);
		ZeroMemory(m_MemView,size);
		m_frame_offset = 0;
	}

//------------------------------------------------------------------- begin : kiraru2002
	void SetStatus(int frame,int fps) {
		if(fps == 60) m_MemView[frame - m_frame_offset] = '6';
		if(fps == 30) m_MemView[frame - m_frame_offset] = '3';
		if(fps == 24) m_MemView[frame - m_frame_offset] = '2';
		if(fps == 0)  m_MemView[frame - m_frame_offset] = 'D';
		if((BYTE)fps >= 128) m_MemView[frame - m_frame_offset] = (BYTE)fps; //0x81-0x8f:put(n)
	}
//------------------------------------------------------------------- end   : kiraru2002

	char GetStatus(int frame) {
		return m_MemView[frame - m_frame_offset];
	}

	void SetOffset(int offset) {
		m_frame_offset = offset;
	}

	~VfrMap(void) {
		UnmapViewOfFile(m_MemView);
		CloseHandle(m_hMemMap);
	}
};


// 出力プラグイン用
class VfrOut
{
	HANDLE	m_hMemMap;
	char	*m_MemView;

public:
	VfrOut(void) {
		char MemFile[100];
		_snprintf(MemFile,100,"vfrout_%d",_getpid());
		m_hMemMap = OpenFileMapping(FILE_MAP_ALL_ACCESS,false,MemFile);
		if(m_hMemMap == NULL) return;
		m_MemView = (char*) MapViewOfFile(m_hMemMap,FILE_MAP_ALL_ACCESS,0,0,0);
	}

	bool Err(void) {
		return m_hMemMap == NULL ? true : false;
	}

	bool IsDelete(int frame) {
		return m_MemView[frame] == 'D' ? true : false;
	}

	bool IsFps24(int frame) {
		return m_MemView[frame] == '2' ? true : false;
	}

//------------------------------------------------------------------- begin : kiraru2002
	char GetStatus(int frame) {
		return m_MemView[frame];
	}
	
	void SetStatus(int frame,int fps) {
		if(fps == 60) m_MemView[frame] = '6';
		if(fps == 30) m_MemView[frame] = '3';
		if(fps == 24) m_MemView[frame] = '2';
		if(fps == 0)  m_MemView[frame] = 'D';
		if((BYTE)fps >= 128) m_MemView[frame] = (BYTE)fps; //0x81-0x8f:put(n) 0x91-0x9f:dup(n)
		if(fps == -1) m_MemView[frame] = 0;
	}
	
	bool IsFps60(int frame) {
		return m_MemView[frame] == '6' ? true : false;
	}
	bool IsFps30(int frame) {
		return m_MemView[frame] == '3' ? true : false;
	}
//------------------------------------------------------------------- end   : kiraru2002

	virtual ~VfrOut(void) {
		if(m_hMemMap) {
			UnmapViewOfFile(m_MemView);
			CloseHandle(m_hMemMap);
		}
	}
};

//------------------------------------------------------------------- begin : kiraru2002

#define VFRINFO_MUTEX_HEADER_NAME "exavi_vfr_vfrinfo_header___%d"
#define VFRINFO_MUTEX_FRAME_NAME  "exavi_vfr_vfrinfo_frame___%d"

typedef struct _VFR_FRAME_INFO {
	BYTE    count;
	BYTE    flag;
	BYTE    rsv[2];
} VFR_INFO_FRAME,*PVFR_INFO_FRAME;

typedef struct _VFR_INFO {
	LONG    version;
	LONG    subversion;
	LONG    length;
	LONG    frames;
	BYTE    denominator;
	BYTE    rsv[15];
	BYTE    reserve[512-32];
} VFR_INFO_HEADER, *PVFR_INFO_HEADER;

typedef struct _VFR_INFO_EXT {
	LONG    id;
	LONG    length;
	BYTE    data[1];
} VFR_INFO_EXT, *PVFR_INFO_EXT;

#define VFR_INFO_FLAG_COPY     2
#define VFR_INFO_FLAG_KEYFRAME 4

class VfrInfoMap
{
	HANDLE    m_hMemMap;
	VFR_INFO_HEADER  *m_vihp;
	VFR_INFO_FRAME   *m_vifp;
	HANDLE            m_hMutexHeader;
	HANDLE            m_hMutexFrame;

public:
	VfrInfoMap(int frames, int ext) : m_hMemMap(NULL), m_vihp(NULL), m_hMutexHeader(NULL), m_hMutexFrame(NULL) {
		char Work[100];
		int	size = sizeof(VFR_INFO_HEADER)+sizeof(VFR_INFO_FRAME)*frames+ext;

		_snprintf(Work,100,"exavi_vfrinfo_%d",_getpid());
		m_hMemMap = CreateFileMapping((HANDLE)0xffffffff,NULL,PAGE_READWRITE,0,size,Work);
		if(m_hMemMap==NULL) return;
		m_vihp = (PVFR_INFO_HEADER)MapViewOfFile(m_hMemMap,FILE_MAP_ALL_ACCESS,0,0,0);
		if(m_vihp==NULL) return;
		ZeroMemory(m_vihp, size);
		m_vihp->version = 1;
		m_vihp->denominator = 1;
		m_vihp->length = size;;
		m_vihp->frames = frames;
		m_vifp = (PVFR_INFO_FRAME)((BYTE*)m_vihp+sizeof(VFR_INFO_HEADER));
		_snprintf(Work,100,VFRINFO_MUTEX_HEADER_NAME,_getpid());
		m_hMutexHeader = CreateMutex(NULL, false, Work);
		_snprintf(Work,100,VFRINFO_MUTEX_FRAME_NAME,_getpid());
		m_hMutexFrame  = CreateMutex(NULL, false, Work);
	}
	virtual ~VfrInfoMap(void) {
		if(m_vihp) UnmapViewOfFile(m_vihp);
		if(m_hMemMap) CloseHandle(m_hMemMap);
		if(m_hMutexHeader) CloseHandle(m_hMutexHeader);
		if(m_hMutexFrame) CloseHandle(m_hMutexFrame);
	}
	bool Err() { return m_hMemMap==NULL || m_vihp==NULL || m_hMutexHeader==NULL || m_hMutexFrame==NULL; }
	DWORD Enter(int type, DWORD ms) {
		return WaitForSingleObject((type==0)?m_hMutexHeader:m_hMutexFrame, ms);
	}
	void Release(int type) {
		ReleaseMutex((type==0)?m_hMutexHeader:m_hMutexFrame);
	}
	void SetVersion(LONG ver) { m_vihp->version = ver; }
	void SetSubVersion(LONG ver) { m_vihp->subversion = ver; }
	void SetLength(LONG len) { m_vihp->length = len; }
	void SetNumFrames(LONG n) { m_vihp->frames = n; }
	void SetDenominator(BYTE n) { m_vihp->denominator = n; }
	void SetCount(LONG n, BYTE cnt) {
		m_vifp[n].count = cnt;
	}
	void SetFlag(LONG n, BYTE f) {
		m_vifp[n].flag |= f;
	}
	void FlipFlag(LONG n, BYTE f) {
		m_vifp[n].flag ^= f;
	}
	void ClearFlag(LONG n, BYTE f) {
		m_vifp[n].flag &= ~f;
	}
	void CopyFlag(LONG n, BYTE f) {
		m_vifp[n].flag = f;
	}
	LONG GetVersion() const { return m_vihp->version; }
	LONG GetSubVersion() const { return m_vihp->subversion; }
	LONG GetLength() const { return m_vihp->length; }
	LONG GetNumFrames() const { return m_vihp->frames; }
	BYTE GetDenominator() const {
		 return  m_vihp->denominator;
	}
	BYTE GetCount(LONG n) const {
		 return  m_vifp[n].count;
	}
	BYTE GetFlag(LONG n) const {
		 return  m_vifp[n].flag;
	}
};

class VfrInfoOut
{
	HANDLE    m_hMemMap;
	VFR_INFO_HEADER *m_vihp;
	VFR_INFO_FRAME  *m_vifp;
	bool      m_writable;

public:
	VfrInfoOut() : m_hMemMap(NULL), m_vihp(NULL), m_writable(false)  {
		char Work[100];
		_snprintf(Work,100,"exavi_vfrinfo_%d",_getpid());
		m_hMemMap = OpenFileMapping(FILE_MAP_ALL_ACCESS,false,Work);
		if(m_hMemMap == NULL) return;
		m_vihp = (PVFR_INFO_HEADER) MapViewOfFile(m_hMemMap,FILE_MAP_ALL_ACCESS,0,0,0);
		m_vifp = (PVFR_INFO_FRAME)((BYTE*)m_vihp+sizeof(VFR_INFO_HEADER));
	}
	virtual ~VfrInfoOut(void) {
		if(m_vihp) UnmapViewOfFile(m_vihp);
		if(m_hMemMap) CloseHandle(m_hMemMap);
	}
	bool Err() { return m_hMemMap==NULL || m_vihp==NULL; }
	bool IsWritable() const { return m_writable; }
	void MakeWritable() { m_writable = true; }
	void MakeNoWritable() { m_writable = false; }
	void OpenMutexHeader(HANDLE &hMutex) {
		char Work[100];
		_snprintf(Work,100,VFRINFO_MUTEX_HEADER_NAME,_getpid());
		hMutex = OpenMutex(MUTEX_ALL_ACCESS, true, Work);
	}
	void OpenMutexFrame(HANDLE &hMutex) {
		char Work[100];
		_snprintf(Work,100,VFRINFO_MUTEX_FRAME_NAME,_getpid());
		hMutex = OpenMutex(MUTEX_ALL_ACCESS, true, Work);
	}
	void Release(HANDLE &hMutex) {
		ReleaseMutex(hMutex);
	}
	DWORD Enter(HANDLE &hMutex, DWORD ms) {
		return WaitForSingleObject(hMutex, ms);
	}
	bool SetVersion(LONG ver) {if(m_writable) {m_vihp->version = ver; return true;} else return false;}
	bool SetSubVersion(LONG ver) {if(m_writable) {m_vihp->subversion = ver; return true;} else return false; }
	bool SetLength(LONG len) {if(m_writable) {m_vihp->length = len; return true;} else return false; }
	bool SetFrames(LONG n) { if(m_writable) {m_vihp->frames = n; return true;} else return false; }
	bool SetDenominator(BYTE n) { if(m_writable) {m_vihp->denominator = n; return true;} else return false; }
	bool SetCount(LONG n, BYTE cnt) {
		if(m_writable) {m_vifp[n].count = cnt; return true;} else return false;
	}
	bool SetFlag(LONG n, BYTE f) {
		if(m_writable) {m_vifp[n].flag |= f; return true;} else return false;
	}
	bool FlipFlag(LONG n, BYTE f) {
		if(m_writable) {m_vifp[n].flag ^= f; return true;} else return false;
	}
	bool ClearFlag(LONG n, BYTE f) {
		if(m_writable) {m_vifp[n].flag &= ~f; return true;} else return false;
	}
	bool CopyFlag(LONG n, BYTE f) {
		if(m_writable) {m_vifp[n].flag = f; return true;} else return false;
	}
	LONG GetVersion() const { return m_vihp->version; }
	LONG GetSubVersion() const { return m_vihp->subversion; }
	LONG GetLength() const { return m_vihp->length; }
	LONG GetNumFrames() const { return m_vihp->frames; }
	BYTE GetDenominator() const {
		 return  m_vihp->denominator;
	}
	BYTE GetCount(LONG n) const {
		 return  m_vifp[n].count;
	}
	BYTE GetFlag(LONG n) const {
		 return  m_vifp[n].flag;
	}
};

//------------------------------------------------------------------- end   : kiraru2002

#endif //vfrmap_h
