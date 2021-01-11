#include <functional>
#include <sstream>

#include <analyze/bounds.h>
#include <analyze/disambiguous.h>
#include <analyze/hash.h>
#include <analyze/linear.h>
#include <analyze/normalize_if.h>
#include <except.h>
#include <pass/simplify.h>

namespace ir {

void FindInnerMostScope::visit(const Var &op) {
    Visitor::visit(op);
    innerMost_ = std::max(innerMost_, varScope_.at(op->name_));
}

void FindInnerMostScope::visit(const Load &op) {
    Visitor::visit(op);
    innerMost_ = std::max(innerMost_, varScope_.at(op->var_));
}

int findInnerMostScope(const std::unordered_map<std::string, int> &varScope,
                       const Expr &op) {
    FindInnerMostScope visitor(varScope);
    visitor(op);
    return visitor.innnerMost();
}

uint64_t SimplifyPass::getHash(const Expr &op) {
    if (hash_.count(op.get())) { // maybe not, beacuse Mutator::visit
        return hash_.at(op.get());
    } else {
        return ::ir::getHash(op);
    }
}

Expr SimplifyPass::visit(const Div &_op) {
    auto __op = Mutator::visit(_op);
    ASSERT(__op->nodeType() == ASTNodeType::Div);
    auto op = __op.as<DivNode>();
    if (op->lhs_->nodeType() == ASTNodeType::IntConst &&
        op->rhs_->nodeType() == ASTNodeType::IntConst) {
        return makeIntConst(op->lhs_.as<IntConstNode>()->val_ /
                            op->rhs_.as<IntConstNode>()->val_);
    }
    return op;
}

Expr SimplifyPass::visit(const Mod &_op) {
    auto __op = Mutator::visit(_op);
    ASSERT(__op->nodeType() == ASTNodeType::Mod);
    auto op = __op.as<DivNode>();
    if (op->lhs_->nodeType() == ASTNodeType::IntConst &&
        op->rhs_->nodeType() == ASTNodeType::IntConst) {
        return makeIntConst(op->lhs_.as<IntConstNode>()->val_ %
                            op->rhs_.as<IntConstNode>()->val_);
    }
    return op;
}

Expr SimplifyPass::visit(const Min &op) {
    ASSERT(op->info_norm_form_.isValid());
    if (checkUpperCmp0(op, std::less_equal<int>())) {
        isFixPoint_ = false;
        return (*this)(op->lhs_);
    }
    if (checkLowerCmp0(op, std::greater_equal<int>())) {
        isFixPoint_ = false;
        return (*this)(op->rhs_);
    }
    return Mutator::visit(op);
}

Expr SimplifyPass::visit(const Max &op) {
    ASSERT(op->info_norm_form_.isValid());
    if (checkUpperCmp0(op, std::greater_equal<int>())) {
        isFixPoint_ = false;
        return (*this)(op->lhs_);
    }
    if (checkLowerCmp0(op, std::less_equal<int>())) {
        isFixPoint_ = false;
        return (*this)(op->rhs_);
    }
    return Mutator::visit(op);
}

Expr SimplifyPass::visit(const LT &op) {
    ASSERT(op->info_norm_form_.isValid());
    if (checkUpperCmp0(op, std::less<int>())) {
        isFixPoint_ = false;
        return makeIntConst(1);
    }
    if (checkLowerCmp0(op, std::greater_equal<int>())) {
        isFixPoint_ = false;
        return makeIntConst(0);
    }
    return Mutator::visit(op);
}

Expr SimplifyPass::visit(const LE &op) {
    ASSERT(op->info_norm_form_.isValid());
    if (checkUpperCmp0(op, std::less_equal<int>())) {
        isFixPoint_ = false;
        return makeIntConst(1);
    }
    if (checkLowerCmp0(op, std::greater<int>())) {
        isFixPoint_ = false;
        return makeIntConst(0);
    }
    return Mutator::visit(op);
}

Expr SimplifyPass::visit(const GT &op) {
    ASSERT(op->info_norm_form_.isValid());
    if (checkUpperCmp0(op, std::less_equal<int>())) {
        isFixPoint_ = false;
        return makeIntConst(0);
    }
    if (checkLowerCmp0(op, std::greater<int>())) {
        isFixPoint_ = false;
        return makeIntConst(1);
    }
    return Mutator::visit(op);
}

Expr SimplifyPass::visit(const GE &op) {
    ASSERT(op->info_norm_form_.isValid());
    if (checkUpperCmp0(op, std::less<int>())) {
        isFixPoint_ = false;
        return makeIntConst(0);
    }
    if (checkLowerCmp0(op, std::greater_equal<int>())) {
        isFixPoint_ = false;
        return makeIntConst(1);
    }
    return Mutator::visit(op);
}

Expr SimplifyPass::visit(const EQ &op) {
    ASSERT(op->info_norm_form_.isValid());
    if (checkUpperCmp0(op, std::less<int>())) {
        isFixPoint_ = false;
        return makeIntConst(0);
    }
    if (checkLowerCmp0(op, std::greater<int>())) {
        isFixPoint_ = false;
        return makeIntConst(0);
    }
    if (upper_.count(op->info_norm_form_.get()) &&
        lower_.count(op->info_norm_form_.get())) {
        for (auto &&upper : upper_.at(op->info_norm_form_.get())) {
            if (upper->nodeType() == ASTNodeType::IntConst &&
                upper.as<IntConstNode>()->val_ == 0) {
                for (auto &&lower : lower_.at(op->info_norm_form_.get())) {
                    if (lower->nodeType() == ASTNodeType::IntConst &&
                        lower.as<IntConstNode>()->val_ == 0) {
                        isFixPoint_ = false;
                        return makeIntConst(1);
                    }
                }
            }
        }
    }
    return Mutator::visit(op);
}

Expr SimplifyPass::visit(const NE &op) {
    ASSERT(op->info_norm_form_.isValid());
    if (checkUpperCmp0(op, std::less<int>())) {
        isFixPoint_ = false;
        return makeIntConst(1);
    }
    if (checkLowerCmp0(op, std::greater<int>())) {
        isFixPoint_ = false;
        return makeIntConst(1);
    }
    if (upper_.count(op->info_norm_form_.get()) &&
        lower_.count(op->info_norm_form_.get())) {
        for (auto &&upper : upper_.at(op->info_norm_form_.get())) {
            if (upper->nodeType() == ASTNodeType::IntConst &&
                upper.as<IntConstNode>()->val_ == 0) {
                for (auto &&lower : lower_.at(op->info_norm_form_.get())) {
                    if (lower->nodeType() == ASTNodeType::IntConst &&
                        lower.as<IntConstNode>()->val_ == 0) {
                        isFixPoint_ = false;
                        return makeIntConst(0);
                    }
                }
            }
        }
    }
    return Mutator::visit(op);
}

Stmt SimplifyPass::visit(const VarDef &_op) {
    if (varScope_.count(_op->name_)) {
        ERROR("Conflict var name: " + _op->name_ +
              ". Nested vars with the same name are not allowed");
    }
    varScope_[_op->name_] = curScope_++;
    auto op = Mutator::visit(_op);
    varScope_.erase(_op->name_), curScope_--;
    return op;
}

Stmt SimplifyPass::visit(const For &_op) {
    if (varScope_.count(_op->iter_)) {
        ERROR("iterators with the same name in nested loops are not allowed");
    }
    varScope_[_op->iter_] = curScope_++;
    auto op = Mutator::visit(_op);
    varScope_.erase(_op->iter_), curScope_--;
    return op;
}

Stmt SimplifyPass::visit(const If &_op) {
    auto __op = Mutator::visit(_op);
    ASSERT(__op->nodeType() == ASTNodeType::If);
    auto op = __op.as<IfNode>();
    if (op->cond_->nodeType() == ASTNodeType::IntConst) {
        isFixPoint_ = false;
        if (op->cond_.as<IntConstNode>()->val_) {
            return op->thenCase_;
        } else {
            return op->elseCase_;
        }
    }
    return op;
}

Stmt SimplifyPass::visit(const Assert &_op) {
    auto __op = Mutator::visit(_op);
    ASSERT(__op->nodeType() == ASTNodeType::Assert);
    auto op = __op.as<AssertNode>();
    if (op->cond_->nodeType() == ASTNodeType::IntConst) {
        isFixPoint_ = false;
        if (op->cond_.as<IntConstNode>()->val_) {
            return op->body_;
        } else {
            std::ostringstream os;
            // Print the unchanged _op
            os << "Assertion always false: " << _op;
            throw InvalidSchedule(os.str());
        }
    }
    return op;
}

Stmt simplifyPass(const Stmt &op) {
    return std::get<0>(simplifyAndGetBounds(op));
}

std::tuple<Stmt, SimplifyPass::BoundsMap, SimplifyPass::BoundsMap>
simplifyAndGetBounds(const Stmt &_op) {
    Stmt op = normalizeIf(_op);
    for (int i = 0;; i++) {
        op = Disambiguous()(op);

        auto hash = getHashMap(op);

        AnalyzeLinear analyzeLinear(hash);
        analyzeLinear(op);
        auto &&linear = analyzeLinear.result();

        AnalyzeBounds analyzeBounds(hash, linear);
        analyzeBounds(op);
        auto &&lower = analyzeBounds.lower();
        auto &&upper = analyzeBounds.upper();

        SimplifyPass simplifyVisitor(hash, lower, upper);
        auto newOp = simplifyVisitor(op);
        if (simplifyVisitor.isFixPoint() || i > 100) {
            if (i > 100) {
                WARNING("SimplifyPass iterates over 100 rounds. Maybe there is "
                        "a bug");
            }
            return {op, lower, upper}; // return the old op, or the lower /
                                       // upper will be invalid
        } else {
            op = newOp;
        }
    }
}

} // namespace ir

