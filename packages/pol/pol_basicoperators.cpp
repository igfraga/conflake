
#include <fmt/format.h>
#include <pol_basicoperators.h>
#include <pom_basictypes.h>
#include <sstream>

namespace pol {

namespace basicoperators {

std::string make_key(const pom::ops::OpKey& op, std::vector<pom::TypeCSP> operands) {
    std::ostringstream oss;
    if (std::holds_alternative<char>(op)) {
        oss << "op" << std::get<char>(op) << "__";
    } else if (std::holds_alternative<std::string>(op)) {
        oss << std::get<std::string>(op) << "__";
    }
    for (auto& operand : operands) {
        oss << operand->mangled();
    }
    return oss.str();
}

class OpTable {
   public:
    OpTable();

    tl::expected<llvm::Value*, Err> generate(llvm::IRBuilderBase*               builder,
                                             const pom::ops::OpInfo&            op_info,
                                             const std::vector<ValueGenerator>& operands) const;

   private:
    llvm::Value* plus_int(llvm::IRBuilderBase* builder, const std::vector<llvm::Value*>& vs) {
        return builder->CreateAdd(vs[0], vs[1], "addtmp");
    }
    llvm::Value* minus_int(llvm::IRBuilderBase* builder, const std::vector<llvm::Value*>& vs) {
        return builder->CreateSub(vs[0], vs[1], "subtmp");
    }
    llvm::Value* multiply_int(llvm::IRBuilderBase* builder, const std::vector<llvm::Value*>& vs) {
        return builder->CreateMul(vs[0], vs[1], "multmp");
    }
    llvm::Value* plus_real(llvm::IRBuilderBase* builder, const std::vector<llvm::Value*>& vs) {
        return builder->CreateFAdd(vs[0], vs[1], "addtmp");
    }
    llvm::Value* minus_real(llvm::IRBuilderBase* builder, const std::vector<llvm::Value*>& vs) {
        return builder->CreateFSub(vs[0], vs[1], "subtmp");
    }
    llvm::Value* multiply_real(llvm::IRBuilderBase* builder, const std::vector<llvm::Value*>& vs) {
        return builder->CreateFMul(vs[0], vs[1], "multmp");
    }

    llvm::Value* lt_real(llvm::IRBuilderBase* builder, const std::vector<llvm::Value*>& vs) {
        return builder->CreateFCmpULT(vs[0], vs[1], "lttmp");
    }
    llvm::Value* gt_real(llvm::IRBuilderBase* builder, const std::vector<llvm::Value*>& vs) {
        return builder->CreateFCmpUGT(vs[0], vs[1], "gttmp");
    }
    llvm::Value* lt_int(llvm::IRBuilderBase* builder, const std::vector<llvm::Value*>& vs) {
        return builder->CreateICmpULT(vs[0], vs[1], "lttmp");
    }
    llvm::Value* gt_int(llvm::IRBuilderBase* builder, const std::vector<llvm::Value*>& vs) {
        return builder->CreateICmpUGT(vs[0], vs[1], "gttmp");
    }

    llvm::Value* or_bool(llvm::IRBuilderBase* builder, const std::vector<llvm::Value*>& vs) {
        return builder->CreateOr(vs[0], vs[1], "ortmp");
    }
    llvm::Value* and_bool(llvm::IRBuilderBase* builder, const std::vector<llvm::Value*>& vs) {
        return builder->CreateAnd(vs[0], vs[1], "andtmp");
    }
    tl::expected<llvm::Value*, Err> if_real(llvm::IRBuilderBase*               builder,
                                            const std::vector<ValueGenerator>& vs) {
        auto& ctx = builder->getContext();
        auto  bv  = vs[0](builder);
        if (!bv) {
            return bv;
        }
        auto cmp = builder->CreateICmpEQ(*bv, llvm::ConstantInt::getTrue(ctx));
        auto fn  = builder->GetInsertBlock()->getParent();

        llvm::BasicBlock* then_bb  = llvm::BasicBlock::Create(ctx, "then", fn);
        llvm::BasicBlock* else_bb  = llvm::BasicBlock::Create(ctx, "else");
        llvm::BasicBlock* merge_bb = llvm::BasicBlock::Create(ctx, "ifcont");
        builder->CreateCondBr(cmp, then_bb, else_bb);

        builder->SetInsertPoint(then_bb);

        auto lv = vs[1](builder);
        if (!lv) {
            return lv;
        }

        builder->CreateBr(merge_bb);

        then_bb = builder->GetInsertBlock();

        fn->getBasicBlockList().push_back(else_bb);
        builder->SetInsertPoint(else_bb);

        auto rv = vs[2](builder);
        if (!rv) {
            return rv;
        }

        builder->CreateBr(merge_bb);
        else_bb = builder->GetInsertBlock();

        fn->getBasicBlockList().push_back(merge_bb);
        builder->SetInsertPoint(merge_bb);
        llvm::PHINode* phi =
            builder->CreatePHI(llvm::Type::getDoubleTy(builder->getContext()), 2, "iftmp");

        phi->addIncoming(*lv, then_bb);
        phi->addIncoming(*rv, else_bb);
        return phi;
    }

    using BinaryOpBuilder =
        std::function<llvm::Value*(llvm::IRBuilderBase*, const std::vector<llvm::Value*>&)>;
    using AdvBinaryOpBuilder = std::function<tl::expected<llvm::Value*, Err>(
        llvm::IRBuilderBase*, const std::vector<ValueGenerator>&)>;

    std::map<std::string, BinaryOpBuilder>    m_ops;
    std::map<std::string, AdvBinaryOpBuilder> m_adv_ops;
};

OpTable::OpTable() {
    auto real    = pom::types::real();
    auto boolean = pom::types::boolean();
    auto integer = pom::types::integer();

    using namespace std::placeholders;
    m_ops[make_key('+', {real, real})] = std::bind(&OpTable::plus_real, this, _1, _2);
    m_ops[make_key('-', {real, real})] = std::bind(&OpTable::minus_real, this, _1, _2);
    m_ops[make_key('*', {real, real})] = std::bind(&OpTable::multiply_real, this, _1, _2);
    m_ops[make_key('<', {real, real})] = std::bind(&OpTable::lt_real, this, _1, _2);
    m_ops[make_key('>', {real, real})] = std::bind(&OpTable::gt_real, this, _1, _2);

    m_ops[make_key('+', {integer, integer})] = std::bind(&OpTable::plus_int, this, _1, _2);
    m_ops[make_key('-', {integer, integer})] = std::bind(&OpTable::minus_int, this, _1, _2);
    m_ops[make_key('*', {integer, integer})] = std::bind(&OpTable::multiply_int, this, _1, _2);
    m_ops[make_key('+', {integer, integer})] = std::bind(&OpTable::plus_int, this, _1, _2);
    m_ops[make_key('-', {integer, integer})] = std::bind(&OpTable::minus_int, this, _1, _2);
    m_ops[make_key('<', {integer, integer})] = std::bind(&OpTable::lt_int, this, _1, _2);
    m_ops[make_key('>', {integer, integer})] = std::bind(&OpTable::gt_int, this, _1, _2);

    m_ops[make_key("or", {boolean, boolean})]  = std::bind(&OpTable::or_bool, this, _1, _2);
    m_ops[make_key("and", {boolean, boolean})] = std::bind(&OpTable::and_bool, this, _1, _2);

    m_adv_ops[make_key("if", {boolean, real, real})] = std::bind(&OpTable::if_real, this, _1, _2);
}

tl::expected<llvm::Value*, Err> OpTable::generate(
    llvm::IRBuilderBase* builder, const pom::ops::OpInfo& op_info,
    const std::vector<ValueGenerator>& operands) const {
    auto key = make_key(op_info.m_op, op_info.m_args);
    auto fo  = m_ops.find(key);
    if (fo == m_ops.end()) {
        // try advanced
        auto fo_adv = m_adv_ops.find(key);
        if (fo_adv == m_adv_ops.end()) {
            return tl::make_unexpected(Err{fmt::format("invalid binary operator {0}", key)});
        }
        return fo_adv->second(builder, operands);
    }

    auto values = execute(operands, builder);
    if (!values) {
        return tl::make_unexpected(values.error());
    }

    return fo->second(builder, *values);
}

tl::expected<llvm::Value*, Err> buildBinOp(llvm::IRBuilderBase*               builder,
                                           const pom::ops::OpInfo&            op_info,
                                           const std::vector<ValueGenerator>& operands) {
    static const OpTable ops;
    return ops.generate(builder, op_info, operands);
}

tl::expected<std::vector<llvm::Value*>, Err> execute(const std::vector<ValueGenerator>& operands,
                                                     llvm::IRBuilderBase*               builder) {
    std::vector<llvm::Value*> values;
    for (auto& vf : operands) {
        auto v = vf(builder);
        if (!v) {
            return tl::make_unexpected(v.error());
        }
        assert(*v);
        values.push_back(*v);
    }
    return values;
}

}  // namespace basicoperators

}  // namespace pol
