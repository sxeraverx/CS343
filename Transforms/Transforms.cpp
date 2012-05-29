#include "Transforms.h"

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>

#include <stdexcept>

using namespace clang;
using namespace clang::tooling;

Transform::~Transform()
{
	for(auto buffer_info = rewriter.buffer_begin(); buffer_info != rewriter.buffer_end(); ++buffer_info)
	{
		const FileEntry *file = rewriter.getSourceMgr().getFileEntryForID(buffer_info->first);
		std::string outName(file->getName());
		size_t ext = outName.rfind(".");
		if (ext == std::string::npos)
			ext = outName.length();
		
		//outName.insert(ext, ".refactored");
		llvm::errs() << "Output to: " << outName << "\n";
		std::string outErrInfo;

		llvm::raw_fd_ostream outFile(outName.c_str(), outErrInfo, 0);

		if(outErrInfo.empty()) {
			RewriteBuffer &buffer = buffer_info->second;
			outFile << std::string(buffer.begin(), buffer.end());
		}
		else {
			llvm::errs() << "Cannot open " << outName << " for writing\n";
		}
		outFile.close();
	}
}

void Transform::InitializeSema(clang::Sema &s)
{
	sema = &s;
	rewriter.setSourceMgr(s.getSourceManager(), s.getLangOpts());
}

TransformRegistry &TransformRegistry::get()
{
	static TransformRegistry instance;
	return instance;
}

void TransformRegistry::add(const std::string &name, transform_creator creator)
{
	m_transforms.insert(std::pair<std::string, transform_creator>(name, creator));
}

const transform_creator TransformRegistry::operator[](const std::string &name) const
{
	auto iter = m_transforms.find(name);
	if(iter==m_transforms.end())
		throw std::out_of_range(name);
	return iter->second;
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
