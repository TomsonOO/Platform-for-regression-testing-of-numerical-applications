import numpy as np
import csv


def solve_heat_equation_2d(Lx, Ly, Nx, Ny, T, Nt, D):
    dx = Lx / Nx
    dy = Ly / Ny
    dt = T / Nt

    # Set the boundary conditions
    T_left = 0.0
    T_right = 0.0
    T_top = 1.0
    T_bottom = 0.0

    # Initialize the temperature array
    T = np.zeros((Nx + 2, Ny + 2))  # add 2 for the boundary points
    T[:, 0] = T_bottom
    T[:, -1] = T_top
    T[0, :] = T_left
    T[-1, :] = T_right

    # Iterate over time steps
    for n in range(Nt):
        # Compute the new temperature array using finite differences
        T_new = np.zeros((Nx + 2, Ny + 2))
        for i in range(1, Nx + 1):
            for j in range(1, Ny + 1):
                T_new[i, j] = T[i, j] + D * dt / (dx ** 2) * (T[i + 1, j] - 2 * T[i, j] + T[i - 1, j]) + D * dt / (
                            dy ** 2) * (T[i, j + 1] - 2 * T[i, j] + T[i, j - 1])

        # Update the temperature array for the next time step
        T = T_new

    # Return the temperature array
    return T[1:-1, 1:-1]  # remove the boundary points


if __name__ == '__main__':
    # Example usage
    Lx = 1.0
    Ly = 1.0
    Nx = 10
    Ny = 10
    T = 1.0
    Nt = 100
    D = 0.1

    temperature_array = solve_heat_equation_2d(Lx, Ly, Nx, Ny, T, Nt, D)

    # Save the result to a CSV file
    with open('result.csv', 'w', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)
        for row in temperature_array:
            csv_writer.writerow(row)

    print(temperature_array)
