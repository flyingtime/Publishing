#ifndef __API_HPP__
#define __API_HPP__

#if defined(__WIN32__) || defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <iostream>
#include <vector>
#include <cstdlib>

using namespace std;

#define RUNCMD_RV_CREATE_PROCESS   -100
#define RUNCMD_RV_GET_EXIT_CODE    -101
#define RUNCMD_RV_WAIT_ABANDONED   -200
#define RUNCMD_RV_WAIT_TIMEOUT     -201
#define RUNCMD_RV_WAIT_FAILED      -202
#define RUNCMD_RV_WAIT_UNCAUGHT    -203

class OSIAPI
{
public:
        OSIAPI() = delete;
        static int RunCommand(const char *pCommand, unsigned int nSeconds = 0);
        static void MakeSleep(unsigned int nSeconds);

        // threads
public:
        static int RunThread(void (*pFunction)(void *), void *nParameter);
        static int WaitForAllThreads();
private:
#if defined(__WIN32__) || defined(_WIN32)
        static vector<HANDLE> m_threads;
#else
        static vector<int> m_threads;
#endif
};

#endif
