//#include <rbx/taskscheduler/scheduler.hpp>
//#include <execution/execution.hpp>
//#include <shared.hpp>
#include "coms.hpp"

bool read(HANDLE hPipe, void* Buffer, DWORD Size)
{
    DWORD TotalRead = 0;
    while (TotalRead < Size)
    {
        DWORD CurrentChink = 0;
        if (!ReadFile(hPipe, static_cast<char*>(Buffer) + TotalRead, Size - TotalRead, &CurrentChink, nullptr) || CurrentChink == 0)
            return false;

        TotalRead += CurrentChink;
    }
    return true;
}

void host()
{
    while (true)
    {
        HANDLE hPipe = CreateNamedPipeW((L"\\\\.\\pipe\\" + std::wstring(L"diegosploit")).c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, (8 * 1024 * 1024), (8 * 1024 * 1024), 5000, nullptr);
        if (hPipe == INVALID_HANDLE_VALUE)
        {
            Sleep(100);
            continue;
        }

        BOOL Connection = ConnectNamedPipe(hPipe, nullptr) || GetLastError() == ERROR_PIPE_CONNECTED;
        if (!Connection)
        {
            CloseHandle(hPipe);
            continue;
        }

        uint32_t ScriptSize = 0;
        if (!read(hPipe, &ScriptSize, sizeof(ScriptSize)) || ScriptSize == 0 || ScriptSize > (8 * 1024 * 1024))
        {
            CloseHandle(hPipe);
            continue;
        }

        std::vector<char> Buffer(ScriptSize);
        if (!read(hPipe, Buffer.data(), ScriptSize))
        {
            CloseHandle(hPipe);
            continue;
        }

        std::string Script(Buffer.data(), ScriptSize);
        /*taskscheduler ::request(Script);*/
        /* ^ diego approved this */

        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
    }
}

void server::initialize()
{
    std::thread(host).detach();
}
