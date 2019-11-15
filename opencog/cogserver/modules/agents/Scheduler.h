/*
 * opencog/cogserver/modules/agents/Scheduler.h
 *
 * Copyright (C) 2008 by OpenCog Foundation
 * Written by Gustavo Gama <gama@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef _OPENCOG_AGENTS_SCHEDULER_H
#define _OPENCOG_AGENTS_SCHEDULER_H

#include <list>
#include <map>
#include <memory>
#include <vector>

#include <opencog/cogserver/server/Registry.h>
#include <opencog/cogserver/modules/agents/Agent.h>
#include <opencog/cogserver/modules/agents/AgentRunnerThread.h>
#include <opencog/cogserver/modules/agents/SystemActivityTable.h>

namespace opencog
{
/** \addtogroup grp_server
 *  @{
 */

/**
 * This class provides the main loop that runs agents.
 *
 * Cycles are handled by the server's main loop (method 'serverLoop').
 * Each cycle has a minimum duration controlled by the parameter
 * "SERVER_CYCLE_DURATION" (in seconds). At the start of every cycle,
 * the server processes the queued requests and then executes an
 * interaction of each scheduled agent. When all the agents are finished,
 * the server sleeps for the remaining time until the end of the cycle.
 *
 * Agent management is done through inheritance from the Registry<Agent>
 * class.  The agent registry API provides several methods to:
 *   1. register, unregister and list agents classes;
 *   2. create and destroy agent instances;
 *   3. start and stop agents.
 * We chose to wrap the register methods in the server class to avoid
 * conflicts with the other registry inheritance (Registry<Command>).
 */
class Scheduler : public Registry<Agent>
{

protected:
    typedef std::unique_ptr<AgentRunnerThread> AgentRunnerThreadPtr;

    long cycleCount;
    bool running;
    // Used to start and stop the Agents loop via shell commands
    bool agentsRunning;

    SimpleRunner agentScheduler;
    std::vector<AgentRunnerThreadPtr> agentThreads;
    std::map<std::string, AgentRunnerThread*> threadNameMap;

    SystemActivityTable _systemActivityTable;

public:
    Scheduler(void);
    ~Scheduler();

    /**** Agent Registry API ****/
    /** Register a new agent class/type. Takes the class' id and a
     *  derived factory for this particular agent type. (note: the
     *  caller owns the factory instance). */
    virtual bool registerAgent(const std::string& id, AbstractFactory<Agent> const* factory);

    /** Unregister an agent class/type. Takes the class' id. */
    virtual bool unregisterAgent(const std::string& id);

    /** Returns a list with the ids of all the registered agent classes. */
    virtual std::list<const char*> agentIds(void) const;

    /** Returns a list of all the currently running agent instances. */
    virtual AgentSeq runningAgents(void);

    /** Creates and returns a new instance of an agent of class 'id'.
     *  If 'start' is true, then the agent will be automatically added
     *  to the list of scheduled agents. */
    virtual AgentPtr createAgent(const std::string& id,
                                 const bool start = false);

    /// Same as above, but returns the correct type.
    template <typename T>
    std::shared_ptr<T> createAgent(const bool start = false) {
        return std::dynamic_pointer_cast<T>(createAgent(T::info().id, start));
    }

    /** Adds agent 'a' to the list of scheduled agents. */
    virtual void startAgent(AgentPtr a, bool dedicated_thread = false,
        const std::string &thread_name = "");

    /** Removes agent 'a' from the list of scheduled agents. */
    virtual void stopAgent(AgentPtr a);

    /** Destroys all agents from class 'id' */
    virtual void stopAllAgents(const std::string& id);

    /** Starts running agents as part of the serverLoop (enabled by default) */
    virtual void startAgentLoop(void);

    /** Stops running agents as part of the serverLoop */
    virtual void stopAgentLoop(void);

    void serverLoop(void);
    void runLoopStep(void);

    /** Customized server loop run. This method is called inside serverLoop
     *  (between processing request queue and scheduled agents) and can be
     *  overwritten by CogServer's subclasses in order to customize the
     *  server loop behavior.
     *
     *  This method controls the execution of server cycles by returning
     *  'true' if the server must run a cycle and 'false' if it must not.
     *  By default it does nothing and returns true.
     *
     *  If EXTERNAL_TICK_MODE config parameter is enabled, serverLoop
     *  will not go sleep at all. So, in this case, this method must be
     *  in charge of going sleep when the server is idle to prevent
     *  a busy-wait loop. */
    virtual bool customLoopRun(void);

    /** Terminates the main loop. The loop will be exited
     *  after the current interaction is finished. */
    virtual void stop(void);

    /** Returns the number of executed cycles so far */
    virtual long getCycleCount(void);

    /** Returns a reference to the system activity table instance */
    virtual SystemActivityTable& systemActivityTable(void);

}; // class

/** @}*/
}  // namespace

#endif // _OPENCOG_AGENTS_SCHEDULER_H
