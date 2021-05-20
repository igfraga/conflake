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
    };
    // clang-format on
    return ops;
};

std::multimap<char, OpInfo> byKeyOps(const std::vector<OpInfo>& ops) {
    std::multimap<char, OpInfo> byk;
    for (auto& op : ops) {
        byk.insert({op.m_op, op});
    }
    return byk;
}

bool matches(const OpInfo& func, const std::vector<TypeCSP>& args) {
    return std::equal(args.begin(), args.end(), func.m_args.begin(),
                      [](auto& x, auto& y) { return *x == *y; });
}

}  // namespace

tl::expected<OpInfo, Err> getOp(char op_key, const std::vector<TypeCSP>& operands) {
    static std::vector<OpInfo>         ops    = makeOps();
    static std::multimap<char, OpInfo> byName = byKeyOps(ops);

    auto fo = byName.find(op_key);
    if (fo == byName.end()) {
        return tl::make_unexpected(Err{fmt::format("Op not found: {0}", op_key)});
    }

    for (; fo != byName.end() && fo->first == op_key; ++fo) {
        if (matches(fo->second, operands)) {
            return fo->second;
        }
    }

    std::ostringstream operands_str;
    for (auto& ty : operands) {
        operands_str << ty->description() << ",";
    }

    return tl::make_unexpected(Err{
        fmt::format("Op not found: {0} with operands of type {1}", op_key, operands_str.str())});
}

}  // namespace ops

}  // namespace pom
