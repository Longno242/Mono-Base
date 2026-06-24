#pragma once

#include <MinHook.h>

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "mono_base/core/logger.hpp"

namespace mono_base::hooks {

class HookManager {
public:
    static HookManager& instance();

    bool init();
    void shutdown();

    template<typename Fn>
    bool create(const char* name, void* target, Fn detour, Fn* original) {
        return create_raw(name, target, reinterpret_cast<void*>(detour), reinterpret_cast<void**>(original));
    }

    bool create_raw(const char* name, void* target, void* detour, void** original);
    bool enable(const char* name);
    bool disable(const char* name);
    bool remove(const char* name);
    bool enable_all();
    bool disable_all();

private:
    HookManager() = default;

    struct Entry {
        std::string name;
        void* target = nullptr;
        bool enabled = false;
    };

    std::mutex mutex_;
    bool initialized_ = false;
    std::unordered_map<std::string, Entry> entries_;
};

inline HookManager& HookManager::instance() {
    static HookManager mgr;
    return mgr;
}

inline bool HookManager::init() {
    std::lock_guard lock(mutex_);
    if (initialized_) {
        return true;
    }

    const MH_STATUS status = MH_Initialize();
    if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED) {
        LOG_ERROR("Hooks", "MH_Initialize failed: %s", MH_StatusToString(status));
        return false;
    }

    initialized_ = true;
    LOG_INFO("Hooks", "MinHook initialized");
    return true;
}

inline void HookManager::shutdown() {
    std::lock_guard lock(mutex_);

    if (!initialized_) {
        return;
    }

    disable_all();
    for (const auto& [_, entry] : entries_) {
        MH_RemoveHook(entry.target);
    }
    entries_.clear();
    MH_Uninitialize();
    initialized_ = false;
}

inline bool HookManager::create_raw(const char* name, void* target, void* detour, void** original) {
    if (!target || !detour || !original) {
        LOG_ERROR("Hooks", "Invalid hook args for '%s'", name ? name : "?");
        return false;
    }

    std::lock_guard lock(mutex_);
    if (!initialized_) {
        LOG_ERROR("Hooks", "HookManager not initialized");
        return false;
    }

    if (entries_.contains(name)) {
        LOG_WARN("Hooks", "Hook '%s' already exists", name);
        return false;
    }

    const MH_STATUS status = MH_CreateHook(target, detour, original);
    if (status != MH_OK) {
        LOG_ERROR("Hooks", "MH_CreateHook('%s') failed: %s", name, MH_StatusToString(status));
        return false;
    }

    entries_[name] = Entry{ name, target, false };
    LOG_DEBUG("Hooks", "Created hook '%s' @ %p", name, target);
    return true;
}

inline bool HookManager::enable(const char* name) {
    std::lock_guard lock(mutex_);
    auto it = entries_.find(name);
    if (it == entries_.end()) {
        LOG_ERROR("Hooks", "Hook '%s' not found", name);
        return false;
    }
    if (it->second.enabled) {
        return true;
    }

    const MH_STATUS status = MH_EnableHook(it->second.target);
    if (status != MH_OK) {
        LOG_ERROR("Hooks", "MH_EnableHook('%s') failed: %s", name, MH_StatusToString(status));
        return false;
    }

    it->second.enabled = true;
    LOG_INFO("Hooks", "Enabled '%s'", name);
    return true;
}

inline bool HookManager::disable(const char* name) {
    std::lock_guard lock(mutex_);
    auto it = entries_.find(name);
    if (it == entries_.end()) {
        return false;
    }
    if (!it->second.enabled) {
        return true;
    }

    MH_DisableHook(it->second.target);
    it->second.enabled = false;
    return true;
}

inline bool HookManager::remove(const char* name) {
    std::lock_guard lock(mutex_);
    auto it = entries_.find(name);
    if (it == entries_.end()) {
        return false;
    }
    disable(name);
    MH_RemoveHook(it->second.target);
    entries_.erase(it);
    return true;
}

inline bool HookManager::enable_all() {
    bool ok = true;
    std::vector<std::string> names;
    {
        std::lock_guard lock(mutex_);
        for (const auto& [name, _] : entries_) {
            names.push_back(name);
        }
    }
    for (const auto& name : names) {
        ok = enable(name.c_str()) && ok;
    }
    return ok;
}

inline bool HookManager::disable_all() {
    bool ok = true;
    std::vector<std::string> names;
    {
        std::lock_guard lock(mutex_);
        for (const auto& [name, entry] : entries_) {
            if (entry.enabled) {
                names.push_back(name);
            }
        }
    }
    for (const auto& name : names) {
        ok = disable(name.c_str()) && ok;
    }
    return ok;
}

} // namespace mono_base::hooks
