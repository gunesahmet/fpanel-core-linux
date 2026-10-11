#pragma once
#ifndef FPANEL_CORE_DOMAINS_H
#define FPANEL_CORE_DOMAINS_H
#include <iostream>
#include <mutex>
#include <tuple>
#include<fstream>
#include<Config/Config.h>
#include<Helpers/Helpers.h>

#include <Rapidjson/document.h>
#include <Rapidjson/writer.h>
#include <Rapidjson/stringbuffer.h>

#include <tsl/ordered_map.h>
using namespace rapidjson;


class Domains {
    Helpers helpers;
    tsl::ordered_map<std::string, std::string> domain_lists;
    std::mutex domain_lists_mutex;
public:
    Domains() {
        // Name => Ip Address
        domain_lists = tsl::ordered_map<std::string, std::string>();
    }

    void addDomain(std::string domain, std::string ip_address)
    {
        std::lock_guard<std::mutex> lockGuard(domain_lists_mutex);
        domain_lists.insert({ domain , ip_address });
        domain_lists.shrink_to_fit();
    }

    void removeDomain(std::string domain)
    {
        std::lock_guard<std::mutex> lockGuard(domain_lists_mutex);
        if(domain_lists.rbegin() != domain_lists.rend()) {
            domain_lists.erase(domain);
            domain_lists.shrink_to_fit();
        }
    }

    void loadDomains(Config *_config)
    {
        std::string command = _config->getValue("httpd_dir") + "php/bin/php "  + _config->getValue("htdocs_dir") + "cli.php Domain list";
        std::string cmd_result = helpers.CmdOutput(command.c_str());
        try {
            Document d;
            const char *json = cmd_result.c_str();

            if (!d.Parse(json).HasParseError()) {
                if(d.IsArray()) {
                    for (SizeType i = 0; i < d.Size(); i++) {
                        domain_lists.insert({ std::string(d[i]["domain_name"].GetString()) , std::string(d[i]["ip_addr"].GetString()) });
                    }
                    std::cout << "Loading done!" << std::endl;
                }

            }
        }
        catch (...) {
        }

        // Domain listesini cli ile php den json olarak cek
    }
};
#endif //FPANEL_CORE_DOMAINS_H
