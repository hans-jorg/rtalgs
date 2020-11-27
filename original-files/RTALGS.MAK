PROJ	=RTALGS
DEBUG	=0
CC	=qcl
AS	=qcl
CFLAGS_G	= /AS /W2 /Za 
CFLAGS_D	= /Zd /Od 
CFLAGS_R	= /O /Ol /Gs /DNDEBUG 
CFLAGS	=$(CFLAGS_G) $(CFLAGS_R)
AFLAGS_G	=/Cx /P1
AFLAGS_D	=/Zi
AFLAGS_R	=/DNDEBUG
AFLAGS	=$(AFLAGS_G) $(AFLAGS_R)
LFLAGS_G	= /CP:0xfff /NOI /SE:0x80 /ST:0x800 
LFLAGS_D	= 
LFLAGS_R	= 
LFLAGS	=$(LFLAGS_G) $(LFLAGS_R)
RUNFLAGS	=-e testvct5.tst
OBJS_EXT = 	
LIBS_EXT = 	

.asm.obj: ; $(AS) $(AFLAGS) -c $*.asm

all:	$(PROJ).EXE

rtalgs.obj:	rtalgs.c $(H)

skipl.obj:	skipl.c $(H)

$(PROJ).EXE:	rtalgs.obj skipl.obj $(OBJS_EXT)
	echo >NUL @<<$(PROJ).crf
rtalgs.obj +
skipl.obj +
$(OBJS_EXT)
$(PROJ).EXE

$(LIBS_EXT);
<<
	qlink $(LFLAGS) @$(PROJ).crf

run: $(PROJ).EXE
	$(PROJ) $(RUNFLAGS)

