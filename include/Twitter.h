#pragma once
#include <iostream>
#define NOMINMAX
#include "Curl.h"

#include <string>
#include <liboauthcpp/liboauthcpp.h>
#include <optional>

class twitterInterface final : public socialMediaInterface
{
    std::string APP_KEY, APP_SECRET;
    std::optional<OAuth::Token> userToken;

    static std::optional<OAuth::Token> getRequestToken(OAuth::Consumer& consumer, OAuth::Client& oauth)
    {  

        std::string base_request_token_url = "https://api.twitter.com/oauth/request_token?oauth_callback=oob";
        std::string oAuthQueryString =
            oauth.getURLQueryString(OAuth::Http::Get, base_request_token_url);
        std::string url = "https://api.twitter.com/oauth/request_token?" + oAuthQueryString;
        requestWrapper request(url);
        const auto res = request.get();
        if (res.response_code == 200)
            return OAuth::Token::extract(res.response);
        else
            return std::nullopt;
    }

    static std::string getAuthLink(const OAuth::Token& request_token)
    {
        return "https://api.twitter.com/oauth/authorize?oauth_token=" + request_token.key();
    }


    static std::optional<OAuth::Token> getAccessToken(OAuth::Consumer& consumer, OAuth::Token& requestToken)
    {
        OAuth::Client oauth{ &consumer, &requestToken };
        std::string oAuthQueryString = oauth.getURLQueryString(OAuth::Http::Get, "https://api.twitter.com/oauth/access_token", std::string(""), true);
        std::string url = "https://api.twitter.com/oauth/access_token?" + oAuthQueryString;
        requestWrapper request(url);
        const auto res = request.get();
        if (res.response_code == 200)
        {
            auto kvp = OAuth::ParseKeyValuePairs(res.response);
            return OAuth::Token::extract(kvp);
        }
        else
            return std::nullopt;
    }


    twitterInterface() = default;
public:

    twitterInterface(use_default_credentials) :
        APP_KEY{ "2IjAR94bcnL8kCF4Su9Zy7VKw" },
        APP_SECRET{ "Xf1OB3EzN91M6wCebf7IkQwjfrQ5mdcS2MHvv2Iqd5efd9S1PF" } {}

    flowController authenticate() final
    {
        OAuth::Consumer consumer(APP_KEY, APP_SECRET);
        OAuth::Client oauth(&consumer);

        auto maybeToken = getRequestToken(consumer, oauth);
        if (!maybeToken.has_value())
            return flowController("Unable to get request token.");

        auto& token = maybeToken.value();

        return flowController(flowController::flowType::redirect,
            getAuthLink(token),
            [this, consumer = consumer, token = token](std::string_view)
            {
                return flowController(flowController::flowType::input, "",
                    [this, consumer = consumer, token = token](std::string_view pin) mutable
                    {
                        token.setPin(std::string{ pin });
                        const auto maybeAccess = getAccessToken(consumer, token);
                        if (!maybeAccess.has_value())
                            return flowController("Unable to generate access token.");
                        this->userToken.reset();
                        this->userToken.emplace(maybeAccess.value());
                        return flowController();
                    }
                );
            }
            );
    }

    std::string getUserID() const final
    {
        if (!userToken.has_value())
            return{};

        OAuth::Consumer consumer(APP_KEY, APP_SECRET);
        OAuth::Token token(userToken->key(), userToken->secret());
        OAuth::Client oauth(&consumer, &token);

        const std::string resource = "https://api.twitter.com/1.1/account/verify_credentials.json";

        std::string oAuthQueryString =
            oauth.getURLQueryString(OAuth::Http::Get, resource + "?include_entities=false&skip_status=true&include_email=false");

        requestWrapper request(resource + "?" + oAuthQueryString);
        const auto result = request.get();

        if (result.response_code != 200)
            return "";

        //Fast and cheap JSON lookup
        const std::string key = "\"id\":";
        auto div = result.response.find(key);
        if (div == std::string::npos)
            return "";
        div += key.size();

        auto end = result.response.find(',', div);
        return result.response.substr(div, end - div);
    }
    std::vector<std::string> getLinkedUsers() const final
    {
        if (!userToken.has_value())
            return{};

        OAuth::Consumer consumer(APP_KEY, APP_SECRET);
        OAuth::Token token(userToken->key(), userToken->secret());
        OAuth::Client oauth(&consumer, &token);

        const std::string resource = "https://api.twitter.com/1.1/friends/ids.json";

        std::string oAuthQueryString =
            oauth.getURLQueryString(OAuth::Http::Get, resource);

        requestWrapper request(resource + "?" + oAuthQueryString);
        const auto result = request.get();

        if (result.response_code != 200)
            return {};

        //Fast and cheap JSON lookup
        const std::string key = "\"ids\":[";
        auto div = result.response.find(key);
        if (div == std::string::npos)
            return {};
        div += key.size();

        const auto end = result.response.find(']', div);

        std::vector<std::string> ret;

        auto left = div;
        while (left != end)
        {
            if (result.response[left] == ',')
                left++;
            auto next = result.response.find(',', left);
            if (next == std::string::npos || next > end)
                next = end;

            ret.emplace_back(result.response.substr(left, next - left));
            left = next;
        }

        return ret;
    }

    std::string toData() const final
    {
        return APP_KEY + ',' + APP_SECRET + ',' + userToken->key() + ',' + userToken->secret();
    }

    void loadFromData(std::string_view data) final
    {
        const auto div1 = data.find(',');
        APP_KEY.assign(data.data(), div1);
        const auto div2 = data.find(',', div1 + 1);
        APP_SECRET.assign(data.data() + div1 + 1, div2 - div1 - 1);
        const auto div3 = data.find(',', div2 + 1);
        std::string userKey{ data.data() + div2 + 1, div3 - div2 - 1 };
        std::string userSecret{ data.data() + div3 + 1, data.size() - div3 - 1 };
        userToken.emplace(userKey, userSecret);
    }
};

namespace twitter
{
    std::string consumer_key = "2IjAR94bcnL8kCF4Su9Zy7VKw"; // Key from Twitter
    std::string consumer_secret = "Xf1OB3EzN91M6wCebf7IkQwjfrQ5mdcS2MHvv2Iqd5efd9S1PF"; // Secret from Twitter
    std::string request_token_url = "https://api.twitter.com/oauth/request_token";
    std::string request_token_query_args = "oauth_callback=oob";
    std::string authorize_url = "https://api.twitter.com/oauth/authorize";
    std::string access_token_url = "https://api.twitter.com/oauth/access_token";
    std::string target_post_url = "https://api.twitter.com/1.1/statuses/update.json";
    std::string target_post_url_query_args = "status=";


    std::string getUserString(std::string prompt) {
        std::cout << prompt << " ";

        std::string res;
        std::cin >> res;
        std::cout << std::endl;
        return res;
    }

    std::optional<OAuth::Token> getRequestToken(OAuth::Consumer& consumer, OAuth::Client& oauth)
    {
        std::string base_request_token_url = request_token_url + (request_token_query_args.empty() ? std::string("") : (std::string("?") + request_token_query_args));
        std::string oAuthQueryString =
            oauth.getURLQueryString(OAuth::Http::Get, base_request_token_url);
        std::string url = request_token_url + "?" + oAuthQueryString;
        requestWrapper request(url);
        const auto res = request.get();
        if (res.response_code == 200)
            return OAuth::Token::extract(res.response);
        else
            return std::nullopt;
    }

    std::string getAuthLink(const OAuth::Token& request_token)
    {
        return authorize_url + "?oauth_token=" + request_token.key();
    }

    std::optional<OAuth::Token> getAccessToken(OAuth::Consumer& consumer, OAuth::Token& requestToken)
    {
        OAuth::Client oauth{ &consumer, &requestToken };
        std::string oAuthQueryString = oauth.getURLQueryString(OAuth::Http::Get, access_token_url, std::string(""), true);
        std::string url = access_token_url + "?" + oAuthQueryString;
        requestWrapper request(url);
        const auto res = request.get();
        if (res.response_code == 200)
        {
            auto kvp = OAuth::ParseKeyValuePairs(res.response);
            return OAuth::Token::extract(kvp);
        }
        else
            return std::nullopt;
    }

    std::optional<OAuth::Token> authenticateApplication()
    {
        OAuth::Consumer consumer(consumer_key, consumer_secret);
        OAuth::Client oauth(&consumer);

        auto maybeToken = getRequestToken(consumer, oauth);
        if (!maybeToken.has_value())
            return std::nullopt;

        auto& token = maybeToken.value();

        std::cout << "Redirect to: " << getAuthLink(token) << '\n';

        std::cout << "Enter pin:";
        {
            std::string pin;
            std::cin >> pin;
            token.setPin(pin);
        }

        const auto maybeAccess = getAccessToken(consumer, token);

        if (!maybeAccess.has_value())
            return std::nullopt;

        const auto& access = maybeAccess.value();

        std::cout << "Access token:" << std::endl;
        std::cout << "    - oauth_token        = " << access.key() << '\n';
        std::cout << "    - oauth_token_secret = " << access.secret() << '\n';

        return access;
    }

    std::optional<std::string> getUserID(OAuth::Token auth)
    {
        OAuth::Consumer consumer(consumer_key, consumer_secret);
        OAuth::Token token(auth.key(), auth.secret());
        OAuth::Client oauth(&consumer, &token);

        const std::string resource = "https://api.twitter.com/1.1/account/verify_credentials.json";

        std::string oAuthQueryString =
            oauth.getURLQueryString(OAuth::Http::Get, resource + "?include_entities=false&skip_status=true&include_email=false");

        requestWrapper request(resource + "?" + oAuthQueryString);
        const auto result = request.get();

        if (result.response_code != 200)
            return std::nullopt;

        //Fast and cheap JSON lookup
        const std::string key = "\"id\":";
        auto div = result.response.find(key);
        if (div == std::string::npos)
            return std::nullopt;
        div += key.size();

        auto end = result.response.find(',', div);
        return result.response.substr(div, end - div);
    }

    std::optional<std::vector<std::string>> getFriendIDs(OAuth::Token auth)
    {
        OAuth::Consumer consumer(consumer_key, consumer_secret);
        OAuth::Token token(auth.key(), auth.secret());
        OAuth::Client oauth(&consumer, &token);

        const std::string resource = "https://api.twitter.com/1.1/friends/ids.json";

        std::string oAuthQueryString =
            oauth.getURLQueryString(OAuth::Http::Get, resource);

        requestWrapper request(resource + "?" + oAuthQueryString);
        const auto result = request.get();

        if (result.response_code != 200)
            return std::nullopt;

        //Fast and cheap JSON lookup
        const std::string key = "\"ids\":[";
        auto div = result.response.find(key);
        if (div == std::string::npos)
            return std::nullopt;
        div += key.size();

        const auto end = result.response.find(']', div);

        std::vector<std::string> ret;

        auto left = div;
        while (left != end)
        {
            if (result.response[left] == ',')
                left++;
            auto next = result.response.find(',', left);
            if (next == std::string::npos || next > end)
                next = end;

            ret.emplace_back(result.response.substr(left, next - left));
            left = next;
        }

        return ret;
    }

    void apply()
    {
        const auto userToken = authenticateApplication();
        if (!userToken.has_value())
        {
            std::cout << "Failed to authenticate.\n";
            std::cin.ignore();
        }
        else
        {
            auto userID = getUserID(userToken.value());
            if (userID.has_value())
            {
                std::cout << "User ID is: " << userID.value() << '\n';
            }
            else
            {
                std::cout << "Unable to retireve user ID.\n";
            }

            auto friendIDs = getFriendIDs(userToken.value());
            if (friendIDs.has_value())
            {
                std::cout << "Linked Users:\n";
                for (const auto& i : friendIDs.value())
                    std::cout << '\t' << i << '\n';
            }
            else
            {
                std::cout << "Unable to retireve linked user IDs.\n";
            }
        }
    }
}
