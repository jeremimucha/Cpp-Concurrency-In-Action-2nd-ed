#include <chrono>
#include <iostream>
#include <vector>
#include <random>
#include "joining_thread.hpp"


template<typename Action>
class periodic_action {
public:
    periodic_action(Action action, int low, int high) noexcept
        : action_{std::move(action)},
          ud_{low, high}
    {
    }

    template<typename... Args>
    decltype(auto) operator()(Args&&... args)
    {
        for (auto i{0u}; i != 10; ++i) {
            auto const tick = ud_(re_);
            action_(std::forward<Args>(args)...);
            std::this_thread::sleep_for(std::chrono::milliseconds{tick});
        }
    }

private:
    Action action_;
    std::uniform_int_distribution<> ud_{};
    static inline std::default_random_engine re_{
        std::chrono::system_clock::now().time_since_epoch().count()
    };
};


int main()
{
    std::vector<joining_thread> threads{};

    for(auto i{0u}; i != 5; ++i) {
        threads.emplace_back(periodic_action{[]{
            std::cerr << "Hello from thread[" << std::this_thread::get_id() << "]\n";
        }, (i+1) * 42, (10-i) * 42});
    }

    // spawned threads will be joined on scope exit
}
