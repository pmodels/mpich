import json
import sys
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
    input_tree = load_tree(args.node_files)
    root = find_root(input_tree)
    if root == -1:
        print('cannot find root')
    level_array = bfs(input_tree, root)
    render_tree(input_tree, level_array, args.format, args.output)

# Suppose input_data is:
# [{'rank': 0, 'nranks': 4, 'parent': -1, 'children': [2, 1]},
#  {'rank': 1, 'nranks': 8, 'parent': 0, 'children': []},
#  {'rank': 2, 'nranks': 4, 'parent': 0, 'children': [3]},
#  {'rank': 3, 'nranks': 4, 'parent': 2, 'children': []}]
# The input_tree stores the input_data in a 2D array:
# [[2, 1], [], [3], []]
def load_tree(node_files):
    input_data = []
    for filename in node_files:
        with open(filename) as the_file:
            input_data.append(json.load(the_file))
    input_tree = []
    for i in range(input_data[0]['nranks']):
        input_tree.append([])
    for node in input_data:
        for child in node['children']:
            input_tree[node['rank']].append(child)
    print(input_tree)
    return input_tree

# Find the root of the input_tree
def find_root(input_tree):
    check_list = []
    for i in range(len(input_tree)):
        check_list.append(0);
    for node in input_tree:
        for child in node:
            check_list[child] = 1
    for i in range(len(check_list)):
        if (check_list[i] == 0):
            return i
    return -1

# Perform a breadth-first search
# The level_array would be: [[0], [2, 1], [3]]
# Level 0 has rank 0, level 1 has rank 2 and 1 and level 2 has rank 3
def bfs(input_tree, root):
    level_array = []
    node_queue = []
    node_queue.append(root)
    cur_level = 0
    while node_queue:
        level_array.append([])
        cur_queue_len = len(node_queue)
        for i in range(cur_queue_len):
            cur_node = node_queue.pop(0)
            level_array[cur_level].append(cur_node)
            for child in input_tree[cur_node]:
                node_queue.append(child)
        cur_level = cur_level + 1
    print(level_array)
    return level_array


def render_tree(input_tree, level_array, format, output):
    dot = Digraph(output)
    # Create invisible edges to keep the order of the children
    for i in range(len(level_array)):
        with dot.subgraph() as s:
            s.attr(rank='same')
            s.attr(rankdir='LR')
            for cur_node in level_array[i]:
                s.node(str(cur_node))
            if len(level_array[i]) >= 2:
                for j in range(len(level_array[i])):
                    if (j >= 1):
                        dot.edge(str(level_array[i][j-1]), str(level_array[i][j]), style='invis')
    for i in range(len(input_tree)):
        for j in range(len(input_tree[i])):
            dot.edge(str(i), str(input_tree[i][j]))            
    dot.render(format=format, view=True)


def parse_args():
    description = 'Render a topology-aware collective tree as a graphical tree.'
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument('node_files', metavar='FILE', nargs='+', help='File(s) containing the JSON-formatted tree nodes to be rendered (e.g. tree-node-*.json).')
    parser.add_argument('--format', '-f', default='svg', help='Output format. Can be any format supported by graphviz. Default: svg.')
    parser.add_argument('--output', '-o', default='tree', help='Output file name. Default tree.')
    return parser.parse_args()


if __name__ == '__main__':
    main()
