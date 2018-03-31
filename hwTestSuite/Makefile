APPNAME = TstSuite
ICONTEXT = "HWTests"
APPID = TstS

SRCS = testSuite.c ugui.c

CC = m68k-palmos-gcc -palmos3.5
BUILDPRC = build-prc
PILRC = pilrc
OBJRES = m68k-palmos-obj-res

# File names
RESFILE = $(APPNAME).rcp
PRC = ../$(APPNAME).prc
OBJS = $(SRCS:.c=.o)

# Compile options
DEFINES = -DHW_TEST
CSFLAGS = -O2 -S $(DEFINES) $(INCLUDES)
CFLAGS = -O2 -g $(DEFINES) $(INCLUDES)

# Makefile targets

.S.o:
	$(CC) $(TARGETFLAGS) -c $<

.c.s:
	$(CC) $(CSFLAGS) $<

$(PRC): code.stamp bin.stamp
	$(BUILDPRC) $@ $(ICONTEXT) $(APPID) *.grc *.bin

code.stamp: $(APPNAME)
	$(OBJRES) $(APPNAME)
	touch code.stamp

bin.stamp: $(RESFILE)
	$(PILRC) $(RESFILE)
	touch bin.stamp

$(APPNAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $@

clean::
	rm -rf *.[oa] $(APPNAME) *.bin *.stamp *.grc *~