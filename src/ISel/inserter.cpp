#include "ISel/inserter.hpp"
#include "type.hpp"
#include "ISel/instruction.hpp"
#include "ISel/node.hpp"
#include "ISel/value.hpp"

namespace scbe::ISel {

Inserter::~Inserter() = default;

void Inserter::insert(std::unique_ptr<Node> node) {
    assert(m_root && "Root is not set");
    node->setRoot(m_root);
    if(auto ins = dyn_cast<Instruction>(node.get())) m_root->m_instructions.push_back(ins);
    m_root->m_nodes.push_back(std::move(node));
}

}