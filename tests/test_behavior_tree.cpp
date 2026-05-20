#include "game/ai/behavior_tree.h"
#include "game/ai/blackboard.h"

#include <doctest/doctest.h>

#include <vector>

using namespace vector::game::ai;

namespace {

NodePtr leaf(const char* tag, Status result) {
    return std::make_unique<Action>(tag, [result](Blackboard&) { return result; });
}

}  // namespace

TEST_CASE("Selector returns first non-failing child") {
    Blackboard bb{};
    std::vector<NodePtr> kids;
    kids.push_back(leaf("a", Status::Failure));
    kids.push_back(leaf("b", Status::Success));
    kids.push_back(leaf("c", Status::Failure));
    Selector sel(std::move(kids));
    CHECK(sel.evaluate(bb) == Status::Success);
}

TEST_CASE("Selector returns Failure when no child succeeds") {
    Blackboard bb{};
    std::vector<NodePtr> kids;
    kids.push_back(leaf("a", Status::Failure));
    kids.push_back(leaf("b", Status::Failure));
    Selector sel(std::move(kids));
    CHECK(sel.evaluate(bb) == Status::Failure);
}

TEST_CASE("Sequence short-circuits on first failure") {
    Blackboard bb{};
    int ran = 0;
    std::vector<NodePtr> kids;
    kids.push_back(std::make_unique<Action>("a",
        [&](Blackboard&) { ++ran; return Status::Success; }));
    kids.push_back(std::make_unique<Action>("b",
        [&](Blackboard&) { ++ran; return Status::Failure; }));
    kids.push_back(std::make_unique<Action>("c",
        [&](Blackboard&) { ++ran; return Status::Success; }));
    Sequence seq(std::move(kids));
    CHECK(seq.evaluate(bb) == Status::Failure);
    CHECK(ran == 2);  // c is not reached
}

TEST_CASE("Inverter flips Success and Failure") {
    Blackboard bb{};
    Inverter ok(leaf("a", Status::Success));
    Inverter no(leaf("b", Status::Failure));
    CHECK(ok.evaluate(bb) == Status::Failure);
    CHECK(no.evaluate(bb) == Status::Success);
}
