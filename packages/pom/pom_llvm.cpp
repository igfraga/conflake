
#include <pom_llvm.h>

#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"

namespace pom {

void initLlvm() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
}

}  // namespace pom
