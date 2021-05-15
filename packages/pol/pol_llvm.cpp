
#include <pol_llvm.h>

#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"

namespace pol {

void initLlvm() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
}

}  // namespace pol
