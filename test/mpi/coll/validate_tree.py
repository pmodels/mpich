from __future__ import print_function
import sys
import json
from collections import defaultdict
import glob


def main():
    filename_pattern = sys.argv[1]
    tree = []
    for filename in glob.glob(filename_pattern):
        with open(filename) as a_file:
            tree.append(json.load(a_file))
    try:
        Validator(tree).validate()
    except AssertionError as e:
        print("Tree is not valid: {}".format(e), file=sys.stderr)
        print("\nFull tree:\n\n{}\n".format(tree), file=sys.stderr)
        exit(1)


class Validator:

    def __init__(self, tree):
        self.tree = tree
        self.rank_parents_map = None
        self.node_rank_map = None

    def validate(self):
        # A tree is valid if it conforms to the following:
        #   1. Each child index must be within the valid range
        #   2. Except for the root, each node must be a child of exactly one node
        #   3. Each node must correctly identify its parent
        #   4. Each rank must be represented in exactly one node
        #   5. There must be no cycles
        self.validate_child_indices()
        self.validate_single_root()
        self.validate_single_parentage()
        self.validate_correct_parent_ids()
        self.validate_ranks_nodes_one_to_one()
        self.validate_acyclic()

    def validate_child_indices(self):
        invalid = []
        for node in self.tree:
            for child_rank in node['children']:
                if child_rank >= len(self.tree) or child_rank < 0:
                    invalid.append(node)
        msg = "The following nodes have one or more invalid child indices: {}".format(invalid)
        assert 0 == len(invalid), msg

    def validate_single_root(self):
        parents_for_rank = self.get_rank_parents_map()
        roots = [node for node in self.tree if node['rank'] not in parents_for_rank]
        msg = "Tree has too many roots; found roots {}".format(roots)
        assert 1 == len(roots), msg

    def validate_single_parentage(self):
        parents_for_rank = self.get_rank_parents_map()
        node_for_rank = self.get_node_rank_map()
        for rank, parents in parents_for_rank.items():
            msg = "Node {} has too many roots; found parents {}".format(node_for_rank[rank], parents)
            assert 1 == len(parents), msg

    def validate_correct_parent_ids(self):
        parents_for_rank = self.get_rank_parents_map()
        node_for_rank = self.get_node_rank_map()
        for rank, parents in parents_for_rank.items():
            node, expected_parent = node_for_rank[rank], parents[0]['rank']
            msg = "Node {} does not correctly identify its parent; should be {}".format(node, expected_parent)
            assert node['parent'] == expected_parent, msg

    def validate_ranks_nodes_one_to_one(self):
        ranks_represented = {node['rank'] for node in self.tree}
        assert len(ranks_represented) == len(self.tree)

    def validate_acyclic(self):
        assert not self.is_cyclic(self.tree), "Tree contains one or more cycles."

    def is_cyclic(self, tree):
        node_for_rank = self.get_node_rank_map()
        visited = set()
        stack = set()
        for node in tree:
            if node['rank'] not in visited:
                if self.subtree_is_cyclic(node, visited, stack, node_for_rank):
                    return True
        return False

    def subtree_is_cyclic(self, subtree, visited, stack, node_for_rank):
        visited.add(subtree['rank'])
        stack.add(subtree['rank'])
        for child_rank in subtree['children']:
            child = node_for_rank[child_rank]
            if child_rank not in visited:
                if self.subtree_is_cyclic(child, visited, stack, node_for_rank):
                    return True
            elif child_rank in stack:
                return True
        stack.remove(subtree['rank'])
        return False

    def get_rank_parents_map(self):
        if not self.rank_parents_map:
            self.rank_parents_map = defaultdict(lambda: [])
            node_for_rank = self.get_node_rank_map()
            for node in self.tree:
                for child_index in node['children']:
                    try:
                        child_node = node_for_rank[child_index]
                        self.rank_parents_map[child_node['rank']].append(node)
                    except KeyError:
                        pass
        return self.rank_parents_map

    def get_node_rank_map(self):
        if not self.node_rank_map:
            self.node_rank_map = {node['rank']: node for node in self.tree}
        return self.node_rank_map


if __name__ == '__main__':
    main()
