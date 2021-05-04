
#pragma once

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/LambdaResolver.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Mangler.h"
#include "llvm/Target/TargetMachine.h"

namespace pom {

class Jit {
   public:
    using ObjLayerT     = llvm::orc::LegacyRTDyldObjectLinkingLayer;
    using CompileLayerT = llvm::orc::LegacyIRCompileLayer<ObjLayerT, llvm::orc::SimpleCompiler>;
    using ModuleKey     = llvm::orc::VModuleKey;

    Jit();

    llvm::TargetMachine& getTargetMachine() { return *TM; }

    ModuleKey addModule(std::unique_ptr<llvm::Module> M);

    void removeModule(ModuleKey K);

    llvm::JITSymbol findSymbol(const std::string Name) { return findMangledSymbol(mangle(Name)); }

   private:
    std::string mangle(const std::string& Name);

    llvm::JITSymbol findMangledSymbol(const std::string& Name);

    llvm::orc::ExecutionSession                ES;
    std::shared_ptr<llvm::orc::SymbolResolver> Resolver;
    std::unique_ptr<llvm::TargetMachine>       TM;
    const llvm::DataLayout                     DL;
    ObjLayerT                                  ObjectLayer;
    CompileLayerT                              CompileLayer;
    std::vector<ModuleKey>                     ModuleKeys;
};

}  // namespace pom
