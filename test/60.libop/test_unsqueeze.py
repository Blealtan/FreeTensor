import torch
import numpy as np

import freetensor as ft
from freetensor import libop


def test_static():
    device = ft.CPU()

    @ft.optimize(device=device, verbose=1)
    def f(x, y):
        x: ft.Var[(3, 4, 5), "float32", "input", "cpu"]
        y: ft.Var[(3, 1, 4, 1, 5), "float32", "output", "cpu"]
        #! label: unsqueeze
        libop.unsqueeze_(x, y, axes=[1, 3])

    x_torch = torch.rand(3, 4, 5, dtype=torch.float32)
    x_arr = ft.Array(x_torch.numpy())
    y_torch = torch.zeros(3, 1, 4, 1, 5, dtype=torch.float32)
    y_arr = ft.Array(y_torch.numpy())
    f(x_arr, y_arr)
    y_torch = torch.tensor(y_arr.numpy())

    assert torch.all(torch.isclose(y_torch, x_torch.reshape(3, 1, 4, 1, 5)))


def test_out_of_place():
    device = ft.CPU()

    @ft.optimize(device=device, verbose=1)
    def f(x):
        x: ft.Var[(3, 4, 5), "float32", "input", "cpu"]
        #! label: unsqueeze
        return libop.unsqueeze(x, axes=[1, 3])

    x_torch = torch.rand(3, 4, 5, dtype=torch.float32)
    x_arr = ft.Array(x_torch.numpy())
    y_arr = f(x_arr)
    y_torch = torch.tensor(y_arr.numpy())

    assert np.array_equal(y_arr.shape, [3, 1, 4, 1, 5])
    assert torch.all(torch.isclose(y_torch, x_torch.reshape(3, 1, 4, 1, 5)))
