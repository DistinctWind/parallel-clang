#include "clang/AST/Stmt.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchersMacros.h"

namespace {
using namespace clang;
AST_MATCHER_P(AttributedStmt, hasParallelAttribute,
              ast_matchers::internal::Matcher<Stmt>, InnerMatcher) {
  for (const auto *attr : Node.getAttrs()) {
    if (!isa<AnnotateAttr>(attr))
      continue;
    const auto *annotateAttr = cast<AnnotateAttr>(attr);
    if (annotateAttr->getAnnotation() == "parallel") {
      // llvm::errs() << "Found parallel attribute\n";
      // Node.getSubStmt()->dump();
      return InnerMatcher.matches(*Node.getSubStmt(), Finder, Builder);
    }
  }
  return false;
}
} // namespace

extern const ast_matchers::internal::VariadicDynCastAllOfMatcher<Stmt,
                                                                 AttributedStmt>
    attributedStmt;