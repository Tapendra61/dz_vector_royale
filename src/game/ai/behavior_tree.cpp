#include "game/ai/behavior_tree.h"

namespace vector::game::ai {

Status Selector::evaluate(Blackboard& bb) {
    for (auto& c : children_) {
        const auto s = c->evaluate(bb);
        if (s == Status::Success || s == Status::Running) return s;
    }
    return Status::Failure;
}

Status Sequence::evaluate(Blackboard& bb) {
    for (auto& c : children_) {
        const auto s = c->evaluate(bb);
        if (s != Status::Success) return s;
    }
    return Status::Success;
}

Status Inverter::evaluate(Blackboard& bb) {
    const auto s = child_->evaluate(bb);
    if (s == Status::Success) return Status::Failure;
    if (s == Status::Failure) return Status::Success;
    return s;
}

}  // namespace vector::game::ai
