#include <autograd/grad.h>
#include <autograd/output_intermediates.h>
#include <ffi.h>

namespace freetensor {

using namespace pybind11::literals;

void init_ffi_autograd(py::module_ &m) {
    py::class_<StmtSetToUserGrad>(m, "StmtSetToUserGrad")
        .def(py::init<const std::unordered_set<ID> &, const Stmt &>())
        .def_readonly("ori_stmts", &StmtSetToUserGrad::oriStmts_)
        .def_readonly("bwd_body", &StmtSetToUserGrad::bwdBody_)
        .def("__str__",
             [](const StmtSetToUserGrad &userGrad) {
                 return "Backward of statements in {" +
                        toString(userGrad.oriStmts_) + "} is " +
                        toString(userGrad.bwdBody_);
             })
        .def("__repr__", [](const StmtSetToUserGrad &userGrad) {
            return "<StmtSeqToUserGrad {" + toString(userGrad.oriStmts_) +
                   "} " + toString(userGrad.bwdBody_) + ">";
        });

    py::enum_<GradTapeMode>(m, "GradTapeMode")
        .value("All", GradTapeMode::All)
        .value("Nothing", GradTapeMode::Nothing)
        .value("NoReuseOnly", GradTapeMode::NoReuseOnly);

    m.def(
        "grad_body",
        static_cast<
            std::tuple<Stmt, Stmt, std::unordered_map<std::string, std::string>,
                       std::unordered_map<std::string, std::string>,
                       std::unordered_map<ID, std::string>> (*)(
                const Stmt &, const std::unordered_set<std::string> &,
                const std::unordered_set<std::string> &,
                const std::unordered_set<ID> &,
                const std::vector<StmtSetToUserGrad> &)>(&gradBody),
        "func"_a, "requires"_a, "provides"_a, "tapes"_a,
        "user_grads"_a = std::vector<StmtSetToUserGrad>{});
    m.def(
        "grad_",
        static_cast<
            std::tuple<Func, Func, std::unordered_map<std::string, std::string>,
                       std::unordered_map<std::string, std::string>> (*)(
                const Func &, const std::unordered_set<std::string> &,
                const std::unordered_set<std::string> &,
                const std::unordered_set<ID> &, bool,
                const std::vector<StmtSetToUserGrad> &)>(&gradFuncInplace),
        "stmt"_a, "requires"_a, "provides"_a, "tapes"_a,
        "tape_in_closure"_a = true,
        "user_grads"_a = std::vector<StmtSetToUserGrad>{});
    m.def(
        "grad",
        static_cast<
            std::tuple<Func, Func, std::unordered_map<std::string, std::string>,
                       std::unordered_map<std::string, std::string>> (*)(
                const Func &, const std::unordered_set<std::string> &,
                const std::unordered_set<std::string> &,
                const std::unordered_set<ID> &, bool,
                const std::vector<StmtSetToUserGrad> &)>(&gradFuncOutOfPlace),
        "stmt"_a, "requires"_a, "provides"_a, "tapes"_a,
        "tape_in_closure"_a = true,
        "user_grads"_a = std::vector<StmtSetToUserGrad>{});

    m.def(
        "grad_body",
        static_cast<
            std::tuple<Stmt, Stmt, std::unordered_map<std::string, std::string>,
                       std::unordered_map<std::string, std::string>,
                       std::unordered_map<ID, std::string>> (*)(
                const Stmt &, const std::unordered_set<std::string> &,
                const std::unordered_set<std::string> &, GradTapeMode,
                const std::vector<StmtSetToUserGrad> &)>(&gradBody),
        "func"_a, "requires"_a, "provides"_a,
        "tape_mode"_a = GradTapeMode::NoReuseOnly,
        "user_grads"_a = std::vector<StmtSetToUserGrad>{});
    m.def(
        "grad_",
        static_cast<
            std::tuple<Func, Func, std::unordered_map<std::string, std::string>,
                       std::unordered_map<std::string, std::string>> (*)(
                const Func &, const std::unordered_set<std::string> &,
                const std::unordered_set<std::string> &, GradTapeMode, bool,
                const std::vector<StmtSetToUserGrad> &)>(&gradFuncInplace),
        "stmt"_a, "requires"_a, "provides"_a,
        "tape_mode"_a = GradTapeMode::NoReuseOnly, "tape_in_closure"_a = true,
        "user_grads"_a = std::vector<StmtSetToUserGrad>{});
    m.def(
        "grad",
        static_cast<
            std::tuple<Func, Func, std::unordered_map<std::string, std::string>,
                       std::unordered_map<std::string, std::string>> (*)(
                const Func &, const std::unordered_set<std::string> &,
                const std::unordered_set<std::string> &, GradTapeMode, bool,
                const std::vector<StmtSetToUserGrad> &)>(&gradFuncOutOfPlace),
        "stmt"_a, "requires"_a, "provides"_a,
        "tape_mode"_a = GradTapeMode::NoReuseOnly, "tape_in_closure"_a = true,
        "user_grads"_a = std::vector<StmtSetToUserGrad>{});

    py::enum_<OutputIntermediatesStage>(m, "OutputIntermediatesStage")
        .value("Forward", OutputIntermediatesStage::Forward)
        .value("Backward", OutputIntermediatesStage::Backward);

    // std::unordered_map<Load, Expr> cannot be exported to Python
    m.def(
        "output_intermediates",
        [](const Stmt &op, const std::unordered_set<ID> &intermediates,
           OutputIntermediatesStage stage, const std::string &varSuffix) {
            return std::get<0>(
                outputIntermediates(op, intermediates, stage, varSuffix));
        },
        "stmt"_a, "intermediates"_a,
        "stage"_a = OutputIntermediatesStage::Forward,
        "var_suffix"_a = ".tape");
}

} // namespace freetensor
