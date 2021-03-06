BEGIN TRANSACTION;
CREATE TABLE IF NOT EXISTS `VideoTrack` (`id_track`	INTEGER PRIMARY KEY AUTOINCREMENT,`codec` TEXT, `width`	UNSIGNED INTEGER, `height` UNSIGNED INTEGER, `fps_num` UNSIGNED INTEGER, `fps_den` UNSIGNED INTEGER, `bitrate` UNSIGNED INTEGER, `sar_num` UNSIGNED INTEGER, `sar_den` UNSIGNED INTEGER, `media_id` UNSIGNED INT, `language` TEXT, `description` TEXT, FOREIGN KEY(`media_id`) REFERENCES `Media`(`id_media`) ON DELETE CASCADE);
CREATE TABLE IF NOT EXISTS `Thumbnail` (`id_thumbnail` INTEGER PRIMARY KEY AUTOINCREMENT, `mrl` TEXT, `origin` INTEGER NOT NULL, `is_generated` BOOLEAN NOT NULL);
CREATE TABLE IF NOT EXISTS `Task` (`id_task` INTEGER PRIMARY KEY AUTOINCREMENT,	`step`	INTEGER NOT NULL DEFAULT 0,	`retry_count`	INTEGER NOT NULL DEFAULT 0,	`mrl`	TEXT,	`file_type`	INTEGER NOT NULL,	`file_id`	UNSIGNED INTEGER,	`parent_folder_id`	UNSIGNED INTEGER,	`parent_playlist_id`	INTEGER,	`parent_playlist_index`	UNSIGNED INTEGER,	`is_refresh`	BOOLEAN NOT NULL DEFAULT 0,	UNIQUE(`mrl`,`parent_playlist_id`,`is_refresh`),	FOREIGN KEY(`parent_playlist_id`) REFERENCES `Playlist`(`id_playlist`) ON DELETE CASCADE,	FOREIGN KEY(`file_id`) REFERENCES `File`(`id_file`) ON DELETE CASCADE,	FOREIGN KEY(`parent_folder_id`) REFERENCES `Folder`(`id_folder`) ON DELETE CASCADE);
CREATE TABLE IF NOT EXISTS `SubtitleTrack` (`id_track`	INTEGER PRIMARY KEY AUTOINCREMENT,	`codec`	TEXT,	`language`	TEXT,	`description`	TEXT, `encoding`	TEXT,	`media_id`	UNSIGNED INT,	FOREIGN KEY(`media_id`) REFERENCES `Media`(`id_media`) ON DELETE CASCADE);
CREATE VIRTUAL TABLE ShowFts USING FTS3(title);
CREATE TABLE IF NOT EXISTS `ShowEpisode` (`id_episode`	INTEGER PRIMARY KEY AUTOINCREMENT,	`media_id`	UNSIGNED INTEGER NOT NULL,	`episode_number`	UNSIGNED INT,	`season_number`	UNSIGNED INT,	`episode_summary`	TEXT,	`tvdb_id`	TEXT,	`show_id`	UNSIGNED INT,	FOREIGN KEY(`media_id`) REFERENCES `Media`(`id_media`) ON DELETE CASCADE,	FOREIGN KEY(`show_id`) REFERENCES `Show`(`id_show`) ON DELETE CASCADE);
CREATE TABLE IF NOT EXISTS `Show` (`id_show`	INTEGER PRIMARY KEY AUTOINCREMENT,`title`	TEXT,`release_date`	UNSIGNED INTEGER,`short_summary`	TEXT,`artwork_mrl`	TEXT,`tvdb_id`	TEXT);
CREATE TABLE IF NOT EXISTS `Settings` (`db_model_version`	UNSIGNED INTEGER NOT NULL);
INSERT INTO `Settings` (db_model_version) VALUES (14);
CREATE TABLE IF NOT EXISTS `PlaylistMediaRelation` (`media_id`	INTEGER,`mrl`	STRING,`playlist_id`	INTEGER,`position`	INTEGER,FOREIGN KEY(`playlist_id`) REFERENCES `Playlist`(`id_playlist`) ON DELETE CASCADE,FOREIGN KEY(`media_id`) REFERENCES `Media`(`id_media`) ON DELETE SET NULL);
CREATE VIRTUAL TABLE PlaylistFts USING FTS3(name);
CREATE TABLE IF NOT EXISTS `Playlist` (`id_playlist`	INTEGER PRIMARY KEY AUTOINCREMENT,`name`	TEXT COLLATE NOCASE,`file_id`	UNSIGNED INT DEFAULT NULL,`creation_date`	UNSIGNED INT NOT NULL,`artwork_mrl`	TEXT,FOREIGN KEY(`file_id`) REFERENCES `File`(`id_file`) ON DELETE CASCADE);
CREATE TABLE IF NOT EXISTS `Movie` (`id_movie`	INTEGER PRIMARY KEY AUTOINCREMENT,`media_id`	UNSIGNED INTEGER NOT NULL,`summary`	TEXT,`imdb_id`	TEXT,FOREIGN KEY(`media_id`) REFERENCES `Media`(`id_media`) ON DELETE CASCADE);
CREATE TABLE IF NOT EXISTS `Metadata` (`id_media`	INTEGER,`entity_type`	INTEGER,`type`	INTEGER,`value`	TEXT,PRIMARY KEY(`id_media`,`entity_type`,`type`));
CREATE VIRTUAL TABLE MediaFts USING FTS3(title,labels);
CREATE TABLE IF NOT EXISTS `MediaArtistRelation` (`media_id`	INTEGER NOT NULL,`artist_id`	INTEGER,FOREIGN KEY(`media_id`) REFERENCES `Media`(`id_media`) ON DELETE CASCADE,PRIMARY KEY(`media_id`,`artist_id`),FOREIGN KEY(`artist_id`) REFERENCES `Artist`(`id_artist`) ON DELETE CASCADE);
CREATE TABLE IF NOT EXISTS `Media` (`id_media`	INTEGER PRIMARY KEY AUTOINCREMENT,`type`	INTEGER,`subtype`	INTEGER NOT NULL DEFAULT 0,`duration`	INTEGER DEFAULT -1,`play_count`	UNSIGNED INTEGER,`last_played_date`	UNSIGNED INTEGER,	`real_last_played_date`	UNSIGNED INTEGER,`insertion_date`	UNSIGNED INTEGER,`release_date`	UNSIGNED INTEGER,`thumbnail_id`	INTEGER,`title`	TEXT COLLATE NOCASE,`filename`	TEXT COLLATE NOCASE,`is_favorite`	BOOLEAN NOT NULL DEFAULT 0,	`is_present`	BOOLEAN NOT NULL DEFAULT 1,	`device_id`	INTEGER,	`nb_playlists`	UNSIGNED INTEGER NOT NULL DEFAULT 0,`folder_id`	UNSIGNED INTEGER,FOREIGN KEY(`thumbnail_id`) REFERENCES `Thumbnail`(`id_thumbnail`),FOREIGN KEY(`folder_id`) REFERENCES `Folder`(`id_folder`));
CREATE TABLE IF NOT EXISTS `LabelFileRelation` (`label_id`	INTEGER,`media_id`	INTEGER,FOREIGN KEY(`label_id`) REFERENCES `Label`(`id_label`) ON DELETE CASCADE,PRIMARY KEY(`label_id`,`media_id`),FOREIGN KEY(`media_id`) REFERENCES `Media`(`id_media`) ON DELETE CASCADE);
CREATE TABLE IF NOT EXISTS `Label` (`id_label`	INTEGER PRIMARY KEY AUTOINCREMENT,`name`	TEXT UNIQUE);
CREATE VIRTUAL TABLE GenreFts USING FTS3(name);
CREATE TABLE IF NOT EXISTS `Genre` (`id_genre`	INTEGER PRIMARY KEY AUTOINCREMENT,`name`	TEXT UNIQUE COLLATE NOCASE,`nb_tracks`	INTEGER NOT NULL DEFAULT 0);
CREATE VIRTUAL TABLE FolderFts USING FTS3(name);
CREATE TABLE IF NOT EXISTS `Folder` (`id_folder`	INTEGER PRIMARY KEY AUTOINCREMENT,`path`	TEXT,`name`	TEXT,`parent_id`	UNSIGNED INTEGER,`is_banned`	BOOLEAN NOT NULL DEFAULT 0,	`device_id`	UNSIGNED INTEGER,`is_removable`	BOOLEAN NOT NULL,`nb_audio`	UNSIGNED INTEGER NOT NULL DEFAULT 0,`nb_video`	UNSIGNED INTEGER NOT NULL DEFAULT 0,FOREIGN KEY(`device_id`) REFERENCES `Device`(`id_device`) ON DELETE CASCADE,FOREIGN KEY(`parent_id`) REFERENCES `Folder`(`id_folder`) ON DELETE CASCADE,UNIQUE(`path`,`device_id`));
CREATE TABLE IF NOT EXISTS `File` (`id_file`	INTEGER PRIMARY KEY AUTOINCREMENT,`media_id`	UNSIGNED INT DEFAULT NULL,`playlist_id`	UNSIGNED INT DEFAULT NULL,`mrl`	TEXT,	`type`	UNSIGNED INTEGER,`last_modification_date`	UNSIGNED INT,`size`	UNSIGNED INT,`folder_id`	UNSIGNED INTEGER,`is_removable`	BOOLEAN NOT NULL,`is_external`	BOOLEAN NOT NULL,`is_network`	BOOLEAN NOT NULL,FOREIGN KEY(`folder_id`) REFERENCES `Folder`(`id_folder`) ON DELETE CASCADE,FOREIGN KEY(`media_id`) REFERENCES `Media`(`id_media`) ON DELETE CASCADE,UNIQUE(`mrl`,`folder_id`),FOREIGN KEY(`playlist_id`) REFERENCES `Playlist`(`id_playlist`) ON DELETE CASCADE);
CREATE TABLE IF NOT EXISTS `ExcludedEntryFolder` (`folder_id`	UNSIGNED INTEGER NOT NULL UNIQUE,FOREIGN KEY(`folder_id`) REFERENCES `Folder`(`id_folder`) ON DELETE CASCADE);
CREATE TABLE IF NOT EXISTS `Device` (`id_device`	INTEGER PRIMARY KEY AUTOINCREMENT,`uuid`	TEXT UNIQUE COLLATE NOCASE,	`scheme`	TEXT,`is_removable`	BOOLEAN,`is_present`	BOOLEAN,`last_seen`	UNSIGNED INTEGER);
CREATE TABLE IF NOT EXISTS `AudioTrack` (`id_track`	INTEGER PRIMARY KEY AUTOINCREMENT,`codec`	TEXT,`bitrate`	UNSIGNED INTEGER,`samplerate`	UNSIGNED INTEGER,`nb_channels`	UNSIGNED INTEGER,`language`	TEXT,`description`	TEXT,`media_id`	UNSIGNED INT,FOREIGN KEY(`media_id`) REFERENCES `Media`(`id_media`) ON DELETE CASCADE);
CREATE VIRTUAL TABLE ArtistFts USING FTS3(name);
CREATE TABLE IF NOT EXISTS `Artist` (`id_artist`	INTEGER PRIMARY KEY AUTOINCREMENT,`name`	TEXT UNIQUE COLLATE NOCASE,`shortbio`	TEXT,`thumbnail_id`	TEXT,`nb_albums`	UNSIGNED INT DEFAULT 0,`nb_tracks`	UNSIGNED INT DEFAULT 0,`mb_id`	TEXT,`is_present`	UNSIGNED INTEGER NOT NULL DEFAULT 0,FOREIGN KEY(`thumbnail_id`) REFERENCES `Thumbnail`(`id_thumbnail`));
CREATE TABLE IF NOT EXISTS `AlbumTrack` (`id_track`	INTEGER PRIMARY KEY AUTOINCREMENT,`media_id`	INTEGER UNIQUE,`duration`	INTEGER NOT NULL,`artist_id`	UNSIGNED INTEGER,`genre_id`	INTEGER,`track_number`	UNSIGNED INTEGER,`album_id`	UNSIGNED INTEGER NOT NULL,`disc_number`	UNSIGNED INTEGER,FOREIGN KEY(`genre_id`) REFERENCES `Genre`(`id_genre`),FOREIGN KEY(`artist_id`) REFERENCES `Artist`(`id_artist`) ON DELETE CASCADE,	FOREIGN KEY(`album_id`) REFERENCES `Album`(`id_album`) ON DELETE CASCADE,FOREIGN KEY(`media_id`) REFERENCES `Media`(`id_media`) ON DELETE CASCADE);
CREATE VIRTUAL TABLE AlbumFts USING FTS3(title,artist);
CREATE TABLE IF NOT EXISTS `Album` (`id_album`	INTEGER PRIMARY KEY AUTOINCREMENT,`title`	TEXT COLLATE NOCASE,`artist_id`	UNSIGNED INTEGER,`release_year`	UNSIGNED INTEGER,`short_summary`	TEXT,`thumbnail_id`	UNSIGNED INT,`nb_tracks`	UNSIGNED INTEGER DEFAULT 0,`duration`	UNSIGNED INTEGER NOT NULL DEFAULT 0,`nb_discs`	UNSIGNED INTEGER NOT NULL DEFAULT 1,`is_present`	UNSIGNED INTEGER NOT NULL DEFAULT 0,FOREIGN KEY(`thumbnail_id`) REFERENCES `Thumbnail`(`id_thumbnail`), FOREIGN KEY(`artist_id`) REFERENCES `Artist`(`id_artist`) ON DELETE CASCADE);
CREATE INDEX IF NOT EXISTS `video_track_media_idx` ON `VideoTrack` (`media_id`);
CREATE INDEX IF NOT EXISTS `subtitle_track_media_idx` ON `SubtitleTrack` (`media_id`);
CREATE INDEX IF NOT EXISTS `show_episode_media_show_idx` ON `ShowEpisode` (`media_id`,`show_id`);
CREATE INDEX IF NOT EXISTS `playlist_media_pl_id_index` ON `PlaylistMediaRelation` (`media_id`,`playlist_id`);
CREATE INDEX IF NOT EXISTS `parent_folder_id_idx` ON `Folder` (`parent_id`);
CREATE INDEX IF NOT EXISTS `movie_media_idx` ON `Movie` (`media_id`);
CREATE INDEX IF NOT EXISTS `media_types_idx` ON `Media` (`type`,`subtype`);
CREATE INDEX IF NOT EXISTS `index_media_presence` ON `Media` (`is_present`);
CREATE INDEX IF NOT EXISTS `index_last_played_date` ON `Media` (`last_played_date`	DESC);
CREATE INDEX IF NOT EXISTS `folder_parent_id` ON `Folder` (`parent_id`);
CREATE INDEX IF NOT EXISTS `folder_device_id_idx` ON `Folder` (`device_id`);
CREATE INDEX IF NOT EXISTS `folder_device_id` ON `Folder` (`device_id`);
CREATE INDEX IF NOT EXISTS `file_media_id_index` ON `File` (`media_id`);
CREATE INDEX IF NOT EXISTS `file_folder_id_index` ON `File` (`folder_id`);
CREATE INDEX IF NOT EXISTS `audio_track_media_idx` ON `AudioTrack` (`media_id`);
CREATE INDEX IF NOT EXISTS `album_track_album_genre_artist_ids` ON `AlbumTrack` (`album_id`,`genre_id`,`artist_id`);
CREATE INDEX IF NOT EXISTS `album_media_artist_genre_album_idx` ON `AlbumTrack` (`media_id`,`artist_id`,`genre_id`,`album_id`);
CREATE INDEX IF NOT EXISTS `album_artist_id_idx` ON `Album` (`artist_id`);
CREATE TRIGGER update_playlist_order_on_insert AFTER INSERT ON PlaylistMediaRelation WHEN new.position IS NOT NULL BEGIN UPDATE PlaylistMediaRelation SET position = position + 1 WHERE playlist_id = new.playlist_id AND position = new.position AND media_id != new.media_id; END;
CREATE TRIGGER update_playlist_order AFTER UPDATE OF position ON PlaylistMediaRelation BEGIN UPDATE PlaylistMediaRelation SET position = position + 1 WHERE playlist_id = new.playlist_id AND position = new.position AND media_id != new.media_id; END;
CREATE TRIGGER update_playlist_fts AFTER UPDATE OF name ON Playlist BEGIN UPDATE PlaylistFts SET name = new.name WHERE rowid = new.id_playlist; END;
CREATE TRIGGER update_media_title_fts AFTER UPDATE OF title ON Media BEGIN UPDATE MediaFts SET title = new.title WHERE rowid = new.id_media; END;
CREATE TRIGGER update_genre_on_track_deleted AFTER DELETE ON AlbumTrack WHEN old.genre_id IS NOT NULL BEGIN UPDATE Genre SET nb_tracks = nb_tracks - 1 WHERE id_genre = old.genre_id; DELETE FROM Genre WHERE nb_tracks = 0; END;
CREATE TRIGGER update_genre_on_new_track AFTER INSERT ON AlbumTrack WHEN new.genre_id IS NOT NULL BEGIN UPDATE Genre SET nb_tracks = nb_tracks + 1 WHERE id_genre = new.genre_id; END;
CREATE TRIGGER update_folder_nb_media_on_update AFTER UPDATE ON Media WHEN new.folder_id IS NOT NULL AND old.type != new.type BEGIN UPDATE Folder SET nb_audio = nb_audio + (CASE old.type WHEN 2 THEN -1 ELSE 0 END)+(CASE new.type WHEN 2 THEN 1 ELSE 0 END),nb_video = nb_video + (CASE old.type WHEN 1 THEN -1 ELSE 0 END)+(CASE new.type WHEN 1 THEN 1 ELSE 0 END)WHERE id_folder = new.folder_id;END;
CREATE TRIGGER update_folder_nb_media_on_insert AFTER INSERT ON Media WHEN new.folder_id IS NOT NULL BEGIN UPDATE Folder SET nb_audio = nb_audio + (CASE new.type WHEN 2 THEN 1 ELSE 0 END),nb_video = nb_video + (CASE new.type WHEN 1 THEN 1 ELSE 0 END) WHERE id_folder = new.folder_id;END;
CREATE TRIGGER update_folder_nb_media_on_delete AFTER DELETE ON Media WHEN old.folder_id IS NOT NULL BEGIN UPDATE Folder SET nb_audio = nb_audio + (CASE old.type WHEN 2 THEN -1 ELSE 0 END),nb_video = nb_video + (CASE old.type WHEN 1 THEN -1 ELSE 0 END) WHERE id_folder = old.folder_id;END;
CREATE TRIGGER is_media_device_present AFTER UPDATE OF is_present ON Device BEGIN UPDATE Media SET is_present=new.is_present WHERE device_id=new.id_device;END;
CREATE TRIGGER is_album_present AFTER UPDATE OF is_present ON Media WHEN new.subtype = 3 BEGIN  UPDATE Album SET is_present=is_present + (CASE new.is_present WHEN 0 THEN -1 ELSE 1 END)WHERE id_album = (SELECT album_id FROM AlbumTrack WHERE media_id = new.id_media); END;
CREATE TRIGGER insert_show_fts AFTER INSERT ON Show BEGIN INSERT INTO ShowFts(rowid,title) VALUES(new.id_show, new.title); END;
CREATE TRIGGER insert_playlist_fts AFTER INSERT ON Playlist BEGIN INSERT INTO PlaylistFts(rowid, name) VALUES(new.id_playlist, new.name); END;
CREATE TRIGGER insert_media_fts AFTER INSERT ON Media BEGIN INSERT INTO MediaFts(rowid,title,labels) VALUES(new.id_media, new.title, ''); END;
CREATE TRIGGER insert_genre_fts AFTER INSERT ON Genre BEGIN INSERT INTO GenreFts(rowid,name) VALUES(new.id_genre, new.name); END;
CREATE TRIGGER insert_folder_fts AFTER INSERT ON Folder BEGIN INSERT INTO FolderFts(rowid,name) VALUES(new.id_folder,new.name);END;
CREATE TRIGGER insert_artist_fts AFTER INSERT ON Artist WHEN new.name IS NOT NULL BEGIN INSERT INTO ArtistFts(rowid,name) VALUES(new.id_artist, new.name); END;
CREATE TRIGGER insert_album_fts AFTER INSERT ON Album WHEN new.title IS NOT NULL BEGIN INSERT INTO AlbumFts(rowid, title) VALUES(new.id_album, new.title); END;
CREATE TRIGGER increment_media_nb_playlist AFTER INSERT ON  PlaylistMediaRelation  BEGIN  UPDATE Media SET nb_playlists = nb_playlists + 1  WHERE id_media = new.media_id; END;
CREATE TRIGGER has_tracks_present AFTER UPDATE OF is_present ON Media WHEN new.subtype = 3 BEGIN  UPDATE Artist SET is_present=is_present + (CASE new.is_present WHEN 0 THEN -1 ELSE 1 END)WHERE id_artist = (SELECT artist_id FROM AlbumTrack  WHERE media_id = new.id_media ); END;
CREATE TRIGGER has_track_remaining AFTER DELETE ON AlbumTrack WHEN old.artist_id != 1 AND  old.artist_id != 2 BEGIN UPDATE Artist SET nb_tracks = nb_tracks - 1, is_present = is_present - 1 WHERE id_artist = old.artist_id; DELETE FROM Artist WHERE id_artist = old.artist_id  AND nb_albums = 0  AND nb_tracks = 0; END;
CREATE TRIGGER has_album_remaining AFTER DELETE ON Album WHEN old.artist_id != 1 AND  old.artist_id != 2 BEGIN UPDATE Artist SET nb_albums = nb_albums - 1 WHERE id_artist = old.artist_id; DELETE FROM Artist WHERE id_artist = old.artist_id  AND nb_albums = 0  AND nb_tracks = 0; END;
CREATE TRIGGER delete_show_fts BEFORE DELETE ON Show BEGIN DELETE FROM ShowFts WHERE rowid = old.id_show; END;
CREATE TRIGGER delete_playlist_fts BEFORE DELETE ON Playlist BEGIN DELETE FROM PlaylistFts WHERE rowid = old.id_playlist; END;
CREATE TRIGGER delete_media_fts BEFORE DELETE ON Media BEGIN DELETE FROM MediaFts WHERE rowid = old.id_media; END;
CREATE TRIGGER delete_label_fts BEFORE DELETE ON Label BEGIN UPDATE MediaFts SET labels = TRIM(REPLACE(labels, old.name, '')) WHERE labels MATCH old.name; END;
CREATE TRIGGER delete_genre_fts BEFORE DELETE ON Genre BEGIN DELETE FROM GenreFts WHERE rowid = old.id_genre; END;
CREATE TRIGGER delete_folder_fts BEFORE DELETE ON Folder BEGIN DELETE FROM FolderFts WHERE rowid = old.id_folder;END;
CREATE TRIGGER delete_artist_fts BEFORE DELETE ON Artist WHEN old.name IS NOT NULL BEGIN DELETE FROM ArtistFts WHERE rowid=old.id_artist; END;
CREATE TRIGGER delete_album_track AFTER DELETE ON AlbumTrack BEGIN  UPDATE Album SET nb_tracks = nb_tracks - 1, is_present = is_present - 1, duration = duration - old.duration WHERE id_album = old.album_id; DELETE FROM Album WHERE id_album=old.album_id AND nb_tracks = 0; END;
CREATE TRIGGER delete_album_fts BEFORE DELETE ON Album WHEN old.title IS NOT NULL BEGIN DELETE FROM AlbumFts WHERE rowid = old.id_album; END;
CREATE TRIGGER decrement_media_nb_playlist AFTER DELETE ON  PlaylistMediaRelation  BEGIN  UPDATE Media SET nb_playlists = nb_playlists - 1  WHERE id_media = old.media_id; END;
CREATE TRIGGER cascade_file_deletion AFTER DELETE ON File BEGIN  DELETE FROM Media WHERE (SELECT COUNT(id_file) FROM File WHERE media_id=old.media_id) = 0 AND id_media=old.media_id; END;
CREATE TRIGGER append_new_playlist_record AFTER INSERT ON PlaylistMediaRelation WHEN new.position IS NULL BEGIN  UPDATE PlaylistMediaRelation SET position = (SELECT COUNT(media_id) FROM PlaylistMediaRelation WHERE playlist_id = new.playlist_id) WHERE playlist_id=new.playlist_id AND media_id = new.media_id; END;
CREATE TRIGGER add_album_track AFTER INSERT ON AlbumTrack BEGIN UPDATE Album SET duration = duration + new.duration, nb_tracks = nb_tracks + 1, is_present = is_present + 1 WHERE id_album = new.album_id; END;
INSERT INTO `Device` (id_device,uuid,scheme,is_removable,is_present,last_seen) VALUES (1,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `Folder` (id_folder,path,name,parent_id,is_banned,device_id,is_removable,nb_audio,nb_video) VALUES (1,'foo/','TestFolder',NULL,0,1,0,0,0);
COMMIT;
