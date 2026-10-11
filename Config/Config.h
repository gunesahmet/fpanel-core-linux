#pragma once
#ifndef LICENCE_CONFIG_H
#define LICENCE_CONFIG_H

#include <iostream>
#include <map>
#include <chrono>
#include <thread>
#include <vector>
#include <mutex>

#include <Logger/Logger.h>
#include <Ini/inireader.h>
#include <Crypto/Crypto.h>
#include <Helpers/Helpers.h>


class Config {
private:
    std::string cloud_config_location = "/usr/local/fpanel/etc/cloud.config.ini";
    std::string panel_config_file = "/usr/local/fpanel/etc/panel.conf";
    std::string panel_dir = "/usr/local/fpanel/";
    std::map<std::string, std::string> configurations;
    std::mutex conf_mutex;
    Helpers _helpers;
    Logger _logger;

public:
    Config()
    {
        configurations = std::map<std::string, std::string>();
    }

    ~Config()
    {
        configurations.clear();
    }

    void setConfigFile(std::string &config_file)
    {
        cloud_config_location = config_file;
    }

    void setLogger(Logger logger)
    {
        _logger = logger;
    }

    void init()
    {
        if (!_helpers.IsFileExists(panel_config_file)) {
            _logger.Error("System Config not found");
            exit(1);
        }

        std::lock_guard<std::mutex> lockGuard(conf_mutex);
        configurations.empty();
        configurations.clear();

        std::string main_ip = _helpers.CmdOutput(panel_dir + "core/bin/networkmng --show-default-ip");
        _helpers.trim(main_ip);

        INIReader cloud_reader(cloud_config_location);
        std::string allowed_ip = "127.0.0.1";
        std::string default_server_ip = "127.0.0.1";
        std::string default_server_port = "8889";
        if (cloud_reader.ParseError() < 0) {
            configurations.insert({ "cloud", "0" });
            configurations.insert({ "is_main", "1" });
        }
        else {
            allowed_ip = cloud_reader.Get("socket.server", "address", "127.0.0.1");
            _helpers.trim(allowed_ip);
            default_server_ip = "0.0.0.0";
            default_server_port = "8889";
            configurations.insert({ "cloud", "1" });

            if(main_ip.compare(allowed_ip) == 0) {
                configurations.insert({ "is_main", "1" });
            }
            else {
                configurations.insert({ "is_main", "0" });
            }
        }


        std::string secret_key;
        std::string secret_iv;
        std::string panel_sql_pass;

        std::ifstream file;
        file.open (panel_config_file);
        if (!file.is_open()) {
            std::cout << "Config can't open" << std::endl;
            _logger.Error("System config can not open");
            exit (EXIT_FAILURE);
        }

        std::string word;
        std::string first_key;

        int i = 0;
        while (file >> word) {
            if(i == 0)
                secret_key = word;
            if(i == 1)
                secret_iv = word.substr(0, 16);
            if(i == 2)
                panel_sql_pass = word;
            i++;
        }
        _helpers.trim(secret_key);
        _helpers.trim(secret_iv);
        configurations.insert({ "secret.key", secret_key });
        configurations.insert({ "secret.iv", secret_iv });


        configurations.insert({ "socket.allowed.address", allowed_ip });
        configurations.insert({ "socket.server.address", default_server_ip });
        configurations.insert({ "socket.server.port", default_server_port });
        configurations.insert({ "server.sql.pass", panel_sql_pass });

        configurations.insert({ "panel_dir", panel_dir });
        configurations.insert({ "root_bin_path", panel_dir + "core/bin/" });
        configurations.insert({ "root_sbin_path", panel_dir + "core/sbin/" });

        configurations.insert({ "httpd_dir", panel_dir + "httpd/" });
        configurations.insert({ "htdocs_dir", panel_dir + "httpd/htdocs/" });
    }

    std::map<std::string, std::string> getConfigs()
    {
        std::lock_guard<std::mutex> lockGuard(conf_mutex);
        return configurations;
    }

    std::string getValue(std::string key)
    {
        std::lock_guard<std::mutex> lockGuard(conf_mutex);
        return configurations.find(key)->second;
    }
};
#endif //FPANEL_CORE_CONFIG_H