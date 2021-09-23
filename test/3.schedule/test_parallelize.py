import ir
import pytest

# For normal cases, see test/codegen


def test_unsolvable_dependency():
    with ir.VarDef("y", (5,), "int32", "output", "cpu") as y:
        with ir.For("i", 0, 4, nid="L1") as i:
            with ir.For("j", i, i + 2, nid="L2") as j:
                y[j] = i
    ast = ir.pop_ast()
    print(ast)
    s = ir.Schedule(ast)
    with pytest.raises(ir.InvalidSchedule):
        s.parallelize("L1", "openmp")
    ast_ = s.ast()  # Should not changed
    assert ast_.match(ast)


def test_not_found():
    with ir.VarDef("y", (4,), "int32", "output", "cpu") as y:
        with ir.For("i", 0, 4) as i:
            y[i] = i
    ast = ir.pop_ast()
    print(ast)
    s = ir.Schedule(ast)
    with pytest.raises(ir.InvalidSchedule):
        s.parallelize("L1", "openmp")
    ast_ = s.ast()  # Should not changed
    assert ast_.match(ast)


def test_no_deps():

    @ir.transform
    def test(ptr, edge1, edge2):
        ir.declare_var(ptr, (11,), "int32", "input", "cpu")
        ir.declare_var(edge1, (50,), "int32", "input", "cpu")
        ir.declare_var(edge2, (50,), "int32", "output", "cpu")
        'nid: Li'
        'no_deps'
        for i in range(10):
            for j in range(ptr[i], ptr[i + 1]):
                edge2[j] = edge1[j] + i

    print(test)
    s = ir.Schedule(test)
    s.parallelize("Li", "openmp")  # No exception here
    print(s.ast())
