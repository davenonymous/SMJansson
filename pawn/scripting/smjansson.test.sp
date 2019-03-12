#pragma semicolon 1
#include <sourcemod>
#include <smjansson>
#include <test>

#define VERSION 		"0.0.2"


public Plugin myinfo = {
	name 		= "SMJansson, Test, Create JSON",
	author 		= "Thrawn",
	description = "",
	version 	= VERSION,
};

public void OnPluginStart() {
	CreateConVar("sm_smjansson_test_version", VERSION, "Tests all SMJansson natives.", FCVAR_SPONLY|FCVAR_REPLICATED|FCVAR_NOTIFY|FCVAR_DONTRECORD);

	bool bStepSuccess = false;

	StringMap hTest = Test_New(103);
	Test_Ok(hTest, LibraryExists("jansson"), "Library is loaded");

	Handle hObj = json_object();
	Test_IsNot(hTest, hObj, INVALID_HANDLE, "Creating JSON Object");
	Test_Ok(hTest, json_is_object(hObj), "Type is Object");

	Handle hString = json_string("value");
	Test_IsNot(hTest, hString, INVALID_HANDLE, "Creating JSON String");
	Test_Ok(hTest, json_is_string(hString), "Type is String");

	char sString[32];
	json_string_value(hString, sString, sizeof(sString));
	Test_Is_String(hTest, sString, "value", "Checking created JSON String value");

	Test_Ok(hTest, json_string_set(hString, "The answer is 42"), "Modifying string value");
	json_string_value(hString, sString, sizeof(sString));
	Test_Is_String(hTest, sString, "The answer is 42", "Checking modified JSON String value");

	bStepSuccess = json_object_set(hObj, "__String", hString);
	Test_Ok(hTest, bStepSuccess, "Attaching modified String to root object");
	delete hString;


	float fNumberValue;
	float fReal;

	Handle hReal = json_real(1.23456789);
	Test_IsNot(hTest, hReal, INVALID_HANDLE, "Creating JSON Real");
	Test_Is(hTest, json_typeof(hReal), view_as<int>(JSON_REAL), "Type is Real");

	fReal = json_real_value(hReal);
	Test_Is_Float(hTest, fReal, 1.23456789, "Checking created JSON Real value");
	fNumberValue = json_number_value(hReal);
	Test_Is_Float(hTest, fReal, 1.23456789, "Checking result of json_number_value");

	Test_Ok(hTest, json_real_set(hReal, -444.556), "Modifying real value");
	fReal = json_real_value(hReal);
	Test_Is_Float(hTest, fReal, -444.556, "Checking modified JSON Real value");

	fNumberValue = json_number_value(hReal);
	Test_Is_Float(hTest, fReal, -444.556, "Checking result of json_number_value");

	bStepSuccess = json_object_set(hObj, "__Float", hReal);
	Test_Ok(hTest, bStepSuccess, "Attaching modified Real to root object");
	delete hReal;



	int iInteger;
	Handle hInteger = json_integer(42);
	Test_IsNot(hTest, hInteger, INVALID_HANDLE, "Creating JSON Integer");
	Test_Is(hTest, json_typeof(hInteger), view_as<int>(JSON_INTEGER), "Type is Integer");

	iInteger = json_integer_value(hInteger);
	Test_Is(hTest, iInteger, 42, "Checking created JSON Integer value");
	fNumberValue = json_number_value(hInteger);
	Test_Is_Float(hTest, fNumberValue, 42.0, "Checking result of json_number_value");

	Test_Ok(hTest, json_integer_set(hInteger, 1337), "Modifying integer value");
	iInteger = json_integer_value(hInteger);
	Test_Is(hTest, iInteger, 1337, "Checking modified JSON Integer value");

	fNumberValue = json_number_value(hInteger);
	Test_Is_Float(hTest, fNumberValue, 1337.0, "Checking result of json_number_value");

	bStepSuccess = json_object_set(hObj, "__Integer", hInteger);
	Test_Ok(hTest, bStepSuccess, "Attaching modified Integer to root object");

	Test_Is(hTest, json_object_size(hObj), 3, "Object has the correct size");
	delete hInteger;



	char sShouldBe[128] = "{\"__Float\": -444.55599975585938, \"__String\": \"The answer is 42\", \"__Integer\": 1337}";

	char sJSON[4096];
	json_dump(hObj, sJSON, sizeof(sJSON), 0);

	Test_Is_String(hTest, sJSON, sShouldBe, "Created JSON is ok");




	Handle hObjNested = json_object();
	bStepSuccess = json_object_set(hObj, "__NestedObject", hObjNested);
	Test_Ok(hTest, bStepSuccess, "Attaching new Object to root object");
	Test_Is(hTest, json_object_size(hObj), 4, "Object has the correct size");

	bStepSuccess = json_object_set_new(hObjNested, "__NestedString", json_string("i am nested"));
	Test_Ok(hTest, bStepSuccess, "Attaching new String to nested object (using reference stealing)");

	Test_Ok(hTest, json_object_del(hObj, "__Float"), "Deleting __Float element from root object");
	Test_Is(hTest, json_object_size(hObj), 3, "Object has the correct size");




	Handle hArray = json_array();
	Test_IsNot(hTest, hArray, INVALID_HANDLE, "Creating JSON Array");
	Test_Is(hTest, json_typeof(hArray), view_as<int>(JSON_ARRAY), "Type is Array");

	Handle hFirst_String = json_string("1");
	Handle hSecond_Float = json_real(2.0);
	Handle hThird_Integer = json_integer(3);
	Handle hFourth_String = json_string("4");
	Test_Ok(hTest, json_array_append(hArray, hFirst_String), "Appending String to Array");
	delete hFirst_String;
	Test_Is(hTest, json_array_size(hArray), 1, "Array has correct size");
	Test_Ok(hTest, json_array_append(hArray, hSecond_Float), "Appending float to Array");
	delete hSecond_Float;
	Test_Is(hTest, json_array_size(hArray), 2, "Array has correct size");
	Test_Ok(hTest, json_array_insert(hArray, 0, hThird_Integer), "Inserting Integer at position 0");
	delete hThird_Integer;
	Test_Is(hTest, json_array_size(hArray), 3, "Array has correct size");
	Test_Ok(hTest, json_array_set(hArray, 1, hFourth_String), "Setting String at position 1");
	delete hFourth_String;
	Test_Is(hTest, json_array_size(hArray), 3, "Array has correct size");

	bStepSuccess = json_object_set(hObjNested, "__Array", hArray);
	Test_Ok(hTest, bStepSuccess, "Attaching Array to nested object");
	delete hObjNested;


	PrintToServer("      - Creating the same Array using reference stealing");
	Handle hArrayStealing = json_array();
	Test_Ok(hTest, json_array_append_new(hArrayStealing, json_string("1")), "Appending new String to Array");
	Test_Ok(hTest, json_array_append_new(hArrayStealing, json_real(2.0)), "Appending new float to Array");
	Test_Ok(hTest, json_array_insert_new(hArrayStealing, 0, json_integer(3)), "Inserting new Integer at position 0");
	Test_Ok(hTest, json_array_set_new(hArrayStealing, 1, json_string("4")), "Setting new String at position 1");

	Test_Ok(hTest, json_equal(hArray, hArrayStealing), "Arrays are equal.");
	delete hArrayStealing;
	delete hArray;


	DeleteFile("testoutput.json");
	bStepSuccess = json_dump_file(hObj, "testoutput.json", 2);
	Test_Ok(hTest, bStepSuccess, "File written without errors");
	Test_Ok(hTest, FileExists("testoutput.json"), "Testoutput file exists");


	// Reload the written file
	Handle hReloaded = json_load_file("testoutput.json");
	Test_IsNot(hTest, hReloaded, INVALID_HANDLE, "Loading JSON from file.");

	Test_Ok(hTest, json_equal(hReloaded, hObj), "Written file and data in memory are equal");

	// Iterate over the reloaded file
	Handle hIterator = json_object_iter(hReloaded);
	Test_IsNot(hTest, hIterator, INVALID_HANDLE, "Creating an iterator for the reloaded object.");

	// Expecting three values in random order
	// Use a trie to look em up and delete them from it -> afterwards size must be 0
	StringMap hTestTrie = new StringMap();
	hTestTrie.SetValue("__String", view_as<int>(JSON_STRING));
	hTestTrie.SetValue("__Integer", view_as<int>(JSON_INTEGER));
	hTestTrie.SetValue("__NestedObject", view_as<int>(JSON_OBJECT));

	while (hIterator != null) {
		char sKey[128];
		json_object_iter_key(hIterator, sKey, sizeof(sKey));

		Handle hValue = json_object_iter_value(hIterator);
		json_type xType = json_typeof(hValue);

		char sType[32];
		Stringify_json_type(xType, sType, sizeof(sType));

		json_type xShouldBeType;
		hTestTrie.GetValue(sKey, xShouldBeType);

		PrintToServer("      - Found key: %s (Type: %s)", sKey, sType);
		Test_Is(hTest, xShouldBeType, view_as<int>(xType), "Type is correct");
		hTestTrie.Remove(sKey);

		if (xType == JSON_INTEGER) {
			Handle hIntegerOverwrite = json_integer(9001);
			Test_Ok(hTest, json_object_iter_set(hReloaded, hIterator, hIntegerOverwrite), "Overwriting integer value at iterator position");
			delete hIntegerOverwrite;
		}

		if (xType == JSON_STRING) {
			Test_Ok(hTest, json_object_iter_set_new(hReloaded, hIterator, json_string("What is the \"Hitchhiker's guide to the galaxy\"?")), "Overwriting string value at iterator position (using reference stealing)");
		}

		if (xType == JSON_OBJECT) {
			StringMap hTestTrieForArray = new StringMap();
			hTestTrieForArray.SetValue("String", 0);
			hTestTrieForArray.SetValue("Integer", 0);
			hTestTrieForArray.SetValue("Real", 0);


			Handle hReloadedArray = json_object_get(hValue, "__Array");
			Test_IsNot(hTest, hReloadedArray, INVALID_HANDLE, "Getting JSON Array from reloaded object");
			Test_Is(hTest, json_array_size(hReloadedArray), 3, "Array has correct size");

			for (int iElement = 0; iElement < json_array_size(hReloadedArray); iElement++) {
				Handle hElement = json_array_get(hReloadedArray, iElement);
				char sArrayType[32];
				Stringify_json_type(json_typeof(hElement), sArrayType, sizeof(sArrayType));

				PrintToServer("      - Found element with type: %s", sArrayType);
				hTestTrieForArray.Remove(sArrayType);
				delete hElement;
			}

			Test_Is(hTest, hTestTrieForArray.Size, 0, "Looped over all array elements");
			delete hTestTrieForArray;

			Test_Ok(hTest, json_array_remove(hReloadedArray, 2), "Deleting 3rd element from array");
			Test_Is(hTest, json_array_size(hReloadedArray), 2, "Array has correct size");

			Handle hArrayForExtending = json_array();
			Handle hStringForExtension = json_string("Extension 1");
			Handle hStringForExtension2 = json_string("Extension 2");
			json_array_append(hArrayForExtending, hStringForExtension);
			json_array_append(hArrayForExtending, hStringForExtension2);
			delete hStringForExtension;
			delete hStringForExtension2;

			Test_Ok(hTest, json_array_extend(hReloadedArray, hArrayForExtending), "Extending array");
			Test_Is(hTest, json_array_size(hReloadedArray), 4, "Array has correct size");

			Test_Ok(hTest, json_array_clear(hArrayForExtending), "Clearing array");
			Test_Is(hTest, json_array_size(hArrayForExtending), 0, "Array is empty");
			delete hArrayForExtending;
			delete hReloadedArray;
		}

		delete hValue;
		hIterator = json_object_iter_next(hReloaded, hIterator);
	}
	Test_Is(hTest, hTestTrie.Size, 0, "Iterator looped over all keys");
	delete hTestTrie;
	Test_OkNot(hTest, json_equal(hReloaded, hObj), "Written file and data in memory are not equal anymore");

	PrintToServer("      - Creating the same object using json_pack");
	ArrayList hParams = new ArrayList(64);
	hParams.PushString("__String");
	hParams.PushString("What is the \"Hitchhiker's guide to the galaxy\"?");
	hParams.PushString("__Integer");
	hParams.Push(9001);
	hParams.PushString("__NestedObject");
	hParams.PushString("__NestedString");
	hParams.PushString("i am nested");
	hParams.PushString("__Array");
	hParams.Push(3);
	hParams.PushString("4");
	hParams.PushString("Extension 1");
	hParams.PushString("Extension 2");
	Handle hPacked = json_pack("{ss, s:is{sss:[isss]}}", hParams);
	delete hParams;
	Test_Ok(hTest, json_equal(hReloaded, hPacked), "Packed JSON is equal to manually created JSON");
	delete hPacked;

	PrintToServer("      - Testing all json_pack values");
	ArrayList hParamsAll = new ArrayList(64);
	hParamsAll.PushString("String");
	hParamsAll.Push(42);
	hParamsAll.Push(13.37);
	hParamsAll.Push(20001.333);
	hParamsAll.Push(true);
	hParamsAll.Push(false);
	Handle hPackAll = json_pack("[sifrbnb]", hParamsAll);
	Test_Ok(hTest, json_is_array(hPackAll), "Packed JSON is an array");

	char sElementOne[32];
	json_array_get_string(hPackAll, 0, sElementOne, sizeof(sElementOne));
	Test_Is_String(hTest, sElementOne, "String", "Element 1 has the correct string value");
	Test_Is(hTest, json_array_get_int(hPackAll, 1), 42, "Element 2 has the correct integer value");
	Test_Is(hTest, json_array_get_float(hPackAll, 2), 13.37, "Element 3 has the correct float value");
	Test_Is(hTest, json_array_get_float(hPackAll, 3), 20001.333, "Element 4 has the correct float value");
	Test_Is(hTest, json_array_get_bool(hPackAll, 4), true, "Element 5 is boolean true.");

	Handle hElementFive = json_array_get(hPackAll, 5);
	Test_Is(hTest, json_typeof(hElementFive), JSON_NULL, "Element 6 is null.");
	delete hElementFive;

	Test_Is(hTest, json_array_get_bool(hPackAll, 6), false, "Element 7 is boolean false.");

	delete hParamsAll;


	PrintToServer("      - Creating new object with 4 keys via load");
	Handle hObjManipulation = json_load("{\"A\":1,\"B\":2,\"C\":3,\"D\":4}");
	Test_Ok(hTest, json_object_del(hObjManipulation, "D"), "Deleting element from object");
	Test_Is(hTest, json_object_size(hObjManipulation), 3, "Object size is correct");


	PrintToServer("      - Creating new object to update the previous one");
	Handle hObjUpdate = json_load("{\"A\":10,\"B\":20,\"C\":30,\"D\":40,\"E\":50,\"F\":60,\"G\":70}");
	Test_Ok(hTest, json_object_update_existing(hObjManipulation, hObjUpdate), "Updating existing keys");

	Handle hReadC = json_object_get(hObjManipulation, "C");
	Test_Is(hTest, json_integer_value(hReadC), 30, "Element update successful");
	Test_Is(hTest, json_object_size(hObjManipulation), 3, "Object size is correct");
	delete hReadC;

	Test_Ok(hTest, json_object_update_missing(hObjManipulation, hObjUpdate), "Updating missing keys");
	Handle hReadF = json_object_get(hObjManipulation, "F");
	Test_Is(hTest, json_integer_value(hReadF), 60, "Element insertion via update successful");
	Test_Is(hTest, json_object_size(hObjManipulation), 7, "Object size is correct");
	delete hReadF;

	Test_Ok(hTest, json_object_clear(hObjManipulation), "Clearing new object");
	Test_Is(hTest, json_object_size(hObjManipulation), 0, "Object is empty");

	PrintToServer("      - Adding one of the original four keys");
	Handle hBNew = json_integer(2);
	json_object_set(hObjManipulation, "B", hBNew);
	Test_Ok(hTest, json_object_update(hObjManipulation, hObjUpdate), "Updating all keys");
	delete hBNew;
	delete hObjUpdate;

	Handle hReadB = json_object_get(hObjManipulation, "B");
	Test_Is(hTest, json_integer_value(hReadB), 20, "Element update successful");
	delete hReadB;

	Test_Is(hTest, json_object_size(hObjManipulation), 7, "Object size is correct");

	PrintToServer("      - Creating and adding an array to the object");
	Handle hCopyArray = json_array();
	Handle hNoMoreVariableNames = json_string("no more!");
	Handle hEvenLessVariableNames = json_string("less n less!");
	json_array_append(hCopyArray, hNoMoreVariableNames);
	json_array_append(hCopyArray, hEvenLessVariableNames);
	json_object_set(hObjManipulation, "Array", hCopyArray);


	Handle hCopy = json_copy(hObjManipulation);
	Test_IsNot(hTest, hCopy, INVALID_HANDLE, "Creating copy of JSON Object");
	Test_Is(hTest, json_object_size(hCopy), 8, "Object size is correct");
	Test_Ok(hTest, json_equal(hCopy, hObjManipulation), "Objects are equal");

	PrintToServer("      - Modifying the array of the original Object");
	Handle hEmptyVariableNames = json_string("empty!");
	json_array_append(hCopyArray, hEmptyVariableNames);

	Test_Ok(hTest, json_equal(hCopy, hObjManipulation), "Content of copy is still identical (was a shallow copy)");


	Handle hDeepCopy = json_deep_copy(hObjManipulation);
	Test_IsNot(hTest, hDeepCopy, INVALID_HANDLE, "Creating deep copy of JSON Object");
	Test_Is(hTest, json_object_size(hDeepCopy), 8, "Object size is correct");
	Test_Ok(hTest, json_equal(hDeepCopy, hObjManipulation), "Objects are equal");

	PrintToServer("      - Modifying the array of the original Object");
	Handle hDeadVariableNames = json_string("dead!");
	json_array_append(hCopyArray, hDeadVariableNames);
	delete hCopyArray;

	Test_OkNot(hTest, json_equal(hDeepCopy, hObjManipulation), "Content of copy is not identical anymore (was a deep copy)");


	Handle hBooleanObject = json_object();
	json_object_set_new(hBooleanObject, "true1", json_true());
	json_object_set_new(hBooleanObject, "false1", json_false());
	json_object_set_new(hBooleanObject, "true2", json_boolean(true));
	json_object_set_new(hBooleanObject, "false2", json_boolean(false));
	json_object_set_new(hBooleanObject, "null", json_null());

	char sBooleanObjectDump[4096];
	json_dump(hBooleanObject, sBooleanObjectDump, sizeof(sBooleanObjectDump), 0);

	char sBooleanShouldBe[4096] = "{\"false2\": false, \"true1\": true, \"false1\": false, \"true2\": true, \"null\": null}";
	Test_Is_String(hTest, sBooleanObjectDump, sBooleanShouldBe, "Created JSON matches");

	delete hBooleanObject;


	char sErrorMsg[255];
	int iLine = -1;
	int iColumn = -1;
	Handle hNoObj = json_load_ex("{\"apple\": 3 \"banana\": 0, \"error\": \"missing a comma\"}", sErrorMsg, sizeof(sErrorMsg), iLine, iColumn);

	if (hNoObj == null) {
		Test_Is(hTest, iLine, 1, "Correct error line");
		Test_Is(hTest, iColumn, 20, "Correct error column");
		Test_Ok(hTest, strlen(sErrorMsg) > 0, "Error message is not empty");
	}
	else {
		delete hNoObj;
	}



	json_dump(hObj, sJSON, sizeof(sJSON));
	PrintToServer("\nJSON 1:\n-------------\n%s\n-------------\n", sJSON);


	char sJSONReloaded[4096];
	json_dump(hReloaded, sJSONReloaded, sizeof(sJSONReloaded), 2);
	PrintToServer("JSON 2:\n-------------\n%s\n-------------\n", sJSONReloaded);

	char sJSONManipulated[4096];
	json_dump(hObjManipulation, sJSONManipulated, sizeof(sJSONManipulated), 0);
	PrintToServer("JSON 3:\n-------------\n%s\n-------------\n", sJSONManipulated);


	PrintToServer("JSON 4:\n-------------\n%s\n-------------\n", sBooleanObjectDump);

	char sJSONPackAll[4096];
	json_dump(hPackAll, sJSONPackAll, sizeof(sJSONPackAll), 4);

	PrintToServer("JSON 5:\n-------------\n%s\n-------------\n", sJSONPackAll);

	delete hPackAll;
	delete hObj;
	delete hObjManipulation;
	delete hReloaded;

	// Finish testing
	Test_End(hTest);
	delete hTest;
}