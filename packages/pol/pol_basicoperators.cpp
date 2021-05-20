
#include <fmt/format.h>
#include <pol_basicoperators.h>
#include <pom_basictypes.h>
#include <sstream>

namespace pol {

namespace basicoperators {

using BinaryOpBuilder =
    std::function<llvm::Value*(llvm::IRBuilderBase*, llvm::Value*, llvm::Value*)>;

std::string make_key(const pom::ops::OpKey& op, std::vector<pom::TypeCSP> operands) {
    std::ostringstream oss;
    if(std::holds_alternative<char>(op)) {
        oss << "op" << std::get<char>(op) << "__";
    }
    else if(std::holds_alternative<std::string>(op)) {
        oss << std::get<std::string>(op) << "__";
    }
    for (auto& operand : operands) {
        oss << operand->mangled();
    }
    return oss.str();
}

struct OpTable {
    OpTable() {
        auto real    = pom::types::real();
        auto boolean = pom::types::boolean();
        auto integer = pom::types::integer();

        using namespace std::placeholders;
        m_ops[make_key('+', {real, real})] = std::bind(&OpTable::plus_real, this, _1, _2, _3);
        m_ops[make_key('-', {real, real})] = std::bind(&OpTable::minus_real, this, _1, _2, _3);
        m_ops[make_key('*', {real, real})] = std::bind(&OpTable::multiply_real, this, _1, _2, _3);
        m_ops[make_key('<', {real, real})] = std::bind(&OpTable::lt_real, this, _1, _2, _3);
        m_ops[make_key('>', {real, real})] = std::bind(&OpTable::gt_real, this, _1, _2, _3);

        m_ops[make_key('+', {integer, integer})] = std::bind(&OpTable::plus_int, this, _1, _2, _3);
        m_ops[make_key('-', {integer, integer})] = std::bind(&OpTable::minus_int, this, _1, _2, _3);
        m_ops[make_key('*', {integer, integer})] =
            std::bind(&OpTable::multiply_int, this, _1, _2, _3);
        m_ops[make_key('+', {integer, integer})] = std::bind(&OpTable::plus_int, this, _1, _2, _3);
        m_ops[make_key('-', {integer, integer})] = std::bind(&OpTable::minus_int, this, _1, _2, _3);
        m_ops[make_key('<', {integer, integer})] = std::bind(&OpTable::lt_int, this, _1, _2, _3);
        m_ops[make_key('>', {integer, integer})] = std::bind(&OpTable::gt_int, this, _1, _2, _3);

        m_ops[make_key("or", {boolean, boolean})] = std::bind(&OpTable::or_bool, this, _1, _2, _3);
        m_ops[make_key("and", {boolean, boolean})] = std::bind(&OpTable::and_bool, this, _1, _2, _3);
    }

    llvm::Value* plus_int(llvm::IRBuilderBase* builder, llvm::Value* lv, llvm::Value* rv) {
        return builder->CreateAdd(lv, rv, "addtmp");
    }
    llvm::Value* minus_int(llvm::IRBuilderBase* builder, llvm::Value* lv, llvm::Value* rv) {
        return builder->CreateSub(lv, rv, "subtmp");
    }
    llvm::Value* multiply_int(llvm::IRBuilderBase* builder, llvm::Value* lv, llvm::Value* rv) {
        return builder->CreateMul(lv, rv, "multmp");
    }
    llvm::Value* plus_real(llvm::IRBuilderBase* builder, llvm::Value* lv, llvm::Value* rv) {
        return builder->CreateFAdd(lv, rv, "addtmp");
    }
    llvm::Value* minus_real(llvm::IRBuilderBase* builder, llvm::Value* lv, llvm::Value* rv) {
        return builder->CreateFSub(lv, rv, "subtmp");
    }
    llvm::Value* multiply_real(llvm::IRBuilderBase* builder, llvm::Value* lv, llvm::Value* rv) {
        return builder->CreateFMul(lv, rv, "multmp");
    }

    llvm::Value* lt_real(llvm::IRBuilderBase* builder, llvm::Value* lv, llvm::Value* rv) {
        return builder->CreateFCmpULT(lv, rv, "lttmp");
    }
    llvm::Value* gt_real(llvm::IRBuilderBase* builder, llvm::Value* lv, llvm::Value* rv) {
        return builder->CreateFCmpUGT(lv, rv, "gttmp");
    }
    llvm::Value* lt_int(llvm::IRBuilderBase* builder, llvm::Value* lv, llvm::Value* rv) {
        return builder->CreateICmpULT(lv, rv, "lttmp");
    }
    llvm::Value* gt_int(llvm::IRBuilderBase* builder, llvm::Value* lv, llvm::Value* rv) {
        return builder->CreateICmpUGT(lv, rv, "gttmp");
    }

    llvm::Value* or_bool(llvm::IRBuilderBase* builder, llvm::Value* lv, llvm::Value* rv) {
        return builder->CreateOr(lv, rv, "ortmp");
    }
    llvm::Value* and_bool(llvm::IRBuilderBase* builder, llvm::Value* lv, llvm::Value* rv) {
        return builder->CreateAnd(lv, rv, "andtmp");
    }


    std::map<std::string, BinaryOpBuilder> m_ops;
};

tl::expected<llvm::Value*, Err> buildBinOp(llvm::IRBuilderBase*    builder,
                                           const pom::ops::OpInfo& op_info, llvm::Value* lv,
                                           llvm::Value* rv) {
    static const OpTable ops;

    std::string key = make_key(op_info.m_op, op_info.m_args);

    auto fo = ops.m_ops.find(key);
    if (fo == ops.m_ops.end()) {
        return tl::make_unexpected(Err{fmt::format("invalid binary operator {0}", key)});
    }
    return fo->second(builder, lv, rv);
}

}  // namespace basicoperators

}  // namespace pol
