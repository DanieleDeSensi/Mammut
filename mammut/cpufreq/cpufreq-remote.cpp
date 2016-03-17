/*
 * cpufreq-remote.cpp
 *
 * Created on: 12/11/2014
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

#ifdef MAMMUT_REMOTE
#include "../cpufreq/cpufreq.hpp"
#include "../cpufreq/cpufreq-remote.hpp"
#include "../cpufreq/cpufreq-remote.pb.h"
#include "../utils.hpp"

namespace mammut{
namespace cpufreq{

DomainRemote::DomainRemote(Communicator* const communicator,
                           DomainId domainIdentifier,
                           std::vector<topology::VirtualCore*> virtualCores):
        Domain(domainIdentifier, virtualCores),
        _communicator(communicator){
    GetAvailableFrequencies gaf;
    GetAvailableFrequenciesRes r;
    gaf.set_id(getId());
    _communicator->remoteCall(gaf, r);
    utils::pbRepeatedToVector<uint32_t>(r.frequencies(), _availableFrequencies);
}

std::vector<Frequency> DomainRemote::getAvailableFrequencies() const{
    return _availableFrequencies;
}

std::vector<Governor> DomainRemote::getAvailableGovernors() const{
    GetAvailableGovernors gag;
    GetAvailableGovernorsRes r;
    std::vector<Governor> availableGovernors;

    gag.set_id(getId());
    _communicator->remoteCall(gag, r);
    std::vector<uint32_t> tmp;
    utils::convertVector<uint32_t, Governor>(utils::pbRepeatedToVector<uint32_t>(r.governors(), tmp),
                                             availableGovernors);
    return availableGovernors;
}

Frequency DomainRemote::getCurrentFrequency(bool userspace) const{
    GetCurrentFrequency gcf;
    GetCurrentFrequencyRes r;

    gcf.set_id(getId());
    gcf.set_userspace(userspace);
    _communicator->remoteCall(gcf, r);
    return r.frequency();
}

Frequency DomainRemote::getCurrentFrequency() const{
    return getCurrentFrequency(false);
}

Frequency DomainRemote::getCurrentFrequencyUserspace() const{
    return getCurrentFrequency(true);
}

Governor DomainRemote::getCurrentGovernor() const{
    GetCurrentGovernor gcg;
    GetCurrentGovernorRes r;

    gcg.set_id(getId());
    _communicator->remoteCall(gcg, r);
    return static_cast<Governor>(r.governor());
}

bool DomainRemote::setFrequencyUserspace(Frequency frequency) const{
    ChangeFrequency cf;
    Result r;
    cf.set_id(getId());
    cf.set_frequency(frequency);
    _communicator->remoteCall(cf, r);
    return r.result();
}

void DomainRemote::getHardwareFrequencyBounds(Frequency& lowerBound, Frequency& upperBound) const{
    GetHardwareFrequencyBounds ghfb;
    GetHardwareFrequencyBoundsRes r;
    ghfb.set_id(getId());
    _communicator->remoteCall(ghfb, r);
    lowerBound = r.lower_bound();
    upperBound = r.upper_bound();
}

bool DomainRemote::getCurrentGovernorBounds(Frequency& lowerBound, Frequency& upperBound) const{
    GetGovernorBounds ggb;
    GetGovernorBoundsRes r;
    ggb.set_id(getId());
    _communicator->remoteCall(ggb, r);
    lowerBound = r.lower_bound();
    upperBound = r.upper_bound();
    return r.result();
}

bool DomainRemote::setGovernorBounds(Frequency lowerBound, Frequency upperBound) const{
    ChangeFrequencyBounds cflb;
    Result r;
    cflb.set_id(getId());
    cflb.set_lower_bound(lowerBound);
    cflb.set_upper_bound(upperBound);
    _communicator->remoteCall(cflb, r);
    return r.result();
}

bool DomainRemote::setGovernor(Governor governor) const{
    ChangeGovernor cg;
    Result r;
    cg.set_id(getId());
    cg.set_governor(governor);
    _communicator->remoteCall(cg, r);
    return r.result();
}

int DomainRemote::getTransitionLatency() const{
    GetTransitionLatency gtl;
    ResultInt r;
    gtl.set_id(getId());
    _communicator->remoteCall(gtl, r);
    return r.result();
}

Voltage DomainRemote::getCurrentVoltage() const{
    GetCurrentVoltage gcv;
    ResultDouble r;
    gcv.set_id(getId());
    _communicator->remoteCall(gcv, r);
    return r.result();
}


CpuFreqRemote::CpuFreqRemote(Communicator* const communicator):_communicator(communicator){
    GetDomains gd;
    GetDomainsRes r;
    _communicator->remoteCall(gd, r);

    topology::Topology* topology = topology::Topology::remote(_communicator);
    std::vector<topology::VirtualCore*> vc = topology->getVirtualCores();
    _domains.resize(r.domains_size());
    for(unsigned int i = 0; i < (size_t) r.domains_size(); i++){
        std::vector<topology::VirtualCoreId> virtualCoresIdentifiers;
        GetDomainsRes::Domain d = r.domains().Get(i);
        utils::pbRepeatedToVector<topology::VirtualCoreId>(d.virtual_cores_ids(), virtualCoresIdentifiers);
        _domains.at(d.id()) = new DomainRemote(_communicator, d.id(), filterVirtualCores(vc, virtualCoresIdentifiers));
    }
    topology::Topology::release(topology);
}

CpuFreqRemote::~CpuFreqRemote(){
    utils::deleteVectorElements<Domain*>(_domains);
}

std::vector<Domain*> CpuFreqRemote::getDomains() const{
    return _domains;
}

bool CpuFreqRemote::isBoostingSupported() const{
    IsBoostingSupported ibs;
    Result r;
    _communicator->remoteCall(ibs, r);
    return r.result();
}

bool CpuFreqRemote::isBoostingEnabled() const{
    IsBoostingEnabled ibe;
    Result r;
    _communicator->remoteCall(ibe, r);
    return r.result();
}

void CpuFreqRemote::enableBoosting() const{
    EnableBoosting eb;
    ResultVoid r;
    _communicator->remoteCall(eb, r);
}

void CpuFreqRemote::disableBoosting() const{
    DisableBoosting db;
    ResultVoid r;
    _communicator->remoteCall(db, r);
}

}
}
#endif
