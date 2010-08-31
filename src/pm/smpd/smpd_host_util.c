/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "smpd.h"
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#if defined(HAVE_DIRECT_H) || defined(HAVE_WINDOWS_H)
#include <direct.h>
#endif
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#undef FCNAME
#define FCNAME "smpd_parse_map_string"
int smpd_parse_map_string(const char *str, smpd_map_drive_node_t **list)
{
    smpd_map_drive_node_t *map_node;
    const char *cur_pos;
    char *iter;

    /* string format: drive:\\host\share;drive2:\\host2\share2... */

    smpd_enter_fn(FCNAME);

    if (str == NULL || list == NULL)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    cur_pos = str;
    while (cur_pos[0] != '\0' && cur_pos[1] == ':')
    {
	map_node = (smpd_map_drive_node_t*)MPIU_Malloc(sizeof(smpd_map_drive_node_t));
	if (map_node == NULL)
	{
	    smpd_err_printf("Error: malloc failed to allocate map structure.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	map_node->ref_count = 0;
	map_node->drive = cur_pos[0];
	cur_pos++;
	cur_pos++;
	iter = map_node->share;
	while (*cur_pos != '\0' && *cur_pos != ';')
	{
	    /* buffer overrun check */
	    if (iter == &map_node->share[SMPD_MAX_EXE_LENGTH])
	    {
		MPIU_Free(map_node);
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    *iter++ = *cur_pos++;
	}
	*iter = '\0';
	if (*cur_pos == ';')
	    cur_pos++;
	map_node->next = *list;
	*list = map_node;
    }
    return SMPD_SUCCESS;
}

#ifdef HAVE_WINDOWS_H
#undef FCNAME
#define FCNAME "smpd_mapall"
#define SMPD_MAP_NETWORKDRVSTRING_MAX   1024
int smpd_mapall(smpd_map_drive_node_t **list){
    char *smpd_map_string;
    int success = 1;

    smpd_enter_fn(FCNAME);
    smpd_map_string = (char *)MPIU_Malloc(SMPD_MAP_NETWORKDRVSTRING_MAX);
    if(!smpd_map_string){
   		printf("Error: unable to allocate mem for mapping network drive\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
    }
    if(smpd_get_network_drives(smpd_map_string, SMPD_MAP_NETWORKDRVSTRING_MAX) == SMPD_SUCCESS){
        if(smpd_parse_map_string(smpd_map_string, list) != SMPD_SUCCESS){
   	    	printf("Error: unable to map network drive\n");
            success = 0;
        }
    }else{
   		printf("Error: unable to get network drives\n");
        success = 0;
    }
    MPIU_Free(smpd_map_string);
    smpd_exit_fn(FCNAME);
    if(success)
        return SMPD_SUCCESS;
    else
        return SMPD_FAIL;
}
#endif

#undef FCNAME
#define FCNAME "smpd_fix_up_host_tree"
void smpd_fix_up_host_tree(smpd_host_node_t *node)
{
    smpd_host_node_t *cur, *iter;
    SMPD_BOOL left_found;

    smpd_enter_fn(FCNAME);

    cur = node;
    while (cur != NULL)
    {
	left_found = SMPD_FALSE;
	iter = cur->next;
	while (iter != NULL)
	{
	    if (iter->parent == cur->id)
	    {
		if (left_found)
		{
		    cur->right = iter;
		    break;
		}
		cur->left = iter;
		left_found = SMPD_TRUE;
	    }
	    iter = iter->next;
	}
	cur = cur->next;
    }
    smpd_exit_fn(FCNAME);
}

#undef FCNAME
#define FCNAME "smpd_append_env_option"
int smpd_append_env_option(char *str, int maxlen, const char *env_name, const char *env_val)
{
    int len;
    
    smpd_enter_fn(FCNAME);
    len = (int)strlen(str);
    if (len > (maxlen-2))
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    str[len] = ' ';
    str = &str[len+1];
    maxlen = maxlen - len - 1;
    if (MPIU_Str_add_string_arg(&str, &maxlen, env_name, env_val) != MPIU_STR_SUCCESS)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    str--;
    *str = '\0'; /* trim the extra space at the end */
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_get_hostname"
int smpd_get_hostname(char *host, int length)
{
#ifdef HAVE_WINDOWS_H
    DWORD len = length;
    smpd_enter_fn(FCNAME);
    /*if (!GetComputerName(host, &len))*/
    if (!GetComputerNameEx(ComputerNameDnsFullyQualified, host, &len))
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
#else
    smpd_enter_fn(FCNAME);
    if (gethostname(host, length))
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
#endif
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_get_pwd_from_file"
int smpd_get_pwd_from_file(char *file_name)
{
    char line[1024];
    FILE *fin;

    smpd_enter_fn(FCNAME);

    /* open the file */
    fin = fopen(file_name, "r");
    if (fin == NULL)
    {
	printf("Error, unable to open account file '%s'\n", file_name);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* read the account */
    if (!fgets(line, 1024, fin))
    {
	printf("Error, unable to read the account in '%s'\n", file_name);
	fclose(fin);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* strip off the newline characters */
    while (strlen(line) && (line[strlen(line)-1] == '\r' || line[strlen(line)-1] == '\n'))
	line[strlen(line)-1] = '\0';
    if (strlen(line) == 0)
    {
	printf("Error, first line in password file must be the account name. (%s)\n", file_name);
	fclose(fin);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* save the account */
    strcpy(smpd_process.UserAccount, line);

    /* read the password */
    if (!fgets(line, 1024, fin))
    {
	printf("Error, unable to read the password in '%s'\n", file_name);
	fclose(fin);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* strip off the newline characters */
    while (strlen(line) && (line[strlen(line)-1] == '\r' || line[strlen(line)-1] == '\n'))
	line[strlen(line)-1] = '\0';

    /* save the password */
    if (strlen(line))
	strcpy(smpd_process.UserPassword, line);
    else
	smpd_process.UserPassword[0] = '\0';

    fclose(fin);

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_get_next_hostname"
int smpd_get_next_hostname(char *host, char *alt_host)
{
    smpd_enter_fn(FCNAME);
    if (smpd_process.s_host_list == NULL)
    {
	if (smpd_process.cur_default_host)
	{
	    if (smpd_process.cur_default_iproc >= smpd_process.cur_default_host->nproc)
	    {
		smpd_process.cur_default_host = smpd_process.cur_default_host->next;
		smpd_process.cur_default_iproc = 0;
		if (smpd_process.cur_default_host == NULL) /* This should never happen because the hosts are in a ring */
		{
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
	    }
	    strcpy(host, smpd_process.cur_default_host->host);
	    strcpy(alt_host, smpd_process.cur_default_host->alt_host);
	    smpd_process.cur_default_iproc++;
	}
	else
	{
	    if (smpd_get_hostname(host, SMPD_MAX_HOST_LENGTH) != 0)
	    {
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    if (smpd_process.s_cur_host == NULL)
    {
	smpd_process.s_cur_host = smpd_process.s_host_list;
	smpd_process.s_cur_count = 0;
    }
    strcpy(host, smpd_process.s_cur_host->host);
    strcpy(alt_host, smpd_process.s_cur_host->alt_host);
    smpd_process.s_cur_count++;
    if (smpd_process.s_cur_count >= smpd_process.s_cur_host->nproc)
    {
	smpd_process.s_cur_host = smpd_process.s_cur_host->next;
	smpd_process.s_cur_count = 0;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_parse_extra_machinefile_options"
int smpd_parse_extra_machinefile_options(const char *line, smpd_host_node_t *node)
{
    char *flag, *p;

    smpd_enter_fn(FCNAME);
    flag = strstr(line, "-ifhn");
    if (flag != NULL)
    {
	p = flag + 5;
	while (isspace(*p))
	    p++;
	MPIU_Strncpy(node->alt_host, p, SMPD_MAX_HOST_LENGTH);
    }

    flag = strstr(line, "ifhn=");
    if (flag != NULL)
    {
	p = flag + 5;
	while (isspace(*p))
	    p++;
	MPIU_Strncpy(node->alt_host, p, SMPD_MAX_HOST_LENGTH);
    }

    flag = strstr(line, "-ifip");
    if (flag != NULL)
    {
	p = flag + 5;
	while (isspace(*p))
	    p++;
	MPIU_Strncpy(node->alt_host, p, SMPD_MAX_HOST_LENGTH);
    }

    flag = strstr(line, "-ifn");
    if (flag != NULL)
    {
	p = flag + 4;
	while (isspace(*p))
	    p++;
	MPIU_Strncpy(node->alt_host, p, SMPD_MAX_HOST_LENGTH);
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_parse_machine_file"
SMPD_BOOL smpd_parse_machine_file(char *file_name)
{
    char line[1024];
    FILE *fin;
    smpd_host_node_t *node, *node_iter;
    char *hostname, *iter;
    int nproc;

    smpd_enter_fn(FCNAME);

    smpd_process.s_host_list = NULL;
    smpd_process.s_cur_host = NULL;
    smpd_process.s_cur_count = 0;

    /* open the file */
    fin = fopen(file_name, "r");
    if (fin == NULL)
    {
	printf("Error, unable to open machine file '%s'\n", file_name);
	smpd_exit_fn(FCNAME);
	return SMPD_FALSE;
    }

    while (fgets(line, 1024, fin))
    {
	/* strip off the newline characters */
	while (strlen(line) && (line[strlen(line)-1] == '\r' || line[strlen(line)-1] == '\n'))
	    line[strlen(line)-1] = '\0';
	hostname = line;
	/* move over any leading whitespace */
	while (isspace(*hostname))
	    hostname++;
	if (strlen(hostname) != 0 && hostname[0] != '#')
	{
	    iter = hostname;
	    /* move over the hostname and see if there is a number after it */
	    while ((*iter != '\0') && !isspace(*iter) && (*iter != ':'))
		iter++;
	    if (*iter != '\0')
	    {
		*iter = '\0';
		iter++;
		while (isspace(*iter))
		    iter++;
		nproc = 1;
		if (isdigit(*iter))
		{
		    nproc = atoi(iter);
		    /* move over the number */
		    while (isdigit(*iter))
			iter++;
		    /* move over the space between the number and any other options */
		    while (isspace(*iter))
			iter++;
		}
		if (nproc < 1)
		    nproc = 1;
	    }
	    else
	    {
		nproc = 1;
	    }
	    node = (smpd_host_node_t*)MPIU_Malloc(sizeof(smpd_host_node_t));
	    if (node == NULL)
	    {
		smpd_err_printf("unable to allocate memory to parse the machinefile\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FALSE;
	    }
	    strcpy(node->host, hostname);
	    node->alt_host[0] = '\0';
	    node->connected = SMPD_FALSE;
	    node->connect_cmd_tag = -1;
	    node->id = -1;
	    node->parent = -1;
	    node->nproc = nproc;
	    node->next = NULL;
	    node->left = NULL;
	    node->right = NULL;
	    smpd_parse_extra_machinefile_options(iter, node);
	    smpd_add_extended_host_to_default_list(node->host, node->alt_host, node->nproc);
	    if (smpd_process.s_host_list == NULL)
		smpd_process.s_host_list = node;
	    else
	    {
		node_iter = smpd_process.s_host_list;
		while (node_iter->next != NULL)
		    node_iter = node_iter->next;
		node_iter->next = node;
	    }
	}
    }
    if (smpd_process.s_host_list != NULL)
    {
	node = smpd_process.s_host_list;
	while (node)
	{
	    smpd_dbg_printf("host = %s, nproc = %d\n", node->host, node->nproc);
	    node = node->next;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_TRUE;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_FALSE;
}

#undef FCNAME
#define FCNAME "smpd_parse_hosts_string"
int smpd_parse_hosts_string(const char *host_str)
{
    char *token, *str;
    char temp_hostname[256];
    smpd_host_node_t *node, *node_iter;
    char *hostname, *iter;
    int nproc;

    smpd_enter_fn(FCNAME);

    /* FIXME: If these are not NULL should they be freed? */
    smpd_process.s_host_list = NULL;
    smpd_process.s_cur_host = NULL;
    smpd_process.s_cur_count = 0;

    str = MPIU_Strdup(host_str);
    if (str == NULL)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FALSE;
    }

    token = strtok(str, " \r\n");
    while (token != NULL)
    {
	strcpy(temp_hostname, token);
	token = strtok(NULL, " \r\n");
	hostname = temp_hostname;
	/* move over any leading whitespace */
	while (isspace(*hostname))
	    hostname++;
	if (strlen(hostname) != 0)
	{
	    iter = hostname;
	    /* move over the hostname and see if there is a number after it */
	    while ((*iter != '\0') && !isspace(*iter) && (*iter != ':'))
		iter++;
	    if (*iter != '\0')
	    {
		*iter = '\0';
		iter++;
		while (isspace(*iter))
		    iter++;
		nproc = 1;
		if (isdigit(*iter))
		{
		    nproc = atoi(iter);
		    /* move over the number */
		    while (isdigit(*iter))
			iter++;
		    /* move over the space between the number and any other options */
		    while (isspace(*iter))
			iter++;
		}
		if (nproc < 1)
		    nproc = 1;
	    }
	    else
	    {
		nproc = 1;
	    }
	    node = (smpd_host_node_t*)MPIU_Malloc(sizeof(smpd_host_node_t));
	    if (node == NULL)
	    {
		smpd_err_printf("unable to allocate memory to parse the hosts string <%s>\n", host_str);
		MPIU_Free(str);
		smpd_exit_fn(FCNAME);
		return SMPD_FALSE;
	    }
	    strcpy(node->host, hostname);
	    node->alt_host[0] = '\0';
	    node->connected = SMPD_FALSE;
	    node->connect_cmd_tag = -1;
	    node->id = -1;
	    node->parent = -1;
	    node->nproc = nproc;
	    node->next = NULL;
	    node->left = NULL;
	    node->right = NULL;
	    smpd_add_extended_host_to_default_list(node->host, node->alt_host, node->nproc);
	    if (smpd_process.s_host_list == NULL)
		smpd_process.s_host_list = node;
	    else
	    {
		node_iter = smpd_process.s_host_list;
		while (node_iter->next != NULL)
		    node_iter = node_iter->next;
		node_iter->next = node;
	    }
	}
    }
    MPIU_Free(str);
    if (smpd_process.s_host_list != NULL)
    {
	node = smpd_process.s_host_list;
	while (node)
	{
	    smpd_dbg_printf("host = %s, nproc = %d\n", node->host, node->nproc);
	    node = node->next;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_TRUE;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_FALSE;
}

/* Free the global SMPD host list */
#undef FCNAME
#define FCNAME "smpd_free_host_list"
int smpd_free_host_list(void )
{
    smpd_host_node_t *pnode;

    while(smpd_process.host_list){
        pnode = smpd_process.host_list;
        smpd_process.host_list = smpd_process.host_list->next;
        MPIU_Free(pnode);
    }
    smpd_process.host_list = NULL;
    /* Reset tree id - mpiexec uses tree id = 0 & all hosts use
     * tree id >= 1
     */
    smpd_process.tree_id = 1;
    /* mpiexec with id=0 is the root of all hosts */
    smpd_process.tree_parent = 0;
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_get_host_id"
int smpd_get_host_id(char *host, int *id_ptr)
{
    smpd_host_node_t *node;
    int bit, mask, temp;

    smpd_enter_fn(FCNAME);

    /* look for the host in the list */
    node = smpd_process.host_list;
    while (node)
    {
	if (strcmp(node->host, host) == 0)
	{
	    /* return the id */
	    *id_ptr = node->id;
	    smpd_exit_fn(FCNAME);
	    return SMPD_SUCCESS;
	}
	if (node->next == NULL)
	    break;
	node = node->next;
    }

    /* allocate a new node */
    if (node != NULL)
    {
	node->next = (smpd_host_node_t *)MPIU_Malloc(sizeof(smpd_host_node_t));
	node = node->next;
    }
    else
    {
	node = (smpd_host_node_t *)MPIU_Malloc(sizeof(smpd_host_node_t));
	smpd_process.host_list = node;
    }
    if (node == NULL)
    {
	smpd_err_printf("malloc failed to allocate a host node structure\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    strcpy(node->host, host);
    node->alt_host[0] = '\0';
    node->parent = smpd_process.tree_parent;
    node->id = smpd_process.tree_id;
    node->connected = SMPD_FALSE;
    node->connect_cmd_tag = -1;
    node->nproc = -1;
    node->next = NULL;
    node->left = NULL;
    node->right = NULL;

    /* move to the next id and parent */
    smpd_process.tree_id++;

    temp = smpd_process.tree_id >> 2;
    bit = 1;
    while (temp)
    {
	bit <<= 1;
	temp >>= 1;
    }
    mask = bit - 1;
    smpd_process.tree_parent = bit | (smpd_process.tree_id & mask);

    /* return the id */
    *id_ptr = node->id;

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_get_next_host"
int smpd_get_next_host(smpd_host_node_t **host_node_pptr, smpd_launch_node_t *launch_node)
{
    int result;
    char host[SMPD_MAX_HOST_LENGTH];
    char alt_host[SMPD_MAX_HOST_LENGTH];
    smpd_host_node_t *host_node_ptr;

    smpd_enter_fn(FCNAME);

    if (host_node_pptr == NULL)
    {
	smpd_err_printf("invalid host_node_pptr argument.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    if (*host_node_pptr == NULL)
    {
	result = smpd_get_next_hostname(host, alt_host);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to get the next available host name\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	result = smpd_get_host_id(host, &launch_node->host_id);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to get a id for host %s\n", host);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	MPIU_Strncpy(launch_node->hostname, host, SMPD_MAX_HOST_LENGTH);
	MPIU_Strncpy(launch_node->alt_hostname, alt_host, SMPD_MAX_HOST_LENGTH);
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    host_node_ptr = *host_node_pptr;
    if (host_node_ptr->nproc == 0)
    {
	(*host_node_pptr) = (*host_node_pptr)->next;
	MPIU_Free(host_node_ptr);
	host_node_ptr = *host_node_pptr;
	if (host_node_ptr == NULL)
	{
	    smpd_err_printf("no more hosts in the list.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    result = smpd_get_host_id(host_node_ptr->host, &launch_node->host_id);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to get a id for host %s\n", host_node_ptr->host);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    MPIU_Strncpy(launch_node->hostname, host_node_ptr->host, SMPD_MAX_HOST_LENGTH);
    MPIU_Strncpy(launch_node->alt_hostname, host_node_ptr->alt_host, SMPD_MAX_HOST_LENGTH);
    host_node_ptr->nproc--;
    if (host_node_ptr->nproc == 0)
    {
	(*host_node_pptr) = (*host_node_pptr)->next;
	MPIU_Free(host_node_ptr);
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_get_argcv_from_file"
SMPD_BOOL smpd_get_argcv_from_file(FILE *fin, int *argcp, char ***argvp)
{
    static char line[SMPD_MAX_LINE_LENGTH];
    static char line_out[SMPD_MAX_LINE_LENGTH];
    static char *argv[SMPD_MAX_ARGC];
    char *iter_line, *iter_out, *last_position;
    int index;
    int num_remaining;

    smpd_enter_fn(FCNAME);

    argv[0] = "bogus.exe";
    while (fgets(line, SMPD_MAX_LINE_LENGTH, fin))
    {
	/* first strip off the \r\n at the end of the line */
	if (line[0] != '\0')
	{
	    iter_line = &line[strlen(line)-1];
	    while ((*iter_line == '\r' || *iter_line == '\n') && (iter_line >= line))
	    {
		*iter_line = '\0';
		iter_line--;
	    }
	}

	iter_out = line_out;
	line_out[0] = '\0';
	iter_line = line;
	index = 1;
	num_remaining = SMPD_MAX_LINE_LENGTH;

	while (iter_line != NULL && (strlen(iter_line) > 0))
	{
	    last_position = iter_line;
	    if (MPIU_Str_get_string(&iter_line, iter_out, num_remaining) == MPIU_STR_SUCCESS)
	    {
		if (last_position == iter_line)
		{
		    /* no string parsed */
		    break;
		}
		argv[index] = iter_out;
		index++;
		if (index == SMPD_MAX_ARGC)
		{
		    argv[SMPD_MAX_ARGC-1] = NULL;
		    break;
		}
		num_remaining = num_remaining - (int)strlen(iter_out) - 1;
		iter_out = &iter_out[strlen(iter_out)+1];
	    }
	    else
	    {
		break;
	    }
	}

	if (index != 1)
	{
	    if (index < SMPD_MAX_ARGC)
		argv[index] = NULL;
	    *argcp = index;
	    *argvp = argv;
	    smpd_exit_fn(FCNAME);
	    return SMPD_TRUE;
	}
    }

    smpd_exit_fn(FCNAME);
    return SMPD_FALSE;
}

#undef FCNAME
#define FCNAME "next_launch_node"
static smpd_launch_node_t *next_launch_node(smpd_launch_node_t *node, int id)
{
    smpd_enter_fn(FCNAME);
    while (node)
    {
	if (node->host_id == id)
	{
	    smpd_exit_fn(FCNAME);
	    return node;
	}
	node = node->next;
    }
    smpd_exit_fn(FCNAME);
    return NULL;
}

#undef FCNAME
#define FCNAME "prev_launch_node"
static smpd_launch_node_t *prev_launch_node(smpd_launch_node_t *node, int id)
{
    smpd_enter_fn(FCNAME);
    while (node)
    {
	if (node->host_id == id)
	{
	    smpd_exit_fn(FCNAME);
	    return node;
	}
	node = node->prev;
    }
    smpd_exit_fn(FCNAME);
    return NULL;
}

#undef FCNAME
#define FCNAME "smpd_create_cliques"
int smpd_create_cliques(smpd_launch_node_t *list)
{
    smpd_launch_node_t *iter, *cur_node;
    int cur_iproc, printed_iproc;
    char *cur_str;

    smpd_enter_fn(FCNAME);

    if (list == NULL)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    if (list->iproc == 0)
    {
	/* in order */
	cur_node = list;
	while (cur_node)
	{
	    /* point to the current structures */
	    printed_iproc = cur_iproc = cur_node->iproc;
	    cur_str = cur_node->clique;
	    cur_str += sprintf(cur_str, "%d", cur_iproc);
	    /* add the ranks of all other nodes with the same id */
	    iter = next_launch_node(cur_node->next, cur_node->host_id);
	    while (iter)
	    {
		if (iter->iproc == cur_iproc + 1)
		{
		    cur_iproc = iter->iproc;
		    iter = next_launch_node(iter->next, iter->host_id);
		    if (iter == NULL)
			cur_str += sprintf(cur_str, "..%d", cur_iproc);
		}
		else
		{
		    if (printed_iproc == cur_iproc)
		    {
			cur_str += sprintf(cur_str, ",%d", iter->iproc);
		    }
		    else
		    {
			cur_str += sprintf(cur_str, "..%d,%d", cur_iproc, iter->iproc);
		    }
		    printed_iproc = cur_iproc = iter->iproc;
		    iter = next_launch_node(iter->next, iter->host_id);
		}
	    }
	    /* copy the clique string to all the nodes with the same id */
	    iter = next_launch_node(cur_node->next, cur_node->host_id);
	    while (iter)
	    {
		strcpy(iter->clique, cur_node->clique);
		iter = next_launch_node(iter->next, iter->host_id);
	    }
	    /* move to the next node that doesn't have a clique string yet */
	    cur_node = cur_node->next;
	    while (cur_node && cur_node->clique[0] != '\0')
		cur_node = cur_node->next;
	}
    }
    else
    {
	/* reverse order */
	cur_node = list;
	/* go to the end of the list */
	while (cur_node->next)
	    cur_node = cur_node->next;
	while (cur_node)
	{
	    /* point to the current structures */
	    printed_iproc = cur_iproc = cur_node->iproc;
	    cur_str = cur_node->clique;
	    cur_str += sprintf(cur_str, "%d", cur_iproc);
	    /* add the ranks of all other nodes with the same id */
	    iter = prev_launch_node(cur_node->prev, cur_node->host_id);
	    while (iter)
	    {
		if (iter->iproc == cur_iproc + 1)
		{
		    cur_iproc = iter->iproc;
		    iter = prev_launch_node(iter->prev, iter->host_id);
		    if (iter == NULL)
			cur_str += sprintf(cur_str, "..%d", cur_iproc);
		}
		else
		{
		    if (printed_iproc == cur_iproc)
		    {
			cur_str += sprintf(cur_str, ",%d", iter->iproc);
		    }
		    else
		    {
			cur_str += sprintf(cur_str, "..%d,%d", cur_iproc, iter->iproc);
		    }
		    printed_iproc = cur_iproc = iter->iproc;
		    iter = prev_launch_node(iter->prev, iter->host_id);
		}
	    }
	    /* copy the clique string to all the nodes with the same id */
	    iter = prev_launch_node(cur_node->prev, cur_node->host_id);
	    while (iter)
	    {
		strcpy(iter->clique, cur_node->clique);
		iter = prev_launch_node(iter->prev, iter->host_id);
	    }
	    /* move to the next node that doesn't have a clique string yet */
	    cur_node = cur_node->prev;
	    while (cur_node && cur_node->clique[0] != '\0')
		cur_node = cur_node->prev;
	}
    }
    /*
    iter = list;
    while (iter)
    {
	printf("clique: <%s>\n", iter->clique);
	iter = iter->next;
    }
    */
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

