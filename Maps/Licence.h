#pragma once
#ifndef FPANEL_CORE_LICENCE_H
#define FPANEL_CORE_LICENCE_H

#include <mutex>
#include <tsl/ordered_map.h>
#include <boost/make_shared.hpp>

class Licence {
    std::map<int, boost::shared_ptr<std::string>, std::less<>> *licence;
    std::mutex licence_mutex;
public:
    Licence() {
        licence = new std::map<int, boost::shared_ptr<std::string>, std::less<>>();
    }

    void add(const std::string& licence_data)
    {
//        std::lock_guard<std::mutex> lockGuard(licence_mutex);
        int req_id = 0;

        if(getTotal() > 3) {
            if(licence->rend() != licence->rbegin()) {
                std::cout << "licence data: " << licence->rbegin()->first << std::endl;
                int first_req_id = licence->rbegin()->first;
                licence->erase(first_req_id);
            }
        }
        if(!licence->empty()) {
            req_id = licence->rend()->first + 1;
        }

        boost::shared_ptr<std::string> licence_ptr(new std::string(licence_data));
        licence->insert({ req_id, licence_ptr });
    }

    void clear()
    {
//        std::lock_guard<std::mutex> lockGuard(licence_mutex);
        licence->clear();
    }

    std::string getLast()
    {
//        std::lock_guard<std::mutex> lockGuard(licence_mutex);
        if(licence->rbegin() != licence->rend()) {
            if(!licence->empty()) {
                boost::shared_ptr<std::string> request = licence->rbegin()->second;
                return request->data();
            }
        }
        return {};
    }

    size_t getTotal()
    {
//        std::lock_guard<std::mutex> lockGuard(licence_mutex);
        return licence->size();
    }
};
#endif //FPANEL_CORE_LICENCE_H