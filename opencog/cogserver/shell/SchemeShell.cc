/*
 * SchemeShell.cc
 *
 * Simple scheme shell
 * Copyright (c) 2008 Linas Vepstas <linas@linas.org>
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

#ifdef HAVE_GUILE

#include <opencog/util/Logger.h>
#include <opencog/util/oc_assert.h>
#include <opencog/atoms/atom_types/atom_names.h>
#include <opencog/cogserver/types/atom_types.h>
#include <opencog/atoms/value/BoolValue.h>
#include <opencog/atoms/value/StringValue.h>
#include <opencog/guile/SchemeEval.h>

#include "SchemeShell.h"

using namespace opencog;

std::string SchemeShell::_prompt;

SchemeShell::SchemeShell(const AtomSpacePtr& asp) :
	_shellspace(asp)
{
	_prompt = "[0;34mguile[1;34m> [0m";

	// Get the CogServerNode to retrieve prompt settings.
	Handle h = asp->get_node(COG_SERVER_NODE, "cogserver");
	if (nullptr == h)
		h = asp->get_node(COG_SERVER_NODE, "test-cogserver");
	OC_ASSERT(h != nullptr, "Internal error: CogServerNode not found!");

	// Check if ANSI colors are enabled
	bool ansi_enabled = true;
	ValuePtr vp = h->getValue(asp->add_atom(Predicate("*-ansi-enabled-*")));
	if (vp and vp->is_type(BOOL_VALUE))
		ansi_enabled = BoolValueCast(vp)->value()[0];

	// Get the appropriate prompt
	if (ansi_enabled)
		vp = h->getValue(asp->add_atom(Predicate("*-ansi-scm-prompt-*")));
	else
		vp = h->getValue(asp->add_atom(Predicate("*-scm-prompt-*")));

	if (vp and vp->is_type(STRING_VALUE))
		_prompt = StringValueCast(vp)->value()[0];

	normal_prompt = _prompt;
	abort_prompt = _prompt;
	pending_prompt = "... ";
	_name = " scm";
}

SchemeShell::~SchemeShell()
{
	// Stall until after all evaluators have finished evaluating.
	while_not_done();
}

GenericEval* SchemeShell::get_evaluator(void)
{
	return SchemeEval::get_scheme_evaluator(_shellspace);
}

#endif
/* ===================== END OF FILE ============================ */
