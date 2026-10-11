#pragma once
#ifndef CORE_COMMANDS_H
#define CORE_COMMANDS_H
#include <iostream>
#include <Helpers/Helpers.h>
#include <Logger/Logger.h>
#include <Config/Config.h>
#include <Maps/Licence.h>
#include <Maps/Domains.h>
#include <Commands/Background.h>
#include <Commands/LicenceCommand.h>


class Commands {
private:
    Helpers _helpers;
    Logger _logger;
    Config *_config;
    Licence *_licence_map;
    LicenceCommand *_licence_command;

public:
    Commands& setLogger(Logger logger)
    {
        _logger = logger;
        return *this;
    }

    Commands& setConfig(Config *config)
    {
        _config = config;
        return *this;
    }

    Commands &setLicenceMap(Licence *licence_map)
    {
        _licence_map = licence_map;
        return *this;
    }

    Commands &setLicenceCommand(LicenceCommand *licence_command)
    {
        _licence_command = licence_command;
        return *this;
    }

    std::string Run(std::string request)
    {
        _helpers.trim(request);
        std::string cmd_result;
        if (request.compare("licencemng --check") == 0) {
            _logger.Debug("Licence checking...");
            cmd_result = _licence_map->getLast();
        }
        else if (request.compare("licencemng --force-check") == 0) {
            _logger.Debug("Licence force checking...");
            _licence_command->forceCheck();
            cmd_result = _licence_map->getLast();
        }
        else {
            _logger.Debug("System Command running...");
            std::string command = _config->getValue("root_bin_path") + request;
            _logger.Debug("command: %s", command.c_str());
            cmd_result = _helpers.CmdOutput(command);
        }
        _logger.Debug("cmd_result: %s", cmd_result.c_str());
        return cmd_result;
    }
};
#endif //CORE_COMMANDS_H
