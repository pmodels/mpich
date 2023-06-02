from validate_tree import *
import unittest
from parameterized import parameterized


class TestCase(unittest.TestCase):

    valid_trees = [
        ([
            {'rank': 0, 'parent': -1, 'children': []},
         ],),
        ([
             {'rank': 1, 'parent': 0, 'children': []},
             {'rank': 0, 'parent': -1, 'children': [1, 2]},
             {'rank': 2, 'parent': 0, 'children': []},
         ],),
        ([
             {'rank': 1, 'parent': 0, 'children': [2, 3]},
             {'rank': 4, 'parent': 0, 'children': []},
             {'rank': 0, 'parent': -1, 'children': [1, 4]},
             {'rank': 3, 'parent': 1, 'children': []},
             {'rank': 2, 'parent': 1, 'children': []},
         ],),
    ]

    trees_with_invalid_child_indices = [
        ([
            {'rank': 0, 'parent': -1, 'children': [1]}
        ],),
        ([
            {'rank': 0, 'parent': -1, 'children': [1]},
            {'rank': 1, 'parent': 0, 'children': [2]},
        ],),
        ([
            {'rank': 0, 'parent': -1, 'children': [1, 2]},
            {'rank': 1, 'parent': 0, 'children': [3]},
            {'rank': 2, 'parent': 0, 'children': []},
        ],),
        ([
            {'rank': 0, 'parent': -1, 'children': [1, 2]},
            {'rank': 1, 'parent': 0, 'children': []},
            {'rank': 2, 'parent': 0, 'children': [-1]},
        ],),
    ]

    @parameterized.expand(trees_with_invalid_child_indices)
    def test_invalid_child_indices(self, tree):
        with self.assertRaises(AssertionError):
            Validator(tree).validate_child_indices()

    @parameterized.expand(valid_trees)
    def test_valid_child_indices(self, tree):
        Validator(tree).validate_child_indices()

    trees_with_multiple_roots =[
        ([
            {'rank': 0, 'parent': -1, 'children': []},
            {'rank': 1, 'parent': -1, 'children': []},
        ],),
        ([
            {'rank': 0, 'parent': -1, 'children': [1]},
            {'rank': 1, 'parent': 0, 'children': []},
            {'rank': 2, 'parent': -1, 'children': []},
        ],),
    ]

    @parameterized.expand(trees_with_multiple_roots)
    def test_multiple_roots(self, tree):
        with self.assertRaises(AssertionError):
            Validator(tree).validate_single_root()

    @parameterized.expand(valid_trees)
    def test_single_root(self, tree):
        Validator(tree).validate_single_root()

    trees_with_multiple_parents = [
        ([
            {'rank': 0, 'parent': -1, 'children': [1, 2]},
            {'rank': 1, 'parent': 0, 'children': [2]},
            {'rank': 2, 'parent': 1, 'children': []},
         ],),
        ([
            {'rank': 0, 'parent': -1, 'children': [1]},
            {'rank': 1, 'parent': 0, 'children': [2, 3, 4]},
            {'rank': 2, 'parent': 1, 'children': [3, 4]},
            {'rank': 3, 'parent': 2, 'children': [4]},
            {'rank': 4, 'parent': 3, 'children': []},
        ],),
    ]

    @parameterized.expand(trees_with_multiple_parents)
    def test_multiple_parents(self, tree):
        with self.assertRaises(AssertionError):
            Validator(tree).validate_single_parentage()

    @parameterized.expand(valid_trees)
    def test_single_parent(self, tree):
        Validator(tree).validate_single_parentage()

    trees_with_incorrect_parent_ids = [
        ([
            {'rank': 0, 'parent': -1, 'children': [1]},
            {'rank': 1, 'parent': 45, 'children': []},
        ],),
        ([
            {'rank': 0, 'parent': -1, 'children': [1]},
            {'rank': 1, 'parent': 0, 'children': [2]},
            {'rank': 2, 'parent': -1234, 'children': []},
        ],),
        ([
            {'rank': 0, 'parent': -1, 'children': [1, 2]},
            {'rank': 1, 'parent': 2, 'children': []},
            {'rank': 2, 'parent': 0, 'children': []},
         ],),
    ]

    @parameterized.expand(trees_with_incorrect_parent_ids)
    def test_incorrect_parent_ids(self, tree):
        with self.assertRaises(AssertionError):
            Validator(tree).validate_correct_parent_ids()

    @parameterized.expand(valid_trees)
    def test_correct_parent_ids(self, tree):
        Validator(tree).validate_correct_parent_ids()

    trees_with_ranks_nodes_not_one_to_one = [
        ([
            {'rank': 0, 'parent': -1, 'children': [1]},
            {'rank': 1, 'parent': 0, 'children': []},
            {'rank': 1, 'parent': 0, 'children': []},
         ],),
        ([
            {'rank': 1, 'parent': 0, 'children': [2, 3]},
            {'rank': 4, 'parent': 0, 'children': []},
            {'rank': 0, 'parent': -1, 'children': [1, 4]},
            {'rank': 4, 'parent': 1, 'children': []},
            {'rank': 2, 'parent': 1, 'children': []},
         ],),
    ]

    @parameterized.expand(trees_with_ranks_nodes_not_one_to_one)
    def test_ranks_nodes_not_one_to_one(self, tree):
        with self.assertRaises(AssertionError):
            Validator(tree).validate_ranks_nodes_one_to_one()

    @parameterized.expand(valid_trees)
    def test_ranks_nodes_one_to_one(self, tree):
        Validator(tree).validate_ranks_nodes_one_to_one()

    @parameterized.expand(valid_trees)
    def test_acyclic_trees(self, tree):
        Validator(tree).validate_acyclic()

    cyclic_trees = [
        ([
            {'rank': 0, 'parent': 0, 'children': [0]},
        ],),
        ([
            {'rank': 0, 'parent': 4, 'children': [1, 2]},
            {'rank': 1, 'parent': 0, 'children': [3, 4]},
            {'rank': 2, 'parent': 0, 'children': []},
            {'rank': 3, 'parent': 1, 'children': []},
            {'rank': 4, 'parent': 1, 'children': [0]}
        ],),
    ]

    @parameterized.expand(cyclic_trees)
    def test_cyclic_trees(self, tree):
        with self.assertRaises(AssertionError):
            Validator(tree).validate_acyclic()

    invalid_trees = (cyclic_trees + trees_with_ranks_nodes_not_one_to_one + trees_with_incorrect_parent_ids +
                     trees_with_multiple_parents + trees_with_multiple_roots + trees_with_invalid_child_indices)

    @parameterized.expand(invalid_trees)
    def test_invalid_trees(self, tree):
        with self.assertRaises(AssertionError):
            Validator(tree).validate()

    @parameterized.expand(valid_trees)
    def test_valid_trees(self, tree):
        Validator(tree).validate()
