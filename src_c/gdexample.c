#include "gdexample.h"

#include "api.h"

const GDExtensionInstanceBindingCallbacks gdexample_class_binding_callbacks = {
    .create_callback = NULL,
    .free_callback = NULL,
    .reference_callback = NULL,
};

void gdexample_class_constructor(GDExample *self){
}

void gdexample_class_destructor(GDExample *self){
}

void gdexample_class_bind_methods(){
}




GDExtensionObjectPtr gdexample_class_create_instance(void *p_class_userdata){
    // Create native Godot object;
    StringName class_name;
    constructors.string_name_new_with_latin1_chars(&class_name, "Sprite2D", false);
    GDExtensionObjectPtr object = api.classdb_construct_object(&class_name);
    destructors.string_name_destructor(&class_name);

    // Create extension object.
    GDExample *self = (GDExample *)api.mem_alloc(sizeof(GDExample));
    gdexample_class_constructor(self);
    self->object = object;

    // Set the extension instance in the native Godot object.
    constructors.string_name_new_with_latin1_chars(&class_name, "GDExample", false);
    api.object_set_instance(object, &class_name, self);
    api.object_set_instance_binding(object, class_library, self, &gdexample_class_binding_callbacks);
    destructors.string_name_destructor(&class_name);

    return object;
}

void gdexample_class_free_instance(void *p_class_userdata, GDExtensionClassInstancePtr p_instance){
    
    if (p_instance == NULL){
        return;
    }

    GDExample *self = (GDExample *)p_instance;
    gdexample_class_destructor(self);
    api.mem_free(self);
}