#include <csignal>
#include <sys/stat.h>
#include <boost/program_options.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/filesystem.hpp>
#include <Logger/Logger.h>
#include <Config/Config.h>
#include <Socket/TcpServer.h>
#include <Maps/Licence.h>
#include <Helpers/Helpers.h>
#include <Commands/LicenceCommand.h>
#include <Commands/Commands.h>

Logger logger;
Config *config;

__pid_t current_pid;
std::string core_pid_file = "/var/run/fpanel-core.pid";
std::string debug_log_file = "/var/log/fpanel/core-debug.logs";
std::string error_log_file = "/var/log/fpanel/core-error.logs";
Helpers helpers;

enum {
    SecondLog = 1,
    MaxLogFileSize = 104857600 // 100MB
};

void writePidToFile(pid_t pid)
{
    std::ofstream pid_file (core_pid_file);
    if (pid_file.is_open()) {
        pid_file << pid;
        pid_file.close();
    }
}

void initLogs()
{
    plog::init(plog::debug, debug_log_file.c_str(), MaxLogFileSize);              // Initialize the default logger instance.
    plog::init<SecondLog>(plog::error, error_log_file.c_str(), MaxLogFileSize);   // Initialize the 2nd logger instance.
}

boost::program_options::variables_map commandLine(int argc, char** argv)
{
    boost::program_options::variables_map vm;

    try {
        bool t_given = false;

        boost::program_options::options_description desc("Allowed options");
        desc.add_options()
                // First parameter describes option name/short name
                // The second is parameter to option
                // The third is description
                ("help,h", boost::program_options::bool_switch(&t_given), "print usage message")
                ("debug,d", boost::program_options::bool_switch(&t_given), "Open debug mode")
                ("server,s", boost::program_options::bool_switch(&t_given), "Start as Server")
                ("systemd,D", boost::program_options::bool_switch(&t_given), "Run for systemd")
                ("config,c", boost::program_options::value<std::string>(), "Configuration file location")
                ("quit,q", boost::program_options::value<std::string>(), "Stop process")
                ;

        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help") && (vm["help"].as<bool>())) {
            std::cout << "FPanel Core Ver 1.0.0, for Linux (amd64)" << std::endl;
            std::cout << "Copyright (c) 2021 Kovengo Software Limited. All rights reserved." << std::endl;
            std::cout << "" << std::endl;
            std::cout << "FPanel is a registered trademark of Kovengo Software Limited. Other names may be trademarks of their respective owners." << std::endl;
            std::cout << "" << std::endl;
            std::cout << "Allowed options:\n"
                         "  --help|-h                Print usage message\n"
                         "  --server|-s              Start as Server\n"
                         "  --systemd|-D             Run for systemd\n"
                         "  --quit|-q                Stop process\n"<< std::endl;
            exit(1);
        }

    }
    catch(std::exception& e) {
        std::cerr << e.what() << std::endl;
        exit(1);
    }
    return vm;
}

void Start(int argc, char** argv)
{
    initLogs();
    if(getuid() != 0) {
        logger.Error("Please run as root user.");
        helpers.Println("Please run as root user.");
        exit(EXIT_FAILURE);
    }
    config = new Config();
    boost::program_options::variables_map vm = commandLine(argc, argv);
    current_pid = getpid();
    pid_t pid, sid;

    if (vm["debug"].as<bool>()) {
        logger.debug = true;
    }
    else {
        if (pid < 0) { exit(EXIT_FAILURE); }
        if (pid > 0) { exit(EXIT_SUCCESS); }
        writePidToFile(current_pid);
        if(!vm["systemd"].as<bool>()) {
            pid = fork();
        }
    }
    // Initialize Logger
    config->setLogger(logger);

    config->init();

    if(vm["server"].as<bool>()) {
        Licence *_licence_map = new Licence();
        Commands *commands = new Commands();
        Background *background = new Background();
        LicenceCommand *_licence_command = new LicenceCommand();

        _licence_command->setLicenceMap(_licence_map)
                .setLogger(logger)
                .setConfig(config)
                .run();

        background->setLogger(logger)
                .setConfig(config)
                .run();

        commands->setLogger(logger)
                .setConfig(config)
                .setLicenceMap(_licence_map)
                .setLicenceCommand(_licence_command);

        helpers.CmdOutput(config->getValue("root_bin_path") + "networkmng --init");

        std::string socket_server_address = config->getValue("socket.server.address");
        std::string socket_server_port = config->getValue("socket.server.port");

        logger.Debug("Server starting %s:%s", socket_server_address.c_str(), socket_server_port.c_str());

        TcpServer tcpServer;
        tcpServer.SetConfig(config)
        .SetLogger(logger)
        .SetHost(socket_server_address)
        .SetPort(std::stoi(socket_server_port))
        .SetCommands(commands)
        .Start();
    }
}

int main(int argc, char** argv)
{
    Start(argc, argv);
    return 0;
}