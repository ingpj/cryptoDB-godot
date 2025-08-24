## 关于
使用了sqlcipher 静态集成 并对 godot 进行暴露

支持 windows x64 & mac m1 15.4+

mac 下 addons 下 .framework 需要暂时绕开签名
```bash
xattr -dr com.apple.quarantine libarm_gd.framework
codesign --force --deep --sign - libarm_gd.framework
```

## 软件

- 编译软件:
  - scons: `python -m pip install --user scons`
  - win: visual studio 2022 commuity c++ 环境
  - mac: xcode-developer-tool
- third_party
  - openssl 3.5.2
    - windows [docs](https://github.com/openssl/openssl/blob/master/NOTES-WINDOWS.md)
      - [perl](http://strawberryperl.com/)
      - [nasm](https://www.nasm.us/)
  - sqlipher 4.10.0
    - mac: 15.4+
    - windows 注意点: nmake 编译 `C2061: syntax error: identifier 'xoshiro_s'` [issues](https://github.com/sqlcipher/sqlcipher/issues/544)


- [godot-cpp 4.4.1](https://github.com/godotengine/godot-cpp)
  - mac: `scons platform=macos custom_api_file=extension_api.json -j10`


## 编译 (win 下 需调整 scons脚本 部分绝对路径 和 编译环境的path): 
  - `scons stage=openssl`
  - `scons stage=sqlcipher`
  - gdextension:
    - win: `scons stage=gdextension`
    - mac: `scons stage=gdextension platform=macos arch=arm64 target=template_release use_lto=yes`

## demo
```gdscript
extends Node

func _ready():
    var db = CryptoDB.new()
    var path = "user://test.db"
    #var key = "MySecretKey123"
    var key = "NewSecretKey456"

    # 打开数据库
    if not db.open(path, key):
        print("Failed to open DB:", db.get_last_error())
        return

    # -----------------------------
    # 1. 维护管理
    # -----------------------------
    print("--- Maintenance ---")
    if db.vacuum():
        print("VACUUM success")
    else:
        print("VACUUM failed:", db.get_last_error())

    var backup_path = "user://backup.db"
    ## 不加密备份成原始db
    #if db.backup_to(backup_path):
    ## 带加密备份，设置新密码
    if db.backup_to(backup_path, "123"):
        print("Backup success:", backup_path)
    else:
        print("Backup failed")

    # -----------------------------
    # 2. 表操作
    # -----------------------------
    print("--- Table Operations ---")
    db.exec("DROP TABLE IF EXISTS players")
    db.exec("""
        CREATE TABLE players (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT,
            score INTEGER
        )
    """)

    print("Tables:", db.list_tables())
    print("Columns of 'players':", db.list_columns("players"))

    # -----------------------------
    # 3. 事务与数据操作
    # -----------------------------
    print("--- Transactions & Data ---")
    db.begin_transaction()
    db.exec("INSERT INTO players (name, score) VALUES ('Alice', 100)")
    db.exec("INSERT INTO players (name, score) VALUES ('Bob', 200)")
    print("Last Insert ID:", db.get_last_insert_rowid())
    db.commit()

    # 查询数据
    var result = db.query("SELECT * FROM players")
    for row in result:
        print("Row:", row)

    # 测试回滚
    db.begin_transaction()
    db.exec("INSERT INTO players (name, score) VALUES ('Charlie', 300)")
    print("Added Charlie, but will rollback")
    db.rollback()

    result = db.query("SELECT * FROM players")
    print("After rollback:")
    for row in result:
        print(row)

    # -----------------------------
    # 4. 高级 SQLCipher 操作
    # -----------------------------
    print("--- SQLCipher Advanced ---")
    print("Cipher Version:", db.get_cipher_version())
    if db.rekey("NewSecretKey456"):
        print("Rekey success")
    else:
        print("Rekey failed:", db.get_last_error())

    db.close()
```



## 密码加强-可在编译前写在 c++内

- python 外部加密
```python
import os

def generate_key_parts(key: str):
    key_bytes = key.encode("utf-8")
    mask_bytes = os.urandom(len(key_bytes))  # 随机掩码
    part1 = [b ^ m for b, m in zip(key_bytes, mask_bytes)]
    part2 = list(mask_bytes)

    print("static const unsigned char key_part1[] = {", ", ".join(f"0x{b:02X}" for b in part1), "};")
    print("static const unsigned char key_part2[] = {", ", ".join(f"0x{b:02X}" for b in part2), "};")

# 输入你想加密的 key
generate_key_parts("MySecretKey123")
```

- c++ 内 函数

```c++
std::string get_obfuscated_key() {
    std::string key;
    for (size_t i = 0; i < sizeof(key_part1); i++) {
        key.push_back(key_part1[i] ^ key_part2[i]); // 异或还原
    }
    return key;
}
```

<!-- ## 测试 sqlcipher
```bash
clang test.c \
  -DSQLITE_HAS_CODEC=1 -DSQLITE_TEMP_STORE=2 \
  -DSQLITE_EXTRA_INIT=sqlcipher_extra_init \
  -DSQLITE_EXTRA_SHUTDOWN=sqlcipher_extra_shutdown \
  -DSQLITE_THREADSAFE=1 \
  -Ithird_party/sqlcipher/include \
  third_party/sqlcipher/lib/libsqlite3.a \
  third_party/openssl/lib/libssl.a \
  third_party/openssl/lib/libcrypto.a \
  -ldl -lpthread -lz \
  -o test
``` -->


## license 
- [SQLCIPHER Community Edition Open Source License](https://github.com/sqlcipher/sqlcipher/blob/master/LICENSE.md):
  ```txt
  Copyright (c) 2025, ZETETIC LLC All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. * Neither the name of the ZETETIC LLC nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY ZETETIC LLC ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL ZETETIC LLC BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  ```