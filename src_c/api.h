#pragma once

/*
This file works as a collection of helpers to call the GDExtension API
in a less verbose way, as well as a cache for methods from the discovery API,
just so we don't have to keep loading the same methods again.
*/

#include "gdextension_interface.h"
#include "defs.h"

extern GDExtensionClassLibraryPtr class_library;

// API methods.

struct Constructors{
    GDExtensionInterfaceStringNameNewWithLatin1Chars string_name_new_with_latin1_chars;
} constructors;

struct Destructors{
    GDExtensionPtrDestructor string_name_destructor;
} destructors;

struct API{
    GDExtensionInterfaceClassdbRegisterExtensionClass2 classdb_register_extension_class2;
    GDExtensionInterfaceClassdbConstructObject classdb_construct_object;
    GDExtensionInterfaceObjectSetInstance object_set_instance;
    GDExtensionInterfaceObjectSetInstanceBinding object_set_instance_binding;
    GDExtensionInterfaceMemAlloc mem_alloc;
    GDExtensionInterfaceMemFree mem_free;
} api;

void load_api(GDExtensionInterfaceGetProcAddress p_get_proc_address);