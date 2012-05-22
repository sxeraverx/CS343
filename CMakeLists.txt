CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

PROJECT (Refactorial)

SET(CMAKE_DISABLE_SOURCE_CHANGES ON)
SET(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
SET(CMAKE_BUILD_TYPE None)
SET(CMAKE_C_COMPILER clang)
SET(CMAKE_CXX_COMPILER clang++)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_LINKER_FLAGS} -g -fno-rtti -fcolor-diagnostics -Wno-c++11-extensions")
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_CXX_LINKER_FLAGS} -g -fno-rtti")
IF(APPLE)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ --std=c++0x")
  SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
ELSE(APPLE)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11")
ENDIF(APPLE)

FIND_PACKAGE(LLVM REQUIRED)

ADD_DEFINITIONS(${LLVM_DEFINITIONS})
INCLUDE_DIRECTORIES(${LLVM_INCLUDE_DIRS} .)
LINK_DIRECTORIES(${LLVM_LIBRARY_DIRS})
LLVM_MAP_COMPONENTS_TO_LIBRARIES(REQ_LLVM_LIBRARIES native)

SET(CLANG_LIBRARIES clangAnalysis clangAST clangBasic clangDriver clangEdit clangFrontend clangLex clangParse clangRewrite clangSema clangSerialization clangTooling)

SET(Transforms_sources Transforms.cpp Accessors.cpp MethodMoveTransform.cpp)

FOREACH(arg ${Transforms_sources})
  LIST(APPEND sources "Transforms/${arg}")
ENDFOREACH(arg ${Transforms_sources})

SET(sources ${sources} main.cpp)

ADD_EXECUTABLE (refactorial ${sources} )
TARGET_LINK_LIBRARIES (refactorial ${REQ_LLVM_LIBRARIES} ${CLANG_LIBRARIES})
