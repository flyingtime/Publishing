#ifndef __API_HPP__
#define __API_HPP__

#if defined(__WIN32__) || defined(_WIN32)
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
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

#if defined(__WIN32__) || defined(_WIN32)
typedef DWORD OSIAPI_THREAD_RETURN_TYPE;
#define OSIAPI_THREAD_RETURN_OK 0
#else
typedef void* OSIAPI_THREAD_RETURN_TYPE;
#define OSIAPI_THREAD_RETURN_OK NULL
#endif

class OSIAPI
{
public:
        OSIAPI() = delete;
        // RunCommand: when nSeconds == 0, that means no timeout
        static int RunCommand(const char *pCommand, unsigned int nSeconds = 0);
        static void MakeSleep(unsigned int nSeconds);

        // threads
public:
        static int RunThread(OSIAPI_THREAD_RETURN_TYPE (*pFunction)(void *), void *nParameter);
        static int WaitForAllThreads();
private:
#if defined(__WIN32__) || defined(_WIN32)
        static vector<HANDLE> m_threads;
#else
        static vector<pid_t> m_threads;
#endif
};

#endif
