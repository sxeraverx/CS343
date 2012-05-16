#include "Transforms/Transforms.h"

#include <clang/AST/Decl.h>
#include <clang/Rewrite/Rewriter.h>
#include <clang/Sema/Sema.h>
#include <clang/Basic/SourceManager.h>

using namespace clang;

DEFINE_TRANSFORM_BEGIN(MethodMove)
public:
virtual void HandleTranslationUnit(ASTContext &ctx) override;
virtual bool HandleTopLevelDecl(DeclGroupRef d) override;
bool TraverseDecl(Decl *D);
    
static void PrintRange(const SourceRange& R, const SourceManager& SM) {
	llvm::outs() << "(";
	R.getBegin().print(llvm::outs(), SM);
	llvm::outs() << " -- ";
	R.getEnd().print(llvm::outs(), SM);
	llvm::outs() << ")";
}
DEFINE_TRANSFORM_END(MethodMove);

bool MethodMoveTransform::TraverseDecl(Decl *D)
{
	if (NamespaceDecl* N = dyn_cast<NamespaceDecl>(D)) {
		llvm::outs() << "Namespace: " << N->getDeclName().getAsString() << "\n";
	}
	else if (CXXRecordDecl* R = dyn_cast<CXXRecordDecl>(D)) {
		llvm::outs() << "CXXRecordDecl: " << R->getDeclName().getAsString() << "\n";            
	}
	else if (CXXMethodDecl* M = dyn_cast<CXXMethodDecl>(D)) {
		llvm::outs() << "Method: " << M->getDeclName().getAsString();
		
		FullSourceLoc FSL(M->getNameInfo().getBeginLoc(), rewriter.getSourceMgr());
		const FileEntry* F = rewriter.getSourceMgr().getFileEntryForID(FSL.getFileID());
		llvm::outs() << "in: " << F->getName() << ", ";
		
		std::string fn(F->getName());
		if (fn.find("/usr") != std::string::npos) {
			return true;
		}
		
		
		std::string capturedSource;
		llvm::raw_string_ostream sst(capturedSource);
            
            
		// llvm::outs() << ", nameInfo: " << M->getNameInfo().getAsString();
		// llvm::outs() << ", beginLoc: ";
		// M->getNameInfo().getBeginLoc().print(llvm::outs(), ci->getSourceManager());
		// llvm::outs() << ", endLoc: ";
		// M->getNameInfo().getEndLoc().print(llvm::outs(), ci->getSourceManager());
		
		llvm::outs() << ", qualifier: " << M->getTypeQualifiers();
		llvm::outs() << ", return type: " << M->getResultType().getAsString();
		llvm::outs() << ", constExpr? " << M->isConstexpr();

		sst << M->getResultType().getAsString();
		sst << " ";
            
		// TODO: Namespace?
		sst << M->getParent()->getDeclName().getAsString();
		sst << "::";
		sst << M->getDeclName().getAsString();
		sst << "(";

		for (auto I = M->param_begin(), E = M->param_end(); I != E; ) {
			llvm::outs() << ", param ";
			PrintRange((*I)->getSourceRange(), rewriter.getSourceMgr());
                
			const char* cdataBegin = rewriter.getSourceMgr().getCharacterData((*I)->getSourceRange().getBegin());
			const char* cdataEnd = rewriter.getSourceMgr().getCharacterData((*I)->getSourceRange().getEnd());
			std::string code(cdataBegin, cdataEnd - cdataBegin + 1);
			sst << code;
                
			++I;
			if (I != E) {
				sst << ", ";
			}
		}

		sst << ")";
		
		// insert the const qualifier
		if (M->getTypeQualifiers() & Qualifiers::Const) {
			sst << " const";
		}
		
		if (CXXConstructorDecl* C = dyn_cast<CXXConstructorDecl>(D)) {
			if (C->init_begin() != C->init_end()) {                
                    
				sst << " : ";
                    
				// grab the initializers
				for (auto I = C->init_begin(), E = C->init_end(); I != E; ) {
					llvm::outs() << ", init'er ";
					PrintRange((*I)->getSourceRange(), rewriter.getSourceMgr());

					const char* cdataBegin = rewriter.getSourceMgr().getCharacterData((*I)->getSourceRange().getBegin());
					const char* cdataEnd = rewriter.getSourceMgr().getCharacterData((*I)->getSourceRange().getEnd());
					std::string code(cdataBegin, cdataEnd - cdataBegin + 1);
					sst << code;

					++I;
					if (I != E) {
						sst << ", ";
					}                    

				}
                    
			}
		}
            
		SourceRange R = M->getSourceRange();
            
		llvm::outs() << ", body? " << M->hasBody() << ", from: ";
            
		R.getBegin().print(llvm::outs(), rewriter.getSourceMgr());
            
		llvm::outs() << ", to: ";
		R.getEnd().print(llvm::outs(), rewriter.getSourceMgr());
            
		llvm::outs() << "\n";            
            
		if (M->hasBody()) {
			Stmt *S = M->getBody();
			SourceLocation LBL = S->getLocStart();
			SourceLocation RBL = S->getLocEnd();
                
			FullSourceLoc FSL(LBL, rewriter.getSourceMgr());
			const FileEntry* F = rewriter.getSourceMgr().getFileEntryForID(FSL.getFileID());
			llvm::outs() << "in: " << F->getName() << ", ";
                
			std::string fn(F->getName());
			if (fn.rfind(".h") != std::string::npos && fn.find("/usr") == std::string::npos) {
                    

				const char* cdataBegin = rewriter.getSourceMgr().getCharacterData(LBL);
				const char* cdataEnd = rewriter.getSourceMgr().getCharacterData(RBL);
				std::string code(cdataBegin, cdataEnd - cdataBegin + 1);
				sst << code;
				sst << "\n";
                    
				SourceLocation B = LBL;
                    
				if (M->isInlineSpecified()) {
					B = R.getBegin();
					rewriter.RemoveText(SourceRange(B, RBL));
				}
				else {
					rewriter.ReplaceText(SourceRange(B, RBL), ";");
				}
                    
				llvm::outs() << "compound stmt, from : ";
                        
				LBL.print(llvm::outs(), rewriter.getSourceMgr());        
				llvm::outs() << ", to: ";
				RBL.print(llvm::outs(), rewriter.getSourceMgr());
				llvm::outs() << "\n\n";

				// write the code
				llvm::outs() << "captured source code: " << sst.str() << "\n";

				auto leof = rewriter.getSourceMgr().getLocForEndOfFile(rewriter.getSourceMgr().getMainFileID());
				capturedSource.insert(0, "\n\n// ----\n");
				rewriter.InsertText(leof, capturedSource);
			}

		}
            
	}
	
	bool traverseResult = true;
	if(DeclContext *DC = dyn_cast<DeclContext>(D)) {
		for(auto subdecl = DC->decls_begin(); traverseResult && subdecl != DC->decls_end(); ++subdecl) {
			traverseResult = traverseResult && TraverseDecl(*subdecl);
		}
	}
        
	return traverseResult;
}

void MethodMoveTransform::HandleTranslationUnit(ASTContext &ctx)
{
	for (Rewriter::buffer_iterator iter = rewriter.buffer_begin(); iter != rewriter.buffer_end(); iter++) {
		const FileEntry* F = rewriter.getSourceMgr().getFileEntryForID(iter->first);
        
		std::string outName(F->getName());
		size_t ext = outName.rfind(".");
		if (ext == std::string::npos) {
			ext = outName.length();
		}
        
		outName.insert(ext, "_out");
        
		llvm::errs() << "Output to: " << outName << "\n";
		std::string OutErrorInfo;
        
		llvm::raw_fd_ostream outFile(outName.c_str(), OutErrorInfo, 0);

        
		if (OutErrorInfo.empty())
			{
				// Now output rewritten source code
				const RewriteBuffer *RewriteBuf = &iter->second;
				outFile << std::string(RewriteBuf->begin(), RewriteBuf->end());
			}
		else
			{
				llvm::errs() << "Cannot open " << outName << " for writing\n";
			}

		outFile.close();
	}
}

bool MethodMoveTransform::HandleTopLevelDecl(DeclGroupRef d)
{
	llvm::outs() << "HandleTopLevelDecl: ";
    
	if (d.isSingleDecl()) {
		clang::Decl* D = d.getSingleDecl();
		llvm::outs() << "single: " << D->getDeclKindName() << "\n";
	}
	else {
		llvm::outs() << "group" << "\n";
	}
    
	typedef DeclGroupRef::iterator iter;
    
	for (iter b = d.begin(), e = d.end(); b != e; ++b)
		{
        
			TraverseDecl(*b);
		}
    
	return true; // keep going
}
