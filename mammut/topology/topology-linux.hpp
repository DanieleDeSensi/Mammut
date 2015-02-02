/*
 * topology-linux.hpp
 *
 * Created on: 14/11/2014
 *
 * =========================================================================
 *  Copyright (C) 2014-, Daniele De Sensi (d.desensi.software@gmail.com)
 *
 *  This file is part of mammut.
 *
 *  mammut is free software: you can redistribute it and/or
 *  modify it under the terms of the Lesser GNU General Public
 *  License as published by the Free Software Foundation, either
 *  version 3 of the License, or (at your option) any later version.

 *  mammut is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  Lesser GNU General Public License for more details.
 *
 *  You should have received a copy of the Lesser GNU General Public
 *  License along with mammut.
 *  If not, see <http://www.gnu.org/licenses/>.
 *
 * =========================================================================
 */

#ifndef MAMMUT_TOPOLOGY_LINUX_HPP_
#define MAMMUT_TOPOLOGY_LINUX_HPP_

#include "topology.hpp"

namespace mammut{
namespace topology{

std::string getTopologyPathFromVirtualCoreId(VirtualCoreId id);

class TopologyLinux: public Topology{
public:
    TopologyLinux();
    void maximizeUtilization() const;
    void resetUtilization() const;
};

class CpuLinux: public Cpu{
private:
    std::string getCpuInfo(const std::string& infoName) const;
public:
    CpuLinux(CpuId cpuId, std::vector<PhysicalCore*> physicalCores);
    std::string getVendorId() const;
    std::string getFamily() const;
    std::string getModel() const;
    void maximizeUtilization() const;
    void resetUtilization() const;
};

class PhysicalCoreLinux: public PhysicalCore{
public:
    PhysicalCoreLinux(CpuId cpuId, PhysicalCoreId physicalCoreId, std::vector<VirtualCore*> virtualCores);
    void maximizeUtilization() const;
    void resetUtilization() const;
};

class VirtualCoreLinux;

class VirtualCoreIdleLevelLinux: public VirtualCoreIdleLevel{
private:
    const VirtualCoreLinux& _virtualCore;
    std::string _path;
    uint _lastAbsTime;
    uint _lastAbsCount;

    //uint64_t _ticks;
    //uint64_t _tmp;
public:
    VirtualCoreIdleLevelLinux(const VirtualCoreLinux& virtualCore, uint levelId);
    std::string getName() const;
    std::string getDesc() const;
    bool isEnableable() const;
    bool isEnabled() const;
    void enable() const;
    void disable() const;
    uint getExitLatency() const;
    uint getConsumedPower() const;
    uint getAbsoluteTime() const;
    uint getTime() const;
    void resetTime();
    uint getAbsoluteCount() const;
    uint getCount() const;
    void resetCount();
};

/**
 * Rise the utilization to 100%.
 * To start:
 *       setStop(false);
 *       start();
 * To stop:
 *       setStop(true);
 *       join();
 */
class SpinnerThread: public utils::Thread{
private:
    utils::LockPthreadMutex _lock;
    bool _stop;

    bool isStopped();
public:
    SpinnerThread();

    void setStop(bool s);
    void run();
};

typedef enum{
    PROC_STAT_NAME = 0,
    PROC_STAT_USER,
    PROC_STAT_NICE,
    PROC_STAT_SYSTEM,
    PROC_STAT_IDLE,
    PROC_STAT_IOWAIT,
    PROC_STAT_IRQ,
    PROC_STAT_SOFTIRQ,
    PROC_STAT_STEAL,
    PROC_STAT_GUEST,
    PROC_STAT_GUEST_NICE
}ProcStatTimeType;

class VirtualCoreLinux: public VirtualCore{
private:
    std::string _hotplugFile;
    std::vector<VirtualCoreIdleLevel*> _idleLevels;
    double _lastProcIdleTime;
    SpinnerThread* _utilizationThread;
    utils::Msr _msr;

    /**
     * Returns a specified time field of the line of this virtual core in /proc/stat file (in microseconds).
     * @return A specified time field of the line of this virtual core in /proc/stat file (in microseconds).
     *         -1 is returned if the field does not exists.
     */
    double getProcStatTime(ProcStatTimeType type) const;

    /**
     * Returns the absolute idle time of this virtual core (in microseconds).
     * @return The absolute idle time of this virtual core (in microseconds).
     */
    double getAbsoluteIdleTime() const;
public:
    VirtualCoreLinux(CpuId cpuId, PhysicalCoreId physicalCoreId, VirtualCoreId virtualCoreId);
    ~VirtualCoreLinux();

    uint64_t getAbsoluteTicks() const;
    void maximizeUtilization() const;
    void resetUtilization() const;
    double getIdleTime() const;
    void resetIdleTime();

    bool isHotPluggable() const;
    bool isHotPlugged() const;
    void hotPlug() const;
    void hotUnplug() const;

    std::vector<VirtualCoreIdleLevel*> getIdleLevels() const;
};

}
}

#endif /* MAMMUT_TOPOLOGY_LINUX_HPP_ */
