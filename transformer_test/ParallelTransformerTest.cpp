#include "attributedStmtMatcher.h"

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/Refactoring/AtomicChange.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/Transformer/RangeSelector.h"
#include "clang/Tooling/Transformer/RewriteRule.h"
#include "clang/Tooling/Transformer/Stencil.h"
#include "clang/Tooling/Transformer/Transformer.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"
#include <memory>

using namespace clang;
using namespace tooling;
using namespace ast_matchers;

TEST(Simple, Equal) { ASSERT_EQ(1 + 1, 2); }

TEST(runToolOnCode, CanSyntaxCheckCode) {
  // runToolOnCode returns whether the action was correctly run over the
  // given code.
  EXPECT_TRUE(clang::tooling::runToolOnCode(
      std::make_unique<clang::SyntaxOnlyAction>(), "class X {};"));
  EXPECT_FALSE(clang::tooling::runToolOnCode(
      std::make_unique<clang::SyntaxOnlyAction>(), "klass X {};"));
}

TEST(ParallelAttr, ASTMatcherWorks) {
  std::string testCode = R"(
void test() {
  int arr[]{1, 2, 3, 4, 5};
  for (auto &i : arr) {
    i = i * 2;
  }
}
  )";
  struct CallBackT : public MatchFinder::MatchCallback {
    bool matched = false;
    void run(const MatchFinder::MatchResult &Result) override {
      const auto *stmt = Result.Nodes.getNodeAs<CXXForRangeStmt>("stmt");
      const auto *var = Result.Nodes.getNodeAs<VarDecl>("var");
      var->print(llvm::errs(), 0, true);
      matched = true;
    }
  } CallBack;
  auto matcher =
      cxxForRangeStmt(hasLoopVariable(varDecl().bind("var"))).bind("stmt");
  MatchFinder Finder;
  Finder.addMatcher(matcher, &CallBack);

  auto Factory = newFrontendActionFactory(&Finder);
  EXPECT_TRUE(runToolOnCode(Factory->create(), testCode));
  EXPECT_TRUE(CallBack.matched);
}

TEST(ParallelAttr, ParallelASTMatcherWorks) {
  std::string testCode = R"(
void test() {
  int arr[]{1, 2, 3, 4, 5};
  [[parallel]]
  for (auto &i : arr) {
    i = i * 2;
  }
  for (auto &i : arr) {
    i = i * 2;
  }
  [[parallel]]
  for (auto &i : arr) {
    i = i * 2;
  }
}
  )";
  struct CallBackT : public MatchFinder::MatchCallback {
    int matched = 0;
    void run(const MatchFinder::MatchResult &Result) override {
      matched++;
      const auto *stmt = Result.Nodes.getNodeAs<AttributedStmt>("stmt");
      stmt->dump();
    }
  } CallBack;
  auto matcher = attributedStmt(hasParallelAttribute(anything())).bind("stmt");
  MatchFinder Finder;
  Finder.addMatcher(matcher, &CallBack);

  auto Factory = newFrontendActionFactory(&Finder);
  EXPECT_TRUE(runToolOnCode(Factory->create(), testCode));
  EXPECT_EQ(CallBack.matched, 2);
}

using namespace transformer;

TEST(Transformer, FunctionNameTransform) {
  StringRef Fun = "fun";
  RewriteRule Rule = makeRule(functionDecl(hasName("bad")).bind(Fun),
                              changeTo(name(std::string(Fun)), cat("good")));

  std::string Input = R"cc(
    int bad(int x);
    int bad(int x) { return x * x; }
  )cc";
  std::string Expected = R"cc(
    int good(int x);
    int good(int x) { return x * x; }
  )cc";

  AtomicChanges Changes;
  auto Trans = std::make_unique<Transformer>(
      std::move(Rule),
      [&](llvm::Expected<llvm::MutableArrayRef<AtomicChange>> C) {
        if (C) {
          Changes.insert(Changes.end(), C->begin(), C->end());
        }
      });
  MatchFinder Finder;
  Trans->registerMatchers(&Finder);
  auto Factory = newFrontendActionFactory(&Finder);
  EXPECT_TRUE(runToolOnCodeWithArgs(
      Factory->create(), Input, std::vector<std::string>(), "input.cc",
      "clang-tool", std::make_shared<PCHContainerOperations>()));
  auto Result =
      applyAtomicChanges("input.cc", Input, Changes, ApplyChangesSpec());
  if (Result)
    llvm::errs() << *Result;
}