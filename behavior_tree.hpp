#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <stack>

enum class ENodeState
{
    Failure,
    Success,
    Running
};

enum class ENodeType
{
    Action,
    Sequence,
    Selector,
    Inverter,
    Condition,
    BlackBoard
};

class CNodeBase
{
public:
    CNodeBase(ENodeType type) : m_type(type) {}
    virtual ~CNodeBase() {}

    virtual ENodeState Execute() = 0;

    ENodeType GetType() const { return m_type; }

private:
    ENodeType m_type;
};

class CActionNode : public CNodeBase
{
public:
    CActionNode(std::string actionName, std::function<ENodeState()> action) : CNodeBase(ENodeType::Action), m_actionName(actionName), m_action(action) {}

    virtual ENodeState Execute() override
    {
        return m_action();
    }

private:
    std::string m_actionName;
    std::function<ENodeState()> m_action;
};

// ==========================
// Compositor
// ==========================

class CCompositeNode : public CNodeBase
{
public:
    CCompositeNode(ENodeType type) : CNodeBase(type) {}

    void AddChild(std::unique_ptr<CNodeBase> child)
    {
        m_children.push_back(std::move(child));
    }

    const std::vector<std::unique_ptr<CNodeBase>> &GetChildren() const { return m_children; }

private:
    std::vector<std::unique_ptr<CNodeBase>> m_children;
};

class CSequenceNode : public CCompositeNode
{
public:
    CSequenceNode() : CCompositeNode(ENodeType::Sequence) {}

    ENodeState Execute() override
    {
        for (const auto &child : GetChildren())
        {
            ENodeState state = child->Execute();
            if (state == ENodeState::Failure)
            {
                return ENodeState::Failure;
            }
            else if (state == ENodeState::Running)
            {
                return ENodeState::Running;
            }
        }

        return ENodeState::Success;
    }
};

class CSelectorNode : public CCompositeNode
{
public:
    CSelectorNode() : CCompositeNode(ENodeType::Selector) {}

    ENodeState Execute() override
    {
        for (const auto &child : GetChildren())
        {
            ENodeState state = child->Execute();
            if (state == ENodeState::Success)
            {
                return ENodeState::Success;
            }
            else if (state == ENodeState::Running)
            {
                return ENodeState::Running;
            }
        }
        return ENodeState::Failure;
    }
};

// ==========================
// Decorator
// ==========================

class CDecoratorNode : public CNodeBase
{
public:
    CDecoratorNode(ENodeType type) : CNodeBase(type) {}

    virtual void SetChild(std::unique_ptr<CNodeBase> child) { m_child = std::move(child); }
    const std::unique_ptr<CNodeBase> &GetChild() const { return m_child; }

private:
    std::unique_ptr<CNodeBase> m_child;
};

class CInverterNode : public CDecoratorNode
{
public:
    CInverterNode() : CDecoratorNode(ENodeType::Inverter) {}

    ENodeState Execute() override
    {
        ENodeState state = GetChild()->Execute();
        if (state == ENodeState::Success)
        {
            return ENodeState::Failure;
        }
        else if (state == ENodeState::Failure)
        {
            return ENodeState::Success;
        }

        return state;
    }
};

class CConditionNode : public CDecoratorNode
{
public:
    CConditionNode(std::function<bool()> condition) : CDecoratorNode(ENodeType::Condition), m_condition(condition) {}

    ENodeState Execute() override
    {
        return m_condition() ? ENodeState::Success : ENodeState::Failure;
    }

private:
    std::function<bool()> m_condition;
};

// ==========================
// BlackBoard Node
// ==========================
enum class EBlackBoardType
{
    Bool,
    Int,
    Double,
    String
};

class CBlackBoardNode
{
public:
    CBlackBoardNode(const std::string &key, EBlackBoardType type) : m_key(key), m_type(type) {}
    virtual void SetValue(void *value) = 0;
    virtual void *GetValue() const = 0;
    const std::string &GetKey() const { return m_key; }

protected:
    std::string m_key;
    EBlackBoardType m_type;
};

class CIntNode : public CBlackBoardNode
{
public:
    CIntNode(const std::string &key) : CBlackBoardNode(key, EBlackBoardType::Int), m_value(0) {}
    CIntNode(const std::string &key, int value) : CBlackBoardNode(key, EBlackBoardType::Int), m_value(value) {}

    void SetValue(void *value) override
    {
        m_value = *static_cast<int *>(value);
    }

    void *GetValue() const override
    {
        return const_cast<int *>(&m_value);
    }

private:
    int m_value;
};

class CDoubleNode : public CBlackBoardNode
{
public:
    CDoubleNode(const std::string &key) : CBlackBoardNode(key, EBlackBoardType::Double), m_value(0.0) {}
    CDoubleNode(const std::string &key, double value) : CBlackBoardNode(key, EBlackBoardType::Double), m_value(value) {}

    void SetValue(void *value) override
    {
        m_value = *static_cast<double *>(value);
    }

    void *GetValue() const override
    {
        return const_cast<double *>(&m_value);
    }

private:
    double m_value;
};

class CStringNode : public CBlackBoardNode
{

public:
    CStringNode(const std::string &key) : CBlackBoardNode(key, EBlackBoardType::String), m_value("") {}
    CStringNode(const std::string &key, std::string value) : CBlackBoardNode(key, EBlackBoardType::String), m_value(value) {}

    void SetValue(void *value) override
    {
        m_value = *static_cast<std::string *>(value);
    }

    void *GetValue() const override
    {
        return const_cast<std::string *>(&m_value);
    }

private:
    std::string m_value;
};

class CBoolNode : public CBlackBoardNode
{
public:
    CBoolNode(const std::string &key) : CBlackBoardNode(key, EBlackBoardType::Bool), m_value(false) {}
    CBoolNode(const std::string &key, bool value) : CBlackBoardNode(key, EBlackBoardType::Bool), m_value(value) {}

    void SetValue(void *value) override
    {
        m_value = *static_cast<bool *>(value);
    }

    void *GetValue() const override
    {
        return const_cast<bool *>(&m_value);
    }

private:
    bool m_value;
};

// ==========================
// BlackBoarder
// ==========================

class CBlackBoard
{
public:
    CBlackBoard() = default;
    ~CBlackBoard() = default;

    template <typename T>
    void SetNode(const std::string &key, void *value)
    {
        auto it = m_nodes.find(key);
        if (it != m_nodes.end())
        {
            it->second->SetValue(value);
        }
        else
        {
            auto node = std::make_unique<T>(key, value);
            m_nodes[key] = std::move(node);
        }
    }

    template <typename T>
    T* GetNode(const std::string &key)
    {
        auto it = m_nodes.find(key);
        if (it != m_nodes.end())
        {
            return dynamic_cast<T*>(it->second.get());
        }
        else
        {
            std::unique_ptr<T> node = std::make_unique<T>(key);
            m_nodes[key] = std::move(node);
            return dynamic_cast<T*>(m_nodes[key].get());
        }
    }

private:
    std::unordered_map<std::string, std::unique_ptr<CBlackBoardNode>> m_nodes;
};


// ==========================
// Behavior Tree
// ==========================

class CBehaviorTree
{
public:
    CBehaviorTree(){};
    ~CBehaviorTree(){};

    void SetRoot(std::shared_ptr<CNodeBase> root)
    {
        m_root = root;
    }

    CBlackBoard& GetBlackBoard()
    {
        return m_blackboard;
    }

    ENodeState Execute()
    {
        return m_root->Execute();
    }

private:
    std::shared_ptr<CNodeBase> m_root;
    CBlackBoard m_blackboard;
};

// ==========================
// Builder
// ==========================
template <class Builder>
class CDecoratorBuilder;

template <class Builder>
class CCompositeBuilder
{
private:
    Builder *builder_;
    CCompositeNode *m_Node;

public:
    CCompositeBuilder(Builder *builder, CCompositeNode *node) : builder_(builder), m_Node(node) {}

    template <class NodeType, typename... Args>
    CCompositeBuilder<Builder> &Action(Args... args)
    {
        auto child = std::make_unique<NodeType>((args)...);
        m_Node->AddChild(std::move(child));
        return *this;
    }

    template <class CompositeType, typename... Args>
    CCompositeBuilder<CCompositeBuilder<Builder>> Composite(Args... args)
    {
        auto child = std::make_unique<CompositeType>((args)...);
        m_Node->AddChild(std::move(child));
        return CCompositeBuilder<CCompositeBuilder<Builder>>(this, (CompositeType *)m_Node->GetChildren().back().get());
    }

    template <class DecoratorType, typename... Args>
    CDecoratorBuilder<CCompositeBuilder<Builder>> Decorator(Args... args)
    {
        auto child = std::make_unique<DecoratorType>((args)...);
        m_Node->AddChild(std::move(child));
        return CDecoratorBuilder<CCompositeBuilder<Builder>>(this, (DecoratorType *)m_Node->GetChildren().back().get());
    }

    Builder &End()
    {
        return *builder_;
    }
};

// 노드 데코레이터 빌더 클래스
template <class Builder>
class CDecoratorBuilder
{
private:
    Builder *builder_;
    CDecoratorNode *node_;

public:
    CDecoratorBuilder(Builder *builder, CDecoratorNode *node) : builder_(builder), node_(node) {}

    template <class NodeType, typename... Args>
    CDecoratorBuilder<Builder> &Child(Args... args)
    {
        auto child = std::make_unique<NodeType>((args)...);
        node_->SetChild(std::move(child));
        return *this;
    }

    template <class CompositeType, typename... Args>
    CCompositeBuilder<CDecoratorBuilder<Builder>> Composite(Args... args)
    {
        auto child = std::make_unique<CompositeType>((args)...);
        node_->SetChild(std::move(child));
        return CCompositeBuilder<CDecoratorBuilder<Builder>>(this, (CompositeType *)node_->GetChild().get());
    }

    template <class DecoratorType, typename... Args>
    CDecoratorBuilder<CDecoratorBuilder<Builder>> Decorator(Args... args)
    {
        auto child = std::make_unique<DecoratorType>((args)...);
        node_->SetChild(std::move(child));
        return CDecoratorBuilder<CDecoratorBuilder<Builder>>(this, (DecoratorType *)node_->GetChild().get());
    }

    Builder &End()
    {
        return *builder_;
    }
};

// 행동 트리 빌더 클래스
class CBehaviorTreeBuilder
{
private:
    std::shared_ptr<CNodeBase> root_;

public:
    template <class NodeType, typename... Args>
    CBehaviorTreeBuilder &Action(Args... args)
    {
        auto node = std::make_shared<NodeType>((args)...);
        root_ = node;
        return *this;
    }

    template <class CompositeType, typename... Args>
    CCompositeBuilder<CBehaviorTreeBuilder> Composite(Args... args)
    {
        auto node = std::make_shared<CompositeType>((args)...);
        root_ = node;
        return CCompositeBuilder<CBehaviorTreeBuilder>(this, (CompositeType *)root_.get());
    }

    template <class DecoratorType, typename... Args>
    CDecoratorBuilder<CBehaviorTreeBuilder> Decorator(Args... args)
    {
        auto node = std::make_shared<DecoratorType>((args)...);
        root_ = node;
        return CDecoratorBuilder<CBehaviorTreeBuilder>(this, (DecoratorType *)root_.get());
    }

    std::shared_ptr<CBehaviorTree> Build()
    {
        assert(root_ != nullptr && "The Behavior Tree is empty!");
        std::shared_ptr<CBehaviorTree> tree = std::make_shared<CBehaviorTree>();
        tree->SetRoot(root_);
        return tree;
    }
};
