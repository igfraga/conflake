
#include <pol_llvm.h>

#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

using namespace llvm;

namespace pol {

void initLlvm() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
}

bool IsConstantOne(Value* val) {
    assert(val && "IsConstantOne does not work with nullptr val");
    const ConstantInt* CVal = dyn_cast<ConstantInt>(val);
    return CVal && CVal->isOne();
}

Value* createMalloc(IRBuilderBase* builder, Type* IntPtrTy,
                          Type* AllocTy, Value* AllocSize, Value* ArraySize,
                          Function* MallocF, const Twine& Name) {
    assert(builder);
    // malloc(type) becomes:
    //       bitcast (i8* malloc(typeSize)) to type*
    // malloc(type, arraySize) becomes:
    //       bitcast (i8* malloc(typeSize*arraySize)) to type*
    if (!ArraySize)
        ArraySize = ConstantInt::get(IntPtrTy, 1);
    else if (ArraySize->getType() != IntPtrTy) {
            //ArraySize = CastInst::CreateIntegerCast(ArraySize, IntPtrTy, false, "", InsertAtEnd);
    }

    if (!IsConstantOne(ArraySize)) {
        if (IsConstantOne(AllocSize)) {
            AllocSize = ArraySize;  // Operand * 1 = Operand
        } else if (Constant* CO = dyn_cast<Constant>(ArraySize)) {
            Constant* Scale = ConstantExpr::getIntegerCast(CO, IntPtrTy, false /*ZExt*/);
            // Malloc arg is constant product of type size and array size
            AllocSize = ConstantExpr::getMul(Scale, cast<Constant>(AllocSize));
        } else {
            // Multiply type size by the array size...
            AllocSize = builder->CreateMul(ArraySize, AllocSize, "mallocsize");
        }
    }

    assert(AllocSize->getType() == IntPtrTy && "malloc arg is wrong size");
    // Create the call to Malloc.
    Module*        M          = builder->GetInsertBlock()->getModule();
    Type*          BPTy       = Type::getInt8PtrTy(builder->getContext());
    FunctionCallee MallocFunc = MallocF;
    if (!MallocFunc){
        // prototype malloc as "void *malloc(size_t)"
        MallocFunc = M->getOrInsertFunction("malloc", BPTy, IntPtrTy);
    }
    PointerType* AllocPtrType = PointerType::getUnqual(AllocTy);
    CallInst*    MCall        = nullptr;
    Value* Result       = nullptr;
    ArrayRef<OperandBundleDef> OpB;
    ArrayRef<Value*> args(&AllocSize, 1ull);
    MCall  = builder->CreateCall(MallocFunc, args, "malloccall");
    //MCall  = CallInst::Create(MallocFunc, AllocSize, OpB, "malloccall");
    Result = MCall;
    if (Result->getType() != AllocPtrType) {
        //InsertAtEnd->getInstList().push_back(MCall);
        // Create a cast instruction to convert to the right type...
        //Result = new BitCastInst(MCall, AllocPtrType, Name);
        Result = builder->CreateBitCast(MCall, AllocPtrType, Name);
    }
    MCall->setTailCall();
    if (Function* F = dyn_cast<Function>(MallocFunc.getCallee())) {
        MCall->setCallingConv(F->getCallingConv());
        if (!F->returnDoesNotAlias()) {
            F->setReturnDoesNotAlias();
        }
    }
    assert(!MCall->getType()->isVoidTy() && "Malloc has void return type");

    return Result;
}

}  // namespace pol
