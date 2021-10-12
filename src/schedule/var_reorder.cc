#include <analyze/all_reads.h>
#include <analyze/all_writes.h>
#include <schedule/var_reorder.h>

namespace ir {

Stmt VarReorder::visit(const VarDef &_op) {
    if (_op->id() == def_) {
        if (_op->buffer_->atype() != AccessType::Cache) {
            throw InvalidSchedule("Reorder on an I/O variable is not allowed");
        }

        found_ = true;

        var_ = _op->name_;
        auto __op = Mutator::visit(_op);
        ASSERT(__op->nodeType() == ASTNodeType::VarDef);
        auto op = __op.as<VarDefNode>();
        var_.clear();

        std::vector<Expr> shape;
        shape.reserve(order_.size());
        ASSERT(order_.size() == op->buffer_->tensor().shape().size());
        for (size_t i = 0, n = order_.size(); i < n; i++) {
            shape.emplace_back(op->buffer_->tensor().shape()[order_[i]]);
        }
        op->buffer_->tensor().setShape(shape);
        return op;
    } else {
        return Mutator::visit(_op);
    }
}

Stmt VarReorder::visit(const Store &_op) {
    auto __op = Mutator::visit(_op);
    ASSERT(__op->nodeType() == ASTNodeType::Store);
    auto op = __op.as<StoreNode>();
    return reorderMemAcc(op);
}

Stmt VarReorder::visit(const ReduceTo &_op) {
    auto __op = Mutator::visit(_op);
    ASSERT(__op->nodeType() == ASTNodeType::ReduceTo);
    auto op = __op.as<ReduceToNode>();
    return reorderMemAcc(op);
}

Expr VarReorder::visit(const Load &_op) {
    auto __op = Mutator::visit(_op);
    ASSERT(__op->nodeType() == ASTNodeType::Load);
    auto op = __op.as<LoadNode>();
    return reorderMemAcc(op);
}

Stmt VarReorder::visit(const MatMul &op) {
    if (!var_.empty() && (allReads(op->equivalent_).count(var_) ||
                          allWrites(op->equivalent_).count(var_))) {
        throw InvalidSchedule("Please call var_reorder before as_matmul");
    }
    return Mutator::visit(op);
}

Stmt varReorder(const Stmt &_ast, const std::string &def,
                const std::vector<int> &order) {
    VarReorder mutator(def, order);
    auto ast = mutator(_ast);
    if (!mutator.found()) {
        throw InvalidSchedule(def + " not found");
    }
    return ast;
}

} // namespace ir

