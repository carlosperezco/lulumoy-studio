/**
 * @file registry.cpp
 * @brief Implementación del registro de operaciones.
 */
#include "cvcore/registry.h"

#include <algorithm>
#include <tuple>

namespace cvcore {

Registry& Registry::instance() {
    // Variable local estática: existe antes de que cualquier módulo se registre.
    static Registry registry;
    return registry;
}

void Registry::add(std::unique_ptr<Operation> op) {
    if (!op) return;
    std::string id = op->id();
    ops_[id] = std::move(op);
}

const Operation* Registry::find(const std::string& id) const {
    auto it = ops_.find(id);
    return it != ops_.end() ? it->second.get() : nullptr;
}

std::vector<const Operation*> Registry::all() const {
    std::vector<const Operation*> list;
    list.reserve(ops_.size());
    for (const auto& entry : ops_) list.push_back(entry.second.get());
    std::sort(list.begin(), list.end(), [](const Operation* a, const Operation* b) {
        return std::make_tuple(a->menu(), a->order(), a->id()) <
               std::make_tuple(b->menu(), b->order(), b->id());
    });
    return list;
}

} // namespace cvcore
