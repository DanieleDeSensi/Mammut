/*
 * demo-topology.cpp
 * This is a minimal demo on topology reading.
 *
 * Created on: 07/01/2015
 *
 * =========================================================================
 *  Copyright (C) 2015-, Daniele De Sensi (d.desensi.software@gmail.com)
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

#include <mammut/communicator-tcp.hpp>
#include <mammut/topology/topology.hpp>

#include <iostream>

int main(int argc, char** argv){
    mammut::CommunicatorTcp* communicator = NULL;
    std::cout << "Usage: " << argv[0] << " [TcpAddress:TcpPort]" << std::endl;

    /** Gets the address and the port of the server and builds the communicator. **/
    if(argc > 1){
        std::string addressPort = argv[1];
        size_t pos = addressPort.find_first_of(":");
        std::string address = addressPort.substr(0, pos);
        uint16_t port = atoi(addressPort.substr(pos + 1).c_str());
        communicator = new mammut::CommunicatorTcp(address, port);
    }

    mammut::topology::Topology* topology;

    /** A local or a remote handler is built. **/
    if(communicator){
        topology = mammut::topology::Topology::remote(communicator);
    }else{
        topology = mammut::topology::Topology::local();
    }


    /*******************************************/
    /*               Topology test             */
    /*******************************************/
    std::vector<mammut::topology::Cpu*> cpus = topology->getCpus();
    std::cout << "The machine has [" << cpus.size() << " CPUs]" << std::endl;
    for(size_t i = 0; i < cpus.size(); i++){
        mammut::topology::Cpu* cpu = cpus.at(i);
        std::vector<mammut::topology::PhysicalCore*> physical = cpu->getPhysicalCores();
        std::cout << "CPU " << cpu->getCpuId() << " has " << physical.size() << " physical cores: [";
        for(size_t j = 0; j < physical.size(); j++){
            mammut::topology::PhysicalCore* pc = physical.at(j);
            std::cout << pc->getPhysicalCoreId();
            if(j < physical.size() - 1){
                std::cout << ", ";
            }

        }
        std::cout << "]" << std::endl;
    }


    std::vector<mammut::topology::PhysicalCore*> physical = topology->getPhysicalCores();
    mammut::topology::VirtualCore* pluggable = NULL;

    for(size_t j = 0; j < physical.size(); j++){
        mammut::topology::PhysicalCore* pc = physical.at(j);
        std::cout << "Physical <" << pc->getCpuId() << ", " << pc->getPhysicalCoreId() << ">: ";
        std::vector<mammut::topology::VirtualCore*> virt = pc->getVirtualCores();
        std::cout << "[";
        for(size_t k = 0; k < virt.size(); k++){
            mammut::topology::VirtualCore* vc = virt.at(k);
            std::cout << vc->getVirtualCoreId();
            if(!vc->isHotPluggable()){
                std::cout << " (hotplug not supported)";
            }else{
                pluggable = vc;
                if(vc->isHotPlugged()){
                    std::cout << " (plugged)";
                }else{
                    std::cout << " (unplugged)";
                }
            }

            if(k < virt.size() - 1){
                std::cout << ", ";
            }
        }
        std::cout << "]" << std::endl;
    }

    /*******************************************/
    /*              HotPlug test               */
    /*******************************************/

    if(pluggable){
        std::cout << "Virtual " << pluggable->getVirtualCoreId() << " is hot pluggable. "
                     "Plugged: " << pluggable->isHotPlugged() << std::endl;;
        std::cout << "Unplugging.." << std::endl;
        pluggable->hotUnplug();
        assert(!pluggable->isHotPlugged());
        std::cout << "Plugging.." << std::endl;
        pluggable->hotPlug();
        assert(pluggable->isHotPlugged());
        std::cout << "Plugging test successful" << std::endl;
    }

    /*******************************************/
    /*              Idle states test           */
    /*******************************************/
    mammut::topology::VirtualCore* vc = topology->getVirtualCore(0);
    std::vector<mammut::topology::VirtualCoreIdleLevel*> idleLevels = vc->getIdleLevels();
    if(idleLevels.size() == 0){
        std::cout << "No idle levels supported by virtual core " << vc->getVirtualCoreId() << "." << std::endl;
    }else{
        std::cout << "The following idle levels are supported by virtual core " << vc->getVirtualCoreId() << ":" << std::endl;
        for(size_t i = 0; i < idleLevels.size(); i++){
            mammut::topology::VirtualCoreIdleLevel* level = idleLevels.at(i);
            std::cout << "[Idle Level: " << level->getLevelId() << "]";
            std::cout << "[Name: " << level->getName() << "]";
            std::cout << "[Desc: " << level->getDesc() << "]";
            std::cout << "[Consumed Power: " << level->getConsumedPower() << "]";
            std::cout << "[Exit latency: " << level->getExitLatency() << "]";
            std::cout << "[Time: " << level->getTime() << "]";
            std::cout << "[Count: " << level->getCount() << "]";
            std::cout << "[Enabled: " << level->isEnabled() << "]";
            std::cout << std::endl;

            bool originallyEnabled = level->isEnabled();
            std::cout << "Try to disable and enable again the state..." << std::endl;
            level->disable();
            assert(!level->isEnabled());
            level->enable();
            assert(level->isEnabled());
            std::cout << "Test successful" << std::endl;
            if(!originallyEnabled){
                level->disable();
                assert(!level->isEnabled());
            }
        }
    }

    mammut::topology::Topology::release(topology);
}