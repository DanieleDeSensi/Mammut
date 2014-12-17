Introduction
================================================================================================================
Mammut (MAchine Micro Management UTilities) is a set of functions for the management of a local or a remote
machine. It is structured as a set of modules, each managing a specific functionality (e.g. CPU frequencies,
topology analysis, etc...). 
Currently, the following modules are present:

+ [Topology](./mammut/topology): Allows the navigation and the management of the CPU, Physical Cores and Virtual Cores
  of the machine.
+ [CPUFreq](./mammut/cpufreq): Allows to read and change the frequency and the governors of the Physical Cores.
+ [Energy](./mammut/energy): Allows to read the energy consumption of the CPUs.

The documentation for each module can be found on the corresponing folder.

To manage a remote machine, ```mammut-server``` must run on that machine. From the user perspective, the only 
difference between managing a local or a remote machine concerns the module initialization. For example,
to get a local instance of the topology module:

```C++
mammut::topology::Topology* t = mammut::topology::Topology::local();
```

and to get a remote instance:

```C++
mammut::CommunicatorTcp* communicator = new mammut::CommunicatorTcp(remoteMachineAddress, remoteMachinePort);
mammut::topology::Topology* t = mammut::topology::Topology::remote(communicator);
```

After that, all the other calls to the module are exactly the same in both cases.
When the module is no more needed, it must be released with:

```C++
mammut::topology::Topology::release(t);
```

Similar ```local()```, ```remote()```, and ```release()``` calls are available for the other modules.

Demo
----------------------------------------------------------------------------------------------------------------
Some demonstrative example can be found in folder [demo](./demo).

Installation
================================================================================================================
Mammut depends on [Google's Protocol Buffer](http://code.google.com/p/protobuf/) library. This library is
used for the communication with ```mammut-server``` when the management of a remote machine is required. 

After installing [Google's Protocol Buffer](http://code.google.com/p/protobuf/), fetch the framework typing:

```
$ git clone git://github.com/DanieleDeSensi/Mammut.git
$ cd ./Mammut/mammut
```

Compile it with:

```
$ make
```

After that, install it with

```
$ make install
```

Server usage
================================================================================================================
To manage a remote machine, ```mammut-server``` must run on that machine. It accepts the following parameters:

+ [Mandatory] --tcpport: TCP port used by the server to wait for remote requests.
+ [Optional] --verbose[level]: Prints information during the execution.
+ [Optional] --cpufreq: Activates cpufreq module.
+ [Optional] --topology: Activates topology module.
+ [Optional] --energy: Activates energy module.

If a module is not activated, any attempt of a client to use that module will fail.

