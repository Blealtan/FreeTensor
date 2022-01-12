#ifndef COUNT_CONTIG_ACCESS_LOOPS_H
#define COUNT_CONTIG_ACCESS_LOOPS_H

#include <unordered_map>

#include <analyze/analyze_linear.h>
#include <analyze/symbol_table.h>
#include <visitor.h>

namespace ir {

class CountContigAccessLoops : public SymbolTable<Visitor> {
    typedef SymbolTable<Visitor> BaseClass;

    std::unordered_map<std::string, std::pair<int64_t, int>>
        counts_; // for ID -> (count, -depth)
    std::unordered_map<std::string, For> var2for_;
    AnalyzeLinear analyzeLinear_;
    int64_t repeat_ = 1;
    int depth_ = 0;

  public:
    const std::unordered_map<std::string, std::pair<int64_t, int>> &
    counts() const {
        return counts_;
    }

  private:
    int64_t getStaticSize(const std::string &var) {
        int64_t ret = 1;
        for (auto &&dim : buffer(var)->tensor().shape()) {
            if (dim->nodeType() == ASTNodeType::IntConst) {
                ret *= dim.as<IntConstNode>()->val_;
            } else {
                return -1;
            }
        }
        return ret;
    }

    template <class T> void visitMemAccess(const T &op) {
        BaseClass::visit(op);
        auto size = getStaticSize(op->var_);
        if (size != -1 && size < 128) {
            // We don't count too small vars here because they are likely
            // registers
            return;
        }
        if (!op->indices_.empty()) {
            auto idx = op->indices_.back();
            analyzeLinear_(idx);
            for (auto &&[h, s] : analyzeLinear_.result().at(idx).coeff_) {
                if ((s.k_ == 1 || s.k_ == -1) &&
                    s.a_->nodeType() == ASTNodeType::Var) {
                    auto &&var = s.a_.template as<VarNode>();
                    counts_[var2for_.at(var->name_)->id()].first += repeat_;
                }
            }
        }
    }

  protected:
    using BaseClass::visit;
    void visit(const For &op) override;
    void visit(const Load &op) override { visitMemAccess(op); }
    void visit(const Store &op) override { visitMemAccess(op); }
    void visit(const ReduceTo &op) override { visitMemAccess(op); }
    void visit(const MatMul &op) override {} // do nothing
};

} // namespace ir

#endif // COUNT_CONTIG_ACCESS_LOOPS_H
