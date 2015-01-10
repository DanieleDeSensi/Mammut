/*
 * topology.hpp
 *
 * Created on: 20/11/2014
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

/**
 * The following terminology will be coherently used in documentation/function names.
 *
 * CPU: A system is composed by one or more CPU. Each of them has one or more physical core.
 * Physical Core: The basic computation unit of the CPU.
 * Virtual core: When HyperThreading is present, more virtual cores will correspond to a same physical core.
 **/

#ifndef TOPOLOGY_HPP_
#define TOPOLOGY_HPP_

#include <mammut/communicator.hpp>
#include <mammut/module.hpp>

#include <stdint.h>
#include <vector>

namespace mammut{
namespace topology{

class VirtualCore;
class PhysicalCore;
class Cpu;

typedef uint32_t CpuId;
typedef uint32_t PhysicalCoreId;
typedef uint32_t VirtualCoreId;

typedef struct{
    CpuId cpuId;
    PhysicalCoreId physicalCoreId;
    VirtualCoreId virtualCoreId;
}VirtualCoreCoordinates;

class Topology: public Module{
    MAMMUT_MODULE_DECL(Topology)
private:
    Communicator* const _communicator;
    std::vector<Cpu*> _cpus;
    std::vector<PhysicalCore*> _physicalCores;
    std::vector<VirtualCore*> _virtualCores;

    Topology();
    Topology(Communicator* const communicator);
    ~Topology();
    void buildCpuVector(std::vector<VirtualCoreCoordinates> coord);
    std::vector<PhysicalCore*> buildPhysicalCoresVector(std::vector<VirtualCoreCoordinates> coord, CpuId cpuId);
    std::vector<VirtualCore*> buildVirtualCoresVector(std::vector<VirtualCoreCoordinates> coord, CpuId cpuId, PhysicalCoreId physicalCoreId);
    bool processMessage(const std::string& messageIdIn, const std::string& messageIn,
                                    std::string& messageIdOut, std::string& messageOut);
public:
    /**
     * Returns the CPUs of the system.
     * @return A vector of CPus.
     */
    std::vector<Cpu*> getCpus() const;

    /**
     * Returns the physical cores of the system.
     * @return A vector of physical cores.
     */
    std::vector<PhysicalCore*> getPhysicalCores() const;

    /**
     * Returns the virtual cores of the system.
     * @return A vector of virtual cores.
     */
    std::vector<VirtualCore*> getVirtualCores() const;

    /**
     * Returns the Cpu with the given identifier, or NULL if it is not present.
     * @param cpuId The identifier of the Cpu.
     * @return The Cpu with the given identifier, or NULL if it is not present.
     */
    Cpu* getCpu(CpuId cpuId) const;

    /**
     * Returns the physical core with the given identifier, or NULL if it is not present.
     * @param physicalCoreId The identifier of the physical core.
     * @return The physical core with the given identifier, or NULL if it is not present.
     */
    PhysicalCore* getPhysicalCore(PhysicalCoreId physicalCoreId) const;

    /**
     * Returns the virtual core with the given identifier, or NULL if it is not present.
     * @param virtualCoreId The identifier of the virtual core.
     * @return The virtual core with the given identifier, or NULL if it is not present.
     */
    VirtualCore* getVirtualCore(VirtualCoreId virtualCoreId) const;

    /**
     * Returns a virtual core belonging to the topology, or NULL if it is not present.
     * @return A virtual core belonging to the topology, or NULL if it is not present.
     */
    VirtualCore* getVirtualCore() const;
};

class Cpu{
private:
    const CpuId _cpuId;
    const std::vector<PhysicalCore*> _physicalCores;
    const std::vector<VirtualCore*> _virtualCores;

    std::vector<VirtualCore*> virtualCoresFromPhysicalCores();
protected:
    Cpu(CpuId cpuId, std::vector<PhysicalCore*> physicalCores);
public:
    /**
     * Returns the identifier of this CPU.
     * @return The identifier of this CPU.
     */
    CpuId getCpuId() const;

    /**
     * Returns the physical cores of this CPU.
     * @return A vector of physical cores.
     */
    std::vector<PhysicalCore*> getPhysicalCores() const;

    /**
     * Returns the virtual cores of this CPU.
     * @return A vector of virtual cores.
     */
    std::vector<VirtualCore*> getVirtualCores() const;

    /**
     * Returns the physical core with the given identifier, or NULL if it is not present on this CPU.
     * @param physicalCoreId The identifier of the physical core.
     * @return The physical core with the given identifier, or NULL if it is not present on this CPU.
     */
    PhysicalCore* getPhysicalCore(PhysicalCoreId physicalCoreId) const;

    /**
     * Returns the virtual core with the given identifier, or NULL if it is not present on this CPU.
     * @param virtualCoreId The identifier of the virtual core.
     * @return The virtual core with the given identifier, or NULL if it is not present on this CPU.
     */
    VirtualCore* getVirtualCore(VirtualCoreId virtualCoreId) const;

    /**
     * Returns a virtual core belonging to this Cpu, or NULL if it is not present.
     * @return A virtual core belonging to this Cpu, or NULL if it is not present.
     */
    VirtualCore* getVirtualCore() const;

    /**
     * Returns the vendor id of this Cpu.
     * @return The vendor id of this Cpu.
     */
    virtual std::string getVendorId() const = 0;

    /**
     * Returns the family of this Cpu.
     * @return The family of this Cpu.
     */
    virtual std::string getFamily() const = 0;

    /**
     * Returns the model of this Cpu.
     * @return The model of this Cpu.
     */
    virtual std::string getModel() const = 0;

    virtual inline ~Cpu(){;}
};

class PhysicalCore{
private:
    const CpuId _cpuId;
    const PhysicalCoreId _physicalCoreId;
    const std::vector<VirtualCore*> _virtualCores;
protected:
    PhysicalCore(CpuId cpuId, PhysicalCoreId physicalCoreId, std::vector<VirtualCore*> virtualCores);
public:
    /**
     * Returns the identifier of this physical core.
     * @preturn The identifier of this physical core.
     */
    PhysicalCoreId getPhysicalCoreId() const;

    /**
     * Returns the identifier of the CPU where this physical core
     * is running.
     * @return The identifier of the CPU where this physical core
     * is running.
     */
    CpuId getCpuId() const;

    /**
     * Returns the virtual cores associated to
     * this physical core.
     * @return The virtual cores associated to
     *         this physical core.
     */
    std::vector<VirtualCore*> getVirtualCores() const;

    /**
     * Returns the virtual core with the given identifier, or NULL if it is not present on this physical core.
     * @param virtualCoreId The identifier of the virtual core.
     * @return The virtual core with the given identifier, or NULL if it is not present on this physical core.
     */
    VirtualCore* getVirtualCore(VirtualCoreId virtualCoreId) const;

    /**
     * Returns a virtual core belonging to this physical core, or NULL if it is not present.
     * @return A virtual core belonging to this physical core, or NULL if it is not present.
     */
    VirtualCore* getVirtualCore() const;
};

class VirtualCoreIdleLevel{
private:
    const VirtualCoreId _virtualCoreId;
    const uint _levelId;
protected:
    VirtualCoreIdleLevel(VirtualCoreId virtualCoreId, uint levelId);
public:
    /**
     * Returns the virtual core identifier associated of this level.
     * @return The virtual core identifier associated of this level.
     */
    VirtualCoreId getVirtualCoreId() const;

    /**
     * Returns the identifier of this idle level.
     * @return The identifier of this idle level.
     */
    uint getLevelId() const;

    /**
     * Returns the name of this level.
     * @return The name of this level.
     */
    virtual std::string getName() const = 0;

    /**
     * Returns a small description about this level.
     * @return A small description about this level.
     */
    virtual std::string getDesc() const = 0;

    /**
     * Returns true if this level is enabled.
     * @return True if this level is enabled, false otherwise.
     */
    virtual bool isEnabled() const = 0;

    /**
     * Enables this level.
     */
    virtual void enable() const = 0;

    /**
     * Disables this level.
     */
    virtual void disable() const = 0;

    /**
     * Returns the latency to exit from this level (in microseconds).
     * @return The latency to exit from this level (in microseconds).
     */
    virtual uint getExitLatency() const = 0;

    /**
     * Returns the power consumed while in this level (in milliwatts).
     * @return The power consumed while in this level (in milliwatts).
     */
    virtual uint getConsumedPower() const = 0;

    /**
     * Returns the total time spent in this level (in microseconds).
     * @return The total time spent in this level (in microseconds).
     */
    virtual uint getTime() const = 0;

    /**
     * Returns the number of times this level was entered.
     * @return The number of times this level was entered.
     */
    virtual uint getCount() const = 0;



    virtual inline ~VirtualCoreIdleLevel(){;}
};

class VirtualCore{
private:
    const CpuId _cpuId;
    const PhysicalCoreId _physicalCoreId;
    const VirtualCoreId _virtualCoreId;
protected:
    VirtualCore(CpuId cpuId, PhysicalCoreId physicalCoreId, VirtualCoreId virtualCoreId);
public:

    /**
     * Returns the identifier of this virtual core.
     * @return The identifier of this virtual core.
     */
    VirtualCoreId getVirtualCoreId() const;

    /**
     * Returns the identifier of the physical core
     * on which this virtual core is running.
     * @return The identifier of the physical core
     * on which this virtual core is running.
     */
    PhysicalCoreId getPhysicalCoreId() const;

    /**
     * Returns the identifier of the CPU on
     * which this virtual core is running.
     * @return The identifier of the CPU on
     * which this virtual core is running.
     */
    CpuId getCpuId() const;

    /*****************************************************/
    /*                   HotPlug Support                 */
    /*****************************************************/

    /**
     * Returns true if this virtual core is hot-pluggable.
     * @return True if this virtual core is hot-pluggable,
     *         false otherwise.
     */
    virtual bool isHotPluggable() const = 0;

    /**
     * Returns true if this virtual core is hot plugged.
     * @return True if this virtual core is hot plugged,
     *         false otherwise.
     */
    virtual bool isHotPlugged() const = 0;

    /**
     * Hotplugs this virtual core.
     */
    virtual void hotPlug() const = 0;

    /**
     * Hotunplugs this virtual core.
     */
    virtual void hotUnplug() const = 0;

    /*****************************************************/
    /*                   CpuIdle Support                 */
    /*****************************************************/

    /**
     * Returns the idle levels (C-States) supported by this virtual core.
     * @return The idle levels supported by this virtual core. If the vector
     *         is empty, no idle levels are supported.
     */
    virtual std::vector<VirtualCoreIdleLevel*> getIdleLevels() const = 0;

    virtual inline ~VirtualCore(){;}
};

}
}

#endif /* TOPOLOGY_HPP_ */
