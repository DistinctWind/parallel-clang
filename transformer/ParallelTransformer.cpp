#include "attributedStmtMatcher.h"

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Refactoring/AtomicChange.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/Transformer/RangeSelector.h"
#include "clang/Tooling/Transformer/RewriteRule.h"
#include "clang/Tooling/Transformer/Stencil.h"
#include "clang/Tooling/Transformer/Transformer.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace clang;
using namespace tooling;
using namespace ast_matchers;
using transformer::addInclude;
using transformer::cat;
using transformer::changeTo;
using transformer::IncludeFormat;
using transformer::makeRule;
using transformer::node;

static cl::OptionCategory
    ParallelTransformCategory("parallel-transform options");

static StatementMatcher buildMatcher() {
  return attributedStmt(
             hasParallelAttribute(
                 cxxForRangeStmt(hasBody(compoundStmt().bind("body")),
                                 hasLoopVariable(varDecl().bind("var")),
                                 hasRangeInit(expr().bind("range")))
                     .bind("for")))
      .bind("attr");
}

bool applySourceChanges(const AtomicChanges &Changes) {
  std::set<std::string> Files;
  for (const auto &Change : Changes)
    Files.insert(Change.getFilePath());
  tooling::ApplyChangesSpec Spec;
  Spec.Cleanup = false;
  for (const auto &File : Files) {
    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> BufferErr =
        llvm::MemoryBuffer::getFile(File);
    if (!BufferErr) {
      llvm::errs() << "error: failed to open " << File << " for rewriting\n";
      return true;
    }
    auto Result = tooling::applyAtomicChanges(File, (*BufferErr)->getBuffer(),
                                              Changes, Spec);
    if (!Result) {
      llvm::errs() << toString(Result.takeError());
      return true;
    }
    llvm::outs() << *Result;
  }
  return false;
}

int main(int argc, const char **argv) {
  auto ExpectedParser =
      CommonOptionsParser::create(argc, argv, ParallelTransformCategory);
  if (!ExpectedParser) {
    llvm::errs() << ExpectedParser.takeError();
    return 1;
  }
  CommonOptionsParser &OptionsParser = ExpectedParser.get();
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  AtomicChanges Changes;
  auto Consumer = [&](Expected<MutableArrayRef<AtomicChange>> C) {
    if (C) {
      Changes.insert(Changes.end(), std::make_move_iterator(C->begin()),
                     std::make_move_iterator(C->end()));
    } else {
      llvm::errs() << "Error generating changes: "
                   << llvm::toString(C.takeError()) << "\n";
    }
  };

  auto varWithoutColon =
      [](const ast_matchers::MatchFinder::MatchResult &result)
      -> Expected<CharSourceRange> {
    auto range = node("var")(result);
    if (!range)
      return range.takeError();
    range->setEnd(range->getEnd().getLocWithOffset(-1));
    return range;
  };

  auto Rule = makeRule(
      buildMatcher(),
      {addInclude("algorithm", IncludeFormat::Angled),
       addInclude("execution", IncludeFormat::Angled),
       changeTo(cat("std::for_each(std::execution::par, ", "std::begin(",
                    node("range"), "), ", "std::end(", node("range"), "), ",
                    "[&](", varWithoutColon, ")", node("body"), ");"))});
  Transformer Transformer(std::move(Rule), std::move(Consumer));
  MatchFinder Finder;
  Transformer.registerMatchers(&Finder);

  Tool.run(newFrontendActionFactory(&Finder).get());

  applySourceChanges(Changes);
  return 0;
}