# SOME DESCRIPTIVE TITLE.
# Copyright (C) YEAR Free Software Foundation, Inc.
# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.
#
#, fuzzy
msgid ""
msgstr ""
"Project-Id-Version: PACKAGE VERSION\n"
"POT-Creation-Date: 2001-02-05 16:04-0500\n"
"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\n"
"Last-Translator: FULL NAME <EMAIL@ADDRESS>\n"
"Language-Team: LANGUAGE <LL@li.org>\n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=CHARSET\n"
"Content-Transfer-Encoding: ENCODING\n"

#: backends/bdb-backend.c:211
msgid "Unloading BerkeleyDB (BDB) backend module."
msgstr ""

#: backends/bdb-backend.c:234
#, c-format
msgid "Opened BerkeleyDB source at root %s"
msgstr ""

#: backends/bdb-backend.c:574
msgid "Initializing BDB backend module"
msgstr ""

#: backends/dir-utils.c:108
#, c-format
msgid "Couldn't find the %s root directory in the address `%s'"
msgstr ""

#: backends/dir-utils.c:124 backends/xml-backend.c:301 backends/xml-dir.c:1051
#, c-format
msgid "Could not make directory `%s': %s"
msgstr ""

#: backends/dir-utils.c:212
#, c-format
msgid "Can't read from or write to the %s root directory in the address `%s'"
msgstr ""

#: backends/xml-backend.c:233
msgid "Unloading XML backend module."
msgstr ""

#: backends/xml-backend.c:286
#, c-format
msgid "Couldn't find the XML root directory in the address `%s'"
msgstr ""

#: backends/xml-backend.c:386
#, c-format
msgid "Can't read from or write to the XML root directory in the address `%s'"
msgstr ""

#: backends/xml-backend.c:396
#, c-format
msgid "Directory/file permissions for XML source at root %s are: %o/%o"
msgstr ""

#: backends/xml-backend.c:680
msgid "Initializing XML backend module"
msgstr ""

#: backends/xml-backend.c:745
#, c-format
msgid "Failed to give up lock on XML dir `%s': %s"
msgstr ""

#: backends/xml-cache.c:119
msgid "Unsynced directory deletions when shutting down XML backend"
msgstr ""

#: backends/xml-cache.c:229
#, c-format
msgid ""
"Unable to remove directory `%s' from the XML backend cache, because it has "
"not been successfully synced to disk"
msgstr ""

#: backends/xml-cache.c:256
#, c-format
msgid ""
"%u items remain in the cache after cleaning already-synced items older than "
"%u seconds"
msgstr ""

#: backends/xml-dir.c:164
#, c-format
msgid "Could not stat `%s': %s"
msgstr ""

#: backends/xml-dir.c:174
#, c-format
msgid "XML filename `%s' is a directory"
msgstr ""

#: backends/xml-dir.c:298 backends/xml-dir.c:305
#, c-format
msgid "Failed to delete `%s': %s"
msgstr ""

#: backends/xml-dir.c:346
#, c-format
msgid "Failed to write file `%s': %s"
msgstr ""

#: backends/xml-dir.c:359
#, c-format
msgid "Failed to set mode on `%s': %s"
msgstr ""

#: backends/xml-dir.c:373 backends/xml-dir.c:383
#, c-format
msgid "Failed to rename `%s' to `%s': %s"
msgstr ""

#: backends/xml-dir.c:389
#, c-format
msgid "Failed to restore `%s' from `%s': %s"
msgstr ""

#: backends/xml-dir.c:401
#, c-format
msgid "Failed to delete old file `%s': %s"
msgstr ""

#. These are all fatal errors
#: backends/xml-dir.c:806
#, c-format
msgid "Failed to stat `%s': %s"
msgstr ""

#: backends/xml-dir.c:950
#, c-format
msgid "Duplicate entry `%s' in `%s', ignoring"
msgstr ""

#: backends/xml-dir.c:972
#, c-format
msgid "Entry with no name in XML file `%s', ignoring"
msgstr ""

#: backends/xml-dir.c:979
#, c-format
msgid "Toplevel node in XML file `%s' is not an <entry>, ignoring"
msgstr ""

#: backends/xml-dir.c:1067
#, c-format
msgid "Failed to create file `%s': %s"
msgstr ""

#: backends/xml-dir.c:1075 gconf/gconf-internals.c:2471
#, c-format
msgid "Failed to close file `%s': %s"
msgstr ""

#. There was an error
#: backends/xml-entry.c:148
#, c-format
msgid "Ignoring XML node with name `%s': %s"
msgstr ""

#: backends/xml-entry.c:199
#, c-format
msgid "%s"
msgstr ""

#: backends/xml-entry.c:326
#, c-format
msgid "Ignoring schema name `%s', invalid: %s"
msgstr ""

#. FIXME for nodes with no value stored, but containing a schema name,
#. * we improperly log an error here
#.
#: backends/xml-entry.c:372
#, c-format
msgid "Ignoring XML node `%s', except for possible schema name: %s"
msgstr ""

#: backends/xml-entry.c:723
#, c-format
msgid "Failed reading default value for schema: %s"
msgstr ""

#: backends/xml-entry.c:937
#, c-format
msgid "No \"type\" attribute for <%s> node"
msgstr ""

#: backends/xml-entry.c:951
#, c-format
msgid "A node has unknown \"type\" attribute `%s', ignoring"
msgstr ""

#: backends/xml-entry.c:966
msgid "No \"value\" attribute for node"
msgstr ""

#: backends/xml-entry.c:1014 backends/xml-entry.c:1090
#, c-format
msgid "Didn't understand XML node <%s> inside an XML list node"
msgstr ""

#: backends/xml-entry.c:1048
msgid "Invalid type (list, pair, or unknown) in a list node"
msgstr ""

#: backends/xml-entry.c:1071
#, c-format
msgid "Bad XML node: %s"
msgstr ""

#: backends/xml-entry.c:1079
#, c-format
msgid "List contains a badly-typed node (%s, should be %s)"
msgstr ""

#: backends/xml-entry.c:1131
#, c-format
msgid "Ignoring bad car from XML pair: %s"
msgstr ""

#: backends/xml-entry.c:1140 backends/xml-entry.c:1163
msgid "parsing XML file: lists and pairs may not be placed inside a pair"
msgstr ""

#: backends/xml-entry.c:1153
#, c-format
msgid "Ignoring bad cdr from XML pair: %s"
msgstr ""

#: backends/xml-entry.c:1172
#, c-format
msgid "Didn't understand XML node <%s> inside an XML pair node"
msgstr ""

#: backends/xml-entry.c:1190
msgid "Didn't find car and cdr for XML pair node"
msgstr ""

#: backends/xml-entry.c:1196
msgid "Missing cdr from pair of values in XML file"
msgstr ""

#: backends/xml-entry.c:1203
msgid "Missing car from pair of values in XML file"
msgstr ""

#: backends/xml-entry.c:1208
msgid "Missing both car and cdr values from pair in XML file"
msgstr ""

#. -- end debug only
#: gconf/gconf-backend.c:167
#, c-format
msgid "No such file `%s'\n"
msgstr ""

#: gconf/gconf-backend.c:195
#, c-format
msgid "Bad address `%s'"
msgstr ""

#: gconf/gconf-backend.c:220
msgid "GConf won't work without dynamic module support (gmodule)"
msgstr ""

#: gconf/gconf-backend.c:230
#, c-format
msgid "Error opening module `%s': %s\n"
msgstr ""

#: gconf/gconf-backend.c:262
#, c-format
msgid "Couldn't locate backend module for `%s'"
msgstr ""

#: gconf/gconf-backend.c:299
msgid "Failed to shut down backend"
msgstr ""

#: gconf/gconf-database.c:233
msgid "Received invalid value in set request"
msgstr ""

#: gconf/gconf-database.c:241
#, c-format
msgid "Couldn't make sense of CORBA value received in set request for key `%s'"
msgstr ""

#: gconf/gconf-database.c:521
msgid "Received request to drop all cached data"
msgstr ""

#: gconf/gconf-database.c:538
msgid "Received request to sync synchronously"
msgstr ""

#: gconf/gconf-database.c:605
msgid "Fatal error: failed to get object reference for ConfigDatabase"
msgstr ""

#: gconf/gconf-database.c:764
#, c-format
msgid "Failed to sync one or more sources: %s"
msgstr ""

#. This error is not fatal; we basically ignore it.
#. * Because it's likely the right thing for the client
#. * app to simply continue.
#.
#: gconf/gconf-database.c:846
#, c-format
msgid ""
"Failed to log addition of listener (%s); will not be able to restore this "
"listener on gconfd restart, resulting in unreliable notification of "
"configuration changes."
msgstr ""

#: gconf/gconf-database.c:871
#, c-format
msgid "Listener ID %lu doesn't exist"
msgstr ""

#: gconf/gconf-database.c:880
#, c-format
msgid ""
"Failed to log removal of listener to logfile (most likely harmless, may "
"result in a notification weirdly reappearing): %s"
msgstr ""

#: gconf/gconf-database.c:992 gconf/gconf-sources.c:1281
#, c-format
msgid "Error getting value for `%s': %s"
msgstr ""

#: gconf/gconf-database.c:1039
#, c-format
msgid "Error setting value for `%s': %s"
msgstr ""

#: gconf/gconf-database.c:1082
#, c-format
msgid "Error unsetting `%s': %s"
msgstr ""

#: gconf/gconf-database.c:1106
#, c-format
msgid "Error getting default value for `%s': %s"
msgstr ""

#: gconf/gconf-database.c:1145
#, c-format
msgid "Error checking existence of `%s': %s"
msgstr ""

#: gconf/gconf-database.c:1169
#, c-format
msgid "Error removing dir `%s': %s"
msgstr ""

#: gconf/gconf-database.c:1195
#, c-format
msgid "Failed to get all entries in `%s': %s"
msgstr ""

#: gconf/gconf-database.c:1221
#, c-format
msgid "Error listing dirs in `%s': %s"
msgstr ""

#: gconf/gconf-database.c:1242
#, c-format
msgid "Error setting schema for `%s': %s"
msgstr ""

#: gconf/gconf-error.c:25
msgid "Success"
msgstr ""

#: gconf/gconf-error.c:26
msgid "Failed"
msgstr ""

#: gconf/gconf-error.c:27
msgid "Configuration server couldn't be contacted"
msgstr ""

#: gconf/gconf-error.c:28
msgid "Permission denied"
msgstr ""

#: gconf/gconf-error.c:29
msgid "Couldn't resolve address for configuration source"
msgstr ""

#: gconf/gconf-error.c:30
msgid "Bad key or directory name"
msgstr ""

#: gconf/gconf-error.c:31
msgid "Parse error"
msgstr ""

#: gconf/gconf-error.c:32
msgid "Corrupt data in configuration source database"
msgstr ""

#: gconf/gconf-error.c:33
msgid "Type mismatch"
msgstr ""

#: gconf/gconf-error.c:34
msgid "Key operation on directory"
msgstr ""

#: gconf/gconf-error.c:35
msgid "Directory operation on key"
msgstr ""

#: gconf/gconf-error.c:36
msgid "Can't overwrite existing read-only value"
msgstr ""

#: gconf/gconf-error.c:37
msgid "Object Activation Framework error"
msgstr ""

#: gconf/gconf-error.c:38
msgid "Operation not allowed without configuration server"
msgstr ""

#: gconf/gconf-error.c:39
msgid "Failed to get a lock"
msgstr ""

#: gconf/gconf-error.c:40
msgid "No database available to save your configuration"
msgstr ""

#: gconf/gconf-internals.c:85
#, c-format
msgid "No '/' in key `%s'"
msgstr ""

#: gconf/gconf-internals.c:249
msgid "Couldn't interpret CORBA value for list element"
msgstr ""

#: gconf/gconf-internals.c:251
#, c-format
msgid "Incorrect type for list element in %s"
msgstr ""

#: gconf/gconf-internals.c:264
msgid "Received list from gconfd with a bad list type"
msgstr ""

#: gconf/gconf-internals.c:445
msgid "Failed to convert object to IOR"
msgstr ""

#: gconf/gconf-internals.c:811
#, c-format
msgid "Couldn't open path file `%s': %s\n"
msgstr ""

#: gconf/gconf-internals.c:866
#, c-format
msgid "Adding source `%s'\n"
msgstr ""

#: gconf/gconf-internals.c:878
#, c-format
msgid "Read error on file `%s': %s\n"
msgstr ""

#: gconf/gconf-internals.c:1321
#, c-format
msgid "Expected list, got %s"
msgstr ""

#: gconf/gconf-internals.c:1331
#, c-format
msgid "Expected list of %s, got list of %s"
msgstr ""

#: gconf/gconf-internals.c:1470
#, c-format
msgid "Expected pair, got %s"
msgstr ""

#: gconf/gconf-internals.c:1484
#, c-format
msgid "Expected (%s,%s) pair, got a pair with one or both values missing"
msgstr ""

#: gconf/gconf-internals.c:1500
#, c-format
msgid "Expected pair of type (%s,%s) got type (%s,%s)"
msgstr ""

#: gconf/gconf-internals.c:1616
msgid "Quoted string doesn't begin with a quotation mark"
msgstr ""

#: gconf/gconf-internals.c:1678
msgid "Quoted string doesn't end with a quotation mark"
msgstr ""

#: gconf/gconf-internals.c:2158 gconf/gconf.c:2945
#, c-format
msgid "CORBA error: %s"
msgstr ""

#: gconf/gconf-internals.c:2174
#, c-format
msgid "OAF problem description: '%s'"
msgstr ""

#: gconf/gconf-internals.c:2180
msgid "attempt to remove not-listed OAF object directory"
msgstr ""

#: gconf/gconf-internals.c:2185
msgid "attempt to add already-listed OAF directory"
msgstr ""

#: gconf/gconf-internals.c:2192
#, c-format
msgid "OAF parse error: %s"
msgstr ""

#: gconf/gconf-internals.c:2197
msgid "Unknown OAF error"
msgstr ""

#: gconf/gconf-internals.c:2280
#, c-format
msgid "No ior file in `%s'"
msgstr ""

#: gconf/gconf-internals.c:2311
#, c-format
msgid "gconfd taking lock `%s' from some other process"
msgstr ""

#: gconf/gconf-internals.c:2322
#, c-format
msgid "Another program has lock `%s'"
msgstr ""

#: gconf/gconf-internals.c:2342
msgid "couldn't contact ORB to ping existing gconfd"
msgstr ""

#: gconf/gconf-internals.c:2350
#, c-format
msgid ""
"Removing stale lock `%s' because IOR couldn't be converted to object "
"reference, IOR `%s'"
msgstr ""

#: gconf/gconf-internals.c:2362
#, c-format
msgid "Removing stale lock `%s' because of error pinging server: %s"
msgstr ""

#: gconf/gconf-internals.c:2374
#, c-format
msgid "GConf configuration daemon (gconfd) has lock `%s'"
msgstr ""

#: gconf/gconf-internals.c:2388
#, c-format
msgid "couldn't create directory `%s': %s"
msgstr ""

#: gconf/gconf-internals.c:2423
#, c-format
msgid "Can't create lock `%s': %s"
msgstr ""

#: gconf/gconf-internals.c:2459
#, c-format
msgid "Can't write to file `%s': %s"
msgstr ""

#: gconf/gconf-internals.c:2502
#, c-format
msgid "Can't open lock file `%s'; assuming it isn't ours: %s"
msgstr ""

#: gconf/gconf-internals.c:2519
#, c-format
msgid "Corrupt lock file `%s', removing anyway"
msgstr ""

#: gconf/gconf-internals.c:2529
#, c-format
msgid ""
"Didn't create lock file `%s' (creator pid %u, our pid %u; assuming someone "
"took our lock"
msgstr ""

#: gconf/gconf-internals.c:2546
#, c-format
msgid "Failed to release lock directory `%s': %s"
msgstr ""

#: gconf/gconf-sources.c:320
#, c-format
msgid "Failed to load source `%s': %s"
msgstr ""

#: gconf/gconf-sources.c:506
#, c-format
msgid "Schema `%s' specified for `%s' stores a non-schema value"
msgstr ""

#: gconf/gconf-sources.c:555
msgid "The '/' name can only be a directory, not a key"
msgstr ""

#: gconf/gconf-sources.c:584
#, c-format
msgid ""
"Value for `%s' set in a read-only source at the front of your configuration "
"path."
msgstr ""

#: gconf/gconf-sources.c:596
#, c-format
msgid "Unable to store a value at key '%s'"
msgstr ""

#: gconf/gconf-sources.c:1151
#, c-format
msgid "Error finding metainfo: %s"
msgstr ""

#: gconf/gconf-sources.c:1220
#, c-format
msgid "Error getting metainfo: %s"
msgstr ""

#: gconf/gconf-sources.c:1244
#, c-format
msgid "Key `%s' listed as schema for key `%s' actually stores type `%s'"
msgstr ""

#: gconf/gconf-value.c:82
#, c-format
msgid "Didn't understand `%s' (expected integer)"
msgstr ""

#: gconf/gconf-value.c:92
#, c-format
msgid "Integer `%s' is too large or small"
msgstr ""

#: gconf/gconf-value.c:113
#, c-format
msgid "Didn't understand `%s' (expected real number)"
msgstr ""

#: gconf/gconf-value.c:146
#, c-format
msgid "Didn't understand `%s' (expected true or false)"
msgstr ""

#: gconf/gconf-value.c:214
#, c-format
msgid "Didn't understand `%s' (list must start with a '[')"
msgstr ""

#: gconf/gconf-value.c:227
#, c-format
msgid "Didn't understand `%s' (list must end with a ']')"
msgstr ""

#: gconf/gconf-value.c:278
#, c-format
msgid "Didn't understand `%s' (extra unescaped ']' found inside list)"
msgstr ""

#: gconf/gconf-value.c:309 gconf/gconf-value.c:462
#, c-format
msgid "Didn't understand `%s' (extra trailing characters)"
msgstr ""

#: gconf/gconf-value.c:348
#, c-format
msgid "Didn't understand `%s' (pair must start with a '(')"
msgstr ""

#: gconf/gconf-value.c:361
#, c-format
msgid "Didn't understand `%s' (pair must end with a ')')"
msgstr ""

#: gconf/gconf-value.c:391 gconf/gconf-value.c:477
#, c-format
msgid "Didn't understand `%s' (wrong number of elements)"
msgstr ""

#: gconf/gconf-value.c:431
#, c-format
msgid "Didn't understand `%s' (extra unescaped ')' found inside pair)"
msgstr ""

#: gconf/gconf.c:54
#, c-format
msgid "`%s': %s"
msgstr ""

#: gconf/gconf.c:289
#, c-format
msgid "Server couldn't resolve the address `%s'"
msgstr ""

#: gconf/gconf.c:568
msgid "Can't add notifications to a local configuration source"
msgstr ""

#: gconf/gconf.c:1724
#, c-format
msgid "Adding client to server's list failed, CORBA error: %s"
msgstr ""

#: gconf/gconf.c:1740
msgid ""
"Error contacting configuration server: OAF returned nil from "
"oaf_activate_from_id() and did not set an exception explaining the problem. "
"Please file an OAF bug report."
msgstr ""

#: gconf/gconf.c:2051
msgid "Failed to init GConf, exiting\n"
msgstr ""

#: gconf/gconf.c:2088
msgid "Must begin with a slash (/)"
msgstr ""

#: gconf/gconf.c:2110
msgid "Can't have two slashes (/) in a row"
msgstr ""

#: gconf/gconf.c:2112
msgid "Can't have a period (.) right after a slash (/)"
msgstr ""

#: gconf/gconf.c:2133
#, c-format
msgid "`%c' is an invalid character in key/directory names"
msgstr ""

#: gconf/gconf.c:2147
msgid "Key/directory may not end with a slash (/)"
msgstr ""

#: gconf/gconf.c:2400
#, c-format
msgid "Failure shutting down config server: %s"
msgstr ""

#: gconf/gconf.c:2461
#, c-format
msgid "Expected float, got %s"
msgstr ""

#: gconf/gconf.c:2496
#, c-format
msgid "Expected int, got %s"
msgstr ""

#: gconf/gconf.c:2531
#, c-format
msgid "Expected string, got %s"
msgstr ""

#: gconf/gconf.c:2568
#, c-format
msgid "Expected bool, got %s"
msgstr ""

#: gconf/gconf.c:2601
#, c-format
msgid "Expected schema, got %s"
msgstr ""

#: gconf/gconfd.c:247
msgid "Shutdown request received"
msgstr ""

#: gconf/gconfd.c:279
msgid ""
"gconfd compiled with debugging; trying to load gconf.path from the source "
"directory"
msgstr ""

#: gconf/gconfd.c:297
#, c-format
msgid ""
"No configuration files found, trying to use the default config source `%s'"
msgstr ""

#: gconf/gconfd.c:306
msgid ""
"No configuration sources in the source path, configuration won't be saved; "
"edit "
msgstr ""

#: gconf/gconfd.c:320
#, c-format
msgid "Error loading some config sources: %s"
msgstr ""

#: gconf/gconfd.c:332
msgid ""
"No config source addresses successfully resolved, can't load or store config "
"data"
msgstr ""

#: gconf/gconfd.c:349
msgid ""
"No writable config sources successfully resolved, may not be able to save "
"some configuration changes"
msgstr ""

#: gconf/gconfd.c:375
#, c-format
msgid "Received signal %d, dumping core. Please report a GConf bug."
msgstr ""

#: gconf/gconfd.c:391
#, c-format
msgid ""
"Received signal %d, shutting down abnormally. Please file a GConf bug report."
msgstr ""

#: gconf/gconfd.c:408
#, c-format
msgid "Received signal %d, shutting down cleanly"
msgstr ""

#. openlog() does not copy logname - what total brokenness.
#. So we free it at the end of main()
#: gconf/gconfd.c:457
#, c-format
msgid "starting (version %s), pid %u user '%s'"
msgstr ""

#: gconf/gconfd.c:461
msgid "GConf was built with debugging features enabled"
msgstr ""

#: gconf/gconfd.c:484
msgid ""
"Failed to init Object Activation Framework: please mail bug report to OAF "
"maintainers"
msgstr ""

#: gconf/gconfd.c:504
msgid "Failed to get object reference for ConfigServer"
msgstr ""

#: gconf/gconfd.c:524
msgid ""
"OAF doesn't know about our IID; indicates broken installation; can't "
"register; exiting\n"
msgstr ""

#: gconf/gconfd.c:528
msgid "Another gconfd already registered with OAF; exiting\n"
msgstr ""

#: gconf/gconfd.c:533
msgid "Unknown error registering gconfd with OAF; exiting\n"
msgstr ""

#: gconf/gconfd.c:571
msgid "Exiting"
msgstr ""

#: gconf/gconfd.c:595
msgid "GConf server is not in use, shutting down."
msgstr ""

#: gconf/gconfd.c:930
#, c-format
msgid "Returning exception: %s"
msgstr ""

#: gconf/gconfd.c:1018
#, c-format
msgid ""
"Failed to open gconfd logfile; won't be able to restore listeners after "
"gconfd shutdown (%s)"
msgstr ""

#: gconf/gconfd.c:1041
#, c-format
msgid ""
"Failed to close gconfd logfile; data may not have been properly saved (%s)"
msgstr ""

#: gconf/gconfd.c:1104
#, c-format
msgid "Could not open saved state file '%s' for writing: %s"
msgstr ""

#: gconf/gconfd.c:1118
#, c-format
msgid "Could not write saved state file '%s' fd: %d: %s"
msgstr ""

#: gconf/gconfd.c:1127
#, c-format
msgid "Failed to close new saved state file '%s': %s"
msgstr ""

#: gconf/gconfd.c:1141
#, c-format
msgid "Could not move aside old saved state file '%s': %s"
msgstr ""

#: gconf/gconfd.c:1151
#, c-format
msgid "Failed to move new save state file into place: %s"
msgstr ""

#: gconf/gconfd.c:1160
#, c-format
msgid ""
"Failed to restore original saved state file that had been moved to '%s': %s"
msgstr ""

#: gconf/gconfd.c:1633
#, c-format
msgid ""
"Unable to restore a listener on address '%s', couldn't resolve the database"
msgstr ""

#: gconf/gconfd.c:1679
#, c-format
msgid "Error reading saved state file: %s"
msgstr ""

#: gconf/gconfd.c:1732
#, c-format
msgid "Unable to open saved state file '%s': %s"
msgstr ""

#: gconf/gconfd.c:1849
#, c-format
msgid ""
"Failed to log addition of listener to gconfd logfile; won't be able to "
"re-add the listener if gconfd exits or shuts down (%s)"
msgstr ""

#: gconf/gconfd.c:1854
#, c-format
msgid ""
"Failed to log removal of listener to gconfd logfile; might erroneously "
"re-add the listener if gconfd exits or shuts down (%s)"
msgstr ""

#: gconf/gconfd.c:1877 gconf/gconfd.c:2038
#, c-format
msgid "Failed to get IOR for client: %s"
msgstr ""

#: gconf/gconfd.c:1892
#, c-format
msgid "Failed to open saved state file: %s"
msgstr ""

#: gconf/gconfd.c:1905
#, c-format
msgid "Failed to write client add to saved state file: %s"
msgstr ""

#: gconf/gconfd.c:1913
#, c-format
msgid "Failed to flush client add to saved state file: %s"
msgstr ""

#: gconf/gconfd.c:1999
msgid ""
"Some client removed itself from the GConf server when it hadn't been added."
msgstr ""

#: gconf/gconftool.c:62
msgid "Help options"
msgstr ""

#: gconf/gconftool.c:71
msgid "Set a key to a value and sync. Use with --type."
msgstr ""

#: gconf/gconftool.c:80
msgid "Print the value of a key to standard output."
msgstr ""

#: gconf/gconftool.c:89
msgid ""
"Set a schema and sync. Use with --short-desc, --long-desc, --owner, and "
"--type."
msgstr ""

#: gconf/gconftool.c:99
msgid "Unset the keys on the command line"
msgstr ""

#: gconf/gconftool.c:108
msgid "Print all key/value pairs in a directory."
msgstr ""

#: gconf/gconftool.c:117
msgid "Print all subdirectories in a directory."
msgstr ""

#: gconf/gconftool.c:126
msgid "Print all subdirectories and entries under a dir, recursively."
msgstr ""

#: gconf/gconftool.c:135
msgid "Return 0 if the directory exists, 2 if it does not."
msgstr ""

#: gconf/gconftool.c:144
msgid "Shut down gconfd. DON'T USE THIS OPTION WITHOUT GOOD REASON."
msgstr ""

#: gconf/gconftool.c:153
msgid "Return 0 if gconfd is running, 2 if not."
msgstr ""

#: gconf/gconftool.c:162
msgid ""
"Launch the config server (gconfd). (Normally happens automatically when "
"needed.)"
msgstr ""

#: gconf/gconftool.c:171
msgid ""
"Specify the type of the value being set, or the type of the value a schema "
"describes. Unique abbreviations OK."
msgstr ""

#: gconf/gconftool.c:172
msgid "int|bool|float|string|list|pair"
msgstr ""

#: gconf/gconftool.c:180
msgid ""
"Specify the type of the list value being set, or the type of the value a "
"schema describes. Unique abbreviations OK."
msgstr ""

#: gconf/gconftool.c:181 gconf/gconftool.c:190 gconf/gconftool.c:199
msgid "int|bool|float|string"
msgstr ""

#: gconf/gconftool.c:189
msgid ""
"Specify the type of the car pair value being set, or the type of the value a "
"schema describes. Unique abbreviations OK."
msgstr ""

#: gconf/gconftool.c:198
msgid ""
"Specify the type of the cdr pair value being set, or the type of the value a "
"schema describes. Unique abbreviations OK."
msgstr ""

#: gconf/gconftool.c:207
msgid "Specify a short half-line description to go in a schema."
msgstr ""

#: gconf/gconftool.c:208 gconf/gconftool.c:217
msgid "DESCRIPTION"
msgstr ""

#: gconf/gconftool.c:216
msgid "Specify a several-line description to go in a schema."
msgstr ""

#: gconf/gconftool.c:225
msgid "Specify the owner of a schema"
msgstr ""

#: gconf/gconftool.c:226
msgid "OWNER"
msgstr ""

#: gconf/gconftool.c:234
msgid "Specify a schema file to be installed"
msgstr ""

#: gconf/gconftool.c:235
msgid "FILENAME"
msgstr ""

#: gconf/gconftool.c:243
msgid "Specify a configuration source to use rather than the default path"
msgstr ""

#: gconf/gconftool.c:244
msgid "SOURCE"
msgstr ""

#: gconf/gconftool.c:252
msgid ""
"Access the config database directly, bypassing server. Requires that gconfd "
"is not running."
msgstr ""

#: gconf/gconftool.c:261
msgid ""
"Properly installs schema files on the command line into the database. "
"GCONF_CONFIG_SOURCE environment variable should be set to a non-default "
"config source or set to the empty string to use the default."
msgstr ""

#: gconf/gconftool.c:270
msgid ""
"Torture-test an application by setting and unsetting a bunch of values of "
"different types for keys on the command line."
msgstr ""

#: gconf/gconftool.c:279
msgid ""
"Torture-test an application by setting and unsetting a bunch of keys inside "
"the directories on the command line."
msgstr ""

#: gconf/gconftool.c:326
#, c-format
msgid ""
"Error on option %s: %s.\n"
"Run '%s --help' to see a full list of available command line options.\n"
msgstr ""

#: gconf/gconftool.c:338
msgid "Can't get and set/unset simultaneously\n"
msgstr ""

#: gconf/gconftool.c:345
msgid "Can't set and get/unset simultaneously\n"
msgstr ""

#: gconf/gconftool.c:353
msgid "Can't use --all-pairs with --get or --set\n"
msgstr ""

#: gconf/gconftool.c:361
msgid "Can't use --all-dirs with --get or --set\n"
msgstr ""

#: gconf/gconftool.c:371
msgid ""
"--recursive-list should not be used with --get, --set, --unset, --all-pairs, "
"or --all-dirs\n"
msgstr ""

#: gconf/gconftool.c:381
msgid ""
"--set_schema should not be used with --get, --set, --unset, --all-pairs, "
"--all-dirs\n"
msgstr ""

#: gconf/gconftool.c:387
msgid "Value type is only relevant when setting a value\n"
msgstr ""

#: gconf/gconftool.c:393
msgid "Must specify a type when setting a value\n"
msgstr ""

#: gconf/gconftool.c:402
msgid "Ping option must be used by itself.\n"
msgstr ""

#: gconf/gconftool.c:411
msgid "--dir-exists option must be used by itself.\n"
msgstr ""

#: gconf/gconftool.c:420
msgid "--install-schema-file must be used by itself.\n"
msgstr ""

#: gconf/gconftool.c:430
msgid "--makefile-install-rule must be used by itself.\n"
msgstr ""

#: gconf/gconftool.c:440
msgid "--break-key must be used by itself.\n"
msgstr ""

#: gconf/gconftool.c:450
msgid "--break-directory must be used by itself.\n"
msgstr ""

#: gconf/gconftool.c:457
msgid ""
"You must specify a config source with --config-source when using --direct\n"
msgstr ""

#: gconf/gconftool.c:463
#, c-format
msgid "Failed to init GConf: %s\n"
msgstr ""

#: gconf/gconftool.c:488
msgid "Must set the GCONF_CONFIG_SOURCE environment variable\n"
msgstr ""

#. Eventually you should be able to run gconfd as long as
#. you're installing to a different database from the one
#. it's using, but I don't trust the locking right now.
#: gconf/gconftool.c:503
msgid ""
"Shouldn't run gconfd while installing new schema files.\n"
"Use gconftool --shutdown to shut down the daemon, most safely while no "
"applications are running\n"
"(though things theoretically work if apps are running).\n"
msgstr ""

#: gconf/gconftool.c:525
#, c-format
msgid "Failed to access configuration source(s): %s\n"
msgstr ""

#: gconf/gconftool.c:685
#, c-format
msgid "Shutdown error: %s\n"
msgstr ""

#: gconf/gconftool.c:730
msgid "Must specify one or more dirs to recursively list.\n"
msgstr ""

#: gconf/gconftool.c:764
#, c-format
msgid "Failure listing pairs in `%s': %s"
msgstr ""

#: gconf/gconftool.c:782
msgid "(no value set)"
msgstr ""

#: gconf/gconftool.c:837
#, c-format
msgid "Failed to spawn the config server (gconfd): %s\n"
msgstr ""

#: gconf/gconftool.c:851
msgid "Must specify a key or keys to get\n"
msgstr ""

#: gconf/gconftool.c:886
#, c-format
msgid "Type: %s\n"
msgstr ""

#: gconf/gconftool.c:887
#, c-format
msgid "List Type: %s\n"
msgstr ""

#: gconf/gconftool.c:888
#, c-format
msgid "Car Type: %s\n"
msgstr ""

#: gconf/gconftool.c:889
#, c-format
msgid "Cdr Type: %s\n"
msgstr ""

#: gconf/gconftool.c:894
#, c-format
msgid "Default Value: %s\n"
msgstr ""

#: gconf/gconftool.c:894 gconf/gconftool.c:896 gconf/gconftool.c:897
#: gconf/gconftool.c:898
msgid "Unset"
msgstr ""

#: gconf/gconftool.c:896
#, c-format
msgid "Owner: %s\n"
msgstr ""

#: gconf/gconftool.c:897
#, c-format
msgid "Short Desc: %s\n"
msgstr ""

#: gconf/gconftool.c:898
#, c-format
msgid "Long Desc: %s\n"
msgstr ""

#: gconf/gconftool.c:907
#, c-format
msgid "No value set for `%s'\n"
msgstr ""

#: gconf/gconftool.c:911
#, c-format
msgid "Failed to get value for `%s': %s\n"
msgstr ""

#: gconf/gconftool.c:954 gconf/gconftool.c:966
#, c-format
msgid "Don't understand type `%s'\n"
msgstr ""

#: gconf/gconftool.c:978
msgid "Must specify alternating keys/values as arguments\n"
msgstr ""

#: gconf/gconftool.c:998
#, c-format
msgid "No value to set for key: `%s'\n"
msgstr ""

#: gconf/gconftool.c:1026
msgid "Cannot set schema as value\n"
msgstr ""

#: gconf/gconftool.c:1036
msgid "When setting a list you must specify a primitive list-type\n"
msgstr ""

#: gconf/gconftool.c:1050
msgid ""
"When setting a pair you must specify a primitive car-type and cdr-type\n"
msgstr ""

#: gconf/gconftool.c:1065
#, c-format
msgid "Error: %s\n"
msgstr ""

#: gconf/gconftool.c:1078 gconf/gconftool.c:1211
#, c-format
msgid "Error setting value: %s"
msgstr ""

#: gconf/gconftool.c:1096 gconf/gconftool.c:1225
#, c-format
msgid "Error syncing: %s"
msgstr ""

#: gconf/gconftool.c:1114
msgid "Must specify key (schema name) as the only argument\n"
msgstr ""

#: gconf/gconftool.c:1156
msgid "List type must be a primitive type: string, int, float or bool\n"
msgstr ""

#: gconf/gconftool.c:1176
msgid "Pair car type must be a primitive type: string, int, float or bool\n"
msgstr ""

#: gconf/gconftool.c:1196
msgid "Pair cdr type must be a primitive type: string, int, float or bool\n"
msgstr ""

#: gconf/gconftool.c:1240
msgid "Must specify one or more dirs to get key/value pairs from.\n"
msgstr ""

#: gconf/gconftool.c:1254
msgid "Must specify one or more keys to unset.\n"
msgstr ""

#: gconf/gconftool.c:1265
#, c-format
msgid "Error unsetting `%s': %s\n"
msgstr ""

#: gconf/gconftool.c:1288
msgid "Must specify one or more dirs to get subdirs from.\n"
msgstr ""

#: gconf/gconftool.c:1322
#, c-format
msgid "Error listing dirs: %s\n"
msgstr ""

#: gconf/gconftool.c:1364
#, c-format
msgid "WARNING: invalid or missing type for schema (%s)\n"
msgstr ""

#: gconf/gconftool.c:1373
#, c-format
msgid "WARNING: invalid or missing list_type for schema (%s)\n"
msgstr ""

#: gconf/gconftool.c:1384 gconf/gconftool.c:1414 gconf/gconftool.c:1443
#, c-format
msgid "WARNING: Failed to parse default value `%s' for schema (%s)\n"
msgstr ""

#: gconf/gconftool.c:1402
#, c-format
msgid "WARNING: invalid or missing car_type or cdr_type for schema (%s)\n"
msgstr ""

#: gconf/gconftool.c:1427
msgid "WARNING: You cannot set a default value for a schema\n"
msgstr ""

#: gconf/gconftool.c:1456
msgid "WARNING: gconftool internal error, unknown GConfValueType\n"
msgstr ""

#: gconf/gconftool.c:1504 gconf/gconftool.c:1525 gconf/gconftool.c:1546
#: gconf/gconftool.c:1567
#, c-format
msgid "WARNING: failed to parse type name `%s'\n"
msgstr ""

#: gconf/gconftool.c:1521
#, c-format
msgid ""
"WARNING: list_type can only be int, float, string or bool and not `%s'\n"
msgstr ""

#: gconf/gconftool.c:1542
#, c-format
msgid "WARNING: car_type can only be int, float, string or bool and not `%s'\n"
msgstr ""

#: gconf/gconftool.c:1563
#, c-format
msgid "WARNING: cdr_type can only be int, float, string or bool and not `%s'\n"
msgstr ""

#: gconf/gconftool.c:1590
msgid "WARNING: empty <applyto> node"
msgstr ""

#: gconf/gconftool.c:1593 gconf/gconftool.c:1856
#, c-format
msgid "WARNING: node <%s> not understood below <schema>\n"
msgstr ""

#: gconf/gconftool.c:1603
msgid "WARNING: no key specified for schema\n"
msgstr ""

#: gconf/gconftool.c:1636
msgid "WARNING: <locale> node has no `name=\"locale\"' attribute, ignoring\n"
msgstr ""

#: gconf/gconftool.c:1642
#, c-format
msgid ""
"WARNING: multiple <locale> nodes for locale `%s', ignoring all past first\n"
msgstr ""

#: gconf/gconftool.c:1723
#, c-format
msgid "WARNING: Invalid node <%s> in a <locale> node\n"
msgstr ""

#: gconf/gconftool.c:1752
#, c-format
msgid "WARNING: failed to install schema `%s' locale `%s': %s\n"
msgstr ""

#: gconf/gconftool.c:1760
#, c-format
msgid "Installed schema `%s' for locale `%s'\n"
msgstr ""

#: gconf/gconftool.c:1782
#, c-format
msgid "WARNING: failed to associate schema `%s' with key `%s': %s\n"
msgstr ""

#: gconf/gconftool.c:1790
#, c-format
msgid "Attached schema `%s' to key `%s'\n"
msgstr ""

#: gconf/gconftool.c:1869
msgid "You must have at least one <locale> entry in a <schema>\n"
msgstr ""

#: gconf/gconftool.c:1898
#, c-format
msgid "WARNING: node <%s> not understood below <schemalist>\n"
msgstr ""

#: gconf/gconftool.c:1920
#, c-format
msgid "Failed to open `%s': %s\n"
msgstr ""

#: gconf/gconftool.c:1927
#, c-format
msgid "Document `%s' is empty?\n"
msgstr ""

#: gconf/gconftool.c:1939
#, c-format
msgid ""
"Document `%s' has the wrong type of root node (<%s>, should be "
"<gconfschemafile>)\n"
msgstr ""

#: gconf/gconftool.c:1952
#, c-format
msgid "Document `%s' has no top level <gconfschemafile> node\n"
msgstr ""

#: gconf/gconftool.c:1966
#, c-format
msgid "WARNING: node <%s> below <gconfschemafile> not understood\n"
msgstr ""

#: gconf/gconftool.c:1977 gconf/gconftool.c:2009
#, c-format
msgid "Error syncing config data: %s"
msgstr ""

#: gconf/gconftool.c:1993
msgid "Must specify some schema files to install\n"
msgstr ""

#: gconf/gconftool.c:2030
#, c-format
msgid ""
"\n"
"%s\n"
msgstr ""

#: gconf/gconftool.c:2050
#, c-format
msgid "Failed to unset breakage key %s: %s\n"
msgstr ""

#: gconf/gconftool.c:2176
msgid "Must specify some keys to break\n"
msgstr ""

#: gconf/gconftool.c:2182
#, c-format
msgid ""
"Trying to break your application by setting bad values for key:\n"
"  %s\n"
msgstr ""

#: gconf/gconftool.c:2200
msgid "Must specify some directories to break\n"
msgstr ""

#: gconf/gconftool.c:2219
#, c-format
msgid ""
"Trying to break your application by setting bad values for keys in "
"directory:\n"
"  %s\n"
msgstr ""

#: wrappers/gtk/gconf-client.c:287 wrappers/gtk/gconf-client.c:304
#, c-format
msgid "GConf Error: %s\n"
msgstr ""

#: wrappers/gtk/gconf-client.c:795
#, c-format
msgid "GConf warning: failure listing pairs in `%s': %s"
msgstr ""

#: wrappers/gtk/gconf-client.c:997
#, c-format
msgid "Expected `%s' got `%s' for key %s"
msgstr ""
