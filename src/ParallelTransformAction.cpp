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

const ast_matchers::internal::VariadicDynCastAllOfMatcher<Stmt, AttributedStmt>
    attributedStmt;

static StatementMatcher buildForRangeMatcher() {
  return attributedStmt(hasParallelAttribute(
      cxxForRangeStmt(hasBody(compoundStmt().bind("body")),
                      hasLoopVariable(varDecl().bind("var")),
                      hasRangeInit(expr().bind("range")))
          .bind("for")));
}

static StatementMatcher buildForRangeBreakMatcher() {
  return attributedStmt(hasParallelAttribute(
      cxxForRangeStmt(
          hasBody(compoundStmt(forEachDescendant(breakStmt().bind("break")))
                      .bind("body")),
          hasLoopVariable(varDecl().bind("var")),
          hasRangeInit(expr().bind("range")))
          .bind("for")));
}

static StatementMatcher buildForRangeContinueMatcher() {
  return attributedStmt(hasParallelAttribute(
      cxxForRangeStmt(hasBody(compoundStmt(forEachDescendant(
                                               continueStmt().bind("continue")))
                                  .bind("body")),
                      hasLoopVariable(varDecl().bind("var")),
                      hasRangeInit(expr().bind("range")))
          .bind("for")));
}

static StatementMatcher buildForRangeReturnMatcher() {
  return attributedStmt(hasParallelAttribute(
      cxxForRangeStmt(
          hasBody(compoundStmt(forEachDescendant(returnStmt().bind("return")))
                      .bind("body")),
          hasLoopVariable(varDecl().bind("var")),
          hasRangeInit(expr().bind("range")))
          .bind("for")));
}

namespace {
struct MatchForRangeCallBack : public MatchFinder::MatchCallback {
  unsigned int diag_warn_for_range;
  MatchForRangeCallBack(unsigned int DiagWarnForRange)
      : diag_warn_for_range(DiagWarnForRange) {}
  void run(const MatchFinder::MatchResult &Result) override {
    const auto &Nodes = Result.Nodes;
    auto &Diag = Result.Context->getDiagnostics();

    const auto *forSt = Nodes.getNodeAs<CXXForRangeStmt>("for");
    const auto *body = Nodes.getNodeAs<CompoundStmt>("body");
    const auto *var = Nodes.getNodeAs<VarDecl>("var");
    const auto *range = Nodes.getNodeAs<Expr>("range");

    Diag.Report(forSt->getBeginLoc(), diag_warn_for_range);
  }
};

struct MatchForRangeBreakCallBack : public MatchFinder::MatchCallback {
  unsigned int DiagErrorUnexpectedBreakStmt;
  MatchForRangeBreakCallBack(unsigned int DiagErrorUnexpectedBreakStmt)
      : DiagErrorUnexpectedBreakStmt(DiagErrorUnexpectedBreakStmt) {}
  void run(const MatchFinder::MatchResult &Result) override {
    const auto &Nodes = Result.Nodes;
    auto &Diag = Result.Context->getDiagnostics();
    const auto *breakSt = Nodes.getNodeAs<BreakStmt>("break");
    if (breakSt) {
      Diag.Report(breakSt->getBreakLoc(), DiagErrorUnexpectedBreakStmt);
    }
  }
};

struct MatchForRangeContinueCallBack : public MatchFinder::MatchCallback {
  unsigned int DiagErrorUnexpectedContinueStmt;
  MatchForRangeContinueCallBack(unsigned int DiagErrorUnexpectedContinueStmt)
      : DiagErrorUnexpectedContinueStmt(DiagErrorUnexpectedContinueStmt) {}
  void run(const MatchFinder::MatchResult &Result) override {
    const auto &Nodes = Result.Nodes;
    auto &Diag = Result.Context->getDiagnostics();
    const auto *continueSt = Nodes.getNodeAs<ContinueStmt>("continue");
    if (continueSt) {
      Diag.Report(continueSt->getContinueLoc(),
                  DiagErrorUnexpectedContinueStmt);
    }
  }
};

struct MatchForRangeReturnCallBack : public MatchFinder::MatchCallback {
  unsigned int DiagErrorUnexpectedReturnStmt;
  MatchForRangeReturnCallBack(unsigned int DiagErrorUnexpectedReturnStmt)
      : DiagErrorUnexpectedReturnStmt(DiagErrorUnexpectedReturnStmt) {}
  void run(const MatchFinder::MatchResult &Result) override {
    const auto &Nodes = Result.Nodes;
    auto &Diag = Result.Context->getDiagnostics();
    const auto *returnSt = Nodes.getNodeAs<ReturnStmt>("return");
    if (returnSt) {
      Diag.Report(returnSt->getReturnLoc(), DiagErrorUnexpectedReturnStmt);
    }
  }
};
} // namespace

void ParallelTransformAction::createDiagID(DiagnosticsEngine &Diag) {
  DiagWarnForRangeParallel = Diag.getCustomDiagID(
      DiagnosticsEngine::Warning,
      "this for-range will be converted to parallel version");
  DiagErrorUnexpectedBreakStmt = Diag.getCustomDiagID(
      DiagnosticsEngine::Error,
      "unexpected control flow statement 'break' found in parallel for-range");
  DiagErrorUnexpectedContinueStmt = Diag.getCustomDiagID(
      DiagnosticsEngine::Error, "unexpected control flow statement 'continue' "
                                "found in parallel for-range");
  DiagErrorUnexpectedReturnStmt = Diag.getCustomDiagID(
      DiagnosticsEngine::Error, "unexpected control flow statement 'return' "
                                "found in parallel for-range");
}

std::unique_ptr<ASTConsumer>
ParallelTransformAction::CreateASTConsumer(CompilerInstance &CI,
                                           StringRef InFile) {
  assert(CI.hasASTContext() && "No ASTContext??");

  auto &Diag = CI.getASTContext().getDiagnostics();
  createDiagID(Diag);

  ASTFinder = std::make_unique<MatchFinder>();
  ForRangeMatchCB =
      std::make_unique<MatchForRangeCallBack>(DiagWarnForRangeParallel);
  BreakMatchCB = std::make_unique<MatchForRangeBreakCallBack>(
      DiagErrorUnexpectedBreakStmt);
  ContinueMatchCB = std::make_unique<MatchForRangeContinueCallBack>(
      DiagErrorUnexpectedContinueStmt);
  ReturnMatchCB = std::make_unique<MatchForRangeReturnCallBack>(
      DiagErrorUnexpectedReturnStmt);
  ASTFinder->addMatcher(
      traverse(TK_IgnoreUnlessSpelledInSource, buildForRangeMatcher()),
      ForRangeMatchCB.get());
  ASTFinder->addMatcher(
      traverse(TK_IgnoreUnlessSpelledInSource, buildForRangeBreakMatcher()),
      BreakMatchCB.get());
  ASTFinder->addMatcher(
      traverse(TK_IgnoreUnlessSpelledInSource, buildForRangeContinueMatcher()),
      ContinueMatchCB.get());
  ASTFinder->addMatcher(
      traverse(TK_IgnoreUnlessSpelledInSource, buildForRangeReturnMatcher()),
      ReturnMatchCB.get());

  return std::move(ASTFinder->newASTConsumer());
}

static FrontendPluginRegistry::Add<ParallelTransformAction>
    Y("parallel_transform",
      "transform annotated for-range loop into std algorithm parallel version");