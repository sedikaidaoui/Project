//#include <Windows.h>
//#include <vector>
//#include <string> 
//#include <iostream>
//#include <TlHelp32.h>
//#include <tchar.h>
//#include <atlbase.h>
//#include <map>
//
//
//
//
//
//class MemorySEG
//{
//
//public:
//
//	const char* GetEmulatorRunning() {
//		if (GetPid("HD-Player.exe") != 0)
//			return "HD-Player.exe";
//	}
//
//
//	void EnableAimHead1()
//	{
//
//		std::vector<DWORD_PTR> addressScan;
//		std::vector<BYTE> scanAimbot = { 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA5, 0x43 };
//
//        MemoryLogs = "Injecting - Aimbot Neck  !";
//        ImGui::Notification({ ImGuiToastType_Success, 4000, MemoryLogs.c_str() });
//
//		if (!AttackProcess(GetEmulatorRunning())) {
//
//			
//
//		}
//
//		SYSTEM_INFO si;
//		GetSystemInfo(&si);
//
//		DWORD_PTR startAddress = (DWORD_PTR)si.lpMinimumApplicationAddress;
//		DWORD_PTR endAddress = (DWORD_PTR)si.lpMaximumApplicationAddress;
//
//		addressScan.clear();
//
//		// Buscar patrones en memoria
//		FindPattern(startAddress, endAddress, scanAimbot.data(), addressScan);
//
//		if (addressScan.empty()) {
//
//            MemoryLogs = "ERROR - Aimbot Neck  !";
//			
//
//		}
//		else {
//			// Definir mapas para valores originales
//			std::map<DWORD_PTR, int> originalValues;
//			std::map<DWORD_PTR, int> originalValues2;
//
//			for (auto currentAddress : addressScan) {
//				DWORD_PTR writeOffset = 162L; // Offset de escritura
//				DWORD_PTR readOffset = 158L;  // Offset de lectura
//
//				DWORD_PTR addressToSave = currentAddress + writeOffset;
//				DWORD_PTR addressToSave9 = currentAddress + readOffset;
//
//				int buffer = 0;
//				int buffer9 = 0;
//
//				// Leer memoria y guardar valores originales
//				ReadProcessMemory(ProcessHandle, (LPCVOID)addressToSave, &buffer, sizeof(buffer), nullptr);
//				originalValues[addressToSave] = buffer;
//
//				ReadProcessMemory(ProcessHandle, (LPCVOID)addressToSave9, &buffer9, sizeof(buffer9), nullptr);
//				originalValues[addressToSave9] = buffer9;
//
//				// Leer cabeza y pecho
//				DWORD_PTR headBytes = currentAddress + readOffset;
//				DWORD_PTR chestBytes = currentAddress + writeOffset;
//
//				int headValue = 0;
//				int chestValue = 0;
//
//				ReadProcessMemory(ProcessHandle, (LPCVOID)headBytes, &headValue, sizeof(headValue), nullptr);
//				ReadProcessMemory(ProcessHandle, (LPCVOID)chestBytes, &chestValue, sizeof(chestValue), nullptr);
//
//				// Intercambiar valores
//				WriteProcessMemory(ProcessHandle, (LPVOID)chestBytes, &headValue, sizeof(headValue), nullptr);
//				WriteProcessMemory(ProcessHandle, (LPVOID)headBytes, &chestValue, sizeof(chestValue), nullptr);
//
//				// Leer memoria nuevamente y guardar en el segundo mapa
//				int buffer1 = 0;
//				int buffer19 = 0;
//
//				ReadProcessMemory(ProcessHandle, (LPCVOID)addressToSave, &buffer1, sizeof(buffer1), nullptr);
//				originalValues2[addressToSave] = buffer1;
//
//				ReadProcessMemory(ProcessHandle, (LPCVOID)addressToSave9, &buffer19, sizeof(buffer19), nullptr);
//				originalValues2[addressToSave9] = buffer19;
//			}
//
//            MemoryLogs = "Injected - Aimbot Neck !";
//            ImGui::Notification({ ImGuiToastType_Success, 4000, MemoryLogs.c_str() });
//			
//		
//
//		}
//
//		CloseHandle(ProcessHandle);
//
//	}
//
//	
//	void EnableAimHead3()
//	{
//		std::vector<DWORD_PTR> addressScan;
//		std::vector<BYTE> scanAimbot = { 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA5, 0x43 };
//
//        MemoryLogs = "Injecting - Aimbot Drag  !";
//        //ImGui::Notification({ ImGuiToastType_Success, 4000, MemoryLogs.c_str() });
//
//		if (!AttackProcess(GetEmulatorRunning())) {
//			
//		}
//
//		SYSTEM_INFO si;
//		GetSystemInfo(&si);
//
//		DWORD_PTR startAddress = (DWORD_PTR)si.lpMinimumApplicationAddress;
//		DWORD_PTR endAddress = (DWORD_PTR)si.lpMaximumApplicationAddress;
//
//		addressScan.clear();
//
//		// Buscar patrones en memoria
//		FindPattern(startAddress, endAddress, scanAimbot.data(), addressScan);
//
//		if (addressScan.empty()) {
//
//            MemoryLogs = "ERROR - Aimbot Drag  !";
//		}
//		else {
//			// Definir mapas para valores originales
//			std::map<DWORD_PTR, int> originalValues;
//			std::map<DWORD_PTR, int> originalValues2;
//
//			for (auto currentAddress : addressScan) {
//				DWORD_PTR writeOffset = 164L; // Offset de escritura
//				DWORD_PTR readOffset = 160L;  // Offset de lectura
//
//				DWORD_PTR addressToSave = currentAddress + writeOffset;
//				DWORD_PTR addressToSave9 = currentAddress + readOffset;
//
//				int buffer = 0;
//				int buffer9 = 0;
//
//				// Leer memoria y guardar valores originales
//				ReadProcessMemory(ProcessHandle, (LPCVOID)addressToSave, &buffer, sizeof(buffer), nullptr);
//				originalValues[addressToSave] = buffer;
//
//				ReadProcessMemory(ProcessHandle, (LPCVOID)addressToSave9, &buffer9, sizeof(buffer9), nullptr);
//				originalValues[addressToSave9] = buffer9;
//
//				// Leer cabeza y pecho
//				DWORD_PTR headBytes = currentAddress + readOffset;
//				DWORD_PTR chestBytes = currentAddress + writeOffset;
//
//				int headValue = 0;
//				int chestValue = 0;
//
//				ReadProcessMemory(ProcessHandle, (LPCVOID)headBytes, &headValue, sizeof(headValue), nullptr);
//				ReadProcessMemory(ProcessHandle, (LPCVOID)chestBytes, &chestValue, sizeof(chestValue), nullptr);
//
//				// Intercambiar valores
//				WriteProcessMemory(ProcessHandle, (LPVOID)chestBytes, &headValue, sizeof(headValue), nullptr);
//				WriteProcessMemory(ProcessHandle, (LPVOID)headBytes, &chestValue, sizeof(chestValue), nullptr);
//
//				// Leer memoria nuevamente y guardar en el segundo mapa
//				int buffer1 = 0;
//				int buffer19 = 0;
//
//				ReadProcessMemory(ProcessHandle, (LPCVOID)addressToSave, &buffer1, sizeof(buffer1), nullptr);
//				originalValues2[addressToSave] = buffer1;
//
//				ReadProcessMemory(ProcessHandle, (LPCVOID)addressToSave9, &buffer19, sizeof(buffer19), nullptr);
//				originalValues2[addressToSave9] = buffer19;
//			}
//
//            MemoryLogs = "Injected - Aimbot Drag !";
//            ImGui::Notification({ ImGuiToastType_Success, 4000, MemoryLogs.c_str() });
//			
//
//		}
//
//		CloseHandle(ProcessHandle);
//	}
//
//	
//
//
//		
//
//
//	std::vector<DWORD_PTR> foundAddresses;
//
//	DWORD ProcessId = 0;
//	HANDLE ProcessHandle;
//
//	typedef struct _MEMORY_REGION {
//		DWORD_PTR dwBaseAddr;
//		DWORD_PTR dwMemorySize;
//	}MEMORY_REGION;
//
//	static int GetPid(const char* procname) {
//
//		if (procname == NULL)
//			return 0;
//		DWORD pid = 0;
//		DWORD threadCount = 0;
//
//		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
//		PROCESSENTRY32 pe;
//
//		pe.dwSize = sizeof(PROCESSENTRY32);
//		Process32First(hSnap, &pe);
//		while (Process32Next(hSnap, &pe)) {
//			if (_tcsicmp(pe.szExeFile, procname) == 0) {
//				if ((int)pe.cntThreads > threadCount) {
//					threadCount = pe.cntThreads;
//
//					pid = pe.th32ProcessID;
//
//				}
//			}
//		}
//		return pid;
//	}
//
//	static int findMyProc(const char* procname) {
//		if (procname == NULL)
//			return 0;
//		DWORD pid = 0;
//		DWORD threadCount = 0;
//
//		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
//		PROCESSENTRY32 pe;
//
//		pe.dwSize = sizeof(PROCESSENTRY32);
//		Process32First(hSnap, &pe);
//		while (Process32Next(hSnap, &pe)) {
//			if (_tcsicmp(pe.szExeFile, procname) == 0) {
//				if ((int)pe.cntThreads > threadCount) {
//					threadCount = pe.cntThreads;
//
//					pid = pe.th32ProcessID;
//
//				}
//			}
//		}
//		return pid;
//	}
//
//	BOOL AttackProcess(const char* procname) {
//		DWORD ProcId = GetPid(procname);
//		if (ProcId == 0)
//			return false;
//
//		ProcessId = ProcId;
//		ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, 0, ProcessId);
//		return ProcessHandle != nullptr;
//	}
//
//	bool ChangeProtection(ULONG Address, size_t size, DWORD NewProtect, DWORD& OldProtect)
//	{
//		return VirtualProtectEx(ProcessHandle, (LPVOID)Address, size, NewProtect, &OldProtect);;
//	}
//
//	bool ReplacePattern(DWORD_PTR dwStartRange, DWORD_PTR dwEndRange, BYTE* SearchAob, BYTE* ReplaceAob, bool ForceWrite = false)
//	{
//		int RepByteSize = _msize(ReplaceAob);
//		if (RepByteSize <= 0) return false;
//		std::vector<DWORD_PTR> foundedAddress;
//		FindPattern(dwStartRange, dwEndRange, SearchAob, foundedAddress);
//		if (foundedAddress.empty())
//			return false;
//
//		OutputDebugStringA(std::to_string(foundedAddress.size()).c_str());
//
//		DWORD OldProtect;
//		for (int i = 0; i < foundedAddress.size(); i++)
//		{
//			ChangeProtection(foundedAddress[i], RepByteSize, PAGE_EXECUTE_READWRITE, OldProtect);
//			WriteProcessMemory(ProcessHandle, (LPVOID)foundedAddress[i], ReplaceAob, RepByteSize, 0);
//		}
//
//		return true;
//	}
//
//	bool FindPattern(DWORD_PTR StartRange, DWORD_PTR EndRange, BYTE* SearchBytes, std::vector<DWORD_PTR>& AddressRet)
//	{
//
//		BYTE* pCurrMemoryData = NULL;
//		MEMORY_BASIC_INFORMATION	mbi;
//		std::vector<MEMORY_REGION> m_vMemoryRegion;
//		mbi.RegionSize = 0x1000;
//		DWORD_PTR dwAddress = StartRange;
//		DWORD_PTR nSearchSize = _msize(SearchBytes);
//
//
//		while (VirtualQueryEx(ProcessHandle, (LPCVOID)dwAddress, &mbi, sizeof(mbi)) && (dwAddress < EndRange) && ((dwAddress + mbi.RegionSize) > dwAddress))
//		{
//
//			if ((mbi.State == MEM_COMMIT) && ((mbi.Protect & PAGE_GUARD) == 0) && (mbi.Protect != PAGE_NOACCESS) && ((mbi.AllocationProtect & PAGE_NOCACHE) != PAGE_NOCACHE))
//			{
//
//				MEMORY_REGION mData = { 0 };
//				mData.dwBaseAddr = (DWORD_PTR)mbi.BaseAddress;
//				mData.dwMemorySize = mbi.RegionSize;
//				m_vMemoryRegion.push_back(mData);
//
//			}
//			dwAddress = (DWORD_PTR)mbi.BaseAddress + mbi.RegionSize;
//
//		}
//
//		std::vector<MEMORY_REGION>::iterator it;
//		for (it = m_vMemoryRegion.begin(); it != m_vMemoryRegion.end(); it++)
//		{
//			MEMORY_REGION mData = *it;
//
//
//			DWORD_PTR dwNumberOfBytesRead = 0;
//			pCurrMemoryData = new BYTE[mData.dwMemorySize];
//			ZeroMemory(pCurrMemoryData, mData.dwMemorySize);
//			ReadProcessMemory(ProcessHandle, (LPCVOID)mData.dwBaseAddr, pCurrMemoryData, mData.dwMemorySize, &dwNumberOfBytesRead);
//			if ((int)dwNumberOfBytesRead <= 0)
//			{
//				delete[] pCurrMemoryData;
//				continue;
//			}
//			DWORD_PTR dwOffset = 0;
//			int iOffset = Memfind(pCurrMemoryData, dwNumberOfBytesRead, SearchBytes, nSearchSize);
//			while (iOffset != -1)
//			{
//				dwOffset += iOffset;
//				AddressRet.push_back(dwOffset + mData.dwBaseAddr);
//				dwOffset += nSearchSize;
//				iOffset = Memfind(pCurrMemoryData + dwOffset, dwNumberOfBytesRead - dwOffset - nSearchSize, SearchBytes, nSearchSize);
//			}
//
//			if (pCurrMemoryData != NULL)
//			{
//				delete[] pCurrMemoryData;
//				pCurrMemoryData = NULL;
//			}
//
//		}
//		return TRUE;
//	}
//
//	int Memfind(BYTE* buffer, DWORD_PTR dwBufferSize, BYTE* bstr, DWORD_PTR dwStrLen) {
//		if (dwBufferSize < 0) {
//			return -1;
//		}
//		DWORD_PTR  i, j;
//		for (i = 0; i < dwBufferSize; i++) {
//			for (j = 0; j < dwStrLen; j++) {
//				if (buffer[i + j] != bstr[j] && bstr[j] != '?')
//					break;
//
//			}
//			if (j == dwStrLen)
//				return i;
//		}
//		return -1;
//	}
//
//};
