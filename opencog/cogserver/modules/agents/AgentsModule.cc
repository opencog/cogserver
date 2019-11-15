/*
 * opencog/cogserver/modules/agents/AgentsModule.cc
 *
 * Copyright (C) 2008 by OpenCog Foundation
 * Written by Gustavo Gama <gama@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <iomanip>
#include <unistd.h>

#include <opencog/util/ansi.h>
#include <opencog/util/oc_assert.h>
#include <opencog/cogserver/server/CogServer.h>
#include <opencog/cogserver/server/ConsoleSocket.h>

#include "AgentsModule.h"

using namespace opencog;

DECLARE_MODULE(AgentsModule)

AgentsModule::AgentsModule(CogServer& cs) :
    Module(cs),
    _scheduler(Scheduler())
{
    registerAgentRequests();
}

AgentsModule::~AgentsModule()
{
    unregisterAgentRequests();
}

void AgentsModule::registerAgentRequests()
{
    do_startAgents_register();
    do_stopAgents_register();
    do_stepAgents_register();
    do_startAgentLoop_register();
    do_stopAgentLoop_register();
    do_listAgents_register();
    do_activeAgents_register();
}

void AgentsModule::unregisterAgentRequests()
{
    do_startAgents_unregister();
    do_stopAgents_unregister();
    do_stepAgents_unregister();
    do_startAgentLoop_unregister();
    do_stopAgentLoop_unregister();
    do_listAgents_unregister();
    do_activeAgents_unregister();
}

void AgentsModule::init()
{
}

// ====================================================================
// Various agents commands
std::string AgentsModule::do_startAgents(Request *dummy, std::list<std::string> args)
{
    std::list<const char*> availableAgents = _scheduler.agentIds();

    std::vector<std::tuple<std::string, bool, std::string>> agents;

    if (args.empty())
        return "Error: No agents to start specified\n";

    for (std::list<std::string>::const_iterator it = args.begin();
         it != args.end(); ++it) {
        auto p1 = it->find(',');
        std::string agent_type = it->substr(0, p1);
        // check that this is a valid type; give an error and return otherwise
        if (availableAgents.end() ==
         find(availableAgents.begin(), availableAgents.end(), agent_type)) {
            std::ostringstream oss;
            oss << "Invalid Agent ID \"" << agent_type << "\"\n";
            return oss.str();
        }

        bool threaded = false;
        std::string thread_name;
        if (p1 != std::string::npos) {
            auto p2 = it->find(',', p1 + 1);
            auto dedicated_str = it->substr(p1 + 1, p2 - p1 - 1);
            threaded = (dedicated_str == "yes");
            if (!threaded && dedicated_str != "no")
                return "Invalid dedicated parameter: " + dedicated_str + '\n';

            if (p2 != std::string::npos)
                thread_name = it->substr(p2 + 1);
        }

        agents.push_back(std::make_tuple(agent_type, threaded, thread_name));
     }

    for (auto it = agents.cbegin();
         it != agents.cend(); ++it) {
        auto agent = _scheduler.createAgent(std::get<0>(*it));
        _scheduler.startAgent(agent, std::get<1>(*it), std::get<2>(*it));
    }

    return "Successfully started agents\n";
}

std::string AgentsModule::do_stopAgents(Request *dummy, std::list<std::string> args)
{
    std::list<const char*> availableAgents = _scheduler.agentIds();

    std::vector<std::string> agents;

    if (args.empty())
        return "Error: No agents to stop specified\n";

    for (std::list<std::string>::const_iterator it = args.begin();
         it != args.end(); ++it) {
        std::string agent_type = *it;
        // check that this is a valid type; give an error and return otherwise
        if (availableAgents.end() ==
         find(availableAgents.begin(), availableAgents.end(), *it)) {
            std::ostringstream oss;
            oss << "Invalid Agent ID \"" << *it << "\"\n";
            return oss.str();
        }

        agents.push_back(agent_type);
    }

    // Doesn't give an error if there is no instance of that agent type
    // running.  TODO FIXME.  Should check.
    for (std::vector<std::string>::const_iterator it = agents.begin();
         it != agents.end(); ++it)
    {
        _scheduler.stopAllAgents(*it);
    }

    return "Successfully stopped agents\n";
}

std::string AgentsModule::do_stepAgents(Request *dummy, std::list<std::string> args)
{
    AgentSeq agents = _scheduler.runningAgents();

    if (args.empty()) {
        for (AgentSeq::const_iterator it = agents.begin();
             it != agents.end(); ++it) {
            (*it)->run();
        }
        return "Ran a step of each active agent\n";
    } else {
        std::list<std::string> unknownAgents;
        int numberAgentsRun = 0;
        for (std::list<std::string>::const_iterator it = args.begin();
             it != args.end(); ++it) {

            std::string agent_type = *it;

            // try to find an already started agent with that name
            AgentSeq::const_iterator tmp = agents.begin();
            for ( ; tmp != agents.end() ; ++tmp ) if ( *it == (*tmp)->classinfo().id ) break;

            AgentPtr agent;
            if (agents.end() == tmp) {
                // construct a temporary agent
                agent = AgentPtr(_scheduler.createAgent(*it, false));
                if (agent) {
                    agent->run();
                    _scheduler.stopAgent(agent);
                    numberAgentsRun++;
                } else {
                    unknownAgents.push_back(*it);
                }
            } else {
                agent = *tmp;
                agent->run();
            }
        }
        std::stringstream returnMsg;
        for (std::list<std::string>::iterator it = unknownAgents.begin();
                it != unknownAgents.end(); ++it) {
            returnMsg << "Unknown agent \"" << *it << "\"" << std::endl;
        }
        returnMsg << "Successfully ran a step of " << numberAgentsRun <<
            "/" << args.size() << " agents." << std::endl;
        return returnMsg.str();
    }
}

std::string AgentsModule::do_stopAgentLoop(Request *dummy, std::list<std::string> args)
{
    _scheduler.stopAgentLoop();

    return "Stopped agent loop\n";
}

std::string AgentsModule::do_startAgentLoop(Request *dummy, std::list<std::string> args)
{
    _scheduler.startAgentLoop();

    return "Started agent loop\n";
}

std::string AgentsModule::do_listAgents(Request *dummy, std::list<std::string> args)
{
    std::list<const char*> agentNames = _scheduler.agentIds();
    std::ostringstream oss;

    for (std::list<const char*>::const_iterator it = agentNames.begin();
         it != agentNames.end(); ++it) {
        oss << (*it) << std::endl;
    }

    return oss.str();
}

std::string AgentsModule::do_activeAgents(Request *dummy, std::list<std::string> args)
{
    AgentSeq agents = _scheduler.runningAgents();
    std::ostringstream oss;

    for (AgentSeq::const_iterator it = agents.begin();
         it != agents.end(); ++it) {
        oss << (*it)->to_string() << std::endl;
    }

    return oss.str();
}
