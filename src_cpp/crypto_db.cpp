#include "crypto_db.h"

// 构造函数
CryptoDB::CryptoDB() {
    db = nullptr;
}

// 析构函数
CryptoDB::~CryptoDB() {
    if (db) {
        sqlite3_close((sqlite3*)db);
        db = nullptr;
    }
}

// 绑定方法到 Godot
void CryptoDB::_bind_methods() {
    ClassDB::bind_method(D_METHOD("open", "path", "key"), &CryptoDB::open);
    ClassDB::bind_method(D_METHOD("close"), &CryptoDB::close);
    ClassDB::bind_method(D_METHOD("query", "sql"), &CryptoDB::query);
    ClassDB::bind_method(D_METHOD("exec", "sql"), &CryptoDB::exec);
    ClassDB::bind_method(D_METHOD("exec_raw", "sql"), &CryptoDB::exec_raw);

    ClassDB::bind_method(D_METHOD("begin_transaction"), &CryptoDB::begin_transaction);
    ClassDB::bind_method(D_METHOD("commit"), &CryptoDB::commit);
    ClassDB::bind_method(D_METHOD("rollback"), &CryptoDB::rollback);

    ClassDB::bind_method(D_METHOD("get_last_insert_rowid"), &CryptoDB::get_last_insert_rowid);
    ClassDB::bind_method(D_METHOD("get_changes"), &CryptoDB::get_changes);
    ClassDB::bind_method(D_METHOD("list_tables"), &CryptoDB::list_tables);
    ClassDB::bind_method(D_METHOD("list_columns", "table"), &CryptoDB::list_columns);

    ClassDB::bind_method(D_METHOD("rekey", "new_key"), &CryptoDB::rekey);
    ClassDB::bind_method(D_METHOD("get_cipher_version"), &CryptoDB::get_cipher_version);
    ClassDB::bind_method(D_METHOD("set_cipher_page_size", "size"), &CryptoDB::set_cipher_page_size);
    ClassDB::bind_method(D_METHOD("set_kdf_iter", "iterations"), &CryptoDB::set_kdf_iter);

    ClassDB::bind_method(D_METHOD("vacuum"), &CryptoDB::vacuum);
    ClassDB::bind_method(D_METHOD("backup_to", "path", "key"), &CryptoDB::backup_to, DEFVAL("")); // 可选参数绑定

    ClassDB::bind_method(D_METHOD("get_last_error"), &CryptoDB::get_last_error);
    ClassDB::bind_method(D_METHOD("get_last_error_code"), &CryptoDB::get_last_error_code);
}

// #region 基础
bool CryptoDB::open(const String& path, const String& key) {
    if (db) {
        UtilityFunctions::print("Database already open.");
        return false;
    }

    String trimmed_key = key.strip_edges();
    if (trimmed_key.is_empty()) {
        UtilityFunctions::push_error("Encryption key cannot be empty.");
        return false;
    }

    // 🔁 将 user://, res:// 等虚拟路径转为真实路径
    String real_path = ProjectSettings::get_singleton()->globalize_path(path);
 
    std::string key_utf8 = std::string(trimmed_key.utf8().get_data());
    std::string path_utf8 = std::string(real_path.utf8().get_data());

    UtilityFunctions::print("Opening DB at: ", real_path);  // 调试用

    int rc = sqlite3_open(path_utf8.c_str(), (sqlite3**)&db);
    if (rc != SQLITE_OK) {
        const char* msg = sqlite3_errmsg((sqlite3*)db);
        UtilityFunctions::push_error("Cannot open database: ", String::utf8(msg));
        if (db) {
            sqlite3_close((sqlite3*)db);
            db = nullptr;
        }
        return false;
    }

    rc = sqlite3_key((sqlite3*)db, key_utf8.c_str(), (int)key_utf8.length());
    if (rc != SQLITE_OK) {
        UtilityFunctions::push_error("Failed to set encryption key: ", String::utf8(sqlite3_errmsg((sqlite3*)db)));
        sqlite3_close((sqlite3*)db);
        db = nullptr;
        return false;
    }

    rc = sqlite3_exec((sqlite3*)db, "SELECT count(*) FROM sqlite_master;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        UtilityFunctions::push_error("Wrong key or corrupted DB: ", String::utf8(sqlite3_errmsg((sqlite3*)db)));
        sqlite3_close((sqlite3*)db);
        db = nullptr;
        return false;
    }

    UtilityFunctions::print("Database opened at: ", real_path);
    return true;
}

// 仅允许 query
Variant CryptoDB::query(const String& sql) {
    if (!db) {
        UtilityFunctions::push_error("Database not open.");
        return Variant();
    }

    // 去掉首尾空格，并转大写，仅允许 select、pragma
    String sql_trimmed = sql.strip_edges().to_upper();
    if (!sql_trimmed.begins_with("SELECT") &&
        !sql_trimmed.begins_with("PRAGMA TABLE_INFO") &&
        !sql_trimmed.begins_with("PRAGMA")) // 根据需要添加其他允许的 PRAGMA
    {
        UtilityFunctions::push_error("Only SELECT or allowed PRAGMA statements are allowed in query().");
        return Variant();
    }

    UtilityFunctions::print("Executing SQL: ", sql);

    sqlite3_stmt* stmt;
    std::string utf8_sql = std::string(sql.utf8().get_data());
    int rc = sqlite3_prepare_v2((sqlite3*)db, utf8_sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        const char* msg = sqlite3_errmsg((sqlite3*)db);
        UtilityFunctions::push_error("SQL prepare failed: ", String::utf8(msg));
        return Variant();
    }

    Array result;

    while (true) {
        rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            Dictionary row;
            int col_count = sqlite3_column_count(stmt);
            for (int i = 0; i < col_count; i++) {
                String col_name = String::utf8(sqlite3_column_name(stmt, i));
                int type = sqlite3_column_type(stmt, i);
                Variant value;
                switch (type) {
                    case SQLITE_INTEGER:
                        value = (int64_t)sqlite3_column_int64(stmt, i);
                        break;
                    case SQLITE_FLOAT:
                        value = sqlite3_column_double(stmt, i);
                        break;
                    case SQLITE_TEXT:
                        value = String::utf8((const char*)sqlite3_column_text(stmt, i));
                        break;
                    case SQLITE_NULL:
                        value = Variant();
                        break;
                    default:
                        value = String::utf8((const char*)sqlite3_column_text(stmt, i));
                }
                row[col_name] = value;
            }
            result.append(row);
        } else if (rc == SQLITE_DONE) {
            // 查询结束，正常退出
            break;
        } else {
            // ❌ 严重错误！必须报错
            const char* msg = sqlite3_errmsg((sqlite3*)db);
            UtilityFunctions::push_error("SQL execution failed (step): ", String::utf8(msg));
            sqlite3_finalize(stmt);
            return Variant();  // 返回 null
        }
    }

    sqlite3_finalize(stmt);
    return result;
}

void CryptoDB::close() {
    if (db) {
        sqlite3_close((sqlite3*)db);
        db = nullptr;
        UtilityFunctions::print("Database closed.");
    }
}

// 执行原始 sql
bool CryptoDB::exec_raw(const String& sql) {
    if (!db) return false;
    char* errMsg = nullptr;
    std::string sql_utf8 = std::string(sql.utf8().get_data());
    int rc = sqlite3_exec((sqlite3*)db, sql_utf8.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        UtilityFunctions::push_error("SQL exec failed: ", String::utf8(errMsg));
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

// 执行 dml
bool CryptoDB::exec(const String& sql) {
    if (!db) return false;

    // 开始事务
    if (!exec_raw("BEGIN;")) return false;

    std::string sql_utf8 = std::string(sql.utf8().get_data());
    const char* sql_tail = sql_utf8.c_str();

    while (*sql_tail != '\0') {
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2((sqlite3*)db, sql_tail, -1, &stmt, &sql_tail);
        if (rc != SQLITE_OK) {
            exec_raw("ROLLBACK;"); // 出错回滚
            if (stmt) sqlite3_finalize(stmt);
            return false;
        }

        if (!stmt) continue;

        // 拒绝 DDL / DCL / TCL
        String sql_text = String::utf8(sqlite3_sql(stmt)).strip_edges().to_upper();
        if (
            sql_text.begins_with("CREATE") ||
            sql_text.begins_with("DROP")   ||
            sql_text.begins_with("ALTER")  ||
            sql_text.begins_with("TRUNCATE") ||
            sql_text.begins_with("ATTACH") ||
            sql_text.begins_with("DETACH") ||
            sql_text.begins_with("GRANT")  ||
            sql_text.begins_with("REVOKE") ||
            sql_text.begins_with("COMMIT") ||
            sql_text.begins_with("ROLLBACK") ||
            sql_text.begins_with("SAVEPOINT") ||
            sql_text.begins_with("BEGIN") ||
            sql_text.begins_with("END")
        ){
            UtilityFunctions::push_error("DDL not allowed in exec().");
            sqlite3_finalize(stmt);
            exec_raw("ROLLBACK;"); // 出错回滚
            return false;
        }

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
            UtilityFunctions::push_error("SQL execution failed: ", String::utf8(sqlite3_errmsg((sqlite3*)db)));
            sqlite3_finalize(stmt);
            exec_raw("ROLLBACK;"); // 出错回滚
            return false;
        }

        sqlite3_finalize(stmt);
    }

    // 全部成功 → 提交事务
    if (!exec_raw("COMMIT;")) {
        exec_raw("ROLLBACK;"); // 提交失败也回滚
        return false;
    }

    return true;
}



// #endregion

// #region 事务
bool CryptoDB::begin_transaction() {
    return exec_raw("BEGIN TRANSACTION;");
}

bool CryptoDB::commit() {
    return exec_raw("COMMIT;");
}

bool CryptoDB::rollback() {
    return exec_raw("ROLLBACK;");
}

// #endregion


// #region 元数据
int CryptoDB::get_last_insert_rowid() {
    if (!db) return 0;
    return (int)sqlite3_last_insert_rowid((sqlite3*)db);
}

int CryptoDB::get_changes() {
    if (!db) return 0;
    return sqlite3_changes((sqlite3*)db);
}

Array CryptoDB::list_tables() {
    Array tables;
    Variant res = query("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name;");
    if (res.get_type() == Variant::ARRAY) {
        for (int i = 0; i < ((Array)res).size(); i++) {
            Dictionary row = ((Array)res)[i];
            tables.append(row["name"]);
        }
    }
    return tables;
}


Array CryptoDB::list_columns(const String &table) {
    Array cols;
    String sql = vformat("PRAGMA table_info(%s);", table);
    Variant res = query(sql);

    if (res.get_type() == Variant::ARRAY) {
        Array rows = res;
        for (int i = 0; i < rows.size(); i++) {
            if (rows[i].get_type() != Variant::DICTIONARY) continue;
            Dictionary row = rows[i];

            // 直接把整个 Dictionary 追加到结果
            cols.append(row);
        }
    }
    return cols;
}

// #endregion

// #region SQLCIPHER
bool CryptoDB::rekey(const String &new_key) {
    if (!db) return false;
    std::string key_utf8 = std::string(new_key.utf8().get_data());
    int rc = sqlite3_rekey((sqlite3*)db, key_utf8.c_str(), (int)key_utf8.length());
    return rc == SQLITE_OK;
}

String CryptoDB::get_cipher_version() {
    Variant res = query("PRAGMA cipher_version;");
    if (res.get_type() == Variant::ARRAY && ((Array)res).size() > 0) {
        Dictionary row = ((Array)res)[0];
        return row.values()[0];
    }
    return "";
}

bool CryptoDB::set_kdf_iter(int iter) {
    if (!db) {
        return false;
    }
    char pragma[64];
    snprintf(pragma, sizeof(pragma), "PRAGMA kdf_iter = %d;", iter);
    int rc = sqlite3_exec(static_cast<sqlite3 *>(db), pragma, nullptr, nullptr, nullptr);
    return rc == SQLITE_OK;
}

bool CryptoDB::set_cipher_page_size(int size) {
    if (!db) {
        return false;
    }
    char pragma[64];
    snprintf(pragma, sizeof(pragma), "PRAGMA cipher_page_size = %d;", size);
    int rc = sqlite3_exec(static_cast<sqlite3 *>(db), pragma, nullptr, nullptr, nullptr);
    return rc == SQLITE_OK;
}


// #endregion

// #region 维护
bool CryptoDB::vacuum() {
    return exec_raw("VACUUM;");
}

bool CryptoDB::backup_to(const String &path, const String &key) {

    if (!db) {
        return false;
    }

    // 先创建目标库文件
    sqlite3 *pFile = nullptr;
    // 🔁 将 user://, res:// 等虚拟路径转为真实路径
    String real_path = ProjectSettings::get_singleton()->globalize_path(path);
    int rc = sqlite3_open(real_path.utf8().get_data(), &pFile);
    if (rc != SQLITE_OK) {
        return false;
    }

    if (!key.is_empty()) {
        // 给目标库加密
        const CharString key_utf8 = key.utf8();
        sqlite3_key(pFile, key_utf8.get_data(), key_utf8.length());
    }

    // 关闭新库（只是为了创建文件并加密）
    sqlite3_close(pFile);

    // 构造 ATTACH SQL
    String attach_sql;
    if (key.is_empty()) {
        attach_sql = vformat("ATTACH DATABASE '%s' AS backup KEY '';", real_path);
    } else {
        attach_sql = vformat("ATTACH DATABASE '%s' AS backup KEY '%s';", real_path, key);
    }

    rc = sqlite3_exec((sqlite3*)db, attach_sql.utf8().get_data(), nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        sqlite3_exec((sqlite3*)db, "DETACH DATABASE backup;", nullptr, nullptr, nullptr);
        return false;
    }

    // 导出数据
    rc = sqlite3_exec((sqlite3*)db, "SELECT sqlcipher_export('backup');", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        sqlite3_exec((sqlite3*)db, "DETACH DATABASE backup;", nullptr, nullptr, nullptr);
        UtilityFunctions::push_error("Failed to backup path_file is exists, plz remove it ");
        return false;
    }

    // 分离目标库
    rc = sqlite3_exec((sqlite3*)db, "DETACH DATABASE backup;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        return false;
    }

    return true;
}




// #endregion

// #region 错误
String CryptoDB::get_last_error() {
    if (!db) return "";
    return String::utf8(sqlite3_errmsg((sqlite3*)db));
}

int CryptoDB::get_last_error_code() {
    if (!db) return 0;
    return sqlite3_errcode((sqlite3*)db);
}

// #endregion