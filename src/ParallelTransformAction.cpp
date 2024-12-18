#include "ParallelTransformAction.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/Transformer/RewriteRule.h"
#include "clang/Tooling/Transformer/Stencil.h"
#include "clang/Tooling/Transformer/Transformer.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>

using namespace llvm;
using namespace clang;
using namespace ast_matchers;
using namespace transformer;

/* Transformer::ChangeSetConsumer ParallelTransformAction::consumer() {
  return [this](Expected<llvm::MutableArrayRef<AtomicChange>> C) {
    if (C) {
      Changes.insert(Changes.end(), std::make_move_iterator(C->begin()),
                     std::make_move_iterator(C->end()));
    } else {
      llvm::errs() << "Error Generating Changes: " << C.takeError() << "\n";
    }
  };
} */

std::unique_ptr<ASTConsumer>
ParallelTransformAction::CreateASTConsumer(CompilerInstance &CI,
                                           StringRef InFile) {
  //   auto Rule = makeRule(cxxForRangeStmt(), changeTo(cat("FORSTMT")));
  //   auto T = std::make_unique<tooling::Transformer>(std::move(Rule),
  //   consumer()); T->registerMatchers(&Finder);
  return Finder.newASTConsumer();
}

void ParallelTransformAction::EndSourceFile() {}

static FrontendPluginRegistry::Add<ParallelTransformAction>
    Z("parallel_transform", "Parallel Transform");