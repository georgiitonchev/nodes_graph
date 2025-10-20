#pragma once

#if defined(WIN32)
#include <windows.h>
#include <objbase.h>
#elif defined(__APPLE__) || defined(__linux__)
#include <uuid/uuid.h>
#endif

#include <iostream>
#include <string>

class Guid {
public:
#if defined(WIN32)
	static std::string CreateGuid() {
		GUID guid;
		HRESULT result = CoCreateGuid(&guid);

		OLECHAR* guidString;
		StringFromCLSID(guid, &guidString);

		auto wstr = std::wstring(guidString);
		int wstrSize = WideCharToMultiByte(CP_UTF8, 0, wstr.data(),
			(int)wstr.size(), nullptr, 0, nullptr, nullptr);

		std::string str(wstrSize, 0);
		WideCharToMultiByte(CP_UTF8, 0, wstr.data(),
			(int)wstr.size(), str.data(), wstrSize, nullptr, nullptr);

		CoTaskMemFree(guidString);

		return str;
	}
#elif defined(__APPLE__) || defined(__linux__)
	static std::string CreateGuid() {

		uuid_t uuid;
		uuid_generate(uuid);

		char uuid_str[37];
		uuid_unparse(uuid, uuid_str);

		return std::string(uuid_str);
	}
#endif
};
