#include <chrono>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>

struct dns_entry {
  std::string data;
};

class dns_cache {
 public:
  dns_entry find_entry(std::string const& domain) const {
    // using a std::shared_lock allows for concurrent read-only access to the
    // shared data acquiring this mutex is only possible if no thread holds an
    // exlusive-access mutex like std::lock_guard or std::unique_lock
    std::shared_lock<std::shared_mutex> lock{entry_mutex_};
    auto const it = entries_.find(domain);
    if (it != std::cend(entries_))
      return it->second;
    return {};
  }

  bool update_or_add_entry(std::string const& domain,
                           dns_entry const& dns_detail) {
    // acquire exlusive access to the shared data - this ensures that no other
    // thread will read or write to the shared structure while it's being
    // udpated.
    std::lock_guard<std::shared_mutex> lock{entry_mutex_};
    return entries_.insert_or_assign(domain, dns_detail).second;
  }

 private:
  std::unordered_map<std::string, dns_entry> entries_{};
  mutable std::shared_mutex entry_mutex_{};
};

int main() {
  dns_cache cache;
  cache.update_or_add_entry("foo", dns_entry{"foo_domain_detail"});
  cache.update_or_add_entry("bar", dns_entry{"bar_domain_detai"});
  cache.update_or_add_entry("baz", dns_entry{"baz_domain_detail"});

  std::mutex print_mutex{};
  auto const reader = [&cache, &print_mutex](auto&& domain) {
    for (auto i{0u}; i != 10; ++i) {
      auto const data = cache.find_entry(domain).data;
      std::lock_guard<std::mutex> lock{print_mutex};
      std::cerr << domain << " domain reader [" << std::this_thread::get_id()
                << "] got: " << data << "\n";
      std::this_thread::sleep_for(std::chrono::milliseconds{120});
    }
  };

  auto const writer = [&cache] {
    char const* const domains[]{"baz", "bar", "foo", "ni"};
    for (auto const& domain : domains) {
      std::this_thread::sleep_for(std::chrono::milliseconds{300});
      cache.update_or_add_entry(
          domain, dns_entry{std::string{"NEW "} + domain + " details"});
    }
  };

  // multiple threads access the cache frequently
  auto reader_foo = std::thread{[reader] { reader("foo"); }};
  auto reader_bar = std::thread{[reader] { reader("bar"); }};
  auto reader_baz = std::thread{[reader] { reader("baz"); }};

  // one thread updates the cache infrequently
  auto updater = std::thread{writer};

  reader_foo.join();
  reader_bar.join();
  reader_baz.join();
  updater.join();
}
