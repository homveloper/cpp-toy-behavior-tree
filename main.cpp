#include <iostream>
#include "behavior_tree.hpp"
#include "BrainTree.hpp"

class Action : public BrainTree::Node {
public:
    Action(std::string name, std::function<Node::Status()> func) :  func(func) {}

    virtual Node::Status update() override {
        return func();
    }

private:
    std::function<Node::Status()> func;
};

int main(int, char**) {

    std::shared_ptr<CBehaviorTree> Tree = CBehaviorTreeBuilder()
    .Composite<CSequenceNode>()
        .Action<CActionNode>("Action2", []() {
            std::cout << "Action2" << std::endl;
            return ENodeState::Success;
        })
        .Action<CActionNode>("Action3", []() {
            std::cout << "Action3" << std::endl;
            return ENodeState::Success;
        })
        .Composite<CSelectorNode>()
            .Action<CActionNode>("Action4", []() {
                std::cout << "Action4" << std::endl;
                return ENodeState::Failure;
            })
            .Action<CActionNode>("Action5", []() {
                std::cout << "Action5" << std::endl;
                return ENodeState::Failure;
            })
        .End()
        .Action<CActionNode>("Action6", []() {
            std::cout << "Action6" << std::endl;
            return ENodeState::Success;
        })
        .Decorator<CInverterNode>()
            .Action<CActionNode>("Action6", []() {
                std::cout << "Action6" << std::endl;
                return ENodeState::Success;
            })
            .Action<CActionNode>("Action7", []() {
                std::cout << "Action7" << std::endl;
                return ENodeState::Success;
            })
        .End()
    .End()
    .Build();

    Tree->Execute();    


    auto Tree2 = BrainTree::Builder()
                    .composite<BrainTree::Sequence>()
                        .leaf<Action>("Action2", []() {
                            std::cout << "Action21" << std::endl;
                            return BrainTree::Node::Status::Success;
                        })
                        .leaf<Action>("Action3", []() {
                            std::cout << "Action31" << std::endl;
                            return BrainTree::Node::Status::Success;
                        })
                        .composite<BrainTree::Selector>()
                            .leaf<Action>("Action4", []() {
                                std::cout << "Action41" << std::endl;
                                return BrainTree::Node::Status::Failure;
                            })
                            .leaf<Action>("Action5", []() {
                                std::cout << "Action51" << std::endl;
                                return BrainTree::Node::Status::Failure;
                            })
                            .decorator<BrainTree::Inverter>()
                                .leaf<Action>("Action6", []() {
                                    std::cout << "Action61" << std::endl;
                                    return BrainTree::Node::Status::Failure;
                                })
                            .end()
                            .leaf<Action>("Action7", []() {
                                std::cout << "Action71" << std::endl;
                                return BrainTree::Node::Status::Success;
                            })
                        .end()
                    .end()
                    .build();

    Tree2->update();
}
