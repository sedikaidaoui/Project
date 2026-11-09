#pragma once

#include "BurstAxisPaths.hpp"

#include <Windows.h>
#include <cstring>
#include <string>

namespace BurstAxis::Fs {
    inline bool CreateHiddenDirectory(const char* path) {
        if (!path || !*path)
            return false;

        std::string current;
        for (const char* p = path; *p; ++p) {
            current.push_back(*p);
            if (*p != '\\' && *p != '/')
                continue;

            if (current.size() > 3)
                CreateDirectoryA(current.c_str(), nullptr);
        }

        CreateDirectoryA(path, nullptr);
        DWORD attrs = GetFileAttributesA(path);
        if (attrs != INVALID_FILE_ATTRIBUTES)
            SetFileAttributesA(path, attrs | FILE_ATTRIBUTE_HIDDEN);

        return attrs != INVALID_FILE_ATTRIBUTES || GetLastError() == ERROR_ALREADY_EXISTS;
    }

    inline void SetHidden(const char* path, bool directory = false) {
        if (!path || !*path)
            return;

        DWORD attrs = GetFileAttributesA(path);
        if (attrs == INVALID_FILE_ATTRIBUTES)
            return;

        attrs |= FILE_ATTRIBUTE_HIDDEN;
        if (directory)
            attrs |= FILE_ATTRIBUTE_DIRECTORY;
        SetFileAttributesA(path, attrs);
    }

    inline void HideDirectoryTree(const char* path) {
        if (!path || !*path)
            return;

        SetHidden(path, true);

        WIN32_FIND_DATAA fd{};
        std::string search = std::string(path) + "\\*";
        HANDLE h = FindFirstFileA(search.c_str(), &fd);
        if (h == INVALID_HANDLE_VALUE)
            return;

        do {
            if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
                continue;

            std::string child = std::string(path) + "\\" + fd.cFileName;
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                HideDirectoryTree(child.c_str());
            else
                SetHidden(child.c_str());
        } while (FindNextFileA(h, &fd));

        FindClose(h);
    }

    inline void EnsureHiddenDataDirectories() {
        CreateHiddenDirectory(Paths::kPublicBase);
        CreateHiddenDirectory(Paths::kDllsDir);
        CreateHiddenDirectory(Paths::kNetworkDir);
    }

    inline void HideSensitiveFiles() {
        SetHidden(Paths::kStateFile);
        SetHidden(Paths::kKeyFile);
        SetHidden(Paths::kLegacyStateFile);
        SetHidden(Paths::kLegacyKeyFile);
    }

    inline void SecureBurstaxisLayout() {
        EnsureHiddenDataDirectories();
        HideDirectoryTree(Paths::kNetworkDir);
        HideSensitiveFiles();
    }
}
