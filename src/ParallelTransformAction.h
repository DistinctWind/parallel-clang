#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include <memory>
#include <string>
#include <vector>

namespace {
using namespace clang;
using namespace tooling;
using namespace ast_matchers;
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

  // Transformer::ChangeSetConsumer consumer();
  void EndSourceFile() override;

private:
  // AtomicChanges Changes;
  MatchFinder Finder;
};
} // namespace
