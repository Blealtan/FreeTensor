import torch
import numpy as np

import freetensor as ft
from freetensor import libop


def test_static():
    device = ft.CPU()

    @ft.optimize(device=device, verbose=1)
    def f(x, y):
        x: ft.Var[(3, 1), "float32", "input", "cpu"]
        y: ft.Var[(3, 5), "float32", "output", "cpu"]
        #! label: expand
        libop.expand_(x, y)

    x_torch = torch.rand(3, 1, dtype=torch.float32)
    x_arr = ft.Array(x_torch.numpy())
    y_torch = torch.zeros(3, 5, dtype=torch.float32)
    y_arr = ft.Array(y_torch.numpy())
    f(x_arr, y_arr)
    y_torch = torch.tensor(y_arr.numpy())

    assert torch.all(torch.isclose(y_torch, x_torch.expand(3, 5)))


def test_out_of_place():
    device = ft.CPU()

    @ft.optimize(device=device, verbose=1)
    def f(x):
        x: ft.Var[(3, 1), "float32", "input", "cpu"]
        #! label: expand
        return libop.expand(
            x, ft.capture_var(ft.Array(np.array([3, 5], dtype=np.int32))))

    x_torch = torch.rand(3, 1, dtype=torch.float32)
    x_arr = ft.Array(x_torch.numpy())
    y_arr = f(x_arr)
    y_torch = torch.tensor(y_arr.numpy())

    assert np.array_equal(y_arr.shape, [3, 5])
    assert torch.all(torch.isclose(y_torch, x_torch.expand(3, 5)))
