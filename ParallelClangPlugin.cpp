#include "clang/AST/ASTContext.h"
#include "clang/AST/Attr.h"
#include "clang/AST/Attrs.inc"
#include "clang/AST/Stmt.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticSema.h"
#include "clang/Basic/ParsedAttrInfo.h"
#include "clang/Sema/ParsedAttr.h"
#include "clang/Sema/Sema.h"
#include "clang/Sema/SemaDiagnostic.h"
#include "llvm/IR/Attributes.h"
using namespace clang;

namespace {
struct ParallelAttrInfo : public ParsedAttrInfo {
  ParallelAttrInfo() {
    OptArgs = 15;
    static constexpr Spelling spellings[] = {
        {ParsedAttr::AS_CXX11, "parallel"},
    };
    Spellings = spellings;
  }

  bool diagAppertainsToStmt(Sema &S, const ParsedAttr &Attr,
                            const Stmt *St) const override {
    if (!isa<CXXForRangeStmt>(St)) {
      S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type_str)
          << Attr << Attr.isRegularKeywordAttribute()
          << "for-range(C++11) loop statements";
      static auto id =
          S.Diags.getCustomDiagID(DiagnosticsEngine::Note, "not %0");
      S.Diag(St->getBeginLoc(), id) << St->getStmtClassName();
      return false;
    }
    return true;
  }

  AttrHandling handleStmtAttribute(Sema &S, Stmt *St, const ParsedAttr &Attr,
                                   class Attr *&Result) const override {
    Result = AnnotateAttr::Create(S.Context, "parallel", nullptr, 0,
                                  Attr.getRange());
    return AttributeApplied;
  }
};
} // namespace

static ParsedAttrInfoRegistry::Add<ParallelAttrInfo>
    X("parallel", "parallel attribute for loop");