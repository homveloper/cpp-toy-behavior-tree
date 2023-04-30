#include <iostream>
#include "behavior_tree.hpp"

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
    .End()
    .Build();

    Tree->Execute();    
}
