#include "Transforms.h"

#include <clang/AST/ParentMap.h>
#include <llvm/Support/raw_ostream.h>

using namespace clang;
using namespace clang::tooling;
using namespace std;

class AccessorsTransform : public Transform
{
private:
	map<string, FieldDecl*> fieldRanges;
	list<string> fields;
	ASTContext *ctx;
public:
	void HandleTranslationUnit(ASTContext &Ctx) override {
		fields = TransformRegistry::get().config["Accessors"].as<decltype(fields)>();
		ctx = &Ctx;
		collect(ctx->getTranslationUnitDecl());
		insertAccessors();
	}
	void collect(const DeclContext *ns_decl) {
		for(auto subdecl = ns_decl->decls_begin(); subdecl != ns_decl->decls_end(); subdecl++) {
			/*
			 * Adding accessors requires a few things to work properly:
			 * First, the member variables must be made protected if not
			 * already protected or private. Second, the actual accessor
			 * methods must be declared and implemented. Finally, usage
			 * of the member variable must be replaced with usage of the
			 * accessors.
			 * 
			 * There is one problem. Usage of a member variable allows
			 * non-const references and pointers to escape, which is
			 * potentially not thread-safe, even if this was not the
			 * intention of the programmer. To that effect, I am
			 * allowing references to escape, but displaying a warning
			 * that this may not be desired, and the action to take
			 * to manually fix this up if this is the case.
			 * 
			 * This can not be automatically fixed up. We don't know
			 * to do in cases like these:
			 * 
			 * Foo foo;
			 * int &z = foo.x;
			 *
			 * Since 'z' can now be either written to or read from,
			 * and statically tracking pointer origins is infeasible,
			 * we simply warn the user that a non-const reference has
			 * escaped.
			 */
			if(const CXXRecordDecl *rc_decl = dyn_cast<CXXRecordDecl>(*subdecl)) {
				for(auto member = rc_decl->field_begin(); member != rc_decl->field_end(); member++)
					if(find(fields.begin(), fields.end(), member->getQualifiedNameAsString())!=fields.end())
            fieldRanges[member->getQualifiedNameAsString()] = *member;
			}
			if(const FunctionDecl *fn_decl = dyn_cast<FunctionDecl>(*subdecl)) {
				if(fn_decl->getNameAsString() != "main")
					continue;
				if(Stmt *body = fn_decl->getBody())
				{
					collect(body, ParentMap(body));
				}
			}
			//recurse into inner contexts
			if(const DeclContext *inner_dc_decl = dyn_cast<DeclContext>(*subdecl))
				collect(inner_dc_decl);
		}
	}
	void rewrite(const BinaryOperator *bin_op, const ParentMap &PM) {
		MemberExpr *lhs_expr;
		if(!(lhs_expr = dyn_cast<MemberExpr>(bin_op->getLHS())))
		{
			collect(bin_op->getLHS(), PM);
			collect(bin_op->getRHS(), PM);
			return;
		}
		for(auto iter = fieldRanges.begin(); iter != fieldRanges.end(); ++iter)
		{
			if(lhs_expr->getMemberDecl() == iter->second)
			{
				const Stmt *top_stmt_within_compound = bin_op;
				while(isa<Expr>(PM.getParent(top_stmt_within_compound))
				      || isa<DeclStmt>(PM.getParent(top_stmt_within_compound)))
					top_stmt_within_compound = PM.getParent(top_stmt_within_compound);
				
				string stmts_str, base_str;
				llvm::raw_string_ostream sstr(stmts_str), base_sstr(base_str);
				lhs_expr->getBase()->printPretty(base_sstr, *ctx, 0, PrintingPolicy(ctx->getLangOpts()));
				
				string getterName = "get" + lhs_expr->getMemberDecl()->getNameAsString();
				getterName[3] = toupper(getterName[3]);
				string setterName = "set" + lhs_expr->getMemberDecl()->getNameAsString();
				setterName[3] = toupper(setterName[3]);
				
				if(bin_op->isCompoundAssignmentOp())
				{
					
					if(bin_op!=top_stmt_within_compound)
					{
						// rewrite something like:
						// int z = (foo.x+=3);
						// to
						// foo.setX(foo.getX()+3);
						// int z = foo.getX();
						
						sstr << base_sstr.str() << "." << setterName << "( ";
						sstr << base_sstr.str() << "." << getterName << "() ";
						sstr << BinaryOperator::getOpcodeStr(BinaryOperator::getOpForCompoundAssignment(bin_op->getOpcode())) << " ";
						bin_op->getRHS()->printPretty(sstr, *ctx, 0, PrintingPolicy(ctx->getLangOpts()));
						collect(bin_op->getRHS(), PM);
						sstr << " );\n";
						//rewriter.InsertTextBefore(top_stmt_within_compound->getLocStart(), sstr.str());
						//rewriter.ReplaceText(bin_op->getSourceRange(), base_sstr.str() + "." + getterName + "()");
						insert(top_stmt_within_compound->getLocStart(), sstr.str());
						replace(bin_op->getSourceRange(), base_sstr.str() + "." + getterName + "()");
						
					}
					else
					{
						// just a simple compound assignment, e.g.
						// foo.x+=3;
						// to
						// foo.setX(foo.getX() + 3);
						sstr << base_sstr.str() << "." << setterName << "( ";
						sstr << base_sstr.str() << "." << getterName << "() ";
						sstr << BinaryOperator::getOpcodeStr(BinaryOperator::getOpForCompoundAssignment(bin_op->getOpcode())) << " ";
						collect(bin_op->getRHS(), PM);
						bin_op->getRHS()->printPretty(sstr, *ctx, 0, PrintingPolicy(ctx->getLangOpts()));
						sstr << " )";
						//rewriter.ReplaceText(bin_op->getSourceRange(), sstr.str());
						replace(bin_op->getSourceRange(), sstr.str());
					}
				}
				else if(bin_op->getOpcode() == clang::BO_Assign)
				{
					sstr << base_sstr.str() << "." << setterName << "( ";
					collect(bin_op->getRHS(), PM);
					bin_op->getRHS()->printPretty(sstr, *ctx, 0, PrintingPolicy(ctx->getLangOpts()));
					sstr << " )";
					//rewriter.ReplaceText(bin_op->getSourceRange(), sstr.str());
					replace(bin_op->getSourceRange(), sstr.str());
				}
			}
		}
	}
	void rewrite(const UnaryOperator *un_op, const ParentMap &PM) {
		MemberExpr *sub_expr;
		if(!(sub_expr = dyn_cast<MemberExpr>(un_op->getSubExpr())))
		{
			collect(un_op->getSubExpr(), PM);
			return;
		}
		for(auto iter = fieldRanges.begin(); iter != fieldRanges.end(); ++iter)
		{
			if(sub_expr->getMemberDecl() == iter->second)
			{
				const Stmt *top_stmt_within_compound = un_op;
				while(isa<Expr>(PM.getParent(top_stmt_within_compound))
				      || isa<DeclStmt>(PM.getParent(top_stmt_within_compound)))
					top_stmt_within_compound = PM.getParent(top_stmt_within_compound);
				const Stmt *top_stmt_or_compound = top_stmt_within_compound;
				if(isa<CompoundStmt>(PM.getParent(top_stmt_within_compound)))
					top_stmt_within_compound = PM.getParent(top_stmt_within_compound);
				
				string base_str;
				llvm::raw_string_ostream base_sstr(base_str);
				sub_expr->getBase()->printPretty(base_sstr, *ctx, 0, PrintingPolicy(ctx->getLangOpts()));
				base_str = base_sstr.str();
				
				string getterName = "get" + sub_expr->getMemberDecl()->getNameAsString();
				getterName[3] = toupper(getterName[3]);
				string setterName = "set" + sub_expr->getMemberDecl()->getNameAsString();
				setterName[3] = toupper(setterName[3]);
				if(!un_op->isIncrementDecrementOp())
				{
					collect(un_op->getSubExpr(), PM);
				}
				else
				{
					string incrStmt;
					{
						llvm::raw_string_ostream sstr(incrStmt);
						sstr << base_str << "." << setterName << "( ";
						sstr << base_str << "." << getterName << "() ";
						sstr << (un_op->isIncrementOp()?"+":"-") << " 1)";
						incrStmt = sstr.str();
					}
					
					string getStmt;
					{
						llvm::raw_string_ostream sstr(getStmt);
						sstr << base_str << "." << getterName << "()";
						getStmt = sstr.str();
					}
					
					bool onlyStmt = top_stmt_within_compound == top_stmt_or_compound;
					bool onlyExpr = un_op == top_stmt_within_compound;
					
					bool needToInsertBraces = false, needToInsertParentBraces = false;
					if( const IfStmt *if_stmt = dyn_cast<IfStmt>(PM.getParent(top_stmt_or_compound)) )
					{
						if(if_stmt->getThen() == top_stmt_or_compound
						   || if_stmt->getElse() == top_stmt_or_compound)
						{
							if(onlyExpr)
								//rewriter.ReplaceText(un_op->getSourceRange(), incrStmt);
								replace(un_op->getSourceRange(), incrStmt);
							else
							{
								//rewriter.ReplaceText(un_op->getSourceRange(), getStmt);
								replace(un_op->getSourceRange(), getStmt);
								if(un_op->isPrefix())
								{
									//rewriter.InsertTextBefore(un_op->getLocStart(), incrStmt + ";\n");
									insert(un_op->getLocStart(), incrStmt + ";\n");
								}
								else
								{
									assert(un_op->isPostfix());
									//rewriter.InsertTextAfterToken(un_op->getLocEnd(), ";\n" + incrStmt);
									insert(findLocAfterSemi(un_op->getLocEnd()), incrStmt + ";\n");
								}
								if(onlyStmt)
									needToInsertBraces = true;
							}
						}
						else
						{
							assert(top_stmt_within_compound == top_stmt_or_compound);
							assert(if_stmt->getCond() == top_stmt_within_compound);
							if(un_op->isPrefix())
							{
								//rewriter.InsertTextBefore(if_stmt->getLocStart(), incrStmt);
								insert(if_stmt->getLocStart(), incrStmt);
							}
							else
							{
								assert(un_op->isPostfix());
								//rewriter.InsertTextAfter(if_stmt->getLocEnd(), incrStmt);
								insert(if_stmt->getLocEnd(), incrStmt);
							}
							if(onlyStmt)
								needToInsertParentBraces = true;
						}
					}
					else if( const ForStmt *for_stmt = dyn_cast<ForStmt>(PM.getParent(top_stmt_or_compound)) )
					{
						if(for_stmt->getBody() == top_stmt_or_compound)
						{
							if(onlyExpr)
								//rewriter.ReplaceText(un_op->getSourceRange(), incrStmt);
								replace(un_op->getSourceRange(), incrStmt);
							else
							{
								//rewriter.ReplaceText(un_op->getSourceRange(), getStmt);
								replace(un_op->getSourceRange(), getStmt);
								if(un_op->isPrefix())
								{
									//rewriter.InsertTextBefore(un_op->getLocStart(), incrStmt + ";\n");
									insert(un_op->getLocStart(), incrStmt);
								}
								else
								{
									assert(un_op->isPostfix());
									//rewriter.InsertTextAfterToken(un_op->getLocEnd(), ";\n" + incrStmt);
									insert(un_op->getLocEnd(), incrStmt);
								}
								if(onlyStmt)
									needToInsertBraces = true;
							}
						}
						else if( for_stmt->getInit() == top_stmt_within_compound )
						{
							assert(onlyStmt); //no blocks in initializer
							if(onlyExpr)
								//rewriter.ReplaceText(un_op->getSourceRange(), incrStmt);
								replace(un_op->getSourceRange(), incrStmt);
							else
							{
								//rewriter.InsertTextBefore(for_stmt->getLocStart(), incrStmt);
								insert(for_stmt->getLocStart(), incrStmt + ";\n");
								if(un_op->isPostfix())
									//not sure if this is legit, but i can't think of a better way
									//rewriter.ReplaceText(un_op->getSourceRange(), "(" + getStmt + " - 1)");
									replace(un_op->getSourceRange(), "(" + getStmt + " - 1)");
								else
									//rewriter.ReplaceText(un_op->getSourceRange(), getStmt);
									replace(un_op->getSourceRange(), getStmt );
							}
						}
						else if( for_stmt->getInc() == top_stmt_within_compound )
						{
							assert(onlyStmt); //no blocks in increment
							
						}
						else
						{
							assert(onlyStmt); //no blocks in conditions
							assert(for_stmt->getCond() == top_stmt_within_compound);
							if(un_op->isPrefix())
							{
								//rewriter.InsertTextBefore(for_stmt->getLocStart(), incrStmt);
							}
							else
							{
								assert(un_op->isPostfix());
								//rewriter.InsertTextAfter(for_stmt->getLocEnd(), incrStmt);
							}
						}
					}
						/*
					bool needToInsertBraces =
						(
							(ifStmt = dyn_cast<IfStmt>(PM.getParent(top_stmt_within_compound))
							 && ((ifStmt->getThen() == top_stmt_within_compound)
							     || ifStmt->getElse() == top_stmt_within_compound))
							|| (forStmt = dyn_cast<ForStmt>(PM.getParent(top_stmt_within_compound))
							    && forStmt->getBody() == top_stmt_within_compound)
							|| (whileStmt = dyn_cast<WhileStmt>(PM
							)
						&& un_op!=top_stmt_within_compound;
					
					if(un_op==top_stmt_within_compound)
					{
						rewriter.ReplaceText(un_op->getSourceRange(), sstr.str());
					}
					else if(un_op->isPrefix())
					{
						rewriter.ReplaceText(un_op->getSourceRange(),
						                     base_str + "." + getterName + "()");
						rewriter.InsertTextBefore(top_stmt_within_compound->getLocStart(),
						                          sstr.str() + ";\n");
					}
					else
					{
						assert(un_op->isPostfix());
						rewriter.ReplaceText(un_op->getSourceRange(),
						                     base_sstr.str() + "." + getterName + "()");
						rewriter.InsertTextAfterToken(top_stmt_within_compound->getLocEnd(),
						                              ";\n" + sstr.str());
						                              }*/
					if(needToInsertBraces)
					{
						//rewriter.InsertTextBefore(top_stmt_within_compound->getLocStart(), "{\n");
						insert(top_stmt_within_compound->getLocStart(), "{\n");
						SourceLocation locAfterSemi = findLocAfterToken(top_stmt_or_compound->getLocEnd(), tok::semi);
						//rewriter.InsertTextAfterToken(locAfterSemi, "\n}");
						insert(locAfterSemi, "}\n");
					}
				}
			}
		}
	}
	void rewrite(const MemberExpr *mem_expr, const ParentMap &PM) {
		for(auto iter = fieldRanges.begin(); iter != fieldRanges.end(); ++iter)
		{
			if(mem_expr->getMemberDecl() == iter->second)
			{
				const Stmt *top_stmt_within_compound = mem_expr;
				while(isa<Expr>(PM.getParent(top_stmt_within_compound))
				      || isa<DeclStmt>(PM.getParent(top_stmt_within_compound)))
					top_stmt_within_compound = PM.getParent(top_stmt_within_compound);
				
				string stmts_str, base_str;
				llvm::raw_string_ostream sstr(stmts_str), base_sstr(base_str);
				mem_expr->getBase()->printPretty(base_sstr, *ctx, 0, PrintingPolicy(ctx->getLangOpts()));
				
				string getterName = "get" + mem_expr->getMemberDecl()->getNameAsString();
				getterName[3] = toupper(getterName[3]);
				string setterName = "set" + mem_expr->getMemberDecl()->getNameAsString();
				setterName[3] = toupper(setterName[3]);
				sstr << base_sstr.str() << "." << getterName << "()";
				//rewriter.ReplaceText(mem_expr->getSourceRange(), sstr.str());
				replace(mem_expr->getSourceRange(), sstr.str());
			}
		}
	}
void collect(const Stmt *stmt, const ParentMap &PM) {
	if(const BinaryOperator *bin_op = dyn_cast<BinaryOperator>(stmt))
		rewrite(bin_op, PM);
	else if(const UnaryOperator *un_op = dyn_cast<UnaryOperator>(stmt))
		rewrite(un_op, PM);
	else if(const MemberExpr *mem_expr = dyn_cast<MemberExpr>(stmt))
	{
		rewrite(mem_expr, PM);
	}
	else
	{
		for(auto child = stmt->child_begin();
		    child != stmt->child_end();
		    ++child)
		{
			//I don't currently understand why, but sometimes, a child
			// statement can be null. I've seen it happen in an IfStmt.
			if(*child)
				collect(*child, PM);
		}
	}
}
void insertAccessors() {
	for(auto iter = fieldRanges.begin(); iter != fieldRanges.end(); ++iter)
	{
		CXXRecordDecl *parent = dyn_cast<CXXRecordDecl>(iter->second->getParent());
		SourceLocation loc;
		string varname = iter->second->getNameAsString();
		string fnname = varname;
		fnname[0] = toupper(fnname[0]);
		string ctype = iter->second->getType().getNonReferenceType().withConst().getAsString();
		string type = iter->second->getType().getNonReferenceType().getAsString();
		stringstream sstr;
		//const getter
		sstr << ctype << " &get" << fnname << "() const { return " << varname << "; };\n";
		//non-const getter
		sstr << type << " &get" << fnname << "()  { return " << varname << "; };\n";
		//setter
		sstr << "void set" << fnname << "(" << ctype << "& _" << varname << ") { " << varname << " = _" << varname << "; };\n";
			
		bool hasUserDefinedMethods = false;
		for(auto iter = parent->method_begin(); iter != parent->method_end(); ++iter)
			if(iter->isUserProvided())
				hasUserDefinedMethods = true;
		if(!hasUserDefinedMethods)
		{
			loc = parent->getRBraceLoc();
			//rewriter.InsertTextBefore(parent->getRBraceLoc(), sstr.str());
			insert(parent->getRBraceLoc(), sstr.str());
		}
		else
		{
			CXXRecordDecl::method_iterator lastMethod = parent->method_begin();
			CXXRecordDecl::method_iterator check = lastMethod;
			do {
				if(!lastMethod->isUserProvided())
				{
					++check;
					++lastMethod;
					continue;
				}
				loc = lastMethod->getSourceRange().getEnd();
				++check;
				if(check==parent->method_end())
					break;
				++lastMethod;
			} while(1);
			//rewriter.InsertTextAfterToken(loc, sstr.str());
			insert(loc, sstr.str());
		}
	}
}
};
REGISTER_TRANSFORM(AccessorsTransform);

