
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

bool isConstantOne(Value* val) {
    assert(val && "IsConstantOne does not work with nullptr val");
    const ConstantInt* CVal = dyn_cast<ConstantInt>(val);
    return CVal && CVal->isOne();
}

Value* createMalloc(IRBuilderBase* builder, Type* int_type, Type* alloc_ty, Value* alloc_size,
                    Value* array_size, Function* malloc_fun, const Twine& name) {
    assert(builder);
    // malloc(type) becomes:
    //       bitcast (i8* malloc(typeSize)) to type*
    // malloc(type, arraySize) becomes:
    //       bitcast (i8* malloc(typeSize*arraySize)) to type*
    if (!array_size) {
        array_size = ConstantInt::get(int_type, 1);
    } else if (array_size->getType() != int_type) {
        // ArraySize = CastInst::CreateIntegerCast(ArraySize, IntPtrTy, false, "", InsertAtEnd);
    }

    if (!isConstantOne(array_size)) {
        if (isConstantOne(alloc_size)) {
            alloc_size = array_size;  // Operand * 1 = Operand
        } else if (Constant* CO = dyn_cast<Constant>(array_size)) {
            Constant* Scale = ConstantExpr::getIntegerCast(CO, int_type, false /*ZExt*/);
            // Malloc arg is constant product of type size and array size
            alloc_size = ConstantExpr::getMul(Scale, cast<Constant>(alloc_size));
        } else {
            // Multiply type size by the array size...
            alloc_size = builder->CreateMul(array_size, alloc_size, "mallocsize");
        }
    }

    assert(alloc_size->getType() == int_type && "malloc arg is wrong size");
    // Create the call to Malloc.
    Module*        mod         = builder->GetInsertBlock()->getModule();
    Type*          ptr_type    = Type::getInt8PtrTy(builder->getContext());
    FunctionCallee malloc_func = malloc_fun;
    if (!malloc_func) {
        // prototype malloc as "void *malloc(size_t)"
        malloc_func = mod->getOrInsertFunction("malloc", ptr_type, int_type);
    }
    PointerType*     alloc_ptr_ty = PointerType::getUnqual(alloc_ty);
    CallInst*        mcall        = nullptr;
    Value*           result       = nullptr;
    ArrayRef<Value*> args(&alloc_size, 1ull);
    mcall = builder->CreateCall(malloc_func, args, "malloccall");
    // MCall  = CallInst::Create(MallocFunc, AllocSize, OpB, "malloccall");
    result = mcall;
    if (result->getType() != alloc_ptr_ty) {
        // InsertAtEnd->getInstList().push_back(MCall);
        // Create a cast instruction to convert to the right type...
        // Result = new BitCastInst(MCall, AllocPtrType, Name);
        result = builder->CreateBitCast(mcall, alloc_ptr_ty, name);
    }
    mcall->setTailCall();
    if (Function* fun = dyn_cast<Function>(malloc_func.getCallee())) {
        mcall->setCallingConv(fun->getCallingConv());
        if (!fun->returnDoesNotAlias()) {
            fun->setReturnDoesNotAlias();
        }
    }
    assert(!mcall->getType()->isVoidTy() && "Malloc has void return type");

    return result;
}

}  // namespace pol
