#include "BehaviorTree.hpp"

#include <algorithm>

// --- SelectorNode ----------------------------------------------------------

SelectorNode::SelectorNode(std::vector<BTNodePtr> children)
    : m_children(std::move(children)) {}

BTResult SelectorNode::tick(const BTContext& context) const {
    // First child that succeeds wins; its command is the selector's command.
    for (const BTNodePtr& child : m_children) {
        BTResult result = child->tick(context);
        if (result.success) {
            return result;
        }
    }
    return BTResult{};   // every child failed: failure, no command
}

// --- SequenceNode ----------------------------------------------------------

SequenceNode::SequenceNode(std::vector<BTNodePtr> children)
    : m_children(std::move(children)) {}

BTResult SequenceNode::tick(const BTContext& context) const {
    BTResult result{ true, "" };   // an empty sequence succeeds
    for (const BTNodePtr& child : m_children) {
        BTResult childResult = child->tick(context);
        if (!childResult.success) {
            return BTResult{};   // first failing child: failure, no command
        }
        if (!childResult.command.empty()) {
            result.command = std::move(childResult.command);   // keep the LAST non-empty one
        }
    }
    return result;
}

// --- ConditionNode ---------------------------------------------------------

ConditionNode::ConditionNode(std::string name)
    : m_name(std::move(name)) {}

BTResult ConditionNode::tick(const BTContext& context) const {
    // A missing fact counts as false, exactly like a fact with value 0.
    const bool found = std::any_of(context.begin(), context.end(),
        [this](const std::pair<std::string, int>& fact) {
            return fact.first == m_name && fact.second != 0;
        });
    return BTResult{ found, "" };
}

// --- ActionNode ------------------------------------------------------------

ActionNode::ActionNode(std::string command)
    : m_command(std::move(command)) {}

BTResult ActionNode::tick(const BTContext& /*context*/) const {
    return BTResult{ true, m_command };
}

// --- BehaviorTree ----------------------------------------------------------

BehaviorTree::BehaviorTree(BTNodePtr root)
    : m_root(std::move(root)) {}

std::string BehaviorTree::tick(const BTContext& context) const {
    if (!m_root) {
        return "";   // no tree: no command
    }
    BTResult result = m_root->tick(context);
    return result.success ? result.command : "";
}
