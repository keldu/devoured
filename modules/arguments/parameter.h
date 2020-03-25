#pragma once

#include <string>
#include <optional>

#include "devoured/devoured.h"

namespace dvr {
	struct Parameter{
	public:
		//CONFIG VALUES
		Devoured::Mode mode;

		std::string path;

		bool interactive = false;
		bool status = false;
		bool devour = false;
		bool spawn = false;
		
		std::optional<std::string> alias = std::nullopt;
		std::optional<std::string> command = std::nullopt;

		std::optional<std::string> target = std::nullopt;
		std::optional<std::string> control = std::nullopt;
	};

	const Parameter parseParams(int argc, char** argv);

}
