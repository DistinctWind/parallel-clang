#include "ParallelTransformAction.h"

#include "clang/AST/ASTTypeTraits.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/StmtCXX.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include <cassert>
#include <memory>

using namespace clang;
using namespace ast_matchers;

extern const internal::VariadicDynCastAllOfMatcher<Stmt, AttributedStmt>
    attributedStmt;

static StatementMatcher buildForRangeMatcher() {
  // return cxxForRangeStmt(hasBody(compoundStmt().bind("body")),
  //                        hasLoopVariable(varDecl().bind("var")),
  //                        hasRangeInit(expr().bind("range")))
  return attributedStmt().bind("for");
}

namespace {
struct MatchForRangeCallBack : public MatchFinder::MatchCallback {
  unsigned int diag_warn_for_range;
  MatchForRangeCallBack(unsigned int DiagWarnForRange)
      : diag_warn_for_range(DiagWarnForRange) {}
  void run(const MatchFinder::MatchResult &Result) override {
    const auto &Nodes = Result.Nodes;
    auto &Diag = Result.Context->getDiagnostics();

    const auto *forSt = Nodes.getNodeAs<AttributedStmt>("for");
    // const auto *body = Nodes.getNodeAs<CompoundStmt>("body");
    // const auto *var = Nodes.getNodeAs<VarDecl>("var");
    // const auto *range = Nodes.getNodeAs<Expr>("range");

    Diag.Report(forSt->getBeginLoc(), diag_warn_for_range);
  }
};
} // namespace

std::unique_ptr<ASTConsumer>
ParallelTransformAction::CreateASTConsumer(CompilerInstance &CI,
                                           StringRef InFile) {
  assert(CI.hasASTContext() && "No ASTContext??");

  auto &Diag = CI.getASTContext().getDiagnostics();
  auto DiagWarnForRange = Diag.getCustomDiagID(
      DiagnosticsEngine::Warning,
      "this for-range will be converted to parallel version");

  ASTFinder = std::make_unique<MatchFinder>();
  ForRangeMatchCB = std::make_unique<MatchForRangeCallBack>(DiagWarnForRange);
  ASTFinder->addMatcher(
      traverse(TK_IgnoreUnlessSpelledInSource, buildForRangeMatcher()),
      ForRangeMatchCB.get());

  return std::move(ASTFinder->newASTConsumer());
}

static FrontendPluginRegistry::Add<ParallelTransformAction>
    Y("parallel_transform",
      "transform annotated for-range loop into std algorithm parallel version");