USEDLIBS = -lclangAnalysis \
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
	-lclangTooling

UNAME = $(shell uname)

COMMONFLAGS = -g -O0
LIBS = `llvm-config --libs engine`  $(USEDLIBS)
LDFLAGS = $(COMMONFLAGS) `llvm-config --ldflags`
CXXFLAGS = $(COMMONFLAGS) `llvm-config --cxxflags` -fno-rtti
CC=clang
CXX=clang++
LD=clang++

ifeq ($(UNAME), Darwin)
	LDFLAGS += -stdlib=libc++
	CXXFLAGS += -std=c++0x -stdlib=libc++
	## Use these for bleeding-edge Clang, otherwise use the one that comes with Xcode 
	# CC=/usr/local/bin/clang
	# CXX=/usr/local/bin/clang++
	# LD=/usr/local/bin/clang++
else
	CXXFLAGS += --std=gnu++11
endif

OUTPUT = clang-script
OBJS = main.o

$(OUTPUT): $(OBJS)
	$(CXX) -o $@ $(LDFLAGS) $^ $(LIBS)

clean:
	rm -f $(OUTPUT)
	rm -f $(OBJS)
	rm -f *~

.PHONY: clean
