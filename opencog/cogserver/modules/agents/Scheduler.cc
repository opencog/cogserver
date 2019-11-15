/*
 * opencog/cogserver/modules/agents/Scheduler.cc
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * Copyright (C) 2008 by OpenCog Foundation
 * Written by Andre Senna <senna@vettalabs.com>
 *            Gustavo Gama <gama@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#include <opencog/util/Logger.h>
#include <opencog/util/exceptions.h>
#include <opencog/util/Config.h>

#include <opencog/cogserver/server/CogServer.h>
#include <opencog/cogserver/modules/agents/Agent.h>
#include <opencog/cogserver/modules/agents/SystemActivityTable.h>

#include "Scheduler.h"

using namespace opencog;

namespace opencog {
struct equal_to_id :
    public std::binary_function<AgentPtr, const std::string&, bool>
{
    bool operator()(AgentPtr a, const std::string& cid) const {
        return (a->classinfo().id != cid);
    }
};
}

Scheduler::~Scheduler()
{
    // Shut down the system activity table.
    _systemActivityTable.halt();

    logger().debug("[Scheduler] exit destructor");
}

Scheduler::Scheduler(void) :
    cycleCount(1), running(false)
{
    _systemActivityTable.init(&cogserver());
    agentScheduler.set_activity_table(&_systemActivityTable);

    agentsRunning = true;
}

SystemActivityTable& Scheduler::systemActivityTable()
{
    return _systemActivityTable;
}

void Scheduler::serverLoop()
{
    struct timeval timer_start, timer_end, elapsed_time;
    time_t cycle_duration = config().get_int("SERVER_CYCLE_DURATION", 100) * 1000;
//    bool externalTickMode = config().get_bool("EXTERNAL_TICK_MODE");

    logger().info("Starting Scheduler loop.");

    gettimeofday(&timer_start, NULL);
    for (running = true; running;)
    {
        runLoopStep();

        gettimeofday(&timer_end, NULL);
        timersub(&timer_end, &timer_start, &elapsed_time);

        // sleep long enough so that the next cycle will only start
        // after config["SERVER_CYCLE_DURATION"] milliseconds
        long delta = cycle_duration;
        delta -= 1000.0*elapsed_time.tv_sec + elapsed_time.tv_usec/1000.0;
        if (delta > 0)
            usleep((unsigned int) delta);
        timer_start = timer_end;
    }
}

void Scheduler::runLoopStep(void)
{
    struct timeval timer_start, timer_end, elapsed_time;
    // this refers to the current cycle, so that logging reports correctly
    // regardless of whether cycle is incremented.
    long currentCycle = this->cycleCount;

    // Process mind agents
    if (customLoopRun() and agentsRunning and 0 < agentScheduler.get_agents().size())
    {
        gettimeofday(&timer_start, NULL);
        agentScheduler.process_agents();

        gettimeofday(&timer_end, NULL);
        timersub(&timer_end, &timer_start, &elapsed_time);
        logger().fine("[Scheduler::runLoopStep cycle = %d] Time to process MindAgents: %f",
               currentCycle,
               elapsed_time.tv_usec/1000000.0, currentCycle
              );
    }

    cycleCount++;
    if (cycleCount < 0) cycleCount = 0;
}

bool Scheduler::customLoopRun(void)
{
    return true;
}

bool Scheduler::registerAgent(const std::string& id, AbstractFactory<Agent> const* factory)
{
    return Registry<Agent>::register_(id, factory);
}

bool Scheduler::unregisterAgent(const std::string& id)
{
    logger().debug("[Scheduler] unregister agent \"%s\"", id.c_str());
    stopAllAgents(id);
    return Registry<Agent>::unregister(id);
}

std::list<const char*> Scheduler::agentIds() const
{
    return Registry<Agent>::all();
}

AgentSeq Scheduler::runningAgents(void)
{
    AgentSeq agents = agentScheduler.get_agents();
    for (auto &runner: agentThreads) {
        auto t = runner->get_agents();
        agents.insert(agents.end(), t.begin(), t.end());
    }
    return agents;
}

AgentPtr Scheduler::createAgent(const std::string& id, const bool start)
{
    AgentPtr a(Registry<Agent>::create(cogserver(), id));
    if (a && start) startAgent(a);
    return a;
}

void Scheduler::startAgent(AgentPtr agent, bool dedicated_thread,
    const std::string &thread_name)
{
    if (dedicated_thread) {
        AgentRunnerThread *runner;
        auto runner_ptr = threadNameMap.find(thread_name);
        if (runner_ptr != threadNameMap.end())
            runner = runner_ptr->second;
        else {
            agentThreads.emplace_back(new AgentRunnerThread);
            runner = agentThreads.back().get();
            runner->set_activity_table(&_systemActivityTable);
            if (!thread_name.empty())
            {
                runner->set_name(thread_name);
                threadNameMap[thread_name] = runner;
            }
        }
        runner->add_agent(agent);
        if (agentsRunning)
            runner->start();
    }
    else
        agentScheduler.add_agent(agent);
}

void Scheduler::stopAgent(AgentPtr agent)
{
    agentScheduler.remove_agent(agent);
    for (auto &runner: agentThreads)
        runner->remove_agent(agent);
    logger().debug("[Scheduler] stopped agent \"%s\"", agent->to_string().c_str());
}

void Scheduler::stopAllAgents(const std::string& id)
{
    agentScheduler.remove_all_agents(id);
    for (auto &runner: agentThreads)
        runner->remove_all_agents(id);
}

void Scheduler::startAgentLoop(void)
{
    agentsRunning = true;
    for (auto &runner: agentThreads)
        runner->start();
}

void Scheduler::stopAgentLoop(void)
{
    agentsRunning = false;
    for (auto &runner: agentThreads)
        runner->stop();
}

long Scheduler::getCycleCount()
{
    return cycleCount;
}

void Scheduler::stop()
{
    running = false;
}
