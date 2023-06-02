import sys
import re
import argparse

try:
    from graphviz import Digraph
except ImportError as e:
    sys.exit('Unable to import graphviz module: {}.\n'.format(e) +
             'Please install the graphviz module by running the following command:\n'
             '\n'
             '      python -m pip install graphviz\n')

def main():
    args = parse_args()
    dot = Digraph('Topology')

    edges = set()

    with open(args.filename) as coords_file:
        for line in coords_file:
            rank_label, coord_node_labels = parse_line(line)
            dot.node(rank_label)
            [dot.node(label) for label in coord_node_labels]
            for index in range(len(coord_node_labels) - 1):
                edges.add((coord_node_labels[index], coord_node_labels[index+1]))
            edges.add((coord_node_labels[-1], rank_label))

    dot.edges(edges)
    dot.render(format=args.format, view=True)


def parse_args():
    description = 'Render a per-rank coordinates topology file (streamed on stdin) as a graphical set of trees.'
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument('filename', help='The file contains the coordinates.')
    parser.add_argument('--format', '-f', default='svg', help='Output format. Can be any format supported by graphviz. Default: svg.')
    return parser.parse_args()


def parse_line(line):
    rank_label, coord_set = line.split(':')
    coords = coord_set.split()
    coord_node_labels = create_coord_node_labels(coords)
    return rank_label, coord_node_labels


def create_coord_node_labels(coords):
    result = []
    coord_combo = ''
    for prefix, coord in zip(['G', 'S', 'P'], reversed(coords)):
        if 0 == len(coord_combo):
            coord_combo = coord
        else:
            coord_combo += "." + coord
        result.append(prefix + coord_combo)
    return result



if __name__ == '__main__':
    main()
