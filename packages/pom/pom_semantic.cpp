
#include <pom_semantic.h>

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <map>

#include <pom_basictypes.h>
#include <pom_functiontype.h>
#include <pom_typebuilder.h>
#include <pom_listtype.h>
#include <pom_ops.h>

#include <cassert>

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
    auto ty = found->second;
    if (var.m_subscript) {
        ty = ty->subscriptedType(*var.m_subscript);
    }
    return ty;
}

tl::expected<TypeCSP, Err> calculateType(const ast::ListExpr& li, const Context& context) {
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
    }
    return std::make_shared<types::List>(ty);
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

    auto op = ops::getOp(expr.m_op, {*lhs_type, *rhs_type});
    if(!op) {
        return tl::make_unexpected(Err{op.error().m_desc});
    }
    return op->m_ret_type;
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

TypeCSP signatureType(const Signature& sig) {
    types::Function fun;
    fun.m_arg_types.resize(sig.m_args.size());
    std::transform(sig.m_args.begin(), sig.m_args.end(), fun.m_arg_types.begin(),
                   [](auto& arg) { return arg.first; });
    fun.m_ret_type = sig.m_return_type;
    return std::make_shared<types::Function>(fun);
}

TypeCSP Function::type() const { return signatureType(m_sig); }

tl::expected<Signature, Err> analyze(const ast::Signature& sig, const Context&) {
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

tl::expected<Function, Err> analyze(const ast::Function& function, const Context& outer_context) {
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
    ost << "===========---------==========" << std::endl;
    return ost;
}

}  // namespace semantic

}  // namespace pom
