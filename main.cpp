/*** CItutorial6.cpp *****************************************************
* This code is licensed under the New BSD license.
* See LICENSE.txt for details.
*
* The CI tutorials remake the original tutorials but using the
* CompilerInstance object which has as one of its purpose to create commonly
* used Clang types.
*****************************************************************************/
#include <iostream>

#include <llvm/Support/Host.h>

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Basic/TargetOptions.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Driver/ArgList.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/Parse/Parser.h>
#include <clang/Parse/ParseAST.h>

/******************************************************************************
*
*****************************************************************************/
class MyASTConsumer : public clang::ASTConsumer
{
public:
    MyASTConsumer() : clang::ASTConsumer() { }
    virtual ~MyASTConsumer() { }

    virtual bool HandleTopLevelDecl( clang::DeclGroupRef d)
    {
        static int count = 0;
        clang::DeclGroupRef::iterator it;
        for( it = d.begin(); it != d.end(); it++)
        {
            count++;
            clang::VarDecl *vd = llvm::dyn_cast<clang::VarDecl>(*it);
            if(!vd)
            {
                continue;
            }
            if( vd->isFileVarDecl() && !vd->hasExternalStorage() )
            {
                std::cerr << "Read top-level variable decl: '";
                std::cerr << vd->getDeclName().getAsString() ;
                std::cerr << std::endl;
            }
        }
        return true;
    }
};

/******************************************************************************
*
*****************************************************************************/
int main(int argc, char **argv)
{
    using clang::CompilerInstance;
    using clang::TargetOptions;
    using clang::TargetInfo;
    using clang::FileEntry;
    using clang::Token;
    using clang::ASTContext;
    using clang::ASTConsumer;
    using clang::Parser;
    using clang::driver::InputArgList;
    using clang::CompilerInvocation;
    using llvm::IntrusiveRefCntPtr;

    CompilerInstance ci;
    InputArgList args(argv, argv+argc);
    ci.createDiagnostics(argc,argv);
    IntrusiveRefCntPtr<CompilerInvocation> cinv(new CompilerInvocation);
    CompilerInvocation::CreateFromArgs(*cinv, argv, argv+argc, ci.getDiagnostics());

    cinv->getHeaderSearchOpts().Verbose = true;
    ci.setInvocation(&*cinv);

    TargetOptions to;
    to.Triple = llvm::sys::getDefaultTargetTriple();
    TargetInfo *pti = TargetInfo::CreateTargetInfo(ci.getDiagnostics(), to);
    ci.setTarget(pti);

    ci.createFileManager();
    ci.createSourceManager(ci.getFileManager());
    clang::HeaderSearch headerSearch(ci.getFileManager(),
				     ci.getDiagnostics(),
				     ci.getLangOpts(),
				     pti);
    ci.createPreprocessor();
    //ci.getPreprocessorOpts().UsePredefines = false;
    for(std::vector<clang::FrontendInputFile>::iterator iter = ci.getFrontendOpts().Inputs.begin(); iter!=ci.getFrontendOpts().Inputs.end(); iter++) {
      //iter->Kind = clang::FrontendOptions::getInputKindForExtension(llvm::StringRef(iter->File).rsplit('.').second);
      llvm::outs() << iter->File << " " << iter->Kind << " " << iter->IsSystem << "\n";
    }
    MyASTConsumer *astConsumer = new MyASTConsumer();
    ci.setASTConsumer(astConsumer);

    ci.createASTContext();

    for(std::vector<clang::FrontendInputFile>::iterator iter = ci.getFrontendOpts().Inputs.begin()+1; iter!=ci.getFrontendOpts().Inputs.end(); iter++) {
      const FileEntry *pFile = ci.getFileManager().getFile(iter->File);
      ci.getSourceManager().createMainFileID(pFile);
      ci.getDiagnosticClient().BeginSourceFile(ci.getLangOpts(),
					       &ci.getPreprocessor());
      clang::ParseAST(ci.getPreprocessor(), astConsumer, ci.getASTContext());
      ci.getDiagnosticClient().EndSourceFile();
    } 
    return 0;
}
