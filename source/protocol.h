#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

namespace dvr {
	/* 
	 * Don't change the Message types with out changing the corresponding max size in network.cpp
	 * I'm wondering if there exists some kind of template magic to autogenerate the sizes
	 * with some helper classes
	 */
	class MessageRequest {
	public:
		MessageRequest() = default;
		MessageRequest(uint16_t, uint8_t, const std::string&, const std::string&);
		uint16_t request_id;
		uint8_t type;
		std::string target;
		std::string content;
	};
	/*
	 * Same here as above
	 */
	class MessageResponse {
	public:
		MessageResponse() = default;
		MessageResponse(uint16_t, uint8_t, const std::string&, const std::string&);
		uint16_t request_id;
		uint8_t return_code;
		std::string target;
		std::string content;
	};

	bool serializeMessageRequest(std::vector<uint8_t>& buffer, MessageRequest& request);
	bool deserializeMessageRequest(std::vector<uint8_t>& buffer, MessageRequest& request);

	bool serializeMessageResponse(std::vector<uint8_t>& buffer, MessageResponse& response);
	bool deserializeMessageResponse(std::vector<uint8_t>& buffer, MessageResponse& response);
}
