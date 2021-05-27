
#include <pol_jit.h>

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/iterator_range.h"
#include "llvm/ExecutionEngine/RTDyldMemoryManager.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/raw_ostream.h"

namespace pol {

Jit::Jit()
    : m_resolver(llvm::orc::createLegacyLookupResolver(
          m_es,
          [this](llvm::StringRef Name) { return findMangledSymbol(std::string(Name)); },
          [](llvm::Error Err) { cantFail(std::move(Err), "lookupFlags failed"); })),
      m_tm(llvm::EngineBuilder().selectTarget()),
      m_dl(m_tm->createDataLayout()),
      m_object_layer(llvm::AcknowledgeORCv1Deprecation,
                     m_es,
                     [this](ModuleKey) {
                         return ObjLayerT::Resources{std::make_shared<llvm::SectionMemoryManager>(),
                                                     m_resolver};
                     }),
      m_compile_layer(
          llvm::AcknowledgeORCv1Deprecation, m_object_layer, llvm::orc::SimpleCompiler(*m_tm))
{
    llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
}

Jit::ModuleKey Jit::addModule(std::unique_ptr<llvm::Module> mod)
{
    auto key = m_es.allocateVModule();
    cantFail(m_compile_layer.addModule(key, std::move(mod)));
    m_module_keys.push_back(key);
    return key;
}

void Jit::removeModule(ModuleKey key)
{
    m_module_keys.erase(llvm::find(m_module_keys, key));
    cantFail(m_compile_layer.removeModule(key));
}

std::string Jit::mangle(const std::string& name)
{
    std::string mangled_name;
    {
        llvm::raw_string_ostream mangled_name_stream(mangled_name);
        llvm::Mangler::getNameWithPrefix(mangled_name_stream, name, m_dl);
    }
    return mangled_name;
}

llvm::JITSymbol Jit::findMangledSymbol(const std::string& name)
{
#ifdef _WIN32
    // The symbol lookup of ObjectLinkingLayer uses the SymbolRef::SF_Exported
    // flag to decide whether a symbol will be visible or not, when we call
    // IRCompileLayer::findSymbolIn with ExportedSymbolsOnly set to true.
    //
    // But for Windows COFF objects, this flag is currently never set.
    // For a potential solution see: https://reviews.llvm.org/rL258665
    // For now, we allow non-exported symbols on Windows as a workaround.
    const bool exported_symbols_only = false;
#else
    const bool exported_symbols_only = true;
#endif

    // Search modules in reverse order: from last added to first added.
    // This is the opposite of the usual search order for dlsym, but makes more
    // sense in a REPL where we want to bind to the newest available definition.
    for (auto h : llvm::make_range(m_module_keys.rbegin(), m_module_keys.rend()))
        if (auto sym = m_compile_layer.findSymbolIn(h, name, exported_symbols_only)) {
            return sym;
        }

    // If we can't find the symbol in the JIT, try looking in the host process.
    if (auto sym_addr = llvm::RTDyldMemoryManager::getSymbolAddressInProcess(name)) {
        return llvm::JITSymbol(sym_addr, llvm::JITSymbolFlags::Exported);
    }

#ifdef _WIN32
    // For Windows retry without "_" at beginning, as RTDyldMemoryManager uses
    // GetProcAddress and standard libraries like msvcrt.dll use names
    // with and without "_" (for example "_itoa" but "sin").
    if (name.length() > 2 && name[0] == '_')
        if (auto sym_addr = RTDyldMemoryManager::getSymbolAddressInProcess(name.substr(1)))
            return JITSymbol(sym_addr, JITSymbolFlags::Exported);
#endif

    return nullptr;
}

}  // namespace pol
