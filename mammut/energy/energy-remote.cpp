/*
 * energy-remote.cpp
 *
 * Created on: 03/12/2014
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

#include <mammut/energy/energy-remote.hpp>
#include <mammut/energy/energy-remote.pb.h>

namespace mammut{
namespace energy{

CounterCpuRemote::CounterCpuRemote(mammut::Communicator* const communicator, topology::Cpu* cpu,
                                   bool hasGraphic, bool hasDram):
    CounterCpu(cpu, hasGraphic, hasDram), _communicator(communicator){
        ;
}

void CounterCpuRemote::reset(){
    CounterCpuReset ccr;
    CounterCpuResetRes r;
    ccr.set_cpu_id(getCpu()->getId());
    _communicator->remoteCall(ccr, r);
}

Joules CounterCpuRemote::getJoules(CounterCpuType type){
    CounterCpuGetJoules ccgj;
    CounterCpuGetJoulesRes r;
    ccgj.set_cpu_id(getCpu()->getId());
    ccgj.set_type(type);
    _communicator->remoteCall(ccgj, r);
    return r.joules();
}

Joules CounterCpuRemote::getJoules(){
    return getJoules(COUNTER_CPU_TYPE_CPU);
}

Joules CounterCpuRemote::getJoulesCores(){
    return getJoules(COUNTER_CPU_TYPE_CORES);
}

Joules CounterCpuRemote::getJoulesGraphic(){
    if(hasJoulesGraphic()){
        return getJoules(COUNTER_CPU_TYPE_GRAPHIC);
    }else{
        return 0;
    }
}

Joules CounterCpuRemote::getJoulesDram(){
    if(hasJoulesDram()){
        return getJoules(COUNTER_CPU_TYPE_DRAM);
    }else{
        return 0;
    }
}

}
}
