set(_SOURCE_FILES 
ParallelAttribute.cpp 
ParallelTransformAction.cpp
)

add_llvm_component_library(ParallelClangPlugin
    MODULE ${_SOURCE_FILES}
    PLUGIN_TOOL clang
)