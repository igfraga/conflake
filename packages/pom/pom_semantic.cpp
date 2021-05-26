
#include <pom_semantic.h>

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <map>

#include <pom_basictypes.h>
#include <pom_functiontype.h>
#include <pom_listtype.h>
#include <pom_ops.h>
#include <pom_typebuilder.h>

#include <cassert>

namespace pom {

namespace semantic {

namespace {

tl::expected<TypeCSP, Err> calculateType(const ast::Expr& expr, Context& context);

tl::expected<TypeCSP, Err> calculateType(const literals::Boolean&, Context&) {
    return types::boolean();
}

tl::expected<TypeCSP, Err> calculateType(const literals::Integer&, Context&) {
    return types::integer();
}

tl::expected<TypeCSP, Err> calculateType(const literals::Real&, Context&) { return types::real(); }

tl::expected<TypeCSP, Err> calculateType(const ast::Literal& lit, Context& context) {
    return std::visit([&](auto& x) { return calculateType(x, context); }, lit);
}

tl::expected<TypeCSP, Err> calculateType(const ast::Var& var, Context& context) {
    auto found = context.m_variables.find(var.m_name);
    if (found == context.m_variables.end()) {
        return tl::make_unexpected(
            Err{fmt::format("Variable {0} not found in this context", var.m_name)});
    }
    auto ty = found->second;
    if (var.m_subscript) {
        ty = ty->subscriptedType(*var.m_subscript);
    }
    return ty;
}

tl::expected<TypeCSP, Err> calculateType(const ast::ListExpr& li, Context& context) {
    TypeCSP ty;
    for (auto& expr : li.m_expressions) {
        auto res = calculateType(*expr, context);
        if (!res) {
            return res;
        }
        assert(*res);
        if (!ty) {
            ty = *res;
        }

        if (*ty != **res) {
            return tl::make_unexpected(
                Err{fmt::format("Mismatching types in lists, found {0} and {1}", ty->description(),
                                (*res)->description())});
        }
        auto ins = context.m_expressions.insert({expr->m_id, ty});
        if (!ins.second) {
            assert(0);
            return tl::make_unexpected(Err{"Duplicated expression id."});
        }
    }
    return std::make_shared<types::List>(ty);
}

tl::expected<TypeCSP, Err> calculateType(const ast::BinaryExpr& expr, Context& context) {
    auto lhs_type = calculateType(*expr.m_lhs, context);
    if (!lhs_type) {
        return lhs_type;
    }
    auto rhs_type = calculateType(*expr.m_rhs, context);
    if (!rhs_type) {
        return rhs_type;
    }

    auto ins_l = context.m_expressions.insert({expr.m_lhs->m_id, *lhs_type});
    auto ins_r = context.m_expressions.insert({expr.m_rhs->m_id, *rhs_type});
    if (!ins_l.second || !ins_r.second) {
        assert(0);
        return tl::make_unexpected(Err{"Duplicated expression id."});
    }

    auto op = ops::getBuiltin(expr.m_op, {*lhs_type, *rhs_type});
    if (!op) {
        return tl::make_unexpected(Err{op.error().m_desc});
    }
    return op->m_ret_type;
}

tl::expected<TypeCSP, Err> calculateType(const ast::Call& call, Context& context) {
    std::vector<TypeCSP> arg_types;
    for (auto& arg : call.m_args) {
        auto arg_ty = calculateType(*arg, context);
        if (!arg_ty) {
            return arg_ty;
        }
        arg_types.push_back(*arg_ty);

        auto ins = context.m_expressions.insert({arg->m_id, *arg_ty});
        if (!ins.second) {
            assert(0);
            return tl::make_unexpected(Err{"Duplicated expression id."});
        }
    }

    auto builtin = ops::getBuiltin(call.m_function, arg_types);
    if (builtin) {
        return builtin->m_ret_type;
    }

    auto found = context.m_variables.find(call.m_function);
    if (found == context.m_variables.end()) {
        return tl::make_unexpected(
            Err{fmt::format("Function {0} not found in this context", call.m_function)});
    }
    if (!found->second->returnType()) {
        return tl::make_unexpected(Err{fmt::format("{0} is not callable", call.m_function)});
    }
    return found->second->returnType();
}

tl::expected<TypeCSP, Err> calculateType(const ast::Expr& expr, Context& context) {
    return std::visit([&](auto& x) { return calculateType(x, context); }, expr.m_val);
}

TypeCSP signatureType(const Signature& sig) {
    types::Function fun;
    fun.m_arg_types.resize(sig.m_args.size());
    std::transform(sig.m_args.begin(), sig.m_args.end(), fun.m_arg_types.begin(),
                   [](auto& arg) { return arg.first; });
    fun.m_ret_type = sig.m_return_type;
    return std::make_shared<types::Function>(fun);
}

}  // namespace

TypeCSP Function::type() const { return signatureType(m_sig); }

tl::expected<Signature, Err> analyze(const ast::Signature& sig, Context&) {
    Signature sem_sig;
    sem_sig.m_name = sig.m_name;
    for (auto& arg : sig.m_args) {
        auto typ = types::build(*arg.m_type);
        if (!typ) {
            return tl::make_unexpected(Err{fmt::format("Unknown type: {0}", *arg.m_type)});
        }
        sem_sig.m_args.push_back({typ, arg.m_name});
    }
    if (sig.m_ret_type) {
        sem_sig.m_return_type = types::build(*sig.m_ret_type);
    }
    return sem_sig;
}

tl::expected<Function, Err> analyze(const ast::Function& function, Context& outer_context) {
    auto sig = analyze(function.m_sig, outer_context);
    if (!sig) {
        return tl::make_unexpected(sig.error());
    }

    auto context = outer_context;
    for (auto& arg : sig->m_args) {
        context.m_variables.insert({arg.second, arg.first});
    }

    if (sig->m_return_type) {
        // allow recursion
        context.m_variables.insert({function.m_sig.m_name, signatureType(*sig)});
    }

    auto ret_type = calculateType(*function.m_code, context);
    if (!ret_type) {
        return tl::make_unexpected(ret_type.error());
    }

    assert(*ret_type);
    if (sig->m_return_type) {
        if (*sig->m_return_type != **ret_type) {
            return tl::make_unexpected(
                Err{fmt::format("Function declared return type {0} but evaluated to type {1}",
                                sig->m_return_type->description(), (*ret_type)->description())});
        }
    } else {
        sig->m_return_type = *ret_type;
    }

    auto ins = context.m_expressions.insert({function.m_code->m_id, *ret_type});
    if(!ins.second) {
        assert(0);
        return tl::make_unexpected(Err{"Duplicated expression id."});
    }
    return Function{*sig, function.m_code, context};
}

tl::expected<TopLevelUnit, Err> analyzeExtern(const ast::Signature& extrn, Context& context) {
    if (!extrn.m_ret_type) {
        return tl::make_unexpected(
            Err{fmt::format("extern function {0} does not specify return type", extrn.m_name)});
    }

    auto sig = analyze(extrn, context);
    if (!sig) {
        return tl::make_unexpected(sig.error());
    }
    assert(sig->m_return_type);
    context.m_variables.insert({extrn.m_name, signatureType(*sig)});
    return *sig;
}

tl::expected<TopLevel, Err> analyze(const parser::TopLevel& top_level) {
    Context  context;
    TopLevel semantic_top_level;

    for (auto& unit : top_level) {
        if (std::holds_alternative<ast::Signature>(unit)) {
            auto tlu = analyzeExtern(std::get<ast::Signature>(unit), context);
            if (!tlu) {
                return tl::make_unexpected(tlu.error());
            }
            semantic_top_level.push_back(*tlu);
        } else if (std::holds_alternative<ast::Function>(unit)) {
            auto sem_fn = analyze(std::get<ast::Function>(unit), context);
            if (!sem_fn) {
                return tl::make_unexpected(sem_fn.error());
            }

            semantic_top_level.push_back(*sem_fn);

            context.m_variables.insert({sem_fn->m_sig.m_name, sem_fn->type()});
        }
    }

    return semantic_top_level;
}

tl::expected<TypeCSP, Err> Context::expressionType(ast::ExprId id) const
{
    auto fo = m_expressions.find(id);
    if(fo == m_expressions.end()) {
        return tl::make_unexpected(Err{fmt::format("Expression id not found: {0}", id)});
    }
    return fo->second;
}

tl::expected<TypeCSP, Err> Context::variableType(std::string_view name) const
{
    auto fo = m_variables.find(std::string(name));
    if(fo == m_variables.end()) {
        return tl::make_unexpected(Err{fmt::format("Variable not found: {0}", name)});
    }
    return fo->second;
}

std::ostream& operator<<(std::ostream& ost, const Signature& sig) {
    ost << sig.m_name << " <- ";
    for (auto& [arg_type, arg_name] : sig.m_args) {
        ost << arg_name << ", ";
    }
    return ost;
}

std::ostream& print(std::ostream& ost, const TopLevelUnit& u) {
    std::visit(
        [&](auto&& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, ast::Signature>) {
                ost << "extern: " << v;
                ost << "->" << v.type()->returnType()->description();
            } else if constexpr (std::is_same_v<T, Function>) {
                ost << "func: " << v.m_sig;
                ost << "->" << v.type()->returnType()->description();
                ost << ": " << *v.m_code;
            }
        },
        u);
    return ost;
}

std::ostream& print(std::ostream& ost, const TopLevel& top_level) {
    for (auto& e : top_level) {
        print(ost, e) << "\n";
    }
    return ost;
}

std::ostream& operator<<(std::ostream& ost, const Context& context) {
    ost << "=========== Context ==========" << std::endl;
    ost << "Variables::" << std::endl;
    for (auto& v : context.m_variables) {
        ost << v.first << ": " << v.second->description() << std::endl;
    }
    ost << "Expressions::" << std::endl;
    for (auto& e : context.m_expressions) {
        ost << e.first << ": " << e.second->description() << std::endl;
    }
    ost << "===========---------==========" << std::endl;
    return ost;
}

}  // namespace semantic

}  // namespace pom
