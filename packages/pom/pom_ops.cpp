#include <pom_ops.h>

#include <pom_basictypes.h>

#include <fmt/format.h>
#include <map>
#include <sstream>

namespace pom {

namespace ops {

namespace {

std::vector<OpInfo> makeOps() {
    auto real    = types::real();
    auto boolean = types::boolean();
    auto integer = types::integer();

    // clang-format off
    std::vector<OpInfo> ops = {
        {'+', {real, real}, real},
        {'+', {integer, integer}, integer},
        {'-', {real, real}, real},
        {'-', {integer, integer}, integer},
        {'*', {real, real}, real},
        {'*', {integer, integer}, integer},

        {'<', {integer, integer}, boolean},
        {'>', {integer, integer}, boolean},
        {'<', {real, real}, boolean},
        {'>', {real, real}, boolean},

        {"or", {boolean, boolean}, boolean},
        {"and", {boolean, boolean}, boolean},

        {"if", {boolean, real, real}, real},
    };
    // clang-format on
    return ops;
};  // namespace ops

std::multimap<OpKey, OpInfo> byKeyOps(const std::vector<OpInfo>& ops) {
    std::multimap<OpKey, OpInfo> byk;
    for (auto& op : ops) {
        byk.insert({op.m_op, op});
    }
    return byk;
}

bool matches(const OpInfo& func, const std::vector<TypeCSP>& args) {
    return std::equal(args.begin(), args.end(), func.m_args.begin(),
                      [](auto& x, auto& y) { return *x == *y; });
}

std::string toStr(const OpKey& key) {
    return std::visit([](auto& x) { return fmt::format("{}", x); }, key);
}

}  // namespace

tl::expected<OpInfo, Err> getBuiltin(OpKey op_key, const std::vector<TypeCSP>& operands) {
    static std::vector<OpInfo>          ops   = makeOps();
    static std::multimap<OpKey, OpInfo> byKey = byKeyOps(ops);

    auto fo = byKey.find(op_key);
    if (fo == byKey.end()) {
        return tl::make_unexpected(Err{fmt::format("Op not found: {0}", toStr(op_key))});
    }

    for (; fo != byKey.end() && fo->first == op_key; ++fo) {
        if (matches(fo->second, operands)) {
            return fo->second;
        }
    }

    std::ostringstream operands_str;
    for (auto& ty : operands) {
        operands_str << ty->description() << ",";
    }

    return tl::make_unexpected(Err{fmt::format("Builtin not found: {0} with operands of type {1}",
                                               toStr(op_key), operands_str.str())});
}

}  // namespace ops

}  // namespace pom
