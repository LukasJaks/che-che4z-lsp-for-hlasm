#ifndef PROCESSING_MACH_PROCESSOR_H
#define PROCESSING_MACH_PROCESSOR_H

#include "low_language_processor.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

//processor of machine instructions
class mach_processor : public low_language_processor
{
	checking::machine_checker checker;
public:
	mach_processor(context::hlasm_context& hlasm_ctx, branching_provider& provider, statement_field_reparser& parser);

	virtual void process(context::unique_stmt_ptr stmt) override;
	virtual void process(context::shared_stmt_ptr stmt) override;

private:
	void process(rebuilt_statement statement, const op_code& opcode);
};

}
}
}
#endif