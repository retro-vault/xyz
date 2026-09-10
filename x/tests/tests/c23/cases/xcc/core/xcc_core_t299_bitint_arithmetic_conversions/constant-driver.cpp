#include "frontend/const_eval.h"
#include <cstdint>
#include <iostream>
#include <map>
#include <string>

using namespace xcc;

static type_ptr read_type(const std::string &name) {
    if (name.size() > 1 && name[1] >= '0' && name[1] <= '9')
        return type::make_bitint(std::stoi(name.substr(1)), name[0] == 'u');
    static const std::map<std::string, type_kind> kinds = {
        {"bool", type_kind::BOOL}, {"schar", type_kind::SCHAR},
        {"uchar", type_kind::UCHAR}, {"short", type_kind::SHORT},
        {"ushort", type_kind::USHORT}, {"int", type_kind::INT},
        {"uint", type_kind::UINT}, {"long", type_kind::LONG},
        {"ulong", type_kind::ULONG}, {"llong", type_kind::LLONG},
        {"ullong", type_kind::ULLONG}};
    return std::make_shared<type>(kinds.at(name));
}

static expr_ptr value(type_ptr type, uint64_t raw) {
    auto literal = std::make_unique<int_literal_expr>();
    literal->type = type::make_ullong();
    literal->value = static_cast<int64_t>(raw);
    auto cast = std::make_unique<cast_expr>();
    cast->type = cast->target_type = type;
    cast->operand = std::move(literal);
    return cast;
}

int main() {
    std::ios::sync_with_stdio(false);
    static const std::map<std::string, bin_op> binary_ops = {
        {"add", bin_op::ADD}, {"sub", bin_op::SUB}, {"mul", bin_op::MUL},
        {"div", bin_op::DIV}, {"mod", bin_op::MOD}, {"and", bin_op::AND},
        {"or", bin_op::OR}, {"xor", bin_op::XOR}, {"shl", bin_op::SHL},
        {"shr", bin_op::SHR}, {"eq", bin_op::EQ}, {"ne", bin_op::NE},
        {"lt", bin_op::LT}, {"le", bin_op::LE}, {"gt", bin_op::GT},
        {"ge", bin_op::GE}, {"land", bin_op::LAND}, {"lor", bin_op::LOR}};
    std::string operation, left_name, right_name, result_name;
    uint64_t left_raw, right_raw;
    while (std::cin >> operation >> left_name >> left_raw >> right_name >> right_raw >> result_name) {
        auto left_type = read_type(left_name), right_type = read_type(right_name);
        auto result_type = read_type(result_name);
        expr_ptr expression;
        if (operation == "neg" || operation == "bnot" || operation == "not") {
            auto unary = std::make_unique<unary_expr>();
            unary->op = operation == "neg" ? unary_op::NEG :
                operation == "bnot" ? unary_op::BNOT : unary_op::NOT;
            unary->type = result_type;
            unary->operand = value(left_type, left_raw);
            expression = std::move(unary);
        } else if (operation == "cond0" || operation == "cond1") {
            auto conditional = std::make_unique<conditional_expr>();
            conditional->type = result_type;
            conditional->cond = value(type::make_int(), operation == "cond1");
            conditional->then_expr = value(left_type, left_raw);
            conditional->else_expr = value(right_type, right_raw);
            expression = std::move(conditional);
        } else {
            auto binary = std::make_unique<binary_expr>();
            binary->op = binary_ops.at(operation);
            binary->type = result_type;
            binary->left = value(left_type, left_raw);
            binary->right = value(right_type, right_raw);
            expression = std::move(binary);
        }
        // A widening parent must observe an already-normalized child value.
        auto outer = std::make_unique<cast_expr>();
        outer->type = outer->target_type = type::make_ullong();
        outer->operand = std::move(expression);
        auto result = const_expr_evaluator::evaluate(outer.get());
        if (result)
            std::cout << static_cast<uint64_t>(*result) << '\n';
        else
            std::cout << "none\n";
    }
}
