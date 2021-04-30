#pragma once
#include <string>
#include <vector>
#include <functional>

class flowController
{
public:
    enum class flowType
    {
        redirect, //Request the user to visit a website or other resource
        prompt, //Provide the user a keyword or code for use externally
        input, //Request the input of an external keyword or code from an external source
        wait, //Wait until some condition
        complete //No more steps
    };
    enum class flowResult
    {
        complete,
        partial,
        failed
    };
private:
    flowType type;
    std::function<flowController(std::string_view)> callback;
    std::string contents;
public:

    flowController() : type(flowType::complete) {}
    flowController(std::string_view errorMessage) : type(flowType::complete), contents(errorMessage) {}

    template <class Fn>
    flowController(flowType _type, std::string_view message, Fn func) : type(_type), contents(message), callback(func) {}

    flowType getType() const { return type; }
    const std::string& getBody() const { return contents; }
    flowController advance(std::string_view data = "") const
    {
        return callback(data);
    }
    flowResult isComplete() const
    {
        if (type == flowType::complete)
        {
            if (contents.empty())
                return flowResult::complete;
            else
                return flowResult::failed;
        }
        else
        {
            return flowResult::partial;
        }
        
    }
};


struct use_default_credentials {};

class socialMediaInterface
{
protected:
    static std::string_view extract(std::string_view source, std::string_view key, size_t& off)
    {
        const auto loc = source.find(key, off);
        if (loc == std::string::npos)
            return "";
        const auto div = loc + key.size();
        //Add 1 to skip the opening '\"'
        const auto end = source.find('\"', div + 1);
        off = end;
        //Remove opening and closing '\"'
        return source.substr(div + 1, end - div - 1);
    }

    static std::string_view extract(std::string_view source, std::string_view key)
    {
        size_t zero = 0;
        return extract(source, key, zero);
    }
public:
	[[nodiscard]]
	virtual flowController authenticate() = 0;
	[[nodiscard]]
	virtual std::string getUserID() const = 0;
	[[nodiscard]]
	virtual std::vector<std::string> getLinkedUsers() const = 0;

	[[nodiscard]]
	virtual std::string toData() const = 0;

    virtual void loadFromData(std::string_view) = 0;

	virtual ~socialMediaInterface() = default;
};