#if defined _jansson_included_
	#endinput
#endif
#define _jansson_included_


/**
 * --- Type
 *
 * The JSON specification (RFC 4627) defines the following data types:
 * object, array, string, number, boolean, and null.
 * JSON types are used dynamically; arrays and objects can hold any
 * other data type, including themselves. For this reason, Jansson's
 * type system is also dynamic in nature. There's one Handle type to
 * represent all JSON values, and the referenced structure knows the
 * type of the JSON value it holds.
 *
 * Every native in this file raises an error when it is passed a Handle
 * that is INVALID_HANDLE, already closed, or not a JSON value.
 *
 */
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

/**
 * Return the type of the JSON value.
 *
 * @param hObj              Handle to the JSON value
 *
 * @error                   Invalid or closed handle, or a value created
 *                          by json_string() from invalid UTF-8.
 * @return                  json_type of the value.
 */
native json_type json_typeof(Handle hObj);

/**
 * The type of a JSON value is queried and tested using these macros.
 * They call json_typeof(), so they raise the same errors: they can not
 * be used to test whether a Handle is INVALID_HANDLE.
 * Read the value of a JSON boolean with json_boolean_value().
 *
 * @param %1                Handle to the JSON value
 *
 * @return                  True if the value has the correct type.
 */
#define json_is_object(%1)			( json_typeof(%1) == JSON_OBJECT )
#define json_is_array(%1)			( json_typeof(%1) == JSON_ARRAY )
#define json_is_string(%1)			( json_typeof(%1) == JSON_STRING )
#define json_is_integer(%1)			( json_typeof(%1) == JSON_INTEGER )
#define json_is_real(%1)			( json_typeof(%1) == JSON_REAL )
#define json_is_true(%1)			( json_typeof(%1) == JSON_TRUE )
#define json_is_false(%1)			( json_typeof(%1) == JSON_FALSE )
#define json_is_null(%1)			( json_typeof(%1) == JSON_NULL )
#define json_is_number(%1)			( json_typeof(%1) == JSON_INTEGER || json_typeof(%1) == JSON_REAL )
#define json_is_boolean(%1)			( json_typeof(%1) == JSON_TRUE || json_typeof(%1) == JSON_FALSE )

/**
 * Returns the value of a JSON boolean: true for JSON true, false for
 * every other value. An alias of json_is_true() with the same errors.
 *
 * @param %1                Handle to the JSON value
 *
 * @return                  True if the value is JSON true.
 */
#define json_boolean_value(%1)		json_is_true(%1)

/**
 * Saves json_type as a String in output
 *
 * @param input             json_type value to convert to string
 * @param output            Buffer to store the json_type value
 * @param maxlength         Maximum length of string buffer.
 *
 * @return                  False if the type does not exist.
 */
stock bool Stringify_json_type(json_type input, char[] output, int maxlength) {
	switch(input) {
		case JSON_OBJECT:		strcopy(output, maxlength, "Object");
		case JSON_ARRAY:		strcopy(output, maxlength, "Array");
		case JSON_STRING:		strcopy(output, maxlength, "String");
		case JSON_INTEGER:		strcopy(output, maxlength, "Integer");
		case JSON_REAL:			strcopy(output, maxlength, "Real");
		case JSON_TRUE:			strcopy(output, maxlength, "True");
		case JSON_FALSE:		strcopy(output, maxlength, "False");
		case JSON_NULL:			strcopy(output, maxlength, "Null");
		default: return false;
	}

	return true;
}



/**
 * --- Equality
 *
 * - Two integer or real values are equal if their contained numeric
 *   values are equal. An integer value is never equal to a real value,
 *   though.
 * - Two strings are equal if their contained UTF-8 strings are equal,
 *   byte by byte. Unicode comparison algorithms are not implemented.
 * - Two arrays are equal if they have the same number of elements and
 *   each element in the first array is equal to the corresponding
 *   element in the second array.
 * - Two objects are equal if they have exactly the same keys and the
 *   value for each key in the first object is equal to the value of
 *   the corresponding key in the second object.
 * - Two true, false or null values have no "contents", so they are
 *   equal if their types are equal.
 *
 */

/**
 * Test whether two JSON values are equal.
 *
 * @param hObj              Handle to the first JSON value
 * @param hOther            Handle to the second JSON value
 *
 * @error                   Invalid or closed handle.
 * @return                  True if the values are equal, false otherwise.
 */
native bool json_equal(Handle hObj, Handle hOther);




/**
 * --- Copying
 *
 * Jansson supports two kinds of copying: shallow and deep. There is
 * a difference between these methods only for arrays and objects.
 *
 * Shallow copying only copies the first level value (array or object)
 * and uses the same child values in the copied value.
 *
 * Deep copying makes a fresh copy of the child values, too. Moreover,
 * all the child values are deep copied in a recursive fashion.
 *
 */

/**
 * Get a shallow copy of the passed value.
 * The child values of a copied object or array are shared with the
 * original, so changing a nested object or array changes both.
 * The returned Handle must be closed.
 *
 * @param hObj              Handle to the JSON value to be copied
 *
 * @error                   Invalid or closed handle.
 * @return                  Handle to the copy.
 */
native Handle json_copy(Handle hObj);

/**
 * Get a deep copy of the passed value. All child values are copied
 * recursively, so the copy is independent of the original.
 * The returned Handle must be closed.
 *
 * @param hObj              Handle to the JSON value to be copied
 *
 * @error                   Invalid or closed handle.
 * @return                  Handle to the copy.
 */
native Handle json_deep_copy(Handle hObj);




/**
 * --- Objects
 *
 * A JSON object is a dictionary of key-value pairs, where the
 * key is a Unicode string and the value is any JSON value.
 *
 */

/**
 * Create a new, empty JSON object.
 * The returned Handle must be closed.
 *
 * @return                  Handle to the new JSON object.
 */
native Handle json_object();

/**
 * Returns the number of keys in hObj.
 *
 * @param hObj              Handle to JSON object
 *
 * @error                   Invalid or closed handle.
 * @return                  Number of keys in hObj,
 *                          or 0 if hObj is not a JSON object.
 */
native int json_object_size(Handle hObj);

/**
 * Get the value stored under sKey in hObj.
 * The returned Handle points to the same value that is stored in hObj,
 * not to a copy: changes made through it are visible in hObj. It must
 * be closed; closing it does not remove the value from hObj.
 * sKey is used as it is: "a.b" is a key named "a.b", not a path.
 *
 * @param hObj              Handle to JSON object to get a value from
 * @param sKey              Key to retrieve
 *
 * @error                   Invalid or closed handle.
 * @return                  Handle to the value, or INVALID_HANDLE if the
 *                          key does not exist or hObj is not a JSON
 *                          object.
 */
native Handle json_object_get(Handle hObj, const char[] sKey);

/**
 * Set the value of sKey to hValue in hObj.
 * If there already is a value for key, it is replaced by the new value.
 * hValue is stored itself, not a copy. The caller still owns the
 * Handle hValue and must close it.
 *
 * @param hObj              Handle to JSON object to set a value on
 * @param sKey              Key to store in the object
 *                          Must be a valid null terminated UTF-8 encoded
 *                          Unicode string.
 * @param hValue            Value to store in the object
 *
 * @error                   Invalid or closed handle.
 * @return                  True on success, false if hObj is not a JSON
 *                          object.
 */
native bool json_object_set(Handle hObj, const char[] sKey, Handle hValue);

/**
 * Set the value of sKey to hValue in hObj.
 * If there already is a value for key, it is replaced by the new value.
 * On success this function closes the Handle hValue; do not use it
 * afterwards. On failure hValue is not closed.
 *
 * @param hObj              Handle to JSON object to set a value on
 * @param sKey              Key to store in the object
 *                          Must be a valid null terminated UTF-8 encoded
 *                          Unicode string.
 * @param hValue            Value to store in the object
 *
 * @error                   Invalid or closed handle.
 * @return                  True on success, false if hObj is not a JSON
 *                          object or sKey is not valid UTF-8.
 */
native bool json_object_set_new(Handle hObj, const char[] sKey, Handle hValue);

/**
 * Delete sKey and its value from hObj.
 * Handles to the deleted value stay valid.
 *
 * @param hObj              Handle to JSON object to delete a key from
 * @param sKey              Key to delete
 *
 * @error                   Invalid or closed handle.
 * @return                  True on success, false if the key does not
 *                          exist or hObj is not a JSON object.
 */
native bool json_object_del(Handle hObj, const char[] sKey);

/**
 * Remove all keys from hObj.
 *
 * @param hObj              Handle to JSON object to remove all
 *                          keys from.
 *
 * @error                   Invalid or closed handle.
 * @return                  True on success, false if hObj is not a JSON
 *                          object.
 */
native bool json_object_clear(Handle hObj);

/**
 * Update hObj with the key-value pairs from hOther, overwriting
 * existing keys. The values are shared with hOther, not copied.
 *
 * @param hObj              Handle to JSON object to update
 * @param hOther            Handle to JSON object to get update
 *                          keys/values from.
 *
 * @error                   Invalid or closed handle.
 * @return                  True on success, false if either value is not
 *                          a JSON object.
 */
native bool json_object_update(Handle hObj, Handle hOther);

/**
 * Like json_object_update(), but only the values of existing keys
 * are updated. No new keys are created.
 *
 * @param hObj              Handle to JSON object to update
 * @param hOther            Handle to JSON object to get update
 *                          keys/values from.
 *
 * @error                   Invalid or closed handle.
 * @return                  True on success, false if either value is not
 *                          a JSON object.
 */
native bool json_object_update_existing(Handle hObj, Handle hOther);

/**
 * Like json_object_update(), but only new keys are created.
 * The value of any existing key is not changed.
 *
 * @param hObj              Handle to JSON object to update
 * @param hOther            Handle to JSON object to get update
 *                          keys/values from.
 *
 * @error                   Invalid or closed handle.
 * @return                  True on success, false if either value is not
 *                          a JSON object.
 */
native bool json_object_update_missing(Handle hObj, Handle hOther);

/**
 * Like json_object_update(), but on success this function closes the
 * Handle hOther; do not use it afterwards. On failure hOther is not
 * closed.
 *
 * @param hObj              Handle to JSON object to update
 * @param hOther            Handle to JSON object to get update
 *                          keys/values from.
 *
 * @error                   Invalid or closed handle.
 * @return                  True on success, false if either value is not
 *                          a JSON object.
 */
native bool json_object_update_new(Handle hObj, Handle hOther);

/**
 * Like json_object_update_existing(), but on success this function closes
 * the Handle hOther; do not use it afterwards. On failure hOther is not
 * closed.
 *
 * @param hObj              Handle to JSON object to update
 * @param hOther            Handle to JSON object to get update
 *                          keys/values from.
 *
 * @error                   Invalid or closed handle.
 * @return                  True on success, false if either value is not
 *                          a JSON object.
 */
native bool json_object_update_existing_new(Handle hObj, Handle hOther);

/**
 * Like json_object_update_missing(), but on success this function closes
 * the Handle hOther; do not use it afterwards. On failure hOther is not
 * closed.
 *
 * @param hObj              Handle to JSON object to update
 * @param hOther            Handle to JSON object to get update
 *                          keys/values from.
 *
 * @error                   Invalid or closed handle.
 * @return                  True on success, false if either value is not
 *                          a JSON object.
 */
native bool json_object_update_missing_new(Handle hObj, Handle hOther);

/**
 * Like json_object_update(), but merges nested objects: where a key holds
 * an object in both hObj and hOther, that object is updated recursively
 * instead of being replaced. All other values of hOther overwrite the
 * values in hObj. The values are shared with hOther, not copied.
 *
 * @param hObj              Handle to JSON object to update
 * @param hOther            Handle to JSON object to get update
 *                          keys/values from.
 *
 * @error                   Invalid or closed handle.
 * @return                  True on success, false if either value is not
 *                          a JSON object.
 */
native bool json_object_update_recursive(Handle hObj, Handle hOther);




/**
 * Object iteration
 *
 * An iterator is a position in an object and a Handle of its own.
 * json_object_iter() and json_object_iter_at() always return an
 * iterator, also for an empty object, a value that is not an object or
 * a key that does not exist. Such an iterator points at nothing:
 * json_object_iter_key() returns -1 and json_object_iter_value()
 * returns INVALID_HANDLE. Always check json_object_iter_key().
 *
 * Do not add or delete keys of the object while iterating over it.
 * Changing the value at the iterator with json_object_iter_set() is
 * fine. The keys come in the order they were added to the object (for
 * decoded JSON text: the order in the text).
 *
 * Example code:
 *  - We assume hObj is a Handle to a valid JSON object.
 *
 *
 * Handle hIterator = json_object_iter(hObj);
 * while (hIterator != null)
 * {
 *     char sKey[128];
 *     if (json_object_iter_key(hIterator, sKey, sizeof(sKey)) == -1)
 *     {
 *         // hObj is empty, the iterator points at nothing
 *         delete hIterator;
 *         break;
 *     }
 *
 *     Handle hValue = json_object_iter_value(hIterator);
 *
 *     // Do something with sKey and hValue
 *
 *     delete hValue;
 *
 *     // Closes hIterator, returns null after the last key
 *     hIterator = json_object_iter_next(hObj, hIterator);
 * }
 *
 */

/**
 * Returns a handle to an iterator which can be used to iterate over
 * all key-value pairs in hObj.
 * If you are not iterating to the end of hObj make sure to close the
 * handle to the iterator manually.
 *
 * @param hObj              Handle to JSON object to get an iterator
 *                          for.
 *
 * @error                   Invalid or closed handle.
 * @return                  Handle to JSON object iterator. It points at
 *                          nothing if hObj is empty or not a JSON object.
 */
native Handle json_object_iter(Handle hObj);

/**
 * Like json_object_iter(), but returns an iterator to the key-value
 * pair in object whose key is equal to key.
 * Iterating forward from there yields key and the keys that were added
 * to the object after it.
 *
 * @param hObj              Handle to JSON object to get an iterator
 *                          for.
 * @param key               Start key for the iterator
 *
 * @error                   Invalid or closed handle.
 * @return                  Handle to JSON object iterator. It points at
 *                          nothing if the key does not exist.
 */
native Handle json_object_iter_at(Handle hObj, const char[] key);

/**
 * Returns an iterator pointing to the next key-value pair in object.
 * This automatically closes the Handle to the iterator hIter.
 *
 * @param hObj              Handle to the JSON object hIter belongs to.
 * @param hIter             Handle to JSON object iterator.
 *
 * @error                   Invalid or closed handle.
 * @return                  Handle to JSON object iterator,
 *                          or INVALID_HANDLE if the whole object has
 *                          been iterated through.
 */
native Handle json_object_iter_next(Handle hObj, Handle hIter);

/**
 * Extracts the associated key of hIter as a null terminated UTF-8
 * encoded string in the passed buffer. A key longer than the buffer is
 * cut off.
 *
 * @param hIter             Handle to JSON object iterator.
 * @param sKeyBuffer        Buffer to store the key.
 * @param maxlength         Maximum length of string buffer.
 *
 * @error                   Invalid or closed handle.
 * @return                  Length of the whole key in bytes (larger than
 *                          or equal to maxlength if it was cut off), or
 *                          -1 if the iterator points at nothing.
 */
native int json_object_iter_key(Handle hIter, char[] sKeyBuffer, int maxlength);

/**
 * Returns a handle to the value hIter is pointing at.
 * The value is shared with the object, not copied. The returned Handle
 * must be closed.
 *
 * @param hIter             Handle to JSON object iterator.
 *
 * @error                   Invalid or closed handle.
 * @return                  Handle to value, or INVALID_HANDLE if the
 *                          iterator points at nothing.
 */
native Handle json_object_iter_value(Handle hIter);

/**
 * Set the value of the key-value pair in hObj, that is pointed to
 * by hIter, to hValue.
 * The caller still owns the Handle hValue and must close it.
 *
 * @param hObj              Handle to JSON object.
 * @param hIter             Handle to JSON object iterator.
 * @param hValue            Handle to JSON value.
 *
 * @error                   Invalid or closed handle.
 * @return                  True on success, false if hObj is not a JSON
 *                          object or the iterator points at nothing.
 */
native bool json_object_iter_set(Handle hObj, Handle hIter, Handle hValue);

/**
 * Set the value of the key-value pair in hObj, that is pointed to
 * by hIter, to hValue.
 * On success this function closes the Handle hValue; do not use it
 * afterwards. On failure hValue is not closed.
 *
 * @param hObj              Handle to JSON object.
 * @param hIter             Handle to JSON object iterator.
 * @param hValue            Handle to JSON value.
 *
 * @error                   Invalid or closed handle.
 * @return                  True on success, false if hObj is not a JSON
 *                          object or the iterator points at nothing.
 */
native bool json_object_iter_set_new(Handle hObj, Handle hIter, Handle hValue);




/**
 * Arrays
 *
 * A JSON array is an ordered collection of other JSON values.
 *
 */

/**
 * Create a new, empty JSON array.
 * The returned Handle must be closed.
 *
 * @return                  Handle to the new JSON array
 */
native Handle json_array();

/**
 * Returns the number of elements in hArray
 *
 * @param hArray            Handle to JSON array
 *
 * @error                   Invalid or closed handle.
 * @return                  Number of elements in hArray,
 *                          or 0 if hArray is not a JSON array.
 */
native int json_array_size(Handle hArray);

/**
 * Returns the element in hArray at position iIndex.
 * The returned Handle points to the same value that is stored in
 * hArray, not to a copy. It must be closed.
 *
 * @param hArray            Handle to JSON array to get a value from
 * @param iIndex            Position to retrieve
 *
 * @error                   Invalid or closed handle.
 * @return                  Handle to the element, or INVALID_HANDLE if
 *                          there is no element at iIndex or hArray is
 *                          not a JSON array.
 */
native Handle json_array_get(Handle hArray, int iIndex);

/**
 * Replaces the element in array at position iIndex with hValue.
 * The valid range for iIndex is from 0 to the return value of
 * json_array_size() minus 1.
 * The caller still owns the Handle hValue and must close it.
 *
 * @param hArray            Handle to JSON array
 * @param iIndex            Position to replace
 * @param hValue            Value to store in the array
 *
 * @error                   Invalid or closed handle.
 * @return                  True on success, false if iIndex is out of
 *                          range or hArray is not a JSON array.
 */
native bool json_array_set(Handle hArray, int iIndex, Handle hValue);

/**
 * Replaces the element in array at position iIndex with hValue.
 * The valid range for iIndex is from 0 to the return value of
 * json_array_size() minus 1.
 * On success this function closes the Handle hValue; do not use it
 * afterwards. On failure hValue is not closed.
 *
 * @param hArray            Handle to JSON array
 * @param iIndex            Position to replace
 * @param hValue            Value to store in the array
 *
 * @error                   Invalid or closed handle.
 * @return                  True on success, false if iIndex is out of
 *                          range or hArray is not a JSON array.
 */
native bool json_array_set_new(Handle hArray, int iIndex, Handle hValue);

/**
 * Appends value to the end of array, growing the size of array by 1.
 * The caller still owns the Handle hValue and must close it.
 *
 * @param hArray            Handle to JSON array
 * @param hValue            Value to append to the array
 *
 * @error                   Invalid or closed handle.
 * @return                  True on success, false if hArray is not a
 *                          JSON array.
 */
native bool json_array_append(Handle hArray, Handle hValue);

/**
 * Appends value to the end of array, growing the size of array by 1.
 * On success this function closes the Handle hValue; do not use it
 * afterwards. On failure hValue is not closed.
 *
 * @param hArray            Handle to JSON array
 * @param hValue            Value to append to the array
 *
 * @error                   Invalid or closed handle.
 * @return                  True on success, false if hArray is not a
 *                          JSON array.
 */
native bool json_array_append_new(Handle hArray, Handle hValue);

/**
 * Inserts value to hArray at position iIndex, shifting the elements at
 * iIndex and after it one position towards the end of the array.
 * The valid range for iIndex is from 0 to the return value of
 * json_array_size(); the size appends to the end.
 * The caller still owns the Handle hValue and must close it.
 *
 * @param hArray            Handle to JSON array
 * @param iIndex            Position to insert at
 * @param hValue            Value to store in the array
 *
 * @error                   Invalid or closed handle.
 * @return                  True on success, false if iIndex is out of
 *                          range or hArray is not a JSON array.
 */
native bool json_array_insert(Handle hArray, int iIndex, Handle hValue);

/**
 * Inserts value to hArray at position iIndex, shifting the elements at
 * iIndex and after it one position towards the end of the array.
 * The valid range for iIndex is from 0 to the return value of
 * json_array_size(); the size appends to the end.
 * On success this function closes the Handle hValue; do not use it
 * afterwards. On failure hValue is not closed.
 *
 * @param hArray            Handle to JSON array
 * @param iIndex            Position to insert at
 * @param hValue            Value to store in the array
 *
 * @error                   Invalid or closed handle.
 * @return                  True on success, false if iIndex is out of
 *                          range or hArray is not a JSON array.
 */
native bool json_array_insert_new(Handle hArray, int iIndex, Handle hValue);

/**
 * Removes the element in hArray at position iIndex, shifting the
 * elements after iIndex one position towards the start of the array.
 *
 * @param hArray            Handle to JSON array
 * @param iIndex            Position to remove
 *
 * @error                   Invalid or closed handle.
 * @return                  True on success, false if there is no
 *                          element at iIndex.
 */
native bool json_array_remove(Handle hArray, int iIndex);

/**
 * Removes all elements from hArray.
 *
 * @param hArray            Handle to JSON array
 *
 * @error                   Invalid or closed handle.
 * @return                  True on success, false if hArray is not a
 *                          JSON array.
 */
native bool json_array_clear(Handle hArray);

/**
 * Appends all elements in hOther to the end of hArray.
 * The elements are shared with hOther, not copied.
 *
 * @param hArray            Handle to JSON array to be extended
 * @param hOther            Handle to JSON array, source to copy from
 *
 * @error                   Invalid or closed handle.
 * @return                  True on success, false if either value is not
 *                          a JSON array.
 */
native bool json_array_extend(Handle hArray, Handle hOther);




/**
 * Booleans & NULL
 *
 * Read a boolean with json_boolean_value() (or json_is_true()).
 * The JSON value null is unrelated to INVALID_HANDLE.
 *
 */

/**
 * Returns a handle to a new JSON Boolean with value true.
 * The returned Handle must be closed.
 *
 * @return                  Handle to the new Boolean object
 */
native Handle json_true();

/**
 * Returns a handle to a new JSON Boolean with value false.
 * The returned Handle must be closed.
 *
 * @return                  Handle to the new Boolean object
 */
native Handle json_false();

/**
 * Returns a handle to a new JSON Boolean with the value passed
 * in bState. The returned Handle must be closed.
 *
 * @param bState            Value for the new Boolean object
 * @return                  Handle to the new Boolean object
 */
native Handle json_boolean(bool bState);

/**
 * Returns a handle to a new JSON null. The returned Handle must be
 * closed.
 *
 * @return                  Handle to the new NULL object
 */
native Handle json_null();




/**
 * Strings
 *
 * Jansson uses UTF-8 as the character encoding. All JSON strings must
 * be valid UTF-8 (or ASCII, as it's a subset of UTF-8). Normal null
 * terminated C strings are used, so JSON strings may not contain
 * embedded null characters.
 *
 * Natives that copy text into a buffer cut it off if it does not fit,
 * never in the middle of a UTF-8 character, and return the length of
 * the whole text. A return value larger than or equal to maxlength
 * means the text was cut off.
 *
 */

/**
 * Returns a handle to a new JSON string. The returned Handle must be
 * closed.
 * If sValue is not valid UTF-8, the returned Handle holds no value:
 * adding it to an object or array fails and json_typeof() raises an
 * error.
 *
 * @param sValue            Value for the new String object
 *                          Must be a valid UTF-8 encoded Unicode string.
 * @return                  Handle to the new String object
 */
native Handle json_string(const char[] sValue);

/**
 * Saves the associated value of hString as a null terminated UTF-8
 * encoded string in the passed buffer.
 *
 * @param hString           Handle to the JSON String object
 * @param sValueBuffer      Buffer to store the value of the String.
 * @param maxlength         Maximum length of string buffer.
 * @error                   Invalid or closed handle.
 * @return                  Length of the whole string in bytes, or -1
 *                          if hString is not a JSON string (the buffer
 *                          is then not changed).
 */
native int json_string_value(Handle hString, char[] sValueBuffer, int maxlength);

/**
 * Sets the associated value of JSON String object to value.
 *
 * @param hString           Handle to the JSON String object
 * @param sValue            Value to set the object to.
 *                          Must be a valid UTF-8 encoded Unicode string.
 * @error                   Invalid or closed handle.
 * @return                  True on success, false if hString is not a
 *                          JSON string.
 */
native bool json_string_set(Handle hString, char[] sValue);




/**
 * Numbers
 *
 * The JSON specification only contains one numeric type, 'number'.
 * The C (and Pawn) programming language has distinct types for integer
 * and floating-point numbers, so for practical reasons Jansson also has
 * distinct types for the two. They are called 'integer' and 'real',
 * respectively. (Whereas 'real' is a 'float' for Pawn).
 * Therefore a number is represented by either a value of the type
 * JSON_INTEGER or of the type JSON_REAL. A number with a decimal point
 * or an exponent, such as 1.0 or 1e3, is a real.
 *
 * Pawn integers and floats have 32 bits. Integers in JSON text that do
 * not fit into 32 bits are kept intact in the JSON value, but
 * json_integer_value() returns only their lowest 32 bits. Reals are
 * written with the fewest digits that read back as the same 64-bit
 * value. A Pawn float turned into that value is often longer than it
 * looks: json_real(0.1) is written as 0.10000000149011612. Use
 * json_dump_ex() with JSON_REAL_PRECISION() to write fewer digits.
 *
 */

/**
 * Returns a handle to a new JSON integer. The returned Handle must be
 * closed.
 *
 * @param iValue            Value for the new Integer object
 * @return                  Handle to the new Integer object
 */
native Handle json_integer(int iValue);

/**
 * Returns the associated value of a JSON Integer Object.
 *
 * @param hInteger          Handle to the JSON Integer object
 * @error                   Invalid or closed handle.
 * @return                  Value of the hInteger, or 0 if hInteger is
 *                          not a JSON integer (also for a JSON real).
 */
native int json_integer_value(Handle hInteger);

/**
 * Sets the associated value of JSON Integer to value.
 *
 * @param hInteger          Handle to the JSON Integer object
 * @param iValue            Value to set the object to.
 * @error                   Invalid or closed handle.
 * @return                  True on success, false if hInteger is not a
 *                          JSON integer.
 */
native bool json_integer_set(Handle hInteger, int iValue);

/**
 * Returns a handle to a new JSON real. The returned Handle must be
 * closed.
 *
 * @param fValue            Value for the new Real object
 * @return                  Handle to the new Real object
 */
native Handle json_real(float fValue);

/**
 * Returns the associated value of a JSON Real.
 *
 * @param hReal             Handle to the JSON Real object
 * @error                   Invalid or closed handle.
 * @return                  Float value of hReal, or 0.0 if hReal is not
 *                          a JSON real (also for a JSON integer).
 */
native float json_real_value(Handle hReal);

/**
 * Sets the associated value of JSON Real to value.
 *
 * @param hReal             Handle to the JSON Real object
 * @param value             Value to set the object to.
 * @error                   Invalid or closed handle.
 * @return                  True on success, false if hReal is not a
 *                          JSON real.
 */
native bool json_real_set(Handle hReal, float value);

/**
 * Returns the associated value of a JSON integer or a
 * JSON Real, cast to Float regardless of the actual type.
 *
 * @param hNumber           Handle to the JSON Number
 * @error                   Invalid or closed handle.
 * @return                  Float value of hNumber,
 *                          or 0.0 if hNumber is not a JSON number.
 */
native float json_number_value(Handle hNumber);




/**
 * Decoding
 *
 * This sections describes the functions that can be used to decode JSON text
 * to the Jansson representation of JSON data. By default the top level value
 * in the JSON text being decoded must be either array or object; pass
 * JSON_DECODE_ANY to accept any JSON value.
 *
 * Each decoding function takes optional flags, combined with |:
 *  JSON_REJECT_DUPLICATES   Fail if an object in the text has the same key
 *                           twice. Without it, the last value wins.
 *  JSON_DISABLE_EOF_CHECK   Stop after the first array or object and ignore
 *                           the rest of the text. Without it, anything but
 *                           whitespace after the value is an error.
 *  JSON_DECODE_ANY          Accept any JSON value at the top level, also a
 *                           string, number, true, false or null.
 *  JSON_DECODE_INT_AS_REAL  Decode every number as a JSON real, so integers
 *                           read with json_real_value() and json_number_value()
 *                           alike.
 * Other values raise an error.
 *
 */

#define JSON_REJECT_DUPLICATES  0x1
#define JSON_DISABLE_EOF_CHECK  0x2
#define JSON_DECODE_ANY         0x4
#define JSON_DECODE_INT_AS_REAL 0x8

/**
 * The reason a decoding function failed, as returned by json_load_ex() and
 * json_load_file_ex(). The values are the ones of Jansson's enum
 * json_error_code; decoding returns these:
 *  json_error_out_of_memory            Not enough memory.
 *  json_error_stack_overflow           Nested deeper than 2048 levels.
 *  json_error_cannot_open_file         The file can not be opened.
 *  json_error_invalid_utf8             The text is not valid UTF-8.
 *  json_error_premature_end_of_input   The text ends in the middle of a value.
 *  json_error_end_of_input_expected    There is more text after the value.
 *  json_error_invalid_syntax           The text is not valid JSON.
 *  json_error_null_character           The text contains \u0000.
 *  json_error_null_byte_in_key         An object key contains \u0000.
 *  json_error_duplicate_key            A key appears twice in an object
 *                                      (only with JSON_REJECT_DUPLICATES).
 *  json_error_numeric_overflow         A number is too big for Jansson.
 * The others are not returned by decoding.
 */
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

/**
 * Decodes the JSON string sJSON and returns the value it contains.
 * Errors while decoding are written to the SourceMod error log.
 * The returned Handle must be closed.
 *
 * @param sJSON             String containing valid JSON
 * @param iFlags            Decoding flags, see 'Decoding'.
 *
 * @error                   Unsupported flags.
 * @return                  Handle to the decoded value (an object or
 *                          array unless JSON_DECODE_ANY is passed),
 *                          or INVALID_HANDLE on error.
 */
native Handle json_load(const char[] sJSON, int iFlags = 0);

/**
 * Decodes the JSON string sJSON and returns the value it contains.
 * This function provides additional error feedback and does not log errors
 * to the sourcemod error log. On success sErrorText, iLine, iColumn and
 * iErrorCode are not changed. The returned Handle must be closed.
 *
 * @param sJSON             String containing valid JSON
 * @param sErrorText        This buffer will be filled with the error
 *                          message.
 * @param maxlen            Size of the buffer
 * @param iLine             This int will contain the line of the error,
 *                          starting at 1.
 * @param iColumn           This int will contain the column of the error,
 *                          starting at 1.
 * @param iFlags            Decoding flags, see 'Decoding'.
 * @param iErrorCode        This will contain the reason of the error.
 *
 * @error                   Unsupported flags.
 * @return                  Handle to the decoded value (an object or
 *                          array unless JSON_DECODE_ANY is passed),
 *                          or INVALID_HANDLE on error.
 */
native Handle json_load_ex(const char[] sJSON, char[] sErrorText, int maxlen, int &iLine, int &iColumn, int iFlags = 0, json_error_code &iErrorCode = json_error_unknown);

/**
 * Decodes the JSON text in file sFilePath and returns the value it
 * contains.
 * Errors while decoding, and a file that can not be opened, are written
 * to the SourceMod error log. The returned Handle must be closed.
 *
 * @param sFilePath         Path to a file containing pure JSON, relative
 *                          to the game directory (as BuildPath() returns
 *                          it). Prefix an absolute path with "file://".
 * @param iFlags            Decoding flags, see 'Decoding'.
 *
 * @error                   Unsupported flags.
 * @return                  Handle to the decoded value (an object or
 *                          array unless JSON_DECODE_ANY is passed),
 *                          or INVALID_HANDLE on error.
 */
native Handle json_load_file(const char sFilePath[PLATFORM_MAX_PATH], int iFlags = 0);

/**
 * Decodes the JSON text in file sFilePath and returns the value it
 * contains.
 * This function provides additional error feedback and does not log errors
 * to the sourcemod error log. On success sErrorText, iLine, iColumn and
 * iErrorCode are not changed. The returned Handle must be closed.
 *
 * @param sFilePath         Path to a file containing pure JSON, relative
 *                          to the game directory (as BuildPath() returns
 *                          it). Prefix an absolute path with "file://".
 * @param sErrorText        This buffer will be filled with the error
 *                          message.
 * @param maxlen            Size of the buffer
 * @param iLine             This int will contain the line of the error,
 *                          starting at 1, or -1 if the file can not be
 *                          opened.
 * @param iColumn           This int will contain the column of the error,
 *                          starting at 1, or -1 if the file can not be
 *                          opened.
 * @param iFlags            Decoding flags, see 'Decoding'.
 * @param iErrorCode        This will contain the reason of the error.
 *
 * @error                   Unsupported flags.
 * @return                  Handle to the decoded value (an object or
 *                          array unless JSON_DECODE_ANY is passed),
 *                          or INVALID_HANDLE on error.
 */
native Handle json_load_file_ex(const char sFilePath[PLATFORM_MAX_PATH], char[] sErrorText, int maxlen, int &iLine, int &iColumn, int iFlags = 0, json_error_code &iErrorCode = json_error_unknown);



/**
 * Encoding
 *
 * This sections describes the functions that can be used to encode values
 * to JSON. By default, only objects and arrays can be encoded directly,
 * since they are the only valid root values of a JSON text.
 * Object keys are written in the order they were added to the object,
 * unless the keys are sorted.
 *
 * json_dump_ex() and json_dump_file_ex() take their options as flags,
 * combined with |:
 *  JSON_INDENT(n)           Pretty-print with n spaces per level, 0 to 31.
 *                           Without it (n = 0) everything is on one line.
 *  JSON_COMPACT             No space after ',' and ':'.
 *  JSON_ENSURE_ASCII        Escape every character outside ASCII.
 *  JSON_SORT_KEYS           Sort the keys of objects.
 *  JSON_ENCODE_ANY          Also encode values that are not an object or
 *                           array, such as a single string or number.
 *  JSON_ESCAPE_SLASH        Write '/' as '\/'.
 *  JSON_REAL_PRECISION(n)   Write reals with at most n significant digits,
 *                           1 to 31. Without it (n = 0) reals are written
 *                           with the fewest digits that read back exactly.
 * Other values raise an error.
 *
 */

#define JSON_MAX_INDENT         0x1F
#define JSON_INDENT(%1)         ((%1) & JSON_MAX_INDENT)
#define JSON_COMPACT            0x20
#define JSON_ENSURE_ASCII       0x40
#define JSON_SORT_KEYS          0x80
#define JSON_ENCODE_ANY         0x200
#define JSON_ESCAPE_SLASH       0x400
#define JSON_REAL_PRECISION(%1) (((%1) & 0x1F) << 11)

/**
 * Saves the JSON representation of hObject in sJSON.
 * Only objects and arrays can be encoded. Output that does not fit into
 * sJSON is cut off.
 *
 * @param hObject           Handle to the JSON object or array to encode
 * @param sJSON             Buffer to store the created JSON string.
 * @param maxlength         Maximum length of string buffer.
 * @param iIndentWidth      Indenting with iIndentWidth spaces.
 *                          The valid range for this is between 0 and 31 (inclusive);
 *                          only the lowest 5 bits are used, so 32 acts like 0. If this
 *                          is set to 0, no newlines are inserted between array and
 *                          object items.
 * @param bEnsureAscii      If this is set, the output is guaranteed
 *                          to consist only of ASCII characters. This is achieved
 *                          by escaping all Unicode characters outside the ASCII range.
 * @param bSortKeys         If this flag is used, all the objects in output are sorted
 *                          by key. This is useful e.g. if two JSON texts are diffed
 *                          or visually compared.
 * @param bPreserveOrder    Has no effect. Object keys are always written in the order
 *                          they were added, unless bSortKeys is set. Kept so that
 *                          existing code compiles.
 * @error                   Invalid or closed handle.
 * @return                  Length of the whole JSON string in bytes (larger
 *                          than or equal to maxlength if it was cut off), or
 *                          -1 if hObject is not a JSON object or array.
 */
native int json_dump(Handle hObject, char[] sJSON, int maxlength, int iIndentWidth = 4, bool bEnsureAscii = false, bool bSortKeys = false, bool bPreserveOrder = false);

/**
 * Write the JSON representation of hObject to the file sFilePath.
 * If sFilePath already exists, it is overwritten. Missing directories
 * are not created. Only objects and arrays can be encoded.
 *
 * @param hObject           Handle to the JSON object or array to encode
 * @param sFilePath         Path of the file to write, relative to the game
 *                          directory (as BuildPath() returns it). Prefix an
 *                          absolute path with "file://".
 * @param iIndentWidth      Indenting with iIndentWidth spaces.
 *                          The valid range for this is between 0 and 31 (inclusive);
 *                          only the lowest 5 bits are used, so 32 acts like 0. If this
 *                          is set to 0, no newlines are inserted between array and
 *                          object items.
 * @param bEnsureAscii      If this is set, the output is guaranteed
 *                          to consist only of ASCII characters. This is achieved
 *                          by escaping all Unicode characters outside the ASCII range.
 * @param bSortKeys         If this flag is used, all the objects in output are sorted
 *                          by key. This is useful e.g. if two JSON texts are diffed
 *                          or visually compared.
 * @param bPreserveOrder    Has no effect. Object keys are always written in the order
 *                          they were added, unless bSortKeys is set. Kept so that
 *                          existing code compiles.
 * @error                   Invalid or closed handle.
 * @return                  True on success, false if hObject is not a JSON
 *                          object or array or the file can not be written.
 */
native bool json_dump_file(Handle hObject, const char[] sFilePath, int iIndentWidth = 4, bool bEnsureAscii = false, bool bSortKeys = false, bool bPreserveOrder = false);

/**
 * Like json_dump(), with the options as flags (see 'Encoding').
 * With JSON_ENCODE_ANY any JSON value can be encoded.
 *
 * @param hObject           Handle to the JSON value to encode
 * @param sJSON             Buffer to store the created JSON string.
 * @param maxlength         Maximum length of string buffer.
 * @param iFlags            Encoding flags, see 'Encoding'.
 * @error                   Invalid or closed handle, or unsupported flags.
 * @return                  Length of the whole JSON string in bytes (larger
 *                          than or equal to maxlength if it was cut off), or
 *                          -1 if the value can not be encoded (not an object
 *                          or array without JSON_ENCODE_ANY).
 */
native int json_dump_ex(Handle hObject, char[] sJSON, int maxlength, int iFlags);

/**
 * Like json_dump_file(), with the options as flags (see 'Encoding').
 * With JSON_ENCODE_ANY any JSON value can be encoded.
 *
 * @param hObject           Handle to the JSON value to encode
 * @param sFilePath         Path of the file to write, relative to the game
 *                          directory (as BuildPath() returns it). Prefix an
 *                          absolute path with "file://".
 * @param iFlags            Encoding flags, see 'Encoding'.
 * @error                   Invalid or closed handle, or unsupported flags.
 * @return                  True on success, false if the value can not be
 *                          encoded (not an object or array without
 *                          JSON_ENCODE_ANY) or the file can not be written.
 */
native bool json_dump_file_ex(Handle hObject, const char[] sFilePath, int iFlags);



/**
 * Convenience stocks
 *
 * These are some custom functions to ease the development using this
 * extension.
 *
 * The json_array_get_* and json_object_get_* stocks require the index or
 * key to exist. If it does not, json_typeof() raises an error. Use
 * json_array_get() or json_object_get() to check first.
 *
 */

/**
 * Returns a handle to a new JSON string. The returned Handle must be
 * closed.
 * Formats the string according to the SourceMod format rules, into a
 * buffer of 4096 bytes; longer results are cut off.
 * The result must be a valid UTF-8 encoded Unicode string.
 *
 * @param sFormat           Formatting rules.
 * @param ...               Variable number of format parameters.
 * @return                  Handle to the new String object
 */
stock Handle json_string_format(const char[] sFormat, any ...) {
	char sTmp[4096];
	VFormat(sTmp, sizeof(sTmp), sFormat, 2);

	return json_string(sTmp);
}

/**
 * Returns a handle to a new JSON string. The returned Handle must be
 * closed.
 * This stock allows to specify the size of the temporary buffer used
 * to create the string. Use this if the default of 4096 is not enough
 * for your string.
 * Formats the string according to the SourceMod format rules.
 * The result must be a valid UTF-8 encoded Unicode string.
 *
 * @param tmpBufferLength   Size of the temporary buffer
 * @param sFormat           Formatting rules.
 * @param ...               Variable number of format parameters.
 * @return                  Handle to the new String object
 */
stock Handle json_string_format_ex(int tmpBufferLength, const char[] sFormat, any ...) {
	char[] sTmp = new char[tmpBufferLength];
	VFormat(sTmp, tmpBufferLength, sFormat, 3);

	return json_string(sTmp);
}


/**
 * Returns the boolean value of the element in hArray at position iIndex.
 *
 * @param hArray            Handle to JSON array to get a value from
 * @param iIndex            Position to retrieve
 *
 * @error                   Invalid or closed handle, or no element at position iIndex.
 * @return                  True if the element is JSON true,
 *                          false otherwise.
 */
stock bool json_array_get_bool(Handle hArray, int iIndex) {
	Handle hElement = json_array_get(hArray, iIndex);

	bool bResult = (json_is_true(hElement) ? true : false);

	delete hElement;
	return bResult;
}

/**
 * Returns the float value of the element in hArray at position iIndex.
 *
 * @param hArray            Handle to JSON array to get a value from
 * @param iIndex            Position to retrieve
 *
 * @error                   Invalid or closed handle, or no element at position iIndex.
 * @return                  Float value of a JSON integer or real,
 *                          or 0.0 if the element is not a JSON number.
 */
stock float json_array_get_float(Handle hArray, int iIndex) {
	Handle hElement = json_array_get(hArray, iIndex);

	float fResult = (json_is_number(hElement) ? json_number_value(hElement) : 0.0);

	delete hElement;
	return fResult;
}

/**
 * Returns the integer value of the element in hArray at position iIndex.
 *
 * @param hArray            Handle to JSON array to get a value from
 * @param iIndex            Position to retrieve
 *
 * @error                   Invalid or closed handle, or no element at position iIndex.
 * @return                  Integer value, or 0 if the element is not
 *                          a JSON integer (also for a JSON real).
 */
stock int json_array_get_int(Handle hArray, int iIndex) {
	Handle hElement = json_array_get(hArray, iIndex);

	int iResult = (json_is_integer(hElement) ? json_integer_value(hElement) : 0);

	delete hElement;
	return iResult;
}

/**
 * Saves the associated value of the element in hArray at position iIndex
 * as a null terminated UTF-8 encoded string in the passed buffer.
 *
 * @param hArray            Handle to JSON array to get a value from
 * @param iIndex            Position to retrieve
 * @param sBuffer           Buffer to store the value of the String.
 * @param maxlength         Maximum length of string buffer.
 *
 * @error                   Invalid or closed handle, or no element at position iIndex.
 * @return                  Length of the whole string in bytes, or -1
 *                          if the element is not a JSON string (the
 *                          buffer is then not changed).
 */
stock int json_array_get_string(Handle hArray, int iIndex, char[] sBuffer, int maxlength) {
	Handle hElement = json_array_get(hArray, iIndex);

	int iResult = -1;
	if(json_is_string(hElement)) {
		iResult = json_string_value(hElement, sBuffer, maxlength);
	}
	delete hElement;

	return iResult;
}

/**
 * Returns the boolean value of the element in hObj at entry sKey.
 *
 * @param hObj              Handle to JSON object to get a value from
 * @param sKey              Key to retrieve
 *
 * @error                   Invalid or closed handle, or no element at entry sKey.
 * @return                  True if the element is JSON true,
 *                          false otherwise.
 */
stock bool json_object_get_bool(Handle hObj, const char[] sKey) {
	Handle hElement = json_object_get(hObj, sKey);

	bool bResult = json_is_true(hElement);

	delete hElement;
	return bResult;
}

/**
 * Returns the float value of the element in hObj at entry sKey.
 *
 * @param hObj              Handle to JSON object to get a value from
 * @param sKey              Key to retrieve
 *
 * @error                   Invalid or closed handle, or no element at entry sKey.
 * @return                  Float value of a JSON integer or real,
 *                          or 0.0 if the element is not a JSON number.
 */
stock float json_object_get_float(Handle hObj, const char[] sKey) {
	Handle hElement = json_object_get(hObj, sKey);

	float fResult = (json_is_number(hElement) ? json_number_value(hElement) : 0.0);

	delete hElement;
	return fResult;
}

/**
 * Returns the integer value of the element in hObj at entry sKey.
 *
 * @param hObj              Handle to JSON object to get a value from
 * @param sKey              Key to retrieve
 *
 * @error                   Invalid or closed handle, or no element at entry sKey.
 * @return                  Integer value, or 0 if the element is not
 *                          a JSON integer (also for a JSON real).
 */
stock int json_object_get_int(Handle hObj, const char[] sKey) {
	Handle hElement = json_object_get(hObj, sKey);

	int iResult = (json_is_integer(hElement) ? json_integer_value(hElement) : 0);

	delete hElement;
	return iResult;
}

/**
 * Saves the associated value of the element in hObj at entry sKey
 * as a null terminated UTF-8 encoded string in the passed buffer.
 *
 * @param hObj              Handle to JSON object to get a value from
 * @param sKey              Key to retrieve
 * @param sBuffer           Buffer to store the value of the String.
 * @param maxlength         Maximum length of string buffer.
 *
 * @error                   Invalid or closed handle, or no element at entry sKey.
 * @return                  Length of the whole string in bytes, or -1
 *                          if the element is not a JSON string (the
 *                          buffer is then not changed).
 */
stock int json_object_get_string(Handle hObj, const char[] sKey, char[] sBuffer, int maxlength) {
	Handle hElement = json_object_get(hObj, sKey);

	int iResult = -1;
	if(json_is_string(hElement)) {
		iResult = json_string_value(hElement, sBuffer, maxlength);
	}
	delete hElement;

	return iResult;
}



/**
 * Pack String Rules
 *
 * Here's the full list of format characters:
 *  n    Output a JSON null value. No argument is consumed.
 *  s    Output a JSON string, consuming one argument.
 *  b    Output a JSON bool value, consuming one argument.
 *  i    Output a JSON integer value, consuming one argument.
 *  f    Output a JSON real value, consuming one argument.
 *  r    Output a JSON real value, consuming one argument.
 *  []   Build an array with contents from the inner format string,
 *       recursive value building is supported.
 *       No argument is consumed.
 *  {}   Build an object with contents from the inner format string.
 *       The first, third, etc. format character represent a key,
 *       and must be s (as object keys are always strings). The
 *       second, fourth, etc. format character represent a value.
 *       Recursive value building is supported.
 *       No argument is consumed.
 *  Spaces, ':' and ',' are ignored and can be used for readability.
 *
 */

/**
 * This method can be used to create json objects/arrays directly
 * without having to create the structure.
 * See 'Pack String Rules' for more details.
 * The returned Handle must be closed.
 *
 * @param sPackString       Pack string similar to the format string of
 *                          Format(). See 'Pack String Rules'.
 * @param hParams           ADT Array containing all keys and values
 *                          in the order they appear in the pack string:
 *                          strings and keys added with PushString()
 *                          (at most 254 bytes are used), other values
 *                          with Push(). The used values are removed
 *                          from hParams; the caller must still close it.
 *
 * @error                   hParams has fewer values than the pack string
 *                          needs. An unknown character in the pack string
 *                          calls SetFailState() and stops the plugin.
 * @return                  Handle to JSON element, or INVALID_HANDLE if
 *                          an object key in the pack string is not s
 *                          (the error is logged).
 */
stock Handle json_pack(const char[] sPackString, ArrayList hParams) {
	int iPos = 0;
	return json_pack_element_(sPackString, iPos, hParams);
}





/**
* Internal stocks used by json_pack(). Don't use these directly!
*
*/
stock Handle json_pack_array_(const char[] sFormat, int &iPos, ArrayList hParams) {
	Handle hObj = json_array();
	int iStrLen = strlen(sFormat);
	for(; iPos < iStrLen;) {
		int this_char = sFormat[iPos];

		if(this_char == 32 || this_char == 58 || this_char == 44) {
			// Skip whitespace, ',' and ':'
			iPos++;
			continue;
		}

		if(this_char == 93) {
			// array end
			iPos++;
			break;
		}

		// Get the next entry as value
		// This automatically increments the position!
		Handle hValue = json_pack_element_(sFormat, iPos, hParams);

		// Append the value to the array.
		json_array_append_new(hObj, hValue);
	}

	return hObj;
}

stock Handle json_pack_object_(const char[] sFormat, int &iPos, ArrayList hParams) {
	Handle hObj = json_object();
	int iStrLen = strlen(sFormat);
	for(; iPos < iStrLen;) {
		int this_char = sFormat[iPos];

		if(this_char == 32 || this_char == 58 || this_char == 44) {
			// Skip whitespace, ',' and ':'
			iPos++;
			continue;
		}

		if(this_char == 125) {
			// } --> object end
			iPos++;
			break;
		}

		if(this_char != 115) {
			LogError("Object keys must be strings at %d.", iPos);
			return INVALID_HANDLE;
		}

		// Get the key string for this object from
		// the hParams array.
		char sKey[255];
		hParams.GetString(0, sKey, sizeof(sKey));
		hParams.Erase(0);

		// Advance one character in the pack string,
		// because we've just read the Key string for
		// this object.
		iPos++;

		// Get the next entry as value
		// This automatically increments the position!
		Handle hValue = json_pack_element_(sFormat, iPos, hParams);

		// Insert into object
		json_object_set_new(hObj, sKey, hValue);
	}

	return hObj;
}

stock Handle json_pack_element_(const char[] sFormat, int &iPos, ArrayList hParams) {
	int this_char = sFormat[iPos];
	while(this_char == 32 || this_char == 58 || this_char == 44) {
		iPos++;
		this_char = sFormat[iPos];
	}

	// Advance one character in the pack string
	iPos++;

	switch(this_char) {
		case 91: {
			// {  --> Array
			return json_pack_array_(sFormat, iPos, hParams);
		}

		case 123: {
			// {  --> Object
			return json_pack_object_(sFormat, iPos, hParams);

		}

		case 98: {
			// b  --> Boolean
			int iValue = hParams.Get(0);
			hParams.Erase(0);

			return json_boolean(view_as<bool>(iValue));
		}

		case 102, 114: {
			// r,f  --> Real (Float)
			float iValue = hParams.Get(0);
			hParams.Erase(0);

			return json_real(iValue);
		}

		case 110: {
			// n --> NULL
			return json_null();
		}

		case 115: {
			// s  --> String
			char sKey[255];
			hParams.GetString(0, sKey, sizeof(sKey));
			hParams.Erase(0);

			return json_string(sKey);
		}

		case 105: {
			// i  --> Integer
			int iValue = hParams.Get(0);
			hParams.Erase(0);

			return json_integer(iValue);
		}
	}

	SetFailState("Invalid pack String '%s'. Type '%s' not supported at %i", sFormat, this_char, iPos);
	return json_null();
}





/**
 * Not yet implemented
 *
 * native json_object_foreach(Handle hObj, ForEachCallback:cb);
 * native Handle json_unpack(const char[] sFormat, ...);
 *
 */






/**
 * Do not edit below this line!
 */
public Extension __ext_smjansson =
{
	name = "SMJansson",
	file = "smjansson.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};

#if !defined REQUIRE_EXTENSIONS
public void __ext_smjansson_SetNTVOptional()
{
	MarkNativeAsOptional("json_typeof");
	MarkNativeAsOptional("json_equal");

	MarkNativeAsOptional("json_copy");
	MarkNativeAsOptional("json_deep_copy");

	MarkNativeAsOptional("json_object");
	MarkNativeAsOptional("json_object_size");
	MarkNativeAsOptional("json_object_get");
	MarkNativeAsOptional("json_object_set");
	MarkNativeAsOptional("json_object_set_new");
	MarkNativeAsOptional("json_object_del");
	MarkNativeAsOptional("json_object_clear");
	MarkNativeAsOptional("json_object_update");
	MarkNativeAsOptional("json_object_update_existing");
	MarkNativeAsOptional("json_object_update_missing");
	MarkNativeAsOptional("json_object_update_new");
	MarkNativeAsOptional("json_object_update_existing_new");
	MarkNativeAsOptional("json_object_update_missing_new");
	MarkNativeAsOptional("json_object_update_recursive");

	MarkNativeAsOptional("json_object_iter");
	MarkNativeAsOptional("json_object_iter_at");
	MarkNativeAsOptional("json_object_iter_next");
	MarkNativeAsOptional("json_object_iter_key");
	MarkNativeAsOptional("json_object_iter_value");
	MarkNativeAsOptional("json_object_iter_set");
	MarkNativeAsOptional("json_object_iter_set_new");

	MarkNativeAsOptional("json_array");
	MarkNativeAsOptional("json_array_size");
	MarkNativeAsOptional("json_array_get");
	MarkNativeAsOptional("json_array_set");
	MarkNativeAsOptional("json_array_set_new");
	MarkNativeAsOptional("json_array_append");
	MarkNativeAsOptional("json_array_append_new");
	MarkNativeAsOptional("json_array_insert");
	MarkNativeAsOptional("json_array_insert_new");
	MarkNativeAsOptional("json_array_remove");
	MarkNativeAsOptional("json_array_clear");
	MarkNativeAsOptional("json_array_extend");

	MarkNativeAsOptional("json_string");
	MarkNativeAsOptional("json_string_value");
	MarkNativeAsOptional("json_string_set");

	MarkNativeAsOptional("json_integer");
	MarkNativeAsOptional("json_integer_value");
	MarkNativeAsOptional("json_integer_set");

	MarkNativeAsOptional("json_real");
	MarkNativeAsOptional("json_real_value");
	MarkNativeAsOptional("json_real_set");
	MarkNativeAsOptional("json_number_value");

	MarkNativeAsOptional("json_boolean");
	MarkNativeAsOptional("json_true");
	MarkNativeAsOptional("json_false");
	MarkNativeAsOptional("json_null");	

	MarkNativeAsOptional("json_load");
	MarkNativeAsOptional("json_load_ex");
	MarkNativeAsOptional("json_load_file");
	MarkNativeAsOptional("json_load_file_ex");

	MarkNativeAsOptional("json_dump");
	MarkNativeAsOptional("json_dump_file");
	MarkNativeAsOptional("json_dump_ex");
	MarkNativeAsOptional("json_dump_file_ex");
}
#endif
