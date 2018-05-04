#ifndef MPIU_DECISION_TREE_TYPES_H_INCLUDED
#define MPIU_DECISION_TREE_TYPES_H_INCLUDED

/* Uses JSON parser from this repository (MIT license):
 * https://github.com/DaveGamble/cJSON
 */
extern cJSON *mpiu_decision_tree;

int MPIU_Decision_tree_parse(char *filename);

/* Function to traverse the JSON object to find the appropriate algorithm for the particular input:
 *
 * coll_name - The stringified name of the collective
 * comm_ptr - The communicator being used in the collective
 * msg_size - The size of messages that will be sent by the collective
 * datatype - The datatype being used by the collective (for 'W' collectives, this value is ignored)
 * coll_info - The JSON node with the information about which algorithm to use.
 */
int MPIU_Decision_tree_traverse(const char *coll_name, MPIR_Comm * comm_ptr, size_t msg_size,
                                MPI_Datatype datatype, const cJSON * coll_info);

#endif /* MPIU_DECISION_TREE_TYPES_H_INCLUDED */
