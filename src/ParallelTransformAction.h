#ifndef TERNARY_CONVERTER_H
#define TERNARY_CONVERTER_H
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/FrontendAction.h"
#include <string>
#include <vector>

namespace clang {
struct ParallelTransformAction : public PluginASTAction {
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
};
} // end namespace clang
#endif
