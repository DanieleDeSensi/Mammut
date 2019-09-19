#ifndef MAMMUT_PROCESS_HPP_
#define MAMMUT_PROCESS_HPP_

#include "../utils.hpp"
#include "../module.hpp"
#include "../topology/topology.hpp"

#include "sys/resource.h"
#define MAMMUT_PROCESS_PRIORITY_MIN (uint) 0
#define MAMMUT_PROCESS_PRIORITY_MAX (uint) (PRIO_MAX - PRIO_MIN)

namespace mammut{
namespace task{

class Task{
public:
    /**
     * Returns the identifier of this task.
     * @return The identifier of this task.
     */
    virtual TaskId getId() const = 0;

    /**
     * Returns the percentage of time spent by this execution unit on a processing
     * core. The percentage is computed over a period of time spanning from the
     * last call of resetCoreUsage (or from the creation of this object) to the
     * moment of this call.
     * @param coreUsage The percentage of time spent by this execution unit on a processing
     * core.
     * @return If false is returned, this execution unit is no more active and the call failed.
     *         Otherwise, true is returned.
     */
    virtual bool getCoreUsage(double& coreUsage) const = 0; //TODO: Time AND percentage

    /**
     * Resets the counter for core usage percentage computation.
     * @return If false is returned, this execution unit is no more active and the call failed.
     *         Otherwise, true is returned.
     */
    virtual bool resetCoreUsage() = 0;

    /**
     * Gets the current priority of this execution unit.
     * @param priority The current priority of this execution unit. The higher
     *        is the value, the higher is the priority. It will be in
     *        the range [MAMMUT_PROCESS_PRIORITY_MIN, MAMMUT_PROCESS_PRIORITY_MAX]
     * @return If false is returned, this execution unit is no more active and the call failed.
     *         Otherwise, true is returned.
     */
    virtual bool getPriority(uint& priority) const = 0;

    /**
     * Sets the priority of this execution unit.
     * NOTE: If executed on a process, the priority of all its thread will be changed too.
     * NOTE: It may require privileged rights.
     * @param priority The priority of this execution unit. The higher
     *        is the value, the higher is the priority. It must be in
     *        the range [MAMMUT_PROCESS_PRIORITY_MIN, MAMMUT_PROCESS_PRIORITY_MAX]
     * @return If false is returned, the priority value is outside the allowed range,
     *         this execution unit is no more active or you do not have the rights
     *         to change the priority.
     *         Otherwise, true is returned.
     */
    virtual bool setPriority(uint priority) const = 0;

    /**
     * Gets the identifier of the virtual core on which this unit is currently running.
     * @param virtualCoreId The identifier of the virtual core on which this unit is currently running.
     * @return If false is returned, this execution unit is no more active and the call failed.
     *         Otherwise, true is returned.
     */
    virtual bool getVirtualCoreId(topology::VirtualCoreId& virtualCoreId) const = 0;

    /**
     * Returns the set of virtual cores on which this unit is allowed to run.
     * @param vcs The set of virtual cores on which this unit is allowed to run.
     * @return If false is returned, this execution unit is no more active and the call failed.
     *         Otherwise, true is returned.
     */
    virtual bool getVirtualCoreIds(std::vector<topology::VirtualCoreId>& virtualCoresIds) const = 0;

    /**
     * Move this execution unit on a specified CPU.
     * NOTE: If executed on a process, all its threads will be moved too.
     * @param cpu The CPU on which this execution unit must be moved.
     * @return If false is returned, this execution unit is no more active and the call failed.
     *         Otherwise, true is returned.
     */
    virtual bool move(const topology::Cpu* cpu) const = 0;

    /**
     * Move this execution unit on a specified physical core.
     * NOTE: If executed on a process, all its threads will be moved too.
     * @param physicalCore The physical core on which this execution unit must be moved.
     * @return If false is returned, this execution unit is no more active and the call failed.
     *         Otherwise, true is returned.
     */
    virtual bool move(const topology::PhysicalCore* physicalCore) const = 0;

    /**
     * Move this execution unit on a specified virtual core.
     * NOTE: If executed on a process, all its threads will be moved too.
     * @param virtualCore The virtual core on which this execution unit must be moved.
     * @return If false is returned, this execution unit is no more active and the call failed.
     *         Otherwise, true is returned.
     */
    virtual bool move(const topology::VirtualCore* virtualCore) const = 0;

    /**
     * Move this execution unit on a specified virtual core.
     * NOTE: If executed on a process, all its threads will be moved too.
     * @param virtualCoreId The identifier of the virtual core on which this execution unit must
     *                      be moved.
     * @return If false is returned, this execution unit is no more active and the call failed.
     *         Otherwise, true is returned.
     */
    virtual bool move(topology::VirtualCoreId virtualCoreId) const = 0;

    /**
     * Move this execution unit on a set of specified virtual cores.
     * @param virtualCores The virtual cores on which this execution unit must be moved.
     * @return If false is returned, this execution unit is no more active and the call failed.
     *         Otherwise, true is returned.
     */
    virtual bool move(const std::vector<const topology::VirtualCore*>& virtualCores) const = 0;

    /**
     * Move this execution unit on a set of specified virtual cores.
     * @param virtualCores The virtual cores on which this execution unit must be moved.
     * @return If false is returned, this execution unit is no more active and the call failed.
     *         Otherwise, true is returned.
     */
    virtual bool move(const std::vector<topology::VirtualCore*>& virtualCores) const = 0;

    /**
     * Move this execution unit on a set of specified virtual cores.
     * @param virtualCoresIds The identifiers of the virtual cores on which this execution unit
     *                        must be moved.
     * @return If false is returned, this execution unit is no more active and the call failed.
     *         Otherwise, true is returned.
     */
    virtual bool move(const std::vector<topology::VirtualCoreId>& virtualCoresIds) const = 0;

    /**
     * Returns true if this execution unit is still active, false otherwise.
     * @return True if this execution unit is still active, false otherwise.
     */
    virtual bool isActive() const = 0;

    virtual ~Task(){;}
};

class ThreadHandler: public virtual Task{
public:
    virtual ~ThreadHandler(){;}
};

class ProcessHandler: public virtual Task{
public:
    /**
     * Returns a list of active thread identifiers on this process.
     * @return A vector of active thread identifiers on this process.
     */
    virtual std::vector<TaskId> getActiveThreadsIdentifiers() const = 0;

    /**
     * Returns the handler associated to a specific thread.
     * @param tid The thread identifier.
     * @return The handler associated to a specific thread or
     *         NULL if the thread doesn't exists. The obtained
     *         handler must be released with releaseThreadHandler call.
     */
    virtual ThreadHandler* getThreadHandler(TaskId tid) const = 0;

    /**
     * Releases the handler obtained through getThreadHandler call.
     * @param thread The thread handler.
     */
    virtual void releaseThreadHandler(ThreadHandler* thread) const = 0;

    virtual ~ProcessHandler(){;}

    /**
     * Returns the instructions executed since the last call
     * of resetInstructions() or getAndResetInstructions() or since the creation of
     * this handler.
     * The count will consider this Process and all the children and
     * threads created since the creation of this handler.
     * @param instructions The instructions executed since the last call
     *               of resetInstructions() or getAndResetInstructions() or since the
     *               creation of this handler.
     * @return True if the process is still active, false otherwise.
     */
    virtual bool getInstructions(double& instructions) = 0;

    /**
     * Resets the count of instructions per cycle.
     * @return True if the process is still active, false otherwise.
     */
    virtual bool resetInstructions() = 0;

    /**
     * Returns the instructions executed since the last call
     * of resetInstructions() or getAndResetInstructions() or since the creation of
     * this handler. Then resets the counter.
     * The count will consider this Process and all the children and
     * threads created since the creation of this handler.
     * @param instructions The instructions executed since the last call
     *               of resetInstructions() or getAndResetInstructions() or since the
     *               creation of this handler.
     * @return True if the process is still active, false otherwise.
     */
    virtual bool getAndResetInstructions(double& instructions) = 0;

    /**
     * Throttles this process by a specified percentage value.
     * @param percentage The percentage of time the process should execute on
     * the CPU. E.g. if percentage == 30, each second of the execution the
     * process will run for 0.3 seconds and sleep for 0.7 seconds.
     * This value must be included in the range ]0, 100].
     * ATTENTION: If throttling is required for more than one process,
     * the sum of their percentages cannot be greater than 100. For example,
     * if you require a 30% throttling on process A, and a 80% throttling on
     * process B, when requiring throttling for process B an exception will
     * be thrown.
     * This is a temporary limitation which would be removed in future
     * versions.
     * @return If false is returned, this execution unit is no more active
     *         and the call failed. Otherwise, true is returned.
     */
    virtual bool throttle(double percentage) = 0;

    /**
     * Removes throttling from this process.
     * @return If false is returned, this execution unit is no more active
     *         and the call failed. Otherwise, true is returned.
     */
    virtual bool removeThrottling() = 0;

    /**
     * Sends a signal to this process.
     * @param signal The type of signal.
     * @return If false is returned, this execution unit is no more active
     *         and the call failed. Otherwise, true is returned.
     */
    virtual bool sendSignal(int signal) const = 0;
};

class TasksManager: public Module{
    MAMMUT_MODULE_DECL(TasksManager)
public:
    /**
     * Returns a list of active processes identifiers.
     * @return A vector of active processes identifiers.
     */
    virtual std::vector<TaskId> getActiveProcessesIdentifiers() const = 0;

    /**
     * Returns the handler associated to a specific process.
     * @param pid The process identifier.
     * @return The handler associated to a specific process or
     *         NULL if the process doesn't exists. The obtained
     *         handler must be released with releaseProcessHandler call.
     */
    virtual ProcessHandler* getProcessHandler(TaskId pid) = 0;

    /**
     * Releases the handler obtained through getProcessHandler call.
     * @param process The process handler.
     */
    virtual void releaseProcessHandler(ProcessHandler* process) const = 0;

    /**
     * Sets the throttling interval.
     * @param throttlingInterval Throttling interval (microseconds).
     **/
    virtual void setThrottlingInterval(ulong throttlingInterval) = 0;

    /**
     * Returns the handler associated to a specific thread.
     * @param pid The process identifier.
     * @param tid The thread identifier.
     * @return The handler associated to a specific thread or
     *         NULL if the thread doesn't exists. The obtained
     *         handler must be released with releaseThreadHandler call.
     */
    virtual ThreadHandler* getThreadHandler(TaskId pid, TaskId tid) const = 0;

    /**
     * Returns the handler associated to the calling thread.
     * @return The handler associated to the calling thread.
     *         The obtained handler must be released with
     *         releaseThreadHandler call.
     */
    virtual ThreadHandler* getThreadHandler() const = 0;

    /**
     * Releases the handler obtained through getThreadHandler call.
     * @param thread The thread handler.
     */
    virtual void releaseThreadHandler(ThreadHandler* thread) const = 0;
};

}
}

#endif /* MAMMUT_PROCESS_HPP_ */
