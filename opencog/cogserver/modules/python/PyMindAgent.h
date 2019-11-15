/*
 * opencog/cython/PyMindAgent.h
 *
 * Copyright (C) 2011 by The OpenCog Foundation
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef _OPENCOG_PYAGENT_H
#define _OPENCOG_PYAGENT_H

#include <string>

#include <opencog/cython/PyIncludeWrapper.h>

#include <opencog/atomspace/AtomSpace.h>
#include <opencog/cogserver/modules/agents/Agent.h>
#include <opencog/cogserver/server/Factory.h>

class PythonModuleUTest;

namespace opencog
{

/** The PyMindAgent Class
 * This class wraps Python MindAgents and allows the CogServer to interact
 * and manage them.
 */
class PyMindAgent : public Agent
{
    friend class ::PythonModuleUTest;

    PyObject* pyagent;
    std::string moduleName;
    std::string className;

public:

    virtual const ClassInfo& classinfo() const;

    //PyMindAgent();

    /** Pass a PyObject that is a Python MindAgent object.  */
    PyMindAgent(CogServer&, const std::string& moduleName, const std::string& className);

    virtual ~PyMindAgent();

    /** Run method - this calls the run method of the encapsulated Python
     * MindAgent class.
     */
    virtual void run();

}; // class

typedef std::shared_ptr<PyMindAgent> PyMindAgentPtr;

}  // namespace

#endif // _OPENCOG_AGENT_H

