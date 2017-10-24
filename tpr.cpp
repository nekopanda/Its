#include <windows.h>
#include <stdio.h>
#include "common.h"
#include "tpr.h"

int TPR::GetDigit(char *p) {
	int n=0;
	if(*p<'0' || *p>'9')	return -1;
	while(*p>='0' && *p<='9') {
		n = n*10 + *p++ - '0';
	}
	return n;
}

int TPR::GetHexByte(char *p) {
	int n=0;
	for(int i=0; i<2; i++) {
		int d = *p++;
		if(d>='0' && d<='9') { d -= '0'; }
		else if(d>='A' && d<='F') { d -= 'A'-10; }
		else if(d>='a' && d<='f') { d -= 'a'-10; }
		else { n=-1; break; }
		n = n*16 + d;
	}
	return n;
}

int TPR::GetHexDword(char *p) {
	int n=0;
	for(int i=0; i<4; i++) {
		int d=GetHexByte(p);
		if (d<0) { n=-1; break;}
		n += (d * (1<<(8*i)));
		p+=2;
	}
	return n;
}

int TPR::FREAD(char *buf, size_t size, size_t count, FILE *fh) {
	int len = fread(buf, size, count, fh);
	if(len < (int)count) {
		if(feof(fh))	return 0;
		else			return -3;
	}
	return len;
}


int TPR::FileRead(const char *filepath) {
	FILE	*fh;
	char	*buf, *ptr;
	bool	TPR_binary;
	int		rc = 0;
	int		length;
	int		i, n;
	const char	*TPR_ID[] = { TPR_DEINT_ID, TPR_TFF_ID, TPR_SUBRANGE_ID, TPR_FRAMERATE1_ID, TPR_FRAMERATE2_ID };
	
	if( (buf = new char [TPR_READ_BUF_SIZE*2]) == NULL)		return -1;

	if((fh=fopen(filepath, "rb")) == NULL)				return -2;
	
	if(FREAD(buf, sizeof(BYTE), 0x11, fh)>0) {
		if(*(BYTE*)buf == 0xff) {
			TPR_binary = true;
		} else {
			TPR_binary = false;
			fclose(fh);
			if((fh=fopen(filepath, "r")) == NULL)	rc=-2;
		}
	} else {
		rc=-3;
	}
	
	if(!rc) {
		if(TPR_binary) {
			//--- binary tpr file ---
			size_t len;
			while(!feof(fh)) {
				len = FREAD(buf, sizeof(BYTE), 1, fh);
				if(len<0)																	{rc=-3; break;}
				if(len==0)																	{rc=1; break;}
				length = MIN(*(BYTE*)buf, TPR_READ_BUF_SIZE-1);
				if(length==0)		continue;
				if((len=FREAD(buf, sizeof(BYTE), length, fh))<=0)							{rc=-3; break;}
				buf[length]=0;
				DebugMsg("%d <%s>", length, buf);
				
				n = -1;
				for(i=0; i<sizeof(TPR_ID)/sizeof(TPR_ID[0]); i++) {
					if (!_memicmp(buf, TPR_ID[i], strlen(TPR_ID[i]))) { //hit!!
						n = i;
						break;
					}
				}
				if((len=FREAD(buf, sizeof(BYTE), 1, fh))<=0)								{rc=-3; break;}
				switch(*(BYTE*)buf) {
				case 0x00:
					DebugMsg("type - 00");
					break;
				case 0x02: // BYTE
					if((len=FREAD(buf, sizeof(BYTE), 1, fh))<=0)							{rc=-100; break;}
					switch(n) {
						case 3: Info.framerate[0] = *buf; break;
						case 4: Info.framerate[1] = *buf;
								Info.fps = 30 * Info.framerate[0] / Info.framerate[1];
								break;
						default: DebugMsg("type - 02 BYTE"); break;
					}
					break;
				case 0x03: // WORD
					if(fseek(fh, 2, SEEK_CUR))												{rc=-101; break;}
					DebugMsg("type - 03 WORD");
					break;
				case 0x04: // DWORD
					if(fseek(fh, 4, SEEK_CUR))												{rc=-102; break;}
					DebugMsg("type - 04 DWORD");
					break;
				case 0x05: // ???
					if(fseek(fh, 10, SEEK_CUR))												{rc=-103; break;}
					DebugMsg("type - 05 10bytes ???");
					break;
				case 0x06: // String
				case 0x07: // Literal
					if((len=FREAD(buf, sizeof(BYTE), 1, fh))<=0)							{rc=-3; break;}
					length=*(BYTE*)buf;
					if(fseek(fh, length, SEEK_CUR))											{rc=-104; break;}
					DebugMsg("type - 06: %d", length);
					break;
				case 0x08: // False
				case 0x09: // True
					if(n==1) { // TPR_TFF_ID
						Info.tff = (*(BYTE*)buf==0x09) ? true : false;
					}
					if(n==2) { // TPR_SUBRANGE_ID
					}
					DebugMsg("type - %02x Bool", *(BYTE*)buf);
					break;
				case 0x0A: // Lists
					if((len=FREAD(buf, sizeof(BYTE), 4, fh))<=0)							{rc=-3; break;}
					length=*(int *)buf;
					if(n<0) {
						if(fseek(fh, length, SEEK_CUR))										{rc=-105; break;}
						DebugMsg("type - 0A: %d", length);
					} else {
						// Dint list
						if((len=FREAD(buf, sizeof(BYTE), sizeof(TPR_BIN_HDR), fh))<=0)		{rc=-3; break;}
						length -= len;
						TPR_BIN_HDR		*tpr_hdr = (TPR_BIN_HDR*)buf;
						Info.num_frames = tpr_hdr->num_frames;
						DebugMsg("type - 0A: %d  <Deint list!!!>", length);
						if(length/(int)sizeof(TPR_BIN_DATA) != Info.num_frames)			{rc=-106; break;}
						for(i=0; i<Info.num_frames; i++) {
							if((len=FREAD(buf, sizeof(BYTE), sizeof(TPR_BIN_DATA), fh))<=0)	{rc=-3; break;}
							TPR_BIN_DATA *tpr_data = (TPR_BIN_DATA*)buf;
							int frame = tpr_data->frame;
							if(frame<0)														{rc=-107; break;}
							if(frame<double_num_frames) {
								Map[frame].attrib = (tpr_data->copy_frame==0) ? (char)1 : -1;
								Map[frame].deint_type = tpr_data->deint_type;
							}
						}
					}
					break;
				case 0x0E: // ???
					if(fseek(fh, 1, SEEK_CUR))		{rc=-107; break;}
					DebugMsg("type - 0E: %d", length);
					break;
				default:
					DebugMsg("type - unknown", *buf);
					rc=-999;
					break;
				}
				if(rc<0) break;
			}
		} else {
			//--- text tpr file ---
			while(!feof(fh)) {
				if( fgets(buf, TPR_READ_BUF_SIZE, fh) == NULL) {
					rc = feof(fh) ? 1 : -3;
					break;
				}
				ptr = buf;
				Skip_Space(ptr);
				n = -1;
				for(i=0; i<sizeof(TPR_ID)/sizeof(TPR_ID[0]); i++) {
					if (!_memicmp(ptr, TPR_ID[i], strlen(TPR_ID[i]))) { //hit!!
						n = i;
						break;
					}
				}
				switch(n) {
				case 0:
					// Deint list
					{
						ptr += strlen(TPR_ID[n]);
						Skip_Space(ptr);
						if(*ptr++ != '=') break;
						Skip_Space(ptr);
						if(*ptr++ != '{') break;
						Skip_Space(ptr);
						
						if(*ptr == LF) {
							if( fgets(buf, TPR_READ_BUF_SIZE, fh) == NULL) {rc=-3; break;}
							ptr = buf;
							Skip_Space(ptr);
						}
						if(rc) break;
						TPR_TEXT_HDR	*tpr_hdr = (TPR_TEXT_HDR *)ptr;
						TPR_TEXT_DATA	*tpr_data = (TPR_TEXT_DATA *)tpr_hdr->data;
						Info.num_frames = GetHexDword(tpr_hdr->num_frames);
						int				carry;
						while(!(rc=Set_TPR(tpr_data, carry))) {}
						while(!feof(fh) && rc==2) { // LF
							if(carry) memcpy(buf, (char*)tpr_data, carry);
							if( (fgets(buf+TPR_READ_BUF_SIZE, TPR_READ_BUF_SIZE, fh)) == NULL) {rc=-3; break;}
							tpr_data = (TPR_TEXT_DATA *)(buf+TPR_READ_BUF_SIZE);
							Skip_Space(tpr_data);
							memcpy(buf+carry, tpr_data, strlen((char*)tpr_data)+1);
							tpr_data = (TPR_TEXT_DATA *)buf;
							while(!(rc=Set_TPR(tpr_data, carry))) {}
						}
					}
					break;
				case 1:
					// TFF
					ptr += strlen(TPR_ID[n]);
					Skip_Space(ptr);
					if(*ptr++ != '=') break;
					Skip_Space(ptr);
					Info.tff = (!_memicmp(ptr, "True", 4)) ? true : false;
					break;
				case 2:
					break;
				case 3: //FRAMERATE1
				case 4: //FRAMERATE2
					ptr += strlen(TPR_ID[n]);
					Skip_Space(ptr);
					if(*ptr++ != '=') break;
					Skip_Space(ptr);
					Info.framerate[n-3] = GetDigit(ptr);
					Info.fps = 30 * Info.framerate[0] / Info.framerate[1];
					break;
				default:
					break;
				}
				if(rc<0) break;
			}
		}
	}
	fclose(fh);
	delete [] buf;
	return rc;
}




int TPR::Set_TPR(TPR_TEXT_DATA *&p, int &c) {
	int rc = 0;
	while(*(char *)p!='}' && *(char *)p!=LF) {
		if((c=strlen((char*)p)-1)<sizeof(TPR_TEXT_DATA))	{rc=2; break;}
		c=0;
		int frame = GetHexDword(p->frame);
		int type = GetHexByte(p->deint_type);
		int copy = GetHexByte(p->copy_frame);
		if(frame<0 || type<0 || copy<0)			{rc=-10; break;}
		if(frame>=double_num_frames)			{rc=1; break;}
		Map[frame].attrib = (copy==0) ? (char)1 : 2;
		Map[frame].deint_type = type;
		p++;
	}
	if(*(char *)p == '}')	rc=1;
	if(*(char *)p == LF)	rc=2;
	return rc;
}

