
#pragma once

#include <memory>
#include <tl/expected.hpp>

#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"

namespace pol {

class Jit
{
   public:
    struct Err
    {
        std::string m_desc;
    };

    Jit(std::unique_ptr<llvm::orc::ExecutionSession> execution_session,
        llvm::orc::JITTargetMachineBuilder           jtmb,
        llvm::DataLayout                             data_layout);

    ~Jit();

    static std::unique_ptr<Jit> Create();

    const llvm::DataLayout& getDataLayout() const { return m_data_layout; }

    llvm::orc::JITDylib& getMainJITDylib() { return m_main_jd; }

    tl::expected<void, Err> addModule(llvm::orc::ThreadSafeModule  tsm,
                                      llvm::orc::ResourceTrackerSP rt = nullptr);

    tl::expected<llvm::JITEvaluatedSymbol, Err> lookup(llvm::StringRef name);

   private:
    std::unique_ptr<llvm::orc::ExecutionSession> m_execution_session;
    llvm::DataLayout                             m_data_layout;
    llvm::orc::MangleAndInterner                 m_mangle;
    llvm::orc::RTDyldObjectLinkingLayer          m_object_layer;
    llvm::orc::IRCompileLayer                    m_compile_layer;
    llvm::orc::JITDylib&                         m_main_jd;
};

}  // namespace pol
