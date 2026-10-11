#pragma once
#ifndef FPANEL_CORE_BACKGROUND_H
#define FPANEL_CORE_BACKGROUND_H
#include <Database/MySQL.h>
#include <Helpers/Helpers.h>
#include <map>
#include <Socket/TcpClient.h>
#include <Crypto//Crypto.h>

using namespace rapidjson;

class Background {
private:
    Helpers _helpers;
    Logger _logger;
    Config *_config{};
    bool statistics_running = false;
    bool update_running = false;

    void initStatistics()
    {
        // int min_time = 2;
        // int max_time = 3;
        int min_time = 21600;
        int max_time = 32400;
        while (true) {
            int rand_time;
            rand_time = rand()%(max_time-min_time + 1) + min_time;

            INIReader reader(_config->getValue("panel_dir") + "httpd/htdocs/config/config.ini");
            if (reader.ParseError() < 0) {
                _logger.Debug("This is not main cloud");
                return;
            }

            long is_random = reader.GetInteger("statistics", "random", 0);
            std::string calculate_time = reader.Get("statistics", "time", "03:00");

            if(is_random == 0) {
                std::string current_date = _helpers.currentDate();
                std::string current_date_time = _helpers.currentDateTime();

                long statistics_timestamp = _helpers.getTimestamp(current_date + " " + calculate_time + ":00");
                long current_timestamp = _helpers.getTimestamp(current_date_time);

                long wait_time = statistics_timestamp - current_timestamp;


                if(wait_time < 0) {
                    wait_time = 86400 + wait_time;
                }
                _logger.Debug("Waiting for    : %d", wait_time);

                _helpers.SecSleep(wait_time);

                while (true) {
                    if (update_running)
                        _helpers.SecSleep(1);
                    else
                        break;
                }
                _logger.Debug("Starting time defined calculation...");
            }
            else {
                _helpers.SecSleep(rand_time);
                _logger.Debug("Starting randomized time calculation...");
            }

            statistics_running = true;
            _helpers.CmdOutput(_helpers.stringFormat("%sstatisticsmng --init",
                                                     _config->getValue("root_bin_path").c_str()));
            statistics_running = false;
            _helpers.SecSleep(5); // Just in case
        }
    }

    void initUpdater()
    {
        int min_time = 21600;
        int max_time = 32400;
        INIReader reader(_config->getValue("panel_dir") + "httpd/htdocs/config/config.ini");
        if (reader.ParseError() < 0) {
            _logger.Debug("This is not main cloud");
            return;
        }
        long is_auto = reader.GetInteger("update", "random", 0);
        if(is_auto == 0) {
            _logger.Debug("Updater is not automatic");
            return;
        }
        while (true) {
            int rand_time;
            rand_time = rand()%(max_time-min_time + 1) + min_time;

            std::string server_address;


            long is_random = reader.GetInteger("update", "random", 0);
            std::string update_time = reader.Get("update", "time", "03:00");

            if(is_random == 0) {
                std::string current_date = _helpers.currentDate();
                std::string current_date_time = _helpers.currentDateTime();

                long update_timestamp = _helpers.getTimestamp(current_date + " " + update_time + ":00");
                long current_timestamp = _helpers.getTimestamp(current_date_time);

                long wait_time = update_timestamp - current_timestamp;

                if(wait_time < 0) {
                    wait_time = 86400 + wait_time;
                }
                _logger.Debug("Waiting for    : %d", wait_time);

                _helpers.SecSleep(wait_time);

                while (true) {
                    if (statistics_running)
                        _helpers.SecSleep(1);
                    else
                        break;
                }
                _logger.Debug("Starting time defined update...");

            }
            else {
                _helpers.SecSleep(rand_time);
                _logger.Debug("Starting randomized time update...");
            }

            update_running = true;
            _helpers.CmdOutput(_config->getValue("root_bin_path") + "updatemng --init --server-addr=" + server_address);
            update_running = false;
            _helpers.SecSleep(5); // Just in case
        }
    }
public:
    Background& setLogger(Logger logger)
    {
        _logger = logger;
        return *this;
    }

    Background& setConfig(Config *config)
    {
        _config = config;
        return *this;
    }

    void run()
    {
        std::thread t1(&Background::initStatistics, this);
        t1.detach();

        std::thread t3(&Background::initUpdater, this);
        t3.detach();
    }
};


#endif //FPANEL_CORE_BACKGROUND_H
