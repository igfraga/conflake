
#include <fmt/format.h>
#include <pol_basicoperators.h>
#include <pom_basictypes.h>
#include <sstream>

namespace pol {

namespace basicoperators {

using BinaryOpBuilder =
    std::function<llvm::Value*(llvm::IRBuilderBase*, const std::vector<llvm::Value*>&)>;

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

struct OpTable {
    OpTable() {
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
    }

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
    llvm::Value* if_real(llvm::IRBuilderBase* builder, const std::vector<llvm::Value*>& vs) {
        return builder->CreateAnd(vs[0], vs[1], "andtmp");
    }

    std::map<std::string, BinaryOpBuilder> m_ops;
};

tl::expected<llvm::Value*, Err> buildBinOp(llvm::IRBuilderBase*             builder,
                                           const pom::ops::OpInfo&          op_info,
                                           const std::vector<llvm::Value*>& operands) {
    static const OpTable ops;

    std::string key = make_key(op_info.m_op, op_info.m_args);

    auto fo = ops.m_ops.find(key);
    if (fo == ops.m_ops.end()) {
        return tl::make_unexpected(Err{fmt::format("invalid binary operator {0}", key)});
    }
    return fo->second(builder, operands);
}

}  // namespace basicoperators

}  // namespace pol
