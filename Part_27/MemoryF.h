#pragma once
#include <Windows.h>
#include <vector>
#include <string> 
#include <iostream>
#include <TlHelp32.h>
#include <unordered_map>




BYTE* ConvertStringToByte(const std::string& str);
BYTE* STB(const std::string& hexString);
class Memory
{



public:
    DWORD ProcessId = 0;
    HANDLE ProcessHandle;

    typedef struct _MEMORY_REGION {
        DWORD_PTR dwBaseAddr;
        DWORD_PTR dwMemorySize;
    } MEMORY_REGION;

    BOOL AttachProcess(DWORD ProcId)
    {
        if (ProcId == 0)
            return false;
        ProcessId = ProcId;
        ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, 0, ProcessId);
        return ProcessHandle != nullptr;
    }



    bool ChangeProtection(ULONG Address, size_t size, DWORD NewProtect, DWORD& OldProtect)
    {
        return VirtualProtectEx(ProcessHandle, (LPVOID)Address, size, NewProtect, &OldProtect);
    }

    const char* GetEmulatorRunning() {
        if (GetPid1("HD-Player.exe") != 0)
            return "HD-Player.exe";

        else if (GetPid1("HD-Player") != 0)
            return "HD-Player";

        else if (GetPid1("MEmuHeadless.exe") != 0)
            return "MEmuHeadless.exe";

        else if (GetPid1("LdVBoxHeadless.exe") != 0)
            return "LdVBoxHeadless.exe";

        else if (GetPid1("AndroidProcess.exe") != 0)
            return "AndroidProcess.exe";

        else if (GetPid1("Nox.exe") != 0)
            return "Nox.exe";
    }

    int GetPid1(const char* procname) {

        if (procname == NULL)
            return 0;
        DWORD pid = 0;
        DWORD threadCount = 0;

        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32 pe;

        pe.dwSize = sizeof(PROCESSENTRY32);
        Process32First(hSnap, &pe);
        while (Process32Next(hSnap, &pe)) {
            if (_tcsicmp(pe.szExeFile, procname) == 0) {
                if ((int)pe.cntThreads > threadCount) {
                    threadCount = pe.cntThreads;

                    pid = pe.th32ProcessID;

                }
            }
        }
        return pid;

    }
    BOOL AttackProcess(const char* procname)
    {
        DWORD ProcId = GetPid1(procname);
        if (ProcId == 0)
            return false;

        ProcessId = ProcId;
        ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, 0, ProcessId);
        return ProcessHandle != nullptr;
    }



    typedef unsigned char BYTE;
    bool AA = true;
    bool AA2 = true;
    bool AA3 = true;
    bool AA4 = true;
    bool AA5 = true;
    bool AA6 = true;
    bool AA7 = true;
    bool AA9 = true;

    bool EnablesSX(std::string type, DWORD_PTR dwStartRange, DWORD_PTR dwEndRange, std::string Search, std::string Replace, std::string nameop) {
        char str[14] = "HD-Player.exe";
        DWORD procid = GetPid1(str);


        BYTE* SearchArrayByte = new BYTE[]{ 0x00 };
        BYTE* ReplaceArrayByte = new BYTE[]{ 0x00 };


        if (type == "Speed") {
            //wall hack
            nameop = "Speed";
            if (AA6) {
                AA6 = false;
                SearchArrayByte = new BYTE[]{ 0x02, 0x2B, 0x07, 0x3D, 0x02, 0x2B, 0x07, 0x3D, 0x02, 0x2B, 0x07, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x9B, 0x6C, 0xF2, 0x41 };
                ReplaceArrayByte = new BYTE[]{ 0xE3, 0xA5, 0x9B, 0x3C, 0xE3, 0xA5, 0x9B, 0x3C, 0x02, 0x2B, 0x07, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x9B, 0x6C, 0xF2, 0x41 };
              
               
                Beep(850, 450);

               


            }
            else {
                AA6 = true;
                SearchArrayByte = new BYTE[]{ 0xE3, 0xA5, 0x9B, 0x3C, 0xE3, 0xA5, 0x9B, 0x3C, 0x02, 0x2B, 0x07, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x9B, 0x6C, 0xF2, 0x41 };
                ReplaceArrayByte = new BYTE[]{ 0x02, 0x2B, 0x07, 0x3D, 0x02, 0x2B, 0x07, 0x3D, 0x02, 0x2B, 0x07, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x9B, 0x6C, 0xF2, 0x41 };
             
              
                Beep(400, 450);

                



            }
        }

        //SearchArrayByte = new BYTE[]{ 0x00, 0x00, 0x50, 0x40, 0x33, 0x33, 0xB3, 0x3F, 0x33, 0x33 };
        //ReplaceArrayByte = new BYTE[]{ 0x9a, 0x99, 0x99, 0x40, 0x33, 0x33, 0xb3, 0x3f, 0x33, 0x33 };

        if (type == "Wallhack") {
            //camara
            nameop = "Wallhack";
            if (AA4) {
                AA4 = false;
                SearchArrayByte = new BYTE[]{ 0x00, 0xAE, 0x47, 0x81, 0x3F, 0xAE, 0x47, 0x81, 0x3F, 0xAE, 0x47, 0x81, 0x3F, 0xAE };
                ReplaceArrayByte = new BYTE[]{ 0xC4, 0x1F, 0x00, 0x00, 0xC3, 0xAE, 0x48, 0x89,  0x3F, 0xAE, 0x47, 0x81, 0x3F, 0xAE };
           
             
                Beep(850, 450);

           

            }
            else {
                AA4 = true;
                SearchArrayByte = new BYTE[]{ 0xC4, 0x1F, 0x00, 0x00, 0xC3, 0xAE, 0x48, 0x89,  0x3F, 0xAE, 0x47, 0x81, 0x3F, 0xAE };
                ReplaceArrayByte = new BYTE[]{ 0x00, 0xAE, 0x47, 0x81, 0x3F, 0xAE, 0x47, 0x81, 0x3F, 0xAE, 0x47, 0x81, 0x3F, 0xAE };
          
              
                Beep(400, 450);

              



            }
        }


        else if (type == "SP") {
            //Mode Ghost
            nameop = "SP";
            if (AA9) {
                AA9 = false;
                SearchArrayByte = new BYTE[]{ 0x00, 0x00, 0x7A, 0x44, '?', '?', '?', '?', 0xF0, 0x4F, 0x2D, 0xE9, 0x1C, 0xB0, 0x8D, 0xE2, 0xA4, 0xD0, 0x4D, 0xE2, 0x00, 0x60, 0xA0, 0xE1, 0x6C, 0x03, 0x9F, 0xE5, 0x02, 0x70, 0xA0, 0xE1, 0x01, 0x40, 0xA0, 0xE1, 0x00, 0x00, 0x8F, 0xE0, 0x00, 0x00, 0xD0, 0xE5, 0x00, 0x00, 0x50, 0xE3, 0x06, 0x00, 0x00, 0x1A, 0x54, 0x03, 0x9F, 0xE5, 0x00, 0x00, 0x9F, 0xE7, 0x00, 0x00, 0x90, 0xE5, '?', '?', '?', '?', 0x48, 0x03, 0x9F, 0xE5, '?', '?', '?', '?' };
                ReplaceArrayByte = new BYTE[]{ 0x00, 0x00, 0x00, 0x00 };
                Beep(400, 450);

            }
            else {
                AA9 = true;

                SearchArrayByte = new BYTE[]{ 0x00, 0x00, 0x00, 0x00 };
                ReplaceArrayByte = new BYTE[]{ 0x00, 0x00, 0x7A, 0x44, '?', '?', '?', '?', 0xF0, 0x4F, 0x2D, 0xE9, 0x1C, 0xB0, 0x8D, 0xE2, 0xA4, 0xD0, 0x4D, 0xE2, 0x00, 0x60, 0xA0, 0xE1, 0x6C, 0x03, 0x9F, 0xE5, 0x02, 0x70, 0xA0, 0xE1, 0x01, 0x40, 0xA0, 0xE1, 0x00, 0x00, 0x8F, 0xE0, 0x00, 0x00, 0xD0, 0xE5, 0x00, 0x00, 0x50, 0xE3, 0x06, 0x00, 0x00, 0x1A, 0x54, 0x03, 0x9F, 0xE5, 0x00, 0x00, 0x9F, 0xE7, 0x00, 0x00, 0x90, 0xE5, '?', '?', '?', '?', 0x48, 0x03, 0x9F, 0xE5, '?', '?', '?', '?' };
                Beep(850, 450);
            }
        }


        else if (type == "FW") {
            //Fly Wall
            nameop = "FW";
            if (AA5) {
                AA5 = false;
                SearchArrayByte = new BYTE[]{ 0x3F, 0xAE, 0x47, 0x81, 0x3F, 0x00, 0x1A, 0xB7, 0xEE, 0xDC, 0x3A, 0x9F, 0xED, 0x30 };
                ReplaceArrayByte = new BYTE[]{ 0xF1, 0xAE, 0x47, 0x81, 0x3F, 0x00, 0x1A, 0xB7, 0xEE, 0xDC, 0x3A, 0x9F, 0xED, 0x30 };
                
              
                Beep(400, 450);

            }
            else {
                AA5 = true;

                SearchArrayByte = new BYTE[]{ 0xF1, 0xAE, 0x47, 0x81, 0x3F, 0x00, 0x1A, 0xB7, 0xEE, 0xDC, 0x3A, 0x9F, 0xED, 0x30 };
                ReplaceArrayByte = new BYTE[]{ 0x3F, 0xAE, 0x47, 0x81, 0x3F, 0x00, 0x1A, 0xB7, 0xEE, 0xDC, 0x3A, 0x9F, 0xED, 0x30 };
              
               
                Beep(850, 450);
            }
        }


        else if (type == "SJ") {
            //Speed Jump
            nameop = "SJ";
            if (AA7) {
                AA7 = false;
                SearchArrayByte = new BYTE[]{ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F };
                ReplaceArrayByte = new BYTE[]{ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xDC, 0x3F, 0x00, 0x00, 0x99, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F };
                Beep(400, 450);

            }
            else {
                AA7 = true;

                SearchArrayByte = new BYTE[]{ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xDC, 0x3F, 0x00, 0x00, 0x99, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F };
                ReplaceArrayByte = new BYTE[]{ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F };

                Beep(850, 450);
            }
        }





      





        if (procid == 0)
        {

        }
        else {

        }

        //const auto& p = ImGui::GetWindowPos();
        float color_gradient = (sin(ImGui::GetTime() * 1.5f) + 1.0f) / 2.0f;


        Memory memory1;
        if (memory1.AttachProcess(procid)) {
            //  ImGui::TextColored(ImColor(color_gradient, color_gradient, color_gradient, 1.0f).Value, "                                                                                    Aplicando...", ImVec2(50 + p.x, 82 + p.y));
        }
        else {
            //  MessageBoxA(NULL, "encontro", NULL, NULL);
        }


        bool Status = memory1.ReplacePattern(dwStartRange, dwEndRange, SearchArrayByte, ReplaceArrayByte, true, nameop);
        if (Status)
        {


        }
        else
        {
        }
        CloseHandle(memory1.ProcessHandle);
        return true;
    }

    BYTE originalValue;

    std::vector<BYTE> originalValues;

    //std::ostringstream oss;



    bool ReplacePattern(DWORD_PTR dwStartRange, DWORD_PTR dwEndRange, BYTE* SearchAob, BYTE* ReplaceAob, bool ForceWrite, const std::string& nameguardar)
    {
        BYTE newValue;
        bool CC = true;
        int RepByteSize = _msize(ReplaceAob);
        if (RepByteSize <= 0) return false;

        std::vector<DWORD_PTR> foundedAddress;
        std::vector<DWORD_PTR> foundedAddress2;
        if (savedAddresses.count(nameguardar) > 0) {
            foundedAddress = savedAddresses[nameguardar];
        }

        else {
            FindPattern(dwStartRange, dwEndRange, SearchAob, foundedAddress);
            savedAddresses[nameguardar] = foundedAddress;
        }




        originalValues.reserve(foundedAddress.size());

        for (DWORD_PTR address : foundedAddress) {

            if (ReadProcessMemory(ProcessHandle, (LPVOID)address, &originalValue, sizeof(BYTE), nullptr)) {
                originalValues.push_back(originalValue);
            }
            else {
                // Ocurrió un error al leer la memoria, manejar el error aquí si es necesario
            }
        }




        if (foundedAddress.empty())
            return false;

        DWORD OldProtect;
        for (int i = 0; i < foundedAddress.size(); i++)
        {
            DWORD_PTR addressValue = foundedAddress[i];
            int integerValue = static_cast<int>(addressValue);
            std::string stringValue = std::to_string(integerValue);


            ChangeProtection(foundedAddress[i], RepByteSize, PAGE_EXECUTE_READWRITE, OldProtect);
            WriteProcessMemory(ProcessHandle, (LPVOID)foundedAddress[i], ReplaceAob, RepByteSize, nullptr);
        }
        return true;
    }

    template <typename T>
    T ReadMemory(ULONG WriteAddress)
    {
        T pBuffer;
        ReadProcessMemory(ProcessHandle, (LPCVOID)WriteAddress, &pBuffer, sizeof(pBuffer), nullptr);
        return pBuffer;
    }

    template <typename T>
    bool WriteMemory(ULONG WriteAddress, T WriteValue, bool ForceWrite = false)
    {
        DWORD OldProtect;
        bool PStatus = false;
        if (ForceWrite)
        {
            PStatus = ChangeProtection(WriteAddress, sizeof(WriteValue), PAGE_EXECUTE_READWRITE, OldProtect);
        }
        bool status = WriteProcessMemory(ProcessHandle, (LPVOID)WriteAddress, &WriteValue, sizeof(WriteValue), nullptr);
        if (OldProtect != 0 && ForceWrite)
        {
            PStatus = ChangeProtection(WriteAddress, sizeof(WriteValue), OldProtect, OldProtect);
        }



        return PStatus && status;
    }

    bool WriteBytes(ULONG WriteAddress, BYTE* RepByte, bool ForceWrite = false) {
        DWORD OldProtect;
        int RepByteSize = _msize(RepByte);
        if (RepByteSize <= 0) return false;
        if (ForceWrite)
        {
            ChangeProtection(WriteAddress, RepByteSize, PAGE_EXECUTE_READWRITE, OldProtect);
        }
        bool status = WriteProcessMemory(ProcessHandle, (LPVOID)WriteAddress, RepByte, RepByteSize, nullptr);
        if (ForceWrite && OldProtect != 0)
        {
            ChangeProtection(WriteAddress, RepByteSize, OldProtect, OldProtect);
        }
        delete[] RepByte;
        return status;
    }

    bool FindPattern(DWORD_PTR StartRange, DWORD_PTR EndRange, BYTE* SearchBytes, std::vector<DWORD_PTR>& AddressRet)
    {
        const size_t SearchBytesSize = _msize(SearchBytes);
        const BYTE WildcardByte = '?';
        const size_t BlockSize = 0x1000;
        MEMORY_BASIC_INFORMATION mbi;
        DWORD_PTR dwAddress = StartRange;

        while (dwAddress < EndRange && VirtualQueryEx(ProcessHandle, (LPCVOID)dwAddress, &mbi, sizeof(mbi)) == sizeof(mbi))
        {
            if (mbi.State == MEM_COMMIT && (mbi.Protect & PAGE_GUARD) == 0 && mbi.Protect != PAGE_NOACCESS && mbi.Protect != PAGE_EXECUTE)
            {
                BYTE* pBuffer = new BYTE[mbi.RegionSize];
                DWORD_PTR dwBytesRead;
                if (ReadProcessMemory(ProcessHandle, mbi.BaseAddress, pBuffer, mbi.RegionSize, &dwBytesRead))
                {
                    BYTE* pMatch = pBuffer;
                    BYTE* pEnd = pBuffer + dwBytesRead - SearchBytesSize + 1;
                    DWORD_PTR BlockStart = (DWORD_PTR)pBuffer + (SearchBytesSize - 1);
                    DWORD_PTR BlockEnd = (DWORD_PTR)pBuffer + dwBytesRead - 1;
                    while (pMatch < pEnd)
                    {
                        bool Match = true;
                        for (size_t i = 0; i < SearchBytesSize; ++i)
                        {
                            if (SearchBytes[i] != WildcardByte && SearchBytes[i] != pMatch[i])
                            {
                                Match = false;
                                break;
                            }
                        }
                        if (Match)
                        {
                            AddressRet.push_back((DWORD_PTR)pMatch - (DWORD_PTR)pBuffer + (DWORD_PTR)mbi.BaseAddress);
                        }
                        ++pMatch;
                        if ((DWORD_PTR)pMatch >= BlockEnd)
                        {
                            BlockStart += BlockSize;
                            BlockEnd += BlockSize;
                            if (BlockStart < (DWORD_PTR)pBuffer + SearchBytesSize - 1)
                            {
                                pMatch = pBuffer + (SearchBytesSize - 1);
                            }
                            else
                            {
                                break;
                            }
                        }
                    }
                }
                delete[] pBuffer;
            }
            dwAddress = (DWORD_PTR)mbi.BaseAddress + mbi.RegionSize;
        }

        return !AddressRet.empty();
    }

private:
    static std::unordered_map<std::string, std::vector<DWORD_PTR>> savedAddresses;
};

std::unordered_map<std::string, std::vector<DWORD_PTR>> Memory::savedAddresses;

