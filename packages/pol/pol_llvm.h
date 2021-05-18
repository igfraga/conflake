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

llvm::Value* createMalloc(llvm::IRBuilderBase* builder, llvm::Type* IntPtrTy, llvm::Type* AllocTy,
                          llvm::Value* AllocSize, llvm::Value* ArraySize, llvm::Function* MallocF,
                          const llvm::Twine& Name);

}  // namespace pol
