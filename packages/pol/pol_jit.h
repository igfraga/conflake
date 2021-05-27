
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

namespace pol {

class Jit
{
   public:
    using ObjLayerT     = llvm::orc::LegacyRTDyldObjectLinkingLayer;
    using CompileLayerT = llvm::orc::LegacyIRCompileLayer<ObjLayerT, llvm::orc::SimpleCompiler>;
    using ModuleKey     = llvm::orc::VModuleKey;

    Jit();

    llvm::TargetMachine& getTargetMachine() { return *m_tm; }

    ModuleKey addModule(std::unique_ptr<llvm::Module> M);

    void removeModule(ModuleKey K);

    llvm::JITSymbol findSymbol(const std::string Name) { return findMangledSymbol(mangle(Name)); }

   private:
    std::string mangle(const std::string& Name);

    llvm::JITSymbol findMangledSymbol(const std::string& Name);

    llvm::orc::ExecutionSession                m_es;
    std::shared_ptr<llvm::orc::SymbolResolver> m_resolver;
    std::unique_ptr<llvm::TargetMachine>       m_tm;
    const llvm::DataLayout                     m_dl;
    ObjLayerT                                  m_object_layer;
    CompileLayerT                              m_compile_layer;
    std::vector<ModuleKey>                     m_module_keys;
};

}  // namespace pol
