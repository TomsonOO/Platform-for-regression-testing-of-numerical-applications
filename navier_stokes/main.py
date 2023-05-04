import sys
import numpy as np
import pandas as pd
from fenics import *

def solve_navier_stokes(T, num_steps, nu, degree, output_file):
    # Create mesh and define function space
    mesh = UnitSquareMesh(num_steps, num_steps)
    V = VectorFunctionSpace(mesh, "P", degree)

    # Define boundary condition
    u_D = Expression(("1.0", "0.0"), degree=degree)

    def boundary(x, on_boundary):
        return on_boundary

    bc = DirichletBC(V, u_D, boundary)

    # Define variational problem
    u = TrialFunction(V)
    v = TestFunction(V)
    f = Constant((0, 0))
    a = nu * inner(grad(u), grad(v)) * dx + inner(dot(grad(u), u), v) * dx
    L = dot(f, v) * dx

    # Compute solution
    u = Function(V)
    solve(a == L, u, bc)

    # Save solution to CSV file
    u_array = u.compute_vertex_values(mesh)
    u_x, u_y = np.split(u_array, 2)
    coordinates = mesh.coordinates()
    data = np.column_stack((coordinates, u_x, u_y))
    df = pd.DataFrame(data, columns=["x", "y", "u_x", "u_y"])
    df.to_csv(output_file, index=False)

if __name__ == "__main__":
    T = float(sys.argv[1])  # final time
    num_steps = int(sys.argv[2])  # number of time steps
    nu = float(sys.argv[3])  # kinematic viscosity
    degree = int(sys.argv[4])  # degree of finite elements
    output_file = sys.argv[5]  # output CSV file name

    solve_navier_stokes(T, num_steps, nu, degree, output_file)
