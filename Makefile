##
## configuration flags
##

USE_STLPORT = 1

##
## set OS variable
##   (set automatically by windows. Sun and linus use OSTYPE
##

ifeq ($(OSTYPE),Linux)
  OS=Linux
  PLATFORM=unix
endif

ifeq ($(OSTYPE),solaris)
  OS=solaris
  PLATFORM=unix
endif

ifeq ($(OS),Windows_NT)
  PLATFORM=win32
endif



##
## set variables based on platform type
##

ifeq ($(PLATFORM),win32)
  CC = cl
  LD = link
  AR = lib

  OBJ = obj
  EXEEXT = .exe
  LIBEXT = .lib

  PCH = $(DIR)/bt.pch

  STLBASE = c:/src/current/lib/cpp/STLport-4.5.3
  STLINC  = $(STLBASE)/stlport
  STLLIB  = $(STLBASE)/lib

  VCINC	  = c:/devstudio/vc98/include
  VCLIB	  = c:/devstudio/vc98/lib

  BOOSTBASE = c:/src/current/lib/cpp/boost
  BOOSTINC  = $(BOOSTBASE)/include
  BOOSTLIB  = C:/boost_1_28_0/libs/regex/build/vc6-stlport

  DEFINES = -DWIN32 

  OPTFLAGS = -O2 -MT -DNDEBUG
  DBGFLAGS = -Zi -MTd -DDEBUG -DDEBUG_OUT_TO_WINDEBUG $(STL_DEBUG_DEFS)
  INCLUDES = $(STDINCLUDES) 

  COPTS = -X -nologo
  CXXOPTS = -X -nologo -GX -GR -Fp"$(PCH)" -Fd"$(DIR)/bt.pdb"

  USEPCHOPT = -Yu"stdinc.h"
  MKPCHOPT =  -Yc"stdinc.h"

  PLATFORM_H = winos.h
  PLATFORM_SRC = winos.cpp

  LDFLAGS = -subsystem:console $(STDLIBPATHS)
  LDFLAGS_DEBUG = -debug 

endif

ifeq ($(PLATFORM),unix)
  CC = c++
  LD = ld
  AR = ar

  OBJ=o
  EXEEXT=
  LIBEXT=.a

  STLINC    = /home/alanp/src/STLport-4.5.3/stlport
  BOOSTINC  = /home/alanp/src/boost

  DEFINES = -DUNIX -D$(OSTYPE)

  OPTFLAGS = -O2 -DNDEBUG
  DBGFLAGS = -g -DDEBUG -DDEBUG_OUT_TO_FILE $(STL_DEBUG_DEFS)
  INCLUDES = $(STDINCLUDES) 

  COPTS = 
  CXXOPTS =

  PLATFORM_H = unixos.h
  PLATFORM_SRC = unixos.cpp

##  LDFLAGS = -Wl,-L/home/alanp/src/boost/libs/regex/build/bin/libboost_regex.a/gcc/debug/runtime-link-dynamic
  LDFLAGS = -L/home/alanp/src/STLport-4.5.3/lib -L/home/alanp/src/boost/libs/regex/build/bin/libboost_regex.a/gcc-stlport/debug/runtime-link-dynamic/stlport-anachronisms-on/stlport-cstd-namespace-std/stlport-debug-alloc-off/stlport-iostream-on/stlport-version-4.5.3
  LDFLAGS_DEBUG = -g -L/home/alanp/src/STLport-4.5.3/lib

  LIBS = -lrt -lstlport_gcc_stldebug

opt-build:   LIBS += -lboost_regex
debug-build: LIBS += -lboost_regex_debug

endif





ifeq ($(PLATFORM),win32)

  ifeq ("$(USE_STLPORT)","1")

    STDINCLUDES =	-I$(STLINC) \
			-I$(VCINC) \
			-I$(BOOSTINC)

    STDLIBPATHS =	-LIBPATH:$(VCLIB) \
			-LIBPATH:$(BOOSTLIB) \
			-LIBPATH:$(STLLIB)


    STL_DEBUG_DEFS = -D _STLP_DEBUG

  else

    STDINCLUDES =	-I$(VCINC) \
			-I$(BOOSTINC)

    STDLIBPATHS =	-LIBPATH:$(VCLIB) \
			-LIBPATH:$(BOOSTLIB) \
			-LIBPATH:$(STLLIB)

  endif

endif


ifeq ($(PLATFORM),unix)

  ifeq ("$(USE_STLPORT)","1")

    STDINCLUDES =	-I $(STLINC) \
			-I $(BOOSTINC)

    STDLIBPATHS =	-l$(BOOSTLIB) \
			-l$(STLLIB)

    STL_DEBUG_DEFS = -D_STLP_DEBUG -D_STLP_USE_STATIC_LIB
  else

    STDINCLUDES =	-I $(BOOSTINC)

    STDLIBPATHS =	-l$(BOOSTLIB) \
			-l$(STLLIB)

  endif

endif


CFLAGS += $(COPTS) $(INCLUDES) $(DEFINES)
CXXFLAGS += $(CXXOPTS) $(INCLUDES) $(DEFINES) 


BTTEST = $(DIR)/bttest$(EXEEXT)
BTDELTEST = $(DIR)/btdeltest$(EXEEXT)
MKRND  = $(DIR)/mkrnd$(EXEEXT)
LIBBT = $(DIR)/libbt$(LIBEXT)

BTTESTOBJS =	$(DIR)/bttest.$(OBJ)
BTDELTESTOBJS =	$(DIR)/btdeltest.$(OBJ)
MKRNDOBJS =	$(DIR)/mkrnd.$(OBJ)
LIBOBJS =	$(DIR)/btalloc.$(OBJ) $(DIR)/btcreate.$(OBJ) $(DIR)/btdump.$(OBJ) \
		$(DIR)/bthdr.$(OBJ) $(DIR)/btinsert.$(OBJ) $(DIR)/btnode.$(OBJ) \
		$(DIR)/btread.$(OBJ) $(DIR)/btsearch.$(OBJ) $(DIR)/btfrag.$(OBJ) \
		$(DIR)/btdelete.$(OBJ) \
		$(DIR)/dbg.$(OBJ) $(DIR)/os.$(OBJ) 


ifeq ($(PLATFORM),win32)
 LIBOBJS += $(DIR)/winos.$(OBJ)
endif

ifeq ($(PLATFORM),unix)
 LIBOBJS += $(DIR)/unixos.$(OBJ)
endif

.PHONY: all opt opt-build debug debug-build dir clean

all: opt

opt-build: CFLAGS += $(OPTFLAGS)
opt-build: CXXFLAGS += $(OPTFLAGS)
opt-build: dir $(BTTEST) $(MKRND)

opt: 
	$(MAKE) -$(MAKEFLAGS) DIR=opt opt-build

debug-build: CFLAGS += $(DBGFLAGS)
debug-build: CXXFLAGS += $(DBGFLAGS)
debug-build: LDFLAGS += $(LDFLAGS_DEBUG)
debug-build: dir $(BTTEST) $(BTDELTEST) $(MKRND)

debug: 
	$(MAKE) -$(MAKEFLAGS) DIR=debug debug-build

dir:
	-mkdir $(DIR)

##############################
## win32 targets
##

ifeq ($(PLATFORM),win32)

$(BTDELTEST): $(LIBBT) $(BTDELTESTOBJS)
	$(LD) $(LDFLAGS) -out:$(BTDELTEST) $(DIR)/stdinc.$(OBJ) $(BTDELTESTOBJS) $(LIBBT) 

$(BTTEST): $(LIBBT) $(BTTESTOBJS)
	$(LD) $(LDFLAGS) -out:$(BTTEST) $(DIR)/stdinc.$(OBJ) $(BTTESTOBJS) $(LIBBT) 

$(MKRND): $(MKRNDOBJS)
	$(LD) $(LDFLAGS) -out:$(MKRND) $(DIR)/stdinc.$(OBJ) $(MKRNDOBJS)

$(LIBBT): $(LIBOBJS)
	$(AR) -out:$(LIBBT) $(LIBOBJS)


$(DIR)/%.$(OBJ): %.cpp
	$(CC) -c $(CXXFLAGS) $(USEPCHOPT) -FR$(DIR)/$*.sbr -Fo$(DIR)/$*.$(OBJ) $< 

$(DIR)/stdinc.$(OBJ): stdinc.cpp
	$(CC) -c $(CXXFLAGS) $(MKPCHOPT) -FR$(DIR)/stdinc.sbr -Fo$(DIR)/stdinc.$(OBJ) $< 

clean:
	-cmd /c rmdir /s /q opt
	-cmd /c rmdir /s /q debug

endif



##############################
## UNIX targets
##

ifeq ($(PLATFORM),unix)

$(BTTEST): $(LIBBT) $(BTTESTOBJS)
	$(CC) -o $(BTTEST) $(LDFLAGS) $(BTTESTOBJS) $(LIBBT) $(LIBS)

$(MKRND): $(MKRNDOBJS)
	$(CC) -o $(MKRND) $(LDFLAGS) $(MKRNDOBJS) $(LIBS)

$(LIBBT): $(LIBOBJS)
	$(AR) -r $(LIBBT) $(LIBOBJS)


$(DIR)/%.$(OBJ): %.cpp
	$(CC) -c $(CXXFLAGS) -o $(DIR)/$*.$(OBJ) $< 

clean:
	-rm -rf opt
	-rm -rf debug

endif


##############################
## Dependancies
##

$(DIR)/stdinc.$(OBJ):	stdinc.h sys.h std.h boost.h

$(DIR)/btalloc.$(OBJ):	$(PCH) stdinc.h sys.h std.h boost.h os.h $(PLATFORM_H) dbg.h bt.h
$(DIR)/btcreate.$(OBJ):	$(PCH) stdinc.h sys.h std.h boost.h os.h $(PLATFORM_H) dbg.h bt.h
$(DIR)/btdelete.$(OBJ):	$(PCH) stdinc.h sys.h std.h boost.h os.h $(PLATFORM_H) dbg.h bt.h
$(DIR)/btdump.$(OBJ):	$(PCH) stdinc.h sys.h std.h boost.h os.h $(PLATFORM_H) dbg.h bt.h
$(DIR)/btfrag.$(OBJ):	$(PCH) stdinc.h sys.h std.h boost.h os.h $(PLATFORM_H) dbg.h bt.h
$(DIR)/bthdr.$(OBJ):	$(PCH) stdinc.h sys.h std.h boost.h os.h $(PLATFORM_H) dbg.h bt.h
$(DIR)/btinsert.$(OBJ):	$(PCH) stdinc.h sys.h std.h boost.h os.h $(PLATFORM_H) dbg.h bt.h
$(DIR)/btnode.$(OBJ):	$(PCH) stdinc.h sys.h std.h boost.h os.h $(PLATFORM_H) dbg.h bt.h
$(DIR)/btread.$(OBJ):	$(PCH) stdinc.h sys.h std.h boost.h os.h $(PLATFORM_H) dbg.h bt.h
$(DIR)/btsearch.$(OBJ):	$(PCH) stdinc.h sys.h std.h boost.h os.h $(PLATFORM_H) dbg.h bt.h
$(DIR)/dbg.$(OBJ):	$(PCH) stdinc.h sys.h std.h boost.h os.h $(PLATFORM_H) dbg.h
$(DIR)/os.$(OBJ):	$(PCH) stdinc.h sys.h std.h boost.h os.h $(PLATFORM_H) dbg.h
$(DIR)/dbg.$(OBJ):	$(PCH) stdinc.h sys.h std.h boost.h os.h $(PLATFORM_H) dbg.h
$(DIR)/mkrnd.$(OBJ):	$(PCH) stdinc.h sys.h std.h boost.h os.h $(PLATFORM_H) 

$(DIR)/bttest.$(OBJ):	$(PCH) stdinc.h sys.h std.h boost.h os.h $(PLATFORM_H) dbg.h bt.h
$(DIR)/btdeltest.$(OBJ):	$(PCH) stdinc.h sys.h std.h boost.h os.h $(PLATFORM_H) dbg.h bt.h

$(DIR)/winos.$(OBJ):	$(PCH) stdinc.h sys.h std.h boost.h os.h winos.h
$(DIR)/unixos.$(OBJ):	$(PCH) stdinc.h sys.h std.h boost.h os.h unixos.h

ifdef PCH
$(PCH): $(DIR)/stdinc.$(OBJ)
endif
