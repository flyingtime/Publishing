#include "pub_mon.hpp"

//
// ProcEntry
//

Policy ProcEntry::m_policy[] = 
{ 
        { POLICY_RESPAWN,          "res=" },
        { POLICY_RESPAWN_INTERVAL, "res_int="},
        { POLICY_RESPAWN_LIMITS,   "max_retry="},
        { POLICY_RESPAWN_FORCE,    "force="},
        { POLICY_TIMEOUT,          "tm="},
        { POLICY_TIMEOUT_INTERVAL, "tm_int="}
};

ostream& operator<< (ostream &_os, const ProcEntry& _entry)
{
        _os << _entry.m_command;
        return _os;
}

ProcEntry::ProcEntry(const char *_pCommand):
        m_command(_pCommand),
        m_policyTable()
{
        // remove possible 'newline' characters
        vector<char> charSet = {'\n', '\r'}; 
        for_each(charSet.begin(), charSet.end(), [this](char _chRep){
                string::size_type nPos = m_command.find(_chRep);
                if (nPos != string::npos) {
                        m_command.replace(nPos, 1, " ");
                }
        });
}

void ProcEntry::AddPolicy(int _nExitCode, unsigned int _nPolicyId)
{
        m_policyTable.push_back(make_pair(_nExitCode, _nPolicyId));
}

void ProcEntry::AddPolicy(const char *_pPolicy)
{
        string line = _pPolicy;
        size_t nFoundPos;
        size_t nLineSize = line.size();
        unsigned int nPolicy = sizeof(m_policy) / sizeof(Policy);

        for (unsigned int nPolicyIndex = 0; nPolicyIndex < nPolicy; nPolicyIndex++) {
                nFoundPos = 0;
                const char* &pPolicyOption = m_policy[nPolicyIndex].pOptionName;
                const int &nPolicyType = m_policy[nPolicyIndex].nType;
                while (nFoundPos < nLineSize) {
                        // find policy string such as "respawn="
                        // find next space or tab character and get substring between '=' and '\t' or '\s'
                        nFoundPos = line.find(pPolicyOption, nFoundPos);
                        if (nFoundPos != string::npos) {
                                size_t nStart = nFoundPos + strlen(pPolicyOption);
                                size_t nEnd = line.find(';', nFoundPos);
                                nEnd = ((nEnd == string::npos) ? nLineSize - 1 : nEnd);
                                nFoundPos = nEnd + 1;

                                if (nStart == nEnd) {
                                        cout << "Warning: attribute is empty for " << m_policy[nPolicyIndex].pOptionName << endl;
                                        continue;
                                }

                                int nExitCode;
                                try {
                                        nExitCode = stoi(line.substr(nStart, nEnd - nStart));
                                } catch (exception &e) {
                                        cout << "Warning: exception caught: " << e.what() << endl;
                                        cout << "Warning: maybe invalid exit code, deprecated" << endl;
                                        continue;
                                }
                                
                                // get exit code
                                AddPolicy(nExitCode, nPolicyType);
                        } else {
                                break;
                        }
                }
        }
}

void ProcEntry::Print()
{
        cout << "command: " << m_command << endl;
        cout << "policy : " << endl;
        for_each(m_policyTable.begin(), m_policyTable.end(), [](const pair<int, unsigned int> policyPair) {
                cout << " action:" << policyPair.second << "  value:" << policyPair.first << endl;
        });
}

int ProcEntry::Run()
{
        unsigned int nTimeout = 0;
        unsigned int nTimeoutInterval = 0;

        // set timeout to 0 if no timeout policy
        if (CheckTimeout(nTimeout, nTimeoutInterval) == false) {
                nTimeout = 0;
        }

        int nRetVal = OSIAPI::RunCommand(m_command.c_str(), nTimeout);
        cout << "Process [" << m_command << "] terminated with exit code = " << nRetVal << endl;
        return nRetVal;
}

bool ProcEntry::CheckTimeout(unsigned int& _nTimeout, unsigned int& _nTimeoutInterval)
{
        bool bIsValidTimeoutPolicy = false;
        _nTimeoutInterval = TIMEOUT_DEFAULT_SLEEP_INTERVAL; // default value for timeout case

        for (auto it = m_policyTable.begin(); it != m_policyTable.end(); it++) {
                switch ((*it).second) {
                case POLICY_TIMEOUT:
                        _nTimeout = static_cast<unsigned int>((*it).first);
                        bIsValidTimeoutPolicy = true;
                        break;
                case POLICY_TIMEOUT_INTERVAL:
                        _nTimeoutInterval = static_cast<unsigned int>((*it).first);
                        break;
                default:
                        break;
                }
        }
        return bIsValidTimeoutPolicy;
}

bool ProcEntry::CheckRespawn(int _nExitCode, unsigned int _nRetry, unsigned int& _nInterval)
{
        // default policy values
        bool bNeedRespawn = false;
        bool bForceRespawn = false;
        bool bReachMaxRetry = false;
        _nInterval = RESPAWN_DEFAULT_INTERVAL;

        for (auto it = m_policyTable.begin(); it != m_policyTable.end(); it++) {
                switch ((*it).second) {
                case POLICY_RESPAWN:
                        if ((*it).first == _nExitCode) {
                                bNeedRespawn = true;
                        }
                        break;
                case POLICY_RESPAWN_INTERVAL:
                        _nInterval = static_cast<unsigned int>((*it).first);
                        break;
                case POLICY_RESPAWN_LIMITS:
                        if (_nRetry >= static_cast<unsigned int>((*it).first)) {
                                bReachMaxRetry = true;
                        }
                        break;
                case POLICY_RESPAWN_FORCE:
                        bForceRespawn = true;
                        break;
                }
        }

        // RESPAWN_FORCE should always be checked ahead of other flags
        if (bForceRespawn) {
                return true;
        } else if (bReachMaxRetry) {
                return false;
        }
        return bNeedRespawn;
}

//
// PubMonitor
//

PubMonitor::PubMonitor()
{
}

PubMonitor::~PubMonitor()
{
        // cleanup entries
        for (auto it = m_procTable.begin(); it != m_procTable.end(); it++) {
                delete *it;
        }
}

// load initial configuration file
int PubMonitor::LoadConfig(const char *_pPath)
{
        if (_pPath == nullptr || strlen(_pPath) == 0) {
                return 0;
        }

        m_confPath = _pPath;

        // open configuration file
        ifstream fileObject(_pPath);
        if (!fileObject.is_open()) {
                cout << "Error: cannot open file :" << _pPath << endl;
                return -1;
        }

        unsigned int nLine = 0, nValidLine = 0;
        ProcEntry *pEntry = nullptr;
        string line;

        while (getline(fileObject, line)) {
                nLine++;

                if (!IsValidLine(line.c_str())) {
                        continue;
                }
                nValidLine++;

                // command line
                if (nValidLine % 2 != 0) {
                        pEntry = new ProcEntry(line.c_str());
                        cout << "Info: loading entry [" << *pEntry << "] ..." << endl;
                }
                // policy line
                if (nValidLine % 2 == 0) {
                        if (pEntry != nullptr) {
                                pEntry->AddPolicy(line.c_str());
                                AddEntry(pEntry);
                                cout << "Info: entry [" << *pEntry << "] is loaded" << endl;
                        }
                        pEntry = nullptr;
                }
        }
        fileObject.close();
        return nValidLine / 2;
}

bool PubMonitor::IsValidLine(const char *_pLine)
{
        int nLength = strlen(_pLine);

        if (nLength == 0) {
                return false;
        }

        // comment line
        if (_pLine[0] == '#') {
                return false;
        }

        // line with spaces and tabs only
        for (int i = 0; i < nLength; i++) {
                if (_pLine[i] != ' ' && _pLine[i] != '\t' && _pLine[i] != '\r' && _pLine[i] != '\n') {
                        return true;
                }
        }
        return false;
}

void PubMonitor::AddEntry(ProcEntry *_pEntry)
{
        m_procTable.push_back(_pEntry);
}

void PubMonitor::PrintTable()
{
        const char *pSplit = "----------";
        for_each(m_procTable.begin(), m_procTable.end(), [pSplit](ProcEntry *pEntry){
                cout << pSplit << endl;
                pEntry->Print();
        });
        cout << pSplit << endl;
}

void PubMonitor::Run()
{
        for (auto it = m_procTable.begin(); it != m_procTable.end(); it++) {
                OSIAPI::RunThread(PubMonitor::MonitorThread, *it);
        }
        OSIAPI::WaitForAllThreads();
}

void PubMonitor::MonitorThread(void *_pParam)
{
        ProcEntry *pEntry;
        int nExitCode;
        unsigned int nInterval = 0, nRetry = 0, nTimeout = 0, nTimeoutReached = 0;
        while (true) {
                pEntry = (ProcEntry *)_pParam;
                nExitCode = pEntry->Run();
                if (nExitCode == RUNCMD_RV_WAIT_TIMEOUT) {
                        nTimeoutReached++;
                        pEntry->CheckTimeout(nTimeout, nInterval);
                        cout << "timeout #" << nTimeoutReached << ", sleep " << nInterval << " seconds ..." << endl;
                        OSIAPI::MakeSleep(nInterval);
                        continue;
                }
                if (pEntry->CheckRespawn(nExitCode, nRetry, nInterval)) {
                        nRetry++;
                        cout << "retry #" << nRetry << ", sleep " << nInterval << " seconds ..." << endl;
                        OSIAPI::MakeSleep(nInterval);
                } else {
                        // child threads exit here
                        cout << "Process [" << *pEntry << "] is exiting, retry #" << nRetry << endl;
                        break;
                }
        }
}
