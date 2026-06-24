#pragma once

#include <algorithm>
#include <cstdio>

#include "mono_base/mono/resolver/detail/api.hpp"
#include "mono_base/mono/resolver/types.hpp"

namespace mono_base::mono {

inline std::uint64_t hash_class_key(const char* namespaze, const char* name) {
    std::uint64_t hash = 0x14650FB0739D0383ULL;
    while (namespaze && *namespaze) {
        hash ^= static_cast<std::uint8_t>(*namespaze++);
        hash *= 0x100000001B3ULL;
    }
    hash ^= ':';
    hash *= 0x100000001B3ULL;
    while (name && *name) {
        hash ^= static_cast<std::uint8_t>(*name++);
        hash *= 0x100000001B3ULL;
    }
    return hash;
}

// ---------------------------------------------------------------------------
// Field
// ---------------------------------------------------------------------------
inline Field::Field(std::string name, std::string type, int offset, MonoField* field)
    : name_(std::move(name)), type_(std::move(type)), offset_(offset), field_(field) {}

inline std::string Field::describe() const {
    char buf[96]{};
    snprintf(buf, sizeof(buf), "%s %s (+0x%X)", type_.c_str(), name_.c_str(), offset_);
    return buf;
}

// ---------------------------------------------------------------------------
// Method
// ---------------------------------------------------------------------------
inline Method::Method(std::string name, std::string return_type, int arg_count,
                      void* native, MonoMethod* method)
    : name_(std::move(name))
    , return_type_(std::move(return_type))
    , arg_count_(arg_count)
    , native_(native)
    , method_(method) {}

inline MonoObject* Method::invoke(void* instance, void** params) const {
    return detail::ctx().api.runtime_invoke(method_, instance, params, nullptr);
}

inline std::string Method::describe() const {
    char buf[128]{};
    snprintf(buf, sizeof(buf), "%s -> %s (%d params) @ %p",
        name_.c_str(), return_type_.c_str(), arg_count_, native_);
    return buf;
}

// ---------------------------------------------------------------------------
// Class
// ---------------------------------------------------------------------------
inline Class::Class(std::string name, std::string namespaze, MonoClass* klass)
    : name_(std::move(name)), namespaze_(std::move(namespaze)), klass_(klass) {}

inline std::string Class::type_name(MonoType* type) {
    if (!type) {
        return "unknown";
    }
    char* name = detail::ctx().api.type_get_name(type);
    if (!name) {
        return "unknown";
    }
    std::string result = name;
    return result;
}

inline Field* Class::field(const std::string& name) {
    if (const auto it = field_cache_.find(name); it != field_cache_.end()) {
        return it->second;
    }

    auto& api = detail::ctx().api;
    MonoField* raw = api.class_get_field_from_name(klass_, name.c_str());
    if (!raw) {
        return nullptr;
    }

    const int offset = api.field_get_offset(raw);
    auto owned = std::make_unique<Field>(name, type_name(api.field_get_type(raw)), offset, raw);
    auto* ptr = owned.get();
    field_cache_[name] = ptr;
    owned_fields_.push_back(std::move(owned));
    return ptr;
}

inline const std::vector<Field*>& Class::fields() {
    if (!fields_ready_) {
        auto& api = detail::ctx().api;
        void* iter = nullptr;

        while (MonoField* raw = static_cast<MonoField*>(api.class_get_fields(klass_, &iter))) {
            const char* fname = api.field_get_name(raw);
            if (!fname || field_cache_.contains(fname)) {
                continue;
            }

            auto owned = std::make_unique<Field>(
                fname, type_name(api.field_get_type(raw)), api.field_get_offset(raw), raw);
            field_cache_[fname] = owned.get();
            owned_fields_.push_back(std::move(owned));
        }

        field_ptrs_.clear();
        for (auto& f : owned_fields_) {
            field_ptrs_.push_back(f.get());
        }
        fields_ready_ = true;
    }
    return field_ptrs_;
}

inline Method* Class::method(const std::string& name, int arg_count) {
    auto& api = detail::ctx().api;
    MonoMethod* raw = api.class_get_method_from_name(klass_, name.c_str(), arg_count);
    if (!raw) {
        return nullptr;
    }

    for (Method* existing : method_ptrs_) {
        if (existing->raw() == raw) {
            return existing;
        }
    }

    void* native = api.compile_method(raw);
    auto owned = std::make_unique<Method>(name, "unknown", arg_count, native, raw);
    auto* ptr = owned.get();
    owned_methods_.push_back(std::move(owned));
    method_ptrs_.push_back(ptr);
    return ptr;
}

inline const std::vector<Method*>& Class::methods() {
    if (!methods_ready_) {
        auto& api = detail::ctx().api;
        void* iter = nullptr;

        while (MonoMethod* raw = static_cast<MonoMethod*>(api.class_get_methods(klass_, &iter))) {
            if (std::any_of(method_ptrs_.begin(), method_ptrs_.end(),
                    [raw](Method* m) { return m->raw() == raw; })) {
                continue;
            }

            const char* mname = api.method_get_name(raw);
            if (!mname) {
                continue;
            }

            void* native = api.compile_method(raw);
            auto owned = std::make_unique<Method>(mname, "unknown", -1, native, raw);
            method_ptrs_.push_back(owned.get());
            owned_methods_.push_back(std::move(owned));
        }

        methods_ready_ = true;
    }
    return method_ptrs_;
}

inline std::string Class::describe() const {
    if (namespaze_.empty()) {
        return name_;
    }
    return namespaze_ + "." + name_;
}

// ---------------------------------------------------------------------------
// Image
// ---------------------------------------------------------------------------
inline Image::Image(std::string name, MonoImage* image)
    : name_(std::move(name)), image_(image) {}

inline Class* Image::klass(const std::string& name, const std::string& namespaze) {
    const std::uint64_t key = hash_class_key(namespaze.c_str(), name.c_str());
    if (const auto it = class_cache_.find(key); it != class_cache_.end()) {
        return it->second;
    }

    MonoClass* raw = detail::ctx().api.class_from_name(image_, namespaze.c_str(), name.c_str());
    if (!raw) {
        return nullptr;
    }

    auto owned = std::make_unique<Class>(name, namespaze, raw);
    auto* ptr = owned.get();
    class_cache_[key] = ptr;
    owned_classes_.push_back(std::move(owned));
    class_ptrs_.push_back(ptr);
    return ptr;
}

inline const std::vector<Class*>& Image::classes() {
  // Mono doesn't expose a simple image->all classes iterator in the embedding API
  // without walking metadata tables. Use klass() lookups instead.
    return class_ptrs_;
}

} // namespace mono_base::mono
