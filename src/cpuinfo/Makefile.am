###########################################################################
#
# Some consistent rules for building asm files:

STRIP_FPIC = sh $(top_srcdir)/strip_fPIC.sh

SUFFIXES = .asm

.asm.lo:
	$(LIBTOOL) --tag=CC --mode=compile $(STRIP_FPIC) $(NASM) -t -D __FLAT__ @NASMFLAGS@ $< -o $*.o

###########################################################################

# The cpuinfo library target
noinst_LTLIBRARIES = libcpuinfo.la

if HAVE_NASM
ARCH_SRCS =			\
	_cpuinfo.asm		\
	_pcihelp.asm
else
ARCH_SRCS =
endif

COMMON_SRCS = 			\
	cpuinfo.h		\
	gcpuinfo.c		\
	SDL_cpuinfo.c

libcpuinfo_la_SOURCES = $(ARCH_SRCS) $(COMMON_SRCS)

EXTRA_DIST =			\
	COPYING.LIB		\
	README

