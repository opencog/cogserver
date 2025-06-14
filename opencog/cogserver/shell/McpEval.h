/*
 * McpEval.h
 *
 * Stub for an MCP evaluator
 * Copyright (c) 2008, 2013, 2014, 2020, 2021, 2022 Linas Vepstas <linas@linas.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _OPENCOG_MCP_EVAL_H
#define _OPENCOG_MCP_EVAL_H

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <opencog/eval/GenericEval.h>
#include <opencog/atomspace/AtomSpace.h>
#include <opencog/persist/json/McpPlugin.h>

/**
 * The McpEval class implements a the generic evaluator API for
 * for processing the MCP protocol.
 */

namespace opencog {
/** \addtogroup grp_server
 *  @{
 */

class McpEval : public GenericEval
{
	private:
		McpEval(const AtomSpacePtr&);
		bool _started;
		bool _done;
		std::string _result;
		AtomSpacePtr _atomspace;

		// Plugin management
		std::vector<std::shared_ptr<McpPlugin>> _plugins;
		std::unordered_map<std::string, std::shared_ptr<McpPlugin>> _tool_to_plugin;

	public:
		virtual ~McpEval();

		virtual void begin_eval(void);
		virtual void eval_expr(const std::string&);
		virtual std::string poll_result(void);

		virtual void interrupt(void);

		// Plugin registration
		void register_plugin(std::shared_ptr<McpPlugin> plugin);
		void unregister_plugin(std::shared_ptr<McpPlugin> plugin);

		// Return per-thread, per-atomspace singleton
		static McpEval* get_evaluator(const AtomSpacePtr&);
};

/** @}*/
}

#endif // _OPENCOG_MCP_EVAL_H
