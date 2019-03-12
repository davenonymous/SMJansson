stock bool Test_SetFilename(StringMap hTest, const char[] value) {
	return hTest.SetString("filename", value);
}

stock bool Test_GetFilename(StringMap hTest, char[] value, int max_size) {
	return hTest.GetString("filename", value, max_size);
}

stock bool Test_SetFails(StringMap hTest, int value) {
	return hTest.SetValue("fails", value);
}

stock bool Test_GetFails(StringMap hTest, int &value) {
	return hTest.GetValue("fails", value);
}

stock bool Test_SetTests(StringMap hTest, int value) {
	return hTest.SetValue("tests", value);
}

stock bool Test_GetTests(StringMap hTest, int &value) {
	return hTest.GetValue("tests", value);
}

stock bool Test_SetFile(StringMap hTest, Handle value) {
	return hTest.SetValue("file", value);
}

stock bool Test_GetFile(StringMap hTest, Handle &value) {
	return hTest.GetValue("file", value);
}

stock bool Test_SetProcessed(StringMap hTest, int value) {
	return hTest.SetValue("processed", value);
}

stock bool Test_GetProcessed(StringMap hTest, int &value) {
	return hTest.GetValue("processed", value);
}

stock void Test_Inherit(StringMap hObj, int count) {
	int tests;
	Test_GetTests(hObj, tests);
	File file;
	Test_GetFile(hObj, file);

	tests = count;
	file = null;

	Test_SetTests(hObj, tests);
	Test_SetFile(hObj, file);

}

stock StringMap Test_New(int count) {
	StringMap hObj = new StringMap();
	Test_Inherit(hObj, count);
	return hObj;
}

stock void Test_Deherit(StringMap hObj) {

}

stock void Test_Destroy(StringMap hObj) {
	Test_Deherit(hObj);

	delete hObj;
}

stock void Test_Pass(StringMap hTest, const char[] sTest) {
	int processed;
	Test_GetProcessed(hTest, processed);

	Test_LogThis(hTest, "ok %i - %s", processed, sTest);
	processed++;


	Test_SetProcessed(hTest, processed);

}

stock void Test_Fail(StringMap hTest, const char[] sTest) {
	int fails;
	Test_GetFails(hTest, fails);
	int processed;
	Test_GetProcessed(hTest, processed);

	Test_LogThis(hTest, "not ok %i - %s", processed, sTest);
	Test_LogThis(hTest, "#   Failed test '%s'", sTest);
	fails++;
	processed++;

	Test_SetFails(hTest, fails);
	Test_SetProcessed(hTest, processed);

}

stock void Test_Is_String(StringMap hTest, const char[] got, const char[] expected, const char[] sTest) {
	int fails;
	Test_GetFails(hTest, fails);
	int processed;
	Test_GetProcessed(hTest, processed);

	if (StrEqual(got, expected)) {
		Test_LogThis(hTest, "ok %i - %s", processed, sTest);
	}
	else {
		Test_LogThis(hTest, "not ok %i - %s", processed, sTest);
		Test_LogThis(hTest, "#   Failed test '%s'", sTest);
		Test_LogThis(hTest, "#          got: '%s'", got);
		Test_LogThis(hTest, "#     expected: '%s'", expected);
		fails++;
	}

	processed++;

	Test_SetFails(hTest, fails);
	Test_SetProcessed(hTest, processed);

}

stock void Test_Is(StringMap hTest, any got, any expected, const char[] sTest) {
	int fails;
	Test_GetFails(hTest, fails);
	int processed;
	Test_GetProcessed(hTest, processed);

	if (got == expected) {
		Test_LogThis(hTest, "ok %i - %s", processed, sTest);
	}
	else {
		Test_LogThis(hTest, "not ok %i - %s", processed, sTest);
		Test_LogThis(hTest, "#   Failed test '%s'", sTest);
		Test_LogThis(hTest, "#          got: '%i'", got);
		Test_LogThis(hTest, "#     expected: '%i'", expected);
		fails++;
	}

	processed++;

	Test_SetFails(hTest, fails);
	Test_SetProcessed(hTest, processed);
}

stock void Test_IsNot(StringMap hTest, any got, any expected, const char[] sTest) {
	int fails;
	Test_GetFails(hTest, fails);
	int processed;
	Test_GetProcessed(hTest, processed);

	if (got != expected) {
		Test_LogThis(hTest, "ok %i - %s", processed, sTest);
	}
	else {
		Test_LogThis(hTest, "not ok %i - %s", processed, sTest);
		Test_LogThis(hTest, "#   Failed test '%s'", sTest);
		Test_LogThis(hTest, "#          got: '%i'", got);
		Test_LogThis(hTest, "#     expected: '%i'", expected);
		fails++;
	}

	processed++;

	Test_SetFails(hTest, fails);
	Test_SetProcessed(hTest, processed);
}

stock void Test_End(StringMap hTest) {
	int tests;
	Test_GetTests(hTest, tests);
	int fails;
	Test_GetFails(hTest, fails);
	int processed;
	Test_GetProcessed(hTest, processed);
	File file;
	Test_GetFile(hTest, file);

	if (tests > processed) {
		Test_LogThis(hTest, "# Looks like you planned to run %i tests, but only ran %i.", tests, processed);
	}

	if (tests < processed) {
		Test_LogThis(hTest, "# Looks like you ran %i more tests than you originally planned.", processed - tests);
	}

	Test_LogThis(hTest, "# You ran %i of %i tests, %i failed, %i passed", processed, tests, fails, processed - fails);
	if (file != null) {
		delete file;
	}


}

stock void Test_Ok(StringMap hTest, bool bOk, const char[] sTest) {
	int fails;
	Test_GetFails(hTest, fails);
	int processed;
	Test_GetProcessed(hTest, processed);

	if (bOk) {
		Test_LogThis(hTest, "ok %i - %s", processed, sTest);
	}
	else {
		Test_LogThis(hTest, "not ok %i - %s", processed, sTest);
		Test_LogThis(hTest, "#   Failed test '%s'", sTest);
		fails++;
	}
	processed++;

	Test_SetFails(hTest, fails);
	Test_SetProcessed(hTest, processed);
}

stock void Test_OkNot(StringMap hTest, bool bOkNot, const char[] sTest) {
	int fails;
	Test_GetFails(hTest, fails);
	int processed;
	Test_GetProcessed(hTest, processed);

	if (!bOkNot) {
		Test_LogThis(hTest, "ok %i - %s", processed, sTest);
	}
	else {
		Test_LogThis(hTest, "not ok %i - %s", processed, sTest);
		Test_LogThis(hTest, "#   Failed test '%s'", sTest);
		fails++;
	}
	processed++;

	Test_SetFails(hTest, fails);
	Test_SetProcessed(hTest, processed);
}

stock void Test_Is_Float(StringMap hTest, float got, float expected, const char[] sTest) {
	int fails;
	Test_GetFails(hTest, fails);
	int processed;
	Test_GetProcessed(hTest, processed);

	if (got == expected) {
		Test_LogThis(hTest, "ok %i - %s", processed, sTest);
	}
	else {
		Test_LogThis(hTest, "not ok %i - %s", processed, sTest);
		Test_LogThis(hTest, "#   Failed test '%s'", sTest);
		Test_LogThis(hTest, "#          got: '%f'", got);
		Test_LogThis(hTest, "#     expected: '%f'", expected);
		fails++;
	}

	processed++;


	Test_SetFails(hTest, fails);
	Test_SetProcessed(hTest, processed);

}

stock void Test_Like(StringMap hTest, const char[] got, const char[] pattern, const char[] sTest) {
	int fails;
	Test_GetFails(hTest, fails);
	int processed;
	Test_GetProcessed(hTest, processed);

	Regex hRegEx = new Regex(pattern);
	if (hRegEx.Match(got) > 0) {
		Test_LogThis(hTest, "ok %i - %s", processed, sTest);
	}
	else {
		Test_LogThis(hTest, "not ok %i - %s", processed, sTest);
		Test_LogThis(hTest, "#   Failed test '%s'", sTest);
		Test_LogThis(hTest, "#          got: '%s'", got);
		Test_LogThis(hTest, "#      pattern: '%s'", pattern);
		fails++;
	}

	delete hRegEx;

	processed++;

	Test_SetFails(hTest, fails);
	Test_SetProcessed(hTest, processed);

}

stock void Test_LogThis(StringMap hTest, const char[] format, any ...) {
	char filename[PLATFORM_MAX_PATH];
	Test_GetFilename(hTest, filename, PLATFORM_MAX_PATH);
	File file;
	Test_GetFile(hTest, file);

	char sMessage[255];
	VFormat(sMessage, sizeof(sMessage), format, 3);

	if (StrEqual(filename, "")) {
		PrintToServer(sMessage);
	}
	else {
		if (file == null) {
			file = OpenFile(filename, "a");
		}

		file.WriteLine(sMessage);
	}

	Test_SetFile(hTest, file);

}