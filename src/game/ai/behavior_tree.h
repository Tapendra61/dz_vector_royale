#pragma once

#include <functional>
#include <memory>
#include <vector>

namespace vector::game::ai {

struct Blackboard;

enum class Status : unsigned char { Success, Failure, Running };

// Minimal tree: virtual `evaluate`. Trees are built once and reused by
// many bots — they hold *no per-bot state*. All bot state lives on the
// blackboard (which routes back to AIControllerComponent).
class Node {
public:
    virtual ~Node() = default;
    virtual Status evaluate(Blackboard& bb) = 0;
    virtual const char* name() const { return "?"; }
};

using NodePtr = std::unique_ptr<Node>;

// Composite that returns Success on the *first* child that succeeds.
// Models "try in priority order".
class Selector final : public Node {
public:
    explicit Selector(std::vector<NodePtr> children) : children_(std::move(children)) {}
    Status evaluate(Blackboard& bb) override;
    const char* name() const override { return "Selector"; }
private:
    std::vector<NodePtr> children_;
};

// Composite that returns Success only if *every* child succeeds in order.
class Sequence final : public Node {
public:
    explicit Sequence(std::vector<NodePtr> children) : children_(std::move(children)) {}
    Status evaluate(Blackboard& bb) override;
    const char* name() const override { return "Sequence"; }
private:
    std::vector<NodePtr> children_;
};

// Negates child status (Success<->Failure; Running passes through).
class Inverter final : public Node {
public:
    explicit Inverter(NodePtr child) : child_(std::move(child)) {}
    Status evaluate(Blackboard& bb) override;
    const char* name() const override { return "Inverter"; }
private:
    NodePtr child_;
};

// Action node — function returns the status directly.
class Action final : public Node {
public:
    using Fn = std::function<Status(Blackboard&)>;
    Action(const char* tag, Fn fn) : tag_(tag), fn_(std::move(fn)) {}
    Status evaluate(Blackboard& bb) override { return fn_(bb); }
    const char* name() const override { return tag_; }
private:
    const char* tag_;
    Fn          fn_;
};

// Condition node — passes Success if `pred(bb)` is true, else Failure.
class Condition final : public Node {
public:
    using Pred = std::function<bool(const Blackboard&)>;
    Condition(const char* tag, Pred pred) : tag_(tag), pred_(std::move(pred)) {}
    Status evaluate(Blackboard& bb) override { return pred_(bb) ? Status::Success : Status::Failure; }
    const char* name() const override { return tag_; }
private:
    const char* tag_;
    Pred        pred_;
};

}  // namespace vector::game::ai
