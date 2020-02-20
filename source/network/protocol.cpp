#include "protocol.h"

#include "async.h"

#include <iostream>

// 2 is the message length size
const size_t message_length_size = 2;
const uint16_t max_message_size = 4096 - message_length_size;
const uint16_t max_target_size = 255;
/*
 * So This is the rest of the max size. We have 
 * 3 uint16_t values and
 * 2 uint8_t values
 * Subtrace all these and the other max sizes and you get the resulting protocol
 * This should be guaranteed to be > 0.
 * Check with a calculator if you change this or sth similar :)
 */

const uint16_t request_static_size = 7;
const uint16_t max_request_content_size = max_message_size - max_target_size - request_static_size;
// Is by coincidence the same
const uint16_t response_static_size = 7;
const uint16_t max_response_content_size = max_message_size - max_target_size - response_static_size;

namespace dvr {
	MessageRequest::MessageRequest(uint16_t rid_p, uint8_t type_p, const std::string& target_p, const std::string& content_p):
		request_id{rid_p},
		type{type_p},
		target{target_p},
		content{content_p}
	{}

	MessageResponse::MessageResponse(uint16_t rid_p, uint8_t rc_p, const std::string& target_p, const std::string& content_p):
		request_id{rid_p},
		return_code{rc_p},
		target{target_p},
		content{content_p}
	{}

	std::ostream& operator<<(std::ostream& stream, const MessageRequest& request){
		stream<<"Request ID: "<<std::to_string(request.request_id)<<"\n";
		stream<<"Type: "<<std::to_string(request.type)<<"\n";
		stream<<"Target: "<<request.target<<"\n";
		stream<<"Content: "<<request.content<<std::endl;
		return stream;
	}

	std::ostream& operator<<(std::ostream& stream, const MessageResponse& response){
		stream<<"Request ID: "<<std::to_string(response.request_id)<<"\n";
		stream<<"ReturnCode: "<<std::to_string(response.return_code)<<"\n";
		stream<<"Target: "<<response.target<<"\n";
		stream<<"Content: "<<response.content<<std::endl;
		return stream;
	}
	// error checks are already done on a higher level set of functions
	size_t deserialize(uint8_t* buffer, uint16_t& value){
		uint16_t& buffer_value = *reinterpret_cast<uint16_t*>(buffer);
		value = le16toh(buffer_value);
		return 2;
	}
	
	size_t deserialize(uint8_t* buffer, std::string& value){
		uint16_t val_size = 0;
		size_t shift = deserialize(buffer, val_size);
		value.resize(val_size);
		for(size_t i = 0; i < val_size; ++i){
			value[i] = buffer[shift+i];
		}
		return shift + val_size;
	}
	
	size_t serialize(uint8_t* buffer, uint16_t value){
		uint16_t& val_buffer = *reinterpret_cast<uint16_t*>(buffer);
		val_buffer = htole16(value);
		return 2;
	}

	size_t serialize(uint8_t* buffer, const std::string& value){
		size_t shift = serialize(buffer, static_cast<uint16_t>(value.size()));
		for(size_t i = 0; i < value.size(); ++i){
			buffer[shift + i] = static_cast<uint8_t>(value[i]);
		}
		return shift + value.size();
	}

	/*
	std::optional<MessageRequest> asyncReadRequest(AsyncIoStream& connection){
		auto opt_buffer = connection.read(message_length_size);
		if(!opt_buffer.has_value()){
			return std::nullopt;
		}
		
		uint16_t msg_size;
		size_t shift = deserialize(*opt_buffer, msg_size);
		if(msg_size >= max_message_size){
			return std::nullopt;
		}

		opt_buffer = connection.read(message_length_size+msg_size);
		if(!opt_buffer.has_value()){
			return std::nullopt;
		}
		MessageRequest msg;
		uint8_t* buffer = *opt_buffer;
		shift += deserialize(&(buffer)[shift], msg.request_id);
		msg.type = buffer[shift++];
		shift += deserialize(&buffer[shift], msg.target);
		if( msg.target.size() >= max_target_size ){
			return std::nullopt;
		}

		shift += deserialize(&buffer[shift], msg.content);
		if( msg.content.size() >= max_request_content_size ){
			return std::nullopt;
		}
		connection.consumeRead(shift);
		return msg;
	}

	bool asyncWriteRequest(AsyncOutputStream& connection, const MessageRequest& request){
		const size_t ct_size = request.content.size();
		const size_t tg_size = request.target.size();
		const size_t msg_size = ct_size + tg_size + request_static_size;

		if( msg_size < max_message_size && ct_size < max_request_content_size && tg_size < max_target_size ){
			std::vector<uint8_t> buffer;
			buffer.resize(message_length_size+msg_size);

			size_t shift = serialize(&buffer[0], msg_size);
			shift += serialize(&buffer[shift], request.request_id);
			buffer[shift++] = request.type;
			shift += serialize(&buffer[shift], request.target);
			shift += serialize(&buffer[shift], request.content);

			connection.write(std::move(buffer));
			return true;
		}else{
			return false;
		}
	}

	std::optional<MessageResponse> asyncReadResponse(Connection& connection){
		auto opt_buffer = connection.read(message_length_size);
		if(!opt_buffer.has_value()){
			return std::nullopt;
		}
		
		uint16_t msg_size;
		size_t shift = deserialize(*opt_buffer, msg_size);
		if(msg_size >= max_message_size){
			return std::nullopt;
		}

		opt_buffer = connection.read(message_length_size+msg_size);
		if(!opt_buffer.has_value()){
			return std::nullopt;
		}
		MessageResponse msg;
		uint8_t* buffer = *opt_buffer;
		shift += deserialize(&(buffer)[shift], msg.request_id);
		msg.return_code = buffer[shift++];
		shift += deserialize(&buffer[shift], msg.target);
		if( msg.target.size() >= max_target_size ){
			return std::nullopt;
		}

		shift += deserialize(&buffer[shift], msg.content);
		if( msg.content.size() >= max_request_content_size ){
			return std::nullopt;
		}
		connection.consumeRead(shift);
		return msg;
	}

	bool asyncWriteResponse(Connection& connection, const MessageResponse& request){
		const size_t ct_size = request.content.size();
		const size_t tg_size = request.target.size();
		const size_t msg_size = ct_size + tg_size + request_static_size;
		if( msg_size < max_message_size && ct_size < max_request_content_size && tg_size < max_target_size ){
			std::vector<uint8_t> buffer;
			buffer.resize(message_length_size+msg_size);

			size_t shift = serialize(&buffer[0], msg_size);
			shift += serialize(&buffer[shift], request.request_id);
			buffer[shift++] = request.return_code;
			shift += serialize(&buffer[shift], request.target);
			shift += serialize(&buffer[shift], request.content);

			connection.write(std::move(buffer));
			return true;
		}else{
			return false;
		}
	}
	*/
}
