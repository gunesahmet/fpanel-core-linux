#pragma once
#ifndef FPANEL_CORE_LICENCE_COMMAND_H
#define FPANEL_CORE_LICENCE_COMMAND_H

#include <Rapidjson/document.h>
#include <Rapidjson/writer.h>
#include <Rapidjson/stringbuffer.h>
#include <Maps/LicenceRequests.h>
#include <Maps/Licence.h>
#include <Crypto/Crypto.h>
#include <Helpers/Helpers.h>
#include <Socket/TcpClient.h>

using namespace rapidjson;

class LicenceCommand {
private:
    Helpers _helpers;
    Logger _logger;
    Config *_config;
    Crypto _crypto;
    LicenceRequests *_licence_requests;
    std::string last_request_json;
    Licence *_licence_map;
    std::string licence_server_key = "7EDbOEyDQAdg8AjZjRgcX4KOExvmfuWP";
    std::string licence_server_iv = "5AfcicxDnO7H6u7K";
    enum request_types { SERVER_REGISTER, LICENCE_REQUEST };
    std::string default_licence_result;
    std::string licence_server_addr = "licence.fpanel.net";
    int licence_server_port = 8871;

    std::string GenerateDefaultResult()
    {
        StringBuffer last_s_json;
        Writer<StringBuffer> writer(last_s_json);

        writer.StartObject();

        writer.Key("created_date");
        writer.String("none");

        writer.Key("licence_hash");
        writer.String("");

        writer.Key("licence_key");
        writer.String("");

        writer.Key("expire_date");
        writer.String("");

        writer.Key("status");
        writer.String("0");

        writer.Key("domains");
        writer.String("10");

        writer.Key("clients");
        writer.String("10");

        writer.Key("current_date");
        writer.String(_helpers.CurrentDateTimeWithMs().c_str());

        writer.EndObject();

        return last_s_json.GetString();
    }

    std::string GenerateLicenceRequest()
    {
        std::string default_ip = _helpers.CmdOutput(_config->getValue("root_bin_path") + "networkmng --show-default-ip");
        _helpers.trim(default_ip);
        std::string licence_key = _helpers.createLicenceKey(default_ip, _config->getValue("secret.key"), _config->getValue("secret.iv"));

        StringBuffer last_s_json;
        Writer<StringBuffer> writer(last_s_json);

        writer.StartObject();

        writer.Key("type");
        writer.Int64(request_types::LICENCE_REQUEST);

        writer.Key("hash");
        writer.String(licence_key.c_str());

        writer.Key("key");
        writer.String(_config->getValue("secret.key").c_str());

        writer.Key("iv");
        writer.String(_config->getValue("secret.iv").c_str());

        writer.Key("ip_addr");
        writer.String(default_ip.c_str());

        writer.EndObject();

        last_request_json = last_s_json.GetString();
//        std::string enc_cmd_result = _crypto.Encrypt(last_request_json, licence_server_key, licence_server_iv);
        last_s_json.Clear();
        return last_request_json;
    }

    void LicenceRequest(const std::string& request)
    {
        TcpClient *_tcp_client = new TcpClient();
        std::string crypted_request = _crypto.Encrypt(request, licence_server_key, licence_server_iv);
        _tcp_client->SetLogger(_logger);
        _tcp_client->SetServer(licence_server_addr, licence_server_port);
        bool connect_status = _tcp_client->Connect();
        if(connect_status) {
            bool send_status = _tcp_client->Send(crypted_request);
            if(send_status) {
                _tcp_client->Read();
                std::string response = _tcp_client->Response();
                std::string decoded_response = _crypto.Decrypt(response, licence_server_key, licence_server_iv);
                _logger.Debug("Licence Response: %s", decoded_response.c_str());
                _licence_map->add(decoded_response);
                _licence_requests->clear(); // If connection success and get a response we clear map for total try
            }
            _tcp_client->Close();
        }
        else {
            _logger.Debug("Default Licence Result: %s", default_licence_result.c_str());
            _licence_requests->add(request); // If connection failed we add a request for calculate total try
            if(_licence_requests->getTotalTry() > 3) {
                _licence_map->add(default_licence_result);
            }
            if(_licence_map->getTotal() == 0) {
                _logger.Debug("Licence map is empty.");
                _licence_map->add(default_licence_result);
            }
            _logger.Debug("Total Try: %d", _licence_requests->getTotalTry());
        }
    }

    void initLicence()
    {
        int min_time = 21600;
        int max_time = 43200;

        std::string request = GenerateLicenceRequest();

        while (true) {
            int rand_time = rand()%(max_time-min_time + 1) + min_time;
            LicenceRequest(request);
            _helpers.SecSleep(rand_time);
        }
    }

public:
    LicenceCommand() {
        _licence_requests = new LicenceRequests();
        std::string result = GenerateDefaultResult();
        default_licence_result = result;
    }

    LicenceCommand& setLicenceMap(Licence *licence_map)
    {
        _licence_map = licence_map;
        return *this;
    }

    LicenceCommand& setLogger(Logger logger)
    {
        _logger = logger;
        return *this;
    }

    LicenceCommand& setConfig(Config *config)
    {
        _config = config;
        return *this;
    }

    void forceCheck()
    {
        std::string request = GenerateLicenceRequest();
        LicenceRequest(request);
    }

    void run()
    {
        std::thread t1(&LicenceCommand::initLicence, this);
        t1.detach();
    }
};


#endif //FPANEL_CORE_LICENCE_COMMAND_H
