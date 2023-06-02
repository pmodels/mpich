#!/usr/bin/env python

import argparse
import random


def get_random_coords(_, __, dim_maxes):
    return [int(random.random() * dim_max) for dim_max in dim_maxes]


def get_fixed_coords(rank, rank_count, dim_maxes):
    return [(rank * dim_max)/rank_count for dim_max in dim_maxes]


def parse_args():
    description = '''
Generate per-rank network topology coordinates in the form 

    0: coord0 coord1 ... coordN
    1: coord0 coord1 ... coordN
    .
    .
    M: coord0 coord1 ... coordN

'''
    argparser = argparse.ArgumentParser(description=description, formatter_class=argparse.RawDescriptionHelpFormatter)
    argparser.add_argument('--ranks', '-n', type=int, default=8, help='Number of ranks.')
    argparser.add_argument('--dim-maxes', '-d', default='4,2', help='Max value for each dimension, comma-separated.')
    argparser.add_argument('--random', '-r', action='store_true')
    return argparser.parse_args()


def main():
    args = parse_args()
    dim_maxes = [int(x) for x in args.dim_maxes.split(',')]
    get_coords = get_random_coords if args.random else get_fixed_coords
    for rank in range(args.ranks):
        coords = get_coords(rank, args.ranks, dim_maxes)
        print('{}: '.format(rank) + ' '.join([str(coord) for coord in coords]))


if __name__ == '__main__':
    main()
