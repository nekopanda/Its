#ifndef tpr_h
#define tpr_h

//--- for TMPGEnc (binary/text) project file ---

#define TPR_READ_BUF_SIZE			256

#define TPR_TEXT_ID			"object TMPEGEncodeJobFile"
#define TPR_PATH_EXT		".tpr"

#define TPR_DEINT_ID		"Video.DeinterlaceEx.List"
#define TPR_TFF_ID			"Video.TopFieldFirst"
#define TPR_SUBRANGE_ID		"Video.SourceRange.Enabled"
#define TPR_FRAMERATE1_ID	"Video.DeinterlaceEx.FrameRate1"
#define TPR_FRAMERATE2_ID	"Video.DeinterlaceEx.FrameRate2"


typedef struct {
	int			num_frames;
	int			framerate[2];
	int			fps;
	bool		tff;
} TPR_INFO;

#pragma pack(1)
typedef struct {
	char			frame[8];
	char			deint_type[2];
	char			copy_frame[2];
} TPR_TEXT_DATA;

typedef struct {
	char			unknown[8];
	char			num_frames[8];
	TPR_TEXT_DATA	data[1];
} TPR_TEXT_HDR;

typedef struct {
	int			frame;
	char		deint_type;
	char		copy_frame;
} TPR_BIN_DATA;

typedef struct {
	BYTE			unknown[4];
	DWORD			num_frames;
} TPR_BIN_HDR;

#pragma pack()
//-----------------------------------


class TPR {
public:
	int				double_num_frames;
	TPR_INFO		Info;
	struct _MAP {
		char	attrib; // 0:none  1:set 2:copy -1:delete
		char	deint_type;
	} *Map;

	TPR(int num_frames) : double_num_frames(num_frames) { Map = new struct _MAP [double_num_frames]; }
	~TPR() { delete [] Map; }
	
	int		FileRead(const char *filepath);
	int		Set_TPR(TPR_TEXT_DATA *&p, int &c);
	int		GetDigit(char *p);
	int		GetHexByte(char *p);
	int		GetHexDword(char *p);
	int		FREAD(char *buf, size_t size, size_t count, FILE *fh);
};

#endif //tpr_h
