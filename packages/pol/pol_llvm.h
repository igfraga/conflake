#pragma once

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

namespace pol {

void initLlvm();

llvm::Value* createMalloc(llvm::IRBuilderBase* builder,
                          llvm::Type*          int_type,
                          llvm::Type*          alloc_type,
                          llvm::Value*         alloc_size,
                          llvm::Value*         array_size,
                          llvm::Function*      malloc_fun,
                          const llvm::Twine&   name);

}  // namespace pol
