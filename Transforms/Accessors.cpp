#include "Transforms.h"

#include <clang/Sema/Sema.h>
#include <llvm/Support/raw_ostream.h>

using namespace clang;

class AccessorsTransform : public Transform {
  Sema *sema;
public:
  std::string getName() override {return "AccessorsTransform"; }
  void InitializeSema(Sema &sema) override {
    this->sema = &sema;
  }
  
  void HandleTranslationUnit(ASTContext &Ctx) override {
    const TranslationUnitDecl *topLevel = Ctx.getTranslationUnitDecl();
    
    printNamespaceFunctions(topLevel);
  }
  void printNamespaceFunctions(const DeclContext *ns_decl) const
  {
    for(auto subdecl = ns_decl->decls_begin(); subdecl != ns_decl->decls_end(); subdecl++) {
      if(const RecordDecl *rc_decl = dyn_cast<RecordDecl>(*subdecl)) {
	llvm::errs() << rc_decl->getQualifiedNameAsString() << "\n";
	for(auto member = rc_decl->field_begin(); member != rc_decl->field_end(); member++) {
	  llvm::errs() << "\t" << member->getType().getAsString() << " " << member->getQualifiedNameAsString() << "\n";
	}
      }
      //recurse into inner contexts
      if(const DeclContext *inner_dc_decl = dyn_cast<DeclContext>(*subdecl))
	printNamespaceFunctions(inner_dc_decl);
    }
  }
};

REGISTER_TRANSFORM(AccessorsTransform);
