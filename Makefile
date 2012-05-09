USEDLIBS = -lclang \
	-lclangAnalysis \
	-lclangAST \
	-lclangBasic \
	-lclangDriver \
	-lclangEdit \
	-lclangFrontend \
	-lclangLex \
	-lclangParse \
	-lclangRewrite \
	-lclangSema \
	-lclangSerialization \
	-lclangTooling \
	-lLLVMMC \
	-lLLVMSupport

UNAME = $(shell uname)

COMMONFLAGS = -g -O0

ifeq ($(UNAME), Darwin)
	LDFLAGS = $(COMMONFLAGS) `llvm-config --ldflags` -stdlib=libc++
	CXXFLAGS = $(COMMONFLAGS) `llvm-config --cxxflags` -fno-rtti -std=c++0x -stdlib=libc++
	## Use these for bleeding-edge Clang, otherwise use the one that comes with Xcode 
	# CC=/usr/local/bin/clang
	# CXX=/usr/local/bin/clang++
	# LD=/usr/local/bin/clang++
	CC=clang
	CXX=clang++
	LD=clang++
else
	LDFLAGS = $(COMMONFLAGS) `llvm-config --ldflags`
	CXXFLAGS = $(COMMONFLAGS) `llvm-config --cxxflags` -fno-rtti --std=gnu++11
	CC=clang
	CXX=clang++
	LD=clang++
endif

LIBS = $(USEDLIBS)
OUTPUT = clang-script
OBJS = main.o

$(OUTPUT): $(OBJS)
	$(CXX) -o $@ $(LDFLAGS) $^ $(LIBS)

clean:
	rm -f $(OUTPUT)
	rm -f $(OBJS)
	rm -f *~

.PHONY: clean
