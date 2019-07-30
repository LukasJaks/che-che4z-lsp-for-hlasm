#include "low_language_processor.h"
#include "../context_manager.h"
#include "../statement_processors/ordinary_processor.h"

#include <optional>

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::processing;

low_language_processor::low_language_processor(context::hlasm_context& hlasm_ctx, branching_provider& provider, statement_field_reparser& parser)
	:instruction_processor(hlasm_ctx), provider(provider),parser(parser)  {}

rebuilt_statement low_language_processor::preprocess(context::unique_stmt_ptr statement)
{
	auto& stmt = dynamic_cast<resolved_statement_impl&>(*statement->access_resolved());
	auto [label,ops] = preprocess_inner(stmt);
	return rebuilt_statement(std::move(stmt), std::move(label), std::move(ops));
}

rebuilt_statement low_language_processor::preprocess(context::shared_stmt_ptr statement)
{
	const auto& stmt = dynamic_cast<const resolved_statement_impl&>(*statement->access_resolved());
	auto [label, ops] = preprocess_inner(stmt);
	return rebuilt_statement(stmt, std::move(label), std::move(ops));
}

low_language_processor::preprocessed_part low_language_processor::preprocess_inner(const resolved_statement_impl& stmt)
{
	context_manager mngr(hlasm_ctx);

	std::optional<semantics::label_si> label;
	std::optional<semantics::operands_si> operands;

	std::string new_label;
	//label
	switch (stmt.label_ref().type)
	{
	case semantics::label_si_type::CONC:
		label.emplace(
			stmt.label_ref().field_range,
			mngr.concatenate_str(std::get<semantics::concat_chain>(stmt.label_ref().value)));
		break;
	case semantics::label_si_type::VAR:
		new_label = mngr.get_var_sym_value(*std::get<semantics::vs_ptr>(stmt.label_ref().value)).template to<context::C_t>();
		if (new_label.empty())
			label.emplace(stmt.label_ref().field_range);
		else
			label.emplace(stmt.label_ref().field_range, std::move(new_label));
		break;
	case semantics::label_si_type::MAC:
		add_diagnostic(diagnostic_s::error_E057("", "", stmt.label_ref().field_range));
		break;
	case semantics::label_si_type::SEQ:
		provider.register_sequence_symbol(std::get<semantics::seq_sym>(stmt.label_ref().value).name, std::get<semantics::seq_sym>(stmt.label_ref().value).symbol_range);
		break;
	default:
		break;
	}

	//operands
	if (!stmt.operands_ref().value.empty() && stmt.operands_ref().value[0]->type == semantics::operand_type::MODEL)
	{
		assert(stmt.operands_ref().value.size() == 1);
		std::string field(mngr.concatenate_str(stmt.operands_ref().value[0]->access_model()->chain));
		operands.emplace(parser.reparse_operand_field(
			&hlasm_ctx,
			std::move(field),
			semantics::range_provider(stmt.operands_ref().value[0]->operand_range,true),
			*ordinary_processor::get_instruction_processing_status(stmt.opcode.value, hlasm_ctx)).first);
	}

	for (auto& op : (operands ? operands->value : stmt.operands_ref().value))
	{
		if (dynamic_cast<semantics::simple_expr_operand*>(op.get()))
			dynamic_cast<semantics::simple_expr_operand*>(op.get())->expression->fill_location_counter(hlasm_ctx.ord_ctx.align(context::no_align));
	}

	return std::make_pair(std::move(label), std::move(operands));
}

void add_diag_(diagnostic_s diag, diagnosable& diagnoser, const context::hlasm_statement& stmt)
{
	auto diagnoser_ctx = dynamic_cast<diagnosable_ctx*>(&diagnoser);
	auto postponed_stmt = dynamic_cast<const context::postponed_statement*>(&stmt);

	if (diagnoser_ctx && postponed_stmt)
		diagnoser_ctx->add_diagnostic(std::move(diag), postponed_stmt->location_stack());
	else
		diagnoser.add_diagnostic(std::move(diag));
}

low_language_processor::transform_result low_language_processor::transform_mnemonic(const resolved_statement& stmt, context::hlasm_context& hlasm_ctx, diagnosable& diagnoser)
{
	// operands obtained from the user
	const auto& operands = stmt.operands_ref().value;
	// the name of the instruction (mnemonic) obtained from the user
	auto instr_name = *stmt.opcode_ref().value;
	// the associated mnemonic structure with the given name
	auto mnemonic = context::instruction::mnemonic_codes.at(instr_name);
	// the machine instruction structure associated with the given instruction name
	auto curr_instr = &context::instruction::machine_instructions.at(mnemonic.instruction);

	// check whether substituted mnemonic values are ok

	// check size of mnemonic operands
	int diff = curr_instr->get()->operands.size() - operands.size() - mnemonic.replaced.size();
	if (std::abs(diff) > curr_instr->get()->no_optional)
	{
		auto curr_diag = diagnostic_op::error_optional_number_of_operands(curr_instr->get()->instr_name, curr_instr->get()->no_optional, curr_instr->get()->operands.size() - mnemonic.replaced.size());
		auto range = stmt.stmt_range_ref();
		diagnoser.add_diagnostic(diagnostic_s{ "",range,
		curr_diag.severity, std::move(curr_diag.code),
		"HLASM Plugin", std::move(curr_diag.message), {} });
		return std::nullopt;
	}

	std::vector<checking::check_op_ptr> substituted_mnems;
	for (auto mnem : mnemonic.replaced)
		substituted_mnems.push_back(std::make_unique<checking::one_operand>(mnem.second));

	std::vector<checking::check_op_ptr> operand_vector;
	// create vector of empty operands
	for (size_t i = 0; i < curr_instr->get()->operands.size() + curr_instr->get()->no_optional; i++)
		operand_vector.push_back(nullptr);
	// add substituted
	for (size_t i = 0; i < mnemonic.replaced.size(); i++)
		operand_vector[mnemonic.replaced[i].first] = std::move(substituted_mnems[i]);
	// add other
	for (size_t i = 0; i < operands.size(); i++)
	{
		auto& operand = operands[i];
		for (size_t j = 0; j < operand_vector.size(); j++)
		{
			if (operand_vector[j] == nullptr)
			{
				// if operand is empty
				if (operand->type == semantics::operand_type::EMPTY || operand->type == semantics::operand_type::UNDEF)
				{
					operand_vector[j] = std::make_unique<checking::empty_operand>();
					operand_vector.at(operand_vector.size() - 1)->operand_range = operand->operand_range;
					continue;
				}

				auto uniq = get_check_op(operand.get(), hlasm_ctx, diagnoser, stmt);

				if (!uniq) return std::nullopt; //contains dependencies

				operand_vector[j] = std::move(uniq);
			}
		}
	}
	return operand_vector;
}

low_language_processor::transform_result low_language_processor::transform_default(const resolved_statement& stmt, context::hlasm_context& hlasm_ctx, diagnosable& diagnoser,bool mach)
{
	std::vector<checking::check_op_ptr> operand_vector;
	for (auto& op : stmt.operands_ref().value)
	{
		// check whether operand isn't empty
		if (op->type == semantics::operand_type::EMPTY || op->type == semantics::operand_type::UNDEF)
		{
			operand_vector.push_back(std::make_unique<checking::empty_operand>());

			continue;
		}

		auto uniq = get_check_op(op.get(), hlasm_ctx, diagnoser, stmt);

		if (!uniq) return std::nullopt;//contains dependencies

		operand_vector.push_back(std::move(uniq));
	}
	return operand_vector;
}

checking::check_op_ptr low_language_processor::get_check_op(const semantics::operand* op, context::hlasm_context& hlasm_ctx, diagnosable& diagnoser, const resolved_statement& stmt)
{
	auto ev_op = dynamic_cast<const semantics::evaluable_operand*>(op);
	assert(ev_op);

	auto tmp = context::instruction::assembler_instructions.find(*stmt.opcode_ref().value);
	bool can_have_ord_syms = tmp != context::instruction::assembler_instructions.end() ? tmp->second.has_ord_symbols : true;

	if (can_have_ord_syms && ev_op->has_dependencies(hlasm_ctx.ord_ctx))
	{
		add_diag_(diagnostic_s::error_E010("", "ordinary symbol", ev_op->operand_range), diagnoser, stmt);
		return nullptr;
	}

	auto uniq = ev_op->get_operand_value(hlasm_ctx.ord_ctx);

	ev_op->collect_diags();
	for (auto& diag : ev_op->diags())
		add_diag_(std::move(diag), diagnoser, stmt);
	ev_op->diags().clear();

	return uniq;
}

void low_language_processor::check(const resolved_statement& stmt,context::hlasm_context& hlasm_ctx, checking::instruction_checker& checker, diagnosable& diagnoser)
{
	auto empty_asm_op = checking::one_operand();
	auto empty_mach_op = checking::empty_operand();

	std::vector<const checking::operand*> operand_ptr_vector;
	transform_result operand_vector;

	auto mnem_tmp = context::instruction::mnemonic_codes.find(*stmt.opcode_ref().value);
	const std::string* instruction_name;

	if (mnem_tmp != context::instruction::mnemonic_codes.end())
	{
		operand_vector = transform_mnemonic(stmt, hlasm_ctx, diagnoser);
		instruction_name = &mnem_tmp->second.instruction;
	}
	else
	{
		operand_vector = transform_default(stmt, hlasm_ctx, diagnoser, dynamic_cast<checking::machine_checker*>(&checker));
		instruction_name = stmt.opcode_ref().value;
	}

	if (!operand_vector)
		return;

	for (const auto& op : *operand_vector)
		operand_ptr_vector.push_back(op.get());

	checker.check(*instruction_name, operand_ptr_vector);

	auto diags = checker.get_diagnostics();
	auto range = stmt.stmt_range_ref();
	for (auto diag : diags)
	{
		if (diagnostic_op::is_error(*diag))
		{
			add_diag_(
				diagnostic_s{ "",range,
					diag->severity, std::move(diag->code),
					"HLASM Plugin", std::move(diag->message), {} },
				diagnoser, stmt);
		}
	}

	checker.clear_diagnostics();
}
