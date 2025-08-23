extends Node

func _ready():
	test_cryptodb()


func test_cryptodb():
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
	if db.backup_to(backup_path):
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
