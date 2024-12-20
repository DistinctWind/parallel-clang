#ifndef TERNARY_CONVERTER_H
#define TERNARY_CONVERTER_H
#include "attributedStmtMatcher.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include <memory>
#include <string>
#include <vector>

namespace clang {
struct ParallelLintAction : public PluginASTAction {
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef InFile) override;

  bool ParseArgs(const CompilerInstance &,
                 const std::vector<std::string> &) override {
    return true;
  }

  ActionType getActionType() override {
    return ActionType::CmdlineAfterMainAction;
  }

private:
  std::unique_ptr<ast_matchers::MatchFinder> ASTFinder;
  std::unique_ptr<ast_matchers::MatchFinder::MatchCallback> ForRangeMatchCB;
  std::unique_ptr<ast_matchers::MatchFinder::MatchCallback> BreakMatchCB;
  std::unique_ptr<ast_matchers::MatchFinder::MatchCallback> ContinueMatchCB;
  std::unique_ptr<ast_matchers::MatchFinder::MatchCallback> ReturnMatchCB;
  std::unique_ptr<ast_matchers::MatchFinder::MatchCallback> GotoMatchCB;

  void createDiagID(DiagnosticsEngine &Diag);
  unsigned int DiagWarnForRangeParallel;
  unsigned int DiagErrorUnexpectedBreakStmt;
  unsigned int DiagErrorUnexpectedContinueStmt;
  unsigned int DiagErrorUnexpectedReturnStmt;
  unsigned int DiagErrorUnexpectedGotoStmt;

  Rewriter RewriteForRangeWriter;
};
} // end namespace clang

#endif
