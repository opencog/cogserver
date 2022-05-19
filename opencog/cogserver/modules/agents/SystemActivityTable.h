/*
 * opencog/cogserver/modules/agents/SystemActivityTable.h
 *
 * Copyright (C) 2009 by OpenCog Foundation
 * Written by Trent Waddington <trent.waddington@gmail.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef _OPENCOG_SYSTEM_ACTIVITY_TABLE_H
#define _OPENCOG_SYSTEM_ACTIVITY_TABLE_H

#include <string>
#include <queue>
#include <map>
#include <mutex>
#include <vector>

#include <opencog/cogserver/modules/agents/Agent.h>
#include <opencog/cogserver/modules/agents/SystemActivityTable.h>

namespace opencog
{
/** \addtogroup grp_server
 *  @{
 */

/**
 * This class implements storage for activity data.
 */
class Activity
{
public:
    Activity(long cycleCount, std::chrono::system_clock::duration elapsedTime,
                              size_t memUsed, size_t atomsUsed,
                              const std::vector<UnorderedHandleSet>& utilized) :
            cycleCount(cycleCount),
            elapsedTime(elapsedTime),
            memUsed(memUsed),
            atomsUsed(atomsUsed) {
        for (size_t n = 0; n < utilized.size(); n++)
            utilizedHandleSets.push_back(utilized[n]);
    }
    long cycleCount;
    std::chrono::system_clock::duration elapsedTime;
    size_t memUsed;
    size_t atomsUsed;
    std::vector<UnorderedHandleSet> utilizedHandleSets;
};
typedef std::vector<Activity*> ActivitySeq;
typedef std::map<AgentPtr, ActivitySeq> AgentActivityTable;

class CogServer;

/**
 * This class implements the entity responsible for logging of all system
 * activity.
 *
 * See http://opencog.org/wiki/OpenCogPrime:AttentionalDataMining
 */
class SystemActivityTable
{
protected:
    AgentActivityTable _agentActivityTable;
    size_t _maxAgentActivityTableSeqSize;
    CogServer* _cogServer;

    /** Protects _agentActivityTable from concurrent access. It is possible to
     * implement fine grained locking per agent. */
    std::mutex _activityTableMutex;

    /** trim Activity sequence to a maximum size */
    void trimActivitySeq(ActivitySeq&, size_t max);

public:

    /** constructor. */
    SystemActivityTable();

    /** destructor. */
    virtual ~SystemActivityTable();

    /** initialize the SystemActivityTable */
    virtual void init(CogServer*);
    virtual void halt();

    /** Returns the agent activity table
     *  Activities will be listed with the most recent first. */
    AgentActivityTable agentActivityTable() {
        std::lock_guard<std::mutex> lock(_activityTableMutex);
        return _agentActivityTable;
    }

    /** Get the maximum size of a sequence in the AgentActivityTable */
    size_t maxAgentActivityTableSeqSize() const {
        return _maxAgentActivityTableSeqSize;
    }

    /** Set the maximum size of a sequence in the AgentActivityTable */
    void setMaxAgentActivityTableSeqSize(size_t);

    /** Logs activity of an Agent.
     *
     *  This will call agent->getUtilizedHandleSets() to get a list of handle
     *  sets utilized in the activity that has just been completed.
     */
    void logActivity(AgentPtr, std::chrono::system_clock::duration,
        size_t memUsed, size_t atomsUsed);

    /** Clear activity of a specified Agent. */
    void clearActivity(AgentPtr);

    /** Clear all activity */
    void clearActivity();

}; // class

/** @}*/
}  // namespace

#endif // _OPENCOG_SYSTEM_ACTIVITY_TABLE_H
