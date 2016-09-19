#ifdef MAMMUT_REMOTE
#include "../energy/energy-remote.hpp"
#include "../energy/energy-remote.pb.h"

namespace mammut{
namespace energy{

CounterPlugRemote::CounterPlugRemote(mammut::Communicator* const communicator):
        _communicator(communicator){
    reset();
}

bool CounterPlugRemote::init(){
    CounterReq cr;
    CounterResBool cri;
    cr.set_type(COUNTER_TYPE_PB_PLUG);
    cr.set_cmd(COUNTER_COMMAND_INIT);
    _communicator->remoteCall(cr, cri);
    return cri.res();
}

Joules CounterPlugRemote::getJoules(){
    CounterReq cr;
    CounterResGetGeneric crgg;
    cr.set_type(COUNTER_TYPE_PB_PLUG);
    cr.set_cmd(COUNTER_COMMAND_GET);
    _communicator->remoteCall(cr, crgg);
    return crgg.joules();
}

void CounterPlugRemote::reset(){
    CounterReq cr;
    CounterResBool cri;
    cr.set_type(COUNTER_TYPE_PB_PLUG);
    cr.set_cmd(COUNTER_COMMAND_RESET);
    _communicator->remoteCall(cr, cri);
}

CounterCpusRemote::CounterCpusRemote(mammut::Communicator* const communicator):
        CounterCpus(topology::Topology::getInstance(communicator)),
        _communicator(communicator){
    CounterReq cr;
    CounterResBool crb;

    cr.set_type(COUNTER_TYPE_PB_CPUS);
    cr.set_cmd(COUNTER_COMMAND_HAS);
    cr.set_subtype(COUNTER_VALUE_TYPE_CORES);
    _communicator->remoteCall(cr, crb);
    _hasCores = crb.res();

    cr.set_type(COUNTER_TYPE_PB_CPUS);
    cr.set_cmd(COUNTER_COMMAND_HAS);
    cr.set_subtype(COUNTER_VALUE_TYPE_GRAPHIC);
    _communicator->remoteCall(cr, crb);
    _hasGraphic = crb.res();

    cr.set_type(COUNTER_TYPE_PB_CPUS);
    cr.set_cmd(COUNTER_COMMAND_HAS);
    cr.set_subtype(COUNTER_VALUE_TYPE_DRAM);
    _communicator->remoteCall(cr, crb);
    _hasDram = crb.res();
}

JoulesCpu CounterCpusRemote::getJoulesComponents(){
    CounterReq cr;
    CounterResGetCpu crgc;
    cr.set_type(COUNTER_TYPE_PB_CPUS);
    cr.set_cmd(COUNTER_COMMAND_GET);
    _communicator->remoteCall(cr, crgc);
    JoulesCpu jc;
    for(int i = 0; i < crgc.joules_size(); i++){
        jc.cpu += crgc.joules(i).cpu();
        jc.cores += crgc.joules(i).cores();
        jc.graphic += crgc.joules(i).graphic();
        jc.dram += crgc.joules(i).dram();
    }
    return jc;
}

JoulesCpu CounterCpusRemote::getJoulesComponents(topology::CpuId cpuId){
    CounterReq cr;
    CounterResGetCpu crgc;
    cr.set_type(COUNTER_TYPE_PB_CPUS);
    cr.set_cmd(COUNTER_COMMAND_GET);
    _communicator->remoteCall(cr, crgc);
    JoulesCpu jc;
    for(int i = 0; i < crgc.joules_size(); i++){
        if(crgc.joules(i).cpuid() == cpuId){
            jc.cpu = crgc.joules(i).cpu();
            jc.cores = crgc.joules(i).cores();
            jc.graphic = crgc.joules(i).graphic();
            jc.dram = crgc.joules(i).dram();
        }
    }
    return jc;
}

Joules CounterCpusRemote::getJoulesCpu(){
    return getJoulesComponents().cpu;
}

Joules CounterCpusRemote::getJoulesCpu(topology::CpuId cpuId){
    return getJoulesComponents(cpuId).cpu;
}

Joules CounterCpusRemote::getJoulesCores(){
    return getJoulesComponents().cores;
}

Joules CounterCpusRemote::getJoulesCores(topology::CpuId cpuId){
    return getJoulesComponents(cpuId).cores;
}

Joules CounterCpusRemote::getJoulesGraphic(){
    return getJoulesComponents().graphic;
}

Joules CounterCpusRemote::getJoulesGraphic(topology::CpuId cpuId){
    return getJoulesComponents(cpuId).graphic;
}

Joules CounterCpusRemote::getJoulesDram(){
    return getJoulesComponents().dram;
}

Joules CounterCpusRemote::getJoulesDram(topology::CpuId cpuId){
    return getJoulesComponents(cpuId).dram;
}

bool CounterCpusRemote::hasJoulesCores(){
	return _hasCores;
}

bool CounterCpusRemote::hasJoulesDram(){
    return _hasDram;
}

bool CounterCpusRemote::hasJoulesGraphic(){
    return _hasGraphic;
}

bool CounterCpusRemote::init(){
    CounterReq cr;
    CounterResBool cri;
    cr.set_type(COUNTER_TYPE_PB_CPUS);
    cr.set_cmd(COUNTER_COMMAND_INIT);
    _communicator->remoteCall(cr, cri);
    return cri.res();
}

void CounterCpusRemote::reset(){
    CounterReq cr;
    CounterResBool cri;
    cr.set_type(COUNTER_TYPE_PB_CPUS);
    cr.set_cmd(COUNTER_COMMAND_RESET);
    _communicator->remoteCall(cr, cri);
}

}
}
#endif
