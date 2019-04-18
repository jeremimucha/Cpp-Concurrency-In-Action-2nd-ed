#pragma once

#include <thread>
#include <vector>

class join_threads {
public:
    explicit join_threads(std::vector<std::thread>& threads) noexcept
        : threads_{threads}
    {
    }
    ~join_threads() noexcept
    {
        for (auto& t : threads_) {
            if (t.joinable())
                t.join();
        }
    }

private:
    std::vector<std::thread>& threads_;
};
