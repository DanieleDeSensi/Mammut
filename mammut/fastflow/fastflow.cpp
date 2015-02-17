/*
 * fastflow.cpp
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

#include "mammut/utils.hpp"
#include "mammut/external/rapidXml/rapidxml.hpp"
#include "mammut/fastflow/fastflow.hpp"

#include <fstream>
#include <streambuf>
#include <string>

namespace mammut{
namespace fastflow{

AdaptiveNode::AdaptiveNode():
  _tasksManager(NULL),
  _thread(NULL),
  _threadFirstCreation(false),
  _threadRunning(false),
  _tasksCount(0),
  _workTicks(0),
  _startTicks(getticks()),
  _managementRequest(MANAGEMENT_REQUEST_GET_AND_RESET_SAMPLE),
  _managementQ(1),
  _responseQ(1){
    _managementQ.init();
    _responseQ.init();
}

AdaptiveNode::~AdaptiveNode(){
    if(_thread){
        _tasksManager->releaseThreadHandler(_thread);
    }

    if(_tasksManager){
        task::TasksManager::release(_tasksManager);
    }
}

void AdaptiveNode::waitThreadCreation(){
    _threadCreated.wait();
}

task::ThreadHandler* AdaptiveNode::getThreadHandler() const{
    if(_thread){
        return _thread;
    }else{
        throw std::runtime_error("AdaptiveNode: Thread not initialized.");
    }
}

void AdaptiveNode::initMammutModules(Communicator* const communicator){
    if(communicator){
        _tasksManager = task::TasksManager::remote(communicator);
    }else{
        _tasksManager = task::TasksManager::local();
    }
}

bool AdaptiveNode::getAndResetSample(NodeSample& sample){
    _managementRequest = MANAGEMENT_REQUEST_GET_AND_RESET_SAMPLE;
    _managementQ.push(&_managementRequest); // The value pushed in the queue will not be read, it could be anything.
    int dummy;
    int* dummyPtr = &dummy;
    while(!_responseQ.pop((void**) &dummyPtr)){
        _threadRunningLock.lock();
        if(!_threadRunning){
            _threadRunningLock.unlock();
            return false;
        }
        _threadRunningLock.unlock();
    }
    sample = _sampleResponse;
    return true;
}

void AdaptiveNode::produceNull(){
    _managementRequest = MANAGEMENT_REQUEST_PRODUCE_NULL;
    _managementQ.push(&_managementRequest); // The value pushed in the queue will not be read, it could be anything.
}

int AdaptiveNode::adp_svc_init(){return 0;}

void AdaptiveNode::adp_svc_end(){;}

int AdaptiveNode::svc_init() CX11_KEYWORD(final){
    _threadRunningLock.lock();
    _threadRunning = true;
    _threadRunningLock.unlock();
    if(!_threadFirstCreation){
        /** Operations performed only the first time the thread is running. **/
        if(_tasksManager){
            _thread = _tasksManager->getThreadHandler();
        }else{
            throw std::runtime_error("AdaptiveWorker: Tasks manager not initialized.");
        }
        _threadCreated.notifyAll();
        _threadFirstCreation = true;
    }
    std::cout << "My svc_init called." << std::endl;
    return adp_svc_init();
}

void* AdaptiveNode::svc(void* task) CX11_KEYWORD(final){
    ticks start = getticks();
    void* t = adp_svc(task);
    ++_tasksCount;
    _workTicks += getticks() - start;

    int dummy;
    int* dummyPtr = &dummy;
    if(_managementQ.pop((void**)&dummyPtr)){
        switch(_managementRequest){
            case MANAGEMENT_REQUEST_GET_AND_RESET_SAMPLE:{
                ticks now = getticks();
                _sampleResponse.loadPercentage = ((double) _workTicks / (double)(now - _startTicks)) * 100.0;
                _sampleResponse.tasksCount = _tasksCount;
                _workTicks = 0;
                _startTicks = now;
                _tasksCount = 0;
                _responseQ.push(dummyPtr);
            }break;
            case MANAGEMENT_REQUEST_PRODUCE_NULL:{
                return NULL;
            }
        }
    }
    return t;
}

void AdaptiveNode::svc_end() CX11_KEYWORD(final){
    _threadRunningLock.lock();
    _threadRunning = false;
    _threadRunningLock.unlock();
    adp_svc_end();
}

void AdaptivityParameters::setDefault(){
    strategyMapping = STRATEGY_MAPPING_LINEAR;
    strategyFrequencies = STRATEGY_FREQUENCY_NO;
    frequencyGovernor = cpufreq::GOVERNOR_USERSPACE;
    turboBoost = false;
    frequencyLowerBound = 0;
    frequencyUpperBound = 0;
    fastReconfiguration = false;
    strategyUnusedVirtualCores = STRATEGY_UNUSED_VC_NONE;
    strategyInactiveVirtualCores = STRATEGY_UNUSED_VC_NONE;
    sensitiveEmitter = false;
    sensitiveCollector = false;
    numSamples = 10;
    samplesToDiscard = 1;
    samplingInterval = 1;
    underloadThresholdFarm = 80.0;
    overloadThresholdFarm = 90.0;
    underloadThresholdWorker = 80.0;
    overloadThresholdWorker = 90.0;
    migrateCollector = false;
    requiredBandwidth = 0;
    maxBandwidthVariation = 5.0;
    voltageTableFile = "";
    observer = NULL;
    if(communicator){
        cpufreq = cpufreq::CpuFreq::remote(this->communicator);
        energy = energy::Energy::remote(this->communicator);
        topology = topology::Topology::remote(this->communicator);
    }else{
        cpufreq = cpufreq::CpuFreq::local();
        energy = energy::Energy::local();
        topology = topology::Topology::local();
    }
}

AdaptivityParameters::AdaptivityParameters(Communicator* const communicator):
        communicator(communicator){
    setDefault();
}

AdaptivityParameters::AdaptivityParameters(const std::string& xmlFileName, Communicator* const communicator):
        communicator(communicator){
    setDefault();
    rapidxml::xml_document<> xmlContent;
    std::ifstream file(xmlFileName.c_str());
    if(!file.is_open()){
        throw std::runtime_error("Impossible to read xml file " + xmlFileName);
    }

    std::string fileContent;
    file.seekg(0, std::ios::end);
    fileContent.reserve(file.tellg());
    file.seekg(0, std::ios::beg);
    fileContent.assign((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    char* fileContentChars = new char[fileContent.size() + 1];
    std::copy(fileContent.begin(), fileContent.end(), fileContentChars);
    fileContentChars[fileContent.size()] = '\0';
    xmlContent.parse<0>(fileContentChars);
    rapidxml::xml_node<> *root = xmlContent.first_node("adaptivityParameters");
    rapidxml::xml_node<> *node = NULL;

    node = root->first_node("strategyMapping");
    if(node){
        strategyMapping = (StrategyMapping) utils::stringToInt(node->value());
    }

    node = root->first_node("strategyFrequencies");
    if(node){
        strategyFrequencies = (StrategyFrequencies) utils::stringToInt(node->value());
    }

    node = root->first_node("frequencyGovernor");
    if(node){
        frequencyGovernor = (cpufreq::Governor) utils::stringToInt(node->value());
    }

    node = root->first_node("turboBoost");
    if(node){
        turboBoost = utils::stringToInt(node->value());
    }

    node = root->first_node("frequencyLowerBound");
    if(node){
        frequencyLowerBound = utils::stringToInt(node->value());
    }

    node = root->first_node("frequencyUpperBound");
    if(node){
        frequencyUpperBound = utils::stringToInt(node->value());
    }

    node = root->first_node("fastReconfiguration");
    if(node){
        fastReconfiguration = utils::stringToInt(node->value());
    }

    node = root->first_node("strategyUnusedVirtualCores");
    if(node){
        strategyUnusedVirtualCores = (StrategyUnusedVirtualCores) utils::stringToInt(node->value());
    }

    node = root->first_node("strategyInactiveVirtualCores");
    if(node){
        strategyInactiveVirtualCores = (StrategyUnusedVirtualCores) utils::stringToInt(node->value());
    }

    node = root->first_node("sensitiveEmitter");
    if(node){
        sensitiveEmitter = utils::stringToInt(node->value());
    }

    node = root->first_node("sensitiveCollector");
    if(node){
        sensitiveCollector = utils::stringToInt(node->value());
    }

    node = root->first_node("numSamples");
    if(node){
        numSamples = utils::stringToInt(node->value());
    }

    node = root->first_node("samplesToDiscard");
    if(node){
        samplesToDiscard = utils::stringToInt(node->value());
    }

    node = root->first_node("samplingInterval");
    if(node){
        samplingInterval = utils::stringToInt(node->value());
    }

    node = root->first_node("underloadThresholdFarm");
    if(node){
        underloadThresholdFarm = utils::stringToDouble(node->value());
    }

    node = root->first_node("overloadThresholdFarm");
    if(node){
        overloadThresholdFarm = utils::stringToDouble(node->value());
    }

    node = root->first_node("underloadThresholdWorker");
    if(node){
        underloadThresholdWorker = utils::stringToDouble(node->value());
    }

    node = root->first_node("overloadThresholdWorker");
    if(node){
        overloadThresholdWorker = utils::stringToDouble(node->value());
    }

    node = root->first_node("migrateCollector");
    if(node){
        migrateCollector = utils::stringToInt(node->value());
    }

    node = root->first_node("requiredBandwidth");
    if(node){
        requiredBandwidth = utils::stringToDouble(node->value());
    }

    node = root->first_node("maxBandwidthVariation");
    if(node){
        maxBandwidthVariation = utils::stringToDouble(node->value());
    }

    node = root->first_node("voltageTableFile");
    if(node){
        voltageTableFile = node->value();
    }

    delete[] fileContentChars;
}

AdaptivityParameters::~AdaptivityParameters(){
    cpufreq::CpuFreq::release(cpufreq);
    energy::Energy::release(energy);
    topology::Topology::release(topology);
}

AdaptivityParametersValidation AdaptivityParameters::validate(){
    std::vector<cpufreq::Domain*> frequencyDomains = cpufreq->getDomains();
    std::vector<topology::VirtualCore*> virtualCores = topology->getVirtualCores();
    std::vector<cpufreq::Frequency> availableFrequencies;
    if(frequencyDomains.size()){
        availableFrequencies = frequencyDomains.at(0)->getAvailableFrequencies();
    }

    if(strategyFrequencies != STRATEGY_FREQUENCY_NO && strategyMapping == STRATEGY_MAPPING_NO){
        return VALIDATION_STRATEGY_FREQUENCY_REQUIRES_MAPPING;
    }

    /** Validate thresholds. **/
    if((underloadThresholdFarm > overloadThresholdFarm) ||
       (underloadThresholdWorker > overloadThresholdWorker) ||
       underloadThresholdFarm < 0 || overloadThresholdFarm > 100 ||
       underloadThresholdWorker < 0 || overloadThresholdWorker > 100){
        return VALIDATION_THRESHOLDS_INVALID;
    }

    /** Validate frequency strategies. **/
    if(strategyFrequencies != STRATEGY_FREQUENCY_NO){
        if(!frequencyDomains.size()){
            return VALIDATION_STRATEGY_FREQUENCY_UNSUPPORTED;
        }

        if(strategyFrequencies != STRATEGY_FREQUENCY_OS){
            frequencyGovernor = cpufreq::GOVERNOR_USERSPACE;
            if(!cpufreq->isGovernorAvailable(frequencyGovernor)){
                return VALIDATION_STRATEGY_FREQUENCY_UNSUPPORTED;
            }
        }
        if((sensitiveEmitter || sensitiveCollector) &&
           !cpufreq->isGovernorAvailable(cpufreq::GOVERNOR_PERFORMANCE) &&
           !cpufreq->isGovernorAvailable(cpufreq::GOVERNOR_USERSPACE)){
            return VALIDATION_EC_SENSITIVE_MISSING_GOVERNORS;
        }

    }else{
        if(sensitiveEmitter || sensitiveCollector){
            return VALIDATION_EC_SENSITIVE_WRONG_F_STRATEGY;
        }
    }

    /** Validate governor availability. **/
    if(!cpufreq->isGovernorAvailable(frequencyGovernor)){
        return VALIDATION_GOVERNOR_UNSUPPORTED;
    }

    /** Validate mapping strategy. **/
    if(strategyMapping == STRATEGY_MAPPING_CACHE_EFFICIENT){
        return VALIDATION_STRATEGY_MAPPING_UNSUPPORTED;
    }

    /** Validate frequency bounds. **/
    if(frequencyLowerBound || frequencyUpperBound){
        if(strategyFrequencies == STRATEGY_FREQUENCY_OS){
            if(!availableFrequencies.size()){
                return VALIDATION_INVALID_FREQUENCY_BOUNDS;
            }

            if(frequencyLowerBound){
                if(!utils::contains(availableFrequencies, frequencyLowerBound)){
                    return VALIDATION_INVALID_FREQUENCY_BOUNDS;
                }
            }else{
                frequencyLowerBound = availableFrequencies.at(0);
            }

            if(frequencyUpperBound){
                if(!utils::contains(availableFrequencies, frequencyUpperBound)){
                    return VALIDATION_INVALID_FREQUENCY_BOUNDS;
                }
            }else{
                frequencyUpperBound = availableFrequencies.back();
            }
        }else{
            return VALIDATION_INVALID_FREQUENCY_BOUNDS;
        }
    }

    /** Validate unused cores strategy. **/
    switch(strategyInactiveVirtualCores){
        case STRATEGY_UNUSED_VC_OFF:{
            bool hotPluggableFound = false;
            for(size_t i = 0; i < virtualCores.size(); i++){
                if(virtualCores.at(i)->isHotPluggable()){
                    hotPluggableFound = true;
                }
            }
            if(!hotPluggableFound){
                return VALIDATION_UNUSED_VC_NO_OFF;
            }
        }break;
        case STRATEGY_UNUSED_VC_LOWEST_FREQUENCY:{
            if(!cpufreq->isGovernorAvailable(cpufreq::GOVERNOR_POWERSAVE) &&
               !cpufreq->isGovernorAvailable(cpufreq::GOVERNOR_USERSPACE)){
                return VALIDATION_UNUSED_VC_NO_FREQUENCIES;
            }
        }break;
        default:
            break;
    }

    /** Validate required bandwidth. **/
    if(requiredBandwidth < 0 || maxBandwidthVariation < 0 || maxBandwidthVariation > 100.0){
        return VALIDATION_WRONG_BANDWIDTH_PARAMETERS;
    }

    /** Validate voltage table. **/
    if(strategyFrequencies == STRATEGY_FREQUENCY_POWER_CONSERVATIVE){
        if(voltageTableFile.empty() || !utils::existsFile(voltageTableFile)){
            return VALIDATION_VOLTAGE_FILE_NEEDED;
        }
    }

    /** Validate fast reconfiguration. **/
    if(fastReconfiguration){
        if(!cpufreq->isGovernorAvailable(cpufreq::GOVERNOR_PERFORMANCE) &&
           (!cpufreq->isGovernorAvailable(cpufreq::GOVERNOR_USERSPACE) ||
            !availableFrequencies.size())){
            return VALIDATION_NO_FAST_RECONF;
        }
    }

    return VALIDATION_OK;
}

}
}
