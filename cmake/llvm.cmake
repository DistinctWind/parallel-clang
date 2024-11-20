include(ExternalProject)

ExternalProject_Add(llvm-project
    PREFIX llvm-project
    GIT_REPOSITORY https://mirrors.bfsu.edu.cn/git/llvm-project.git
    GIT_TAG main
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    GIT_CONFIG remote.origin.fetch='^refs/heads/users/*' remote.origin.fetch='^refs/heads/revert-*'
    SOURCE_SUBDIR llvm
    CMAKE_ARGS
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/llvm-install
        -DCMAKE_BUILD_TYPE=Release
        -DLLVM_ENABLE_PROJECTS=clang
    BUILD_COMMAND ${CMAKE_COMMAND} --build . --target all
    INSTALL_COMMAND ${CMAKE_COMMAND} --build . --target install
)

set(LLVM_DIR ${CMAKE_BINARY_DIR}/llvm-install/lib/cmake/llvm)