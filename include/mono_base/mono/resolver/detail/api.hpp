#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <Windows.h>

struct MonoDomain;
struct MonoAssembly;
struct MonoImage;
struct MonoClass;
struct MonoMethod;
struct MonoField;
struct MonoObject;
struct MonoString;
struct MonoType;

namespace mono_base::mono {

class Image;

namespace detail {

using mono_get_root_domain_t = MonoDomain*(*)();
using mono_thread_attach_t = void*(*)(MonoDomain*);
using mono_assembly_foreach_t = void(*)(void(*func)(MonoAssembly*, void*), void*);
using mono_assembly_get_image_t = MonoImage*(*)(MonoAssembly*);
using mono_image_get_name_t = const char*(*)(MonoImage*);
using mono_class_from_name_t = MonoClass*(*)(MonoImage*, const char*, const char*);
using mono_class_get_name_t = const char*(*)(MonoClass*);
using mono_class_get_namespace_t = const char*(*)(MonoClass*);
using mono_class_get_method_from_name_t = MonoMethod*(*)(MonoClass*, const char*, int);
using mono_compile_method_t = void*(*)(MonoMethod*);
using mono_class_get_field_from_name_t = MonoField*(*)(MonoClass*, const char*);
using mono_field_get_offset_t = int(*)(MonoField*);
using mono_class_vtable_t = void*(*)(MonoDomain*, MonoClass*);
using mono_vtable_get_static_field_data_t = void*(*)(void*);
using mono_field_get_value_t = void(*)(MonoObject*, MonoField*, void*);
using mono_field_set_value_t = void(*)(MonoObject*, MonoField*, void*);
using mono_field_static_get_value_t = void(*)(MonoField*, void*);
using mono_field_static_set_value_t = void(*)(MonoField*, void*);
using mono_field_get_type_t = MonoType*(*)(MonoField*);
using mono_type_get_name_t = char*(*)(MonoType*);
using mono_method_get_name_t = const char*(*)(MonoMethod*);
using mono_runtime_invoke_t = MonoObject*(*)(MonoMethod*, void*, void**, MonoObject**);
using mono_object_unbox_t = void*(*)(MonoObject*);
using mono_string_to_utf8_t = char*(*)(MonoString*);
using mono_string_new_t = MonoString*(*)(MonoDomain*, const char*);
using mono_class_get_methods_t = void*(*)(MonoClass*, void**);
using mono_class_get_fields_t = void*(*)(MonoClass*, void**);
using mono_field_get_name_t = const char*(*)(MonoField*);
using mono_object_get_class_t = MonoClass*(*)(MonoObject*);
using mono_class_get_type_t = MonoType*(*)(MonoClass*);
using mono_type_get_object_t = MonoObject*(*)(MonoDomain*, MonoType*);
using mono_value_box_t = MonoObject*(*)(MonoDomain*, MonoClass*, void*);
using mono_object_to_string_t = MonoString*(*)(MonoObject*, MonoObject**);

struct ApiTable {
    mono_get_root_domain_t get_root_domain = nullptr;
    mono_thread_attach_t thread_attach = nullptr;
    mono_assembly_foreach_t assembly_foreach = nullptr;
    mono_assembly_get_image_t assembly_get_image = nullptr;
    mono_image_get_name_t image_get_name = nullptr;
    mono_class_from_name_t class_from_name = nullptr;
    mono_class_get_name_t class_get_name = nullptr;
    mono_class_get_namespace_t class_get_namespace = nullptr;
    mono_class_get_method_from_name_t class_get_method_from_name = nullptr;
    mono_compile_method_t compile_method = nullptr;
    mono_class_get_field_from_name_t class_get_field_from_name = nullptr;
    mono_field_get_offset_t field_get_offset = nullptr;
    mono_class_vtable_t class_vtable = nullptr;
    mono_vtable_get_static_field_data_t vtable_get_static_field_data = nullptr;
    mono_field_get_value_t field_get_value = nullptr;
    mono_field_set_value_t field_set_value = nullptr;
    mono_field_static_get_value_t field_static_get_value = nullptr;
    mono_field_static_set_value_t field_static_set_value = nullptr;
    mono_field_get_type_t field_get_type = nullptr;
    mono_type_get_name_t type_get_name = nullptr;
    mono_method_get_name_t method_get_name = nullptr;
    mono_runtime_invoke_t runtime_invoke = nullptr;
    mono_object_unbox_t object_unbox = nullptr;
    mono_string_to_utf8_t string_to_utf8 = nullptr;
    mono_string_new_t string_new = nullptr;
    mono_class_get_methods_t class_get_methods = nullptr;
    mono_class_get_fields_t class_get_fields = nullptr;
    mono_field_get_name_t field_get_name = nullptr;
    mono_object_get_class_t object_get_class = nullptr;
    mono_class_get_type_t class_get_type = nullptr;
    mono_type_get_object_t type_get_object = nullptr;
    mono_value_box_t value_box = nullptr;
    mono_object_to_string_t object_to_string = nullptr;
};

struct Context {
    HMODULE module = nullptr;
    const char* module_name = nullptr;
    MonoDomain* domain = nullptr;
    bool initialized = false;
    ApiTable api{};
    std::vector<std::unique_ptr<Image>> images;
    std::unordered_map<std::string, Image*> image_lookup;
};

Context& ctx();

void* resolve_export(const char* name);
bool resolve_api();
void enumerate_assemblies();

} // namespace detail
} // namespace mono_base::mono
