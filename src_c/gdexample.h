#pragma once

#include "gdextension_interface.h"

#include "defs.h"

// Struct to hold the node data.
typedef struct{
    // Metadata.
    GDExtensionObjectPtr object; // Stores the underlying Godot object.
} GDExample;

// Constructor for the node.
void gdexample_class_constructor(GDExample *self);

// Destructor for the node.
void gdexample_class_destructor(GDExample *self);

// Bindings.
void gdexample_class_bind_methods();
GDExtensionObjectPtr gdexample_class_create_instance(void *p_class_userdata);
void gdexample_class_free_instance(void *p_class_userdata, GDExtensionClassInstancePtr p_instance);