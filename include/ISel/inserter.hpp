#pragma once

namespace scbe::ISel {

class Root;
class Chain;
class FrameIndex;
class Register;
class Value;
class Node;

class Inserter {
public:
    ~Inserter();

    Root* getRoot() { return m_root; }
    void setRoot(Root* root) { m_root = root; }
    void insert(std::unique_ptr<Node> node);

private:
    Root* m_root = nullptr;
};

}