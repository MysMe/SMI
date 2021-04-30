#pragma once
#include <iostream>
#define NOMINMAX
#include "Curl.h"
#include <optional>
#include "Interface.h"

class facebookInterface final : public socialMediaInterface
{
    std::string APP_ID, APP_TOKEN;
    std::string access_token;

    flowController getAccessToken(std::string unique_code, std::string_view)
    {
        requestWrapper request("https://graph.facebook.com/v2.6/device/login_status");
        std::string control_token = "access_token=" + APP_ID + '|' + APP_TOKEN + "&code=" + std::string(unique_code);
        const auto response = request.post(control_token);

        access_token = extract(response.response, "\"access_token\":");
        if (access_token.empty())
        {
            return flowController("Failed to extract access token.");
        }
        else
        {
            return flowController();
        }
    }

    facebookInterface() = default;
public:
    facebookInterface(use_default_credentials) :
        APP_ID("267288184992249"),
        APP_TOKEN("409e36440f171651ac5dcc7405668f7f")
    {}

    flowController authenticate() final
    {
        const std::string resource = "https://graph.facebook.com/v2.6/device/login";
        const std::string access_token = "access_token=" + APP_ID + '|' + APP_TOKEN;

        requestWrapper request(resource);
        const auto response = request.post(access_token);

        //Extract the unique code, user code and verification URL
        size_t pos = 0;
        const auto code = extract(response.response, "\"code\":", pos);
        const auto user = extract(response.response, "\"user_code\":", pos);
        std::string url = std::string(extract(response.response, "\"verification_uri\":", pos));
        url.erase(std::remove_if(url.begin(), url.end(), [](char v) {return v == '\\'; }), url.end());

        //Redirect the user to facebook login
        return flowController(flowController::flowType::redirect, url,
            [&obj = *this, unique_code = std::string(code), user_code = std::string(user)](std::string_view) mutable
        {
            //Provide the user with the access code
            return flowController(flowController::flowType::prompt, user_code,
                [&obj = obj, unique_code = std::move(unique_code)](std::string_view) mutable
            {
                return flowController(flowController::flowType::wait, "",
                    [&obj = obj, unique_code = std::move(unique_code)](std::string_view)
                {
                    //Check for access
                    return obj.getAccessToken(unique_code, "");
                });
            });
        });
    }

    std::string getUserID() const final
    {
        requestWrapper request("https://graph.facebook.com/me?fields=id&access_token=" + access_token);
        const auto response = request.get();
        return std::string(extract(response.response, "\"id\":"));
    }
    std::vector<std::string> getLinkedUsers() const final
    {
        requestWrapper request("https://graph.facebook.com/me/friends?fields=id&access_token=" + access_token);
        const auto response = request.get();

        const std::string tag = "\"data\":[";
        const auto div = response.response.find(tag);
        if (div == std::string::npos)
            return {};
        const auto end = response.response.find(']', div + tag.size());

        std::string_view search{ response.response.data() + div + tag.size(), end - div - tag.size() };

        std::vector<std::string> ret;
        size_t pos = 0;
        while (true)
        {
            std::string current{ extract(search, "\"id\":", pos) };
            if (current.empty())
                break;
            ret.emplace_back(std::move(current));
        }

        return ret;
    }

    std::string toData() const final
    {
        return APP_ID + ',' + APP_TOKEN + ',' + access_token;
    }

    void loadFromData(std::string_view data) final
    {
        const auto div1 = data.find(',');
        APP_ID.assign(data.data(), div1);
        const auto div2 = data.find(',', div1 + 1);
        APP_TOKEN.assign(data.data() + div1 + 1, div2 - div1 - 1);
        access_token.assign(data.data() + div2 + 1, data.size() - div2 - 1);
    }
};