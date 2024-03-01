import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import find_peaks

# notes:
# polynomial coefficients go from highest to lowest

def generate_chebyshev_nodes(min, max, count):
    ks = np.arange(1, count + 1, dtype=np.float64)
    return 0.5 * (min + max) + 0.5 * (max - min) * np.cos((2 * ks - 1) * np.pi / (2 * count))

# find polynomial satisfying the osciallation criteria
# returns coefficients of the polynomial and the error term.
def find_polynomial(nodes, function):
    node_count = len(nodes)
    a = np.vander(nodes, node_count - 1)
    sign_alternating_ones = np.empty((node_count, 1))
    sign_alternating_ones[::2, 0] = -1
    sign_alternating_ones[1::2, 0] = 1
    a = np.append(a, sign_alternating_ones, axis=1)
    b = np.vectorize(function)(nodes)
    x = np.linalg.solve(a, b)
    return (x[:-1], x[-1])

def ramez_algorithm(function_to_approximate, degree, interval_min, interval_max):
    node_count = degree + 2

    initial_nodes = generate_chebyshev_nodes(interval_min, interval_max, node_count)
    current_nodes = initial_nodes

    for i in range(2000):
        polynomial, error = find_polynomial(current_nodes, function_to_approximate) 
        sample_count = 2000
        xs = np.linspace(interval_min, interval_max, sample_count)
        ys = np.abs(np.polyval(polynomial, xs) - function_to_approximate(xs))
        error_maxima_indices = find_peaks(ys)[0]
        error_maxima = xs[error_maxima_indices]
        if len(error_maxima_indices) != node_count - 2:
            break
        current_nodes = np.concatenate((np.array(error_maxima), np.array([interval_min, interval_max])))
        # The nodes have to be sorted to make the system of equations with alternating error work.
        current_nodes = np.sort(current_nodes)

    print(f'terminated after {i + 1} iterations')

    return polynomial, current_nodes

def print_polynomial_horner(polynomial):
    for i in range(len(polynomial)):
        coefficient = polynomial[len(polynomial) - 1 - i]
        print(coefficient, end='')
        if i != len(polynomial) - 1:
            print(' + r * (', end='')

    for i in range(len(polynomial) - 1):
        print(')', end='')
    print()
    

def debug_remez(function_to_approximate, degree, interval_min, interval_max):
    polynomial, nodes = ramez_algorithm(function_to_approximate, degree, interval_min, interval_max)
    xs = np.linspace(interval_min, interval_max, 2000)

    print(polynomial)
    print_polynomial_horner(polynomial)

    plt.scatter(nodes, function_to_approximate(nodes))
    plt.plot(xs, function_to_approximate(xs))
    plt.plot(xs, np.polyval(polynomial, xs))
    plt.show()

    error_values = np.polyval(polynomial, xs) - function_to_approximate(xs)
    plt.plot(xs, error_values)
    # plt.scatter(error_maxima, np.abs(function_to_approximate(error_maxima) - np.polyval(polynomial, error_maxima)))
    plt.scatter(nodes, np.polyval(polynomial, nodes) - function_to_approximate(nodes))
    plt.show()

    error = np.max(error_values)
    print(f"error: {error}")
    return polynomial, nodes

ln2_over_2 = 0.34657359028
# debug_remez(np.exp, 4, -ln2_over_2, ln2_over_2)
# debug_remez(np.log1p, 4, 0, 1)
debug_remez(np.log1p, 6, 0, 1)
# debug_remez(np.cos, 5, 0, np.pi/4)