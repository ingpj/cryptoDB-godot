#ifndef CRYPTODB_H
#define CRYPTODB_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/project_settings.hpp>

extern "C" {
    #include <sqlite3.h>
    int sqlite3_key(sqlite3 *db, const void *pKey, int nKey);
    int sqlite3_rekey(sqlite3 *db, const void *pKey, int nKey);
}

using namespace godot;

class CryptoDB : public RefCounted {
    GDCLASS(CryptoDB, RefCounted);

    private:
        void* db; // sqlite3*

    protected:
        static void _bind_methods();

    public:
        CryptoDB();
        ~CryptoDB();

        // 基础
        bool open(const String& path, const String& key);
        void close();
        Variant query(const String& sql);
        bool exec(const String& sql); // 不返回数据 dml
        bool exec_raw(const String& sql); // 不返回数据 支持 ddl

        // 事务
        bool begin_transaction();
        bool commit();
        bool rollback();

        // 元数据
        int get_last_insert_rowid();
        int get_changes();
        Array list_tables();
        Array list_columns(const String &table);

        // SQLCipher 特有
        bool rekey(const String &new_key);
        String get_cipher_version();
        bool set_cipher_page_size(int size);
        bool set_kdf_iter(int iterations);

        // 维护
        bool vacuum();
        bool backup_to(const String &path, const String &key = "");    // 导出数据库，如果 key 参数存在且不为空 则加密数据库，否则 明文导出

        // 错误
        String get_last_error();
        int get_last_error_code();
};

#endif
