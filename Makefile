# Compiler and linker
CC = clang
LINKER = llvm-link
OPT = opt

# Compilation flags
CFLAGS = -O3 -emit-llvm -c -g
LDFLAGS = -lm -lpython3.12 -lonig -g
DEFINES = -DHAVE_STDIO_H=1 \
          -DHAVE_STDLIB_H=1 \
          -DHAVE_STRING_H=1 \
          -DHAVE_INTTYPES_H=1 \
          -DHAVE_STDINT_H=1 \
          -DHAVE_STRINGS_H=1 \
          -DHAVE_SYS_STAT_H=1 \
          -DHAVE_SYS_TYPES_H=1 \
          -DHAVE_UNISTD_H=1 \
          -DHAVE_WCHAR_H=1 \
          -DSTDC_HEADERS=1 \
          -D_ALL_SOURCE=1 \
          -D_DARWIN_C_SOURCE=1 \
          -D_GNU_SOURCE=1 \
          -D_HPUX_ALT_XOPEN_SOCKET_API=1 \
          -D_NETBSD_SOURCE=1 \
          -D_OPENBSD_SOURCE=1 \
          -D_POSIX_PTHREAD_SEMANTICS=1 \
          -D__STDC_WANT_IEC_60559_ATTRIBS_EXT__=1 \
          -D__STDC_WANT_IEC_60559_BFP_EXT__=1 \
          -D__STDC_WANT_IEC_60559_DFP_EXT__=1 \
          -D__STDC_WANT_IEC_60559_FUNCS_EXT__=1 \
          -D__STDC_WANT_IEC_60559_TYPES_EXT__=1 \
          -D__STDC_WANT_LIB_EXT2__=1 \
          -D__STDC_WANT_MATH_SPEC_FUNCS__=1 \
          -D_TANDEM_SOURCE=1 \
          -D__EXTENSIONS__=1 \
          -DPACKAGE=\"jq\" \
          -DVERSION=\"1.7.1-27-g913b264-dirty\" \
          -DHAVE_DLFCN_H=1 \
          -DLT_OBJDIR=\".libs/\" \
          -DHAVE_MEMMEM=1 \
          -DUSE_DECNUM=1 \
          -DHAVE_PTHREAD_PRIO_INHERIT=1 \
          -DHAVE_PTHREAD=1 \
          -DHAVE_ALLOCA_H=1 \
          -DHAVE_ALLOCA=1 \
          -DHAVE_ISATTY=1 \
          -DHAVE_STRPTIME=1 \
          -DHAVE_STRFTIME=1 \
          -DHAVE_SETENV=1 \
          -DHAVE_TIMEGM=1 \
          -DHAVE_GMTIME_R=1 \
          -DHAVE_GMTIME=1 \
          -DHAVE_LOCALTIME_R=1 \
          -DHAVE_LOCALTIME=1 \
          -DHAVE_GETTIMEOFDAY=1 \
          -DHAVE_TM_TM_GMT_OFF=1 \
          -DHAVE_SETLOCALE=1 \
          -DHAVE_PTHREAD_KEY_CREATE=1 \
          -DHAVE_PTHREAD_ONCE=1 \
          -DHAVE_ATEXIT=1 \
          -DHAVE_ACOS=1 \
          -DHAVE_ACOSH=1 \
          -DHAVE_ASIN=1 \
          -DHAVE_ASINH=1 \
          -DHAVE_ATAN2=1 \
          -DHAVE_ATAN=1 \
          -DHAVE_ATANH=1 \
          -DHAVE_CBRT=1 \
          -DHAVE_CEIL=1 \
          -DHAVE_COPYSIGN=1 \
          -DHAVE_COS=1 \
          -DHAVE_COSH=1 \
          -DHAVE_DREM=1 \
          -DHAVE_ERF=1 \
          -DHAVE_ERFC=1 \
          -DHAVE_EXP10=1 \
          -DHAVE_EXP2=1 \
          -DHAVE_EXP=1 \
          -DHAVE_EXPM1=1 \
          -DHAVE_FABS=1 \
          -DHAVE_FDIM=1 \
          -DHAVE_FLOOR=1 \
          -DHAVE_FMA=1 \
          -DHAVE_FMAX=1 \
          -DHAVE_FMIN=1 \
          -DHAVE_FMOD=1 \
          -DHAVE_FREXP=1 \
          -DHAVE_GAMMA=1 \
          -DHAVE_HYPOT=1 \
          -DHAVE_J0=1 \
          -DHAVE_J1=1 \
          -DHAVE_JN=1 \
          -DHAVE_LDEXP=1 \
          -DHAVE_LGAMMA=1 \
          -DHAVE_LOG10=1 \
          -DHAVE_LOG1P=1 \
          -DHAVE_LOG2=1 \
          -DHAVE_LOG=1 \
          -DHAVE_LOGB=1 \
          -DHAVE_MODF=1 \
          -DHAVE_LGAMMA_R=1 \
          -DHAVE_NEARBYINT=1 \
          -DHAVE_NEXTAFTER=1 \
          -DHAVE_NEXTTOWARD=1 \
          -DHAVE_POW=1 \
          -DHAVE_REMAINDER=1 \
          -DHAVE_RINT=1 \
          -DHAVE_ROUND=1 \
          -DHAVE_SCALB=1 \
          -DHAVE_SCALBLN=1 \
          -DHAVE_SIGNIFICAND=1 \
          -DHAVE_SCALBN=1 \
          -DHAVE_ILOGB=1 \
          -DHAVE_SIN=1 \
          -DHAVE_SINH=1 \
          -DHAVE_SQRT=1 \
          -DHAVE_TAN=1 \
          -DHAVE_TANH=1 \
          -DHAVE_TGAMMA=1 \
          -DHAVE_TRUNC=1 \
          -DHAVE_Y0=1 \
          -DHAVE_Y1=1 \
          -DHAVE_YN=1 \
          -DHAVE___THREAD=1 \
          -DIEEE_8087=1 \
          -Wno-deprecated-non-prototype \
          -DHAVE_LIBONIG=1

# Define source directories
SRC_DIRS = cjq/bootstrap cjq/jq/src cjq/llvmlib cjq/pylib

# Specific source files
C_SOURCES = $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c))
C_SOURCES += cjq/trace/cjq_trace.c
C_SOURCES += cjq/jq/src/decNumber/decNumber.c
C_SOURCES += cjq/jq/src/decNumber/decContext.c
IR_FILE = ir.ll

# Object files
BC_FILES = $(C_SOURCES:.c=.bc) ir.bc
UNOPT_BC = cjq.bc
OPT_BC = cjq_inlined.bc

# Executable
EXECUTABLE_UNOPT = run
EXECUTABLE_OPT = run_opt

# Default target
all: $(EXECUTABLE_UNOPT) $(EXECUTABLE_OPT)

# Rule to compile .c files to .bc files
%.bc: %.c
	$(CC) $(CFLAGS) $(DEFINES) -o $@ $<

# Rule to compile the IR file to .bc
ir.bc: $(IR_FILE)
	$(CC) $(CFLAGS) $(DEFINES) -o $@ $<

# Link all .bc files for unoptimized version
$(UNOPT_BC): $(BC_FILES)
	$(LINKER) -o $@ $^

# Optimize the linked bitcode
$(OPT_BC): $(UNOPT_BC)
	$(OPT) -passes='default<O3>,inline' -inline-threshold=1000 -o $@ $^
	$(OPT) -O3 -o $@ $@

# Compile the linked bitcode to an executable (unoptimized)
$(EXECUTABLE_UNOPT): $(BC_FILES)
	$(CC) -g $(BC_FILES) -lm -lpython3.12 -lonig -o $@ $(DEFINES) -L/usr/local/lib -lonig

# Compile the optimized bitcode to an executable
$(EXECUTABLE_OPT): $(OPT_BC)
	$(CC) -O3 $< $(LDFLAGS) -o $@

# Rule to generate LLVM bitcode and compile to shared object
llvmgen:
	clang -O0 -fPIC -shared -o jq_util.so cjq/jq/src/*.c cjq/jq/src/decNumber/decNumber.c cjq/jq/src/decNumber/decContext.c cjq/trace/*.c cjq/llvmlib/*.c cjq/pylib/*.c -g -lm -L/home/rubio/anaconda3/envs/numbaEnv/lib/python3.12/config-3.12-x86_64-linux-gnu -I/home/rubio/anaconda3/envs/numbaEnv/include/python3.12 -lpython3.12 -DHAVE_STDIO_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_STRINGS_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_UNISTD_H=1 -DHAVE_WCHAR_H=1 -DSTDC_HEADERS=1 -D_ALL_SOURCE=1 -D_DARWIN_C_SOURCE=1 -D_GNU_SOURCE=1 -D_HPUX_ALT_XOPEN_SOCKET_API=1 -D_NETBSD_SOURCE=1 -D_OPENBSD_SOURCE=1 -D_POSIX_PTHREAD_SEMANTICS=1 -D__STDC_WANT_IEC_60559_ATTRIBS_EXT__=1 -D__STDC_WANT_IEC_60559_BFP_EXT__=1 -D__STDC_WANT_IEC_60559_DFP_EXT__=1 -D__STDC_WANT_IEC_60559_FUNCS_EXT__=1 -D__STDC_WANT_IEC_60559_TYPES_EXT__=1 -D__STDC_WANT_LIB_EXT2__=1 -D__STDC_WANT_MATH_SPEC_FUNCS__=1 -D_TANDEM_SOURCE=1 -D__EXTENSIONS__=1 -DPACKAGE=\"jq\" -DVERSION=\"1.7.1-27-g913b264-dirty\" -DHAVE_DLFCN_H=1 -DLT_OBJDIR=\".libs/\" -DHAVE_MEMMEM=1 -DUSE_DECNUM=1 -DHAVE_PTHREAD_PRIO_INHERIT=1 -DHAVE_PTHREAD=1 -DHAVE_ALLOCA_H=1 -DHAVE_ALLOCA=1 -DHAVE_ISATTY=1 -DHAVE_STRPTIME=1 -DHAVE_STRFTIME=1 -DHAVE_SETENV=1 -DHAVE_TIMEGM=1 -DHAVE_GMTIME_R=1 -DHAVE_GMTIME=1 -DHAVE_LOCALTIME_R=1 -DHAVE_LOCALTIME=1 -DHAVE_GETTIMEOFDAY=1 -DHAVE_TM_TM_GMT_OFF=1 -DHAVE_SETLOCALE=1 -DHAVE_PTHREAD_KEY_CREATE=1 -DHAVE_PTHREAD_ONCE=1 -DHAVE_ATEXIT=1 -DHAVE_ACOS=1 -DHAVE_ACOSH=1 -DHAVE_ASIN=1 -DHAVE_ASINH=1 -DHAVE_ATAN2=1 -DHAVE_ATAN=1 -DHAVE_ATANH=1 -DHAVE_CBRT=1 -DHAVE_CEIL=1 -DHAVE_COPYSIGN=1 -DHAVE_COS=1 -DHAVE_COSH=1 -DHAVE_DREM=1 -DHAVE_ERF=1 -DHAVE_ERFC=1 -DHAVE_EXP10=1 -DHAVE_EXP2=1 -DHAVE_EXP=1 -DHAVE_EXPM1=1 -DHAVE_FABS=1 -DHAVE_FDIM=1 -DHAVE_FLOOR=1 -DHAVE_FMA=1 -DHAVE_FMAX=1 -DHAVE_FMIN=1 -DHAVE_FMOD=1 -DHAVE_FREXP=1 -DHAVE_GAMMA=1 -DHAVE_HYPOT=1 -DHAVE_J0=1 -DHAVE_J1=1 -DHAVE_JN=1 -DHAVE_LDEXP=1 -DHAVE_LGAMMA=1 -DHAVE_LOG10=1 -DHAVE_LOG1P=1 -DHAVE_LOG2=1 -DHAVE_LOG=1 -DHAVE_LOGB=1 -DHAVE_MODF=1 -DHAVE_LGAMMA_R=1 -DHAVE_NEARBYINT=1 -DHAVE_NEXTAFTER=1 -DHAVE_NEXTTOWARD=1 -DHAVE_POW=1 -DHAVE_REMAINDER=1 -DHAVE_RINT=1 -DHAVE_ROUND=1 -DHAVE_SCALB=1 -DHAVE_SCALBLN=1 -DHAVE_SIGNIFICAND=1 -DHAVE_SCALBN=1 -DHAVE_ILOGB=1 -DHAVE_SIN=1 -DHAVE_SINH=1 -DHAVE_SQRT=1 -DHAVE_TAN=1 -DHAVE_TANH=1 -DHAVE_TGAMMA=1 -DHAVE_TRUNC=1 -DHAVE_Y0=1 -DHAVE_Y1=1 -DHAVE_YN=1 -DHAVE___THREAD=1 -DIEEE_8087=1 -DHAVE_LIBONIG=1 -L/usr/local/lib -lonig
	clang -O0 cjq/trace/*.c cjq/jq/src/*.c cjq/llvmlib/*.c cjq/pylib/*.c cjq/jq/src/decNumber/decNumber.c cjq/jq/src/decNumber/decContext.c -g -lm -o llvmgen -L/home/rubio/anaconda3/envs/numbaEnv/lib/python3.12/config-3.12-x86_64-linux-gnu -I/home/rubio/anaconda3/envs/numbaEnv/include/python3.12 -lpython3.12 -DHAVE_STDIO_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_STRINGS_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_UNISTD_H=1 -DHAVE_WCHAR_H=1 -DSTDC_HEADERS=1 -D_ALL_SOURCE=1 -D_DARWIN_C_SOURCE=1 -D_GNU_SOURCE=1 -D_HPUX_ALT_XOPEN_SOCKET_API=1 -D_NETBSD_SOURCE=1 -D_OPENBSD_SOURCE=1 -D_POSIX_PTHREAD_SEMANTICS=1 -D__STDC_WANT_IEC_60559_ATTRIBS_EXT__=1 -D__STDC_WANT_IEC_60559_BFP_EXT__=1 -D__STDC_WANT_IEC_60559_DFP_EXT__=1 -D__STDC_WANT_IEC_60559_FUNCS_EXT__=1 -D__STDC_WANT_IEC_60559_TYPES_EXT__=1 -D__STDC_WANT_LIB_EXT2__=1 -D__STDC_WANT_MATH_SPEC_FUNCS__=1 -D_TANDEM_SOURCE=1 -D__EXTENSIONS__=1 -DPACKAGE=\"jq\" -DVERSION=\"1.7.1-27-g913b264-dirty\" -DHAVE_DLFCN_H=1 -DLT_OBJDIR=\".libs/\" -DHAVE_MEMMEM=1 -DUSE_DECNUM=1 -DHAVE_PTHREAD_PRIO_INHERIT=1 -DHAVE_PTHREAD=1 -DHAVE_ALLOCA_H=1 -DHAVE_ALLOCA=1 -DHAVE_ISATTY=1 -DHAVE_STRPTIME=1 -DHAVE_STRFTIME=1 -DHAVE_SETENV=1 -DHAVE_TIMEGM=1 -DHAVE_GMTIME_R=1 -DHAVE_GMTIME=1 -DHAVE_LOCALTIME_R=1 -DHAVE_LOCALTIME=1 -DHAVE_GETTIMEOFDAY=1 -DHAVE_TM_TM_GMT_OFF=1 -DHAVE_SETLOCALE=1 -DHAVE_PTHREAD_KEY_CREATE=1 -DHAVE_PTHREAD_ONCE=1 -DHAVE_ATEXIT=1 -DHAVE_ACOS=1 -DHAVE_ACOSH=1 -DHAVE_ASIN=1 -DHAVE_ASINH=1 -DHAVE_ATAN2=1 -DHAVE_ATAN=1 -DHAVE_ATANH=1 -DHAVE_CBRT=1 -DHAVE_CEIL=1 -DHAVE_COPYSIGN=1 -DHAVE_COS=1 -DHAVE_COSH=1 -DHAVE_DREM=1 -DHAVE_ERF=1 -DHAVE_ERFC=1 -DHAVE_EXP10=1 -DHAVE_EXP2=1 -DHAVE_EXP=1 -DHAVE_EXPM1=1 -DHAVE_FABS=1 -DHAVE_FDIM=1 -DHAVE_FLOOR=1 -DHAVE_FMA=1 -DHAVE_FMAX=1 -DHAVE_FMIN=1 -DHAVE_FMOD=1 -DHAVE_FREXP=1 -DHAVE_GAMMA=1 -DHAVE_HYPOT=1 -DHAVE_J0=1 -DHAVE_J1=1 -DHAVE_JN=1 -DHAVE_LDEXP=1 -DHAVE_LGAMMA=1 -DHAVE_LOG10=1 -DHAVE_LOG1P=1 -DHAVE_LOG2=1 -DHAVE_LOG=1 -DHAVE_LOGB=1 -DHAVE_MODF=1 -DHAVE_LGAMMA_R=1 -DHAVE_NEARBYINT=1 -DHAVE_NEXTAFTER=1 -DHAVE_NEXTTOWARD=1 -DHAVE_POW=1 -DHAVE_REMAINDER=1 -DHAVE_RINT=1 -DHAVE_ROUND=1 -DHAVE_SCALB=1 -DHAVE_SCALBLN=1 -DHAVE_SIGNIFICAND=1 -DHAVE_SCALBN=1 -DHAVE_ILOGB=1 -DHAVE_SIN=1 -DHAVE_SINH=1 -DHAVE_SQRT=1 -DHAVE_TAN=1 -DHAVE_TANH=1 -DHAVE_TGAMMA=1 -DHAVE_TRUNC=1 -DHAVE_Y0=1 -DHAVE_Y1=1 -DHAVE_YN=1 -DHAVE___THREAD=1 -DIEEE_8087=1 -DHAVE_LIBONIG=1 -L/usr/local/lib -lonig

# Clean up
clean:
	rm -f $(BC_FILES) $(UNOPT_BC) $(OPT_BC) $(EXECUTABLE_UNOPT) $(EXECUTABLE_OPT)

# Recompile only if the source files change
.PHONY: all clean llvmgen

