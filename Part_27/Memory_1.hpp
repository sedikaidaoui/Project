#pragma once

#include <Windows.h>
#include <unordered_map>
#include <string>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>

#include <synchapi.h>

struct SRWLockGuard {
    SRWLOCK* lock;
    bool exclusive;
    SRWLockGuard(SRWLOCK* l, bool ex = true) : lock(l), exclusive(ex) {
        if (exclusive) AcquireSRWLockExclusive(lock);
        else AcquireSRWLockShared(lock);
    }
    ~SRWLockGuard() {
        if (exclusive) ReleaseSRWLockExclusive(lock);
        else ReleaseSRWLockShared(lock);
    }
};

class Mem {
public:
    inline static void* vmPtr = nullptr;
    inline static void* pVMAddr = nullptr;
    inline static void* cpuAddr = nullptr;

    inline static std::unordered_map<uintptr_t, uintptr_t> Cache;
    inline static SRWLOCK CacheLock = SRWLOCK_INIT;

    using PGMPhysReadFunc = int(__cdecl*)(void*, uintptr_t, void*, size_t);
    using VMMGetCpuByIdFunc = void* (__cdecl*)(void*, int);
    using PGMPhysGCPtr2GCPhysFunc = int(__cdecl*)(void*, uintptr_t, uintptr_t*);
    using PGMPhysSimpleWriteGCPhysFunc = int(__cdecl*)(void*, uintptr_t, void*, size_t);

    inline static PGMPhysReadFunc ogPhysRead = nullptr;
    inline static void* TargetPhysRead = nullptr; // Added back for MinHook
    inline static VMMGetCpuByIdFunc ogCPU = nullptr;
    inline static PGMPhysGCPtr2GCPhysFunc ogCast = nullptr;
    inline static PGMPhysSimpleWriteGCPhysFunc ogWrite = nullptr;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* buffer) {
        buffer->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    static int __cdecl HookedPGMPhysRead(void* pVM, uintptr_t GCPhys, void* pvBuf, size_t cbRead) {
        if (!vmPtr) {
            vmPtr = pVM;
            std::cout << "Memory initialized: " << vmPtr << std::endl;
        }
        return ogPhysRead(pVM, GCPhys, pvBuf, cbRead);
    }

    static int HookWrite(void* pVM, uintptr_t GCPhys, void* pvBuf, size_t cbRead) {
        if (!pVM || !ogWrite) return -1;
        __try {
            return ogWrite(pVM, GCPhys, pvBuf, cbRead);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return -1;
        }
    }

    static int HookRead(void* pVM, uintptr_t GCPhys, void* pvBuf, size_t cbRead) {
        if (!pVM || !ogPhysRead) return -1;
        __try {
            return ogPhysRead(pVM, GCPhys, pvBuf, cbRead);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return -1;
        }
    }

    static void* CPU(void* pVM, int cpuId) {
        if (!pVM || !ogCPU) return nullptr;
        __try {
            return ogCPU(pVM, cpuId);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
    }

    static int Cast(void* pVCpu, uintptr_t address, uintptr_t* physAddress) {
        if (!pVCpu || !ogCast) return -1;
        __try {
            return ogCast(pVCpu, address, physAddress);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return -1;
        }
    }

    static void Initialize(void* pVM) {
        pVMAddr = pVM;
        if (!pVM) return;
        uint32_t validCpus = 0;
        for (uint32_t i = 0; i < MAX_CPU; ++i) {
            void* cpu = CPU(pVM, i);
            cpuAddrs[i] = cpu;
            if (cpu != nullptr) {
                validCpus++;
            }
        }
        cpuAddr = cpuAddrs[0];
        {
            SRWLockGuard lock(&CacheLock, true);
            Cache.clear();
        }
    }

    static constexpr uint32_t MAX_CPU = 16U;
    static constexpr uintptr_t MIN_VALID_ADDRESS = 0x1000;

    inline static void* cpuAddrs[MAX_CPU] = {nullptr};

    static bool Convert(uintptr_t address, uintptr_t& phys) {
        phys = 0;
        if (!pVMAddr || address <= MIN_VALID_ADDRESS) return false;

        // Try cache first (Shared lock for reading)
        {
            SRWLockGuard lock(&CacheLock, false);
            auto it = Cache.find(address);
            if (it != Cache.end() && it->second != 0) {
                phys = it->second;
                return true;
            }
        }

        // Try translating across all active vCPUs
        for (uint32_t i = 0; i < MAX_CPU; ++i) {
            void* cpu = cpuAddrs[i];
            if (!cpu) continue;

            uintptr_t tempPhys = 0;
            if (Cast(cpu, address, &tempPhys) == 0 && tempPhys != 0) {
                phys = tempPhys;
                {
                    SRWLockGuard lock(&CacheLock, true);
                    Cache[address] = tempPhys;
                }
                return true;
            }
        }
        return false;
    }

    static bool ConvertDirect(uintptr_t address, uintptr_t& phys) {
        phys = 0;
        if (!pVMAddr || address <= MIN_VALID_ADDRESS) return false;

        // Try active vCPUs directly
        for (uint32_t i = 0; i < MAX_CPU; ++i) {
            void* cpu = cpuAddrs[i];
            if (!cpu) continue;

            uintptr_t tempPhys = 0;
            if (Cast(cpu, address, &tempPhys) == 0 && tempPhys != 0) {
                phys = tempPhys;
                return true;
            }
        }

        // Fallback to cached translation if direct translation failed due to temporary thread scheduling
        {
            SRWLockGuard lock(&CacheLock, false);
            auto it = Cache.find(address);
            if (it != Cache.end() && it->second != 0) {
                phys = it->second;
                return true;
            }
        }
        return false;
    }


    template<typename T>
    static bool ReadFast2(uintptr_t address, T* data) {
        uintptr_t physAddress;
        if (!ConvertDirect(address, physAddress)) return false;
        return HookRead(pVMAddr, physAddress, data, sizeof(T)) == 0;
    }

    // Value-returning Read (Old style compatibility)
    template<typename T>
    static T Read(uintptr_t address) {
        T result{};
        uintptr_t physAddress;
        if (Convert(address, physAddress)) {
            HookRead(pVMAddr, physAddress, &result, sizeof(T));
        }
        return result;
    }

    template<typename T>
    static T ReadS(uintptr_t address) { return Read<T>(address); }

    // Pointer-returning Read (Old style compatibility)
    template<typename T>
    static bool Read(uintptr_t address, T& data) {
        uintptr_t physAddress;
        if (!Convert(address, physAddress)) return false;
        return HookRead(pVMAddr, physAddress, &data, sizeof(T)) == 0;
    }

    template<typename T>
    static bool Write(uintptr_t address, const T& value) {
        uintptr_t physAddress;
        if (!Convert(address, physAddress)) return false;
        
        // Sử dụng HookWrite (gọi trực tiếp vào engine giả lập) 
        // để né tránh hoàn toàn việc gọi WinAPI (WriteProcessMemory)
        return HookWrite(pVMAddr, physAddress, (void*)&value, sizeof(T)) == 0;
    }

    // Một hàm Write an toàn hơn, kiểm tra giá trị cũ trước khi ghi (Stealth Write)
    template<typename T>
    static bool WriteStealth(uintptr_t address, const T& value) {
        T current{};
        if (Read(address, current)) {
            if (current == value) return true; // Giống nhau thì không ghi, né scan
        }
        return Write(address, value);
    }

    template<typename T>
    static bool ReadArray(uintptr_t address, std::vector<T>& array) {
        uintptr_t convertedAddress;
        if (!Convert(address, convertedAddress)) return false;

        size_t size = sizeof(T) * array.size();
        return HookRead(pVMAddr, convertedAddress, array.data(), size) == 0;
    }

    static std::string utf16_to_utf8(const std::wstring& wstr) {
        std::string str;
        int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (len > 0) {
            str.resize(len - 1);
            WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], len, nullptr, nullptr);
        }
        return str;
    }

    static std::string String(uintptr_t address, int size, bool unicode = true) {
        std::vector<byte> stringBytes(size);
        if (!ReadArray(address, stringBytes)) return "";

        std::string result;
        if (unicode) {
            std::wstring wstr(reinterpret_cast<wchar_t*>(stringBytes.data()), size / 2);
            result = utf16_to_utf8(wstr);
        }
        else {
            result = std::string(reinterpret_cast<char*>(stringBytes.data()), size);
        }

        size_t nullPos = result.find('\0');
        if (nullPos != std::string::npos) {
            result = result.substr(0, nullPos);
        }
        return result;
    }
};

inline Mem Mem;