#include "api.hpp"

#if defined(__WIN32__) || defined(_WIN32)
        vector<HANDLE> OSIAPI::m_threads;
#else
        vector<pid_t> OSIAPI::m_threads;
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

int OSIAPI::RunThread(OSIAPI_THREAD_RETURN_TYPE (*_pFunction)(void*), void *_pParameter)
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

struct ParamSet
{
        const char *pCommand;
        void *pExitCode;
};

#define DELIMITER_CHAR ' '
#define MAX_ARGUMENT_NUMBER 32
int OSIAPI::RunCommand(const char *_pCommand, unsigned int _nSeconds)
{
        pid_t worker;
        worker = fork();
        if (worker == 0) {
                // handle delimiters and convert to the same character
                string command = _pCommand;
                for (unsigned int i = 0; i < command.size(); i++) {
                        if (command[i] == '\t' || command[i] == '\n' || command[i] == '\r') {
                                command[i] = DELIMITER_CHAR;
                        }
                }

                // get each command argument in the vector
                stringstream ss(command);
                vector<string> argsContainer;
                string arg;
                while (getline(ss, arg, DELIMITER_CHAR)) {
                        argsContainer.push_back(arg);
                }

                // check arguments number
                if (argsContainer.size() > MAX_ARGUMENT_NUMBER) {
                        cout << "Error: command: " << _pCommand << " has too many arguments" << endl;
                        exit(RUNCMD_RV_CREATE_PROCESS);
                }

                // vector to array
                char *args[argsContainer.size() + 1];
                args[argsContainer.size()] = nullptr;
                int i = 0;
                for (auto it = argsContainer.begin(); it != argsContainer.end(); it++) {
                        args[i] = &(*it)[0];
                        i++;
                }

                // execute command
                int nStatus = execv(args[0], args);
                cout << "Error: " << "execv failed with: " << nStatus << " errno: " << strerror(errno) << endl;
                exit(nStatus);
        } else {
                int nExit;
                if (_nSeconds == 0) {
                        waitpid(worker, &nExit, 0);
                } else {
                        unsigned int nNow = 0;
                        pid_t waitPid;
                        while (1) {
                                waitPid = waitpid(worker, &nExit, WNOHANG);
                                if (waitPid == 0) {
                                        if (nNow >= _nSeconds) {
                                                kill(worker, SIGKILL);
                                                return RUNCMD_RV_WAIT_TIMEOUT;
                                        }
                                } else if (waitPid == worker) {
                                        break;
                                } else if (waitPid == -1) {
                                        return RUNCMD_RV_WAIT_FAILED;
                                } else {
                                        return RUNCMD_RV_WAIT_UNCAUGHT;
                                }
                                nNow++;
                                sleep(1);
                        }
                }

                if(WIFEXITED(nExit)) {
                        return WEXITSTATUS(nExit);
                }
                if(WIFSIGNALED(nExit)) {
                        cout << "Notice: process " << worker << " killed: signal " << WTERMSIG(nExit) << endl;
                        if (WCOREDUMP(nExit)) {
                                cout << "Notice: core dumped" << endl;
                        }
                        return 128 + WTERMSIG(nExit);
                }
        }
        return 0;
}

int OSIAPI::RunThread(OSIAPI_THREAD_RETURN_TYPE (*_pFunction)(void*), void *_pParameter)
{
        pid_t nPid = fork();
        if (nPid == 0) {
                _pFunction(_pParameter);
                exit(0);
        } else {
                OSIAPI::m_threads.push_back(nPid);
        }
        return 0;
}

int OSIAPI::WaitForAllThreads()
{
        int nStatus;
        for (auto it = OSIAPI::m_threads.begin(); it != OSIAPI::m_threads.end();) {
                waitpid(*it, &nStatus, 0);
                OSIAPI::m_threads.erase(it);
        }
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

void OSIAPI::GetTime(string& _time)
{
        time_t nTimestamp;
        struct tm *timeStruct;
        char formatedTime[64];
        nTimestamp = time(nullptr);
        timeStruct = localtime(&nTimestamp);
        strftime(formatedTime, 64, "%Y-%m-%d %H:%M:%S", timeStruct);
        _time = formatedTime;
}

void OSIAPI::PrintTime()
{
        string time;
        GetTime(time);
        cout << "[" << time << "]";
}
