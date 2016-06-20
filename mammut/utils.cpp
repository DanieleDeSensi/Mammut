#include "./utils.hpp"
#include "algorithm"
#include "cctype"

#include "task/task.hpp"
#if defined (__linux__)
#include "dirent.h"
#endif
#include "errno.h"
#include "fcntl.h"
#include "fstream"
#include "functional"
#include "locale"
#include "stdexcept"
#include "sstream"
#include "stdio.h"
#include "string.h"
#include "syscall.h"
#include "unistd.h"
#include "sys/syscall.h"
#include "sys/stat.h"
#include "sys/time.h"
#include "sys/types.h"

namespace mammut{
namespace utils{

using namespace std;

Lock::~Lock(){
    ;
}

LockPthreadMutex::LockPthreadMutex(){
    if(pthread_mutex_init(&_lock, NULL) != 0){
        throw runtime_error("LockPthreadMutex: Couldn't initialize mutex");
    }
}

LockPthreadMutex::~LockPthreadMutex(){
    if(pthread_mutex_destroy(&_lock) != 0){
        throw runtime_error("LockPthreadMutex: Couldn't destroy mutex");
    }
}

void LockPthreadMutex::lock(){
    if(pthread_mutex_lock(&_lock) != 0){
        throw runtime_error("Error while locking.");
    }
}

void LockPthreadMutex::unlock(){
    if(pthread_mutex_unlock(&_lock) != 0){
        throw runtime_error("Error while unlocking.");
    }
}

pthread_mutex_t* LockPthreadMutex::getLock(){
    return &_lock;
}

LockEmpty::LockEmpty(){
    ;
}

LockEmpty::~LockEmpty(){
    ;
}

void LockEmpty::lock(){
    ;
}

void LockEmpty::unlock(){
    ;
}

ScopedLock::ScopedLock(Lock& lock):_lock(lock){
    _lock.lock();
}

ScopedLock::~ScopedLock(){
    _lock.unlock();
}

Thread::Thread():_thread(0), _running(false), _threadHandler(NULL),
                 _pm(mammut::task::TasksManager::local()){
    ;
}

Thread::~Thread(){
    mammut::task::TasksManager::release(_pm);
}

void* Thread::threadDispatcher(void* arg){
    Thread* t = static_cast<Thread*>(arg);
    t->_threadHandler = t->_pm->getThreadHandler(getpid(), gettid());
    t->_pidSet.notifyAll();

    t->_running = true;
    t->run();
    t->_running = false;
    return NULL;
}

void Thread::start(){
    if(!_running){
        int rc = pthread_create(&_thread, NULL, threadDispatcher, this);
        if(rc != 0){
            throw runtime_error("Thread: pthread_create failed. "
                                     "Error code: " + utils::intToString(rc));
        }
        _pidSet.wait();
    }else{
        throw runtime_error("Thread: Multiple start. It must be joined "
                                 "before starting it again.");
    }
}

mammut::task::ThreadHandler* Thread::getThreadHandler() const{
    return _threadHandler;
}

bool Thread::running(){
    return _running;
}

void Thread::join(){
    int rc = pthread_join(_thread, NULL);
    if(rc != 0){
        throw runtime_error("Thread: join failed. Error code: " +
                                 utils::intToString(rc));
    }
    _pm->releaseThreadHandler(_threadHandler);
    _threadHandler = NULL;
}

Monitor::Monitor():_mutex(){
    int rc = pthread_cond_init(&_condition, NULL);
    if(rc != 0){
        throw runtime_error("Monitor: couldn't initialize condition. "
                                 "Error code: " + utils::intToString(rc));
    }
    _predicate = false;
}

bool Monitor::predicate(){
    bool r;
    _mutex.lock();
    r = _predicate;
    _mutex.unlock();
    return r;

}

void Monitor::wait(){
    int rc = 0;
    _mutex.lock();
    while(!_predicate){
        rc = pthread_cond_wait(&_condition, _mutex.getLock());
    }
    _predicate = false;
    _mutex.unlock();
    if(rc != 0){
        throw runtime_error("Monitor: error in pthread_cond_wait. Error code: " + utils::intToString(rc));
    }
}

bool Monitor::timedWait(int milliSeconds){
    struct timeval now;
    gettimeofday(&now, NULL);

    struct timespec delay;
    delay.tv_sec  = now.tv_sec + milliSeconds / MAMMUT_MILLISECS_IN_SEC;
    delay.tv_nsec = now.tv_usec*1000 + (milliSeconds % MAMMUT_MILLISECS_IN_SEC) * MAMMUT_NANOSECS_IN_MSEC;
    if(delay.tv_nsec >= MAMMUT_NANOSECS_IN_SEC){
        delay.tv_nsec -= MAMMUT_NANOSECS_IN_SEC;
        delay.tv_sec++;
    }
    int rc = 0;
    _mutex.lock();
    while(!_predicate && rc != ETIMEDOUT){
        rc = pthread_cond_timedwait(&_condition, _mutex.getLock(), &delay);
    }
    _predicate = false;
    _mutex.unlock();
    if(rc == 0){
        return true;
    }else if(rc == ETIMEDOUT){
        return false;
    }else{
        throw runtime_error("Error: " + utils::intToString(rc) + " in pthread_cond_timedwait.");
    }
}

void Monitor::notifyOne(){
    _mutex.lock();
    _predicate = true;
    pthread_cond_signal(&_condition);
    _mutex.unlock();
}

void Monitor::notifyAll(){
    _mutex.lock();
    _predicate = true;
    pthread_cond_broadcast(&_condition);
    _mutex.unlock();
}

string getModuleNameFromMessageId(const string& messageId){
    vector<string> tokens = split(messageId, '.');
    if(tokens.size() != 3 || tokens.at(0).compare("mammut")){
        throw runtime_error("Wrong message id: " + messageId);
    }
    return tokens.at(1);
}

#ifdef MAMMUT_REMOTE
string getModuleNameFromMessage(::google::protobuf::MessageLite* const message){
    return getModuleNameFromMessageId(message->GetTypeName());
}

bool setMessageFromData(const ::google::protobuf::MessageLite* outData, string& messageIdOut, string& messageOut){
    messageIdOut = outData->GetTypeName();
    string tmp;
    if(!outData->SerializeToString(&tmp)){
        return false;
    }
    messageOut = tmp;
    return true;
}
#endif

#if defined (__linux__)
bool existsDirectory(const string& dirName){
    struct stat sb;
    return (stat(dirName.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode));
}
#endif

bool existsFile(const string& fileName){
    ifstream f(fileName.c_str());
    return f.good();
}

#if (__linux__)
int executeCommand(const string& command, bool waitResult){
    int status = system((command + " > /dev/null 2>&1" + (!waitResult?" &":"")).c_str());
    if(status == -1){
        throw runtime_error("Impossible to execute command " + command);
    }
    return WEXITSTATUS(status);
}
#else
int executeCommand(const string& command, bool waitResult){
    throw runtime_error("executeCommand not supported on this OS.");
}
#endif

vector<string> getCommandOutput(const string& command){
    vector<string> r;
    FILE *commandFile = popen(command.c_str(), "r");
    char *line = NULL;
    size_t len = 0;

    if(commandFile == NULL){
        throw runtime_error("Impossible to execute command " + command);
    }

    while(getline(&line, &len, commandFile) != -1){
        r.push_back(line);
    }
    free(line);
    pclose(commandFile);
    return r;
}

int stringToInt(const string& s){
    return atoi(s.c_str());
}

uint stringToUint(const string& s){
    return atoll(s.c_str());
}

ulong stringToUlong(const string& s){
    return atoll(s.c_str());
}


double stringToDouble(const string& s){
    return atof(s.c_str());
}

string readFirstLineFromFile(const string& fileName){
    string r;
    ifstream file(fileName.c_str());
    if(file){
        getline(file, r);
        file.close();
    }else{
        throw runtime_error("Impossible to open file " + fileName);
    }
    return r;
}

vector<string> readFile(const string& fileName){
    vector<string> r;
    ifstream file(fileName.c_str());
    if(file){
        string curLine;
        while(getline(file, curLine)){
            r.push_back(curLine);
        }
        file.close();
    }else{
        throw runtime_error("Impossible to open file " + fileName);
    }
    return r;
}

void writeFile(const string& fileName, const vector<string>& lines){
    ofstream file;
    file.open(fileName.c_str());
    if(!file.is_open()){
        throw runtime_error("Impossible to open file: " + fileName);
    }
    for(size_t i = 0; i < lines.size(); i++){
        file << lines.at(i) << "\n";
    }
    file.close();
}

void writeFile(const string& fileName, const string& line){
    ofstream file;
    file.open(fileName.c_str());
    if(!file.is_open()){
        throw runtime_error("Impossible to open file: " + fileName);
    }
    file << line << "\n";
    file.close();
}

void dashedRangeToIntegers(const string& dashedRange, int& rangeStart, int& rangeStop){
    size_t dashPos = dashedRange.find_first_of("-");
    rangeStart = stringToInt(dashedRange.substr(0, dashPos));
    rangeStop = stringToInt(dashedRange.substr(dashPos + 1));
}

string intToString(int x){
    string s;
    stringstream out;
    out << x;
    return out.str();
}

vector<string>& split(const string& s, char delim, vector<string>& elems){
    stringstream ss(s);
    string item;
    while(getline(ss, item, delim)){
        elems.push_back(item);
    }
    return elems;
}


vector<string> split(const string& s, char delim){
    vector<string> elems;
    split(s, delim, elems);
    return elems;
}

string& ltrim(string& s){
    s.erase(s.begin(), find_if(s.begin(), s.end(), not1(ptr_fun<int, int>(isspace))));
    return s;
}

string& rtrim(string& s) {
    s.erase(find_if(s.rbegin(), s.rend(), not1(ptr_fun<int, int>(isspace))).base(), s.end());
    return s;
}

string& trim(string& s) {
        return ltrim(rtrim(s));
}

string errnoToStr(){
    return strerror(errno);
}

#if defined (__linux__)
vector<string> getFilesNamesInDir(const string& path, bool files, bool directories){
    vector<string> filesNames;
    DIR* dir;
    struct dirent* ent;
    if((dir = opendir(path.c_str())) != NULL){
        /* print all the files and directories within directory */
        while((ent = readdir(dir)) != NULL){
            struct stat st;
            lstat(ent->d_name, &st);
            if((S_ISDIR(st.st_mode) && directories) ||
               (!S_ISDIR(st.st_mode) && files)){
                if(string(".").compare(ent->d_name) && string("..").compare(ent->d_name)){
                    filesNames.push_back(ent->d_name);
                }
            }
        }
        closedir(dir);
    }else{
        throw runtime_error("getFilesList: " + errnoToStr());
    }
    return filesNames;
}
#endif

bool isNumber(const string& s){
    string::const_iterator it = s.begin();
    while (it != s.end() && isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

uint getClockTicksPerSecond(){
    return sysconf(_SC_CLK_TCK);
}

Msr::Msr(uint32_t id){
    string msrFileName = "/dev/cpu/" + intToString(id) + "/msr";
    _fd = open(msrFileName.c_str(), O_RDONLY);
}

Msr::~Msr(){
    if(available()){
        close(_fd);
    }
}

bool Msr::available() const{
    return _fd != -1;
}

bool Msr::read(uint32_t which, uint64_t& value) const{
    ssize_t r = pread(_fd, (void*) &value, sizeof(value), (off_t) which);
    if(r == 0){
        return false;
    }else if(r != sizeof(value)){
        throw runtime_error("Error while reading msr register: " + utils::errnoToStr());
    }
    return true;
}

bool Msr::readBits(uint32_t which, unsigned int highBit,
                      unsigned int lowBit, uint64_t& value) const{
    bool r = read(which, value);
    if(!r){
        return false;
    }

    int bits = highBit - lowBit + 1;
    if(bits < 64){
        /* Show only part of register */
        value >>= lowBit;
        value &= ((uint64_t)1 << bits) - 1;
    }

    /* Make sure we get sign correct */
    if (value & ((uint64_t)1 << (bits - 1))){
        value &= ~((uint64_t)1 << (bits - 1));
        value = -value;
    }
    return true;
}

#ifndef AMESTER_ROOT
#define AMESTER_ROOT "/tmp/sensors"
#endif

AmesterSensor::AmesterSensor(string name):
    _file((string(AMESTER_ROOT) + string("/") + name).c_str(), ios_base::in){
}

AmesterSensor::~AmesterSensor(){
    ;
}

vector<string> AmesterSensor::readFields() const{
    _file.seekg(0, _file.beg);
    string line;
    getline(_file, line);
    return split(line, ',');
}

bool AmesterSensor::exists() const{
    return _file.is_open();
}

double AmesterSensor::readSum() const{
    vector<string> fields = readFields();
    double sum = 0;
    // Skip first field (timestamp).
    for(size_t i = 1; i < fields.size(); i++){
        sum += stringToDouble(fields[i]);
    }
    return sum;
}


double AmesterSensor::readAme(uint ameId) const{
    vector<string> fields = readFields();
    if(1 + ameId >= fields.size()){
        throw runtime_error("Nonexisting ameId.");
    }
    return stringToDouble(fields[1 + ameId]);
}

pid_t gettid(){
#ifdef SYS_gettid
    return syscall(SYS_gettid);
#else
    throw runtime_error("gettid() not available.");
#endif
}

double getMillisecondsTime(){
    struct timespec spec;
    if(clock_gettime(CLOCK_MONOTONIC, &spec) == -1){
        throw runtime_error(string("clock_gettime failed: ") + string(strerror(errno)));
    }
    return spec.tv_sec * 1000.0 + spec.tv_nsec / 1.0e6;
}


}
}
