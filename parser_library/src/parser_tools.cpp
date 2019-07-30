#include "parser_tools.h"
#include "../include/shared/lexer.h"

namespace hlasm_plugin
{
	namespace parser_tools
	{
		std::vector<antlr4::tree::ParseTree*> get_sub_trees(antlr4::tree::ParseTree * tree, size_t type)
		{
			std::vector<antlr4::tree::ParseTree *> return_trees;
			if (!tree->children.empty())
			{
				if (((antlr4::ParserRuleContext *)tree)->getRuleIndex() == type)
					return_trees.push_back(tree);
				for (auto child : tree->children)
				{
					auto child_trees = get_sub_trees(child, type);
					if (!child_trees.empty())
						return_trees.insert(return_trees.end(), child_trees.begin(), child_trees.end());
				}
			}
			else if (((antlr4::tree::TerminalNode *)tree)->getSymbol()->getType() == type)
			{
				return_trees.push_back(tree);
			}
			return return_trees;
		}

		bool is_tree_type(antlr4::tree::ParseTree * tree, size_t type)
		{
			if (tree->children.size() == 0)
				return ((antlr4::tree::TerminalNode *)tree)->getSymbol()->getType() == type;
			else
				return ((antlr4::ParserRuleContext *)tree)->getRuleIndex() == type;
		}

		useful_tree::useful_tree(std::vector<antlr4::ParserRuleContext *> tree, const antlr4::dfa::Vocabulary& vocab, const std::vector<std::string>& rules)
			:  tree_(tree), vocab_(vocab), rules_(rules)
		{};

		void useful_tree::out_tree(std::ostream &stream)
		{
			for(auto node : tree_)
			out_tree_rec(node, "", stream);
		}

		void useful_tree::out_tree_rec(antlr4::ParserRuleContext * tree, std::string indent, std::ostream & stream)
		{
			if (tree->children.empty())
			{
				if (tree->getText() == "")
					stream << indent << rules_[tree->getRuleIndex()] << ": " << "\"" << tree->getText() << "\"" << std::endl;
				else
				{
					auto type = ((antlr4::tree::TerminalNode *)tree)->getSymbol()->getType();
					stream << indent << vocab_.getSymbolicName(type);
					if (type != parser_library::lexer::EOLLN && type != parser_library::lexer::SPACE) stream << ": " << "\"" << tree->getText() << "\"";
					stream << std::endl;
				}
			}
			else
			{
				stream << indent << rules_[tree->getRuleIndex()] << ": " << "\"" << tree->getText() << "\"" << std::endl;
				indent.insert(0, "\t");
				for (auto child : tree->children)
				{
					out_tree_rec((antlr4::ParserRuleContext *)child, indent, stream);
				}
			}
		}
	}
}
