#include "Transforms.h"

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>

using namespace clang;
using namespace clang::tooling;

TransformRegistry &TransformRegistry::get()
{
	static TransformRegistry instance;
	return instance;
}

void TransformRegistry::add(transform_creator creator)
{
	m_transforms.push_back(creator);
}

TransformRegistry::iterator TransformRegistry::begin()
{
	return m_transforms.begin();
}

TransformRegistry::iterator TransformRegistry::end()
{
	return m_transforms.end();
}

TransformRegistration::TransformRegistration(transform_creator creator)
{
	TransformRegistry::get().add(creator);
}


class TransformAction : public clang::ASTFrontendAction {
private:
	transform_creator tcreator;
public:
	TransformAction(transform_creator creator) {tcreator = creator;}
protected:
	ASTConsumer *CreateASTConsumer(CompilerInstance &CI, llvm::StringRef) override {
		return tcreator();
	}

	virtual bool BeginInvocation(CompilerInstance &CI) override {
		CI.getHeaderSearchOpts().AddPath("/usr/local/lib/clang/3.2/include", clang::frontend::System, false, false, false);
		return true;
	}
};

TransformFactory::TransformFactory(transform_creator creator) {
	tcreator = creator;
}
FrontendAction *TransformFactory::create() {
	return new TransformAction(tcreator);
}
