#ifndef CONTEXT_MACRO_PARAM_DATA_H
#define CONTEXT_MACRO_PARAM_DATA_H

#include <memory>
#include <vector>
#include "common_types.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {

class macro_param_data_component;
using macro_data_ptr = std::unique_ptr<macro_param_data_component>;
using macro_data_shared_ptr = std::shared_ptr<macro_param_data_component>;

//base class for data of macro parameters
//data in macro parameters are immutable
class macro_param_data_component
{
public:
	//gets value of current data, composite or simple
	virtual const C_t& get_value() const = 0;
	//gets value of the idx-th value, when exceeds size of data, returns default value
	virtual const macro_param_data_component* get_ith(size_t idx) const = 0;

	//dummy data returning default value everytime
	static const macro_data_shared_ptr dummy;

	virtual ~macro_param_data_component();
};

//dummy macro data class returning default value everytime
class macro_param_data_dummy :public macro_param_data_component
{
public:
	//gets default value ("")
	const C_t& get_value() const override;

	//gets this dummy
	const macro_param_data_component* get_ith(size_t idx) const override;
};

//class representing data of macro parameters holding only single string (=C_t)
class macro_param_data_single : public macro_param_data_component
{
	const C_t data_;
public:
	//returns whole data, here the only string
	virtual const C_t& get_value() const override;

	//gets value of the idx-th value, when exceeds size of data, returns default value
	//get_ith(0) returns this to mimic HLASM
	virtual const macro_param_data_component* get_ith(size_t idx) const override;

	macro_param_data_single(const C_t& value);
	macro_param_data_single(C_t&& value);
};

//class representing data of macro parameters holding more nested data
class macro_param_data_composite : public macro_param_data_component
{
	const std::vector<macro_data_ptr> data_;
	mutable C_t value_;
public:
	//returns data of all nested classes in brackets separated by comma
	virtual const C_t& get_value() const override;

	//gets value of the idx-th value, when exceeds size of data, returns default value
	virtual const macro_param_data_component* get_ith(size_t idx) const override;

	macro_param_data_composite(std::vector<macro_data_ptr> value);
};

}
}
}
#endif
