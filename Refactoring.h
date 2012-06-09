//===--- Refactoring.h - Framework for clang refactoring tools --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  Interfaces supporting refactorings that span multiple translation units.
//  While single translation unit refactorings are supported via the Rewriter,
//  when refactoring multiple translation units changes must be stored in a
//  SourceManager independent form, duplicate changes need to be removed, and
//  all changes must be applied at once at the end of the refactoring so that
//  the code is always parseable.
//
//===----------------------------------------------------------------------===//

#ifndef REFACTORING_H
#define REFACTORING_H

#include "llvm/ADT/StringRef.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Tooling/Tooling.h"
#include <string>
#include <vector>

namespace clang
{
	class Rewriter;
}

/// \brief A text replacement.
///
/// Represents a SourceManager independent replacement of a range of text in a
/// specific file.
class Replacement {
public:
  /// \brief Creates an invalid (not applicable) replacement.
  Replacement();

  /// \brief Creates a replacement of the range [Offset, Offset+Length) in
  /// FilePath with ReplacementText.
  ///
  /// \param FilePath A source file accessible via a SourceManager.
  /// \param Offset The byte offset of the start of the range in the file.
  /// \param Length The length of the range in bytes.
  Replacement(llvm::StringRef FilePath, unsigned Offset,
              unsigned Length, llvm::StringRef ReplacementText);

  /// \brief Creates a Replacement of the range [Start, Start+Length) with
  /// ReplacementText.
  Replacement(clang::SourceManager &Sources, clang::SourceLocation Start, unsigned Length,
              llvm::StringRef ReplacementText);

  /// \brief Creates a Replacement of the given range with ReplacementText.
  Replacement(clang::SourceManager &Sources, const clang::CharSourceRange &Range,
              llvm::StringRef ReplacementText);

  /// \brief Creates a Replacement of the node with ReplacementText.
  template <typename Node>
  Replacement(clang::SourceManager &Sources, const Node &NodeToReplace,
              llvm::StringRef ReplacementText);

  /// \brief Returns whether this replacement can be applied to a file.
  ///
  /// Only replacements that are in a valid file can be applied.
  bool isApplicable() const;

  /// \brief Accessors.
  /// @{
  llvm::StringRef getFilePath() const { return FilePath; }
  unsigned getOffset() const { return Offset; }
  unsigned getLength() const { return Length; }
  /// @}

  /// \brief Applies the replacement on the Rewriter.
  bool apply(clang::Rewriter &Rewrite) const;

  /// \brief Returns a human readable string representation.
  std::string toString() const;

  /// \brief Comparator to be able to use Replacement in std::set for uniquing.
  class Equal {
  public:
    bool operator()(const Replacement &R1, const Replacement &R2) const;
  };

 private:
  void setFromSourceLocation(clang::SourceManager &Sources, clang::SourceLocation Start,
                             unsigned Length, llvm::StringRef ReplacementText);
  void setFromSourceRange(clang::SourceManager &Sources, const clang::CharSourceRange &Range,
                          llvm::StringRef ReplacementText);

  std::string FilePath;
  unsigned Offset;
  unsigned Length;
  std::string ReplacementText;
};

/// \brief A set of Replacements.
typedef std::vector<Replacement> Replacements;

/// \brief Apply all replacements on the Rewriter.
///
/// If at least one Apply returns false, ApplyAll returns false. Every
/// Apply will be executed independently of the result of other
/// Apply operations.
bool applyAllReplacements(Replacements &Replaces, clang::Rewriter &Rewrite);

/// \brief A tool to run refactorings.
///
/// This is a refactoring specific version of \see ClangTool.
/// All text replacements added to getReplacements() during the run of the
/// tool will be applied and saved after all translation units have been
/// processed.
class RefactoringTool {
public:
  /// \see ClangTool::ClangTool.
  RefactoringTool(const clang::tooling::CompilationDatabase &Compilations,
                  clang::ArrayRef<std::string> SourcePaths);

  /// \brief Returns a set of replacements. All replacements added during the
  /// run of the tool will be applied after all translation units have been
  /// processed.
  Replacements &getReplacements();

  /// \see ClangTool::run.
  int run(clang::tooling::FrontendActionFactory *ActionFactory);

private:
  clang::tooling::ClangTool Tool;
  Replacements Replace;
};

template <typename Node>
Replacement::Replacement(clang::SourceManager &Sources, const Node &NodeToReplace,
                         llvm::StringRef ReplacementText) {
  const clang::CharSourceRange Range =
	  clang::CharSourceRange::getTokenRange(NodeToReplace->getSourceRange());
  setFromSourceRange(Sources, Range, ReplacementText);
}

#endif // end REFACTORING_H

