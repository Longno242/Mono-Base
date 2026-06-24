#include "mono_base/core/config.hpp"
#include "mono_base/core/logger.hpp"
#include "mono_base/mono/resolver/detail/api.hpp"
#include "mono_base/mono/resolver/types.hpp"
#include "mono_base/mono/resolver/types.inl.hpp"

namespace mono_base::mono::detail {

Context& ctx() {
    static Context instance;
    return instance;
}

void* resolve_export(const char* name) {
    auto& c = ctx();
    if (!c.module) {
        return nullptr;
    }
    return reinterpret_cast<void*>(GetProcAddress(c.module, name));
}

bool resolve_api() {
    auto& c = ctx();

#define RESOLVE(field, export_name) \
    c.api.field = reinterpret_cast<decltype(c.api.field)>(resolve_export(export_name)); \
    if (!c.api.field) LOG_ERROR("Mono", "Missing export: " export_name)

    RESOLVE(get_root_domain, "mono_get_root_domain");
    RESOLVE(thread_attach, "mono_thread_attach");
    RESOLVE(assembly_foreach, "mono_assembly_foreach");
    RESOLVE(assembly_get_image, "mono_assembly_get_image");
    RESOLVE(image_get_name, "mono_image_get_name");
    RESOLVE(class_from_name, "mono_class_from_name");
    RESOLVE(class_get_name, "mono_class_get_name");
    RESOLVE(class_get_namespace, "mono_class_get_namespace");
    RESOLVE(class_get_method_from_name, "mono_class_get_method_from_name");
    RESOLVE(compile_method, "mono_compile_method");
    RESOLVE(class_get_field_from_name, "mono_class_get_field_from_name");
    RESOLVE(field_get_offset, "mono_field_get_offset");
    RESOLVE(class_vtable, "mono_class_vtable");
    RESOLVE(vtable_get_static_field_data, "mono_vtable_get_static_field_data");
    RESOLVE(field_get_value, "mono_field_get_value");
    RESOLVE(field_set_value, "mono_field_set_value");
    RESOLVE(field_static_get_value, "mono_field_static_get_value");
    RESOLVE(field_static_set_value, "mono_field_static_set_value");
    RESOLVE(field_get_type, "mono_field_get_type");
    RESOLVE(type_get_name, "mono_type_get_name");
    RESOLVE(method_get_name, "mono_method_get_name");
    RESOLVE(runtime_invoke, "mono_runtime_invoke");
    RESOLVE(object_unbox, "mono_object_unbox");
    RESOLVE(string_to_utf8, "mono_string_to_utf8");
    RESOLVE(string_new, "mono_string_new");
    RESOLVE(class_get_methods, "mono_class_get_methods");
    RESOLVE(class_get_fields, "mono_class_get_fields");
    RESOLVE(field_get_name, "mono_field_get_name");
    RESOLVE(object_get_class, "mono_object_get_class");
    RESOLVE(class_get_type, "mono_class_get_type");
    RESOLVE(type_get_object, "mono_type_get_object");
    RESOLVE(value_box, "mono_value_box");
    RESOLVE(object_to_string, "mono_object_to_string");

#undef RESOLVE

    const bool ok =
        c.api.get_root_domain && c.api.thread_attach && c.api.assembly_foreach &&
        c.api.assembly_get_image && c.api.image_get_name && c.api.class_from_name &&
        c.api.class_get_method_from_name && c.api.compile_method &&
        c.api.class_get_field_from_name && c.api.field_get_offset &&
        c.api.field_get_value && c.api.field_static_get_value &&
        c.api.type_get_name && c.api.method_get_name && c.api.runtime_invoke;

    if (!ok) {
        LOG_FATAL("Mono", "Mono API resolution failed");
    }

    return ok;
}

static void assembly_callback(MonoAssembly* assembly, void* user) {
    auto* context = static_cast<Context*>(user);
    if (!assembly) {
        return;
    }

    MonoImage* image = context->api.assembly_get_image(assembly);
    if (!image) {
        return;
    }

    const char* name = context->api.image_get_name(image);
    if (!name || !*name) {
        return;
    }

    if (context->image_lookup.contains(name)) {
        return;
    }

    auto img = std::make_unique<Image>(name, image);
    context->image_lookup[name] = img.get();
    context->images.push_back(std::move(img));
}

void enumerate_assemblies() {
    auto& c = ctx();
    c.api.assembly_foreach(assembly_callback, &c);
}

} // namespace mono_base::mono::detail
