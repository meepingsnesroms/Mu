APPNAME = BootExtract
ICONTEXT = "VZBootEX"
APPID = Bdmp

SRCS = bootloaderExtractor.c

# Tool locations
ifeq ($(PALM_OS_TOOLCHAIN),)
$(error PALM_OS_TOOLCHAIN not defined)
endif

ifeq ($(PALM_OS_RESOURCE_TOOLS),)
$(error PALM_OS_RESOURCE_TOOLS not defined)
endif

CC = $(PALM_OS_TOOLCHAIN)/m68k-palmos/gcc295/gcc/gcc-cross -palmos3.5
BUILDPRC = $(PALM_OS_TOOLCHAIN)/tools/build-prc
PILRC = $(PALM_OS_RESOURCE_TOOLS)/pilrc
OBJRES = $(PALM_OS_TOOLCHAIN)/tools/obj-res

# File names
RESFILE = $(APPNAME).rcp
PRC = ../$(APPNAME).prc
OBJS = $(SRCS:.c=.o)

# Compile options
DEFINES = -DHW_TEST
CSFLAGS = -O2 -S $(DEFINES) $(INCLUDES)
CFLAGS = -O2 -g $(DEFINES) $(INCLUDES)

# Makefile targets

all: 

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