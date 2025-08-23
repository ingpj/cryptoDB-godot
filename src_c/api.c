#include "api.h"

GDExtensionClassLibraryPtr class_library = NULL;

void load_api(GDExtensionInterfaceGetProcAddress p_get_proc_address){
    // Get helper functions first.
    GDExtensionInterfaceVariantGetPtrDestructor variant_get_ptr_destructor = (GDExtensionInterfaceVariantGetPtrDestructor)p_get_proc_address("variant_get_ptr_destructor");

    // Constructors.
    constructors.string_name_new_with_latin1_chars = (GDExtensionInterfaceStringNameNewWithLatin1Chars) p_get_proc_address("string_name_new_with_latin1_chars");

    // Destructors.
    destructors.string_name_destructor = variant_get_ptr_destructor(GDEXTENSION_VARIANT_TYPE_STRING_NAME);


    // API.
    api.classdb_register_extension_class2 = p_get_proc_address("classdb_register_extension_class2");
    api.classdb_construct_object = (GDExtensionInterfaceClassdbConstructObject)p_get_proc_address("classdb_construct_object");
    api.object_set_instance = p_get_proc_address("object_set_instance");
    api.object_set_instance_binding = p_get_proc_address("object_set_instance_binding");
    api.mem_alloc = (GDExtensionInterfaceMemAlloc)p_get_proc_address("mem_alloc");
    api.mem_free = (GDExtensionInterfaceMemFree)p_get_proc_address("mem_free");
}