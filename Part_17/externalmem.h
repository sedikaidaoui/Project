//#pragma once
//#include <Windows.h>
//#include <vector>
//#include <string>
//#include <unordered_map>
//#include <iostream>
//#include <TlHelp32.h>
//#include <tchar.h>
//#define WIN32_LEAN_AND_MEAN
//#include <winternl.h>
//#include <sstream>
//
//#include <future>
//
//#include <iomanip>
//
//
//#pragma comment(lib, "ntdll.lib")
//
//
//
//
//std::string MemoryLogs = "";
//
//extern "C" NTSTATUS ZwReadVirtualMemory(HANDLE hProcess, LPVOID lpBaseAddress, void* lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead = NULL);
//extern "C" NTSTATUS ZwWriteVirtualMemory(HANDLE hProcess, LPVOID lpBaseAddress, void* lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead = NULL);
//extern "C" NTSTATUS ZwProtectVirtualMemory(HANDLE hProcess, LPVOID BaseAddress, size_t  NumberOfBytesToProtect, ULONG NewAccessProtection, PULONG OldAccessProtection);
//
//class fastmemory
//{
//
//public:
//    DWORD ProcessId = 0;
//    HANDLE ProcessHandle;
//
//    typedef struct _MEMORY_REGION
//    {
//        DWORD_PTR dwBaseAddr;
//        DWORD_PTR dwMemorySize;
//    }MEMORY_REGION;
//
//    int GetPid(const char* procname)
//    {
//        if (procname == NULL)
//            return 0;
//
//        DWORD pid = 0;
//        DWORD threadCount = 0;
//
//        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
//        if (hSnap == INVALID_HANDLE_VALUE)
//            return 0;
//
//        PROCESSENTRY32 pe;
//        pe.dwSize = sizeof(PROCESSENTRY32);
//
//        if (Process32First(hSnap, &pe))
//        {
//            do
//            {
//
//#ifdef UNICODE
//                char exeFile[MAX_PATH];
//                size_t convertedChars = 0;
//                wcstombs_s(&convertedChars, exeFile, pe.szExeFile, _TRUNCATE);
//                if (_stricmp(exeFile, procname) == 0)
//
//#else
//
//                if (_stricmp(pe.szExeFile, procname) == 0)
//
//#endif
//
//                {
//                    if ((int)pe.cntThreads > threadCount)
//                    {
//                        threadCount = pe.cntThreads;
//                        pid = pe.th32ProcessID;
//                    }
//                }
//            } while (Process32Next(hSnap, &pe));
//        }
//
//        CloseHandle(hSnap);
//
//        return pid;
//    }
//
//    const char* GetEmulatorRunning()
//    {
//        if (GetPid("HD-Player.exe") != 0)
//            return "HD-Player.exe";
//
//        else if (GetPid("LdVBoxHeadless.exe") != 0)
//            return "LdVBoxHeadless.exe";
//
//        else if (GetPid("MEmuHeadless.exe") != 0)
//            return "MEmuHeadless.exe";
//
//        else if (GetPid("LdVBoxHeadless.exe") != 0)
//            return "LdVBoxHeadless.exe";
//
//        else if (GetPid("AndroidProcess.exe") != 0)
//            return "AndroidProcess.exe";
//
//        else if (GetPid("aow_exe.exe") != 0)
//            return "aow_exe.exe";
//
//        else if (GetPid("NoxVMHandle.exe") != 0)
//            return "NoxVMHandle.exe";
//    }
//
//
//
//
//
//
//
//
//    struct EntityCameraHackHere2
//    {
//        DWORD_PTR addressCameraHack2;
//        std::vector<BYTE> patternCameraHack2;
//    };
//
//    std::vector<EntityCameraHackHere2> OldCameraHack2;
//    std::vector<DWORD_PTR> NewCameraHack2;
//
//    struct ConfigStruct
//    {
//        float cameraVal = 0.0f;
//    } Config;
//
//    float previousCameraVal = Config.cameraVal;
//    bool camera = true;
//
//    bool SaveAoBCameraHackNew()
//    {
//        SYSTEM_INFO si;
//        GetSystemInfo(&si);
//        DWORD_PTR startAddress = reinterpret_cast<DWORD_PTR>(si.lpMinimumApplicationAddress);
//        DWORD_PTR endAddress = reinterpret_cast<DWORD_PTR>(si.lpMaximumApplicationAddress);
//
//        // <-- new search pattern (40 bytes)
//        std::vector<BYTE> SearchCameraHack = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F };
//
//        if (!AttackProcess("HD-Player.exe"))
//        {
//
//            notificationSystem.AddNotification("Emulator Not found", 2500);
//            return false;
//        }
//
//        notificationSystem.AddNotification("Activating...", 2500);
//        NewCameraHack2.clear();
//        OldCameraHack2.clear();
//
//        if (!FastFindPatternMem(startAddress, endAddress, SearchCameraHack.data(), NewCameraHack2))
//        {
//            notificationSystem.AddNotification("Pattern Not Found!", 2500);
//            return false;
//        }
//
//        for (const auto& address : NewCameraHack2)
//        {
//            EntityCameraHackHere2 entity;
//            entity.addressCameraHack2 = address;
//            entity.patternCameraHack2 = SearchCameraHack; // save original pattern
//            OldCameraHack2.push_back(entity);
//        }
//
//       
//        notificationSystem.AddNotification("Camera Left : Loaded !", 2500);
//        return true;
//    }
//
//    void CameraOn()
//    {
//        if (!AttackProcess("HD-Player.exe"))
//        {
//            notificationSystem.AddNotification("Emulator Not found", 2500);
//         
//            return;
//        }
//
//        if (NewCameraHack2.empty())
//        {
//            if (!SaveAoBCameraHackNew())
//                return;
//        }
//
//
//        std::vector<BYTE> ReplaceCameraHack = { 0x00, 0x00, 0x00, 0x00, 0x22, 0x9E, 0x93, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F };
//
//        for (DWORD_PTR addr : NewCameraHack2)
//        {
//            Replace(addr, ReplaceCameraHack.data(), static_cast<SIZE_T>(ReplaceCameraHack.size()));
//        }
//        notificationSystem.AddNotification("Camera Left : Enabled !", 2500);
//     
//    }
//
//    void CameraOff()
//    {
//        if (!AttackProcess("HD-Player.exe"))
//        {
//            notificationSystem.AddNotification("Emulator Not found", 2500);
//            return;
//        }
//
//        if (NewCameraHack2.empty())
//        {
//            if (!SaveAoBCameraHackNew())
//                return;
//        }
//
//
//        std::vector<BYTE> OriginalCameraHack = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F };
//
//        for (DWORD_PTR addr : NewCameraHack2)
//        {
//            Replace(addr, OriginalCameraHack.data(), static_cast<SIZE_T>(OriginalCameraHack.size()));
//        }
//        notificationSystem.AddNotification("Camera Left : Disabled !", 2500);
//      
//    }
//
//
//
//
//    struct FastFireData
//    {
//        DWORD_PTR addressFastFire;
//        std::vector<BYTE> originalBytes;
//        int patternType;
//    };
//
//    std::vector<FastFireData> OriginalFastFire;
//    std::vector<DWORD_PTR> FoundFastFire;
//
//    bool fastfire()
//    {
//        Beep(500, 500);
//
//        if (!AttackProcess("HD-Player.exe"))
//        {
//            notificationSystem.AddNotification("Emulator Not found", 2500);
//            return false;
//        }
//
//     
//        notificationSystem.AddNotification("Scanning ...", 2500);
//
//        SYSTEM_INFO si;
//        GetSystemInfo(&si);
//        DWORD_PTR startAddress = reinterpret_cast<DWORD_PTR>(si.lpMinimumApplicationAddress);
//        DWORD_PTR endAddress = reinterpret_cast<DWORD_PTR>(si.lpMaximumApplicationAddress);
//
//      
//        std::vector<BYTE> SearchFastFire1 = { 0xAA, 0xAA, 0x2A, 0xEA, 0x99, 0x99, 0x39, 0xEA, 0x97, 0xA0, 0x5D, 0x95 };
//        std::vector<BYTE> SearchFastFire2 = { 0xAA, 0xAA, 0x2A, 0xEA, 0xAA, 0xAA, 0x2A, 0xEA, 0x61, 0x78, 0xE7, 0x94 };
//
//
//        FoundFastFire.clear();
//        OriginalFastFire.clear();
//
//        std::vector<DWORD_PTR> foundPattern1;
//        std::vector<DWORD_PTR> foundPattern2;
//
//       
//        bool found1 = FastFindPatternMem(startAddress, endAddress, SearchFastFire1.data(), foundPattern1);
//        bool found2 = FastFindPatternMem(startAddress, endAddress, SearchFastFire2.data(), foundPattern2);
//
//        if (!found1 && !found2)
//        {
//            notificationSystem.AddNotification("Pattern Not found", 2500);
//            return false;
//        }
//
//    
//        for (const auto& addr : foundPattern1)
//        {
//            FastFireData data;
//            data.addressFastFire = addr;
//            data.patternType = 1;
//            OriginalFastFire.push_back(data);
//            FoundFastFire.push_back(addr);
//        }
//
//        for (const auto& addr : foundPattern2)
//        {
//            FastFireData data;
//            data.addressFastFire = addr;
//            data.patternType = 2;
//            OriginalFastFire.push_back(data);
//            FoundFastFire.push_back(addr);
//        }
//
//        if (FoundFastFire.empty())
//        {
//           
//            notificationSystem.AddNotification("Address Not Found!", 2500);
//            return false;
//        }
//
//        Beep(500, 500);
//     
//        
//
//        return true;
//    }
//
//    bool Activatefastfire()
//    {
//        if (!AttackProcess("HD-Player.exe"))
//        {
//            notificationSystem.AddNotification("Emulator Not found", 2500);
//            return false;
//        }
//
//       
//        std::vector<BYTE> replaceBytes1 = { 0xA8, 0x81, 0xAD, 0x97, 0xA8, 0x81, 0xAD, 0x97, 0xA8, 0x81, 0xAD, 0x97 };
//        std::vector<BYTE> replaceBytes2 = { 0xA2, 0x93, 0xCA, 0x91, 0xA2, 0x93, 0xCA, 0x91, 0xA2, 0x93, 0xCA, 0x91 };
//
//        if (OriginalFastFire.empty())
//        {
//            MemoryLogs = "Error: No fast fire addresses found. Please scan first.";
//            return false;
//        }
//
//        notificationSystem.AddNotification("Activating ...", 2500);
//    
//        int successCount = 0;
//        int totalCount = OriginalFastFire.size();
//
//       
//        for (auto& data : OriginalFastFire)
//        {
//            data.originalBytes.resize(12);
//
//            if (!ReadProcessMemory(ProcessHandle, reinterpret_cast<LPCVOID>(data.addressFastFire),
//                data.originalBytes.data(), 12, nullptr))
//            {
//                DWORD error = GetLastError();
//                MemoryLogs += "\nFailed to read original bytes at address: " +
//                    std::to_string(data.addressFastFire) +
//                    " Error: " + std::to_string(error);
//            }
//        }
//
//      
//        for (const auto& data : OriginalFastFire)
//        {
//            const std::vector<BYTE>* replaceBytes = nullptr;
//
//            if (data.patternType == 1)
//                replaceBytes = &replaceBytes1;
//            else if (data.patternType == 2)
//                replaceBytes = &replaceBytes2;
//            else
//                continue;
//
//            if (WriteProcessMemory(ProcessHandle, reinterpret_cast<LPVOID>(data.addressFastFire),
//                replaceBytes->data(), replaceBytes->size(), nullptr))
//            {
//                successCount++;
//            }
//            else
//            {
//                DWORD error = GetLastError();
//                MemoryLogs += "\nFailed at address: " + std::to_string(data.addressFastFire) +
//                    " (Pattern " + std::to_string(data.patternType) + ")" +
//                    " Error: " + std::to_string(error);
//            }
//        }
//
//        if (successCount > 0)
//        {
//            Beep(800, 300);
//         
//            notificationSystem.AddNotification("Activated", 2500);
//            return true;
//        }
//        else
//        {
//         
//            notificationSystem.AddNotification("Failed", 2500);
//            return false;
//        }
//    }
//
//    bool OFFfastfire()
//    {
//        if (!AttackProcess("HD-Player.exe"))
//        {
//            notificationSystem.AddNotification("Emulator Not found", 2500);
//            return false;
//        }
//
//        if (OriginalFastFire.empty())
//        {
//            MemoryLogs = "Error: No original data stored. Cannot restore.";
//            return false;
//        }
//
//        notificationSystem.AddNotification("Deactivating...", 2500);
//   
//
//        int successCount = 0;
//        int totalCount = OriginalFastFire.size();
//
//        for (const auto& original : OriginalFastFire)
//        {
//            if (original.originalBytes.empty())
//                continue;
//
//            if (WriteProcessMemory(ProcessHandle,
//                reinterpret_cast<LPVOID>(original.addressFastFire),
//                original.originalBytes.data(),
//                original.originalBytes.size(),
//                nullptr))
//            {
//                successCount++;
//            }
//            else
//            {
//                DWORD error = GetLastError();
//                MemoryLogs += "\nFailed to restore address: " +
//                    std::to_string(original.addressFastFire) +
//                    " Error: " + std::to_string(error);
//            }
//        }
//
//        if (successCount > 0)
//        {
//            Beep(600, 300);
//
//            notificationSystem.AddNotification("Fast Fire v2 : Deactivated Successfully!", 2500);
//
//            MemoryLogs = "Success: Restored " + std::to_string(successCount) +
//                "/" + std::to_string(totalCount) + " addresses";
//            return true;
//        }
//        else
//        {
//            notificationSystem.AddNotification("Fast Fire v2 : Deactivation Failed!", 2500);
//            return false;
//        }
//    }
//
//    // GLITCH FIRE
//    struct EntityglichHere
//    {
//        DWORD_PTR addressglich;
//        std::vector<BYTE> patternglich;
//    };
//
//    std::vector<EntityglichHere> Oldglich;
//    std::vector<DWORD_PTR> Newglich;
//
//    bool glichfire()
//    {
//        std::vector<BYTE> replace = { 0x00, 0xC0, 0x30, 0x00, 0x00, 0x00, 0x3F };
//
//        if (Newglich.empty())
//        {
//            notificationSystem.AddNotification("", 2500);
//            MemoryLogs = "No addresses found!";
//            return false;
//        }
//
//
//        for (const auto& address : Newglich)
//        {
//            if (!WriteProcessMemory(ProcessHandle, reinterpret_cast<LPVOID>(address), replace.data(), replace.size(), nullptr))
//            {
//                MemoryLogs += "Failed to write memory at address";
//                continue;
//            }
//           
//
//            notificationSystem.AddNotification("Glich Fire : Activated !", 2500);
//            Beep(500, 500);
//        }
//
//        return true;
//    }
//
//
//    bool OFFglichfire()
//    {
//        std::vector<BYTE> replace = { 0x00, 0xC0, 0x3F, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40 };
//
//
//        if (Newglich.empty())
//        {
//            return false;
//        }
//
//
//        for (const auto& address : Newglich)
//        {
//            if (!WriteProcessMemory(ProcessHandle, reinterpret_cast<LPVOID>(address), replace.data(), replace.size(), nullptr))
//            {
//                MemoryLogs += "Failed to write memory at address";
//                continue;
//            }
//
//         
//            notificationSystem.AddNotification("Glich Fire : Desactivated !", 2500);
//          
//            Beep(500, 500);
//        }
//
//        return true;
//    }
//
//
//
//
//
//
//
//
//
//
//
//
//    std::string MakeAob(float value)
//    {
//        unsigned char* bytes = reinterpret_cast<unsigned char*>(&value);
//        std::ostringstream oss;
//        for (int i = 0; i < sizeof(float); ++i)
//        {
//            if (i > 0) oss << "-";
//            oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]);
//        }
//
//        std::string hexString = oss.str();
//        std::replace(hexString.begin(), hexString.end(), '-', ' ');
//        return hexString;
//    }
//
//
//    bool Replace(DWORD_PTR address, BYTE* replacePattern, SIZE_T size)
//    {
//        return WriteProcessMemory(ProcessHandle, reinterpret_cast<LPVOID>(address), replacePattern, size, nullptr);
//    }
//
//    std::vector<BYTE> IntToByteArray(int value)
//    {
//        std::vector<BYTE> byteArray(sizeof(value));
//        BYTE* pValue = reinterpret_cast<BYTE*>(&value);
//
//        for (size_t i = 0; i < sizeof(value); ++i)
//        {
//            byteArray[i] = pValue[i];
//        }
//
//        return byteArray;
//    }
//
//
//    void ReWrite(std::string type, DWORD_PTR dwStartRange, DWORD_PTR dwEndRange, BYTE* Search, BYTE* Replace)
//    {
//        if (!AttackProcess(GetEmulatorRunning()))
//            MemoryLogs = type + ": An unexpected error occurred";
//
//        MemoryLogs = "Applying - " + type;
//        bool Status = ReplacePattern(dwStartRange, dwEndRange, Search, Replace);
//        if (Status)
//            MemoryLogs = type + " - Enabled!";
//        else
//            MemoryLogs = type + " : Failed to Enable!";
//
//        CloseHandle(ProcessHandle);
//    }
//
//    void deWrite(std::string type, DWORD_PTR dwStartRange, DWORD_PTR dwEndRange, BYTE* Search, BYTE* Replace)
//    {
//        if (!AttackProcess(GetEmulatorRunning()))
//            MemoryLogs = type + ": An unexpected error occurred";;
//
//        bool Status = ReplacePattern(dwStartRange, dwEndRange, Search, Replace);
//        if (Status)
//            MemoryLogs = type + " - Disabled!";
//        else
//            MemoryLogs = type + " : Failed to Disable!";
//
//        CloseHandle(ProcessHandle);
//    }
//
//    BOOL AttackProcess(const char* procname)
//    {
//        DWORD ProcId = GetPid(GetEmulatorRunning());
//        if (ProcId == 0)
//            return false;
//
//        ProcessId = ProcId;
//        ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, 0, ProcessId);
//        return ProcessHandle != nullptr;
//    }
//
//    bool ReplacePattern(DWORD_PTR dwStartRange, DWORD_PTR dwEndRange, BYTE* SearchAob, BYTE* ReplaceAob)
//    {
//        int RepByteSize = _msize(ReplaceAob);
//        if (RepByteSize <= 0) return false;
//        std::vector<DWORD_PTR> foundedAddress;
//        FastFindPatternMem(dwStartRange, dwEndRange, SearchAob, foundedAddress);
//        if (foundedAddress.empty())
//            return false;
//
//        DWORD OldProtect;
//        for (int i = 0; i < foundedAddress.size(); i++)
//        {
//            ZwProtectVirtualMemory(ProcessHandle, (LPVOID)foundedAddress[i], RepByteSize, PAGE_EXECUTE_READWRITE, &OldProtect);
//            ZwWriteVirtualMemory(ProcessHandle, (LPVOID)foundedAddress[i], ReplaceAob, RepByteSize, 0);
//            ZwProtectVirtualMemory(ProcessHandle, (LPVOID)foundedAddress[i], RepByteSize, PAGE_EXECUTE_READ, &OldProtect);
//        }
//
//        return true;
//    }
//
//
//    bool ChangePattern(DWORD_PTR dwStartRange, DWORD_PTR dwEndRange, BYTE* Search, BYTE* Replace)
//    {
//        if (!AttackProcess(GetEmulatorRunning())) return false;
//
//        bool Status = ReplacePattern(dwStartRange, dwEndRange, Search, Replace);
//        if (Status) return true;
//        else return false;
//
//        CloseHandle(ProcessHandle);
//    }
//
//    bool HookPattern(DWORD_PTR dwStartRange, DWORD_PTR dwEndRange, BYTE* SearchAob, BYTE* ReplaceAob, std::vector<DWORD_PTR>& AddressRet)
//    {
//        if (!AttackProcess(GetEmulatorRunning())) return false;
//        int RepByteSize = _msize(ReplaceAob);
//        if (RepByteSize <= 0) return false;
//
//        if (AddressRet.empty())
//        {
//            FastFindPatternMem(dwStartRange, dwEndRange, SearchAob, AddressRet);
//            if (AddressRet.empty()) return false;
//
//            DWORD OldProtect;
//            for (int i = 0; i < AddressRet.size(); i++)
//            {
//                WriteProcessMemory(ProcessHandle, (LPVOID)AddressRet[i], ReplaceAob, RepByteSize, 0);
//            }
//
//            return true;
//        }
//        else {
//            DWORD OldProtect;
//            for (int i = 0; i < AddressRet.size(); i++)
//            {
//                WriteProcessMemory(ProcessHandle, (LPVOID)AddressRet[i], ReplaceAob, RepByteSize, 0);
//            }
//            return true;
//        }
//        CloseHandle(ProcessHandle);
//    }
//
//    bool FastFindPatternMem(DWORD_PTR StartRange, DWORD_PTR EndRange, BYTE* SearchBytes, std::vector<DWORD_PTR>& AddressRet) {
//        MEMORY_BASIC_INFORMATION mbi;
//        mbi.RegionSize = 0x1000;
//        DWORD_PTR dwAddress = StartRange;
//        DWORD_PTR nSearchSize = _msize(SearchBytes);
//
//        std::vector<MEMORY_REGION> m_vMemoryRegion;
//
//        // Collect all memory regions
//        while (VirtualQueryEx(ProcessHandle, (LPCVOID)dwAddress, &mbi, sizeof(mbi)) && (dwAddress < EndRange) && ((dwAddress + mbi.RegionSize) > dwAddress)) {
//            if ((mbi.State == MEM_COMMIT) && ((mbi.Protect & PAGE_GUARD) == 0) && (mbi.Protect != PAGE_NOACCESS) && ((mbi.AllocationProtect & PAGE_NOCACHE) != PAGE_NOCACHE)) {
//                MEMORY_REGION mData = { (DWORD_PTR)mbi.BaseAddress, mbi.RegionSize };
//                m_vMemoryRegion.push_back(mData);
//            }
//            dwAddress = (DWORD_PTR)mbi.BaseAddress + mbi.RegionSize;
//        }
//
//        std::mutex mtx;
//
//        auto processRegion = [&](MEMORY_REGION mData) {
//            BYTE* pCurrMemoryData = new BYTE[mData.dwMemorySize];
//            ZeroMemory(pCurrMemoryData, mData.dwMemorySize);
//            DWORD_PTR dwNumberOfBytesRead = 0;
//
//            // Read process memory
//            ReadProcessMemory(ProcessHandle, (LPCVOID)mData.dwBaseAddr, pCurrMemoryData, mData.dwMemorySize, &dwNumberOfBytesRead);
//            if ((int)dwNumberOfBytesRead > 0) {
//                DWORD_PTR dwOffset = 0;
//                int iOffset = Memfind(pCurrMemoryData, dwNumberOfBytesRead, SearchBytes, nSearchSize);
//                while (iOffset != -1) {
//                    dwOffset += iOffset;
//                    DWORD_PTR firstByteAddress = dwOffset + mData.dwBaseAddr;
//
//                    std::lock_guard<std::mutex> lock(mtx);
//                    AddressRet.push_back(firstByteAddress);
//
//                    dwOffset += nSearchSize;
//                    iOffset = Memfind(pCurrMemoryData + dwOffset, dwNumberOfBytesRead - dwOffset - nSearchSize, SearchBytes, nSearchSize);
//                }
//            }
//
//            delete[] pCurrMemoryData;
//            };
//
//        // Launch threads to process memory regions concurrently
//        std::vector<std::future<void>> futures;
//        for (const auto& region : m_vMemoryRegion) {
//            futures.push_back(std::async(std::launch::async, processRegion, region));
//        }
//
//
//        for (auto& fut : futures)
//        {
//            fut.get();
//        }
//
//        return true;
//    }
//
//    int Memfind(BYTE* buffer, DWORD_PTR dwBufferSize, BYTE* bstr, DWORD_PTR dwStrLen) {
//        if (dwBufferSize < 0) {
//            return -1;
//        }
//        DWORD_PTR  i, j;
//        for (i = 0; i < dwBufferSize; i++) {
//            for (j = 0; j < dwStrLen; j++) {
//                if (buffer[i + j] != bstr[j] && bstr[j] != '?')
//                    break;
//
//            }
//            if (j == dwStrLen)
//                return i;
//        }
//        return -1;
//    }
//};
