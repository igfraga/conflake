
#include <pol_jit.h>

#include <fmt/format.h>

namespace pol {

Jit::Jit(std::unique_ptr<llvm::orc::ExecutionSession> execution_session,
         llvm::orc::JITTargetMachineBuilder           jtmb,
         llvm::DataLayout                             data_layout)
    : m_execution_session(std::move(execution_session)),
      m_data_layout(std::move(data_layout)),
      m_mangle(*this->m_execution_session, this->m_data_layout),
      m_object_layer(*this->m_execution_session, []() { return std::make_unique<llvm::SectionMemoryManager>(); }),
      m_compile_layer(*this->m_execution_session, m_object_layer, std::make_unique<llvm::orc::ConcurrentIRCompiler>(std::move(jtmb))),
      m_main_jd(this->m_execution_session->createBareJITDylib("<main>"))
{
    m_main_jd.addGenerator(
        cantFail(llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(m_data_layout.getGlobalPrefix())));
}

Jit::~Jit()
{
    if (auto Err = m_execution_session->endSession()) {
        m_execution_session->reportError(std::move(Err));
    }
}

std::unique_ptr<Jit> Jit::Create()
{
    auto epc = llvm::orc::SelfExecutorProcessControl::Create();
    if (!epc) {
        assert(0);
        return nullptr;
    }

    auto execution_session = std::make_unique<llvm::orc::ExecutionSession>(std::move(*epc));

    llvm::orc::JITTargetMachineBuilder jtmb(execution_session->getExecutorProcessControl().getTargetTriple());

    auto dl = jtmb.getDefaultDataLayoutForTarget();
    if (!dl) {
        assert(0);
        return nullptr;
    }

    return std::make_unique<Jit>(std::move(execution_session), std::move(jtmb), std::move(*dl));
}

tl::expected<void, Jit::Err> Jit::addModule(llvm::orc::ThreadSafeModule tsm, llvm::orc::ResourceTrackerSP resource_tracker)
{
    if (!resource_tracker) {
        resource_tracker = m_main_jd.getDefaultResourceTracker();
    }
    auto error = m_compile_layer.add(resource_tracker, std::move(tsm));
    if(error) {
        return tl::make_unexpected(Err{"Failed to add module"});
    }
    return {};
}

tl::expected<llvm::JITEvaluatedSymbol, Jit::Err> Jit::lookup(llvm::StringRef name)
{
    auto found = m_execution_session->lookup({&m_main_jd}, m_mangle(name.str()));
    if(!found) {
        return tl::make_unexpected(Err{fmt::format("Error finding symbol: {0}", name)});
    }
    return found.get();
}

}  // namespace pol
