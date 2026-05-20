#pragma once

#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace vector::core {

// Minimal type-erased pub/sub. Subscribers are kept by shared_ptr to allow
// safe unsubscription via the returned token. Built for low-frequency
// gameplay events (e.g., death, pickup, mode change) — *not* for hot loops.
class EventBus {
public:
    // RAII handle that owns the subscriber's lifetime. While the
    // Subscription is alive, callbacks fire. When it goes out of scope,
    // the shared_ptr drops to 0, the bus's weak_ptr expires, and the bus
    // prunes the entry on the next publish.
    class Subscription {
    public:
        Subscription() = default;
        explicit Subscription(std::shared_ptr<void> handle) : handle_(std::move(handle)) {}
        bool valid() const { return static_cast<bool>(handle_); }
    private:
        std::shared_ptr<void> handle_;
        friend class EventBus;
    };

    template <typename Event, typename Fn>
    Subscription subscribe(Fn&& fn) {
        auto handler = std::make_shared<std::function<void(const Event&)>>(std::forward<Fn>(fn));
        auto& list = subscribers_[std::type_index(typeid(Event))];
        list.push_back(std::weak_ptr<void>(std::static_pointer_cast<void>(handler)));
        return Subscription(std::static_pointer_cast<void>(handler));
    }

    template <typename Event>
    void publish(const Event& evt) {
        auto it = subscribers_.find(std::type_index(typeid(Event)));
        if (it == subscribers_.end()) return;

        auto& list = it->second;
        for (std::size_t i = 0; i < list.size();) {
            if (auto sp = list[i].lock()) {
                auto* fn = static_cast<std::function<void(const Event&)>*>(sp.get());
                (*fn)(evt);
                ++i;
            } else {
                list[i] = list.back();
                list.pop_back();
            }
        }
    }

private:
    std::unordered_map<std::type_index, std::vector<std::weak_ptr<void>>> subscribers_;
};

}  // namespace vector::core
