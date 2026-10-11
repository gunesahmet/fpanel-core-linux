#pragma once
#ifndef FPANEL_CORE_JOBS_H
#define FPANEL_CORE_JOBS_H
#include <iostream>
#include <mutex>
#include <tuple>
#include <tsl/ordered_map.h>

class Jobs {
    tsl::ordered_map<int, std::string> queue_jobs;
    std::mutex queue_job_mutex;
public:
    Jobs() {
        queue_jobs = tsl::ordered_map<int, std::string>();
    }

    void addJob(std::string job)
    {
        std::lock_guard<std::mutex> lockGuard(queue_job_mutex);
        int job_id = 1;
        if(queue_jobs.rbegin() != queue_jobs.rend()) {
            if(queue_jobs.size() > 0) {
                int last_job_id = queue_jobs.rbegin()->first;
                job_id = last_job_id + 1;
            }
        }
        queue_jobs.insert({ job_id , job });
    }

    std::string getJob()
    {
        std::lock_guard<std::mutex> lockGuard(queue_job_mutex);
        if(queue_jobs.rbegin() != queue_jobs.rend()) {
            if(queue_jobs.size() > 0) {
                int job_id = queue_jobs.begin()->first;
                std::string job = queue_jobs.begin()->second;
                queue_jobs.erase(job_id);
                return job;
            }
        }
        return {};
    }
};
#endif //FPANEL_CORE_JOBS_H
