#include "common_types.h"
#include <cctype>

namespace hlasm_plugin::parser_library::context
{

std::string & to_upper(std::string & s)
{
	for (auto & c : s) c = static_cast<char>(std::toupper(c));
	return s;
}

SET_t::SET_t(context::A_t value)
	:a_value(value), b_value(object_traits<B_t>::default_v()), c_value(object_traits<C_t>::default_v()), type(SET_t_enum::A_TYPE) {}

SET_t::SET_t(context::B_t value)
	: a_value(object_traits<A_t>::default_v()), b_value(value), c_value(object_traits<C_t>::default_v()), type(SET_t_enum::B_TYPE) {}

SET_t::SET_t(context::C_t value)
	: a_value(object_traits<A_t>::default_v()), b_value(object_traits<B_t>::default_v()), c_value(value), type(SET_t_enum::C_TYPE) {}

SET_t::SET_t()
	: a_value(object_traits<A_t>::default_v()), b_value(object_traits<B_t>::default_v()), c_value(object_traits<C_t>::default_v()), type(SET_t_enum::UNDEF_TYPE) {}

A_t& SET_t::access_a() { return a_value; }

B_t& SET_t::access_b() { return b_value; }

C_t& SET_t::access_c() { return c_value; }

A_t SET_t::C2A(const context::C_t & value) const
{
	//TODO selfdefterm
	if (value.empty())
		return context::object_traits<A_t>::default_v();
	try
	{
		return std::stoi(value);
	}
	catch (...) {}
	return 0;
}

}
