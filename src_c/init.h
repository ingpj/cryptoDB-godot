#pragma once

#include "defs.h"  // 工具头
#include "gdextension_interface.h"

void initialize_gd_module(void *p_userdata, GDExtensionInitializationLevel p_level);  // 初始化模块
void deinitialize_gd_module(void *p_userdata, GDExtensionInitializationLevel p_level); // 反初始化模块

GDExtensionBool GDE_EXPORT gd_library_init(
    GDExtensionInterfaceGetProcAddress p_get_proc_address, // 工具获取器
    GDExtensionClassLibraryPtr p_library,                  // 你的库句柄
    GDExtensionInitialization *r_initialization            // 要填写的注册信息
);