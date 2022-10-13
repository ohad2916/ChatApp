#pragma once
#include <string>
#include <format>


struct Message{
	enum Type {text=1,namechange,disconnect};
	Type _type;
	short _size;
	std::string content{};

	//basic constructor,takes a string reference for content
	Message(Type type, const std::string& msg) :								//copying
		_type(type), _size(msg.size() + 1)
	{
		content = std::format("{}{}", (char)_type, msg);
	}

	//constructs from char* instead of std::string
	Message(Type type, const char& msg,size_t size) :							//copy from char* instead of std::string
		_type(type), _size(size + 1)
	{
		content = std::format("{}{}", (char)_type, msg);
	}
	Message(Type type, const char&& msg, size_t size) :							//copy from char* instead of std::string
		_type(type), _size(size + 1)
	{
		content = std::format("{}{}", (char)_type, msg);
	}


	const char* getData() {
		return content.c_str();
	}
	//destructor
	~Message() {
		//does nothhing for now. might be used to deal with heap allocation later on.
	}
	//cancel default constructor
	Message() = delete;

};