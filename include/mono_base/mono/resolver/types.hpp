#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

struct MonoClass;
struct MonoImage;
struct MonoField;
struct MonoMethod;
struct MonoType;
struct MonoObject;

#include "mono_base/mono/resolver/detail/api.hpp"

namespace mono_base::mono {

class Field;
class Method;

class Class {
public:
    Class(std::string name, std::string namespaze, MonoClass* klass);

    [[nodiscard]] const std::string& name() const { return name_; }
    [[nodiscard]] const std::string& namespaze() const { return namespaze_; }
    [[nodiscard]] MonoClass* raw() const { return klass_; }

    [[nodiscard]] Field* field(const std::string& name);
    [[nodiscard]] const std::vector<Field*>& fields();

    [[nodiscard]] Method* method(const std::string& name, int arg_count = -1);
    [[nodiscard]] const std::vector<Method*>& methods();
    [[nodiscard]] std::string describe() const;

private:
    [[nodiscard]] std::string type_name(MonoType* type);

    std::string name_;
    std::string namespaze_;
    MonoClass* klass_ = nullptr;

    std::vector<std::unique_ptr<Field>> owned_fields_;
    std::vector<Field*> field_ptrs_;
    std::unordered_map<std::string, Field*> field_cache_;

    std::vector<std::unique_ptr<Method>> owned_methods_;
    std::vector<Method*> method_ptrs_;

    bool fields_ready_ = false;
    bool methods_ready_ = false;
};

class Image {
public:
    Image(std::string name, MonoImage* image);

    [[nodiscard]] const std::string& name() const { return name_; }
    [[nodiscard]] MonoImage* raw() const { return image_; }

    [[nodiscard]] Class* klass(const std::string& name, const std::string& namespaze = "");
    [[nodiscard]] const std::vector<Class*>& classes();

private:
    std::string name_;
    MonoImage* image_ = nullptr;

    std::vector<std::unique_ptr<Class>> owned_classes_;
    std::vector<Class*> class_ptrs_;
    std::unordered_map<std::uint64_t, Class*> class_cache_;
    bool classes_ready_ = false;
};

class Field {
public:
    Field(std::string name, std::string type, int offset, MonoField* field);

    [[nodiscard]] const std::string& name() const { return name_; }
    [[nodiscard]] const std::string& type() const { return type_; }
    [[nodiscard]] int offset() const { return offset_; }
    [[nodiscard]] MonoField* raw() const { return field_; }

    template<typename T>
    T get_static() const {
        T value{};
        detail::ctx().api.field_static_get_value(field_, &value);
        return value;
    }

    template<typename T>
    void set_static(const T& value) const {
        T copy = value;
        detail::ctx().api.field_static_set_value(field_, &copy);
    }

    template<typename T>
    T get_instance(void* object) const {
        T value{};
        detail::ctx().api.field_get_value(static_cast<MonoObject*>(object), field_, &value);
        return value;
    }

    template<typename T>
    void set_instance(void* object, const T& value) const {
        T copy = value;
        detail::ctx().api.field_set_value(static_cast<MonoObject*>(object), field_, &copy);
    }

    [[nodiscard]] std::string describe() const;

private:
    std::string name_;
    std::string type_;
    int offset_ = 0;
    MonoField* field_ = nullptr;
};

class Method {
public:
    Method(std::string name, std::string return_type, int arg_count,
           void* native, MonoMethod* method);

    [[nodiscard]] const std::string& name() const { return name_; }
    [[nodiscard]] const std::string& return_type() const { return return_type_; }
    [[nodiscard]] int arg_count() const { return arg_count_; }
    [[nodiscard]] void* native() const { return native_; }
    [[nodiscard]] MonoMethod* raw() const { return method_; }

    template<typename Ret, typename... Args>
    Ret call(void* instance, Args... args) const {
        using fn_t = Ret(*)(void*, Args...);
        return reinterpret_cast<fn_t>(native_)(instance, args...);
    }

    template<typename Ret, typename... Args>
    Ret call_static(Args... args) const {
        using fn_t = Ret(*)(Args...);
        return reinterpret_cast<fn_t>(native_)(args...);
    }

    MonoObject* invoke(void* instance, void** params = nullptr) const;

    [[nodiscard]] std::string describe() const;

private:
    std::string name_;
    std::string return_type_;
    int arg_count_ = 0;
    void* native_ = nullptr;
    MonoMethod* method_ = nullptr;
};

} // namespace mono_base::mono
