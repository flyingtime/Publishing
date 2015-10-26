#include "api.hpp"

#if defined(__WIN32__) || defined(_WIN32)
        vector<HANDLE> OSIAPI::m_threads;
#else
        vector<int> OSIAPI::m_threads;
#endif


// functions

#if defined(__WIN32__) || defined(_WIN32)

int OSIAPI::RunCommand(const char *_pCommand, unsigned int _nSeconds)
{
        int nRetVal = 0;

        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        DWORD dwExitCode = 0;

        ZeroMemory(&si, sizeof(si));
        ZeroMemory(&pi, sizeof(pi));

        BOOL bRetVal = CreateProcess(nullptr, (LPSTR)_pCommand, nullptr, nullptr, FALSE,
                                     NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
        if (bRetVal == true) {
                DWORD dwMilliSeconds;
                if (_nSeconds == 0) {
                        dwMilliSeconds = INFINITE;
                } else {
                        dwMilliSeconds = _nSeconds * 1000;
                }
                DWORD dwRetVal = WaitForSingleObject(pi.hProcess, dwMilliSeconds);

                switch (dwRetVal) {
                case WAIT_ABANDONED:
                        nRetVal = RUNCMD_RV_WAIT_ABANDONED;
                        break;
                case WAIT_OBJECT_0:
                        // Get the exit code.
                        bRetVal = GetExitCodeProcess(pi.hProcess, &dwExitCode);
                        if (bRetVal == true) {
                                nRetVal = static_cast<int>(dwExitCode);
                        } else {
                                // failed to get exit code
                                nRetVal = RUNCMD_RV_GET_EXIT_CODE;
                        }
                        break;
                case WAIT_TIMEOUT:
                        TerminateProcess(pi.hProcess, 1);
                        nRetVal = RUNCMD_RV_WAIT_TIMEOUT;
                        break;
                case WAIT_FAILED:
                        nRetVal = RUNCMD_RV_WAIT_FAILED;
                        break;
                default:
                        nRetVal = RUNCMD_RV_WAIT_UNCAUGHT;
                        break;
                }

                // Close the handles.
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
        } else {
                // failed to create a new process
                nRetVal = RUNCMD_RV_CREATE_PROCESS;
        }
        return nRetVal;
}

int OSIAPI::RunThread(void (*_pFunction)(void*), void *_pParameter)
{
        HANDLE hThread;
        DWORD dwThreadId;

        hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)_pFunction, _pParameter, 0, &dwThreadId);
        OSIAPI::m_threads.push_back(hThread);
        return 0;
}

int OSIAPI::WaitForAllThreads()
{
        // wait until all threads have terminated
        WaitForMultipleObjects(OSIAPI::m_threads.size(), OSIAPI::m_threads.data(), TRUE, INFINITE);
        for (auto it = OSIAPI::m_threads.begin(); it != OSIAPI::m_threads.end(); it++) {
                CloseHandle(*it);
                OSIAPI::m_threads.erase(it);
        }
        return 0;
}

#else

int OSIAPI::RunCommand(const char *_pCommand, unsigned int _nSeconds)
{
        int nRetVal = system(_pCommand);
        nRetVal >>= 8;
        return nRetVal;
}

int OSIAPI::RunThread(void (*_pFunction)(void*), void *_pParameter)
{
        std::cout << "Notice: posix threading is not supported now" << endl;
        exit(1);

        return 0;
}

int OSIAPI::WaitForAllThreads()
{
        std::cout << "Notice: posix waiting is not supported now" << endl;
        exit(1);

        return 0;
}
#endif

void OSIAPI::MakeSleep(unsigned int nSeconds)
{
#if defined(__WIN32__) || defined(_WIN32)
        Sleep(nSeconds * 1000);
#else
        sleep(nSeconds);
#endif
}
