# SMJansson

SMJansson is a SourceMod extension that lets plugins read and write JSON. It wraps the C library
[Jansson](https://github.com/akheron/jansson) (version 2.15.1, bundled in `jansson/`). With it a plugin can:

- turn JSON text or a JSON file into values it can read,
- build JSON values (objects, arrays, strings, numbers, booleans, null) and change them,
- turn those values back into JSON text or write them to a file.

Every JSON value is a SourceMod `Handle`. The functions are plain natives named `json_*`, with the same names and
meanings as in the Jansson C library. There are no methodmaps.

## Contents

- [Installation](#installation)
- [Using SMJansson in a plugin](#using-smjansson-in-a-plugin)
- [Rules you need to know](#rules-you-need-to-know)
  - [Rule 1: close every handle you get](#rule-1-close-every-handle-you-get)
  - [Rule 2: functions ending in _new close the value for you](#rule-2-functions-ending-in-_new-close-the-value-for-you)
  - [Rule 3: a value taken from an object or array is shared, not copied](#rule-3-a-value-taken-from-an-object-or-array-is-shared-not-copied)
  - [Rule 4: a missing key is not the same as JSON null](#rule-4-a-missing-key-is-not-the-same-as-json-null)
  - [How errors are reported](#how-errors-are-reported)
  - [Numbers: integers, reals and their limits](#numbers-integers-reals-and-their-limits)
  - [Strings, UTF-8 and buffer sizes](#strings-utf-8-and-buffer-sizes)
  - [Order of object keys](#order-of-object-keys)
  - [File paths](#file-paths)
- [Examples](#examples)
- [Function reference](#function-reference)
- [Troubleshooting](#troubleshooting)
- [What SMJansson cannot do](#what-smjansson-cannot-do)
- [Building from source](#building-from-source)
- [Making a release](#making-a-release)
- [License](#license)

## Installation

You need SourceMod on the game server. SMJansson needs no other extension and no configuration. The same files work
for every game SourceMod supports.

1. Download the archive for your server's operating system from the
   [releases](https://github.com/davenonymous/SMJansson/releases): `smjansson-<version>-linux.tar.gz` for Linux or
   `smjansson-<version>-windows.zip` for Windows.
2. Extract it into the game folder, the folder that contains `addons/` (for Team Fortress 2 that is `tf/`). It adds
   these files:

   | File | Purpose |
   |---|---|
   | `addons/sourcemod/extensions/smjansson.ext.so` (Linux) or `.dll` (Windows) | The extension for 32-bit servers. |
   | `addons/sourcemod/extensions/x64/smjansson.ext.so` (Linux) or `.dll` (Windows) | The extension for 64-bit servers. |
   | `addons/sourcemod/scripting/include/smjansson.inc` | The include file for compiling plugins. The server does not need it. |
   | `addons/sourcemod/licenses/smjansson/` | The license texts of SMJansson and Jansson. |

   SourceMod uses the binary that matches the server: a 32-bit server the one in `extensions/`, a 64-bit server the
   one in `extensions/x64/`. Keeping both is fine.

3. Copy `smjansson.inc` to `addons/sourcemod/scripting/include/` on the machine where you compile your plugins, if
   that is not the server.

The Linux binaries need glibc 2.29 or newer (any Linux distribution released since 2019). To build the files
yourself instead, see [Building from source](#building-from-source).

You do not need to load the extension yourself. SourceMod loads it the first time a plugin that includes
`smjansson.inc` is loaded.

To check that it is loaded, run `sm exts list` in the server console. The list contains a line like this:

```
[05] SMJansson (2.15.1/1): JSON parser/writer
```

## Using SMJansson in a plugin

Include the file after `sourcemod`:

```sourcepawn
#include <sourcemod>
#include <smjansson>
```

By default the extension is **required**: if it is missing on the server, your plugin does not load.

To make it **optional**, include it like this and check `LibraryExists("jansson")` before you call any `json_*`
function:

```sourcepawn
#include <sourcemod>
#undef REQUIRE_EXTENSIONS
#include <smjansson>
#define REQUIRE_EXTENSIONS

public void OnPluginStart()
{
    if (!LibraryExists("jansson"))
    {
        LogMessage("SMJansson is not installed, JSON features are off.");
        return;
    }
    // ... json_* calls are safe here
}
```

The library name for `LibraryExists`, `OnLibraryAdded` and `OnLibraryRemoved` is `jansson`.

## Rules you need to know

### Rule 1: close every handle you get

Every function that returns a `Handle` gives you a **new handle that you own**. Close it with `delete` (or
`CloseHandle`) when you no longer need it. If you do not, it stays in memory until your plugin unloads.

These functions return a new handle that you must close:

| Group | Functions |
|---|---|
| Create a value | `json_object`, `json_array`, `json_string`, `json_string_format`, `json_string_format_ex`, `json_integer`, `json_real`, `json_true`, `json_false`, `json_boolean`, `json_null` |
| Read from an object or array | `json_object_get`, `json_array_get`, `json_object_iter_value` |
| Copy | `json_copy`, `json_deep_copy` |
| Parse | `json_load`, `json_load_ex`, `json_load_file`, `json_load_file_ex` |
| Build | `json_pack` |
| Iterate | `json_object_iter`, `json_object_iter_at`, `json_object_iter_next` (see below) |

Iterator handles are special: `json_object_iter_next` closes the iterator you pass to it. If you loop until it returns
`null`, there is nothing to close. If you stop the loop early (`break` or `return`), close the iterator yourself. See
[Example 6](#example-6-loop-over-all-keys-of-an-object).

Closing a handle never changes the JSON data that other handles point to. See Rule 3.

### Rule 2: functions ending in _new close the value for you

These functions put a value into an object or array: `json_object_set`, `json_array_set`, `json_array_append`,
`json_array_insert`, `json_object_iter_set`. Each one has a twin ending in `_new`. So do the functions that merge one
object into another: `json_object_update`, `json_object_update_existing` and `json_object_update_missing`.

| Function | What happens to the value handle you pass |
|---|---|
| without `_new`, e.g. `json_array_append` | Nothing. You still own it and must close it. |
| with `_new`, e.g. `json_array_append_new` | **On success** (returns `true`) it is closed for you. Do not use or close it again. **On failure** (returns `false`) it is **not** closed; you still own it. |

The `_new` form exists so you can create and insert a value in one line without keeping the handle:

```sourcepawn
json_object_set_new(hObj, "name", json_string("Scout"));
```

Using a handle after a `_new` function closed it is an error (`Invalid <JSON Object> handle ... (error 3)`).

### Rule 3: a value taken from an object or array is shared, not copied

`json_object_get`, `json_array_get` and `json_object_iter_value` return a handle to the **same** value that is
inside the object or array, not to a copy. This means:

- If you change the value through that handle (for example add a key to a nested object), the change is visible in
  the parent.
- Closing that handle does not remove the value from the parent.
- Closing the parent does not break that handle. The value stays alive until the last handle to it is closed.

The same is true in the other direction: `json_object_set` and the other functions without `_new` put the value
itself into the parent, not a copy. If you change it afterwards through your own handle, the parent changes too.

If you need an independent copy, use `json_deep_copy`.

### Rule 4: a missing key is not the same as JSON null

In SourcePawn, `null` means "no handle" (the same as `INVALID_HANDLE`). It has nothing to do with the JSON value
`null`.

| JSON text | `json_object_get(hObj, "team")` returns |
|---|---|
| `{}` (key missing) | `null` (no handle). Nothing to close. |
| `{"team": null}` | A handle to a JSON null value: `json_is_null()` is `true`. You must close it. |

The convenience stocks `json_object_get_int`, `json_object_get_string` and the other `json_object_get_*` and
`json_array_get_*` stocks **require the key or index to exist**. If it does not exist, they raise an error and your
function stops. See [Example 4](#example-4-read-a-key-that-might-be-missing) for a safe way to read optional keys.

### How errors are reported

There are four ways a call can fail. Which one applies is listed for each function in the
[Function reference](#function-reference).

1. **Return value.** The function returns `false`, `-1`, `0`, `0.0` or `null` (no handle). Nothing is logged.
   Example: `json_array_remove` with an index that does not exist returns `false`.
2. **Error that stops your function.** SourceMod writes `[SM] Exception reported: ...` with a call stack to the
   error log (`addons/sourcemod/logs/errors_<date>.log`) and the rest of the current function (the callback, timer,
   command handler and so on) does not run. Your plugin keeps running. This happens when you pass `null`, a closed
   handle or a handle that is not a JSON value to a `json_*` function. The message is
   `Invalid <JSON Object> handle 0 (error 4)` for `null` and `Invalid <JSON Object> handle <hex> (error 3)` for a
   closed handle. (Some functions say `<Object>`, `<Array>`, `<String>`, `<Integer>` or `<Real>` instead of
   `<JSON Object>`.)
3. **Error logged by SMJansson.** `json_load` and `json_load_file` write the parse error to the same error log, for
   example `[SMJSON] Error in line 1, col 4: string or '}' expected near 'bad'`, and return `null`. Use
   `json_load_ex` or `json_load_file_ex` to get the error text yourself instead.
4. **Plugin stops.** `json_pack` with an unknown character in its format string calls `SetFailState`. Your plugin is
   stopped until it is reloaded. Only `json_pack` does this.

### Numbers: integers, reals and their limits

JSON has one number type. SMJansson, like Jansson, has two:

- **integer** (`JSON_INTEGER`): a number without a decimal point or exponent, such as `42` or `-7`.
- **real** (`JSON_REAL`): a number with a decimal point or exponent, such as `1.5`, `1.0` or `1e3`. Note that `1.0`
  and `1e3` are reals, not integers.

What this means for you:

- `json_integer_value` returns `0` for a real, and `json_real_value` returns `0.0` for an integer. To read a number
  that might be either, use `json_number_value`. It returns a `float` for both. Or parse the text with
  `JSON_DECODE_INT_AS_REAL`, which makes every number a real (see [Parsing JSON text](#parsing-json-text)).
- `json_equal` treats the integer `1` and the real `1.0` as **different**.
- SourcePawn integers have 32 bits. JSON text can contain larger integers. Jansson keeps them intact (they are
  written back unchanged by `json_dump`), but `json_integer_value` returns only their lowest 32 bits: `2147483648`
  is read as `-2147483648`, and `1099511627776` (2 to the power of 40) is read as `0`. If you need such numbers, for
  example 64-bit Steam IDs, store them as strings.
- SourcePawn floats have 32 bits, so most decimal fractions are not exact. Reals are written with the fewest digits
  that read back as exactly the same number, and for a 32-bit float that is often more digits than you typed:
  `json_real(0.1)` is written as `0.10000000149011612` and `json_real(-444.556)` as `-444.5559997558594`. To write
  fewer digits, use `json_dump_ex` with `JSON_REAL_PRECISION(7)`; then `json_real(0.1)` is written as `0.1` (see
  [Example 10](#example-10-compact-output-fewer-digits-and-single-values)).
- A real is always written with a decimal point: `json_real(2.0)` is written as `2.0`, and `1e3` read from text is
  written back as `1000.0`.

### Strings, UTF-8 and buffer sizes

All JSON strings and object keys are UTF-8. Strings that come from SourceMod (player names, chat, file contents) are
already UTF-8, so you do not need to convert anything.

If you pass text that is not valid UTF-8 to `json_string`, you get a handle that holds no value: adding it to an
object or array fails (returns `false`) and `json_typeof` raises the error `JSON Object is NULL.`.

Functions that copy text into a buffer are `json_string_value`, `json_object_iter_key`, `json_dump`,
`json_dump_ex`, `json_object_get_string` and `json_array_get_string`. They all behave the same way:

- If the text does not fit, it is cut off so that it fits into the buffer (at most `maxlength - 1` bytes plus the
  terminating zero). It is never cut in the middle of a UTF-8 character.
- The return value is the length of the **whole** text in bytes, not the length that was copied. So you can detect
  that the text was cut off:

```sourcepawn
char sJSON[1024];
int iLength = json_dump(hObj, sJSON, sizeof(sJSON));
if (iLength >= sizeof(sJSON))
{
    // sJSON holds only the first 1023 bytes; a buffer of iLength + 1 would be needed.
}
```

### Order of object keys

Object keys are kept in the order they were added. For parsed JSON text that is the order in the text. This order is
used by `json_object_iter` loops and by every function that writes JSON.

To write the keys in alphabetical order (byte by byte) instead, pass `bSortKeys = true` to `json_dump` or
`json_dump_file`, or `JSON_SORT_KEYS` to `json_dump_ex` or `json_dump_file_ex`. `json_object_iter` loops always use
the order the keys were added.

The last parameter of `json_dump` and `json_dump_file`, `bPreserveOrder`, has no effect. It is only there so that
older plugins still compile.

### File paths

`json_load_file`, `json_load_file_ex`, `json_dump_file` and `json_dump_file_ex` take a path **relative to the game
directory**, the
directory that contains `addons/` (for Team Fortress 2 that is `tf/`). This is the same as SourceMod's own file
functions such as `OpenFile` and `FileExists`.

The easiest way to get a correct path is SourceMod's `BuildPath`:

```sourcepawn
char sPath[PLATFORM_MAX_PATH];
BuildPath(Path_SM, sPath, sizeof(sPath), "configs/myplugin.json");
// sPath is now "addons/sourcemod/configs/myplugin.json"
```

More details:

- An absolute path does not work as it is. Prefix it with `file://` to use one, for example
  `file:///home/steam/settings.json`.
- Do not use the `%` character in file names. The path is treated as a format string, so `%` sequences are replaced.
- `json_dump_file` and `json_dump_file_ex` overwrite an existing file. They do not create missing directories; if
  the directory does not exist, they return `false`.

## Examples

All examples are complete and have been compiled and run on a Team Fortress 2 server. The output shown is the real
output.

### Example 1: a complete plugin that writes and reads JSON

<!-- example: 1 -->
```sourcepawn
#include <sourcemod>
#include <smjansson>

public void OnPluginStart()
{
    // Build the object {"map": "cp_badlands", "rounds": 5}
    Handle hRoot = json_object();
    json_object_set_new(hRoot, "map", json_string("cp_badlands"));
    json_object_set_new(hRoot, "rounds", json_integer(5));

    // Turn it into text on one line (indentation 0)
    char sJSON[256];
    json_dump(hRoot, sJSON, sizeof(sJSON), 0);
    PrintToServer("%s", sJSON);
    delete hRoot;

    // Turn the text back into values
    Handle hParsed = json_load(sJSON);
    if (hParsed == null)
    {
        return; // json_load wrote the reason to the SourceMod error log
    }

    char sMap[64];
    json_object_get_string(hParsed, "map", sMap, sizeof(sMap));
    PrintToServer("map = %s, rounds = %d", sMap, json_object_get_int(hParsed, "rounds"));
    delete hParsed;
}
```

Output:

```
{"map": "cp_badlands", "rounds": 5}
map = cp_badlands, rounds = 5
```

### Example 2: build nested JSON with every value type

<!-- example: 2 -->
```sourcepawn
void PrintMatchJSON()
{
    Handle hMatch = json_object();
    json_object_set_new(hMatch, "map", json_string("cp_process_final"));
    json_object_set_new(hMatch, "duration", json_real(1800.5));
    json_object_set_new(hMatch, "overtime", json_false());
    json_object_set_new(hMatch, "winner", json_null());

    Handle hScores = json_array();
    json_array_append_new(hScores, json_integer(3));
    json_array_append_new(hScores, json_integer(2));
    json_object_set_new(hMatch, "scores", hScores);   // hScores is closed now, do not use it again

    Handle hServer = json_object();
    json_object_set_new(hServer, "name", json_string("My Server"));
    json_object_set_new(hServer, "port", json_integer(27015));
    json_object_set_new(hMatch, "server", hServer);   // hServer is closed now, do not use it again

    // Indent with 2 spaces
    char sJSON[1024];
    json_dump(hMatch, sJSON, sizeof(sJSON), 2);
    PrintToServer("%s", sJSON);

    delete hMatch;
}
```

Output:

```
{
  "map": "cp_process_final",
  "duration": 1800.5,
  "overtime": false,
  "winner": null,
  "scores": [
    3,
    2
  ],
  "server": {
    "name": "My Server",
    "port": 27015
  }
}
```

### Example 3: parse JSON text and read every value type

<!-- example: 3 -->
```sourcepawn
void ReadPlayerJSON()
{
    char sText[] = "{\"name\": \"Scout\", \"kills\": 12, \"accuracy\": 0.25, \"medic\": false, \"team\": null}";

    // json_load_ex gives you the error instead of writing it to the error log
    char sError[256];
    int iLine, iColumn;
    Handle hPlayer = json_load_ex(sText, sError, sizeof(sError), iLine, iColumn);
    if (hPlayer == null)
    {
        LogError("Invalid JSON (line %d, column %d): %s", iLine, iColumn, sError);
        return;
    }

    // The json_object_get_* stocks need the key to exist (see Example 4)
    char sName[64];
    json_object_get_string(hPlayer, "name", sName, sizeof(sName));
    int iKills = json_object_get_int(hPlayer, "kills");
    float fAccuracy = json_object_get_float(hPlayer, "accuracy");
    bool bMedic = json_object_get_bool(hPlayer, "medic");
    PrintToServer("%s: %d kills, accuracy %.2f, medic: %s", sName, iKills, fAccuracy, bMedic ? "yes" : "no");

    // "team" exists, but its value is JSON null
    Handle hTeam = json_object_get(hPlayer, "team");
    if (json_is_null(hTeam))
    {
        PrintToServer("team is null");
    }
    delete hTeam;

    delete hPlayer;
}
```

Output:

```
Scout: 12 kills, accuracy 0.25, medic: no
team is null
```

### Example 4: read a key that might be missing

The `json_object_get_*` stocks raise an error when the key does not exist. To read a key that may be missing, ask
for the value with `json_object_get` first; it returns `null` for a missing key. These two helpers return a default
value instead:

<!-- example: 4 -->
```sourcepawn
// Returns the integer stored under sKey, or iDefault if the key is missing or not an integer.
int GetIntOr(Handle hObj, const char[] sKey, int iDefault)
{
    Handle hValue = json_object_get(hObj, sKey);
    if (hValue == null)
    {
        return iDefault;
    }

    int iResult = json_is_integer(hValue) ? json_integer_value(hValue) : iDefault;
    delete hValue;
    return iResult;
}

// Copies the string stored under sKey into sBuffer, or sDefault if the key is missing or not a string.
void GetStringOr(Handle hObj, const char[] sKey, char[] sBuffer, int iMaxLength, const char[] sDefault)
{
    Handle hValue = json_object_get(hObj, sKey);
    if (hValue == null || json_string_value(hValue, sBuffer, iMaxLength) == -1)
    {
        strcopy(sBuffer, iMaxLength, sDefault);
    }
    delete hValue;
}

void ReadOptionalKeys()
{
    Handle hSettings = json_load("{\"rounds\": 3}");

    char sGreeting[64];
    GetStringOr(hSettings, "greeting", sGreeting, sizeof(sGreeting), "Welcome!");
    PrintToServer("rounds = %d, timeout = %d, greeting = %s",
        GetIntOr(hSettings, "rounds", 5), GetIntOr(hSettings, "timeout", 30), sGreeting);

    delete hSettings;
}
```

Output:

```
rounds = 3, timeout = 30, greeting = Welcome!
```

`delete` on a `null` handle does nothing, so `delete hValue` is safe in `GetStringOr` even when the key is missing.

### Example 5: load a config file, fill in defaults, loop over an array

The file `addons/sourcemod/configs/myplugin.json`:

<!-- file: addons/sourcemod/configs/myplugin.json -->
```json
{
    "admins": ["STEAM_0:1:1234", "STEAM_0:0:5678"],
    "messages": {
        "welcome": "Hello and have fun!"
    }
}
```

<!-- example: 5 -->
```sourcepawn
void LoadMyConfig()
{
    char sPath[PLATFORM_MAX_PATH];
    BuildPath(Path_SM, sPath, sizeof(sPath), "configs/myplugin.json");

    char sError[256];
    int iLine, iColumn;
    Handle hFile = json_load_file_ex(sPath, sError, sizeof(sError), iLine, iColumn);
    if (hFile == null)
    {
        // A missing file is reported here too, with line and column -1
        LogError("Could not load %s (line %d, column %d): %s", sPath, iLine, iColumn, sError);
        return;
    }

    // Start with the defaults and put the file on top of them. Nested objects are merged
    // key by key, so "messages" keeps the default "goodbye" next to the "welcome" of the file.
    Handle hConfig = json_load("{\"max_rounds\": 5, \"admins\": [], \"messages\": {\"welcome\": \"Welcome!\", \"goodbye\": \"Bye!\"}}");
    json_object_update_recursive(hConfig, hFile);
    delete hFile;

    PrintToServer("max_rounds: %d", json_object_get_int(hConfig, "max_rounds"));

    Handle hMessages = json_object_get(hConfig, "messages");
    char sText[128];
    json_object_get_string(hMessages, "welcome", sText, sizeof(sText));
    PrintToServer("welcome: %s", sText);
    json_object_get_string(hMessages, "goodbye", sText, sizeof(sText));
    PrintToServer("goodbye: %s", sText);
    delete hMessages;

    // Loop over an array
    Handle hAdmins = json_object_get(hConfig, "admins");
    int iCount = json_array_size(hAdmins);
    for (int i = 0; i < iCount; i++)
    {
        char sSteamId[32];
        json_array_get_string(hAdmins, i, sSteamId, sizeof(sSteamId));
        PrintToServer("admin %d: %s", i, sSteamId);
    }
    delete hAdmins;

    delete hConfig;
}
```

Output:

```
max_rounds: 5
welcome: Hello and have fun!
goodbye: Bye!
admin 0: STEAM_0:1:1234
admin 1: STEAM_0:0:5678
```

### Example 6: loop over all keys of an object

<!-- example: 6 -->
```sourcepawn
void PrintKeysAndTypes(Handle hObj)
{
    Handle hIter = json_object_iter(hObj);
    while (hIter != null)
    {
        char sKey[128];
        if (json_object_iter_key(hIter, sKey, sizeof(sKey)) == -1)
        {
            // The object is empty: the iterator points at nothing.
            // We leave the loop early, so we close the iterator ourselves.
            delete hIter;
            break;
        }

        Handle hValue = json_object_iter_value(hIter);
        char sType[16];
        Stringify_json_type(json_typeof(hValue), sType, sizeof(sType));
        PrintToServer("%s is %s", sKey, sType);
        delete hValue;

        // Closes hIter and returns the next iterator, or null after the last key
        hIter = json_object_iter_next(hObj, hIter);
    }
}

void LoopExample()
{
    Handle hObj = json_load("{\"name\": \"Heavy\", \"health\": 300}");
    PrintKeysAndTypes(hObj);
    delete hObj;

    Handle hEmpty = json_object();
    PrintKeysAndTypes(hEmpty);   // prints nothing
    delete hEmpty;
}
```

Output:

```
name is String
health is Integer
```

The check of `json_object_iter_key` is needed: for an empty object, `json_object_iter` still returns a handle (not
`null`), and that handle points at nothing.

### Example 7: change, remove and merge values

<!-- example: 7 -->
```sourcepawn
void ChangeValues()
{
    Handle hStats = json_load("{\"kills\": 10, \"deaths\": 4, \"tmp\": true, \"weapons\": [\"scattergun\"]}");

    // Replace a value: setting an existing key replaces its value
    json_object_set_new(hStats, "kills", json_integer(11));

    // Remove a key
    json_object_del(hStats, "tmp");

    // Change a nested value in place: hWeapons is the same array that is inside hStats (Rule 3)
    Handle hWeapons = json_object_get(hStats, "weapons");
    json_array_append_new(hWeapons, json_string("pistol"));
    delete hWeapons;

    // Merge another object in: its keys overwrite or add to hStats
    Handle hUpdate = json_load("{\"deaths\": 5, \"assists\": 2}");
    json_object_update(hStats, hUpdate);
    delete hUpdate;

    char sJSON[512];
    json_dump(hStats, sJSON, sizeof(sJSON), 0);
    PrintToServer("%s", sJSON);

    delete hStats;
}
```

Output:

```
{"kills": 11, "deaths": 5, "weapons": ["scattergun", "pistol"], "assists": 2}
```

### Example 8: save JSON to a file

<!-- example: 8 -->
```sourcepawn
void SaveStats()
{
    Handle hStats = json_object();
    json_object_set_new(hStats, "matches", json_integer(42));
    json_object_set_new(hStats, "last_map", json_string("koth_product_final"));

    char sPath[PLATFORM_MAX_PATH];
    BuildPath(Path_SM, sPath, sizeof(sPath), "data/myplugin-stats.json");

    // Written with 4 spaces of indentation, the default
    if (!json_dump_file(hStats, sPath))
    {
        LogError("Could not write %s", sPath);
    }

    delete hStats;
}
```

The file `addons/sourcemod/data/myplugin-stats.json` then contains:

```json
{
    "matches": 42,
    "last_map": "koth_product_final"
}
```

### Example 9: build JSON in one call with json_pack

`json_pack` builds a whole structure from a format string and an `ArrayList` of values. See
[json_pack](#json_pack) for all format characters.

<!-- example: 9 -->
```sourcepawn
void PackExample()
{
    // One entry per key and value, in the order they appear in the format string.
    // Strings need a block size big enough for the longest string.
    ArrayList hValues = new ArrayList(ByteCountToCells(64));
    hValues.PushString("map");          // key
    hValues.PushString("cp_gullywash_final1");
    hValues.PushString("players");      // key
    hValues.Push(12);
    hValues.PushString("ratio");        // key
    hValues.Push(0.5);
    hValues.PushString("scores");       // key
    hValues.Push(3);
    hValues.Push(1);
    hValues.PushString("live");         // key
    hValues.Push(true);

    Handle hJSON = json_pack("{s:s, s:i, s:f, s:[ii], s:b}", hValues);
    delete hValues;

    char sJSON[256];
    json_dump(hJSON, sJSON, sizeof(sJSON), 0);
    PrintToServer("%s", sJSON);
    delete hJSON;
}
```

Output:

```
{"map": "cp_gullywash_final1", "players": 12, "ratio": 0.5, "scores": [3, 1], "live": true}
```

### Example 10: compact output, fewer digits and single values

`json_dump_ex` takes its options as flags combined with `|`. See [json_dump_ex](#json_dump_ex) for all flags.

<!-- example: 10 -->
```sourcepawn
void WriteCompactJSON()
{
    Handle hShot = json_object();
    json_object_set_new(hShot, "x", json_real(0.1));
    json_object_set_new(hShot, "y", json_real(-2.75));
    json_object_set_new(hShot, "demo", json_string("demos/match.dem"));

    char sJSON[256];

    // No flags: one line, reals with every digit of the float
    json_dump_ex(hShot, sJSON, sizeof(sJSON), 0);
    PrintToServer("%s", sJSON);

    // No spaces, at most 7 significant digits, "/" written as "\/"
    json_dump_ex(hShot, sJSON, sizeof(sJSON), JSON_COMPACT | JSON_REAL_PRECISION(7) | JSON_ESCAPE_SLASH);
    PrintToServer("%s", sJSON);

    // Pretty-printed with 2 spaces and sorted keys
    json_dump_ex(hShot, sJSON, sizeof(sJSON), JSON_INDENT(2) | JSON_SORT_KEYS);
    PrintToServer("%s", sJSON);

    delete hShot;

    // A value that is not an object or array needs JSON_ENCODE_ANY
    Handle hName = json_string("Scout");
    json_dump_ex(hName, sJSON, sizeof(sJSON), JSON_ENCODE_ANY);
    PrintToServer("%s", sJSON);
    delete hName;
}
```

Output:

```
{"x": 0.10000000149011612, "y": -2.75, "demo": "demos/match.dem"}
{"x":0.1,"y":-2.75,"demo":"demos\/match.dem"}
{
  "demo": "demos/match.dem",
  "x": 0.10000000149011612,
  "y": -2.75
}
"Scout"
```

### Example 11: parse options and error codes

The parse functions take flags as an optional parameter. `json_load_ex` and `json_load_file_ex` also tell you why
parsing failed, as a `json_error_code`. See [Parsing JSON text](#parsing-json-text) for all flags.

<!-- example: 11 -->
```sourcepawn
void ParseWithOptions()
{
    // A single number is accepted with JSON_DECODE_ANY
    Handle hNumber = json_load("42", JSON_DECODE_ANY);
    PrintToServer("number: %d", json_integer_value(hNumber));
    delete hNumber;

    // Refuse a key that appears twice, and find out why parsing failed
    char sError[256];
    int iLine, iColumn;
    json_error_code iErrorCode;
    Handle hPlayer = json_load_ex("{\"name\": \"Scout\", \"name\": \"Spy\"}", sError, sizeof(sError),
        iLine, iColumn, JSON_REJECT_DUPLICATES, iErrorCode);
    if (hPlayer == null)
    {
        if (iErrorCode == json_error_duplicate_key)
        {
            PrintToServer("duplicate key (line %d, column %d): %s", iLine, iColumn, sError);
        }
        return;
    }
    delete hPlayer;
}
```

Output:

```
number: 42
duplicate key (line 1, column 24): duplicate object key near '"name"'
```

## Function reference

Every heading below is the exact function name, so you can search this file for a name. The signatures are the ones
in `smjansson.inc`. "Error" means an [error that stops your function](#how-errors-are-reported).

Unless stated otherwise, every function raises an error when a handle parameter is `null`, closed, or not a JSON
value from SMJansson.

Quick index:

| I want to... | Use |
|---|---|
| parse JSON text | [json_load](#json_load), [json_load_ex](#json_load_ex), with [parse flags](#parse-flags) |
| parse a JSON file | [json_load_file](#json_load_file), [json_load_file_ex](#json_load_file_ex) |
| turn values into JSON text | [json_dump](#json_dump), [json_dump_ex](#json_dump_ex) |
| write a JSON file | [json_dump_file](#json_dump_file), [json_dump_file_ex](#json_dump_file_ex) |
| find out why parsing failed | [json_error_code](#json_error_code) |
| find out the type of a value | [json_typeof](#json_typeof), [json_is_object and the other json_is_ macros](#json_is_object-and-the-other-json_is_-macros) |
| create a value | [json_object](#json_object), [json_array](#json_array), [json_string](#json_string), [json_integer](#json_integer), [json_real](#json_real), [json_boolean](#json_boolean), [json_null](#json_null) |
| read a plain value | [json_string_value](#json_string_value), [json_integer_value](#json_integer_value), [json_real_value](#json_real_value), [json_number_value](#json_number_value), [json_boolean_value](#json_boolean_value) |
| read a value of an object in one call | [json_object_get_string](#json_object_get_string), [json_object_get_int](#json_object_get_int), [json_object_get_float](#json_object_get_float), [json_object_get_bool](#json_object_get_bool) |
| read an element of an array in one call | [json_array_get_string](#json_array_get_string), [json_array_get_int](#json_array_get_int), [json_array_get_float](#json_array_get_float), [json_array_get_bool](#json_array_get_bool) |
| work with an object | [json_object_get](#json_object_get), [json_object_set](#json_object_set), [json_object_del](#json_object_del), [json_object_size](#json_object_size) |
| loop over an object | [json_object_iter](#json_object_iter), [json_object_iter_next](#json_object_iter_next) |
| work with an array | [json_array_get](#json_array_get), [json_array_append](#json_array_append), [json_array_insert](#json_array_insert), [json_array_remove](#json_array_remove), [json_array_size](#json_array_size) |
| merge objects | [json_object_update](#json_object_update), [json_object_update_existing](#json_object_update_existing), [json_object_update_missing](#json_object_update_missing), [json_object_update_recursive](#json_object_update_recursive) |
| join arrays | [json_array_extend](#json_array_extend) |
| compare or copy | [json_equal](#json_equal), [json_copy](#json_copy), [json_deep_copy](#json_deep_copy) |
| build a structure in one call | [json_pack](#json_pack) |

### Types

#### json_type

```sourcepawn
enum json_type {
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_INTEGER,
    JSON_REAL,
    JSON_TRUE,
    JSON_FALSE,
    JSON_NULL
}
```

The type of a JSON value. `true` and `false` are two separate types; there is no single boolean type.

#### json_typeof

```sourcepawn
json_type json_typeof(Handle hObj)
```

Returns the type of the value. Error if `hObj` is `null`, closed or not a JSON value. Also an error
(`JSON Object is NULL.`) for the empty handle that `json_string` returns for invalid UTF-8.

#### json_is_object and the other json_is_ macros

```sourcepawn
json_is_object(hObj)    // JSON_OBJECT
json_is_array(hObj)     // JSON_ARRAY
json_is_string(hObj)    // JSON_STRING
json_is_integer(hObj)   // JSON_INTEGER
json_is_real(hObj)      // JSON_REAL
json_is_true(hObj)      // JSON_TRUE
json_is_false(hObj)     // JSON_FALSE
json_is_null(hObj)      // JSON_NULL
json_is_number(hObj)    // JSON_INTEGER or JSON_REAL
json_is_boolean(hObj)   // JSON_TRUE or JSON_FALSE
```

Return `true` if the value has that type. These are macros that call `json_typeof`, so they raise the same errors:
they cannot be used to test whether a handle is `null`. Check `hObj == null` first.

To read a boolean value, use `json_boolean_value`.

#### json_boolean_value

```sourcepawn
json_boolean_value(hObj)
```

Returns `true` if the value is `true`, and `false` for `false` and every other type. This is a macro for
`json_is_true`, so it raises the same errors.

#### Stringify_json_type

```sourcepawn
bool Stringify_json_type(json_type input, char[] output, int maxlength)
```

Writes the name of a type into `output`: `Object`, `Array`, `String`, `Integer`, `Real`, `True`, `False` or `Null`.
Returns `false` if `input` is not a `json_type`. Useful for log messages.

### Creating values

Each of these returns a new handle that you must close (or pass to a `_new` function).

#### json_object

```sourcepawn
Handle json_object()
```

Returns a new, empty object (`{}`).

#### json_array

```sourcepawn
Handle json_array()
```

Returns a new, empty array (`[]`).

#### json_string

```sourcepawn
Handle json_string(const char[] sValue)
```

Returns a new string with the text `sValue`. `sValue` must be valid UTF-8; otherwise the returned handle holds no
value (see [Strings, UTF-8 and buffer sizes](#strings-utf-8-and-buffer-sizes)).

#### json_string_format

```sourcepawn
Handle json_string_format(const char[] sFormat, any ...)
```

Like `json_string`, but formats the text first, with the same rules as SourceMod's `Format`. The formatted text is
cut off after 4095 bytes; use `json_string_format_ex` for longer text.

#### json_string_format_ex

```sourcepawn
Handle json_string_format_ex(int tmpBufferLength, const char[] sFormat, any ...)
```

Like `json_string_format`, with a buffer of `tmpBufferLength` bytes instead of 4096.

#### json_integer

```sourcepawn
Handle json_integer(int iValue)
```

Returns a new integer.

#### json_real

```sourcepawn
Handle json_real(float fValue)
```

Returns a new real. See [Numbers](#numbers-integers-reals-and-their-limits) for how it is written.

#### json_true

```sourcepawn
Handle json_true()
```

Returns a new handle to the value `true`.

#### json_false

```sourcepawn
Handle json_false()
```

Returns a new handle to the value `false`.

#### json_boolean

```sourcepawn
Handle json_boolean(bool bState)
```

Returns `json_true()` if `bState` is `true`, otherwise `json_false()`.

#### json_null

```sourcepawn
Handle json_null()
```

Returns a new handle to the value `null`.

### Reading and changing plain values

#### json_string_value

```sourcepawn
int json_string_value(Handle hString, char[] sValueBuffer, int maxlength)
```

Copies the text of a string into `sValueBuffer`. Returns the length of the whole text in bytes (see
[buffer sizes](#strings-utf-8-and-buffer-sizes)). Returns `-1` if the value is not a string; the buffer is then not
changed.

#### json_string_set

```sourcepawn
bool json_string_set(Handle hString, char[] sValue)
```

Changes the text of a string. Returns `false` if the value is not a string.

#### json_integer_value

```sourcepawn
int json_integer_value(Handle hInteger)
```

Returns the value of an integer. Returns `0` if the value is not an integer, **including reals**. Integers outside
the 32-bit range lose their upper bits (see [Numbers](#numbers-integers-reals-and-their-limits)).

#### json_integer_set

```sourcepawn
bool json_integer_set(Handle hInteger, int iValue)
```

Changes the value of an integer. Returns `false` if the value is not an integer.

#### json_real_value

```sourcepawn
float json_real_value(Handle hReal)
```

Returns the value of a real. Returns `0.0` if the value is not a real, **including integers**.

#### json_real_set

```sourcepawn
bool json_real_set(Handle hReal, float value)
```

Changes the value of a real. Returns `false` if the value is not a real.

#### json_number_value

```sourcepawn
float json_number_value(Handle hNumber)
```

Returns the value of an integer or a real as a `float`. Returns `0.0` for any other type. Use this when a number may
be written either way in the JSON text (`5` or `5.0`).

### Objects

#### json_object_size

```sourcepawn
int json_object_size(Handle hObj)
```

Returns the number of keys. Returns `0` if the value is not an object.

#### json_object_get

```sourcepawn
Handle json_object_get(Handle hObj, const char[] sKey)
```

Returns a new handle to the value stored under `sKey`. The value is shared with the object (Rule 3). Returns `null`
if the key does not exist or `hObj` is not an object.

The key is used as it is: `json_object_get(hObj, "a.b")` looks for a key named `a.b`, not for `b` inside `a`. To
reach a nested value, call `json_object_get` once per level and close each handle.

#### json_object_set

```sourcepawn
bool json_object_set(Handle hObj, const char[] sKey, Handle hValue)
```

Stores `hValue` under `sKey`. If the key exists, its value is replaced. `hValue` is not copied (Rule 3) and stays
yours to close. Returns `false` if `hObj` is not an object.

#### json_object_set_new

```sourcepawn
bool json_object_set_new(Handle hObj, const char[] sKey, Handle hValue)
```

Like `json_object_set`, but closes `hValue` on success (Rule 2). Returns `false` if `hObj` is not an object or
`sKey` is not valid UTF-8; `hValue` is then not closed.

#### json_object_del

```sourcepawn
bool json_object_del(Handle hObj, const char[] sKey)
```

Removes `sKey` and its value. Returns `false` if the key does not exist or `hObj` is not an object. Handles you
still hold to the removed value stay valid.

#### json_object_clear

```sourcepawn
bool json_object_clear(Handle hObj)
```

Removes all keys. Returns `false` if `hObj` is not an object.

#### json_object_update

```sourcepawn
bool json_object_update(Handle hObj, Handle hOther)
```

Copies every key of `hOther` into `hObj`. Keys that exist in both get the value from `hOther`. The values are shared,
not copied (Rule 3). `hOther` is not changed. Returns `false` if either value is not an object.

#### json_object_update_existing

```sourcepawn
bool json_object_update_existing(Handle hObj, Handle hOther)
```

Like `json_object_update`, but only for keys that already exist in `hObj`. No key is added.

#### json_object_update_missing

```sourcepawn
bool json_object_update_missing(Handle hObj, Handle hOther)
```

Like `json_object_update`, but only for keys that do not exist in `hObj` yet. No existing value is changed. Nested
objects are not merged; for that use `json_object_update_recursive`.

#### json_object_update_recursive

```sourcepawn
bool json_object_update_recursive(Handle hObj, Handle hOther)
```

Like `json_object_update`, but merges nested objects: where a key holds an object in both `hObj` and `hOther`, that
object is updated the same way, key by key, at every level. All other values of `hOther` replace the ones in `hObj`.
The values are shared, not copied (Rule 3). `hOther` is not changed. Returns `false` if either value is not an
object. Useful for filling in defaults (see
[Example 5](#example-5-load-a-config-file-fill-in-defaults-loop-over-an-array)).

#### json_object_update_new

```sourcepawn
bool json_object_update_new(Handle hObj, Handle hOther)
```

Like `json_object_update`, but closes `hOther` on success (Rule 2). On failure `hOther` is not closed.

#### json_object_update_existing_new

```sourcepawn
bool json_object_update_existing_new(Handle hObj, Handle hOther)
```

Like `json_object_update_existing`, but closes `hOther` on success (Rule 2).

#### json_object_update_missing_new

```sourcepawn
bool json_object_update_missing_new(Handle hObj, Handle hOther)
```

Like `json_object_update_missing`, but closes `hOther` on success (Rule 2).

### Reading object values with the convenience stocks

These stocks read one value of an object in one call and close the handle they use internally. **The key must exist.**
If it does not, they raise the error `Invalid <JSON Object> handle 0 (error 4)`. To read keys that may be missing, see
[Example 4](#example-4-read-a-key-that-might-be-missing).

#### json_object_get_string

```sourcepawn
int json_object_get_string(Handle hObj, const char[] sKey, char[] sBuffer, int maxlength)
```

Copies the text of the string under `sKey` into `sBuffer` and returns the length of the whole text. Returns `-1` if
the value is not a string; the buffer is then not changed.

#### json_object_get_int

```sourcepawn
int json_object_get_int(Handle hObj, const char[] sKey)
```

Returns the integer under `sKey`. Returns `0` if the value is not an integer, including reals.

#### json_object_get_float

```sourcepawn
float json_object_get_float(Handle hObj, const char[] sKey)
```

Returns the number under `sKey` as a `float`. Works for integers and reals. Returns `0.0` for any other type.

#### json_object_get_bool

```sourcepawn
bool json_object_get_bool(Handle hObj, const char[] sKey)
```

Returns `true` if the value under `sKey` is `true`. Returns `false` for `false` and for every other type.

### Looping over an object

An iterator is a position inside an object. It is a handle of its own. The complete pattern is
[Example 6](#example-6-loop-over-all-keys-of-an-object).

Rules:

- `json_object_iter` and `json_object_iter_at` always return an iterator handle, also for an empty object, a value
  that is not an object, or a key that does not exist. Such an iterator points at nothing: `json_object_iter_key`
  returns `-1` and `json_object_iter_value` returns `null`. Always check the return value of `json_object_iter_key`.
- Do not add or remove keys of the object while you loop over it. Changing the value at the current position with
  `json_object_iter_set` or `json_object_iter_set_new` is fine.
- The keys come in the order they were added (see [Order of object keys](#order-of-object-keys)).

#### json_object_iter

```sourcepawn
Handle json_object_iter(Handle hObj)
```

Returns an iterator at the first key of `hObj`.

#### json_object_iter_at

```sourcepawn
Handle json_object_iter_at(Handle hObj, const char[] key)
```

Returns an iterator at the key `key`. Moving on from there with `json_object_iter_next` visits `key` and the keys
that were added after it, not all keys.

#### json_object_iter_next

```sourcepawn
Handle json_object_iter_next(Handle hObj, Handle hIter)
```

**Closes `hIter`** and returns an iterator at the next key, or `null` after the last key. `hObj` must be the object
that `hIter` belongs to. Use it as `hIter = json_object_iter_next(hObj, hIter);`.

#### json_object_iter_key

```sourcepawn
int json_object_iter_key(Handle hIter, char[] sKeyBuffer, int maxlength)
```

Copies the key at the iterator into `sKeyBuffer` and returns the length of the whole key. Returns `-1` if the iterator
points at nothing; the buffer is then not changed.

#### json_object_iter_value

```sourcepawn
Handle json_object_iter_value(Handle hIter)
```

Returns a new handle to the value at the iterator (shared, Rule 3), or `null` if the iterator points at nothing.

#### json_object_iter_set

```sourcepawn
bool json_object_iter_set(Handle hObj, Handle hIter, Handle hValue)
```

Replaces the value at the iterator with `hValue`. `hValue` stays yours to close. Returns `false` if `hObj` is not an
object or the iterator points at nothing.

#### json_object_iter_set_new

```sourcepawn
bool json_object_iter_set_new(Handle hObj, Handle hIter, Handle hValue)
```

Like `json_object_iter_set`, but closes `hValue` on success (Rule 2).

### Arrays

Array positions (indexes) start at `0`. The last element is at `json_array_size(hArray) - 1`.

#### json_array_size

```sourcepawn
int json_array_size(Handle hArray)
```

Returns the number of elements. Returns `0` if the value is not an array.

#### json_array_get

```sourcepawn
Handle json_array_get(Handle hArray, int iIndex)
```

Returns a new handle to the element at `iIndex` (shared, Rule 3). Returns `null` if there is no element at `iIndex`
or `hArray` is not an array.

#### json_array_set

```sourcepawn
bool json_array_set(Handle hArray, int iIndex, Handle hValue)
```

Replaces the element at `iIndex` with `hValue`. `iIndex` must point at an existing element (`0` to size - 1); to add
an element use `json_array_append` or `json_array_insert`. `hValue` stays yours to close. Returns `false` if there is
no element at `iIndex` or `hArray` is not an array.

#### json_array_set_new

```sourcepawn
bool json_array_set_new(Handle hArray, int iIndex, Handle hValue)
```

Like `json_array_set`, but closes `hValue` on success (Rule 2).

#### json_array_append

```sourcepawn
bool json_array_append(Handle hArray, Handle hValue)
```

Adds `hValue` at the end. `hValue` stays yours to close. Returns `false` if `hArray` is not an array.

#### json_array_append_new

```sourcepawn
bool json_array_append_new(Handle hArray, Handle hValue)
```

Like `json_array_append`, but closes `hValue` on success (Rule 2).

#### json_array_insert

```sourcepawn
bool json_array_insert(Handle hArray, int iIndex, Handle hValue)
```

Inserts `hValue` at `iIndex`; the element that was there and all after it move one position back. `iIndex` can be
`0` to size; size adds at the end. `hValue` stays yours to close. Returns `false` if `iIndex` is outside that range.

#### json_array_insert_new

```sourcepawn
bool json_array_insert_new(Handle hArray, int iIndex, Handle hValue)
```

Like `json_array_insert`, but closes `hValue` on success (Rule 2).

#### json_array_remove

```sourcepawn
bool json_array_remove(Handle hArray, int iIndex)
```

Removes the element at `iIndex`; all elements after it move one position forward. Returns `false` if there is no
element at `iIndex`.

#### json_array_clear

```sourcepawn
bool json_array_clear(Handle hArray)
```

Removes all elements. Returns `false` if `hArray` is not an array.

#### json_array_extend

```sourcepawn
bool json_array_extend(Handle hArray, Handle hOther)
```

Appends all elements of `hOther` to the end of `hArray`. The elements are shared, not copied (Rule 3). `hOther` is not
changed. Returns `false` if either value is not an array.

### Reading array elements with the convenience stocks

These work like the [object stocks](#reading-object-values-with-the-convenience-stocks), with an index instead of a
key. **The index must exist** (`0` to size - 1). If it does not, they raise the error
`Invalid <JSON Object> handle 0 (error 4)`.

#### json_array_get_string

```sourcepawn
int json_array_get_string(Handle hArray, int iIndex, char[] sBuffer, int maxlength)
```

Copies the text of the string at `iIndex` and returns the length of the whole text. Returns `-1` if the element is not
a string; the buffer is then not changed.

#### json_array_get_int

```sourcepawn
int json_array_get_int(Handle hArray, int iIndex)
```

Returns the integer at `iIndex`, or `0` if the element is not an integer (including reals).

#### json_array_get_float

```sourcepawn
float json_array_get_float(Handle hArray, int iIndex)
```

Returns the integer or real at `iIndex` as a `float`, or `0.0` for any other type.

#### json_array_get_bool

```sourcepawn
bool json_array_get_bool(Handle hArray, int iIndex)
```

Returns `true` if the element at `iIndex` is `true`, otherwise `false`.

### Comparing and copying

#### json_equal

```sourcepawn
bool json_equal(Handle hObj, Handle hOther)
```

Returns `true` if both values are equal:

- integers with integers and reals with reals, by value. An integer is never equal to a real (`1` is not `1.0`);
- strings byte by byte;
- arrays if they have the same length and equal elements at the same positions;
- objects if they have the same keys with equal values; the order of the keys does not matter;
- `true`, `false` and `null` if both have the same type.

#### json_copy

```sourcepawn
Handle json_copy(Handle hObj)
```

Returns a shallow copy: a new object or array whose values are **shared** with the original. Adding or removing
keys or elements in the copy does not change the original, but changing a nested object or array changes it in
both. For a fully independent copy use `json_deep_copy`.

#### json_deep_copy

```sourcepawn
Handle json_deep_copy(Handle hObj)
```

Returns a complete, independent copy: nested objects and arrays are copied as well.

### Parsing JSON text

By default the text must contain exactly one object or one array at the top level. Text such as `"hello"`, `42` or
an empty string is rejected, and so is text after the closing `}` or `]`. The flags below change that.

#### Parse flags

Every parse function takes an optional `iFlags` parameter. Combine flags with `|`.

| Flag | Effect |
|---|---|
| `JSON_DECODE_ANY` | Accept any value at the top level, also a string, number, `true`, `false` or `null`. |
| `JSON_DISABLE_EOF_CHECK` | Stop after the first value and ignore the rest of the text. |
| `JSON_REJECT_DUPLICATES` | Fail if an object has the same key twice. Without it, the last value wins. |
| `JSON_DECODE_INT_AS_REAL` | Make every number a real, also `42`. |

Any other value raises the error `Unsupported flags <hex>`.

#### json_error_code

```sourcepawn
enum json_error_code {
    json_error_unknown,
    json_error_out_of_memory,
    json_error_stack_overflow,
    json_error_cannot_open_file,
    json_error_invalid_argument,
    json_error_invalid_utf8,
    json_error_premature_end_of_input,
    json_error_end_of_input_expected,
    json_error_invalid_syntax,
    json_error_invalid_format,
    json_error_wrong_type,
    json_error_null_character,
    json_error_null_value,
    json_error_null_byte_in_key,
    json_error_duplicate_key,
    json_error_numeric_overflow,
    json_error_item_not_found,
    json_error_index_out_of_range
}
```

Why parsing failed, as returned by `json_load_ex` and `json_load_file_ex`. Parsing returns these values:

| Value | Meaning |
|---|---|
| `json_error_out_of_memory` | Not enough memory. |
| `json_error_stack_overflow` | The text is nested deeper than 2048 levels. |
| `json_error_cannot_open_file` | The file cannot be opened. |
| `json_error_invalid_utf8` | The text is not valid UTF-8. |
| `json_error_premature_end_of_input` | The text ends in the middle of a value. |
| `json_error_end_of_input_expected` | There is more text after the value. |
| `json_error_invalid_syntax` | The text is not valid JSON, or its top level is not an object or array. |
| `json_error_null_character` | A string contains `\u0000`. |
| `json_error_null_byte_in_key` | An object key contains `\u0000`. |
| `json_error_duplicate_key` | A key appears twice in an object (only with `JSON_REJECT_DUPLICATES`). |
| `json_error_numeric_overflow` | A number is too big. |

The other values are not returned by parsing.

#### json_load

```sourcepawn
Handle json_load(const char[] sJSON, int iFlags = 0)
```

Parses `sJSON` and returns a new handle to the value. On failure returns `null` and writes the error to the
SourceMod error log, for example:

```
[SMJSON] Error in line 1, col 15: '[' or '{' expected near '"just a string"'
```

#### json_load_ex

```sourcepawn
Handle json_load_ex(const char[] sJSON, char[] sErrorText, int maxlen, int &iLine, int &iColumn, int iFlags = 0, json_error_code &iErrorCode = json_error_unknown)
```

Like `json_load`, but on failure nothing is logged. Instead the error is stored in `sErrorText` (for example
`':' expected near '2'`), `iLine` and `iColumn` are set to its position (both start at 1), and `iErrorCode`, if you
pass it, is set to the reason (see [json_error_code](#json_error_code)). On success these parameters are not changed.

#### json_load_file

```sourcepawn
Handle json_load_file(const char sFilePath[PLATFORM_MAX_PATH], int iFlags = 0)
```

Like `json_load`, but reads the file `sFilePath` (see [File paths](#file-paths)). A missing file is reported in the
error log like a parse error:

```
[SMJSON] Error in line -1, col -1: unable to open /home/tf2/server/tf/does/not/exist.json: No such file or directory
```

#### json_load_file_ex

```sourcepawn
Handle json_load_file_ex(const char sFilePath[PLATFORM_MAX_PATH], char[] sErrorText, int maxlen, int &iLine, int &iColumn, int iFlags = 0, json_error_code &iErrorCode = json_error_unknown)
```

Like `json_load_file`, but returns the error like `json_load_ex` does. For a file that cannot be opened, `iLine` and
`iColumn` are `-1`, `sErrorText` starts with `unable to open` and `iErrorCode` is `json_error_cannot_open_file`.

### Writing JSON text

#### json_dump

```sourcepawn
int json_dump(Handle hObject, char[] sJSON, int maxlength, int iIndentWidth = 4, bool bEnsureAscii = false, bool bSortKeys = false, bool bPreserveOrder = false)
```

Writes `hObject` as JSON text into `sJSON` and returns the length of the whole text in bytes (see
[buffer sizes](#strings-utf-8-and-buffer-sizes)). Returns `-1` if `hObject` is not an object or array; only objects
and arrays can be written.

| Parameter | Meaning |
|---|---|
| `iIndentWidth` | `1` to `31`: one value per line, indented by this many spaces per level. `0`: everything on one line, as `{"a": 1, "b": [1, 2]}`. Only values from `0` to `31` work: larger values are cut to their lowest 5 bits, so `32` acts like `0`. For output without the spaces after `:` and `,`, use `json_dump_ex` with `JSON_COMPACT`. |
| `bEnsureAscii` | `true`: every character outside ASCII is written as an escape, for example `ü` as `\u00FC`. |
| `bSortKeys` | `true`: keys in alphabetical order. `false`: keys in the order they were added. See [Order of object keys](#order-of-object-keys). |
| `bPreserveOrder` | No effect. Kept so that older plugins still compile. |

#### json_dump_file

```sourcepawn
bool json_dump_file(Handle hObject, const char[] sFilePath, int iIndentWidth = 4, bool bEnsureAscii = false, bool bSortKeys = false, bool bPreserveOrder = false)
```

Writes `hObject` as JSON text to the file `sFilePath` (see [File paths](#file-paths)), with the same options as
`json_dump`. An existing file is overwritten. Returns `true` on success. Returns `false` if `hObject` is not an
object or array, or if the file cannot be written (for example because its directory does not exist).

#### json_dump_ex

```sourcepawn
int json_dump_ex(Handle hObject, char[] sJSON, int maxlength, int iFlags)
```

Like `json_dump`, with the options as flags. Combine them with `|`; `0` means one line, no options. See
[Example 10](#example-10-compact-output-fewer-digits-and-single-values).

| Flag | Effect |
|---|---|
| `JSON_INDENT(n)` | One value per line, indented by `n` spaces per level, `n` from `1` to `31`. |
| `JSON_COMPACT` | No space after `:` and `,`, as `{"a":1,"b":[1,2]}`. |
| `JSON_ENSURE_ASCII` | Every character outside ASCII is written as an escape, for example `ü` as `\u00FC`. |
| `JSON_SORT_KEYS` | Keys in alphabetical order instead of the order they were added. |
| `JSON_REAL_PRECISION(n)` | Reals with at most `n` significant digits, `n` from `1` to `31`. `json_real(0.1)` with `n` = `7` is written as `0.1`. |
| `JSON_ESCAPE_SLASH` | `/` is written as `\/`. |
| `JSON_ENCODE_ANY` | Values that are not an object or array can be written too, such as a single string: `"Scout"`. |

Any other value raises the error `Unsupported flags <hex>`.

Returns the length of the whole text in bytes, like `json_dump`. Returns `-1` if the value cannot be written: a
value that is not an object or array without `JSON_ENCODE_ANY`.

#### json_dump_file_ex

```sourcepawn
bool json_dump_file_ex(Handle hObject, const char[] sFilePath, int iFlags)
```

Like `json_dump_file`, with the flags of `json_dump_ex`. Returns `true` on success, `false` if the value cannot be
written (see `json_dump_ex`) or the file cannot be written.

### Building a structure in one call

#### json_pack

```sourcepawn
Handle json_pack(const char[] sPackString, ArrayList hParams)
```

Builds an object or array from the format string `sPackString` and the values in `hParams`, and returns a new handle
to it. See [Example 9](#example-9-build-json-in-one-call-with-json_pack).

Format characters:

| Character | Builds | Takes from `hParams` |
|---|---|---|
| `{` ... `}` | an object. Inside, keys and values alternate: every key is `s`, followed by the format of its value. | nothing |
| `[` ... `]` | an array of the values inside | nothing |
| `s` | a string (or, as a key in an object, the key) | one string, added with `PushString` |
| `i` | an integer | one `int`, added with `Push` |
| `f` or `r` | a real | one `float`, added with `Push` |
| `b` | `true` or `false` | one `bool`, added with `Push` |
| `n` | `null` | nothing |
| space, `:` and `,` | nothing; use them for readability | nothing |

Objects and arrays can be nested in any depth: `{s:[ii], s:{s:s}}`.

Things to know:

- `hParams` must contain exactly the values the format string asks for, in the same order. Create it with a block
  size big enough for your strings, for example `new ArrayList(ByteCountToCells(64))`. Strings and keys longer than
  254 bytes are cut off.
- `json_pack` **removes the values it uses from `hParams`**. Afterwards the list is empty. You still have to close it.
- If `hParams` has too few values, `json_pack` raises an error (`Invalid index 0 (count: 0)`).
- If a key in an object is not `s`, `json_pack` logs `Object keys must be strings at <position>.` and returns `null`.
- If the format string contains any other character, `json_pack` calls `SetFailState`: **your plugin is stopped**
  with the message `Invalid pack String '<format>'. Type '<character>' not supported at <position>`. Use only fixed
  format strings that you have tested.

## Troubleshooting

Search this section for the message you see.

**`Invalid <JSON Object> handle 0 (error 4)`** (or `<Object>`, `<Array>`, `<String>`, `<Integer>`, `<Real>`)
A `json_*` function got `null` instead of a handle. Common causes: `json_load` failed and its result was used without
checking for `null`; `json_object_get` or `json_array_get` returned `null` for a missing key or index; a
`json_object_get_*` or `json_array_get_*` stock was used with a key or index that does not exist (see
[Rule 4](#rule-4-a-missing-key-is-not-the-same-as-json-null)).

**`Invalid <JSON Object> handle <hex number> (error 3)`**
The handle was already closed. Common causes: it was passed to a `_new` function before (see
[Rule 2](#rule-2-functions-ending-in-_new-close-the-value-for-you)); it was an iterator that `json_object_iter_next`
closed; it was closed twice.

**`JSON Object is NULL.`**
The value was created by `json_string` with text that is not valid UTF-8 (see
[Strings, UTF-8 and buffer sizes](#strings-utf-8-and-buffer-sizes)).

**`[SMJSON] Error in line ..., col ...: ...`** in the error log
`json_load` or `json_load_file` could not parse the text or open the file. The message says where and why. A message
with line `-1` means the file could not be opened; check the path (see [File paths](#file-paths)).

**`'[' or '{' expected near ...`**
The text is valid JSON, but its top level is not an object or array. Pass `JSON_DECODE_ANY` to accept it (see
[Parse flags](#parse-flags)).

**`Unsupported flags <hex>`**
A parse function, `json_dump_ex` or `json_dump_file_ex` got a flag it does not know. Use only the flags listed in
[Parse flags](#parse-flags) and [json_dump_ex](#json_dump_ex).

**`Object keys must be strings at ...`** or **`Invalid pack String ...`**
The format string of `json_pack` is wrong (see [json_pack](#json_pack)).

**`Required extension "SMJansson" file("smjansson.ext") not running`**
Your plugin does not load because SMJansson is not installed, or the binary for your server's architecture is
missing: a 64-bit server needs `extensions/x64/` (see [Installation](#installation)). Run `sm exts list` to see
whether it is loaded.

**The output of `json_dump` is cut off**
The buffer is too small. Compare the return value with the buffer size (see
[Strings, UTF-8 and buffer sizes](#strings-utf-8-and-buffer-sizes)).

**A number reads as 0**
`json_integer_value` was used on a real (for example `5.0`), or `json_real_value` on an integer. Use
`json_number_value` (see [Numbers](#numbers-integers-reals-and-their-limits)).

**Finding handles that are never closed**
Run `sm_dump_handles <file>` in the server console. SMJansson's handles are listed with the types `JanssonObject`
and `JanssonIterator`, with the plugin that owns them.

## What SMJansson cannot do

- Read integers beyond 32 bits as numbers (store them as strings).
- Keep strings that contain the character `\u0000` (SourcePawn strings end there).
- Look up nested values by a path such as `a.b.c`.
- Unpack JSON into variables in one call (`json_unpack` of the Jansson C library is not available).

## Building from source

SMJansson uses only SourceMod's extension interface, so it is built once per operating system and architecture, not
once per game; it needs neither Metamod:Source nor a game SDK.

You need:

- Python 3 and [AMBuild](https://github.com/alliedmodders/ambuild) 2.2 or later,
- a C++17 compiler: clang 5 or later or GCC 9 or later on Linux (with the 32-bit libraries, `gcc-multilib` and
  `g++-multilib` on Debian and Ubuntu, for 32-bit builds), Visual Studio 2017 15.7 or later on Windows,
- a source checkout of [SourceMod](https://github.com/alliedmodders/sourcemod) with its submodules, branch
  `1.12-dev`.

Steps on Linux, with `/path/to/sourcemod` replaced by your path:

```
git clone https://github.com/davenonymous/SMJansson.git
cd SMJansson/build
python3 ../configure.py --enable-optimize --targets=x86,x86_64 --sm-path=/path/to/sourcemod
ambuild
```

On Windows the steps are the same in a command prompt (`python` instead of `python3`). AMBuild finds Visual Studio
by itself. If you start it from a Visual Studio developer command prompt instead, it uses only that prompt's
architecture: then build `--targets=x86` and `--targets=x86_64` in two separate build directories, each from the
matching prompt.

- The `build/` directory already exists in the repository; configure and build inside it.
- `--targets` is `x86` for 32-bit servers, `x86_64` for 64-bit servers, or both.
- The bundled Jansson in `jansson/` is found automatically. `--jansson-path` points to another copy of Jansson
  2.15.1.
- Jansson's build settings are in `jansson-config/` (`jansson_config.h` and `jansson_private_config.h`), written by
  hand in place of the ones Jansson's own build would generate, so that `jansson/` stays an unchanged copy of the
  release. Builds with clang on Linux are tested on Team Fortress 2 servers; builds with Visual Studio 2022 on
  Windows compile and link, but have not been run on a server.

The result is in `build/package/`, laid out like a server's `addons/sourcemod/` directory (`.dll` instead of `.so`
on Windows):

```
build/package/addons/sourcemod/extensions/smjansson.ext.so       (32-bit)
build/package/addons/sourcemod/extensions/x64/smjansson.ext.so   (64-bit)
build/package/addons/sourcemod/scripting/include/smjansson.inc
build/package/addons/sourcemod/scripting/include/test.inc
build/package/addons/sourcemod/scripting/smjansson.test.sp
```

`smjansson.test.sp` is a test plugin that calls every native and prints the results to the server console. It needs
`test.inc`.

## Making a release

The GitHub Actions workflow in `.github/workflows/ci.yml` builds Linux and Windows on every push and pull request and
keeps the archives as workflow artifacts. Pushing a tag that starts with `v` also publishes a release with both
archives:

1. Set the version in `sdk/smsdk_config.h`, for example `#define SMEXT_CONF_VERSION "2.15.1/1"` (Jansson version,
   then the revision of SMJansson for that Jansson version).
2. Commit and push.
3. Tag the commit with the same version, written as `v<Jansson version>-<revision>`, and push the tag:

```
git tag v2.15.1-1
git push origin v2.15.1-1
```

The release is only published if the tag matches `SMEXT_CONF_VERSION` ("v2.15.1-1" and "2.15.1/1"); otherwise the
workflow fails. Its notes list the changes since the previous release.

## License

SMJansson is licensed under the same terms as SourceMod: the GNU General Public License version 3 with SourceMod's
special exceptions. See [LICENSE.txt](LICENSE.txt), [licenses/GPLv3.txt](licenses/GPLv3.txt) and
[licenses/GPLv2.txt](licenses/GPLv2.txt).

Jansson, bundled in `jansson/`, is licensed under the MIT license, except `jansson/src/dtoa.c`, which has its own
permissive license. See [jansson/LICENSE](jansson/LICENSE).
