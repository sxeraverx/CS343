//===- PrintFunctionNames.cpp ---------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Example clang plugin which simply prints the names of all the top-level decls
// in the input file.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/AST.h"
#include <clang/Sema/SemaConsumer.h>
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
using namespace clang;

#include "Transforms/Transforms.h"

int main(int argc, char **argv)
{
  std::string errorMessage("Could not load compilation database");
  llvm::OwningPtr<tooling::CompilationDatabase> Compilations(tooling::CompilationDatabase::loadFromDirectory(".", errorMessage));
  tooling::ClangTool ct(*Compilations.take(), std::vector<std::string>(argv+1, argv+argc));
  for(auto iter = TransformRegistry::get().begin(); iter != TransformRegistry::get().end(); iter++)
    {
      ct.run(new TransformFactory(*iter));
    }
  return 0;
}
