USEDLIBS = -lLLVMCppBackendCodeGen -lLLVMCppBackendInfo -lLLVMTarget \
	-lLLVMCore -lLLVMMC -lLLVMObject -lLLVMSupport -lclangFrontend \
	-lclangEdit -lclangParse -lclangSema -lclangAnalysis -lclangAST \
	-lclangLex -lclangBasic -lclangDriver -lclangSerialization -lLLVMMC \
	-lLLVMSupport -lclangRewrite

LDFLAGS = -g -O0 `llvm-config --ldflags`
CXXFLAGS = -g -O0 `llvm-config --cxxflags` -fno-rtti

LIBS =  $(USEDLIBS)

CC=clang
CXX=clang++
LD=clang++

OUTPUT = MethodMove
OBJS = MethodMove.o

$(OUTPUT): $(OBJS)
	$(CXX) -o $@ $(LDFLAGS) $^ $(LIBS)

clean:
	rm -f $(OUTPUT)
	rm -f $(OBJS)
	rm -f *~

.PHONY: clean
