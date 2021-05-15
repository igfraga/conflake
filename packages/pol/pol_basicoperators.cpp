
#include <fmt/format.h>
#include <pol_basicoperators.h>

namespace pol {

namespace basicoperators {

using BinaryOpBuilder =
    std::function<llvm::Value*(llvm::IRBuilderBase*, llvm::Value*, llvm::Value*)>;

struct IntegerOpTable {
    IntegerOpTable() {
        using namespace std::placeholders;
        m_ops['+'] = std::bind(&IntegerOpTable::plus, this, _1, _2, _3);
        m_ops['-'] = std::bind(&IntegerOpTable::minus, this, _1, _2, _3);
        m_ops['*'] = std::bind(&IntegerOpTable::multiply, this, _1, _2, _3);
    }

    llvm::Value* plus(llvm::IRBuilderBase* builder, llvm::Value* lv, llvm::Value* rv) {
        return builder->CreateAdd(lv, rv, "addtmp");
    }
    llvm::Value* minus(llvm::IRBuilderBase* builder, llvm::Value* lv, llvm::Value* rv) {
        return builder->CreateSub(lv, rv, "subtmp");
    }
    llvm::Value* multiply(llvm::IRBuilderBase* builder, llvm::Value* lv, llvm::Value* rv) {
        return builder->CreateMul(lv, rv, "multmp");
    }

    std::map<char, BinaryOpBuilder> m_ops;
};

struct RealOpTable {
    RealOpTable() {
        using namespace std::placeholders;
        m_ops['+'] = std::bind(&RealOpTable::plus, this, _1, _2, _3);
        m_ops['-'] = std::bind(&RealOpTable::minus, this, _1, _2, _3);
        m_ops['*'] = std::bind(&RealOpTable::multiply, this, _1, _2, _3);
    }

    llvm::Value* plus(llvm::IRBuilderBase* builder, llvm::Value* lv, llvm::Value* rv) {
        return builder->CreateFAdd(lv, rv, "addtmp");
    }
    llvm::Value* minus(llvm::IRBuilderBase* builder, llvm::Value* lv, llvm::Value* rv) {
        return builder->CreateFSub(lv, rv, "subtmp");
    }
    llvm::Value* multiply(llvm::IRBuilderBase* builder, llvm::Value* lv, llvm::Value* rv) {
        return builder->CreateFMul(lv, rv, "multmp");
    }

    std::map<char, BinaryOpBuilder> m_ops;
};

tl::expected<llvm::Value*, Err> buildBinOp(llvm::IRBuilderBase* builder, char op,
                                           const pom::Type& type, llvm::Value* lv,
                                           llvm::Value* rv) {
    static const IntegerOpTable int_ops;
    static const RealOpTable    real_ops;

    auto mkerr = [&]() {
        return Err{fmt::format("invalid binary operator {0} on {1}", op, type.description())};
    };

    const std::map<char, BinaryOpBuilder>* ops;
    if (type.mangled() == "integer") {
        ops = &int_ops.m_ops;
    } else if (type.mangled() == "real") {
        ops = &real_ops.m_ops;
    } else {
        return tl::make_unexpected(mkerr());
    }

    auto fo = ops->find(op);
    if (fo == ops->end()) {
        return tl::make_unexpected(mkerr());
    }
    return fo->second(builder, lv, rv);
}

}  // namespace basicoperators

}  // namespace pol
