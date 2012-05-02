USEDLIBS = -lclang -lclangFrontendTool -lclangTooling \
	-lclangStaticAnalyzerFrontend -lclangCodeGen -lclangARCMigrate \
	-lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore \
	-lclangRewrite -lclangFrontend -lclangSerialization -lclangParse \
	-lclangDriver -lclangSema -lclangEdit -lclangAnalysis -lclangAST \
	-lclangLex -lclangBasic

LDFLAGS = -g -O0 `llvm-config --ldflags`
CXXFLAGS = -g -O0 `llvm-config --cxxflags`

LIBS = -lLLVM-3.1svn $(USEDLIBS)

CC=clang
CXX=clang++
LD=clang++

OUTPUT = clang-script
OBJS = main.o

$(OUTPUT): $(OBJS)
	$(CXX) -o $@ $(LDFLAGS) $^ $(LIBS)

clean:
	rm -f $(OUTPUT)
	rm -f $(OBJS)
	rm -f *~

.PHONY: clean