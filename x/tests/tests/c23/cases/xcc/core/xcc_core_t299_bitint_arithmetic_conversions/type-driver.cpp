#include "frontend/types.h"
#include <iostream>
#include <map>
#include <string>
using namespace xcc;
type_ptr read_type(const std::string &name) {
    if (name[0] == 's' && name.size() > 1 && name[1] >= '0' && name[1] <= '9')
        return type::make_bitint(std::stoi(name.substr(1)), false);
    if (name[0] == 'u' && name.size() > 1 && name[1] >= '0' && name[1] <= '9')
        return type::make_bitint(std::stoi(name.substr(1)), true);
    const std::map<std::string, type_kind> kinds = {
        {"bool",type_kind::BOOL},{"char",type_kind::CHAR},{"schar",type_kind::SCHAR},
        {"uchar",type_kind::UCHAR},{"short",type_kind::SHORT},{"ushort",type_kind::USHORT},
        {"int",type_kind::INT},{"uint",type_kind::UINT},{"long",type_kind::LONG},
        {"ulong",type_kind::ULONG},{"llong",type_kind::LLONG},{"ullong",type_kind::ULLONG},
        {"enum",type_kind::ENUM},{"char8",type_kind::CHAR8T}};
    return std::make_shared<type>(kinds.at(name));
}
std::string describe(type_ptr value) {
    if (value->kind == type_kind::BITINT)
        return std::string(value->is_unsigned() ? "u" : "s") + std::to_string(value->bitint_width);
    switch(value->kind) {
    case type_kind::INT:return "int"; case type_kind::UINT:return "uint";
    case type_kind::LONG:return "long"; case type_kind::ULONG:return "ulong";
    case type_kind::LLONG:return "llong"; case type_kind::ULLONG:return "ullong";
    default:return value->to_string();
    }
}
int main(int argc,char **argv) {
    set_plain_char_unsigned(argc < 2 || std::string(argv[1]) == "unsigned");
    std::string left,right;
    while(std::cin>>left>>right) {
        auto a=read_type(left),b=read_type(right);
        std::cout<<describe(integer_promote(a))<<' '<<describe(integer_promote(b))<<' '
                 <<describe(usual_arith_conv(a,b))<<'\n';
    }
}
