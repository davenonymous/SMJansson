/**
 * Copyright 2026 davenonymous
 *
 * The tests of smjansson.inc, run with smrunner (sm-runner) against the
 * built extension:
 *
 *     smrunner test test_smjansson.smx --extensions <dist>/extensions
 *
 * Every native and stock is called as a plugin calls it, and checked
 * against what the include documents: the values, the ownership of the
 * Handles (which natives close the value they are given), the return
 * values on failure, the decoding and encoding flags, the error codes and
 * the native errors.
 */

// no critic (ProhibitLongFiles)
#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <smjansson>
#include <smrunner>

/* ------------------------------------------------------------------ helpers */

/* Runs failing, which must stop with a native error containing text. */
void ExpectNativeError(Function failing, const char[] text) {
	Runner.ExpectError(text);
	Call_StartFunction(null, failing);
	Call_Finish();
}

/* The Handle the failing functions below pass to a native. */
Handle g_handle;

/* The ArrayList of the tests that need one where a JSON value belongs. */
ArrayList g_list;

/* Encodes value compactly with sorted keys, any value at the top level. */
void Encode(Handle value, char[] buffer, int maxlength) {
	int length = json_dump_ex(value, buffer, maxlength, JSON_COMPACT | JSON_SORT_KEYS | JSON_ENCODE_ANY);
	Assert.True(length >= 0 && length < maxlength, "the value can be encoded into %d bytes", maxlength);
}

/* Fails unless value encodes (compactly, keys sorted) to expected. */
void AssertEncodes(Handle value, const char[] expected) {
	char json[1024];
	Encode(value, json, sizeof(json));
	Assert.StrEqual(json, expected);
}

/* Decodes text, which must be valid JSON of any kind. */
Handle Decode(const char[] text) {
	char error[256];
	int line;
	int column;
	Handle value = json_load_ex(text, error, sizeof(error), line, column, JSON_DECODE_ANY);
	Assert.True(value != null, "%s decodes: %s", text, error);
	return value;
}

/* A string of the single byte 0xFF, which is not UTF-8. */
void InvalidUtf8(char[] buffer, int maxlength) {
	Assert.True(maxlength >= 2);
	buffer[0] = 0xFF;
	buffer[1] = '\0';
}

/* Whether a file of SourceMod's error log contains text. */
bool ErrorLogContains(const char[] text) {
	char logs[PLATFORM_MAX_PATH];
	BuildPath(Path_SM, logs, sizeof(logs), "logs");
	DirectoryListing listing = OpenDirectory(logs);
	if (listing == null) {
		return false;
	}

	bool found = false;
	char name[PLATFORM_MAX_PATH];
	FileType type;
	while (!found && listing.GetNext(name, sizeof(name), type)) {
		if (type == FileType_File && StrContains(name, "errors_") == 0) {
			found = FileContains(logs, name, text);
		}
	}
	delete listing;
	return found;
}

bool FileContains(const char[] folder, const char[] name, const char[] text) {
	char path[PLATFORM_MAX_PATH];
	Format(path, sizeof(path), "%s/%s", folder, name);
	File file = OpenFile(path, "r");
	if (file == null) {
		return false;
	}

	bool found = false;
	char line[1024];
	while (!found && file.ReadLine(line, sizeof(line))) {
		found = StrContains(line, text) != -1;
	}
	delete file;
	return found;
}

/* Reads the whole text file at path (relative to the game folder). */
void ReadTextFile(const char[] path, char[] buffer, int maxlength) {
	File file = OpenFile(path, "r");
	Assert.True(file != null, "%s can be read", path);
	int length = file.ReadString(buffer, maxlength, -1);
	delete file;
	Assert.True(length >= 0, "%s has text", path);
	buffer[length] = '\0';
}

void WriteTextFile(const char[] path, const char[] text) {
	File file = OpenFile(path, "w");
	Assert.True(file != null, "%s can be written", path);
	file.WriteString(text, false);
	delete file;
}

/* -------------------------------------------------------------------- types */

public void Test_TypeofTellsEveryType() {
	Handle values[8];
	values[0] = json_object();
	values[1] = json_array();
	values[2] = json_string("text");
	values[3] = json_integer(42);
	values[4] = json_real(1.5);
	values[5] = json_true();
	values[6] = json_false();
	values[7] = json_null();

	json_type expected[8] = {JSON_OBJECT, JSON_ARRAY, JSON_STRING, JSON_INTEGER, JSON_REAL, JSON_TRUE, JSON_FALSE,
	                         JSON_NULL};
	for (int i = 0; i < sizeof(values); i++) {
		Assert.Equal(json_typeof(values[i]), expected[i], "value %d", i);
		delete values[i];
	}
}

public void Test_TypeMacrosMatchOnlyTheirType() {
	Handle integer = json_integer(1);
	Handle real = json_real(1.0);
	Handle yes = json_true();
	Handle no = json_false();
	Handle nothing = json_null();

	Assert.True(json_is_integer(integer));
	Assert.False(json_is_real(integer));
	Assert.True(json_is_number(integer));
	Assert.True(json_is_number(real));
	Assert.True(json_is_real(real));
	Assert.False(json_is_number(yes));

	Assert.True(json_is_boolean(yes));
	Assert.True(json_is_boolean(no));
	Assert.False(json_is_boolean(nothing));
	Assert.True(json_is_true(yes));
	Assert.True(json_is_false(no));
	Assert.True(json_is_null(nothing));
	Assert.False(json_is_object(nothing));
	Assert.False(json_is_array(nothing));
	Assert.False(json_is_string(nothing));

	Assert.True(json_boolean_value(yes));
	Assert.False(json_boolean_value(no));
	Assert.False(json_boolean_value(nothing), "null is not true");
	Assert.False(json_boolean_value(integer), "1 is not true");

	delete integer;
	delete real;
	delete yes;
	delete no;
	delete nothing;
}

public void Test_StringifyNamesEveryType() {
	char names[8][8] = {"Object", "Array", "String", "Integer", "Real", "True", "False", "Null"};
	char name[16];
	for (int i = 0; i < sizeof(names); i++) {
		Assert.True(Stringify_json_type(view_as<json_type>(i), name, sizeof(name)));
		Assert.StrEqual(name, names[i]);
	}

	strcopy(name, sizeof(name), "unchanged");
	Assert.False(Stringify_json_type(view_as<json_type>(8), name, sizeof(name)), "8 is no json_type");
	Assert.StrEqual(name, "unchanged");
}

/* ----------------------------------------------------------------- equality */

bool DecodedEqual(const char[] first, const char[] second) {
	Handle a = Decode(first);
	Handle b = Decode(second);
	bool equal = json_equal(a, b);
	delete a;
	delete b;
	return equal;
}

public void Test_EqualComparesTheContents() {
	Assert.True(DecodedEqual("1", "1"));
	Assert.False(DecodedEqual("1", "2"));
	Assert.False(DecodedEqual("1", "1.0"), "an integer is never equal to a real");
	Assert.True(DecodedEqual("1.5", "1.5"));
	Assert.True(DecodedEqual("\"a\"", "\"a\""));
	Assert.False(DecodedEqual("\"a\"", "\"A\""));
	Assert.True(DecodedEqual("[1, [2]]", "[1, [2]]"));
	Assert.False(DecodedEqual("[1, 2]", "[2, 1]"), "the order of elements counts");
	Assert.False(DecodedEqual("[1]", "[1, 1]"));
	Assert.True(DecodedEqual("{\"a\": 1, \"b\": {\"c\": 2}}", "{\"b\": {\"c\": 2}, \"a\": 1}"),
	            "the order of keys does not count");
	Assert.False(DecodedEqual("{\"a\": 1}", "{\"a\": 1, \"b\": 2}"));
	Assert.True(DecodedEqual("true", "true"));
	Assert.True(DecodedEqual("null", "null"));
	Assert.False(DecodedEqual("true", "false"));
	Assert.False(DecodedEqual("null", "false"));
}

/* ------------------------------------------------------------------ copying */

public void Test_ShallowCopySharesTheChildValues() {
	Handle original = Decode("{\"inner\": {\"a\": 1}, \"list\": [1]}");
	Handle copy = json_copy(original);
	Assert.True(copy != original);
	Assert.True(json_equal(copy, original));

	Handle inner = json_object_get(copy, "inner");
	json_object_set_new(inner, "b", json_integer(2));
	delete inner;
	json_object_set_new(copy, "top", json_true());

	AssertEncodes(original, "{\"inner\":{\"a\":1,\"b\":2},\"list\":[1]}");
	AssertEncodes(copy, "{\"inner\":{\"a\":1,\"b\":2},\"list\":[1],\"top\":true}");
	delete original;
	delete copy;
}

public void Test_DeepCopyIsIndependentOfTheOriginal() {
	Handle original = Decode("{\"inner\": {\"a\": 1}, \"list\": [1]}");
	Handle copy = json_deep_copy(original);
	Assert.True(json_equal(copy, original));

	Handle inner = json_object_get(copy, "inner");
	json_object_set_new(inner, "b", json_integer(2));
	delete inner;
	Handle list = json_object_get(copy, "list");
	json_array_append_new(list, json_integer(2));
	delete list;

	AssertEncodes(original, "{\"inner\":{\"a\":1},\"list\":[1]}");
	AssertEncodes(copy, "{\"inner\":{\"a\":1,\"b\":2},\"list\":[1,2]}");
	delete original;
	delete copy;
}

public void Test_CopiesOfScalarsHaveTheSameValue() {
	Handle text = json_string("text");
	Handle copy = json_copy(text);
	Handle deep = json_deep_copy(text);
	Assert.True(json_equal(copy, text));
	Assert.True(json_equal(deep, text));
	delete text;
	delete copy;
	delete deep;
}

/* ------------------------------------------------------------------ objects */

public void Test_ObjectStoresValuesUnderKeys() {
	Handle obj = json_object();
	Assert.Equal(json_object_size(obj), 0);

	Handle one = json_integer(1);
	Assert.True(json_object_set(obj, "one", one));
	Assert.True(json_object_set_new(obj, "a.b", json_string("dotted")));
	Assert.Equal(json_object_size(obj), 2);

	Handle got = json_object_get(obj, "one");
	Assert.Equal(json_integer_value(got), 1);
	delete got;

	got = json_object_get(obj, "a.b");
	char text[16];
	json_string_value(got, text, sizeof(text));
	Assert.StrEqual(text, "dotted", "a.b is a key, not a path");
	delete got;

	Assert.Equal(json_object_get(obj, "a"), INVALID_HANDLE);
	Assert.Equal(json_object_get(obj, "missing"), INVALID_HANDLE);

	Assert.Equal(json_integer_value(one), 1, "json_object_set leaves the value's Handle open");
	delete one;
	delete obj;
}

public void Test_ObjectSetReplacesTheValueOfAKey() {
	Handle obj = json_object();
	json_object_set_new(obj, "key", json_integer(1));
	json_object_set_new(obj, "key", json_string("two"));
	Assert.Equal(json_object_size(obj), 1);
	AssertEncodes(obj, "{\"key\":\"two\"}");
	delete obj;
}

public void Test_ObjectGetReturnsTheStoredValueNotACopy() {
	Handle obj = Decode("{\"count\": 1, \"list\": []}");

	Handle count = json_object_get(obj, "count");
	json_integer_set(count, 2);
	delete count;

	Handle list = json_object_get(obj, "list");
	json_array_append_new(list, json_null());
	delete list;

	AssertEncodes(obj, "{\"count\":2,\"list\":[null]}");
	Assert.Equal(json_object_size(obj), 2, "closing the Handles removed nothing");
	delete obj;
}

public void Test_ObjectSetNewClosesTheValueOnSuccess() {
	Handle obj = json_object();
	g_handle = json_integer(1);
	Assert.True(json_object_set_new(obj, "key", g_handle));
	ExpectNativeError(TypeofOfTheHandle, "Invalid <JSON Object> handle");
	delete obj;
}

void TypeofOfTheHandle() {
	json_typeof(g_handle);
}

public void Test_ObjectSetNewKeepsTheValueOnFailure() {
	Handle array = json_array();
	Handle value = json_integer(1);
	Assert.False(json_object_set_new(array, "key", value), "an array has no keys");
	Assert.Equal(json_integer_value(value), 1, "the value's Handle is still open");

	Handle obj = json_object();
	char key[2];
	InvalidUtf8(key, sizeof(key));
	Assert.False(json_object_set_new(obj, key, value), "a key must be UTF-8");
	Assert.Equal(json_integer_value(value), 1, "the value's Handle is still open");
	Assert.Equal(json_object_size(obj), 0);

	delete value;
	delete array;
	delete obj;
}

public void Test_ObjectNativesFailOnOtherValues() {
	Handle array = json_array();
	Handle value = json_null();
	Handle obj = json_object();
	Assert.Equal(json_object_size(array), 0);
	Assert.Equal(json_object_get(array, "key"), INVALID_HANDLE);
	Assert.False(json_object_set(array, "key", value));
	Assert.False(json_object_del(array, "key"));
	Assert.False(json_object_clear(array));
	Assert.False(json_object_update(array, obj));
	Assert.False(json_object_update(obj, array));
	Assert.False(json_object_update_existing(array, obj));
	Assert.False(json_object_update_missing(obj, array));
	Assert.False(json_object_update_recursive(array, obj));
	delete array;
	delete value;
	delete obj;
}

public void Test_ObjectDelRemovesOneKey() {
	Handle obj = Decode("{\"a\": {\"x\": 1}, \"b\": 2}");
	Handle a = json_object_get(obj, "a");

	Assert.True(json_object_del(obj, "a"));
	Assert.False(json_object_del(obj, "a"), "the key is gone");
	Assert.False(json_object_del(obj, "missing"));
	AssertEncodes(obj, "{\"b\":2}");
	AssertEncodes(a, "{\"x\":1}");

	delete a;
	delete obj;
}

public void Test_ObjectClearRemovesEveryKey() {
	Handle obj = Decode("{\"a\": 1, \"b\": 2}");
	Assert.True(json_object_clear(obj));
	Assert.Equal(json_object_size(obj), 0);
	Assert.True(json_object_clear(obj), "an empty object can be cleared");
	delete obj;
}

public void Test_ObjectUpdateOverwritesAndAddsKeys() {
	Handle obj = Decode("{\"a\": 1, \"b\": 2}");
	Handle other = Decode("{\"b\": 20, \"c\": {\"d\": 30}}");
	Assert.True(json_object_update(obj, other));
	AssertEncodes(obj, "{\"a\":1,\"b\":20,\"c\":{\"d\":30}}");

	Handle c = json_object_get(other, "c");
	json_object_set_new(c, "e", json_true());
	delete c;
	AssertEncodes(obj, "{\"a\":1,\"b\":20,\"c\":{\"d\":30,\"e\":true}}");
	AssertEncodes(other, "{\"b\":20,\"c\":{\"d\":30,\"e\":true}}");

	delete obj;
	delete other;
}

public void Test_ObjectUpdateExistingOnlyChangesExistingKeys() {
	Handle obj = Decode("{\"a\": 1, \"b\": 2}");
	Handle other = Decode("{\"b\": 20, \"c\": 30}");
	Assert.True(json_object_update_existing(obj, other));
	AssertEncodes(obj, "{\"a\":1,\"b\":20}");
	delete obj;
	delete other;
}

public void Test_ObjectUpdateMissingOnlyAddsNewKeys() {
	Handle obj = Decode("{\"a\": 1, \"b\": 2}");
	Handle other = Decode("{\"b\": 20, \"c\": 30}");
	Assert.True(json_object_update_missing(obj, other));
	AssertEncodes(obj, "{\"a\":1,\"b\":2,\"c\":30}");
	delete obj;
	delete other;
}

public void Test_ObjectUpdateRecursiveMergesNestedObjects() {
	Handle obj = Decode("{\"a\": {\"x\": 1, \"y\": 2}, \"b\": {\"z\": 1}, \"c\": 1}");
	Handle other = Decode("{\"a\": {\"y\": 20, \"w\": 30}, \"b\": 5, \"c\": {\"n\": 1}}");
	Assert.True(json_object_update_recursive(obj, other));
	AssertEncodes(obj, "{\"a\":{\"w\":30,\"x\":1,\"y\":20},\"b\":5,\"c\":{\"n\":1}}");
	delete obj;
	delete other;
}

public void Test_ObjectUpdateNewVariantsCloseTheOtherOnSuccess() {
	Handle obj = Decode("{\"a\": 1, \"b\": 2}");

	g_handle = Decode("{\"b\": 20, \"c\": 30}");
	Assert.True(json_object_update_new(obj, g_handle));
	ExpectNativeError(TypeofOfTheHandle, "Invalid <JSON Object> handle");
	AssertEncodes(obj, "{\"a\":1,\"b\":20,\"c\":30}");

	g_handle = Decode("{\"a\": 10, \"d\": 40}");
	Assert.True(json_object_update_existing_new(obj, g_handle));
	ExpectNativeError(TypeofOfTheHandle, "Invalid <JSON Object> handle");
	AssertEncodes(obj, "{\"a\":10,\"b\":20,\"c\":30}");

	g_handle = Decode("{\"a\": 100, \"d\": 40}");
	Assert.True(json_object_update_missing_new(obj, g_handle));
	ExpectNativeError(TypeofOfTheHandle, "Invalid <JSON Object> handle");
	AssertEncodes(obj, "{\"a\":10,\"b\":20,\"c\":30,\"d\":40}");

	delete obj;
}

public void Test_ObjectUpdateNewVariantsKeepTheOtherOnFailure() {
	Handle array = json_array();
	Handle other = Decode("{\"a\": 1}");
	Assert.False(json_object_update_new(array, other));
	Assert.False(json_object_update_existing_new(array, other));
	Assert.False(json_object_update_missing_new(array, other));
	Assert.Equal(json_object_size(other), 1, "the other object's Handle is still open");
	delete array;
	delete other;
}

/* ---------------------------------------------------------------- iteration */

public void Test_IteratorVisitsTheKeysInTheOrderTheyWereAdded() {
	Handle obj = Decode("{\"c\": 1, \"a\": 2, \"b\": 3}");
	json_object_set_new(obj, "0", json_integer(4));

	Handle iterator = json_object_iter(obj);
	int sum = 0;
	char keys[64];
	while (iterator != null) {
		char key[16];
		Assert.True(json_object_iter_key(iterator, key, sizeof(key)) != -1);
		StrCat(keys, sizeof(keys), key);

		Handle value = json_object_iter_value(iterator);
		sum += json_integer_value(value);
		delete value;

		iterator = json_object_iter_next(obj, iterator);
	}

	Assert.StrEqual(keys, "cab0");
	Assert.Equal(sum, 10);
	delete obj;
}

public void Test_IteratorOfAnEmptyObjectPointsAtNothing() {
	Handle obj = json_object();
	Handle array = json_array();
	Handle iterators[3];
	iterators[0] = json_object_iter(obj);
	iterators[1] = json_object_iter(array);
	iterators[2] = json_object_iter_at(obj, "missing");

	for (int i = 0; i < sizeof(iterators); i++) {
		Assert.True(iterators[i] != null, "iterator %d is a Handle", i);
		char key[16] = "unchanged";
		Assert.Equal(json_object_iter_key(iterators[i], key, sizeof(key)), -1, "iterator %d", i);
		Assert.Equal(json_object_iter_value(iterators[i]), INVALID_HANDLE, "iterator %d", i);
		Assert.False(json_object_iter_set_new(obj, iterators[i], json_null()), "iterator %d", i);
		delete iterators[i];
	}

	delete obj;
	delete array;
}

public void Test_IteratorAtAKeyStartsThere() {
	Handle obj = Decode("{\"a\": 1, \"b\": 2, \"c\": 3}");
	Handle iterator = json_object_iter_at(obj, "b");

	char keys[16];
	while (iterator != null) {
		char key[16];
		json_object_iter_key(iterator, key, sizeof(key));
		StrCat(keys, sizeof(keys), key);
		iterator = json_object_iter_next(obj, iterator);
	}

	Assert.StrEqual(keys, "bc");
	delete obj;
}

public void Test_IteratorKeyIsCutOffAndTellsTheWholeLength() {
	Handle obj = Decode("{\"abcdef\": 1}");
	Handle iterator = json_object_iter(obj);

	char key[4];
	Assert.Equal(json_object_iter_key(iterator, key, sizeof(key)), 6);
	Assert.StrEqual(key, "abc");

	delete iterator;
	delete obj;
}

public void Test_IteratorSetReplacesTheValueAtTheIterator() {
	Handle obj = Decode("{\"a\": 1, \"b\": 2}");
	Handle iterator = json_object_iter_at(obj, "b");

	Handle value = json_string("two");
	Assert.True(json_object_iter_set(obj, iterator, value));
	Assert.True(json_is_string(value), "json_object_iter_set leaves the value's Handle open");
	delete value;

	g_handle = json_true();
	Assert.True(json_object_iter_set_new(obj, iterator, g_handle));
	ExpectNativeError(TypeofOfTheHandle, "Invalid <JSON Object> handle");

	AssertEncodes(obj, "{\"a\":1,\"b\":true}");
	delete iterator;
	delete obj;
}

public void Test_IteratorSetFailsOnAnotherValue() {
	Handle obj = Decode("{\"a\": 1}");
	Handle array = json_array();
	Handle iterator = json_object_iter(obj);
	Handle value = json_null();
	Assert.False(json_object_iter_set(array, iterator, value));
	Assert.False(json_object_iter_set_new(array, iterator, value));
	Assert.True(json_is_null(value), "the value's Handle is still open");
	delete value;
	delete iterator;
	delete array;
	delete obj;
}

public void Test_IteratorNextClosesTheIterator() {
	Handle obj = Decode("{\"a\": 1, \"b\": 2}");
	g_handle = json_object_iter(obj);
	Handle next = json_object_iter_next(obj, g_handle);
	Assert.True(next != null);
	ExpectNativeError(KeyOfTheIterator, "Invalid <JSON Iterator> handle");

	Assert.Equal(json_object_iter_next(obj, next), INVALID_HANDLE, "b is the last key");
	g_handle = next;
	ExpectNativeError(KeyOfTheIterator, "Invalid <JSON Iterator> handle");
	delete obj;
}

void KeyOfTheIterator() {
	char key[16];
	json_object_iter_key(g_handle, key, sizeof(key));
}

/* ------------------------------------------------------------------- arrays */

public void Test_ArrayAppendsAndGetsElements() {
	Handle array = json_array();
	Assert.Equal(json_array_size(array), 0);

	Handle one = json_integer(1);
	Assert.True(json_array_append(array, one));
	Assert.Equal(json_integer_value(one), 1, "json_array_append leaves the value's Handle open");
	delete one;
	Assert.True(json_array_append_new(array, json_string("two")));
	Assert.Equal(json_array_size(array), 2);

	Handle element = json_array_get(array, 1);
	Assert.True(json_is_string(element));
	delete element;
	Assert.Equal(json_array_get(array, 2), INVALID_HANDLE);
	Assert.Equal(json_array_get(array, -1), INVALID_HANDLE);

	AssertEncodes(array, "[1,\"two\"]");
	delete array;
}

public void Test_ArrayGetReturnsTheStoredValueNotACopy() {
	Handle array = Decode("[[1]]");
	Handle inner = json_array_get(array, 0);
	json_array_append_new(inner, json_integer(2));
	delete inner;
	AssertEncodes(array, "[[1,2]]");
	delete array;
}

public void Test_ArraySetReplacesAnElementInRange() {
	Handle array = Decode("[1, 2]");
	Handle value = json_string("x");
	Assert.True(json_array_set(array, 1, value));
	Assert.True(json_is_string(value), "json_array_set leaves the value's Handle open");
	Assert.False(json_array_set(array, 2, value), "2 is past the end");
	Assert.False(json_array_set(array, -1, value));
	delete value;

	g_handle = json_null();
	Assert.True(json_array_set_new(array, 0, g_handle));
	ExpectNativeError(TypeofOfTheHandle, "Invalid <JSON Object> handle");

	value = json_true();
	Assert.False(json_array_set_new(array, 5, value));
	Assert.True(json_is_true(value), "a failed json_array_set_new leaves the Handle open");
	delete value;

	AssertEncodes(array, "[null,\"x\"]");
	delete array;
}

public void Test_ArrayInsertShiftsTheElements() {
	Handle array = Decode("[1, 3]");
	Handle value = json_integer(2);
	Assert.True(json_array_insert(array, 1, value));
	Assert.Equal(json_integer_value(value), 2, "json_array_insert leaves the value's Handle open");
	Assert.True(json_array_insert(array, 3, value), "the size appends");
	Assert.False(json_array_insert(array, 5, value), "past the size");
	delete value;

	Assert.True(json_array_insert_new(array, 0, json_integer(0)));
	value = json_null();
	Assert.False(json_array_insert_new(array, -1, value));
	Assert.True(json_is_null(value), "a failed json_array_insert_new leaves the Handle open");
	delete value;

	AssertEncodes(array, "[0,1,2,3,2]");
	delete array;
}

public void Test_ArrayRemoveShiftsTheElements() {
	Handle array = Decode("[0, 1, 2]");
	Handle first = json_array_get(array, 0);
	Assert.True(json_array_remove(array, 0));
	Assert.False(json_array_remove(array, 2), "there are two elements left");
	Assert.False(json_array_remove(array, -1));
	AssertEncodes(array, "[1,2]");
	Assert.Equal(json_integer_value(first), 0, "Handles to the removed element stay valid");
	delete first;
	delete array;
}

public void Test_ArrayClearRemovesEveryElement() {
	Handle array = Decode("[1, 2]");
	Assert.True(json_array_clear(array));
	Assert.Equal(json_array_size(array), 0);
	delete array;
}

public void Test_ArrayExtendAppendsTheSharedElements() {
	Handle array = Decode("[1]");
	Handle other = Decode("[[2], 3]");
	Assert.True(json_array_extend(array, other));
	AssertEncodes(array, "[1,[2],3]");

	Handle inner = json_array_get(other, 0);
	json_array_append_new(inner, json_integer(4));
	delete inner;
	AssertEncodes(array, "[1,[2,4],3]");

	delete array;
	delete other;
}

public void Test_ArrayNativesFailOnOtherValues() {
	Handle obj = json_object();
	Handle array = json_array();
	Handle value = json_null();
	Assert.Equal(json_array_size(obj), 0);
	Assert.Equal(json_array_get(obj, 0), INVALID_HANDLE);
	Assert.False(json_array_set(obj, 0, value));
	Assert.False(json_array_append(obj, value));
	Assert.False(json_array_append_new(obj, value));
	Assert.False(json_array_insert(obj, 0, value));
	Assert.False(json_array_remove(obj, 0));
	Assert.False(json_array_clear(obj));
	Assert.False(json_array_extend(obj, array));
	Assert.False(json_array_extend(array, obj));
	Assert.True(json_is_null(value), "the value's Handle is still open");
	delete obj;
	delete array;
	delete value;
}

/* --------------------------------------------------------- booleans and null */

public void Test_BooleanMakesTrueOrFalse() {
	Handle yes = json_boolean(true);
	Handle no = json_boolean(false);
	Assert.Equal(json_typeof(yes), JSON_TRUE);
	Assert.Equal(json_typeof(no), JSON_FALSE);
	AssertEncodes(yes, "true");
	AssertEncodes(no, "false");
	delete yes;
	delete no;
}

public void Test_NullIsAValueNotAnInvalidHandle() {
	Handle nothing = json_null();
	Assert.True(nothing != INVALID_HANDLE);
	AssertEncodes(nothing, "null");

	Handle array = json_array();
	Assert.True(json_array_append(array, nothing));
	AssertEncodes(array, "[null]");
	delete array;
	delete nothing;
}

/* ------------------------------------------------------------------ strings */

public void Test_StringHoldsAndChangesItsText() {
	Handle text = json_string("value");
	char buffer[32];
	Assert.Equal(json_string_value(text, buffer, sizeof(buffer)), 5);
	Assert.StrEqual(buffer, "value");

	Assert.True(json_string_set(text, "The answer is 42"));
	Assert.Equal(json_string_value(text, buffer, sizeof(buffer)), 16);
	Assert.StrEqual(buffer, "The answer is 42");
	delete text;
}

public void Test_StringValueIsCutOffBetweenCharacters() {
	Handle text = json_string("aäb");
	char buffer[3];
	Assert.Equal(json_string_value(text, buffer, sizeof(buffer)), 4, "the length of the whole text in bytes");
	Assert.StrEqual(buffer, "a", "ä takes two bytes and does not fit");

	char whole[8];
	json_string_value(text, whole, sizeof(whole));
	Assert.StrEqual(whole, "aäb");
	delete text;
}

public void Test_StringNativesFailOnOtherValues() {
	Handle number = json_integer(1);
	char buffer[16] = "unchanged";
	Assert.Equal(json_string_value(number, buffer, sizeof(buffer)), -1);
	Assert.StrEqual(buffer, "unchanged");
	Assert.False(json_string_set(number, "text"));
	delete number;
}

public void Test_StringOfInvalidUtf8HoldsNoValue() {
	char text[2];
	InvalidUtf8(text, sizeof(text));
	g_handle = json_string(text);
	Assert.True(g_handle != null);

	Handle array = json_array();
	Handle obj = json_object();
	Assert.False(json_array_append(array, g_handle));
	Assert.False(json_object_set(obj, "key", g_handle));
	Assert.Equal(json_array_size(array), 0);
	Assert.Equal(json_object_size(obj), 0);
	ExpectNativeError(TypeofOfTheHandle, "JSON Object is NULL.");

	delete g_handle;
	delete array;
	delete obj;
}

/* ------------------------------------------------------------------ numbers */

public void Test_IntegerHoldsAndChangesItsValue() {
	Handle number = json_integer(-7);
	Assert.Equal(json_integer_value(number), -7);
	Assert.True(json_integer_set(number, 2147483647));
	Assert.Equal(json_integer_value(number), 2147483647);
	Assert.FloatEqual(json_number_value(number), 2147483647.0, 1.0);
	Assert.FloatEqual(json_real_value(number), 0.0, 0.0, "an integer is not a real");
	Assert.False(json_real_set(number, 1.0));
	delete number;
}

public void Test_RealHoldsAndChangesItsValue() {
	Handle number = json_real(1.5);
	Assert.FloatEqual(json_real_value(number), 1.5, 0.0);
	Assert.True(json_real_set(number, -0.25));
	Assert.FloatEqual(json_real_value(number), -0.25, 0.0);
	Assert.FloatEqual(json_number_value(number), -0.25, 0.0);
	Assert.Equal(json_integer_value(number), 0, "a real is not an integer");
	Assert.False(json_integer_set(number, 1));
	delete number;
}

public void Test_NumberValueOfOtherValuesIsZero() {
	Handle text = json_string("1");
	Assert.FloatEqual(json_number_value(text), 0.0, 0.0);
	Assert.Equal(json_integer_value(text), 0);
	Assert.FloatEqual(json_real_value(text), 0.0, 0.0);
	delete text;
}

public void Test_IntegersBeyond32BitsKeepTheirValue() {
	Handle array = Decode("[4294967297, -4294967296]");
	Handle big = json_array_get(array, 0);
	Assert.Equal(json_integer_value(big), 1, "the lowest 32 bits of 2^32 + 1");
	delete big;
	AssertEncodes(array, "[4294967297,-4294967296]");
	delete array;
}

public void Test_RealsAreWrittenWithTheDigitsThatReadBackExactly() {
	Handle array = json_array();
	json_array_append_new(array, json_real(0.1));
	json_array_append_new(array, json_real(0.5));

	char json[64];
	json_dump_ex(array, json, sizeof(json), JSON_COMPACT);
	Assert.StrEqual(json, "[0.10000000149011612,0.5]");
	json_dump_ex(array, json, sizeof(json), JSON_COMPACT | JSON_REAL_PRECISION(3));
	Assert.StrEqual(json, "[0.1,0.5]");
	delete array;
}

/* ----------------------------------------------------------------- decoding */

public void Test_LoadDecodesObjectsAndArrays() {
	Handle obj = json_load("{\"a\": [1, 2.5, \"x\", true, false, null], \"b\": {}}");
	Assert.True(obj != null);
	Assert.True(json_is_object(obj));
	AssertEncodes(obj, "{\"a\":[1,2.5,\"x\",true,false,null],\"b\":{}}");
	delete obj;

	Handle array = json_load("  [ ]  ");
	Assert.True(json_is_array(array));
	delete array;
}

public void Test_LoadNeedsDecodeAnyForOtherValuesAtTheTop() {
	Assert.Equal(json_load("1"), INVALID_HANDLE);
	Assert.Equal(json_load("\"text\""), INVALID_HANDLE);

	char error[256];
	int line;
	int column;
	json_error_code code;
	Assert.Equal(json_load_ex("null", error, sizeof(error), line, column, 0, code), INVALID_HANDLE);
	Assert.Equal(code, json_error_invalid_syntax, "%s", error);

	char texts[5][8] = {"1", "1.5", "\"text\"", "true", "null"}; // no critic (RequireLateDeclarations) - with types
	json_type types[5] = {JSON_INTEGER, JSON_REAL, JSON_STRING, JSON_TRUE, JSON_NULL};
	for (int i = 0; i < sizeof(texts); i++) {
		Handle value = json_load(texts[i], JSON_DECODE_ANY);
		Assert.True(value != null, "%s decodes", texts[i]);
		Assert.Equal(json_typeof(value), types[i], "%s", texts[i]);
		delete value;
	}
}

public void Test_LoadLogsItsErrors() {
	Assert.Equal(json_load("{\"a\": }"), INVALID_HANDLE);
	Assert.True(ErrorLogContains("Error in line 1, col 7"), "the error is in SourceMod's error log");
}

/* Decodes text with flags, which must fail with code, and returns the error. */
void LoadFails(const char[] text, int flags, json_error_code expected, int expectedLine = 0, int expectedColumn = 0) {
	char error[256];
	int line = -100;
	int column = -100;
	json_error_code code;
	Assert.Equal(json_load_ex(text, error, sizeof(error), line, column, flags, code), INVALID_HANDLE,
	             "%s does not decode", text);
	Assert.Equal(code, expected, "%s: %s", text, error);
	Assert.True(error[0] != '\0', "%s has an error text", text);
	if (expectedLine != 0) {
		Assert.Equal(line, expectedLine, "line of %s", text);
		Assert.Equal(column, expectedColumn, "column of %s", text);
	}
}

public void Test_LoadExTellsWhyAndWhereDecodingFailed() {
	LoadFails("{\n  \"a\": x\n}", 0, json_error_invalid_syntax, 2, 8);
	LoadFails("[1, 2", 0, json_error_premature_end_of_input);
	LoadFails("[1] [2]", 0, json_error_end_of_input_expected);
	LoadFails("[\"\\u0000\"]", 0, json_error_null_character);
	LoadFails("{\"a\\u0000b\": 1}", JSON_DECODE_ANY, json_error_null_byte_in_key);
	LoadFails("[1e400]", 0, json_error_numeric_overflow);
	LoadFails("[99999999999999999999]", 0, json_error_numeric_overflow);

	char invalid[8] = "[\"x\"]";
	invalid[2] = 0xFF;
	LoadFails(invalid, 0, json_error_invalid_utf8);

	char deep[2100];
	for (int i = 0; i < 2049; i++) {
		deep[i] = '[';
	}
	LoadFails(deep, 0, json_error_stack_overflow);
}

public void Test_LoadExChangesNothingOnSuccess() {
	char error[16] = "unchanged"; // no critic (RequireLateDeclarations) - all four are outputs
	int line = -5;                // no critic (RequireLateDeclarations)
	int column = -6;              // no critic (RequireLateDeclarations)
	json_error_code code = json_error_item_not_found;
	Handle value = json_load_ex("[1]", error, sizeof(error), line, column, 0, code);
	Assert.True(value != null);
	Assert.StrEqual(error, "unchanged");
	Assert.Equal(line, -5);
	Assert.Equal(column, -6);
	Assert.Equal(code, json_error_item_not_found);
	delete value;
}

public void Test_RejectDuplicatesRefusesARepeatedKey() {
	Handle obj = json_load("{\"a\": 1, \"a\": 2}");
	AssertEncodes(obj, "{\"a\":2}");
	delete obj;

	LoadFails("{\"a\": 1, \"a\": 2}", JSON_REJECT_DUPLICATES, json_error_duplicate_key);
}

public void Test_DisableEofCheckIgnoresTheRest() {
	Handle value = json_load("[1] trailing text", JSON_DISABLE_EOF_CHECK);
	Assert.True(value != null);
	AssertEncodes(value, "[1]");
	delete value;
}

public void Test_DecodeIntAsRealMakesEveryNumberAReal() {
	Handle array = json_load("[1, 2.5]", JSON_DECODE_INT_AS_REAL);
	Handle first = json_array_get(array, 0);
	Assert.True(json_is_real(first));
	Assert.FloatEqual(json_real_value(first), 1.0, 0.0);
	delete first;
	delete array;
}

public void Test_UnsupportedDecodingFlagsAreANativeError() {
	ExpectNativeError(LoadWithAnUnknownFlag, "Unsupported flags 10");
	ExpectNativeError(LoadExWithAnUnknownFlag, "Unsupported flags 10");
	ExpectNativeError(LoadFileWithAnUnknownFlag, "Unsupported flags 10");
	ExpectNativeError(LoadFileExWithAnUnknownFlag, "Unsupported flags 10");
}

void LoadWithAnUnknownFlag() {
	json_load("[]", 0x10);
}

void LoadExWithAnUnknownFlag() {
	char error[64];
	int line;
	int column;
	json_load_ex("[]", error, sizeof(error), line, column, 0x10);
}

void LoadFileWithAnUnknownFlag() {
	json_load_file("cfg/missing.json", 0x10);
}

void LoadFileExWithAnUnknownFlag() {
	char error[64];
	int line;
	int column;
	json_load_file_ex("cfg/missing.json", error, sizeof(error), line, column, 0x10);
}

public void Test_LoadFileReadsAPathInTheGameFolder() {
	WriteTextFile("cfg/smjansson-test.json", "{\"from\": \"file\"}");
	Handle obj = json_load_file("cfg/smjansson-test.json");
	Assert.True(obj != null);
	AssertEncodes(obj, "{\"from\":\"file\"}");
	delete obj;

	WriteTextFile("cfg/smjansson-scalar.json", "42");
	Assert.Equal(json_load_file("cfg/smjansson-scalar.json"), INVALID_HANDLE);
	Handle number = json_load_file("cfg/smjansson-scalar.json", JSON_DECODE_ANY);
	Assert.Equal(json_integer_value(number), 42);
	delete number;
}

public void Test_FilesTakeAnAbsolutePathAfterFileUrl() {
	char path[PLATFORM_MAX_PATH];
	Format(path, sizeof(path), "file:///tmp/smjansson-test-%d-%d.json", GetTime(), GetURandomInt());
	Handle array = Decode("[\"absolute\"]");
	Assert.True(json_dump_file_ex(array, path, JSON_COMPACT), "%s can be written", path);
	delete array;

	char error[256];
	int line;
	int column;
	array = json_load_file_ex(path, error, sizeof(error), line, column);
	DeleteFile(path);
	Assert.True(array != null, "%s: %s", path, error);
	AssertEncodes(array, "[\"absolute\"]");
	delete array;
}

public void Test_LoadFileExTellsAFileThatCannotBeOpened() {
	char error[256];
	int line;
	int column;
	json_error_code code;
	Assert.Equal(json_load_file_ex("cfg/missing.json", error, sizeof(error), line, column, 0, code), INVALID_HANDLE);
	Assert.Equal(code, json_error_cannot_open_file, "%s", error);
	Assert.Equal(line, -1);
	Assert.Equal(column, -1);
}

public void Test_LoadFileExTellsWhereTheFileIsInvalid() {
	WriteTextFile("cfg/smjansson-invalid.json", "[\n1,\n2,,\n]");
	char error[256];
	int line;
	int column;
	json_error_code code;
	Assert.Equal(json_load_file_ex("cfg/smjansson-invalid.json", error, sizeof(error), line, column, 0, code),
	             INVALID_HANDLE);
	Assert.Equal(code, json_error_invalid_syntax, "%s", error);
	Assert.Equal(line, 3);
}

public void Test_LoadFileLogsAFileThatCannotBeOpened() {
	Assert.Equal(json_load_file("cfg/missing.json"), INVALID_HANDLE);
	Assert.True(ErrorLogContains("missing.json"), "the error is in SourceMod's error log");
}

/* ----------------------------------------------------------------- encoding */

public void Test_DumpIndentsWithFourSpacesByDefault() {
	Handle obj = Decode("{\"a\": [1]}");
	char json[128];
	Assert.Equal(json_dump(obj, json, sizeof(json)), 30);
	Assert.StrEqual(json, "{\n    \"a\": [\n        1\n    ]\n}");

	json_dump(obj, json, sizeof(json), 0);
	Assert.StrEqual(json, "{\"a\": [1]}", "without an indent everything is on one line");
	json_dump(obj, json, sizeof(json), 32);
	Assert.StrEqual(json, "{\"a\": [1]}", "32 acts like 0");
	json_dump(obj, json, sizeof(json), 1);
	Assert.StrEqual(json, "{\n \"a\": [\n  1\n ]\n}");
	delete obj;
}

public void Test_DumpKeepsTheOrderOfTheKeysUnlessSorted() {
	Handle obj = Decode("{\"b\": 1, \"a\": 2}");
	char json[64];
	json_dump(obj, json, sizeof(json), 0);
	Assert.StrEqual(json, "{\"b\": 1, \"a\": 2}");
	json_dump(obj, json, sizeof(json), 0, false, false, true);
	Assert.StrEqual(json, "{\"b\": 1, \"a\": 2}", "bPreserveOrder has no effect");
	json_dump(obj, json, sizeof(json), 0, false, true);
	Assert.StrEqual(json, "{\"a\": 2, \"b\": 1}");
	delete obj;
}

public void Test_DumpCanEnsureAscii() {
	Handle array = json_array();
	json_array_append_new(array, json_string("ä"));
	char json[64];
	json_dump(array, json, sizeof(json), 0);
	Assert.StrEqual(json, "[\"ä\"]");
	json_dump(array, json, sizeof(json), 0, true);
	Assert.StrEqual(json, "[\"\\u00E4\"]");
	delete array;
}

public void Test_DumpIsCutOffAndTellsTheWholeLength() {
	Handle array = Decode("[123456789]");
	char json[6];
	Assert.Equal(json_dump(array, json, sizeof(json), 0), 11);
	Assert.StrEqual(json, "[1234");
	Assert.Equal(json_dump_ex(array, json, sizeof(json), 0), 11);
	Assert.StrEqual(json, "[1234");
	delete array;
}

public void Test_DumpOnlyEncodesObjectsAndArrays() {
	Handle number = json_integer(1);
	char json[16] = "unchanged";
	Assert.Equal(json_dump(number, json, sizeof(json)), -1);
	Assert.Equal(json_dump_ex(number, json, sizeof(json), 0), -1);
	Assert.Equal(json_dump_ex(number, json, sizeof(json), JSON_ENCODE_ANY), 1);
	Assert.StrEqual(json, "1");
	delete number;
}

public void Test_DumpExTakesItsOptionsAsFlags() {
	Handle obj = Decode("{\"b\": \"/ä\", \"a\": [1, 2]}");
	char json[128];

	json_dump_ex(obj, json, sizeof(json), 0);
	Assert.StrEqual(json, "{\"b\": \"/ä\", \"a\": [1, 2]}");
	json_dump_ex(obj, json, sizeof(json), JSON_COMPACT);
	Assert.StrEqual(json, "{\"b\":\"/ä\",\"a\":[1,2]}");
	json_dump_ex(obj, json, sizeof(json), JSON_COMPACT | JSON_SORT_KEYS);
	Assert.StrEqual(json, "{\"a\":[1,2],\"b\":\"/ä\"}");
	json_dump_ex(obj, json, sizeof(json), JSON_COMPACT | JSON_ESCAPE_SLASH | JSON_ENSURE_ASCII);
	Assert.StrEqual(json, "{\"b\":\"\\/\\u00E4\",\"a\":[1,2]}");
	json_dump_ex(obj, json, sizeof(json), JSON_INDENT(2) | JSON_COMPACT);
	Assert.StrEqual(json, "{\n  \"b\":\"/ä\",\n  \"a\":[\n    1,\n    2\n  ]\n}");
	delete obj;
}

public void Test_UnsupportedEncodingFlagsAreANativeError() {
	g_handle = json_array();
	ExpectNativeError(DumpExWithAnUnknownFlag, "Unsupported flags 100");
	ExpectNativeError(DumpFileExWithAnUnknownFlag, "Unsupported flags 100");
	delete g_handle;
}

void DumpExWithAnUnknownFlag() {
	char json[16];
	json_dump_ex(g_handle, json, sizeof(json), 0x100);
}

void DumpFileExWithAnUnknownFlag() {
	json_dump_file_ex(g_handle, "cfg/never.json", 0x100);
}

public void Test_DumpFileWritesAndOverwritesTheFile() {
	Handle obj = Decode("{\"b\": 1, \"a\": \"ä\"}");
	WriteTextFile("cfg/smjansson-out.json", "old text that is longer than the new one");
	Assert.True(json_dump_file(obj, "cfg/smjansson-out.json", 0, true, true));

	char text[128];
	ReadTextFile("cfg/smjansson-out.json", text, sizeof(text));
	Assert.StrEqual(text, "{\"a\": \"\\u00E4\", \"b\": 1}");

	Assert.True(json_dump_file(obj, "cfg/smjansson-out.json"));
	ReadTextFile("cfg/smjansson-out.json", text, sizeof(text));
	Assert.StrEqual(text, "{\n    \"b\": 1,\n    \"a\": \"ä\"\n}");

	Handle back = json_load_file("cfg/smjansson-out.json");
	Assert.True(json_equal(back, obj));
	delete back;
	delete obj;
}

public void Test_DumpFileExTakesItsOptionsAsFlags() {
	Handle text = json_string("a/b");
	Assert.False(json_dump_file_ex(text, "cfg/smjansson-any.json", 0), "a string needs JSON_ENCODE_ANY");
	Assert.True(json_dump_file_ex(text, "cfg/smjansson-any.json", JSON_ENCODE_ANY | JSON_ESCAPE_SLASH));

	char content[64];
	ReadTextFile("cfg/smjansson-any.json", content, sizeof(content));
	Assert.StrEqual(content, "\"a\\/b\"");
	delete text;
}

public void Test_DumpFileFailsWithoutItsFolderOrForScalars() {
	Handle obj = json_object();
	Assert.False(json_dump_file(obj, "cfg/missing-folder/out.json"), "missing folders are not created");
	Assert.False(DirExists("cfg/missing-folder"));
	delete obj;

	Handle number = json_integer(1);
	Assert.False(json_dump_file(number, "cfg/smjansson-number.json"));
	delete number;
}

/* ------------------------------------------------------------ native errors */

public void Test_NativesRefuseAnInvalidHandle() {
	g_handle = INVALID_HANDLE;
	ExpectNativeErrors();
}

public void Test_NativesRefuseAClosedHandle() {
	g_handle = json_object();
	delete g_handle;
	ExpectNativeErrors();
}

public void Test_NativesRefuseAHandleThatIsNotJson() {
	g_list = new ArrayList();
	g_handle = g_list;
	ExpectNativeErrors();
	delete g_list;
}

/* Calls each native that takes a JSON value with g_handle, through the
 * public Call_ functions below. */
void ExpectNativeErrors() {
	char natives[][] = {
		"Call_typeof", "Call_equal", "Call_copy", "Call_deep_copy", "Call_object_size", "Call_object_get",
		"Call_object_set", "Call_object_set_new", "Call_object_del", "Call_object_clear", "Call_object_update",
		"Call_object_update_existing", "Call_object_update_missing", "Call_object_update_new",
		"Call_object_update_existing_new", "Call_object_update_missing_new", "Call_object_update_recursive",
		"Call_object_iter", "Call_object_iter_at", "Call_object_iter_next", "Call_object_iter_key",
		"Call_object_iter_value", "Call_object_iter_set", "Call_object_iter_set_new", "Call_array_size",
		"Call_array_get", "Call_array_set", "Call_array_set_new", "Call_array_append", "Call_array_append_new",
		"Call_array_insert", "Call_array_insert_new", "Call_array_remove", "Call_array_clear", "Call_array_extend",
		"Call_string_value", "Call_string_set", "Call_integer_value", "Call_integer_set", "Call_real_value",
		"Call_real_set", "Call_number_value", "Call_dump", "Call_dump_file", "Call_dump_ex", "Call_dump_file_ex",
	};
	for (int i = 0; i < sizeof(natives); i++) {
		Function call = GetFunctionByName(null, natives[i]);
		Assert.True(call != INVALID_FUNCTION, "%s exists", natives[i]);
		ExpectNativeError(call, "handle");
	}
}

public void Call_typeof() {
	json_typeof(g_handle);
}

public void Call_equal() {
	Handle other = json_null();
	json_equal(g_handle, other);
	delete other;
}

public void Call_copy() {
	json_copy(g_handle);
}

public void Call_deep_copy() {
	json_deep_copy(g_handle);
}

public void Call_object_size() {
	json_object_size(g_handle);
}

public void Call_object_get() {
	json_object_get(g_handle, "key");
}

public void Call_object_set() {
	Handle value = json_null();
	json_object_set(g_handle, "key", value);
	delete value;
}

public void Call_object_set_new() {
	json_object_set_new(g_handle, "key", json_null());
}

public void Call_object_del() {
	json_object_del(g_handle, "key");
}

public void Call_object_clear() {
	json_object_clear(g_handle);
}

public void Call_object_update() {
	Handle other = json_object();
	json_object_update(g_handle, other);
	delete other;
}

public void Call_object_update_existing() {
	Handle other = json_object();
	json_object_update_existing(g_handle, other);
	delete other;
}

public void Call_object_update_missing() {
	Handle other = json_object();
	json_object_update_missing(g_handle, other);
	delete other;
}

public void Call_object_update_new() {
	json_object_update_new(g_handle, json_object());
}

public void Call_object_update_existing_new() {
	json_object_update_existing_new(g_handle, json_object());
}

public void Call_object_update_missing_new() {
	json_object_update_missing_new(g_handle, json_object());
}

public void Call_object_update_recursive() {
	Handle other = json_object();
	json_object_update_recursive(g_handle, other);
	delete other;
}

public void Call_object_iter() {
	json_object_iter(g_handle);
}

public void Call_object_iter_at() {
	json_object_iter_at(g_handle, "key");
}

public void Call_object_iter_next() {
	Handle obj = json_object();
	json_object_iter_next(obj, g_handle);
	delete obj;
}

public void Call_object_iter_key() {
	char key[16];
	json_object_iter_key(g_handle, key, sizeof(key));
}

public void Call_object_iter_value() {
	json_object_iter_value(g_handle);
}

public void Call_object_iter_set() {
	Handle obj = json_object();
	Handle value = json_null();
	json_object_iter_set(obj, g_handle, value);
	delete value;
	delete obj;
}

public void Call_object_iter_set_new() {
	Handle obj = json_object();
	json_object_iter_set_new(obj, g_handle, json_null());
	delete obj;
}

public void Call_array_size() {
	json_array_size(g_handle);
}

public void Call_array_get() {
	json_array_get(g_handle, 0);
}

public void Call_array_set() {
	Handle value = json_null();
	json_array_set(g_handle, 0, value);
	delete value;
}

public void Call_array_set_new() {
	json_array_set_new(g_handle, 0, json_null());
}

public void Call_array_append() {
	Handle value = json_null();
	json_array_append(g_handle, value);
	delete value;
}

public void Call_array_append_new() {
	json_array_append_new(g_handle, json_null());
}

public void Call_array_insert() {
	Handle value = json_null();
	json_array_insert(g_handle, 0, value);
	delete value;
}

public void Call_array_insert_new() {
	json_array_insert_new(g_handle, 0, json_null());
}

public void Call_array_remove() {
	json_array_remove(g_handle, 0);
}

public void Call_array_clear() {
	json_array_clear(g_handle);
}

public void Call_array_extend() {
	Handle other = json_array();
	json_array_extend(g_handle, other);
	delete other;
}

public void Call_string_value() {
	char value[16];
	json_string_value(g_handle, value, sizeof(value));
}

public void Call_string_set() {
	json_string_set(g_handle, "text");
}

public void Call_integer_value() {
	json_integer_value(g_handle);
}

public void Call_integer_set() {
	json_integer_set(g_handle, 1);
}

public void Call_real_value() {
	json_real_value(g_handle);
}

public void Call_real_set() {
	json_real_set(g_handle, 1.0);
}

public void Call_number_value() {
	json_number_value(g_handle);
}

public void Call_dump() {
	char json[16];
	json_dump(g_handle, json, sizeof(json));
}

public void Call_dump_file() {
	json_dump_file(g_handle, "cfg/never.json");
}

public void Call_dump_ex() {
	char json[16];
	json_dump_ex(g_handle, json, sizeof(json), 0);
}

public void Call_dump_file_ex() {
	json_dump_file_ex(g_handle, "cfg/never.json", 0);
}

/* ------------------------------------------------------------------- stocks */

public void Test_StringFormatStocksFormatTheirText() {
	Handle text = json_string_format("%s=%d", "answer", 42);
	char buffer[32];
	json_string_value(text, buffer, sizeof(buffer));
	Assert.StrEqual(buffer, "answer=42");
	delete text;

	text = json_string_format_ex(6, "%s", "cut off here");
	json_string_value(text, buffer, sizeof(buffer));
	Assert.StrEqual(buffer, "cut o", "the temporary buffer holds 5 bytes and the end");
	delete text;
}

public void Test_ArrayGetStocksReadTheElements() {
	Handle array = Decode("[true, 1.5, 7, \"text\", null, false]");
	Assert.True(json_array_get_bool(array, 0));
	Assert.False(json_array_get_bool(array, 4));
	Assert.False(json_array_get_bool(array, 5));
	Assert.FloatEqual(json_array_get_float(array, 1), 1.5, 0.0);
	Assert.FloatEqual(json_array_get_float(array, 2), 7.0, 0.0, "an integer reads as a float");
	Assert.FloatEqual(json_array_get_float(array, 3), 0.0, 0.0);
	Assert.Equal(json_array_get_int(array, 2), 7);
	Assert.Equal(json_array_get_int(array, 1), 0, "a real is not an integer");

	char text[16] = "unchanged";
	Assert.Equal(json_array_get_string(array, 2, text, sizeof(text)), -1);
	Assert.StrEqual(text, "unchanged");
	Assert.Equal(json_array_get_string(array, 3, text, sizeof(text)), 4);
	Assert.StrEqual(text, "text");
	delete array;
}

public void Test_ObjectGetStocksReadTheValues() {
	Handle obj = Decode("{\"b\": true, \"f\": 1.5, \"i\": 7, \"s\": \"text\", \"n\": null}");
	Assert.True(json_object_get_bool(obj, "b"));
	Assert.False(json_object_get_bool(obj, "n"));
	Assert.FloatEqual(json_object_get_float(obj, "f"), 1.5, 0.0);
	Assert.FloatEqual(json_object_get_float(obj, "i"), 7.0, 0.0);
	Assert.FloatEqual(json_object_get_float(obj, "s"), 0.0, 0.0);
	Assert.Equal(json_object_get_int(obj, "i"), 7);
	Assert.Equal(json_object_get_int(obj, "f"), 0);

	char text[16] = "unchanged";
	Assert.Equal(json_object_get_string(obj, "i", text, sizeof(text)), -1);
	Assert.StrEqual(text, "unchanged");
	Assert.Equal(json_object_get_string(obj, "s", text, sizeof(text)), 4);
	Assert.StrEqual(text, "text");
	delete obj;
}

public void Test_GetStocksNeedTheEntryToExist() {
	g_handle = Decode("[1]");
	ExpectNativeError(ArrayGetIntPastTheEnd, "handle");
	delete g_handle;

	g_handle = Decode("{}");
	ExpectNativeError(ObjectGetIntOfAMissingKey, "handle");
	delete g_handle;
}

void ArrayGetIntPastTheEnd() {
	json_array_get_int(g_handle, 1);
}

void ObjectGetIntOfAMissingKey() {
	json_object_get_int(g_handle, "missing");
}

public void Test_PackBuildsNestedValues() {
	ArrayList params = new ArrayList(ByteCountToCells(255));
	params.PushString("name");
	params.PushString("Jansson");
	params.PushString("list");
	params.Push(7);
	params.Push(true);
	params.Push(0.5);
	params.Push(1.5);
	params.PushString("inner");
	params.PushString("key");
	params.PushString("value");

	Handle obj = json_pack("{s:s, s:[i, b, f, r, n], s:{s:s}}", params);
	Assert.True(obj != null);
	AssertEncodes(obj, "{\"inner\":{\"key\":\"value\"},\"list\":[7,true,0.5,1.5,null],\"name\":\"Jansson\"}");
	Assert.Equal(params.Length, 0, "every value was used and removed");
	delete obj;
	delete params;
}

public void Test_PackBuildsAnArrayAtTheTop() {
	ArrayList params = new ArrayList(ByteCountToCells(255));
	params.PushString("a");
	params.Push(false);
	params.PushString("extra");

	Handle array = json_pack("[s, b, []]", params);
	AssertEncodes(array, "[\"a\",false,[]]");
	Assert.Equal(params.Length, 1, "the unused value stays");
	delete array;
	delete params;
}

public void Test_PackRefusesAKeyThatIsNotAString() {
	ArrayList params = new ArrayList(ByteCountToCells(255));
	params.Push(1);
	params.Push(2);
	Assert.Equal(json_pack("{i:i}", params), INVALID_HANDLE);
	Assert.True(ErrorLogContains("Object keys must be strings at 1."), "the error is logged");
	delete params;
}

public void Test_PackNeedsAValueForEachFormatCharacter() {
	g_list = new ArrayList(ByteCountToCells(255));
	ExpectNativeError(PackWithoutValues, "Invalid index 0");
	delete g_list;
}

void PackWithoutValues() {
	json_pack("[i]", g_list);
}
