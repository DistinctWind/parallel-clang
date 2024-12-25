# Parallel Clang

This is an experimental yet extensive, source-to-source compiler project for adding customized attributes into C++ language, providing a linter and transformer, by extending Clang and using LibTooling.

More specifically, it adds the `[[parallel]]` custom attribute to C++ language, which only applies to for-range loops. The linter Clang plugin will check whether the `[[parallel]]` attribute is properly used, and the stand-alone tool `parallel-transformer` will take the responsibility to convert that marked for-range loop into something actually parallel, by rewriting the source code.

Take the `example/valid.cpp` as an example:

```c++
void test() {
  int arr[]{1, 2, 3, 4, 5};
  [[parallel]]
  for (auto &i : arr) {
    i = i * 2;
  }
}
```

This `[[parallel]]` marked for-range loop will be converted to an underlying parallel framework, using C++ 17's Execution Policy API here:

```c++
#include <algorithm>
#include <execution>
void test() {
  int arr[]{1, 2, 3, 4, 5};
  std::for_each(std::execution::par, std::begin(arr), std::end(arr), [&](auto &i){
    i = i * 2;
  });
}
```
 
## Requirements and Build Instructions

This project requires latest LLVM 20 build, considering that LLVM 20 doesn't have a release yet, you should build LLVM 20 by yourself, and define cmake variables `LLVM_DIR` and `Clang_DIR` to let cmake find that custom LLVM 20 build instead of system-wide one if you had LLVM (or Clang) installed somewhere.


For example, download latest LLVM, build and install it by running:

```shell
git clone git@github.com:llvm/llvm-project.git
cd llvm-project
cmake –S llvm –B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=~/.local/llvm-git \
    -DLLVM_ENABLE_PROJECTS="clang"
cmake --build./build
cmake --build./build --target install
```

Note that LLVM is quite huge and compile takes about 2 hours to be done.

Then, build and install the project, parallel-clang, with LLVM path specified:

```shell
cd parallel-clang
cmake –S. –B build \
    -G Ninja \
    -DLLVM_DIR=~/.local/llvm-git/lib/cmake/llvm \
    -DClang_DIR~/.local/llvm-git/lib/cmake/clang
cmake --build ./build
```

Note that this plugin is **built with system compiler**. We didn't specify to use compiler located `~/.local/llvm-git` at all. However, this plugin should be loaded to Clang 20 to check attribute usage.

You will get two key files in the `./build` directory:

1. `./build/lint/ParallelLintPlugin.so`: Linter plugin that finds out potential wrong usage for `[[parallel]]` attribute.
2. `./build/transformer/parallel-transformer`: Stand-alone executable that recognize and convert the `[[parallel]]` marked for-range loops, result will be printed to stdout.

## Usage

### Clang Lint Plugin

This plugin can be found in `./build/lint/ParallelLintPlugin.so`.

Load the Clang plugin with `-fplugin=...` and enable that plugin by passing `-plugin parallel_lint` directly to cc1 (not Clang driver). This plugin should be loaded to the same Clang that specified in `LLVM_DIR`.

```shell
cd parallel-clang
~/.local/llvm-git/bin/clang++ \
     -fsyntax-only \
     -fplugin=./build/lint/ParallelLintPlugin.so \
     -Xclang -plugin \
     -Xclang parallel_lint \
     <input file>
```

If the input file is correct (for example `./example/valid.cpp`), only a warning will be shown:

```text
./example/valid.cpp:10:3: warning: this for-range will be converted to parallel version
   10 |   for (auto &i : arr) {
      |   ^
1 warning generated.
```

By using `./example/test_wrong_usage.cpp` as an input file, should get:

```text
./example/test_wrong_usage.cpp:2:5: error: 'parallel' attribute only applies to for-range(C++11) loop statements
    2 |   [[parallel]]
      |     ^
./example/test_wrong_usage.cpp:3:3: note: not 'ForStmt'
    3 |   for (int i = 1; i < 10; i++) {}
      |   ^
./example/test_wrong_usage.cpp:4:5: error: 'parallel' attribute only applies to for-range(C++11) loop statements
    4 |   [[parallel]]
      |     ^
./example/test_wrong_usage.cpp:5:3: note: not 'WhileStmt'
    5 |   while (true) {}
      |   ^
./example/test_wrong_usage.cpp:6:5: error: 'parallel' attribute only applies to for-range(C++11) loop statements
    6 |   [[parallel]]
      |     ^
3 errors generated.
```

By using `./example/test_unexpected_control_flow.cpp` as an input file, should get:

```text
./example/test_unexpected_control_flow.cpp:5:7: warning: this for-range will be converted to parallel version
    5 |       for (auto &i : arr) {
      |       ^
./example/test_unexpected_control_flow.cpp:7:11: error: unexpected control flow statement 'break' found in parallel for-range
    7 |           break;
      |           ^
./example/test_unexpected_control_flow.cpp:9:11: error: unexpected control flow statement 'break' found in parallel for-range
    9 |           break;
      |           ^
./example/test_unexpected_control_flow.cpp:8:11: error: unexpected control flow statement 'continue' found in parallel for-range
    8 |           continue;
      |           ^
./example/test_unexpected_control_flow.cpp:10:11: error: unexpected control flow statement 'return' found in parallel for-range
   10 |           return;
      |           ^
./example/test_unexpected_control_flow.cpp:11:13: error: unexpected control flow statement 'goto' found in parallel for-range
   11 |           { goto lab; }
      |             ^
1 warning and 5 errors generated.
```

### Stand-alone Transformer

This executable can be found in `./build/transformer/parallel-transformer`. Only verified  (aka, no error reported from the Linter Plugin) C++ source file as input makes sense.

This transformer actually takes a compile database `compile_commands.json` to locate include files and libraries. See [CommonOptionsParser](https://clang.llvm.org/doxygen/classclang_1_1tooling_1_1CommonOptionsParser.html#details) for more.

For simplicity, we will disable that feature by adding a trailing `--` after source file. That means invoking `parallel-transformer` with:

```shell
./build/transformer/parallel-transformer <input file> --
```

By using `./example/valid.cpp` as an input file, should get:

```text
#include <algorithm>
#include <execution>
void test_traditional() {
  for (int i = 1; i < 10; i++) {
  }
  int x;
}

void test() {
  int arr[]{1, 2, 3, 4, 5};
  std::for_each(std::execution::par, std::begin(arr), std::end(arr), [&](auto &i){
    i = i * 2;
  });
  for (auto &i : arr) {
    i = i * 2;
  }
}
```

## Project Structure

![Project Structure](https://s21.ax1x.com/2024/12/25/pAjxne0.png)

Note that Parallel Lint Plugin is a Module Library and should be run with Clang. 

## Details

### User-Defined Attribute `[[parallel]]`

This ability comes from [Clang Plugin](https://clang.llvm.org/docs/ClangPlugins.html), providing an interface for defining attributes by extending `ParsedAttrInfo`. The attribute is defined in `./attribute` directory. The same attribute definition also works in stand-alone tools using LibTooling.

Note that adding attribute directly to a Statement is quite new to LLVM, see this [commit](https://github.com/llvm/llvm-project/commit/9a97a57d9ee9dbaa4f7ecfdaba565171ea49b7ac) for more details. This is also why this project requires LLVM 20.

### Local AST Matcher for `AttributedStmt`

Clang used to have a [plan](https://reviews.llvm.org/D120949?id=418341#inline-1163697) adding `attributedStmt`, but it got removed because of the expensiveness of compiling `ASTMatchers.h`. According to [the collaborator answer to my issue](https://github.com/llvm/llvm-project/issues/117879), a local AST Mather should be defined.

AST Matcher has a [reference](https://clang.llvm.org/docs/LibASTMatchersReference.html) that is useful.

In `./ast_matcher`, two AST Matchers are defined to achieve the goal matching `[[parallel]]` for-range loops.

#### Node Matcher for `AttributedStmt`

```c++
extern const ast_matchers::internal::VariadicDynCastAllOfMatcher<Stmt, AttributedStmt>
    attributedStmt;
```

This is a Clang defined utility AST Matcher only matches if current `Stmt` is a `AttributedStmt`. The matcher name `attributedStmt` follows other matchers' convenience.

#### Narrowing Matcher for `[[parallel]]` Annotation

This part is not as shown as much by official [LibASTMatchers doc](https://clang.llvm.org/docs/LibASTMatchers.html). We need to define a narrowing matcher that makes sure current `AttributedStmt` also has a `AnnotateAttr` named `parallel`.

In `ASTMatchersMacros.h`, the most suited declaration is [`AST_MATCHER_P`](https://clang.llvm.org/doxygen/ASTMatchersMacros_8h.html#ae9c7f3955d5fdd516853af78dea0ac9c), because we also need a parameter to get the inner for-range loop. See `./ast_matcher/attributedStmtMatcher.h` for details.

### Use LibTransformer

Clang provides a structural way to modify an AST by using [Clang Transformer](https://clang.llvm.org/docs/ClangTransformerTutorial.html). However, the document doesn't say much about how to use a `RewriteRule` to generate transformed code.

Fortunately, the usage of `RewriteRule` can be extracted from the [unit test of transformer](https://github.com/llvm/llvm-project/blob/main/clang/unittests/Tooling/TransformerTest.cpp). Consider unit tests as a running code example. After reading the first 200 lines of code, you will get how to make use of `RewriteRule` and `addInclude`/`changeTo` edits. This [blog](https://medium.com/@mugiseyebrows/source-to-source-transformation-using-transformers-in-clang-b6507a191ca4) may also help.

### Delete Colon of `var` Node

After completing `RewriteRule`, you will notice that `node` range selector will include the trailing colon `:` of for-range loop variable declaration, as explained in its [doc](https://clang.llvm.org/doxygen/namespaceclang_1_1transformer.html#a598c31d824a2295b5865152ff99ae44f). However, we don't need that because underlying C++ Lambda doesn't accept a colon in function variable declaration.

After reading the [type definition of `Stencil`](https://clang.llvm.org/doxygen/namespaceclang_1_1transformer.html#a6d0af023ab1a8dfcea4d5b1174e492ce). A small trick modifying the source range for variable is done to remove that trailing colon:

```c++
  auto varWithoutColon =
      [](const ast_matchers::MatchFinder::MatchResult &result)
      -> Expected<CharSourceRange> {
    auto range = node("var")(result);
    if (!range)
      return range.takeError();
    range->setEnd(range->getEnd().getLocWithOffset(-1));
    return range;
  };
```