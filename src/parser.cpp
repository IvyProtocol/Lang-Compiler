#include "../include/Parser.hpp"
#include <print>

ASTNode::~ASTNode() = default;

LiteralASTNode::LiteralASTNode(std::string_view val) : value(std::move(val)) {}

StubASTNode::StubASTNode(std::string_view n) : name(n) {}

VariableExprASTNode::VariableExprASTNode(std::string_view n) : name(n) {}

VarDeclASTNode::VarDeclASTNode(std::string_view id, TypeSpecifier t,
                               std::unique_ptr<ASTNode> init)
    : identifier(id), type(t), initializer(std::move(init)) {}

AssignmentASTNode::AssignmentASTNode(std::string_view name,
        std::unique_ptr<ASTNode> val)
: variable_name(name), new_value(std::move(val)) {}

CallASTNode::CallASTNode(std::string_view name,
        std::vector<std::unique_ptr<ASTNode>> args)
: call(name), arguments(std::move(args)) {}

ArrayLiteralASTNode::ArrayLiteralASTNode(std::vector<std::unique_ptr<ASTNode>> elems)
    : elements(std::move(elems)) {}

BinaryExprASTNode::BinaryExprASTNode(std::unique_ptr<ASTNode> l, Token o, std::unique_ptr<ASTNode> r)
    : left(std::move(l)), op(o), right(std::move(r)) {}

UnaryExprASTNode::UnaryExprASTNode(std::unique_ptr<ASTNode> opnd, Token o)
    : operand(std::move(opnd)), op(o) {}

void LiteralASTNode::debug_print() const {
  std::print("[Literal: {}]", value);
}

void StubASTNode::debug_print() const { std::println("\n [Stub: {}]", name); }

void VariableExprASTNode::debug_print() const {
    std::print("[Variable: {}]", name);
};

void VarDeclASTNode::debug_print() const {
  std::print("[VarDecl] Type: {}{}, Name: {}, Array-Size: {}", type.is_const ? "const " : "",
               type.base_types, identifier, type.is_array);

  if (initializer) {
    std::print(" := ");
    initializer->debug_print();
  }
  std::println(";");
}

void AssignmentASTNode::debug_print() const {
  std::print("[Assignment] {} := ", variable_name);
  if (new_value) {
    new_value->debug_print();
  }
  std::println(";");
}

void CallASTNode::debug_print() const {
  std::print("[Call: {}(", call);
  for (size_t i {}; i < arguments.size(); i++) {
    arguments[i] -> debug_print();
    if (i < arguments.size() - 1) std::print(", ");
  }
  std::print(")]");
}

void ArrayLiteralASTNode::debug_print() const {
  std::print("[ArrayLiteral: ");
  for ( size_t i {}; i < elements.size(); i++ ) {
    elements[i] -> debug_print();
    if ( i < elements.size() - 1 ) std::print(", ");
  }
  std::print("]");
}

void BinaryExprASTNode::debug_print() const {
    std::print("(");
    if (left) {
        left->debug_print();
    }

    std::print(" {} ", op.value);

    if (right) {
        right->debug_print();
    }
    std::print(")");
}

void UnaryExprASTNode::debug_print() const {
    std::print("({}", op.value);
    if (operand) operand->debug_print();
    std::print(")");
}
