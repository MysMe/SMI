#include <iostream>
#define NOMINMAX
#include "Curl.h"

#include <string>
#include <liboauthcpp/liboauthcpp.h>
#include <optional>
#include "Facebook.h"
#include "Twitter.h"

flowController applyFlow(flowController& flow)
{
    switch (flow.getType())
    {
    case(flowController::flowType::redirect):
        std::cout << "Visit " << flow.getBody() << '\n';
        return flow.advance("");

    case(flowController::flowType::prompt):
        std::cout << "Input " << flow.getBody() << '\n';
        return flow.advance("");

    case(flowController::flowType::wait):
        std::cout << "When complete, press enter.\n";
        std::cin.ignore();
        return flow.advance("");

    case(flowController::flowType::input):
    {
        std::cout << "Input data:\n";
        std::string contents;
        std::getline(std::cin, contents);
        return flow.advance(contents);
    }

    default:
    case(flowController::flowType::complete):
        std::cout << "Complete.\n";
        return flow.advance("");
    }
}

bool enactFlow(flowController& flow)
{
    while (flow.isComplete() == flowController::flowResult::partial)
        flow = applyFlow(flow);
    return flow.isComplete() == flowController::flowResult::complete;
}

constexpr auto twitterExampleAuth = "2IjAR94bcnL8kCF4Su9Zy7VKw,Xf1OB3EzN91M6wCebf7IkQwjfrQ5mdcS2MHvv2Iqd5efd9S1PF,1374738572197122052-c6DfSvEmwy6RqlokZMXsSpEWQV5K5w,EZmQXU9J6oLfGx9fvmE8wznPtjsqQ5VJK9XCQJ2z8JU5x";
constexpr auto facebookExampleAuth = "267288184992249,409e36440f171651ac5dcc7405668f7f,GGQVlhUzNwNFBaY29DeGFyY0JJQzllb2JBOE1zQzJFZAlFEMTlTZAUk3VEtjaGlVLWNLYlEtcDlGSTdaZAG5lQUVxUnQ1QXU3cXNsTmpZATE50U3hhSHVSaW9DM2owekJIdjNaeGZAUOEtUTUxDWTQweXExTE05Mlk5Nm5jRDBHSGdpbHdQdwZDZD";


unsigned int getUserOption(std::string_view msg, const unsigned int min, const unsigned int max)
{
    std::cout << msg << '\n';
    unsigned int input;
    while (!(std::cin >> input) || input < min || input > max)
    {
        std::cout << "Invalid entry.\n";
        //Clear invalid data from input buffer
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    //Clear any extra data (e.g. the trailing newline) from the input buffer
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return input;
}

void applyOption(socialMediaInterface& SMI, const unsigned int option, std::string_view additionalData)
{
    switch (option)
    {
    case(1): //Authenticate
        enactFlow(SMI.authenticate());
        break;
    case(2): //Load
        SMI.loadFromData(additionalData);
        std::cout << "Load Complete.\n";
        break;
    case(3): //Show UID
    {
        const auto ID = SMI.getUserID();
        if (ID.empty())
            std::cout << "Unable to get user ID from " << additionalData << ". Ensure that this account is authenticated.\n";
        else
            std::cout << additionalData << " UID: " << ID << ".\n";
        break;
    }
    case(4): //Show linked users
    {
        const auto users = SMI.getLinkedUsers();
        if (users.empty())
        {
            std::cout << additionalData << " did not provide any linked users.\n";
        }
        else
        {
            std::cout << additionalData << " linked users:\n";
            for (const auto& i : users)
                std::cout << '\t' << i << '\n';
        }
        break;
    }
    default:
        assert(false);
    }
}

int main()
{
    twitterInterface twitter{ use_default_credentials() };
    facebookInterface facebook{ use_default_credentials() };

    while (true)
    {
        const unsigned int option = getUserOption(
            R"(Select option:
1. Authorise a Twitter account (account required).
2. Authorise a Facebook account (account required).
3. Use example Twitter account.
4. Use example Facebook accout.
5. Get Twitter ID.
6. Get Facebook ID.
7. Get Twitter linked users.
8. Get Facebook linked users.
9. Exit.
)", 1, 9);

        if (option == 9)
            break;

        //For every Nth option on one interface, the N+1th option is the same but for the other interface
        const bool currentInterface = option & 1;

        //Note the reliance on integer truncation
        const unsigned int sharedOption = (option + 1) / 2;

        socialMediaInterface& SMI = currentInterface ? static_cast<socialMediaInterface&>(twitter) : static_cast<socialMediaInterface&>(facebook);
        //Additional data is either the name of the interface or the authentication information, depending on the option
        std::string_view additionalData;
        if (sharedOption == 2)
            additionalData = currentInterface ? twitterExampleAuth : facebookExampleAuth;
        else
            additionalData = currentInterface ? "Twitter" : "Facebook";
        applyOption(SMI, sharedOption, additionalData);
        std::cout << '\n';
    }
    return 0;
}