#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <cstring>

#include "api.hpp"

using namespace std;

typedef enum _PType {
        POLICY_RESPAWN,
        POLICY_RESPAWN_INTERVAL,
        POLICY_RESPAWN_LIMITS,
        POLICY_RESPAWN_FORCE,
        POLICY_TIMEOUT,
        POLICY_TIMEOUT_INTERVAL
} PType;

typedef struct _Policy
{
        PType nType;
        const char *pOptionName;
} Policy;

#define RESPAWN_DEFAULT_INTERVAL        5 // seconds
#define TIMEOUT_DEFAULT_SLEEP_INTERVAL  1 // seconds
class ProcEntry
{
public:
        ProcEntry(const char *pCommand);
        void AddPolicy(int nExitCode, unsigned int nPolicyId);
        void AddPolicy(const char *pPolicy);
        void Print();
        int Run(unsigned int nTimeout = 0);
        bool CheckRespawn(int nExitCode, unsigned int nRetry, unsigned int& nInterval);
        bool CheckTimeout(unsigned int& nTimeout, unsigned int& nTimeoutInterval);
        friend ostream& operator<< (ostream &os, const ProcEntry& entry);
private:
        // policy strings
        static Policy m_policy[];

        string m_command;
        vector< pair<int, unsigned int> > m_policyTable;
};

class PubMonitor
{
public:
        PubMonitor();
        ~PubMonitor();
        int LoadConfig(const char *pPath);
        void Run();
        void PrintTable();
private:
        void AddEntry(ProcEntry *pEntry);
        bool IsValidLine(const char *pLine);
        static OSIAPI_THREAD_RETURN_TYPE MonitorThread(void *pParam);

        string m_confPath;
        vector<ProcEntry *> m_procTable;
};

