#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

#include "mono_base/mono/resolver/detail/api.hpp"
#include "mono_base/mono/resolver/types.hpp"

namespace mono_base::mono {

// ---------------------------------------------------------------------------
// Memory — safe reads/writes for raw pointers and field offsets
// ---------------------------------------------------------------------------

template<typename T>
[[nodiscard]] inline bool mem_read(const void* address, T& out) {
    static_assert(std::is_trivially_copyable_v<T>);
    if (!address) {
        return false;
    }
    __try {
        std::memcpy(&out, address, sizeof(T));
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

template<typename T>
[[nodiscard]] inline bool mem_write(void* address, const T& value) {
    static_assert(std::is_trivially_copyable_v<T>);
    if (!address) {
        return false;
    }
    __try {
        std::memcpy(address, &value, sizeof(T));
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

[[nodiscard]] inline void* read_ptr(void* base, int offset) {
    void* value = nullptr;
    if (!base || offset <= 0 || !mem_read(static_cast<char*>(base) + offset, value)) {
        return nullptr;
    }
    return value;
}

[[nodiscard]] inline bool write_ptr(void* base, int offset, void* value) {
    if (!base || offset <= 0) {
        return false;
    }
    return mem_write(static_cast<char*>(base) + offset, value);
}

template<typename T>
[[nodiscard]] inline T read_at(void* base, int offset) {
    T value{};
    if (base && offset > 0) {
        mem_read(static_cast<char*>(base) + offset, value);
    }
    return value;
}

template<typename T>
inline bool write_at(void* base, int offset, const T& value) {
    if (!base || offset <= 0) {
        return false;
    }
    return mem_write(static_cast<char*>(base) + offset, value);
}

// ---------------------------------------------------------------------------
// Strings — MonoString <-> std::string
// ---------------------------------------------------------------------------

[[nodiscard]] inline std::string string_from_mono(MonoString* str) {
    if (!str) {
        return {};
    }
    auto& api = detail::ctx().api;
    if (!api.string_to_utf8) {
        return {};
    }
    char* utf8 = api.string_to_utf8(str);
    if (!utf8) {
        return {};
    }
    return utf8;
}

[[nodiscard]] inline MonoString* string_to_mono(const char* text) {
    if (!text) {
        return nullptr;
    }
    auto& api = detail::ctx().api;
    if (!api.string_new || !api.get_root_domain) {
        return nullptr;
    }
    return api.string_new(detail::ctx().domain, text);
}

[[nodiscard]] inline std::string field_get_string(const Field* field, void* instance = nullptr) {
    if (!field) {
        return {};
    }
    auto& api = detail::ctx().api;
    MonoString* str = nullptr;
    if (instance) {
        api.field_get_value(static_cast<MonoObject*>(instance), field->raw(), &str);
    } else {
        api.field_static_get_value(field->raw(), &str);
    }
    return string_from_mono(str);
}

inline void field_set_string(const Field* field, const std::string& value, void* instance = nullptr) {
    if (!field) {
        return;
    }
    MonoString* str = string_to_mono(value.c_str());
    if (!str) {
        return;
    }
    auto& api = detail::ctx().api;
    if (instance) {
        api.field_set_value(static_cast<MonoObject*>(instance), field->raw(), &str);
    } else {
        api.field_static_set_value(field->raw(), &str);
    }
}

// ---------------------------------------------------------------------------
// Boxing / unboxing — invoke return values and value types
// ---------------------------------------------------------------------------

template<typename T>
[[nodiscard]] inline T unbox(MonoObject* boxed) {
    static_assert(std::is_trivially_copyable_v<T>);
    if (!boxed) {
        return {};
    }
    void* data = detail::ctx().api.object_unbox(boxed);
    if (!data) {
        return {};
    }
    T value{};
    std::memcpy(&value, data, sizeof(T));
    return value;
}

[[nodiscard]] inline bool invoke_bool(const Method* method, void* instance = nullptr, void** params = nullptr) {
    return unbox<bool>(method ? method->invoke(instance, params) : nullptr);
}

[[nodiscard]] inline int invoke_int(const Method* method, void* instance = nullptr, void** params = nullptr) {
    return unbox<int>(method ? method->invoke(instance, params) : nullptr);
}

[[nodiscard]] inline float invoke_float(const Method* method, void* instance = nullptr, void** params = nullptr) {
    return unbox<float>(method ? method->invoke(instance, params) : nullptr);
}

[[nodiscard]] inline std::string invoke_string(const Method* method, void* instance = nullptr,
                                               void** params = nullptr) {
    if (!method) {
        return {};
    }
    return string_from_mono(reinterpret_cast<MonoString*>(method->invoke(instance, params)));
}

inline void invoke_void(const Method* method, void* instance = nullptr, void** params = nullptr) {
    if (method) {
        method->invoke(instance, params);
    }
}

// ---------------------------------------------------------------------------
// Static fields — direct slot pointers (singletons, static refs)
// ---------------------------------------------------------------------------

[[nodiscard]] inline void** static_field_slot(Class* klass, const Field* field) {
    if (!klass || !field) {
        return nullptr;
    }
    auto& api = detail::ctx().api;
    if (!api.class_vtable || !api.vtable_get_static_field_data) {
        return nullptr;
    }

    void* vtable = api.class_vtable(detail::ctx().domain, klass->raw());
    if (!vtable) {
        return nullptr;
    }

    void* static_data = api.vtable_get_static_field_data(vtable);
    if (!static_data) {
        return nullptr;
    }

    return reinterpret_cast<void**>(static_cast<char*>(static_data) + field->offset());
}

[[nodiscard]] inline void* static_object_ptr(Class* klass, const char* field_name) {
    if (!klass || !field_name) {
        return nullptr;
    }
    Field* field = klass->field(field_name);
    if (!field) {
        return nullptr;
    }
    void** slot = static_field_slot(klass, field);
    if (!slot) {
        return nullptr;
    }
    void* value = nullptr;
    if (!mem_read(static_cast<void*>(slot), value)) {
        return nullptr;
    }
    return value;
}

[[nodiscard]] inline void* static_object(Class* klass, const char* field_name) {
    return static_object_ptr(klass, field_name);
}

// ---------------------------------------------------------------------------
// Object info — class names, Unity object names, ToString
// ---------------------------------------------------------------------------

[[nodiscard]] inline std::string class_name(MonoClass* klass) {
    if (!klass) {
        return {};
    }
    auto& api = detail::ctx().api;
    const char* namespaze = api.class_get_namespace ? api.class_get_namespace(klass) : nullptr;
    const char* name = api.class_get_name ? api.class_get_name(klass) : nullptr;
    if (!name) {
        return {};
    }
    if (namespaze && *namespaze) {
        return std::string(namespaze) + "." + name;
    }
    return name;
}

[[nodiscard]] inline std::string object_class_name(void* object) {
    if (!object) {
        return {};
    }
    auto& api = detail::ctx().api;
    if (!api.object_get_class) {
        return {};
    }
    return class_name(api.object_get_class(static_cast<MonoObject*>(object)));
}

[[nodiscard]] inline std::string object_to_string(void* object) {
    if (!object) {
        return {};
    }
    auto& api = detail::ctx().api;
    if (api.object_to_string) {
        MonoString* str = api.object_to_string(static_cast<MonoObject*>(object), nullptr);
        if (str) {
            return string_from_mono(str);
        }
    }
  // Fallback: invoke System.Object.ToString()
    if (api.object_get_class && api.class_get_method_from_name && api.runtime_invoke) {
        MonoClass* klass = api.object_get_class(static_cast<MonoObject*>(object));
        MonoMethod* to_string = api.class_get_method_from_name(klass, "ToString", 0);
        if (to_string) {
            MonoObject* result = api.runtime_invoke(to_string, object, nullptr, nullptr);
            return string_from_mono(reinterpret_cast<MonoString*>(result));
        }
    }
    return {};
}

[[nodiscard]] inline std::string object_name(void* object) {
    if (!object) {
        return {};
    }

    auto& api = detail::ctx().api;
    if (!api.object_get_class) {
        return {};
    }

    MonoClass* raw_class = api.object_get_class(static_cast<MonoObject*>(object));

    auto try_field = [&](const char* field_name) -> std::string {
        if (!api.class_get_field_from_name || !api.field_get_value) {
            return {};
        }
        MonoField* raw_field = api.class_get_field_from_name(raw_class, field_name);
        if (!raw_field) {
            return {};
        }
        MonoString* str = nullptr;
        api.field_get_value(static_cast<MonoObject*>(object), raw_field, &str);
        return string_from_mono(str);
    };

    if (std::string name = try_field("m_Name"); !name.empty()) {
        return name;
    }
    if (std::string name = try_field("name"); !name.empty()) {
        return name;
    }

    if (api.class_get_method_from_name && api.runtime_invoke) {
        MonoMethod* getter = api.class_get_method_from_name(raw_class, "get_name", 0);
        if (getter) {
            MonoObject* result = api.runtime_invoke(getter, object, nullptr, nullptr);
            if (std::string name = string_from_mono(reinterpret_cast<MonoString*>(result)); !name.empty()) {
                return name;
            }
        }
    }

    if (std::string text = object_to_string(object); !text.empty()) {
        return text;
    }

    char buf[128]{};
    snprintf(buf, sizeof(buf), "%s@%p", class_name(raw_class).c_str(), object);
    return buf;
}

[[nodiscard]] inline std::string object_label(void* object) {
    if (!object) {
        return "null";
    }
    std::string name = object_name(object);
    if (!name.empty()) {
        return name;
    }
    return object_class_name(object);
}

// ---------------------------------------------------------------------------
// Lookup helpers
// ---------------------------------------------------------------------------

[[nodiscard]] inline std::vector<Method*> methods_named(Class* klass, const std::string& name) {
    std::vector<Method*> result;
    if (!klass) {
        return result;
    }
    for (Method* method : klass->methods()) {
        if (method->name() == name) {
            result.push_back(method);
        }
    }
    return result;
}

[[nodiscard]] inline Method* method_best_match(Class* klass, const std::string& name, int arg_count) {
    if (!klass) {
        return nullptr;
    }
    Method* exact = klass->method(name, arg_count);
    if (exact) {
        return exact;
    }
    for (Method* method : methods_named(klass, name)) {
        if (method->arg_count() == arg_count || method->arg_count() < 0) {
            return method;
        }
    }
    const auto matches = methods_named(klass, name);
    return matches.empty() ? nullptr : matches.front();
}

} // namespace mono_base::mono
