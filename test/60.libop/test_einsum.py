import torch
import numpy as np

import freetensor as ft
from freetensor import libop


def test_basic():
    device = ft.CPU()

    @ft.optimize(device=device, verbose=1)
    def f(a, b, y):
        a: ft.Var[(4, 5), "float32", "input", "cpu"]
        b: ft.Var[(5,), "float32", "input", "cpu"]
        y: ft.Var[(4,), "float32", "output", "cpu"]
        #! label: einsum
        libop.einsum_("ij,j->i", a, b, y)

    a_torch = torch.rand(4, 5, dtype=torch.float32)
    a_arr = ft.Array(a_torch.numpy())
    b_torch = torch.rand(5, dtype=torch.float32)
    b_arr = ft.Array(b_torch.numpy())
    y_torch = torch.zeros(4, dtype=torch.float32)
    y_arr = ft.Array(y_torch.numpy())
    f(a_arr, b_arr, y_arr)
    y_torch = torch.tensor(y_arr.numpy())

    y_std = torch.einsum("ij,j->i", a_torch, b_torch)
    assert torch.all(torch.isclose(y_torch, y_std))


def test_broadcast():
    device = ft.CPU()

    @ft.optimize(device=device, verbose=1)
    def f(a, b, y):
        a: ft.Var[(4, 1), "float32", "input", "cpu"]
        b: ft.Var[(5,), "float32", "input", "cpu"]
        y: ft.Var[(4,), "float32", "output", "cpu"]
        #! label: einsum
        libop.einsum_("ij,j->i", a, b, y)

    a_torch = torch.rand(4, 1, dtype=torch.float32)
    a_arr = ft.Array(a_torch.numpy())
    b_torch = torch.rand(5, dtype=torch.float32)
    b_arr = ft.Array(b_torch.numpy())
    y_torch = torch.zeros(4, dtype=torch.float32)
    y_arr = ft.Array(y_torch.numpy())
    f(a_arr, b_arr, y_arr)
    y_torch = torch.tensor(y_arr.numpy())

    y_std = torch.einsum("ij,j->i", a_torch, b_torch)
    assert torch.all(torch.isclose(y_torch, y_std))


def test_out_of_place():
    device = ft.CPU()

    @ft.optimize(device=device, verbose=1)
    def f(a, b):
        a: ft.Var[(4, 5), "float32", "input", "cpu"]
        b: ft.Var[(5,), "float32", "input", "cpu"]
        #! label: gemm
        return libop.einsum("ij,j->i", a, b)

    a_torch = torch.rand(4, 5, dtype=torch.float32)
    a_arr = ft.Array(a_torch.numpy())
    b_torch = torch.rand(5, dtype=torch.float32)
    b_arr = ft.Array(b_torch.numpy())
    y_arr = f(a_arr, b_arr)
    y_torch = torch.tensor(y_arr.numpy())

    y_std = torch.einsum("ij,j->i", a_torch, b_torch)
    assert np.array_equal(y_arr.shape, [4])
    assert torch.all(torch.isclose(y_torch, y_std))
