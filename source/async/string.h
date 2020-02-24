#pragma once

#include <string>
#include <string_view>

namespace dvr {
	class String {
	private:
		std::string data;
	public:
		String();
		String(const std::string& s);
		String(std::string&& s);
	};

	class StringPtr {
	private:
		std::string_view data;
	public:
		StringPtr(String& s);
	};
}
