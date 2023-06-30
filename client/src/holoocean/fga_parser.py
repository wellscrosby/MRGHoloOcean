import math
import numpy as np
import linecache

def xyz_to_fga_index(x, y, z, dim_x, dim_y):
    # index of the position in the fga file
    index = x + y * dim_x + z * dim_x * dim_y
    
    #add 3 to get past file header
    return index + 3

def pos_to_cell_location(x, y, z, dim_x, dim_y, dim_z, max_x, max_y, max_z, min_x, min_y, min_z):
    # get the cell location of the position
    cell_x = (float(dim_x - 1)/ (max_x - min_x)) * (x - min_x)
    cell_y = (float(dim_y - 1)/ (max_y - min_y)) * (y - min_y)
    cell_z = (float(dim_z - 1)/ (max_z - min_z)) * (z - min_z)
    
    return cell_x, cell_y, cell_z

def setup_fga(filename):
    # get the dimensions
    dims = linecache.getline(filename, 1).split(",")
    dim_x = int(float(dims[0]))
    dim_y = int(float(dims[1]))
    dim_z = int(float(dims[2]))
    
    # get the min and max locations
    mins = linecache.getline(filename, 2).split(",")
    min_x = float(mins[0])
    min_y = float(mins[1])
    min_z = float(mins[2])
    
    maxs = linecache.getline(filename, 3).split(",")
    max_x = float(maxs[0])
    max_y = float(maxs[1])
    max_z = float(maxs[2])
    
    return dim_x, dim_y, dim_z, max_x, max_y, max_z, min_x, min_y, min_z, filename

def weigthed_avg_vel(pos, setup):
    x, y, z = pos
    dim_x, dim_y, dim_z, max_x, max_y, max_z, min_x, min_y, min_z, filename = setup

    if x < min_x or x > max_x or y < min_y or y > max_y or z < min_z or z > 0.0:
        return np.array([0.0,0.0,0.0])

    cell_x, cell_y, cell_z = pos_to_cell_location(x, y, z, dim_x, dim_y, dim_z, \
                                                  max_x, max_y, max_z, min_x, min_y, min_z)

    floor_x = math.floor(cell_x)
    floor_y = math.floor(cell_y)
    floor_z = math.floor(cell_z)

    inv_distances = []
    combinations = [[0,0,0], [0,0,1], [0,1,0], [0,1,1], [1,0,0], [1,0,1], [1,1,0], [1,1,1]]

    sum_inv_distances = 0.0
    for combination in combinations:
        inv_dist = 1.0 / math.sqrt(((floor_x + combination[0]) - cell_x)**2 + \
                                    ((floor_y + combination[1]) - cell_y)**2 + \
                                    ((floor_z + combination[2]) - cell_z)**2)
        inv_distances.append(inv_dist)

        sum_inv_distances += inv_dist

    result = np.array([0.0,0.0,0.0])
    for index, combination in enumerate(combinations):
        line = linecache.getline(filename, xyz_to_fga_index(floor_x + combination[0], floor_y + combination[1], floor_z + combination[2], dim_x, dim_y) + 1).split(",")
        line = np.float_(line[0:-1])

        result += (inv_distances[index] / sum_inv_distances) * line

    return result