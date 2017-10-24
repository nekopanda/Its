NAME    = Its
INDIR   = .
OUTDIR  = ./release
TARGET  = $(NAME).dll
INCLS   = \
          ../include/avisynth.h
OBJS    = \
          "$(OUTDIR)/its_init.obj"		\
          "$(OUTDIR)/its_frame.obj"		\
          "$(OUTDIR)/file.obj"			\
          "$(OUTDIR)/tpr.obj"			\
          "$(OUTDIR)/error.obj"			\
          "$(OUTDIR)/drawfont.obj"		\
          "$(OUTDIR)/debugmsg.obj"
RESFILE = "$(OUTDIR)/$(NAME).res"
DEFFILE = 
NAME2    = NullSkip
TARGET2  = $(NAME2).dll
OBJS2    = "$(OUTDIR)/$(NAME2).obj" "$(OUTDIR)/debugmsg.obj"
RESFILE2 =

#-----------------------
CC   = cl
LINK = link
RC   = rc
ASM  = ml

LIBS = kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib \
       advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib
COPT   = /nologo /c /W3 /G6 /GX /O2 /Oi /FAcs /Fa"$(OUTDIR)/" \
         /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fo"$(OUTDIR)/"
LOPT   = /DLL /nologo /OPT:NOWIN98 /SUBSYSTEM:WINDOWS /machine:I386 \
         /implib:$*.lib /out:$@
ASMOPT = /c /coff /Fl"$(OUTDIR)/" /Sa /Fo"$(OUTDIR)/"
RCOPT  = /r /fo$(RESFILE)

#-----------------------
ALL:"$(OUTDIR)/$(TARGET)" "$(OUTDIR)/$(TARGET2)"

"$(OUTDIR)/$(TARGET)": "$(OUTDIR)" $(OBJS) $(RESFILE) $(DEFFILE)
	$(LINK) $(LOPT) @<<
        $(OBJS)
        $(LIBS)
        $(RESFILE)
<<

"$(OUTDIR)/$(TARGET2)": "$(OUTDIR)" $(OBJS2) $(RESFILE2)
	$(LINK) $(LOPT) @<<
        $(OBJS2)
        $(LIBS)
        $(RESFILE2)
<<

$(OBJS): $(INCLS)
###$(RESFILE): resource.h

"$(OUTDIR)":
	if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(OUTDIR)/its_init.obj":	"$(INDIR)/its.h" "$(INDIR)/ItsCut.h"
"$(OUTDIR)/its_frame.obj":	"$(INDIR)/its.h"
"$(OUTDIR)/file.obj":		"$(INDIR)/its.h"
"$(OUTDIR)/tpr.obj":		"$(INDIR)/common.h" "$(INDIR)/tpr.h"
"$(OUTDIR)/error.obj":		"$(INDIR)/its.h"
"$(OUTDIR)/drawfont.obj":	"$(INDIR)/drawfont.h"
"$(OUTDIR)/debugmsg.obj":	"$(INDIR)/common.h"
"$(INDIR)/its.h":			"$(INDIR)/tpr.h" "$(INDIR)/vfrmap.h" "$(INDIR)/drawfont.h"
                            "$(INDIR)/common.h"  "$(INDIR)/ItsCut.h"

"$(OUTDIR)/$(NAME2).obj":	"$(INDIR)/vfrmap.h" "$(INDIR)/common.h"

#-----------------------
clean:
#	-@erase "$(OUTDIR)\$(TARGET)"
	-@erase "$(OUTDIR)\*.lib"
	-@erase "$(OUTDIR)\*.exp"
	-@erase "$(OUTDIR)\*.obj"
	-@erase "$(OUTDIR)\*.cod" 2>nul
	-@erase "$(OUTDIR)\*.lst" 2>nul
	-@erase "$(OUTDIR)\*.res" 2>nul

#-----------------------
.SUFFIX:
.SUFFIX: .cpp .obj .rc .res .asm .c

{$(INDIR)}.c{$(OUTDIR)}.obj::
	$(CC) $(COPT) $<

{$(INDIR)}.cpp{$(OUTDIR)}.obj::
	$(CC) $(COPT) $<

{$(INDIR)}.rc{$(OUTDIR)}.res::
	$(RC) $(RCOPT) $<

{$(INDIR)}.asm{$(OUTDIR)}.obj::
	$(ASM) $(ASMOPT) $<

