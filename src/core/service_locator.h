#pragma once

#include <memory>
#include <typeindex>
#include <unordered_map>

namespace vector::core {

// Context object passed explicitly through game systems — *not* a global
// singleton (roadmap §3.3). Holds non-owning pointers to long-lived
// services. Lifetime is owned by `main`; systems borrow.
class ServiceLocator {
public:
    template <typename T>
    void provide(T* service) {
        services_[std::type_index(typeid(T))] = service;
    }

    template <typename T>
    T* get() const {
        auto it = services_.find(std::type_index(typeid(T)));
        return it == services_.end() ? nullptr : static_cast<T*>(it->second);
    }

    template <typename T>
    T& require() const {
        auto* p = get<T>();
        // System invariant: requested service must be registered.
        return *p;
    }

private:
    std::unordered_map<std::type_index, void*> services_;
};

}  // namespace vector::core
