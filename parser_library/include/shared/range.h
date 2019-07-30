#ifndef HLASMPLUGIN_PARSERLIBRARY_RANGE_H
#define HLASMPLUGIN_PARSERLIBRARY_RANGE_H

#include "parser_library_export.h"
#include <string>

namespace hlasm_plugin {
namespace parser_library {

using position_t = uint64_t;

struct PARSER_LIBRARY_EXPORT position
{
	position() : line(0), column(0) {}
	position(position_t line, position_t column) : line(line), column(column) {}
	bool operator==(const position& oth) const
	{
		return line == oth.line && column == oth.column;
	}
	position_t line;
	position_t column;
};

struct PARSER_LIBRARY_EXPORT range
{
	range() {}
	range(position start, position end) : start(start), end(end) {}
	explicit range(position start) : start(start), end(start) {}
	bool operator==(const range& r) const
	{
		return start == r.start && end == r.end;
	}
	position start;
	position end;
};

struct PARSER_LIBRARY_EXPORT location
{
	location() {}
	location(position pos, std::string file) :pos(pos), file(file) {}
	bool operator==(const location& oth) const
	{
		return pos == oth.pos && file == oth.file;
	}
	position pos;
	std::string file;
};

}
}
#endif
