"CREATE TABLE IF NOT EXISTS " + Folder::Table::Name +
"("
    "id_folder INTEGER PRIMARY KEY AUTOINCREMENT,"
    "path TEXT,"
    "parent_id UNSIGNED INTEGER,"
    "is_banned BOOLEAN NOT NULL DEFAULT 0,"
    "device_id UNSIGNED INTEGER,"
    "is_removable BOOLEAN NOT NULL,"
    "nb_media UNSIGNED INTEGER NOT NULL DEFAULT 0,"

    "FOREIGN KEY (parent_id) REFERENCES " + Folder::Table::Name +
    "(id_folder) ON DELETE CASCADE,"

    "FOREIGN KEY (device_id) REFERENCES " + Device::Table::Name +
    "(id_device) ON DELETE CASCADE,"

    "UNIQUE(path, device_id) ON CONFLICT FAIL"
")",

"CREATE INDEX IF NOT EXISTS folder_device_id ON " + Folder::Table::Name +
    "(device_id)",

"CREATE TABLE IF NOT EXISTS ExcludedEntryFolder"
"("
    "folder_id UNSIGNED INTEGER NOT NULL,"

    "FOREIGN KEY (folder_id) REFERENCES " + Folder::Table::Name +
    "(id_folder) ON DELETE CASCADE,"

    "UNIQUE(folder_id) ON CONFLICT FAIL"
")",
