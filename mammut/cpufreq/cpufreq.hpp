/*
 * cpufreq.hpp
 *
 * Created on: 10/11/2014
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

#ifndef CPUFREQ_HPP_
#define CPUFREQ_HPP_

#include <mammut/communicator.hpp>
#include <mammut/module.hpp>
#include <mammut/topology/topology.hpp>

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace mammut{
namespace cpufreq{

typedef enum{
    GOVERNOR_CONSERVATIVE,
    GOVERNOR_ONDEMAND,
    GOVERNOR_USERSPACE,
    GOVERNOR_POWERSAVE,
    GOVERNOR_PERFORMANCE,
    GOVERNOR_NUM
}Governor;

typedef uint32_t Frequency;
typedef uint32_t DomainId;

/**
 * Represents a set of virtual cores related between each other.
 * When the frequency/governor changes for one core in the domain,
 * it changes for the other cores in the same domain too.
 */
class Domain{
    const DomainId _domainIdentifier;
    const std::vector<topology::VirtualCore*> _virtualCores;
protected:
    Domain(DomainId domainIdentifier, std::vector<topology::VirtualCore*> virtualCores);
public:
    virtual inline ~Domain(){;}

    /**
     * Returns the virtual cores inside the domain.
     * @return The virtual cores inside the domain.
     */
    std::vector<topology::VirtualCore*> getVirtualCores() const;

    /**
     * Returns the identifiers of the virtual cores inside the domain.
     * @return The identifiers of the virtual cores inside the domain.
     */
    std::vector<topology::VirtualCoreId> getVirtualCoresIdentifiers() const;

    /**
     * Returns the identifier of the domain.
     * @return The identifier of the domain.
     */
    DomainId getId() const;

    /**
     * Gets the frequency steps (in KHz) available (sorted from lowest to highest).
     * @return The frequency steps (in KHz) available (sorted from lowest to highest).
     **/
    virtual std::vector<Frequency> getAvailableFrequencies() const = 0;

    /**
     * Gets the governors available.
     * @return The governors available.
     **/
    virtual std::vector<Governor> getAvailableGovernors() const = 0;

    /**
     * Gets the current frequency.
     * @return The current frequency.
     */
    virtual Frequency getCurrentFrequency() const = 0;

    /**
     * Gets the current frequency set by userspace governor.
     * @return The current frequency if the current governor is userspace, a meaningless value otherwise.
     */
    virtual Frequency getCurrentFrequencyUserspace() const = 0;

    /**
     * Gets the current governor.
     * @return The current governor.
     */
    virtual Governor getCurrentGovernor() const = 0;

    /**
     * Change the running frequency.
     * @param frequency The frequency to be set (specified in kHZ).
     * @return true if the frequency is valid and the governor is userspace, false otherwise.
     **/
    virtual bool changeFrequency(Frequency frequency) const = 0;

    /**
     * Change the frequency bounds.
     * @param lowerBound The new frequency lower bound (specified in kHZ).
     * @param upperBound The new frequency lower bound (specified in kHZ).
     * @return true if the bounds are valid and the governor is not userspace, false otherwise.
     **/
    virtual bool changeFrequencyBounds(Frequency lowerBound, Frequency upperBound) const = 0;

    /**
     * Changes the governor.
     * @param governor The identifier of the governor.
     * @return true if the governor is valid, false otherwise.
     **/
    virtual bool changeGovernor(Governor governor) const = 0;
};

class CpuFreq: public Module{
    MAMMUT_MODULE_DECL(CpuFreq)
private:
    Communicator* const _communicator;
    topology::Topology* _topology;
    std::vector<Domain*> _domains;

    CpuFreq();
    CpuFreq(Communicator* const communicator);
    ~CpuFreq();
public:
    /**
     * Gets the domains division of the cores.
     * A domain is a set of cores such that when the frequency/governor for one core
     * in the domain is changed, the frequency/governor of all the other cores in the
     * same domain is changed too.
     * @return A vector of domains.
     */
    std::vector<Domain*> getDomains() const;

    /**
     * Returns the governor name associated to a specific identifier.
     * @param governor The identifier of the governor.
     * @return The governor name associated to governor.
     */
    static std::string getGovernorNameFromGovernor(Governor governor);

    /**
     * Returns the governor identifier starting from the name of the governor.
     * @param governorName The name of the governor.
     * @return The identifier associated to governorName, or GOVERNOR_NUM
     *         if no association is present.
     */
    static Governor getGovernorFromGovernorName(std::string governorName);
private:
    static std::vector<std::string> _governorsNames;
    static std::vector<std::string> initGovernorsNames();
};

}
}

#endif /* CPUFREQ_HPP_ */
