/*
 * PythonShell.h
 *
 * Simple python shell
 *
 * @author Ramin Barati <rekino@gmail.com>
 * @date   2013-07-02
 *
 * @Note
 *   This code is almost identical to Linas' SchemeShell, so most of the credits
 *   goes to him.
 *
 * Reference:
 *   http://www.linuxjournal.com/article/3641?page=0,2
 *   http://www.codeproject.com/KB/cpp/embedpython_1.aspx
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
#ifdef HAVE_CYTHON

#include <opencog/cython/PythonEval.h>
#include "PythonShell.h"

using namespace opencog;

PythonShell::PythonShell(const Handle& hcsn) :
    _shellspace(AtomSpaceCast(hcsn->getAtomSpace()))
{
    normal_prompt = "py> ";
    pending_prompt = "... ";
    abort_prompt += normal_prompt;
    _name = "pyth";
}

PythonShell::~PythonShell()
{
    // Stall until the evaluator is done.
    while_not_done();
}

GenericEval* PythonShell::get_evaluator(void)
{
    return PythonEval::get_python_evaluator(_shellspace);
}

void PythonShell::eval(const std::string &expr)
{
    bool selfie = self_destruct;
    self_destruct = false;

    // Accumulate multi-line Python constructs (def, class, if, etc.)
    // These end with ':' and need a blank line to terminate.
    if (not _pending_lines.empty())
    {
        _pending_lines += expr + "\n";

        // Blank line terminates multi-line construct in Python
        if (expr.empty())
        {
            GenericShell::eval(_pending_lines);
            _pending_lines.clear();
        }
    }
    else
    {
        // Check if this line starts a multi-line construct
        // (ends with ':' after stripping whitespace and comments)
        std::string trimmed = expr;
        size_t comment = trimmed.find('#');
        if (comment != std::string::npos)
            trimmed = trimmed.substr(0, comment);
        while (not trimmed.empty() and
               (trimmed.back() == ' ' or trimmed.back() == '\t'))
            trimmed.pop_back();

        if (not trimmed.empty() and trimmed.back() == ':')
            _pending_lines = expr + "\n";
        else
            GenericShell::eval(expr);
    }

    if (selfie) {
        // Eval an empty string as a end-of-file marker. This is needed
        // to flush pending input in the python shell, as otherwise,
        // there is no way to know that no more python input will
        // arrive!
        if (not _pending_lines.empty())
        {
            GenericShell::eval(_pending_lines);
            _pending_lines.clear();
        }
        GenericShell::eval("");
        self_destruct = true;
    }
}

#endif
