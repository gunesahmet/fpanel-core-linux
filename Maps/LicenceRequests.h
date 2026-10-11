#pragma once
#ifndef FPANEL_CORE_LICENCE_REQUESTS_H
#define FPANEL_CORE_LICENCE_REQUESTS_H

#include <mutex>
#include <tsl/ordered_map.h>
#include <boost/make_shared.hpp>

class LicenceRequests {
private:
    std::map<long, boost::shared_ptr<std::string>, std::less<>> *requests;
//    std::mutex licence_request_mutex;
public:
    LicenceRequests() {
        requests = new std::map<long, boost::shared_ptr<std::string>, std::less<>>();
    }

    void add(const std::string& request)
    {
//        std::lock_guard<std::mutex> lockGuard(licence_request_mutex);
        int req_id = 1;

        if(getTotalTry() > 3) {
            if(requests->rend() != requests->rbegin()) {
                std::cout << "licence request data: " << requests->rbegin()->first << std::endl;
                int first_req_id = requests->rbegin()->first;
                requests->erase(first_req_id);
            }
        }
        if(!requests->empty()) {
            req_id = requests->rend()->first + 1;
        }
        boost::shared_ptr<std::string> ptr(new std::string(request));
        requests->insert({ req_id, ptr });
    }

    void clear()
    {
//        std::lock_guard<std::mutex> lockGuard(licence_request_mutex);
        requests->clear();
    }

    size_t getTotalTry()
    {
//        std::lock_guard<std::mutex> lockGuard(licence_request_mutex);
        return requests->size();
    }
};
#endif //FPANEL_CORE_LICENCE_REQUESTS_H