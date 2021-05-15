
#include <pom_semantic.h>

#include <fmt/format.h>
#include <map>

namespace pom {

namespace semantic {

namespace {

inline Type realType() { return Type{"real"}; }

inline Type integerType() { return Type{"int"}; }

inline Type boolType() { return Type{"bool"}; }

}  // namespace

tl::expected<Type, Err> calculateType(const ast::Expr& expr, const Context& context);

tl::expected<Type, Err> calculateType(const literals::Boolean&, const Context&) {
    return boolType();
}

tl::expected<Type, Err> calculateType(const literals::Integer&, const Context&) {
    return integerType();
}

tl::expected<Type, Err> calculateType(const literals::Real&, const Context&) { return realType(); }

tl::expected<Type, Err> calculateType(const ast::Literal& lit, const Context& context) {
    return std::visit([&](auto& x) { return calculateType(x, context); }, lit);
}

tl::expected<Type, Err> calculateType(const ast::Var& var, const Context& context) {
    auto found = context.m_variables.find(var.m_name);
    if (found == context.m_variables.end()) {
        return tl::make_unexpected(Err{fmt::format("Variable {0} not found in this context", var.m_name)});
    }
    return found->second;
}

tl::expected<Type, Err> calculateType(const ast::BinaryExpr& expr, const Context& context) {
    auto lhs_type = calculateType(*expr.m_lhs, context);
    if(!lhs_type) {
        return lhs_type;
    }
    auto rhs_type = calculateType(*expr.m_rhs, context);
    if(!rhs_type) {
        return rhs_type;
    }

    if(*rhs_type != *lhs_type) {
        return tl::make_unexpected(Err{fmt::format("Type error calling '{0}'", expr.m_op)});
    }

    return rhs_type;
}

tl::expected<Type, Err> calculateType(const ast::Call& call, const Context& context) {
    auto found = context.m_variables.find(call.m_function);
    if (found == context.m_variables.end()) {
        return tl::make_unexpected(Err{fmt::format("Function {0} not found in this context", call.m_function)});
    }
    if(!found->second.returnType()) {
        return tl::make_unexpected(Err{fmt::format("{0} is not callable", call.m_function)});
    }
    return *(found->second.returnType());
}

tl::expected<Type, Err> calculateType(const ast::Expr& expr, const Context& context) {
    return std::visit([&](auto& x) { return calculateType(x, context); }, expr.m_val);
}

tl::expected<Function, Err> analyze(const ast::Function& function, const Context& outer_context) {
    auto& sig = function.m_sig;

    auto context = outer_context;
    for (auto& arg : sig.m_args) {
        context.m_variables.insert({arg.second, arg.first});
    }

    //TODO: fix recursion
    context.m_variables.insert({function.m_sig.m_name, Type("function", Type("real"))});

    auto ret_type = calculateType(*function.m_code, context);
    if(!ret_type) {
        return tl::make_unexpected(ret_type.error());
    }

    return Function{function, *ret_type, context};
}


tl::expected<TopLevel, Err> analyze(const parser::TopLevel& top_level) {

    Context context;
    TopLevel semantic_top_level;

    for (auto& unit : top_level) {
        if(std::holds_alternative<ast::Signature>(unit)) {
            auto& extrn = std::get<ast::Signature>(unit);
            // TODO: improve this:
            context.m_variables.insert({extrn.m_name, Type("function", extrn.m_args[0].first)});
            semantic_top_level.push_back(extrn);
        }
        else if(std::holds_alternative<ast::Function>(unit)) {
            auto& fn = std::get<ast::Function>(unit);

            auto sem_fn = analyze(fn, context);
            if(!sem_fn) {
                return tl::make_unexpected(sem_fn.error());
            }

            semantic_top_level.push_back(*sem_fn);

            context.m_variables.insert({fn.m_sig.m_name, Type("function", sem_fn->m_return_type)});
        }
    }

    return semantic_top_level;
}

void printTlu(std::ostream& ost, const TopLevelUnit& u) {
    std::visit(
        [&](auto&& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, ast::Signature>) {
                ost << "extern: ";
                ast::print(ost, v);
                ost << "\n";
            } else if constexpr (std::is_same_v<T, Function>) {
                ost << "func: ";
                ast::print(ost, v.m_function.m_sig);
                ost << ": ";
                ast::print(ost, *v.m_function.m_code);
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

void print(std::ostream& ost, const Context& context)
{
    ost << "=========== Context ==========" << std::endl;
    ost << "Variables::" << std::endl;
    for(auto& v : context.m_variables) {
        ost << v.first << ": " << v.second.name() << std::endl;
    }
    ost << "===========---------==========" << std::endl;
}

}  // namespace semantic

}  // namespace pom
