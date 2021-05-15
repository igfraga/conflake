
#include <pom_semantic.h>

#include <fmt/format.h>
#include <map>

#include <pom_basictypes.h>
#include <pom_functiontype.h>

namespace pom {

namespace semantic {

tl::expected<TypeCSP, Err> calculateType(const ast::Expr& expr, const Context& context);

tl::expected<TypeCSP, Err> calculateType(const literals::Boolean&, const Context&) {
    return types::boolean();
}

tl::expected<TypeCSP, Err> calculateType(const literals::Integer&, const Context&) {
    return types::integer();
}

tl::expected<TypeCSP, Err> calculateType(const literals::Real&, const Context&) {
    return types::real();
}

tl::expected<TypeCSP, Err> calculateType(const ast::Literal& lit, const Context& context) {
    return std::visit([&](auto& x) { return calculateType(x, context); }, lit);
}

tl::expected<TypeCSP, Err> calculateType(const ast::Var& var, const Context& context) {
    auto found = context.m_variables.find(var.m_name);
    if (found == context.m_variables.end()) {
        return tl::make_unexpected(
            Err{fmt::format("Variable {0} not found in this context", var.m_name)});
    }
    return found->second;
}

tl::expected<TypeCSP, Err> calculateType(const ast::BinaryExpr& expr, const Context& context) {
    auto lhs_type = calculateType(*expr.m_lhs, context);
    if (!lhs_type) {
        return lhs_type;
    }
    auto rhs_type = calculateType(*expr.m_rhs, context);
    if (!rhs_type) {
        return rhs_type;
    }

    if (**rhs_type != **lhs_type) {
        return tl::make_unexpected(Err{fmt::format("Type error calling '{0}'", expr.m_op)});
    }

    return rhs_type;
}

tl::expected<TypeCSP, Err> calculateType(const ast::Call& call, const Context& context) {
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

tl::expected<TypeCSP, Err> calculateType(const ast::Expr& expr, const Context& context) {
    return std::visit([&](auto& x) { return calculateType(x, context); }, expr.m_val);
}

TypeCSP signatureType(const Signature& sig, TypeCSP ret_type) {
    types::Function fun;
    fun.m_arg_types.resize(sig.m_args.size());
    std::transform(sig.m_args.begin(), sig.m_args.end(), fun.m_arg_types.begin(),
                   [](auto& arg) { return arg.first; });
    fun.m_ret_type = ret_type;
    return std::make_shared<types::Function>(fun);
}

TypeCSP Function::type() const { return signatureType(m_sig, m_return_type); }

tl::expected<Signature, Err> analyze(const ast::Signature& sig, const Context&) {
    Signature sem_sig;
    sem_sig.m_name = sig.m_name;
    for (auto& arg : sig.m_args) {
        auto typ = types::basicTypeFromStr(arg.first);
        if (!typ) {
            return tl::make_unexpected(Err{fmt::format("Unknown type: {0}", arg.first)});
        }
        sem_sig.m_args.push_back({typ, arg.second});
    }
    return sem_sig;
}

tl::expected<Function, Err> analyze(const ast::Function& function, const Context& outer_context) {
    auto sig = analyze(function.m_sig, outer_context);
    if (!sig) {
        return tl::make_unexpected(sig.error());
    }

    auto context = outer_context;
    for (auto& arg : sig->m_args) {
        context.m_variables.insert({arg.second, arg.first});
    }

    // recursion TODO: generalize type
    auto fun_type = signatureType(*sig, types::real());
    context.m_variables.insert({function.m_sig.m_name, fun_type});

    auto ret_type = calculateType(*function.m_code, context);
    if (!ret_type) {
        return tl::make_unexpected(ret_type.error());
    }

    return Function{*sig, function.m_code, context, *ret_type};
}

tl::expected<TopLevel, Err> analyze(const parser::TopLevel& top_level) {
    Context  context;
    TopLevel semantic_top_level;

    for (auto& unit : top_level) {
        if (std::holds_alternative<ast::Signature>(unit)) {
            auto& extrn = std::get<ast::Signature>(unit);
            auto  sig   = analyze(extrn, context);
            if (!sig) {
                return tl::make_unexpected(sig.error());
            }
            context.m_variables.insert({extrn.m_name, signatureType(*sig, types::real())}); // TODO: generalize
            semantic_top_level.push_back(*sig);
        } else if (std::holds_alternative<ast::Function>(unit)) {
            auto& fn = std::get<ast::Function>(unit);

            auto sem_fn = analyze(fn, context);
            if (!sem_fn) {
                return tl::make_unexpected(sem_fn.error());
            }

            semantic_top_level.push_back(*sem_fn);

            context.m_variables.insert({fn.m_sig.m_name, sem_fn->type()});
        }
    }

    return semantic_top_level;
}

void print(std::ostream& ost, const Signature& sig) {
    ost << sig.m_name << " <- ";
    for (auto& [arg_type, arg_name] : sig.m_args) {
        ost << arg_name << ", ";
    }
}

void printTlu(std::ostream& ost, const TopLevelUnit& u) {
    std::visit(
        [&](auto&& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, ast::Signature>) {
                ost << "extern: ";
                ast::print(ost, v);
                ost << "->" << v.type()->returnType()->description();
                ost << "\n";
            } else if constexpr (std::is_same_v<T, Function>) {
                ost << "func: ";
                print(ost, v.m_sig);
                ost << "->" << v.type()->returnType()->description();
                ost << ": ";
                ast::print(ost, *v.m_code);
                ost << "\n";
            }
        },
        u);
}

void print(std::ostream& ost, const TopLevel& top_level) {
    for (auto& e : top_level) {
        printTlu(ost, e);
    }
}

void print(std::ostream& ost, const Context& context) {
    ost << "=========== Context ==========" << std::endl;
    ost << "Variables::" << std::endl;
    for (auto& v : context.m_variables) {
        ost << v.first << ": " << v.second->description() << std::endl;
    }
    ost << "===========---------==========" << std::endl;
}

}  // namespace semantic

}  // namespace pom
