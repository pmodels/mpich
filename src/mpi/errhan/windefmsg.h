/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 * This file automatically created by extracterrmsgs
 * DO NOT EDIT
 */
typedef struct msgpair {
        const unsigned int sentinal1;
        const char *short_name, *long_name; 
        const unsigned int sentinal2; } msgpair;
#if MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG_NONE
/* The names are in sorted order, allowing the use of a simple
  linear search or bisection algorithm to find the message corresponding to
  a particular message */
static const char short_gen0[] = "**CreateFileMapping";
static const char long_gen0[]  = "CreateFileMapping failed";
static const char short_gen1[] = "**CreateThread";
static const char long_gen1[]  = "CreateThread failed";
static const char short_gen2[] = "**FindWindowEx";
static const char long_gen2[]  = "FindWindowEx failed";
static const char short_gen3[] = "**GetMemTwice";
static const char long_gen3[]  = "Global shared memory initializer called more than once";
static const char short_gen4[] = "**MPIDI_CH3I_SHM_Attach_to_mem";
static const char long_gen4[]  = "MPIDI_CH3I_SHM_Attach_to_mem failed";
static const char short_gen5[] = "**MPIU_Strdup";
static const char long_gen5[]  = "MPIU_Strdup failed";
static const char short_gen6[] = "**MapViewOfFileEx";
static const char long_gen6[]  = "MapViewOfFileEx failed";
static const char short_gen7[] = "**OpenProcess";
static const char long_gen7[]  = "OpenProcess failed";
static const char short_gen8[] = "**allocmem";
static const char long_gen8[]  = "Unable to allocate memory for MPI_Alloc_mem";
static const char short_gen9[] = "**arg";
static const char long_gen9[]  = "Invalid argument";
static const char short_gen10[] = "**argarrayneg";
static const char long_gen10[]  = "Negative value in array ";
static const char short_gen11[] = "**argerrcode";
static const char long_gen11[]  = "Invalid error code";
static const char short_gen12[] = "**argneg";
static const char long_gen12[]  = "Invalid argument; must be non-negative";
static const char short_gen13[] = "**argnonpos";
static const char long_gen13[]  = "Invalid argument; must be positive";
static const char short_gen14[] = "**argrange";
static const char long_gen14[]  = "Argument is not within valid range";
static const char short_gen15[] = "**argstr_hostd";
static const char long_gen15[]  = "no space for the host description";
static const char short_gen16[] = "**argstr_port";
static const char long_gen16[]  = "no space for the listener port";
static const char short_gen17[] = "**argstr_shmhost";
static const char long_gen17[]  = "no space for the host name";
static const char short_gen18[] = "**argstr_shmq";
static const char long_gen18[]  = "no space for the shared memory queue name";
static const char short_gen19[] = "**assert";
static const char long_gen19[]  = "Invalid assert argument";
static const char short_gen20[] = "**attach_to_mem";
static const char long_gen20[]  = "attach to shared memory segment failed";
static const char short_gen21[] = "**attrsentinal";
static const char long_gen21[]  = "Internal fields in an attribute have been overwritten; possible errors in using the attribute value in user code.";
static const char short_gen22[] = "**bad_conn";
static const char long_gen22[]  = "bad conn structure pointer";
static const char short_gen23[] = "**bad_listenersock";
static const char long_gen23[]  = "bad listener sock";
static const char short_gen24[] = "**bad_set";
static const char long_gen24[]  = "bad set parameter";
static const char short_gen25[] = "**bad_sock";
static const char long_gen25[]  = "bad sock";
static const char short_gen26[] = "**badpacket";
static const char long_gen26[]  = "Received a packet of unknown type";
static const char short_gen27[] = "**base";
static const char long_gen27[]  = "Invalid base address";
static const char short_gen28[] = "**boot_attach";
static const char long_gen28[]  = "failed to attach to a bootstrap queue";
static const char short_gen29[] = "**boot_create";
static const char long_gen29[]  = "unable to create a bootstrap message queue";
static const char short_gen30[] = "**boot_detach";
static const char long_gen30[]  = "detaching from message queue failed";
static const char short_gen31[] = "**boot_recv";
static const char long_gen31[]  = "receiving bootstrap message failed";
static const char short_gen32[] = "**boot_send";
static const char long_gen32[]  = "sending bootstrap message failed";
static const char short_gen33[] = "**boot_tostring";
static const char long_gen33[]  = "unable to get a string representation of the boostrap queue";
static const char short_gen34[] = "**boot_unlink";
static const char long_gen34[]  = "unable to unlink the shared memory message queue";
static const char short_gen35[] = "**bootqmsg";
static const char long_gen35[]  = "invalid bootstrap queue message size";
static const char short_gen36[] = "**bsendbufsmall";
static const char long_gen36[]  = "Buffer size is smaller than MPI_BSEND_OVERHEAD";
static const char short_gen37[] = "**bsendnobuf";
static const char long_gen37[]  = "No buffer to detach. ";
static const char short_gen38[] = "**buf_inplace";
static const char long_gen38[]  = "buffer cannot be MPI_IN_PLACE";
static const char short_gen39[] = "**bufalias";
static const char long_gen39[]  = "Buffers must not be aliased";
static const char short_gen40[] = "**bufbsend";
static const char long_gen40[]  = "Insufficient space in Bsend buffer";
static const char short_gen41[] = "**bufexists";
static const char long_gen41[]  = "Buffer already attached with MPI_BUFFER_ATTACH.";
static const char short_gen42[] = "**buffer";
static const char long_gen42[]  = "Invalid buffer pointer";
static const char short_gen43[] = "**bufnull";
static const char long_gen43[]  = "Null buffer pointer";
static const char short_gen44[] = "**buscard";
static const char long_gen44[]  = "unable to create a business card";
static const char short_gen45[] = "**buscard_len";
static const char long_gen45[]  = "no space left in the business card to add a parameter";
static const char short_gen46[] = "**business_card";
static const char long_gen46[]  = "Invalid business card";
static const char short_gen47[] = "**ca_guids";
static const char long_gen47[]  = "unable to get the guids from the infiniband access layer";
static const char short_gen48[] = "**cancelunknown";
static const char long_gen48[]  = "Attempt to cancel an unknown type of request";
static const char short_gen49[] = "**cartcoordinvalid";
static const char long_gen49[]  = "Cartesian coordinate is invalid (not in range)";
static const char short_gen50[] = "**cartdim";
static const char long_gen50[]  = "Size of Cartesian grid is larger than the size of the communicator";
static const char short_gen51[] = "**ch3_finalize";
static const char long_gen51[]  = "Channel finalization failed";
static const char short_gen52[] = "**ch3_init";
static const char long_gen52[]  = "Channel init failed";
static const char short_gen53[] = "**ch3progress";
static const char long_gen53[]  = "Unable to make message passing progress";
static const char short_gen54[] = "**ch3|badca";
static const char long_gen54[]  = "specified completion action in not known";
static const char short_gen55[] = "**ch3|badmsgtype";
static const char long_gen55[]  = "request contained an invalid message type";
static const char short_gen56[] = "**ch3|badreqtype";
static const char long_gen56[]  = "request contained an invalid request type";
static const char short_gen57[] = "**ch3|canceleager";
static const char long_gen57[]  = "failure occurred while performing local cancellation of a eager message";
static const char short_gen58[] = "**ch3|cancelreq";
static const char long_gen58[]  = "failure occurred while sending remote cancellation request packet";
static const char short_gen59[] = "**ch3|cancelresp";
static const char long_gen59[]  = "failure occurred while attempting to send cancel response packet";
static const char short_gen60[] = "**ch3|cancelrndv";
static const char long_gen60[]  = "failure occurred while performing local cancellation of a rendezvous message";
static const char short_gen61[] = "**ch3|ctspkt";
static const char long_gen61[]  = "failure occurred while attempting to send CTS packet";
static const char short_gen62[] = "**ch3|eagermsg";
static const char long_gen62[]  = "failure occurred while attempting to send an eager message";
static const char short_gen63[] = "**ch3|flowcntlpkt";
static const char long_gen63[]  = "UNIMPLEMENTED: unable to handle flow control packets";
static const char short_gen64[] = "**ch3|loadrecviov";
static const char long_gen64[]  = "failure occurred while loading the receive I/O vector";
static const char short_gen65[] = "**ch3|loadsendiov";
static const char long_gen65[]  = "failure occurred while loading the send I/O vector";
static const char short_gen66[] = "**ch3|nopktcontainermem";
static const char long_gen66[]  = "failed to allocate memory for a packet reorder container";
static const char short_gen67[] = "**ch3|ooocancelreq";
static const char long_gen67[]  = "UNIMPLEMENTED: unable to process out-of-order cancellation requests";
static const char short_gen68[] = "**ch3|pktordered";
static const char long_gen68[]  = "failure occurred while processing a reordered packet";
static const char short_gen69[] = "**ch3|postrecv";
static const char long_gen69[]  = "failure occurred while posting a receive for message data";
static const char short_gen70[] = "**ch3|recvdata";
static const char long_gen70[]  = "failure occurred while attempting to receive message data";
static const char short_gen71[] = "**ch3|rmamsg";
static const char long_gen71[]  = "failure occurred while attempting to send an RMA message";
static const char short_gen72[] = "**ch3|rtspkt";
static const char long_gen72[]  = "failure occurred while attempting to send RTS packet";
static const char short_gen73[] = "**ch3|selfsenddeadlock";
static const char long_gen73[]  = "DEADLOCK: attempting to send a message to the local process without a prior matching receive";
static const char short_gen74[] = "**ch3|senddata";
static const char long_gen74[]  = "failure occurred while attempting to send message data";
static const char short_gen75[] = "**ch3|sock|accept";
static const char long_gen75[]  = "[ch3:sock] accepting a new connection failed";
static const char short_gen76[] = "**ch3|sock|addrinuse";
static const char long_gen76[]  = "[ch3:sock] tcp port already in use";
static const char short_gen77[] = "**ch3|sock|badbuscard";
static const char long_gen77[]  = "[ch3:sock] GetHostAndPort - Invalid business card";
static const char short_gen78[] = "**ch3|sock|badhost";
static const char long_gen78[]  = "[ch3:soc] invalid host description";
static const char short_gen79[] = "**ch3|sock|badpacket";
static const char long_gen79[]  = "[ch3:sock] received packet of unknow type";
static const char short_gen80[] = "**ch3|sock|badsock";
static const char long_gen80[]  = "[ch3:sock] internal error - bad sock";
static const char short_gen81[] = "**ch3|sock|connalloc";
static const char long_gen81[]  = "[ch3:sock] unable to allocate an internal connection structure";
static const char short_gen82[] = "**ch3|sock|connallocfailed";
static const char long_gen82[]  = "[ch3:sock] unable to allocate a connection structure";
static const char short_gen83[] = "**ch3|sock|connclose";
static const char long_gen83[]  = "[ch3:sock] active connection unexpectedly closed";
static const char short_gen84[] = "**ch3|sock|connfailed";
static const char long_gen84[]  = "[ch3:sock] failed to connnect to remote process";
static const char short_gen85[] = "**ch3|sock|connrefused";
static const char long_gen85[]  = "[ch3:sock] connection refused";
static const char short_gen86[] = "**ch3|sock|connterm";
static const char long_gen86[]  = "[ch3:sock] active connection unexpectedly terminated";
static const char short_gen87[] = "**ch3|sock|conparent";
static const char long_gen87[]  = "[ch3:sock] spawned group unable to connect back to the parent";
static const char short_gen88[] = "**ch3|sock|failure";
static const char long_gen88[]  = "[ch3:sock] unknown failure";
static const char short_gen89[] = "**ch3|sock|hostlookup";
static const char long_gen89[]  = "[ch3:sock] hostname lookup failed";
static const char short_gen90[] = "**ch3|sock|immedread";
static const char long_gen90[]  = "immediate read operation failed";
static const char short_gen91[] = "**ch3|sock|immedwrite";
static const char long_gen91[]  = "immediate write operation failed";
static const char short_gen92[] = "**ch3|sock|open_lrecv_data";
static const char long_gen92[]  = "[ch3:sock] failed to handle open lrecv data packet";
static const char short_gen93[] = "**ch3|sock|openlsend";
static const char long_gen93[]  = "[ch3:sock] failed to handle open lsend packet";
static const char short_gen94[] = "**ch3|sock|pmi_finalize";
static const char long_gen94[]  = "PMI_Finalize failed";
static const char short_gen95[] = "**ch3|sock|postconnect";
static const char long_gen95[]  = "[ch3:sock] post connect failed";
static const char short_gen96[] = "**ch3|sock|postread";
static const char long_gen96[]  = "attempt to post a read operation failed";
static const char short_gen97[] = "**ch3|sock|postwrite";
static const char long_gen97[]  = "attempt to post a write operation failed";
static const char short_gen98[] = "**ch3|sock|progress_finalize";
static const char long_gen98[]  = "[ch3:sock] progress_finalize failed";
static const char short_gen99[] = "**ch3|sock|scconnaccept";
static const char long_gen99[]  = "[ch3:sock] failed to handle connection accept packet";
static const char short_gen100[] = "**ch3|sock|scopenresp";
static const char long_gen100[]  = "[ch3:sock] failed to handle open response packet";
static const char short_gen101[] = "**ch3|sock|strdup";
static const char long_gen101[]  = "[ch3:sock] MPIU_Strdup failed";
static const char short_gen102[] = "**ch3|syncack";
static const char long_gen102[]  = "failure occurred while attempting to send eager synchronization packet";
static const char short_gen103[] = "**ch3|unknownpkt";
static const char long_gen103[]  = "received unknown packet type";
static const char short_gen104[] = "**comm";
static const char long_gen104[]  = "Invalid communicator";
static const char short_gen105[] = "**commnotinter";
static const char long_gen105[]  = "An intercommunicator is required but an intracommunicator was provided.";
static const char short_gen106[] = "**commnotintra";
static const char long_gen106[]  = "An intracommunicator is required but an intercommunicator was provided.";
static const char short_gen107[] = "**commnull";
static const char long_gen107[]  = "Null communicator";
static const char short_gen108[] = "**commperm";
static const char long_gen108[]  = "Cannot free permanent communicator";
static const char short_gen109[] = "**conn_still_active";
static const char long_gen109[]  = "connection closed while still active";
static const char short_gen110[] = "**connfailed";
static const char long_gen110[]  = "Failed to connect to remote process";
static const char short_gen111[] = "**connrefused";
static const char long_gen111[]  = "Connection refused";
static const char short_gen112[] = "**conversion";
static const char long_gen112[]  = "An error occurred in a user-defined data conversion function";
static const char short_gen113[] = "**count";
static const char long_gen113[]  = "Invalid count";
static const char short_gen114[] = "**countneg";
static const char long_gen114[]  = "Negative count";
static const char short_gen115[] = "**darrayblock";
static const char long_gen115[]  = "Value of m must be positive for block(m) distribution";
static const char short_gen116[] = "**darrayblock2";
static const char long_gen116[]  = "m * nprocs is less than array_size and is not valid for block(m) distribution (optional arguments: {m*nprocs}, {array_size} (int))";
static const char short_gen117[] = "**darraycyclic";
static const char long_gen117[]  = "Value of m must be positive for a cyclic(m) distribution";
static const char short_gen118[] = "**darraydist";
static const char long_gen118[]  = "For MPI_DISTRIBUTE_NONE, the number of processes in that dimension of the grid must be 1";
static const char short_gen119[] = "**darrayunknown";
static const char long_gen119[]  = "Unknown distribution type ";
static const char short_gen120[] = "**datarep";
static const char long_gen120[]  = "The requested datarep name has already been specified to MPI_REGISTER_DATAREP";
static const char short_gen121[] = "**datarepunsupported";
static const char long_gen121[]  = "Unsupported datarep passed to MPI_File_set_view ";
static const char short_gen122[] = "**desc_len";
static const char long_gen122[]  = "host description buffer too small";
static const char short_gen123[] = "**dims";
static const char long_gen123[]  = "Invalid dimension argument";
static const char short_gen124[] = "**dimsmany";
static const char long_gen124[]  = "Number of dimensions is too large ";
static const char short_gen125[] = "**dimspartition";
static const char long_gen125[]  = "Cannot partition nodes as requested ";
static const char short_gen126[] = "**dtype";
static const char long_gen126[]  = "Invalid datatype";
static const char short_gen127[] = "**dtypecommit";
static const char long_gen127[]  = "Datatype has not been committed ";
static const char short_gen128[] = "**dtypemismatch";
static const char long_gen128[]  = "Receiving data with a datatype whose signature does not match that of the sending datatype.";
static const char short_gen129[] = "**dtypenull";
static const char long_gen129[]  = "Null datatype";
static const char short_gen130[] = "**dtypeperm";
static const char long_gen130[]  = "Cannot free permanent data type ";
static const char short_gen131[] = "**duphandle";
static const char long_gen131[]  = "unable to duplicate a handle";
static const char short_gen132[] = "**dupprocesses";
static const char long_gen132[]  = "Local and remote groups in MPI_Intercomm_create must not contain the same processes";
static const char short_gen133[] = "**edgeoutrange";
static const char long_gen133[]  = "Edge index in graph topology is out of range";
static const char short_gen134[] = "**errhandnotfile";
static const char long_gen134[]  = "Error handler is not a file error handler";
static const char short_gen135[] = "**errhandnotwin";
static const char long_gen135[]  = "Error handler is not a win error handler";
static const char short_gen136[] = "**fail";
static const char long_gen136[]  = "";
static const char short_gen137[] = "**file";
static const char long_gen137[]  = "Invalid MPI_File";
static const char short_gen138[] = "**fileaccess";
static const char long_gen138[]  = "Access denied to file";
static const char short_gen139[] = "**fileamode";
static const char long_gen139[]  = "Invalid amode value in MPI_File_open ";
static const char short_gen140[] = "**fileamodeone";
static const char long_gen140[]  = "Exactly one of MPI_MODE_RDONLY, MPI_MODE_WRONLY, or MPI_MODE_RDWR must be specified";
static const char short_gen141[] = "**fileamoderead";
static const char long_gen141[]  = "Cannot use MPI_MODE_CREATE or MPI_MODE_EXCL with MPI_MODE_RDONLY ";
static const char short_gen142[] = "**fileamodeseq";
static const char long_gen142[]  = "Cannot specify MPI_MODE_SEQUENTIAL with MPI_MODE_RDWR";
static const char short_gen143[] = "**fileexist";
static const char long_gen143[]  = "File exists";
static const char short_gen144[] = "**fileinuse";
static const char long_gen144[]  = "File in use by some process";
static const char short_gen145[] = "**filename";
static const char long_gen145[]  = "Invalid file name";
static const char short_gen146[] = "**filenamedir";
static const char long_gen146[]  = "Invalid or missing directory";
static const char short_gen147[] = "**filenamelong";
static const char long_gen147[]  = "Pathname too long";
static const char short_gen148[] = "**filenoexist";
static const char long_gen148[]  = "File does not exist";
static const char short_gen149[] = "**filenospace";
static const char long_gen149[]  = "Not enough space for file ";
static const char short_gen150[] = "**fileopunsupported";
static const char long_gen150[]  = "Unsupported file operation ";
static const char short_gen151[] = "**filequota";
static const char long_gen151[]  = "Quota exceeded for files";
static const char short_gen152[] = "**filerdonly";
static const char long_gen152[]  = "Read-only file or filesystem name";
static const char short_gen153[] = "**finalize_boot";
static const char long_gen153[]  = "destroying the message queue failed";
static const char short_gen154[] = "**finalize_progress";
static const char long_gen154[]  = "finalizing the progress engine failed";
static const char short_gen155[] = "**finalize_progress_finalize";
static const char long_gen155[]  = "Progress finalize failed";
static const char short_gen156[] = "**finalize_release_mem";
static const char long_gen156[]  = "Release shared memory failed";
static const char short_gen157[] = "**finish_qp";
static const char long_gen157[]  = "unable to establish a queue pair connection";
static const char short_gen158[] = "**ftok";
static const char long_gen158[]  = "failed to create a sysv key from a file name";
static const char short_gen159[] = "**ftruncate";
static const char long_gen159[]  = "unable to resize the shared memory object";
static const char short_gen160[] = "**get_guids";
static const char long_gen160[]  = "unable to get the guids for the infiniband channel adapter";
static const char short_gen161[] = "**gethostbyname";
static const char long_gen161[]  = "gethostbyname failed";
static const char short_gen162[] = "**getinfo";
static const char long_gen162[]  = "getaddrinfo failed";
static const char short_gen163[] = "**graphnnodes";
static const char long_gen163[]  = "Number of graph nodes exceeds size of communicator.";
static const char short_gen164[] = "**group";
static const char long_gen164[]  = "Invalid group";
static const char short_gen165[] = "**groupnotincomm";
static const char long_gen165[]  = "Specified group is not within the communicator";
static const char short_gen166[] = "**handle_sock_op";
static const char long_gen166[]  = "handle_sock_op failed";
static const char short_gen167[] = "**handle_written";
static const char long_gen167[]  = "unable to handle written data";
static const char short_gen168[] = "**hostlookup";
static const char long_gen168[]  = "Host lookup failed";
static const char short_gen169[] = "**ibu_op";
static const char long_gen169[]  = "invalid infiniband operation";
static const char short_gen170[] = "**ibu_wait";
static const char long_gen170[]  = "ibu_wait failed";
static const char short_gen171[] = "**ibwrite";
static const char long_gen171[]  = "infiniband write failed";
static const char short_gen172[] = "**indexneg";
static const char long_gen172[]  = "Index value in graph topology must be nonnegative";
static const char short_gen173[] = "**indexnonmonotone";
static const char long_gen173[]  = "Index values in graph topology must be monotone nondecreasing";
static const char short_gen174[] = "**info";
static const char long_gen174[]  = "Invalid MPI_Info";
static const char short_gen175[] = "**infokey";
static const char long_gen175[]  = "Invalid key for MPI_Info ";
static const char short_gen176[] = "**infokeyempty";
static const char long_gen176[]  = "Empty or blank key ";
static const char short_gen177[] = "**infokeylong";
static const char long_gen177[]  = "Key is too long";
static const char short_gen178[] = "**infokeynull";
static const char long_gen178[]  = "Null key";
static const char short_gen179[] = "**infonkey";
static const char long_gen179[]  = "Requested nth key does not exist";
static const char short_gen180[] = "**infonokey";
static const char long_gen180[]  = "MPI_Info key is not defined ";
static const char short_gen181[] = "**infoval";
static const char long_gen181[]  = "Invalid MPI_Info value ";
static const char short_gen182[] = "**infovallong";
static const char long_gen182[]  = "Value is too long ";
static const char short_gen183[] = "**infovalnull";
static const char long_gen183[]  = "Null value";
static const char short_gen184[] = "**init";
static const char long_gen184[]  = "Initialization failed";
static const char short_gen185[] = "**init_buscard";
static const char long_gen185[]  = "failed to get my business card";
static const char short_gen186[] = "**init_description";
static const char long_gen186[]  = "unable to get the host description";
static const char short_gen187[] = "**init_getptr";
static const char long_gen187[]  = "failed to get the vcr";
static const char short_gen188[] = "**init_ibu";
static const char long_gen188[]  = "infiniband initialization failed";
static const char short_gen189[] = "**init_ibu_set";
static const char long_gen189[]  = "unable to create an infinband completion queue";
static const char short_gen190[] = "**init_progress";
static const char long_gen190[]  = "progress_init failed";
static const char short_gen191[] = "**init_vcrdup";
static const char long_gen191[]  = "failed to duplicate the virtual connection reference";
static const char short_gen192[] = "**init_vcrt";
static const char long_gen192[]  = "failed to create VCRT";
static const char short_gen193[] = "**initialized";
static const char long_gen193[]  = "MPI not initialized. Call MPI_Init or MPI_Init_thread first";
static const char short_gen194[] = "**inittwice";
static const char long_gen194[]  = "Cannot call MPI_INIT or MPI_INIT_THREAD more than once";
static const char short_gen195[] = "**inpending";
static const char long_gen195[]  = "Pending request (no error)";
static const char short_gen196[] = "**instatus";
static const char long_gen196[]  = "See the MPI_ERROR field in MPI_Status for the error code";
static const char short_gen197[] = "**intern";
static const char long_gen197[]  = "Internal MPI error!";
static const char short_gen198[] = "**inttoosmall";
static const char long_gen198[]  = "An address does not fit into a Fortran INTEGER.  Use MPI_Get_address instead";
static const char short_gen199[] = "**invalid_handle";
static const char long_gen199[]  = "invalid handle";
static const char short_gen200[] = "**invalid_listener";
static const char long_gen200[]  = "invalid listener";
static const char short_gen201[] = "**invalid_refcount";
static const char long_gen201[]  = "invalid reference count";
static const char short_gen202[] = "**io";
static const char long_gen202[]  = "Other I/O error ";
static const char short_gen203[] = "**ioRMWrdwr";
static const char long_gen203[]  = "Must open file with MPI_MODE_RDWR for read-modify-write ";
static const char short_gen204[] = "**ioagnomatch";
static const char long_gen204[]  = "No aggregators match";
static const char short_gen205[] = "**ioamodeseq";
static const char long_gen205[]  = "Cannot use this function when the file is opened with amode MPI_MODE_SEQUENTIAL ";
static const char short_gen206[] = "**iobadcount";
static const char long_gen206[]  = "Invalid count argument";
static const char short_gen207[] = "**iobaddisp";
static const char long_gen207[]  = "Invalid displacement argument";
static const char short_gen208[] = "**iobadfh";
static const char long_gen208[]  = "Invalid file handle";
static const char short_gen209[] = "**iobadoffset";
static const char long_gen209[]  = "Invalid offset argument";
static const char short_gen210[] = "**iobadsize";
static const char long_gen210[]  = "Invalid size argument";
static const char short_gen211[] = "**iobadwhence";
static const char long_gen211[]  = "Invalid whence argument";
static const char short_gen212[] = "**iocp";
static const char long_gen212[]  = "unable to create an I/O completion port";
static const char short_gen213[] = "**iodatarepnomem";
static const char long_gen213[]  = "User must allocate memory for datarep";
static const char short_gen214[] = "**iodispifseq";
static const char long_gen214[]  = "disp must be set to MPI_DISPLACEMENT_CURRENT since file was opened with MPI_MODE_SEQUENTIAL";
static const char short_gen215[] = "**ioetype";
static const char long_gen215[]  = "Only an integral number of etypes can be accessed";
static const char short_gen216[] = "**iofilecorrupt";
static const char long_gen216[]  = "File corrupt";
static const char short_gen217[] = "**iofilenull";
static const char long_gen217[]  = "Null file handle";
static const char short_gen218[] = "**iofiletype";
static const char long_gen218[]  = "Filetype must be constructed out of one or more etypes";
static const char short_gen219[] = "**iofstype";
static const char long_gen219[]  = "Cannot determine filesystem type";
static const char short_gen220[] = "**iofstypeunsupported";
static const char long_gen220[]  = "Specified filesystem is not available";
static const char short_gen221[] = "**ioneedrd";
static const char long_gen221[]  = "Read access is required to this file";
static const char short_gen222[] = "**ionegoffset";
static const char long_gen222[]  = "Negative offset argument";
static const char short_gen223[] = "**iopreallocrdwr";
static const char long_gen223[]  = "Must open file with MPI_MODE_RDWR to preallocate disk space";
static const char short_gen224[] = "**iosequnsupported";
static const char long_gen224[]  = "MPI_MODE_SEQUENTIAL not supported on this file system";
static const char short_gen225[] = "**iosharedfailed";
static const char long_gen225[]  = "Could not access shared file pointer";
static const char short_gen226[] = "**iosharedunsupported";
static const char long_gen226[]  = "Shared file pointers not supported";
static const char short_gen227[] = "**iosplitcoll";
static const char long_gen227[]  = "Only one active split collective I/O operation is allowed per file handle";
static const char short_gen228[] = "**iosplitcollnone";
static const char long_gen228[]  = "No split collective I/O operation is active";
static const char short_gen229[] = "**iov_offset";
static const char long_gen229[]  = "invalid iov offset";
static const char short_gen230[] = "**keyval";
static const char long_gen230[]  = "Invalid keyval";
static const char short_gen231[] = "**keyvalinvalid";
static const char long_gen231[]  = "Attribute key was MPI_KEYVAL_INVALID";
static const char short_gen232[] = "**keyvalnotcomm";
static const char long_gen232[]  = "Keyval was not defined for communicators";
static const char short_gen233[] = "**keyvalnotdatatype";
static const char long_gen233[]  = "Keyval was not defined for datatype";
static const char short_gen234[] = "**keyvalnotwin";
static const char long_gen234[]  = "Keyval was not defined for window objects";
static const char short_gen235[] = "**listen";
static const char long_gen235[]  = "listen failed";
static const char short_gen236[] = "**locktype";
static const char long_gen236[]  = "Invalid lock type";
static const char short_gen237[] = "**mmap";
static const char long_gen237[]  = "unable to map memory, mmap failed";
static const char short_gen238[] = "**mpi_abort";
static const char long_gen238[]  = "MPI_Abort failed";
static const char short_gen239[] = "**mpi_accumulate";
static const char long_gen239[]  = "MPI_Accumulate failed";
static const char short_gen240[] = "**mpi_add_error_class";
static const char long_gen240[]  = "MPI_Add_error_class failed";
static const char short_gen241[] = "**mpi_add_error_code";
static const char long_gen241[]  = "MPI_Add_error_code failed";
static const char short_gen242[] = "**mpi_add_error_string";
static const char long_gen242[]  = "MPI_Add_error_string failed";
static const char short_gen243[] = "**mpi_address";
static const char long_gen243[]  = "MPI_Address failed";
static const char short_gen244[] = "**mpi_allgather";
static const char long_gen244[]  = "MPI_Allgather failed";
static const char short_gen245[] = "**mpi_allgatherv";
static const char long_gen245[]  = "MPI_Allgatherv failed";
static const char short_gen246[] = "**mpi_alloc_mem";
static const char long_gen246[]  = "MPI_Alloc_mem failed";
static const char short_gen247[] = "**mpi_allreduce";
static const char long_gen247[]  = "MPI_Allreduce failed";
static const char short_gen248[] = "**mpi_alltoall";
static const char long_gen248[]  = "MPI_Alltoall failed";
static const char short_gen249[] = "**mpi_alltoallv";
static const char long_gen249[]  = "MPI_Alltoallv failed";
static const char short_gen250[] = "**mpi_alltoallw";
static const char long_gen250[]  = "MPI_Alltoallw failed";
static const char short_gen251[] = "**mpi_attr_delete";
static const char long_gen251[]  = "MPI_Attr_delete failed";
static const char short_gen252[] = "**mpi_attr_get";
static const char long_gen252[]  = "MPI_Attr_get failed";
static const char short_gen253[] = "**mpi_attr_put";
static const char long_gen253[]  = "MPI_Attr_put failed";
static const char short_gen254[] = "**mpi_barrier";
static const char long_gen254[]  = "MPI_Barrier failed";
static const char short_gen255[] = "**mpi_bcast";
static const char long_gen255[]  = "MPI_Bcast failed";
static const char short_gen256[] = "**mpi_bsend";
static const char long_gen256[]  = "MPI_Bsend failed";
static const char short_gen257[] = "**mpi_bsend_init";
static const char long_gen257[]  = "MPI_Bsend_init failed";
static const char short_gen258[] = "**mpi_buffer_attach";
static const char long_gen258[]  = "MPI_Buffer_attach failed";
static const char short_gen259[] = "**mpi_buffer_detach";
static const char long_gen259[]  = "MPI_Buffer_detach failed";
static const char short_gen260[] = "**mpi_cancel";
static const char long_gen260[]  = "MPI_Cancel failed";
static const char short_gen261[] = "**mpi_cart_coords";
static const char long_gen261[]  = "MPI_Cart_coords failed";
static const char short_gen262[] = "**mpi_cart_create";
static const char long_gen262[]  = "MPI_Cart_create failed";
static const char short_gen263[] = "**mpi_cart_get";
static const char long_gen263[]  = "MPI_Cart_get failed";
static const char short_gen264[] = "**mpi_cart_map";
static const char long_gen264[]  = "MPI_Cart_map failed";
static const char short_gen265[] = "**mpi_cart_rank";
static const char long_gen265[]  = "MPI_Cart_rank failed";
static const char short_gen266[] = "**mpi_cart_shift";
static const char long_gen266[]  = "MPI_Cart_shift failed";
static const char short_gen267[] = "**mpi_cart_sub";
static const char long_gen267[]  = "MPI_Cart_sub failed";
static const char short_gen268[] = "**mpi_cartdim_get";
static const char long_gen268[]  = "MPI_Cartdim_get failed";
static const char short_gen269[] = "**mpi_close_port";
static const char long_gen269[]  = "MPI_Close_port failed";
static const char short_gen270[] = "**mpi_comm_accept";
static const char long_gen270[]  = "MPI_Comm_accept failed";
static const char short_gen271[] = "**mpi_comm_call_errhandler";
static const char long_gen271[]  = "MPI_Comm_call_errhandler failed";
static const char short_gen272[] = "**mpi_comm_compare";
static const char long_gen272[]  = "MPI_Comm_compare failed";
static const char short_gen273[] = "**mpi_comm_connect";
static const char long_gen273[]  = "MPI_Comm_connect failed";
static const char short_gen274[] = "**mpi_comm_create";
static const char long_gen274[]  = "MPI_Comm_create failed";
static const char short_gen275[] = "**mpi_comm_create_errhandler";
static const char long_gen275[]  = "MPI_Comm_create_errhandler failed";
static const char short_gen276[] = "**mpi_comm_create_keyval";
static const char long_gen276[]  = "MPI_Comm_create_keyval failed";
static const char short_gen277[] = "**mpi_comm_delete_attr";
static const char long_gen277[]  = "MPI_Comm_delete_attr failed";
static const char short_gen278[] = "**mpi_comm_disconnect";
static const char long_gen278[]  = "MPI_Comm_disconnect failed";
static const char short_gen279[] = "**mpi_comm_dup";
static const char long_gen279[]  = "MPI_Comm_dup failed";
static const char short_gen280[] = "**mpi_comm_free";
static const char long_gen280[]  = "MPI_Comm_free failed";
static const char short_gen281[] = "**mpi_comm_free_keyval";
static const char long_gen281[]  = "MPI_Comm_free_keyval failed";
static const char short_gen282[] = "**mpi_comm_get_attr";
static const char long_gen282[]  = "MPI_Comm_get_attr failed";
static const char short_gen283[] = "**mpi_comm_get_errhandler";
static const char long_gen283[]  = "MPI_Comm_get_errhandler failed";
static const char short_gen284[] = "**mpi_comm_get_name";
static const char long_gen284[]  = "MPI_Comm_get_name failed";
static const char short_gen285[] = "**mpi_comm_get_parent";
static const char long_gen285[]  = "MPI_Comm_get_parent failed";
static const char short_gen286[] = "**mpi_comm_group";
static const char long_gen286[]  = "MPI_Comm_group failed";
static const char short_gen287[] = "**mpi_comm_join";
static const char long_gen287[]  = "MPI_Comm_join failed";
static const char short_gen288[] = "**mpi_comm_rank";
static const char long_gen288[]  = "MPI_Comm_rank failed";
static const char short_gen289[] = "**mpi_comm_remote_group";
static const char long_gen289[]  = "MPI_Comm_remote_group failed";
static const char short_gen290[] = "**mpi_comm_remote_size";
static const char long_gen290[]  = "MPI_Comm_remote_size failed";
static const char short_gen291[] = "**mpi_comm_set_attr";
static const char long_gen291[]  = "MPI_Comm_set_attr failed";
static const char short_gen292[] = "**mpi_comm_set_errhandler";
static const char long_gen292[]  = "MPI_Comm_set_errhandler failed";
static const char short_gen293[] = "**mpi_comm_set_name";
static const char long_gen293[]  = "MPI_Comm_set_name failed";
static const char short_gen294[] = "**mpi_comm_size";
static const char long_gen294[]  = "MPI_Comm_size failed";
static const char short_gen295[] = "**mpi_comm_spawn";
static const char long_gen295[]  = "MPI_Comm_spawn failed";
static const char short_gen296[] = "**mpi_comm_spawn_multiple";
static const char long_gen296[]  = "MPI_Comm_spawn_multiple failed";
static const char short_gen297[] = "**mpi_comm_split";
static const char long_gen297[]  = "MPI_Comm_split failed";
static const char short_gen298[] = "**mpi_comm_test_inter";
static const char long_gen298[]  = "MPI_Comm_test_inter failed";
static const char short_gen299[] = "**mpi_dims_create";
static const char long_gen299[]  = "MPI_Dims_create failed";
static const char short_gen300[] = "**mpi_errhandler_create";
static const char long_gen300[]  = "MPI_Errhandler_create failed";
static const char short_gen301[] = "**mpi_errhandler_free";
static const char long_gen301[]  = "MPI_Errhandler_free failed";
static const char short_gen302[] = "**mpi_errhandler_get";
static const char long_gen302[]  = "MPI_Errhandler_get failed";
static const char short_gen303[] = "**mpi_errhandler_set";
static const char long_gen303[]  = "MPI_Errhandler_set failed";
static const char short_gen304[] = "**mpi_error_class";
static const char long_gen304[]  = "MPI_Error_class failed";
static const char short_gen305[] = "**mpi_error_string";
static const char long_gen305[]  = "MPI_Error_string failed";
static const char short_gen306[] = "**mpi_exscan";
static const char long_gen306[]  = "MPI_Exscan failed";
static const char short_gen307[] = "**mpi_file_call_errhandler";
static const char long_gen307[]  = "MPI_File_call_errhandler failed";
static const char short_gen308[] = "**mpi_file_create_errhandler";
static const char long_gen308[]  = "MPI_File_create_errhandler failed";
static const char short_gen309[] = "**mpi_file_get_errhandler";
static const char long_gen309[]  = "MPI_File_get_errhandler failed";
static const char short_gen310[] = "**mpi_file_set_errhandler";
static const char long_gen310[]  = "MPI_File_set_errhandler failed";
static const char short_gen311[] = "**mpi_finalize";
static const char long_gen311[]  = "MPI_Finalize failed";
static const char short_gen312[] = "**mpi_finalized";
static const char long_gen312[]  = "MPI_Finalized failed";
static const char short_gen313[] = "**mpi_free_mem";
static const char long_gen313[]  = "MPI_Free_mem failed";
static const char short_gen314[] = "**mpi_gather";
static const char long_gen314[]  = "MPI_Gather failed";
static const char short_gen315[] = "**mpi_gatherv";
static const char long_gen315[]  = "MPI_Gatherv failed";
static const char short_gen316[] = "**mpi_get";
static const char long_gen316[]  = "MPI_Get failed";
static const char short_gen317[] = "**mpi_get_address";
static const char long_gen317[]  = "MPI_Get_address failed";
static const char short_gen318[] = "**mpi_get_count";
static const char long_gen318[]  = "MPI_Get_count failed";
static const char short_gen319[] = "**mpi_get_elements";
static const char long_gen319[]  = "MPI_Get_elements failed";
static const char short_gen320[] = "**mpi_get_processor_name";
static const char long_gen320[]  = "MPI_Get_processor_name failed";
static const char short_gen321[] = "**mpi_get_version";
static const char long_gen321[]  = "MPI_Get_version failed";
static const char short_gen322[] = "**mpi_graph_create";
static const char long_gen322[]  = "MPI_Graph_create failed";
static const char short_gen323[] = "**mpi_graph_get";
static const char long_gen323[]  = "MPI_Graph_get failed";
static const char short_gen324[] = "**mpi_graph_map";
static const char long_gen324[]  = "MPI_Graph_map failed";
static const char short_gen325[] = "**mpi_graph_neighbors";
static const char long_gen325[]  = "MPI_Graph_neighbors failed";
static const char short_gen326[] = "**mpi_graph_neighbors_count";
static const char long_gen326[]  = "MPI_Graph_neighbors_count failed";
static const char short_gen327[] = "**mpi_graphdims_get";
static const char long_gen327[]  = "MPI_Graphdims_get failed";
static const char short_gen328[] = "**mpi_grequest_complete";
static const char long_gen328[]  = "MPI_Grequest_complete failed";
static const char short_gen329[] = "**mpi_grequest_start";
static const char long_gen329[]  = "MPI_Grequest_start failed";
static const char short_gen330[] = "**mpi_group_compare";
static const char long_gen330[]  = "MPI_Group_compare failed";
static const char short_gen331[] = "**mpi_group_difference";
static const char long_gen331[]  = "MPI_Group_difference failed";
static const char short_gen332[] = "**mpi_group_excl";
static const char long_gen332[]  = "MPI_Group_excl failed";
static const char short_gen333[] = "**mpi_group_free";
static const char long_gen333[]  = "MPI_Group_free failed";
static const char short_gen334[] = "**mpi_group_incl";
static const char long_gen334[]  = "MPI_Group_incl failed";
static const char short_gen335[] = "**mpi_group_intersection";
static const char long_gen335[]  = "MPI_Group_intersection failed";
static const char short_gen336[] = "**mpi_group_range_excl";
static const char long_gen336[]  = "MPI_Group_range_excl failed";
static const char short_gen337[] = "**mpi_group_range_incl";
static const char long_gen337[]  = "MPI_Group_range_incl failed";
static const char short_gen338[] = "**mpi_group_rank";
static const char long_gen338[]  = "MPI_Group_rank failed";
static const char short_gen339[] = "**mpi_group_size";
static const char long_gen339[]  = "MPI_Group_size failed";
static const char short_gen340[] = "**mpi_group_translate_ranks";
static const char long_gen340[]  = "MPI_Group_translate_ranks failed";
static const char short_gen341[] = "**mpi_group_union";
static const char long_gen341[]  = "MPI_Group_union failed";
static const char short_gen342[] = "**mpi_ibsend";
static const char long_gen342[]  = "MPI_Ibsend failed";
static const char short_gen343[] = "**mpi_info_create";
static const char long_gen343[]  = "MPI_Info_create failed";
static const char short_gen344[] = "**mpi_info_delete";
static const char long_gen344[]  = "MPI_Info_delete failed";
static const char short_gen345[] = "**mpi_info_dup";
static const char long_gen345[]  = "MPI_Info_dup failed";
static const char short_gen346[] = "**mpi_info_free";
static const char long_gen346[]  = "MPI_Info_free failed";
static const char short_gen347[] = "**mpi_info_get";
static const char long_gen347[]  = "MPI_Info_get failed";
static const char short_gen348[] = "**mpi_info_get_nkeys";
static const char long_gen348[]  = "MPI_Info_get_nkeys failed";
static const char short_gen349[] = "**mpi_info_get_nthkey";
static const char long_gen349[]  = "MPI_Info_get_nthkey failed";
static const char short_gen350[] = "**mpi_info_get_valuelen";
static const char long_gen350[]  = "MPI_Info_get_valuelen failed";
static const char short_gen351[] = "**mpi_info_set";
static const char long_gen351[]  = "MPI_Info_set failed";
static const char short_gen352[] = "**mpi_init";
static const char long_gen352[]  = "MPI_Init failed";
static const char short_gen353[] = "**mpi_init_thread";
static const char long_gen353[]  = "MPI_Init_thread failed";
static const char short_gen354[] = "**mpi_initialized";
static const char long_gen354[]  = "MPI_Initialized failed";
static const char short_gen355[] = "**mpi_intercomm_create";
static const char long_gen355[]  = "MPI_Intercomm_create failed";
static const char short_gen356[] = "**mpi_intercomm_merge";
static const char long_gen356[]  = "MPI_Intercomm_merge failed";
static const char short_gen357[] = "**mpi_iprobe";
static const char long_gen357[]  = "MPI_Iprobe failed";
static const char short_gen358[] = "**mpi_irecv";
static const char long_gen358[]  = "MPI_Irecv failed";
static const char short_gen359[] = "**mpi_irsend";
static const char long_gen359[]  = "MPI_Irsend failed";
static const char short_gen360[] = "**mpi_is_thread_main";
static const char long_gen360[]  = "MPI_Is_thread_main failed";
static const char short_gen361[] = "**mpi_isend";
static const char long_gen361[]  = "MPI_Isend failed";
static const char short_gen362[] = "**mpi_issend";
static const char long_gen362[]  = "MPI_Issend failed";
static const char short_gen363[] = "**mpi_keyval_create";
static const char long_gen363[]  = "MPI_Keyval_create failed";
static const char short_gen364[] = "**mpi_keyval_free";
static const char long_gen364[]  = "MPI_Keyval_free failed";
static const char short_gen365[] = "**mpi_lookup_name";
static const char long_gen365[]  = "MPI_Lookup_name failed";
static const char short_gen366[] = "**mpi_op_create";
static const char long_gen366[]  = "MPI_Op_create failed";
static const char short_gen367[] = "**mpi_op_free";
static const char long_gen367[]  = "MPI_Op_free failed";
static const char short_gen368[] = "**mpi_open_port";
static const char long_gen368[]  = "MPI_Open_port failed";
static const char short_gen369[] = "**mpi_pack";
static const char long_gen369[]  = "MPI_Pack failed";
static const char short_gen370[] = "**mpi_pack_external";
static const char long_gen370[]  = "MPI_Pack_external failed";
static const char short_gen371[] = "**mpi_pack_external_size";
static const char long_gen371[]  = "MPI_Pack_external_size failed";
static const char short_gen372[] = "**mpi_pack_size";
static const char long_gen372[]  = "MPI_Pack_size failed";
static const char short_gen373[] = "**mpi_pcontrol";
static const char long_gen373[]  = "MPI_Pcontrol failed";
static const char short_gen374[] = "**mpi_probe";
static const char long_gen374[]  = "MPI_Probe failed";
static const char short_gen375[] = "**mpi_publish_name";
static const char long_gen375[]  = "MPI_Publish_name failed";
static const char short_gen376[] = "**mpi_put";
static const char long_gen376[]  = "MPI_Put failed";
static const char short_gen377[] = "**mpi_query_thread";
static const char long_gen377[]  = "MPI_Query_thread failed";
static const char short_gen378[] = "**mpi_recv";
static const char long_gen378[]  = "MPI_Recv failed";
static const char short_gen379[] = "**mpi_recv_init";
static const char long_gen379[]  = "MPI_Recv_init failed";
static const char short_gen380[] = "**mpi_reduce";
static const char long_gen380[]  = "MPI_Reduce failed";
static const char short_gen381[] = "**mpi_reduce_scatter";
static const char long_gen381[]  = "MPI_Reduce_scatter failed";
static const char short_gen382[] = "**mpi_register_datarep";
static const char long_gen382[]  = "MPI_Register_datarep failed";
static const char short_gen383[] = "**mpi_request_free";
static const char long_gen383[]  = "MPI_Request_free failed";
static const char short_gen384[] = "**mpi_request_get_status";
static const char long_gen384[]  = "MPI_Request_get_status failed";
static const char short_gen385[] = "**mpi_rsend";
static const char long_gen385[]  = "MPI_Rsend failed";
static const char short_gen386[] = "**mpi_rsend_init";
static const char long_gen386[]  = "MPI_Rsend_init failed";
static const char short_gen387[] = "**mpi_scan";
static const char long_gen387[]  = "MPI_Scan failed";
static const char short_gen388[] = "**mpi_scatter";
static const char long_gen388[]  = "MPI_Scatter failed";
static const char short_gen389[] = "**mpi_scatterv";
static const char long_gen389[]  = "MPI_Scatterv failed";
static const char short_gen390[] = "**mpi_send";
static const char long_gen390[]  = "MPI_Send failed";
static const char short_gen391[] = "**mpi_send_init";
static const char long_gen391[]  = "MPI_Send_init failed";
static const char short_gen392[] = "**mpi_sendrecv";
static const char long_gen392[]  = "MPI_Sendrecv failed";
static const char short_gen393[] = "**mpi_sendrecv_replace";
static const char long_gen393[]  = "MPI_Sendrecv_replace failed";
static const char short_gen394[] = "**mpi_ssend";
static const char long_gen394[]  = "MPI_Ssend failed";
static const char short_gen395[] = "**mpi_ssend_init";
static const char long_gen395[]  = "MPI_Ssend_init failed";
static const char short_gen396[] = "**mpi_start";
static const char long_gen396[]  = "MPI_Start failed";
static const char short_gen397[] = "**mpi_startall";
static const char long_gen397[]  = "MPI_Start_all failed";
static const char short_gen398[] = "**mpi_status_set_cancelled";
static const char long_gen398[]  = "MPI_Status_set_cancelled failed";
static const char short_gen399[] = "**mpi_status_set_elements";
static const char long_gen399[]  = "MPI_Status_set_elements failed";
static const char short_gen400[] = "**mpi_test";
static const char long_gen400[]  = "MPI_Test failed";
static const char short_gen401[] = "**mpi_test_cancelled";
static const char long_gen401[]  = "MPI_Test_cancelled failed";
static const char short_gen402[] = "**mpi_testall";
static const char long_gen402[]  = "MPI_Testall failed";
static const char short_gen403[] = "**mpi_testany";
static const char long_gen403[]  = "MPI_Testany failed";
static const char short_gen404[] = "**mpi_testsome";
static const char long_gen404[]  = "MPI_Testsome failed";
static const char short_gen405[] = "**mpi_topo_test";
static const char long_gen405[]  = "MPI_Topo_test failed";
static const char short_gen406[] = "**mpi_type_commit";
static const char long_gen406[]  = "MPI_Type_commit failed";
static const char short_gen407[] = "**mpi_type_contiguous";
static const char long_gen407[]  = "MPI_Type_continuous failed";
static const char short_gen408[] = "**mpi_type_create_darray";
static const char long_gen408[]  = "MPI_Type_create_darray failed";
static const char short_gen409[] = "**mpi_type_create_hindexed";
static const char long_gen409[]  = "MPI_Type_create_hindexed failed";
static const char short_gen410[] = "**mpi_type_create_hvector";
static const char long_gen410[]  = "MPI_Type_create_hvector failed";
static const char short_gen411[] = "**mpi_type_create_indexed_block";
static const char long_gen411[]  = "MPI_Type_create_indexed_block failed";
static const char short_gen412[] = "**mpi_type_create_keyval";
static const char long_gen412[]  = "MPI_Type_create_keyval failed";
static const char short_gen413[] = "**mpi_type_create_resized";
static const char long_gen413[]  = "MPI_Type_create_resized failed";
static const char short_gen414[] = "**mpi_type_create_struct";
static const char long_gen414[]  = "MPI_Type_create_struct failed";
static const char short_gen415[] = "**mpi_type_create_subarray";
static const char long_gen415[]  = "MPI_Type_create_subarray failed";
static const char short_gen416[] = "**mpi_type_delete_attr";
static const char long_gen416[]  = "MPI_Type_delete_attr failed";
static const char short_gen417[] = "**mpi_type_dup";
static const char long_gen417[]  = "MPI_Type_dup failed";
static const char short_gen418[] = "**mpi_type_extent";
static const char long_gen418[]  = "MPI_Type_extent failed";
static const char short_gen419[] = "**mpi_type_free";
static const char long_gen419[]  = "MPI_Type_free failed";
static const char short_gen420[] = "**mpi_type_free_keyval";
static const char long_gen420[]  = "MPI_Type_free_keyval failed";
static const char short_gen421[] = "**mpi_type_get_attr";
static const char long_gen421[]  = "MPI_Type_get_attr failed";
static const char short_gen422[] = "**mpi_type_get_contents";
static const char long_gen422[]  = "MPI_Type_get_contents failed";
static const char short_gen423[] = "**mpi_type_get_envelope";
static const char long_gen423[]  = "MPI_Type_get_envelope failed";
static const char short_gen424[] = "**mpi_type_get_extent";
static const char long_gen424[]  = "MPI_Type_get_extent failed";
static const char short_gen425[] = "**mpi_type_get_name";
static const char long_gen425[]  = "MPI_Type_get_name failed";
static const char short_gen426[] = "**mpi_type_get_true_extent";
static const char long_gen426[]  = "MPI_Type_get_true_extent failed";
static const char short_gen427[] = "**mpi_type_hindexed";
static const char long_gen427[]  = "MPI_Type_hindexed failed";
static const char short_gen428[] = "**mpi_type_hvector";
static const char long_gen428[]  = "MPI_Type_hvector failed";
static const char short_gen429[] = "**mpi_type_indexed";
static const char long_gen429[]  = "MPI_Type_indexed failed";
static const char short_gen430[] = "**mpi_type_lb";
static const char long_gen430[]  = "MPI_Type_lb failed";
static const char short_gen431[] = "**mpi_type_match_size";
static const char long_gen431[]  = "MPI_Type_match_size failed";
static const char short_gen432[] = "**mpi_type_set_attr";
static const char long_gen432[]  = "MPI_Type_set_attr failed";
static const char short_gen433[] = "**mpi_type_set_name";
static const char long_gen433[]  = "MPI_Type_set_name failed";
static const char short_gen434[] = "**mpi_type_size";
static const char long_gen434[]  = "MPI_Type_size failed";
static const char short_gen435[] = "**mpi_type_struct";
static const char long_gen435[]  = "MPI_Type_struct failed";
static const char short_gen436[] = "**mpi_type_ub";
static const char long_gen436[]  = "MPI_Type_ub failed";
static const char short_gen437[] = "**mpi_type_vector";
static const char long_gen437[]  = "MPI_Type_vector failed";
static const char short_gen438[] = "**mpi_unpack";
static const char long_gen438[]  = "MPI_Unpack failed";
static const char short_gen439[] = "**mpi_unpack_external";
static const char long_gen439[]  = "MPI_Unpack_external failed";
static const char short_gen440[] = "**mpi_unpublish_name";
static const char long_gen440[]  = "MPI_Unpublish_name failed";
static const char short_gen441[] = "**mpi_wait";
static const char long_gen441[]  = "MPI_Wait failed";
static const char short_gen442[] = "**mpi_waitall";
static const char long_gen442[]  = "MPI_Waitall failed";
static const char short_gen443[] = "**mpi_waitany";
static const char long_gen443[]  = "MPI_Waitany failed";
static const char short_gen444[] = "**mpi_waitsome";
static const char long_gen444[]  = "MPI_Waitsome failed";
static const char short_gen445[] = "**mpi_win_call_errhandler";
static const char long_gen445[]  = "MPI_Win_call_errhandler failed";
static const char short_gen446[] = "**mpi_win_complete";
static const char long_gen446[]  = "MPI_Win_complete failed";
static const char short_gen447[] = "**mpi_win_create";
static const char long_gen447[]  = "MPI_Win_create failed";
static const char short_gen448[] = "**mpi_win_create_errhandler";
static const char long_gen448[]  = "MPI_Win_create_errhandler failed";
static const char short_gen449[] = "**mpi_win_create_keyval";
static const char long_gen449[]  = "MPI_Win_create_keyval failed";
static const char short_gen450[] = "**mpi_win_delete_attr";
static const char long_gen450[]  = "MPI_Win_delete_attr failed";
static const char short_gen451[] = "**mpi_win_fence";
static const char long_gen451[]  = "MPI_Win_fence failed";
static const char short_gen452[] = "**mpi_win_free";
static const char long_gen452[]  = "MPI_Win_free failed";
static const char short_gen453[] = "**mpi_win_free_keyval";
static const char long_gen453[]  = "MPI_Win_free_keyval failed";
static const char short_gen454[] = "**mpi_win_get_attr";
static const char long_gen454[]  = "MPI_Win_get_attr failed";
static const char short_gen455[] = "**mpi_win_get_errhandler";
static const char long_gen455[]  = "MPI_Win_get_errhandler failed";
static const char short_gen456[] = "**mpi_win_get_group";
static const char long_gen456[]  = "MPI_Win_get_group failed";
static const char short_gen457[] = "**mpi_win_get_name";
static const char long_gen457[]  = "MPI_Win_get_name failed";
static const char short_gen458[] = "**mpi_win_lock";
static const char long_gen458[]  = "MPI_Win_lock failed";
static const char short_gen459[] = "**mpi_win_post";
static const char long_gen459[]  = "MPI_Win_post failed";
static const char short_gen460[] = "**mpi_win_set_attr";
static const char long_gen460[]  = "MPI_Win_set_attr failed";
static const char short_gen461[] = "**mpi_win_set_errhandler";
static const char long_gen461[]  = "MPI_Win_set_errhandler failed";
static const char short_gen462[] = "**mpi_win_set_name";
static const char long_gen462[]  = "MPI_Win_set_name failed";
static const char short_gen463[] = "**mpi_win_start";
static const char long_gen463[]  = "MPI_Win_start failed";
static const char short_gen464[] = "**mpi_win_test";
static const char long_gen464[]  = "MPI_Win_test failed";
static const char short_gen465[] = "**mpi_win_unlock";
static const char long_gen465[]  = "MPI_Win_unlock failed";
static const char short_gen466[] = "**mpi_win_wait";
static const char long_gen466[]  = "MPI_Win_wait failed";
static const char short_gen467[] = "**mpi_wtick";
static const char long_gen467[]  = "MPI_Wtick failed";
static const char short_gen468[] = "**mq_close";
static const char long_gen468[]  = "failed to close a posix message queue";
static const char short_gen469[] = "**mq_open";
static const char long_gen469[]  = "failed to open a posix message queue";
static const char short_gen470[] = "**mq_receive";
static const char long_gen470[]  = "failed to receive a posix message queue message";
static const char short_gen471[] = "**mq_send";
static const char long_gen471[]  = "failed to send a posix message queue message";
static const char short_gen472[] = "**mqp_failure";
static const char long_gen472[]  = "failed to make progress on the shared memory bootstrap message queue";
static const char short_gen473[] = "**mqshm_create";
static const char long_gen473[]  = "failed to create a shared memory message queue";
static const char short_gen474[] = "**mqshm_receive";
static const char long_gen474[]  = "failed to receive a bootstrap message";
static const char short_gen475[] = "**mqshm_send";
static const char long_gen475[]  = "failed to send a bootstrap message";
static const char short_gen476[] = "**mqshm_unlink";
static const char long_gen476[]  = "unable to unlink the shared memory message queue";
static const char short_gen477[] = "**msgctl";
static const char long_gen477[]  = "msgctl failed";
static const char short_gen478[] = "**msgget";
static const char long_gen478[]  = "msgget failed";
static const char short_gen479[] = "**msgrcv";
static const char long_gen479[]  = "msgrcv failed";
static const char short_gen480[] = "**msgsnd";
static const char long_gen480[]  = "msgsnd failed";
static const char short_gen481[] = "**namepubfile";
static const char long_gen481[]  = "Unable to publish service name";
static const char short_gen482[] = "**namepubnotpub";
static const char long_gen482[]  = "Lookup failed for service name ";
static const char short_gen483[] = "**nameservice";
static const char long_gen483[]  = "Invalid service name (see MPI_Publish_name)";
static const char short_gen484[] = "**needthreads";
static const char long_gen484[]  = "This function needs threads and threads have not been enabled";
static const char short_gen485[] = "**nextbootmsg";
static const char long_gen485[]  = "failed to get the next bootstrap message";
static const char short_gen486[] = "**noca";
static const char long_gen486[]  = "unable to find an active infiniband channel adapter";
static const char short_gen487[] = "**noerrclasses";
static const char long_gen487[]  = "No more user-defined error classes";
static const char short_gen488[] = "**noerrcodes";
static const char long_gen488[]  = "No more user-defined error codes";
static const char short_gen489[] = "**nomem";
static const char long_gen489[]  = "Out of memory";
static const char short_gen490[] = "**nomemreq";
static const char long_gen490[]  = "failure occurred while allocating memory for a request object";
static const char short_gen491[] = "**nonamepub";
static const char long_gen491[]  = "No name publishing service available";
static const char short_gen492[] = "**notcarttopo";
static const char long_gen492[]  = "No Cartesian topology associated with this communicator";
static const char short_gen493[] = "**notcstatignore";
static const char long_gen493[]  = "MPI_STATUS_IGNORE cannot be passed to MPI_Status_c2f()";
static const char short_gen494[] = "**notfstatignore";
static const char long_gen494[]  = "MPI_STATUS_IGNORE cannot be passed to MPI_Status_f2c()";
static const char short_gen495[] = "**notgenreq";
static const char long_gen495[]  = "Attempt to complete a request with MPI_GREQUEST_COMPLETE that was not started with MPI_GREQUEST_START";
static const char short_gen496[] = "**notgraphtopo";
static const char long_gen496[]  = "No Graph topology associated with this communicator";
static const char short_gen497[] = "**notimpl";
static const char long_gen497[]  = "Function not implemented";
static const char short_gen498[] = "**notopology";
static const char long_gen498[]  = "No topology associated with this communicator";
static const char short_gen499[] = "**notsame";
static const char long_gen499[]  = "Inconsistent arguments to collective routine ";
static const char short_gen500[] = "**nouniquehigh";
static const char long_gen500[]  = "Could not determine which group to place first in merged intracommunicator.  Please use the parameter high to choose which group should be placed first.";
static const char short_gen501[] = "**nulledge";
static const char long_gen501[]  = "Edge detected from a node to the same node";
static const char short_gen502[] = "**nullptr";
static const char long_gen502[]  = "Null pointer";
static const char short_gen503[] = "**nullptrtype";
static const char long_gen503[]  = "Null pointer";
static const char short_gen504[] = "**op";
static const char long_gen504[]  = "Invalid MPI_Op";
static const char short_gen505[] = "**open";
static const char long_gen505[]  = "open failed";
static const char short_gen506[] = "**opnotpredefined";
static const char long_gen506[]  = "only predefined ops are valid";
static const char short_gen507[] = "**opundefined";
static const char long_gen507[]  = "MPI_Op operation not defined for this datatype ";
static const char short_gen508[] = "**opundefined_rma";
static const char long_gen508[]  = "RMA target received unknown RMA operation";
static const char short_gen509[] = "**other";
static const char long_gen509[]  = "Other MPI error";
static const char short_gen510[] = "**pctwice";
static const char long_gen510[]  = "post close called twice";
static const char short_gen511[] = "**pd_alloc";
static const char long_gen511[]  = "unable to allocate a protection domain";
static const char short_gen512[] = "**permattr";
static const char long_gen512[]  = "Cannot set permanent attribute";
static const char short_gen513[] = "**permop";
static const char long_gen513[]  = "Cannot free permanent MPI_Op ";
static const char short_gen514[] = "**pfinal_sockclose";
static const char long_gen514[]  = "sock_close failed";
static const char short_gen515[] = "**pglookup";
static const char long_gen515[]  = "unable to find the process group structure";
static const char short_gen516[] = "**pkt_ptr";
static const char long_gen516[]  = "invalid shm queue packet pointer";
static const char short_gen517[] = "**pmi_barrier";
static const char long_gen517[]  = "PMI_Barrier failed";
static const char short_gen518[] = "**pmi_finalize";
static const char long_gen518[]  = "PMI_Finalize failed";
static const char short_gen519[] = "**pmi_get_id";
static const char long_gen519[]  = "PMI_Get_id failed";
static const char short_gen520[] = "**pmi_get_id_length_max";
static const char long_gen520[]  = "PMI_Get_id_length_max failed";
static const char short_gen521[] = "**pmi_get_rank";
static const char long_gen521[]  = "PMI_Get_rank failed";
static const char short_gen522[] = "**pmi_get_size";
static const char long_gen522[]  = "PMI_Get_size failed";
static const char short_gen523[] = "**pmi_init";
static const char long_gen523[]  = "PMI_Init failed";
static const char short_gen524[] = "**pmi_kvs_commit";
static const char long_gen524[]  = "PMI_KVS_Commit failed";
static const char short_gen525[] = "**pmi_kvs_create";
static const char long_gen525[]  = "PMI_KVS_Create failed";
static const char short_gen526[] = "**pmi_kvs_get";
static const char long_gen526[]  = "PMI_KVS_Get failed";
static const char short_gen527[] = "**pmi_kvs_get_key_length_max";
static const char long_gen527[]  = "PMI_KVS_Get_key_length_max failed";
static const char short_gen528[] = "**pmi_kvs_get_my_name";
static const char long_gen528[]  = "PMI_KVS_Get_my_name failed";
static const char short_gen529[] = "**pmi_kvs_get_name_length_max";
static const char long_gen529[]  = "PMI_KVS_Get_name_length_max failed";
static const char short_gen530[] = "**pmi_kvs_get_value_length_max";
static const char long_gen530[]  = "PMI_KVS_Get_value_length_max failed";
static const char short_gen531[] = "**pmi_kvs_put";
static const char long_gen531[]  = "PMI_KVS_Put failed";
static const char short_gen532[] = "**pmi_spawn_multiple";
static const char long_gen532[]  = "PMI_Spawn_multiple failed";
static const char short_gen533[] = "**poke";
static const char long_gen533[]  = "progress_poke failed";
static const char short_gen534[] = "**port";
static const char long_gen534[]  = "Invalid port";
static const char short_gen535[] = "**post_accept";
static const char long_gen535[]  = "post accept failed";
static const char short_gen536[] = "**post_connect";
static const char long_gen536[]  = "failed to post a connection";
static const char short_gen537[] = "**postpkt";
static const char long_gen537[]  = "Unable to post a read for the next packet header";
static const char short_gen538[] = "**process_group";
static const char long_gen538[]  = "Process group initialization failed";
static const char short_gen539[] = "**progress";
static const char long_gen539[]  = "progress engine failure";
static const char short_gen540[] = "**progress_finalize";
static const char long_gen540[]  = "finalization of the progress engine failed";
static const char short_gen541[] = "**progress_handle_sock_op";
static const char long_gen541[]  = "handle_sock_op failed";
static const char short_gen542[] = "**progress_init";
static const char long_gen542[]  = "unable to initialize the progress engine";
static const char short_gen543[] = "**progress_sock_wait";
static const char long_gen543[]  = "sock_wait failed";
static const char short_gen544[] = "**progress_test";
static const char long_gen544[]  = "progress_test engine failure";
static const char short_gen545[] = "**rangedup";
static const char long_gen545[]  = "The range array specifies duplicate entries";
static const char short_gen546[] = "**rangeendinvalid";
static const char long_gen546[]  = "Some element of a range array is either negative or too large";
static const char short_gen547[] = "**rangestartinvalid";
static const char long_gen547[]  = "Some element of a range array is either negative or too large";
static const char short_gen548[] = "**rank";
static const char long_gen548[]  = "Invalid rank";
static const char short_gen549[] = "**rankarray";
static const char long_gen549[]  = "Invalid rank in rank array";
static const char short_gen550[] = "**rankdup";
static const char long_gen550[]  = "Duplicate ranks in rank array ";
static const char short_gen551[] = "**ranklocal";
static const char long_gen551[]  = "Error specifying local_leader ";
static const char short_gen552[] = "**rankremote";
static const char long_gen552[]  = "Error specifying remote_leader ";
static const char short_gen553[] = "**rdma_finalize";
static const char long_gen553[]  = "Channel rdma finalization failed";
static const char short_gen554[] = "**rdma_init";
static const char long_gen554[]  = "Channel rdma initialization failed";
static const char short_gen555[] = "**read_progress";
static const char long_gen555[]  = "Unable to make read progress";
static const char short_gen556[] = "**recvbuf_inplace";
static const char long_gen556[]  = "recvbuf cannot be MPI_IN_PLACE";
static const char short_gen557[] = "**request";
static const char long_gen557[]  = "Invalid MPI_Request";
static const char short_gen558[] = "**requestnotpersist";
static const char long_gen558[]  = "Request is not persistent in MPI_Start or MPI_Startall.";
static const char short_gen559[] = "**requestpersistactive";
static const char long_gen559[]  = "Persistent request passed to MPI_Start or MPI_Startall is already active.";
static const char short_gen560[] = "**rmaconflict";
static const char long_gen560[]  = "Conflicting accesses to window ";
static const char short_gen561[] = "**rmadisp";
static const char long_gen561[]  = "Invalid displacement argument in RMA call ";
static const char short_gen562[] = "**rmasize";
static const char long_gen562[]  = "Invalid size argument in RMA call";
static const char short_gen563[] = "**rmasync";
static const char long_gen563[]  = "Wrong synchronization of RMA calls ";
static const char short_gen564[] = "**root";
static const char long_gen564[]  = "Invalid root";
static const char short_gen565[] = "**rsendnomatch";
static const char long_gen565[]  = "Ready send had no matching receive ";
static const char short_gen566[] = "**sendbuf_inplace";
static const char long_gen566[]  = "sendbuf cannot be MPI_IN_PLACE";
static const char short_gen567[] = "**servicename";
static const char long_gen567[]  = "Attempt to lookup an unknown service name ";
static const char short_gen568[] = "**shm_op";
static const char long_gen568[]  = "invalid shm operation";
static const char short_gen569[] = "**shm_open";
static const char long_gen569[]  = "unable to open a shared memory object";
static const char short_gen570[] = "**shm_read_progress";
static const char long_gen570[]  = "shared memory read progress failed";
static const char short_gen571[] = "**shm_unlink";
static const char long_gen571[]  = "failed to unlink shared memory";
static const char short_gen572[] = "**shm_wait";
static const char long_gen572[]  = "wait function failed";
static const char short_gen573[] = "**shmat";
static const char long_gen573[]  = "shmat failed";
static const char short_gen574[] = "**shmconnect_getmem";
static const char long_gen574[]  = "failed to allocate shared memory for a write queue";
static const char short_gen575[] = "**shmctl";
static const char long_gen575[]  = "failed to mark the sysv segment for removal";
static const char short_gen576[] = "**shmget";
static const char long_gen576[]  = "shmget failed";
static const char short_gen577[] = "**shmgetmem";
static const char long_gen577[]  = "Unable to allocate shared memory";
static const char short_gen578[] = "**shmhost";
static const char long_gen578[]  = "process not on the same host";
static const char short_gen579[] = "**shmq_index";
static const char long_gen579[]  = "invalid shm queue index";
static const char short_gen580[] = "**shmsize";
static const char long_gen580[]  = "Shared memory subsystem unable to allocate a block of the requested size";
static const char short_gen581[] = "**shmwrite";
static const char long_gen581[]  = "shared memory write failed";
static const char short_gen582[] = "**snprintf";
static const char long_gen582[]  = "snprintf returned an invalid number";
static const char short_gen583[] = "**sock_byname";
static const char long_gen583[]  = "gethostbyname failed";
static const char short_gen584[] = "**sock_closed";
static const char long_gen584[]  = "socket closed";
static const char short_gen585[] = "**sock_connect";
static const char long_gen585[]  = "connect failed";
static const char short_gen586[] = "**sock_create";
static const char long_gen586[]  = "unable to create a socket";
static const char short_gen587[] = "**sock_gethost";
static const char long_gen587[]  = "gethostname failed";
static const char short_gen588[] = "**sock_init";
static const char long_gen588[]  = "unable to initialize the sock library";
static const char short_gen589[] = "**sock_nop_accept";
static const char long_gen589[]  = "accept called without having received an op_accept";
static const char short_gen590[] = "**sock_post_close";
static const char long_gen590[]  = "posting a close of the socket failed";
static const char short_gen591[] = "**socket";
static const char long_gen591[]  = "WSASocket failed";
static const char short_gen592[] = "**sock|badbuf";
static const char long_gen592[]  = "the supplied buffer contains invalid memory";
static const char short_gen593[] = "**sock|badhandle";
static const char long_gen593[]  = "sock contains an invalid handle";
static const char short_gen594[] = "**sock|badhdbuf";
static const char long_gen594[]  = "a memory fault occurred while accessing the host description string";
static const char short_gen595[] = "**sock|badhdlen";
static const char long_gen595[]  = "host description string to small to store description";
static const char short_gen596[] = "**sock|badhdmax";
static const char long_gen596[]  = "the length of the host description string must be non-negative";
static const char short_gen597[] = "**sock|badiovn";
static const char long_gen597[]  = "size of iov is invalid";
static const char short_gen598[] = "**sock|badlen";
static const char long_gen598[]  = "bad length parameter(s)";
static const char short_gen599[] = "**sock|badport";
static const char long_gen599[]  = "port number is out of range";
static const char short_gen600[] = "**sock|badsock";
static const char long_gen600[]  = "supplied sock is corrupt";
static const char short_gen601[] = "**sock|close_cancel";
static const char long_gen601[]  = "operation cancelled because sock was closed locally";
static const char short_gen602[] = "**sock|closing";
static const char long_gen602[]  = "sock is in the process of being closed locally";
static const char short_gen603[] = "**sock|closing_already";
static const char long_gen603[]  = "a close operation is already posted";
static const char short_gen604[] = "**sock|connclosed";
static const char long_gen604[]  = "connection closed by peer";
static const char short_gen605[] = "**sock|connfailed";
static const char long_gen605[]  = "connection failure";
static const char short_gen606[] = "**sock|connrefused";
static const char long_gen606[]  = "connection refused";
static const char short_gen607[] = "**sock|getport";
static const char long_gen607[]  = "failed to obtain port number of the listener";
static const char short_gen608[] = "**sock|hostres";
static const char long_gen608[]  = "unable to resolve host name to an address";
static const char short_gen609[] = "**sock|listener_bad_sock";
static const char long_gen609[]  = " supplied sock is not a listener";
static const char short_gen610[] = "**sock|listener_bad_state";
static const char long_gen610[]  = " supplied listener sock is in a bad state";
static const char short_gen611[] = "**sock|listener_read";
static const char long_gen611[]  = "read operation not allowed on a listener";
static const char short_gen612[] = "**sock|listener_write";
static const char long_gen612[]  = "write operation not allowed on a listener";
static const char short_gen613[] = "**sock|nosock";
static const char long_gen613[]  = "no new sock was available to accept";
static const char short_gen614[] = "**sock|notconnected";
static const char long_gen614[]  = "sock is not connected";
static const char short_gen615[] = "**sock|oserror";
static const char long_gen615[]  = "unexpected operating system error";
static const char short_gen616[] = "**sock|osnomem";
static const char long_gen616[]  = "operating system routine failed due to lack of memory";
static const char short_gen617[] = "**sock|poll|accept";
static const char long_gen617[]  = "accept failed to acquire a new socket";
static const char short_gen618[] = "**sock|poll|bind";
static const char long_gen618[]  = "unable to bind socket to port";
static const char short_gen619[] = "**sock|poll|eqfail";
static const char long_gen619[]  = "fatal error: failed to enqueue an event; event was lost";
static const char short_gen620[] = "**sock|poll|eqmalloc";
static const char long_gen620[]  = "MPIU_Malloc failed to allocate memory for an event queue structure";
static const char short_gen621[] = "**sock|poll|listen";
static const char long_gen621[]  = "listen() failed";
static const char short_gen622[] = "**sock|poll|nodelay";
static const char long_gen622[]  = "unable to set TCP no delay attribute on socket";
static const char short_gen623[] = "**sock|poll|nonblock";
static const char long_gen623[]  = "unable to set socket to nonblocking";
static const char short_gen624[] = "**sock|poll|pipe";
static const char long_gen624[]  = "unable to allocate pipe to wakeup a blocking poll()";
static const char short_gen625[] = "**sock|poll|pipenonblock";
static const char long_gen625[]  = "unable to set wakeup pipe to nonblocking";
static const char short_gen626[] = "**sock|poll|reuseaddr";
static const char long_gen626[]  = "unable to set reuseaddr attribute on socket";
static const char short_gen627[] = "**sock|poll|socket";
static const char long_gen627[]  = "unable to obtain new socket";
static const char short_gen628[] = "**sock|poll|unhandledstate";
static const char long_gen628[]  = "encountered an unexpected state";
static const char short_gen629[] = "**sock|poll|unhandledtype";
static const char long_gen629[]  = "encountered an unexpected sock type";
static const char short_gen630[] = "**sock|reads";
static const char long_gen630[]  = "attempt to perform multiple simultaneous reads";
static const char short_gen631[] = "**sock|setalloc";
static const char long_gen631[]  = "unable to allocate a new sock set object";
static const char short_gen632[] = "**sock|sockalloc";
static const char long_gen632[]  = "unable to allocate a new sock object";
static const char short_gen633[] = "**sock|uninit";
static const char long_gen633[]  = "Sock library has not been initialized";
static const char short_gen634[] = "**sock|writes";
static const char long_gen634[]  = "attempt to perform multiple simultaneous writes";
static const char short_gen635[] = "**spawn";
static const char long_gen635[]  = "Error in spawn call";
static const char short_gen636[] = "**ssmwrite";
static const char long_gen636[]  = "sock/shared memory write failed";
static const char short_gen637[] = "**ssmwritev";
static const char long_gen637[]  = "sock/shared memory writev failed";
static const char short_gen638[] = "**stride";
static const char long_gen638[]  = "Range does not terminate";
static const char short_gen639[] = "**stridezero";
static const char long_gen639[]  = "Zero stride is invalid";
static const char short_gen640[] = "**strncpy";
static const char long_gen640[]  = "insufficient buffer length to complete strncpy";
static const char short_gen641[] = "**success";
static const char long_gen641[]  = "No MPI error";
static const char short_gen642[] = "**tag";
static const char long_gen642[]  = "Invalid tag";
static const char short_gen643[] = "**test_sock_wait";
static const char long_gen643[]  = "sock_wait failed";
static const char short_gen644[] = "**toomanycomm";
static const char long_gen644[]  = "Too many communicators";
static const char short_gen645[] = "**topology";
static const char long_gen645[]  = "Invalid topology";
static const char short_gen646[] = "**topotoolarge";
static const char long_gen646[]  = "Topology size is greater than communicator size";
static const char short_gen647[] = "**truncate";
static const char long_gen647[]  = "Message truncated";
static const char short_gen648[] = "**typematchnoclass";
static const char long_gen648[]  = "The value of typeclass is not one of MPI_TYPECLASS_REAL, MPI_TYPECLASS_INTEGER, or MPI_TYPECLASS_COMPLEX";
static const char short_gen649[] = "**typematchsize";
static const char long_gen649[]  = "No MPI datatype available for the given typeclass and size";
static const char short_gen650[] = "**typenamelen";
static const char long_gen650[]  = " Specified datatype name is too long";
static const char short_gen651[] = "**unknown";
static const char long_gen651[]  = "Unknown error.  Please file a bug report.";
static const char short_gen652[] = "**unsupporteddatarep";
static const char long_gen652[]  = "Only native data representation currently supported";
static const char short_gen653[] = "**vc_state";
static const char long_gen653[]  = "invalid vc state";
static const char short_gen654[] = "**win";
static const char long_gen654[]  = "Invalid MPI_Win";
static const char short_gen655[] = "**winunlockrank";
static const char long_gen655[]  = "Invalid rank argument";
static const char short_gen656[] = "**winwait";
static const char long_gen656[]  = "WaitForSingleObject failed";
static const char short_gen657[] = "**write_progress";
static const char long_gen657[]  = "Write progress failed";
static const char short_gen658[] = "**wsasock";
static const char long_gen658[]  = "WSAStartup failed";

static const int generic_msgs_len = 659;
static const msgpair generic_err_msgs[] = {
{ 0xacebad03, short_gen0, long_gen0, 0xcb0bfa11 },
{ 0xacebad03, short_gen1, long_gen1, 0xcb0bfa11 },
{ 0xacebad03, short_gen2, long_gen2, 0xcb0bfa11 },
{ 0xacebad03, short_gen3, long_gen3, 0xcb0bfa11 },
{ 0xacebad03, short_gen4, long_gen4, 0xcb0bfa11 },
{ 0xacebad03, short_gen5, long_gen5, 0xcb0bfa11 },
{ 0xacebad03, short_gen6, long_gen6, 0xcb0bfa11 },
{ 0xacebad03, short_gen7, long_gen7, 0xcb0bfa11 },
{ 0xacebad03, short_gen8, long_gen8, 0xcb0bfa11 },
{ 0xacebad03, short_gen9, long_gen9, 0xcb0bfa11 },
{ 0xacebad03, short_gen10, long_gen10, 0xcb0bfa11 },
{ 0xacebad03, short_gen11, long_gen11, 0xcb0bfa11 },
{ 0xacebad03, short_gen12, long_gen12, 0xcb0bfa11 },
{ 0xacebad03, short_gen13, long_gen13, 0xcb0bfa11 },
{ 0xacebad03, short_gen14, long_gen14, 0xcb0bfa11 },
{ 0xacebad03, short_gen15, long_gen15, 0xcb0bfa11 },
{ 0xacebad03, short_gen16, long_gen16, 0xcb0bfa11 },
{ 0xacebad03, short_gen17, long_gen17, 0xcb0bfa11 },
{ 0xacebad03, short_gen18, long_gen18, 0xcb0bfa11 },
{ 0xacebad03, short_gen19, long_gen19, 0xcb0bfa11 },
{ 0xacebad03, short_gen20, long_gen20, 0xcb0bfa11 },
{ 0xacebad03, short_gen21, long_gen21, 0xcb0bfa11 },
{ 0xacebad03, short_gen22, long_gen22, 0xcb0bfa11 },
{ 0xacebad03, short_gen23, long_gen23, 0xcb0bfa11 },
{ 0xacebad03, short_gen24, long_gen24, 0xcb0bfa11 },
{ 0xacebad03, short_gen25, long_gen25, 0xcb0bfa11 },
{ 0xacebad03, short_gen26, long_gen26, 0xcb0bfa11 },
{ 0xacebad03, short_gen27, long_gen27, 0xcb0bfa11 },
{ 0xacebad03, short_gen28, long_gen28, 0xcb0bfa11 },
{ 0xacebad03, short_gen29, long_gen29, 0xcb0bfa11 },
{ 0xacebad03, short_gen30, long_gen30, 0xcb0bfa11 },
{ 0xacebad03, short_gen31, long_gen31, 0xcb0bfa11 },
{ 0xacebad03, short_gen32, long_gen32, 0xcb0bfa11 },
{ 0xacebad03, short_gen33, long_gen33, 0xcb0bfa11 },
{ 0xacebad03, short_gen34, long_gen34, 0xcb0bfa11 },
{ 0xacebad03, short_gen35, long_gen35, 0xcb0bfa11 },
{ 0xacebad03, short_gen36, long_gen36, 0xcb0bfa11 },
{ 0xacebad03, short_gen37, long_gen37, 0xcb0bfa11 },
{ 0xacebad03, short_gen38, long_gen38, 0xcb0bfa11 },
{ 0xacebad03, short_gen39, long_gen39, 0xcb0bfa11 },
{ 0xacebad03, short_gen40, long_gen40, 0xcb0bfa11 },
{ 0xacebad03, short_gen41, long_gen41, 0xcb0bfa11 },
{ 0xacebad03, short_gen42, long_gen42, 0xcb0bfa11 },
{ 0xacebad03, short_gen43, long_gen43, 0xcb0bfa11 },
{ 0xacebad03, short_gen44, long_gen44, 0xcb0bfa11 },
{ 0xacebad03, short_gen45, long_gen45, 0xcb0bfa11 },
{ 0xacebad03, short_gen46, long_gen46, 0xcb0bfa11 },
{ 0xacebad03, short_gen47, long_gen47, 0xcb0bfa11 },
{ 0xacebad03, short_gen48, long_gen48, 0xcb0bfa11 },
{ 0xacebad03, short_gen49, long_gen49, 0xcb0bfa11 },
{ 0xacebad03, short_gen50, long_gen50, 0xcb0bfa11 },
{ 0xacebad03, short_gen51, long_gen51, 0xcb0bfa11 },
{ 0xacebad03, short_gen52, long_gen52, 0xcb0bfa11 },
{ 0xacebad03, short_gen53, long_gen53, 0xcb0bfa11 },
{ 0xacebad03, short_gen54, long_gen54, 0xcb0bfa11 },
{ 0xacebad03, short_gen55, long_gen55, 0xcb0bfa11 },
{ 0xacebad03, short_gen56, long_gen56, 0xcb0bfa11 },
{ 0xacebad03, short_gen57, long_gen57, 0xcb0bfa11 },
{ 0xacebad03, short_gen58, long_gen58, 0xcb0bfa11 },
{ 0xacebad03, short_gen59, long_gen59, 0xcb0bfa11 },
{ 0xacebad03, short_gen60, long_gen60, 0xcb0bfa11 },
{ 0xacebad03, short_gen61, long_gen61, 0xcb0bfa11 },
{ 0xacebad03, short_gen62, long_gen62, 0xcb0bfa11 },
{ 0xacebad03, short_gen63, long_gen63, 0xcb0bfa11 },
{ 0xacebad03, short_gen64, long_gen64, 0xcb0bfa11 },
{ 0xacebad03, short_gen65, long_gen65, 0xcb0bfa11 },
{ 0xacebad03, short_gen66, long_gen66, 0xcb0bfa11 },
{ 0xacebad03, short_gen67, long_gen67, 0xcb0bfa11 },
{ 0xacebad03, short_gen68, long_gen68, 0xcb0bfa11 },
{ 0xacebad03, short_gen69, long_gen69, 0xcb0bfa11 },
{ 0xacebad03, short_gen70, long_gen70, 0xcb0bfa11 },
{ 0xacebad03, short_gen71, long_gen71, 0xcb0bfa11 },
{ 0xacebad03, short_gen72, long_gen72, 0xcb0bfa11 },
{ 0xacebad03, short_gen73, long_gen73, 0xcb0bfa11 },
{ 0xacebad03, short_gen74, long_gen74, 0xcb0bfa11 },
{ 0xacebad03, short_gen75, long_gen75, 0xcb0bfa11 },
{ 0xacebad03, short_gen76, long_gen76, 0xcb0bfa11 },
{ 0xacebad03, short_gen77, long_gen77, 0xcb0bfa11 },
{ 0xacebad03, short_gen78, long_gen78, 0xcb0bfa11 },
{ 0xacebad03, short_gen79, long_gen79, 0xcb0bfa11 },
{ 0xacebad03, short_gen80, long_gen80, 0xcb0bfa11 },
{ 0xacebad03, short_gen81, long_gen81, 0xcb0bfa11 },
{ 0xacebad03, short_gen82, long_gen82, 0xcb0bfa11 },
{ 0xacebad03, short_gen83, long_gen83, 0xcb0bfa11 },
{ 0xacebad03, short_gen84, long_gen84, 0xcb0bfa11 },
{ 0xacebad03, short_gen85, long_gen85, 0xcb0bfa11 },
{ 0xacebad03, short_gen86, long_gen86, 0xcb0bfa11 },
{ 0xacebad03, short_gen87, long_gen87, 0xcb0bfa11 },
{ 0xacebad03, short_gen88, long_gen88, 0xcb0bfa11 },
{ 0xacebad03, short_gen89, long_gen89, 0xcb0bfa11 },
{ 0xacebad03, short_gen90, long_gen90, 0xcb0bfa11 },
{ 0xacebad03, short_gen91, long_gen91, 0xcb0bfa11 },
{ 0xacebad03, short_gen92, long_gen92, 0xcb0bfa11 },
{ 0xacebad03, short_gen93, long_gen93, 0xcb0bfa11 },
{ 0xacebad03, short_gen94, long_gen94, 0xcb0bfa11 },
{ 0xacebad03, short_gen95, long_gen95, 0xcb0bfa11 },
{ 0xacebad03, short_gen96, long_gen96, 0xcb0bfa11 },
{ 0xacebad03, short_gen97, long_gen97, 0xcb0bfa11 },
{ 0xacebad03, short_gen98, long_gen98, 0xcb0bfa11 },
{ 0xacebad03, short_gen99, long_gen99, 0xcb0bfa11 },
{ 0xacebad03, short_gen100, long_gen100, 0xcb0bfa11 },
{ 0xacebad03, short_gen101, long_gen101, 0xcb0bfa11 },
{ 0xacebad03, short_gen102, long_gen102, 0xcb0bfa11 },
{ 0xacebad03, short_gen103, long_gen103, 0xcb0bfa11 },
{ 0xacebad03, short_gen104, long_gen104, 0xcb0bfa11 },
{ 0xacebad03, short_gen105, long_gen105, 0xcb0bfa11 },
{ 0xacebad03, short_gen106, long_gen106, 0xcb0bfa11 },
{ 0xacebad03, short_gen107, long_gen107, 0xcb0bfa11 },
{ 0xacebad03, short_gen108, long_gen108, 0xcb0bfa11 },
{ 0xacebad03, short_gen109, long_gen109, 0xcb0bfa11 },
{ 0xacebad03, short_gen110, long_gen110, 0xcb0bfa11 },
{ 0xacebad03, short_gen111, long_gen111, 0xcb0bfa11 },
{ 0xacebad03, short_gen112, long_gen112, 0xcb0bfa11 },
{ 0xacebad03, short_gen113, long_gen113, 0xcb0bfa11 },
{ 0xacebad03, short_gen114, long_gen114, 0xcb0bfa11 },
{ 0xacebad03, short_gen115, long_gen115, 0xcb0bfa11 },
{ 0xacebad03, short_gen116, long_gen116, 0xcb0bfa11 },
{ 0xacebad03, short_gen117, long_gen117, 0xcb0bfa11 },
{ 0xacebad03, short_gen118, long_gen118, 0xcb0bfa11 },
{ 0xacebad03, short_gen119, long_gen119, 0xcb0bfa11 },
{ 0xacebad03, short_gen120, long_gen120, 0xcb0bfa11 },
{ 0xacebad03, short_gen121, long_gen121, 0xcb0bfa11 },
{ 0xacebad03, short_gen122, long_gen122, 0xcb0bfa11 },
{ 0xacebad03, short_gen123, long_gen123, 0xcb0bfa11 },
{ 0xacebad03, short_gen124, long_gen124, 0xcb0bfa11 },
{ 0xacebad03, short_gen125, long_gen125, 0xcb0bfa11 },
{ 0xacebad03, short_gen126, long_gen126, 0xcb0bfa11 },
{ 0xacebad03, short_gen127, long_gen127, 0xcb0bfa11 },
{ 0xacebad03, short_gen128, long_gen128, 0xcb0bfa11 },
{ 0xacebad03, short_gen129, long_gen129, 0xcb0bfa11 },
{ 0xacebad03, short_gen130, long_gen130, 0xcb0bfa11 },
{ 0xacebad03, short_gen131, long_gen131, 0xcb0bfa11 },
{ 0xacebad03, short_gen132, long_gen132, 0xcb0bfa11 },
{ 0xacebad03, short_gen133, long_gen133, 0xcb0bfa11 },
{ 0xacebad03, short_gen134, long_gen134, 0xcb0bfa11 },
{ 0xacebad03, short_gen135, long_gen135, 0xcb0bfa11 },
{ 0xacebad03, short_gen136, long_gen136, 0xcb0bfa11 },
{ 0xacebad03, short_gen137, long_gen137, 0xcb0bfa11 },
{ 0xacebad03, short_gen138, long_gen138, 0xcb0bfa11 },
{ 0xacebad03, short_gen139, long_gen139, 0xcb0bfa11 },
{ 0xacebad03, short_gen140, long_gen140, 0xcb0bfa11 },
{ 0xacebad03, short_gen141, long_gen141, 0xcb0bfa11 },
{ 0xacebad03, short_gen142, long_gen142, 0xcb0bfa11 },
{ 0xacebad03, short_gen143, long_gen143, 0xcb0bfa11 },
{ 0xacebad03, short_gen144, long_gen144, 0xcb0bfa11 },
{ 0xacebad03, short_gen145, long_gen145, 0xcb0bfa11 },
{ 0xacebad03, short_gen146, long_gen146, 0xcb0bfa11 },
{ 0xacebad03, short_gen147, long_gen147, 0xcb0bfa11 },
{ 0xacebad03, short_gen148, long_gen148, 0xcb0bfa11 },
{ 0xacebad03, short_gen149, long_gen149, 0xcb0bfa11 },
{ 0xacebad03, short_gen150, long_gen150, 0xcb0bfa11 },
{ 0xacebad03, short_gen151, long_gen151, 0xcb0bfa11 },
{ 0xacebad03, short_gen152, long_gen152, 0xcb0bfa11 },
{ 0xacebad03, short_gen153, long_gen153, 0xcb0bfa11 },
{ 0xacebad03, short_gen154, long_gen154, 0xcb0bfa11 },
{ 0xacebad03, short_gen155, long_gen155, 0xcb0bfa11 },
{ 0xacebad03, short_gen156, long_gen156, 0xcb0bfa11 },
{ 0xacebad03, short_gen157, long_gen157, 0xcb0bfa11 },
{ 0xacebad03, short_gen158, long_gen158, 0xcb0bfa11 },
{ 0xacebad03, short_gen159, long_gen159, 0xcb0bfa11 },
{ 0xacebad03, short_gen160, long_gen160, 0xcb0bfa11 },
{ 0xacebad03, short_gen161, long_gen161, 0xcb0bfa11 },
{ 0xacebad03, short_gen162, long_gen162, 0xcb0bfa11 },
{ 0xacebad03, short_gen163, long_gen163, 0xcb0bfa11 },
{ 0xacebad03, short_gen164, long_gen164, 0xcb0bfa11 },
{ 0xacebad03, short_gen165, long_gen165, 0xcb0bfa11 },
{ 0xacebad03, short_gen166, long_gen166, 0xcb0bfa11 },
{ 0xacebad03, short_gen167, long_gen167, 0xcb0bfa11 },
{ 0xacebad03, short_gen168, long_gen168, 0xcb0bfa11 },
{ 0xacebad03, short_gen169, long_gen169, 0xcb0bfa11 },
{ 0xacebad03, short_gen170, long_gen170, 0xcb0bfa11 },
{ 0xacebad03, short_gen171, long_gen171, 0xcb0bfa11 },
{ 0xacebad03, short_gen172, long_gen172, 0xcb0bfa11 },
{ 0xacebad03, short_gen173, long_gen173, 0xcb0bfa11 },
{ 0xacebad03, short_gen174, long_gen174, 0xcb0bfa11 },
{ 0xacebad03, short_gen175, long_gen175, 0xcb0bfa11 },
{ 0xacebad03, short_gen176, long_gen176, 0xcb0bfa11 },
{ 0xacebad03, short_gen177, long_gen177, 0xcb0bfa11 },
{ 0xacebad03, short_gen178, long_gen178, 0xcb0bfa11 },
{ 0xacebad03, short_gen179, long_gen179, 0xcb0bfa11 },
{ 0xacebad03, short_gen180, long_gen180, 0xcb0bfa11 },
{ 0xacebad03, short_gen181, long_gen181, 0xcb0bfa11 },
{ 0xacebad03, short_gen182, long_gen182, 0xcb0bfa11 },
{ 0xacebad03, short_gen183, long_gen183, 0xcb0bfa11 },
{ 0xacebad03, short_gen184, long_gen184, 0xcb0bfa11 },
{ 0xacebad03, short_gen185, long_gen185, 0xcb0bfa11 },
{ 0xacebad03, short_gen186, long_gen186, 0xcb0bfa11 },
{ 0xacebad03, short_gen187, long_gen187, 0xcb0bfa11 },
{ 0xacebad03, short_gen188, long_gen188, 0xcb0bfa11 },
{ 0xacebad03, short_gen189, long_gen189, 0xcb0bfa11 },
{ 0xacebad03, short_gen190, long_gen190, 0xcb0bfa11 },
{ 0xacebad03, short_gen191, long_gen191, 0xcb0bfa11 },
{ 0xacebad03, short_gen192, long_gen192, 0xcb0bfa11 },
{ 0xacebad03, short_gen193, long_gen193, 0xcb0bfa11 },
{ 0xacebad03, short_gen194, long_gen194, 0xcb0bfa11 },
{ 0xacebad03, short_gen195, long_gen195, 0xcb0bfa11 },
{ 0xacebad03, short_gen196, long_gen196, 0xcb0bfa11 },
{ 0xacebad03, short_gen197, long_gen197, 0xcb0bfa11 },
{ 0xacebad03, short_gen198, long_gen198, 0xcb0bfa11 },
{ 0xacebad03, short_gen199, long_gen199, 0xcb0bfa11 },
{ 0xacebad03, short_gen200, long_gen200, 0xcb0bfa11 },
{ 0xacebad03, short_gen201, long_gen201, 0xcb0bfa11 },
{ 0xacebad03, short_gen202, long_gen202, 0xcb0bfa11 },
{ 0xacebad03, short_gen203, long_gen203, 0xcb0bfa11 },
{ 0xacebad03, short_gen204, long_gen204, 0xcb0bfa11 },
{ 0xacebad03, short_gen205, long_gen205, 0xcb0bfa11 },
{ 0xacebad03, short_gen206, long_gen206, 0xcb0bfa11 },
{ 0xacebad03, short_gen207, long_gen207, 0xcb0bfa11 },
{ 0xacebad03, short_gen208, long_gen208, 0xcb0bfa11 },
{ 0xacebad03, short_gen209, long_gen209, 0xcb0bfa11 },
{ 0xacebad03, short_gen210, long_gen210, 0xcb0bfa11 },
{ 0xacebad03, short_gen211, long_gen211, 0xcb0bfa11 },
{ 0xacebad03, short_gen212, long_gen212, 0xcb0bfa11 },
{ 0xacebad03, short_gen213, long_gen213, 0xcb0bfa11 },
{ 0xacebad03, short_gen214, long_gen214, 0xcb0bfa11 },
{ 0xacebad03, short_gen215, long_gen215, 0xcb0bfa11 },
{ 0xacebad03, short_gen216, long_gen216, 0xcb0bfa11 },
{ 0xacebad03, short_gen217, long_gen217, 0xcb0bfa11 },
{ 0xacebad03, short_gen218, long_gen218, 0xcb0bfa11 },
{ 0xacebad03, short_gen219, long_gen219, 0xcb0bfa11 },
{ 0xacebad03, short_gen220, long_gen220, 0xcb0bfa11 },
{ 0xacebad03, short_gen221, long_gen221, 0xcb0bfa11 },
{ 0xacebad03, short_gen222, long_gen222, 0xcb0bfa11 },
{ 0xacebad03, short_gen223, long_gen223, 0xcb0bfa11 },
{ 0xacebad03, short_gen224, long_gen224, 0xcb0bfa11 },
{ 0xacebad03, short_gen225, long_gen225, 0xcb0bfa11 },
{ 0xacebad03, short_gen226, long_gen226, 0xcb0bfa11 },
{ 0xacebad03, short_gen227, long_gen227, 0xcb0bfa11 },
{ 0xacebad03, short_gen228, long_gen228, 0xcb0bfa11 },
{ 0xacebad03, short_gen229, long_gen229, 0xcb0bfa11 },
{ 0xacebad03, short_gen230, long_gen230, 0xcb0bfa11 },
{ 0xacebad03, short_gen231, long_gen231, 0xcb0bfa11 },
{ 0xacebad03, short_gen232, long_gen232, 0xcb0bfa11 },
{ 0xacebad03, short_gen233, long_gen233, 0xcb0bfa11 },
{ 0xacebad03, short_gen234, long_gen234, 0xcb0bfa11 },
{ 0xacebad03, short_gen235, long_gen235, 0xcb0bfa11 },
{ 0xacebad03, short_gen236, long_gen236, 0xcb0bfa11 },
{ 0xacebad03, short_gen237, long_gen237, 0xcb0bfa11 },
{ 0xacebad03, short_gen238, long_gen238, 0xcb0bfa11 },
{ 0xacebad03, short_gen239, long_gen239, 0xcb0bfa11 },
{ 0xacebad03, short_gen240, long_gen240, 0xcb0bfa11 },
{ 0xacebad03, short_gen241, long_gen241, 0xcb0bfa11 },
{ 0xacebad03, short_gen242, long_gen242, 0xcb0bfa11 },
{ 0xacebad03, short_gen243, long_gen243, 0xcb0bfa11 },
{ 0xacebad03, short_gen244, long_gen244, 0xcb0bfa11 },
{ 0xacebad03, short_gen245, long_gen245, 0xcb0bfa11 },
{ 0xacebad03, short_gen246, long_gen246, 0xcb0bfa11 },
{ 0xacebad03, short_gen247, long_gen247, 0xcb0bfa11 },
{ 0xacebad03, short_gen248, long_gen248, 0xcb0bfa11 },
{ 0xacebad03, short_gen249, long_gen249, 0xcb0bfa11 },
{ 0xacebad03, short_gen250, long_gen250, 0xcb0bfa11 },
{ 0xacebad03, short_gen251, long_gen251, 0xcb0bfa11 },
{ 0xacebad03, short_gen252, long_gen252, 0xcb0bfa11 },
{ 0xacebad03, short_gen253, long_gen253, 0xcb0bfa11 },
{ 0xacebad03, short_gen254, long_gen254, 0xcb0bfa11 },
{ 0xacebad03, short_gen255, long_gen255, 0xcb0bfa11 },
{ 0xacebad03, short_gen256, long_gen256, 0xcb0bfa11 },
{ 0xacebad03, short_gen257, long_gen257, 0xcb0bfa11 },
{ 0xacebad03, short_gen258, long_gen258, 0xcb0bfa11 },
{ 0xacebad03, short_gen259, long_gen259, 0xcb0bfa11 },
{ 0xacebad03, short_gen260, long_gen260, 0xcb0bfa11 },
{ 0xacebad03, short_gen261, long_gen261, 0xcb0bfa11 },
{ 0xacebad03, short_gen262, long_gen262, 0xcb0bfa11 },
{ 0xacebad03, short_gen263, long_gen263, 0xcb0bfa11 },
{ 0xacebad03, short_gen264, long_gen264, 0xcb0bfa11 },
{ 0xacebad03, short_gen265, long_gen265, 0xcb0bfa11 },
{ 0xacebad03, short_gen266, long_gen266, 0xcb0bfa11 },
{ 0xacebad03, short_gen267, long_gen267, 0xcb0bfa11 },
{ 0xacebad03, short_gen268, long_gen268, 0xcb0bfa11 },
{ 0xacebad03, short_gen269, long_gen269, 0xcb0bfa11 },
{ 0xacebad03, short_gen270, long_gen270, 0xcb0bfa11 },
{ 0xacebad03, short_gen271, long_gen271, 0xcb0bfa11 },
{ 0xacebad03, short_gen272, long_gen272, 0xcb0bfa11 },
{ 0xacebad03, short_gen273, long_gen273, 0xcb0bfa11 },
{ 0xacebad03, short_gen274, long_gen274, 0xcb0bfa11 },
{ 0xacebad03, short_gen275, long_gen275, 0xcb0bfa11 },
{ 0xacebad03, short_gen276, long_gen276, 0xcb0bfa11 },
{ 0xacebad03, short_gen277, long_gen277, 0xcb0bfa11 },
{ 0xacebad03, short_gen278, long_gen278, 0xcb0bfa11 },
{ 0xacebad03, short_gen279, long_gen279, 0xcb0bfa11 },
{ 0xacebad03, short_gen280, long_gen280, 0xcb0bfa11 },
{ 0xacebad03, short_gen281, long_gen281, 0xcb0bfa11 },
{ 0xacebad03, short_gen282, long_gen282, 0xcb0bfa11 },
{ 0xacebad03, short_gen283, long_gen283, 0xcb0bfa11 },
{ 0xacebad03, short_gen284, long_gen284, 0xcb0bfa11 },
{ 0xacebad03, short_gen285, long_gen285, 0xcb0bfa11 },
{ 0xacebad03, short_gen286, long_gen286, 0xcb0bfa11 },
{ 0xacebad03, short_gen287, long_gen287, 0xcb0bfa11 },
{ 0xacebad03, short_gen288, long_gen288, 0xcb0bfa11 },
{ 0xacebad03, short_gen289, long_gen289, 0xcb0bfa11 },
{ 0xacebad03, short_gen290, long_gen290, 0xcb0bfa11 },
{ 0xacebad03, short_gen291, long_gen291, 0xcb0bfa11 },
{ 0xacebad03, short_gen292, long_gen292, 0xcb0bfa11 },
{ 0xacebad03, short_gen293, long_gen293, 0xcb0bfa11 },
{ 0xacebad03, short_gen294, long_gen294, 0xcb0bfa11 },
{ 0xacebad03, short_gen295, long_gen295, 0xcb0bfa11 },
{ 0xacebad03, short_gen296, long_gen296, 0xcb0bfa11 },
{ 0xacebad03, short_gen297, long_gen297, 0xcb0bfa11 },
{ 0xacebad03, short_gen298, long_gen298, 0xcb0bfa11 },
{ 0xacebad03, short_gen299, long_gen299, 0xcb0bfa11 },
{ 0xacebad03, short_gen300, long_gen300, 0xcb0bfa11 },
{ 0xacebad03, short_gen301, long_gen301, 0xcb0bfa11 },
{ 0xacebad03, short_gen302, long_gen302, 0xcb0bfa11 },
{ 0xacebad03, short_gen303, long_gen303, 0xcb0bfa11 },
{ 0xacebad03, short_gen304, long_gen304, 0xcb0bfa11 },
{ 0xacebad03, short_gen305, long_gen305, 0xcb0bfa11 },
{ 0xacebad03, short_gen306, long_gen306, 0xcb0bfa11 },
{ 0xacebad03, short_gen307, long_gen307, 0xcb0bfa11 },
{ 0xacebad03, short_gen308, long_gen308, 0xcb0bfa11 },
{ 0xacebad03, short_gen309, long_gen309, 0xcb0bfa11 },
{ 0xacebad03, short_gen310, long_gen310, 0xcb0bfa11 },
{ 0xacebad03, short_gen311, long_gen311, 0xcb0bfa11 },
{ 0xacebad03, short_gen312, long_gen312, 0xcb0bfa11 },
{ 0xacebad03, short_gen313, long_gen313, 0xcb0bfa11 },
{ 0xacebad03, short_gen314, long_gen314, 0xcb0bfa11 },
{ 0xacebad03, short_gen315, long_gen315, 0xcb0bfa11 },
{ 0xacebad03, short_gen316, long_gen316, 0xcb0bfa11 },
{ 0xacebad03, short_gen317, long_gen317, 0xcb0bfa11 },
{ 0xacebad03, short_gen318, long_gen318, 0xcb0bfa11 },
{ 0xacebad03, short_gen319, long_gen319, 0xcb0bfa11 },
{ 0xacebad03, short_gen320, long_gen320, 0xcb0bfa11 },
{ 0xacebad03, short_gen321, long_gen321, 0xcb0bfa11 },
{ 0xacebad03, short_gen322, long_gen322, 0xcb0bfa11 },
{ 0xacebad03, short_gen323, long_gen323, 0xcb0bfa11 },
{ 0xacebad03, short_gen324, long_gen324, 0xcb0bfa11 },
{ 0xacebad03, short_gen325, long_gen325, 0xcb0bfa11 },
{ 0xacebad03, short_gen326, long_gen326, 0xcb0bfa11 },
{ 0xacebad03, short_gen327, long_gen327, 0xcb0bfa11 },
{ 0xacebad03, short_gen328, long_gen328, 0xcb0bfa11 },
{ 0xacebad03, short_gen329, long_gen329, 0xcb0bfa11 },
{ 0xacebad03, short_gen330, long_gen330, 0xcb0bfa11 },
{ 0xacebad03, short_gen331, long_gen331, 0xcb0bfa11 },
{ 0xacebad03, short_gen332, long_gen332, 0xcb0bfa11 },
{ 0xacebad03, short_gen333, long_gen333, 0xcb0bfa11 },
{ 0xacebad03, short_gen334, long_gen334, 0xcb0bfa11 },
{ 0xacebad03, short_gen335, long_gen335, 0xcb0bfa11 },
{ 0xacebad03, short_gen336, long_gen336, 0xcb0bfa11 },
{ 0xacebad03, short_gen337, long_gen337, 0xcb0bfa11 },
{ 0xacebad03, short_gen338, long_gen338, 0xcb0bfa11 },
{ 0xacebad03, short_gen339, long_gen339, 0xcb0bfa11 },
{ 0xacebad03, short_gen340, long_gen340, 0xcb0bfa11 },
{ 0xacebad03, short_gen341, long_gen341, 0xcb0bfa11 },
{ 0xacebad03, short_gen342, long_gen342, 0xcb0bfa11 },
{ 0xacebad03, short_gen343, long_gen343, 0xcb0bfa11 },
{ 0xacebad03, short_gen344, long_gen344, 0xcb0bfa11 },
{ 0xacebad03, short_gen345, long_gen345, 0xcb0bfa11 },
{ 0xacebad03, short_gen346, long_gen346, 0xcb0bfa11 },
{ 0xacebad03, short_gen347, long_gen347, 0xcb0bfa11 },
{ 0xacebad03, short_gen348, long_gen348, 0xcb0bfa11 },
{ 0xacebad03, short_gen349, long_gen349, 0xcb0bfa11 },
{ 0xacebad03, short_gen350, long_gen350, 0xcb0bfa11 },
{ 0xacebad03, short_gen351, long_gen351, 0xcb0bfa11 },
{ 0xacebad03, short_gen352, long_gen352, 0xcb0bfa11 },
{ 0xacebad03, short_gen353, long_gen353, 0xcb0bfa11 },
{ 0xacebad03, short_gen354, long_gen354, 0xcb0bfa11 },
{ 0xacebad03, short_gen355, long_gen355, 0xcb0bfa11 },
{ 0xacebad03, short_gen356, long_gen356, 0xcb0bfa11 },
{ 0xacebad03, short_gen357, long_gen357, 0xcb0bfa11 },
{ 0xacebad03, short_gen358, long_gen358, 0xcb0bfa11 },
{ 0xacebad03, short_gen359, long_gen359, 0xcb0bfa11 },
{ 0xacebad03, short_gen360, long_gen360, 0xcb0bfa11 },
{ 0xacebad03, short_gen361, long_gen361, 0xcb0bfa11 },
{ 0xacebad03, short_gen362, long_gen362, 0xcb0bfa11 },
{ 0xacebad03, short_gen363, long_gen363, 0xcb0bfa11 },
{ 0xacebad03, short_gen364, long_gen364, 0xcb0bfa11 },
{ 0xacebad03, short_gen365, long_gen365, 0xcb0bfa11 },
{ 0xacebad03, short_gen366, long_gen366, 0xcb0bfa11 },
{ 0xacebad03, short_gen367, long_gen367, 0xcb0bfa11 },
{ 0xacebad03, short_gen368, long_gen368, 0xcb0bfa11 },
{ 0xacebad03, short_gen369, long_gen369, 0xcb0bfa11 },
{ 0xacebad03, short_gen370, long_gen370, 0xcb0bfa11 },
{ 0xacebad03, short_gen371, long_gen371, 0xcb0bfa11 },
{ 0xacebad03, short_gen372, long_gen372, 0xcb0bfa11 },
{ 0xacebad03, short_gen373, long_gen373, 0xcb0bfa11 },
{ 0xacebad03, short_gen374, long_gen374, 0xcb0bfa11 },
{ 0xacebad03, short_gen375, long_gen375, 0xcb0bfa11 },
{ 0xacebad03, short_gen376, long_gen376, 0xcb0bfa11 },
{ 0xacebad03, short_gen377, long_gen377, 0xcb0bfa11 },
{ 0xacebad03, short_gen378, long_gen378, 0xcb0bfa11 },
{ 0xacebad03, short_gen379, long_gen379, 0xcb0bfa11 },
{ 0xacebad03, short_gen380, long_gen380, 0xcb0bfa11 },
{ 0xacebad03, short_gen381, long_gen381, 0xcb0bfa11 },
{ 0xacebad03, short_gen382, long_gen382, 0xcb0bfa11 },
{ 0xacebad03, short_gen383, long_gen383, 0xcb0bfa11 },
{ 0xacebad03, short_gen384, long_gen384, 0xcb0bfa11 },
{ 0xacebad03, short_gen385, long_gen385, 0xcb0bfa11 },
{ 0xacebad03, short_gen386, long_gen386, 0xcb0bfa11 },
{ 0xacebad03, short_gen387, long_gen387, 0xcb0bfa11 },
{ 0xacebad03, short_gen388, long_gen388, 0xcb0bfa11 },
{ 0xacebad03, short_gen389, long_gen389, 0xcb0bfa11 },
{ 0xacebad03, short_gen390, long_gen390, 0xcb0bfa11 },
{ 0xacebad03, short_gen391, long_gen391, 0xcb0bfa11 },
{ 0xacebad03, short_gen392, long_gen392, 0xcb0bfa11 },
{ 0xacebad03, short_gen393, long_gen393, 0xcb0bfa11 },
{ 0xacebad03, short_gen394, long_gen394, 0xcb0bfa11 },
{ 0xacebad03, short_gen395, long_gen395, 0xcb0bfa11 },
{ 0xacebad03, short_gen396, long_gen396, 0xcb0bfa11 },
{ 0xacebad03, short_gen397, long_gen397, 0xcb0bfa11 },
{ 0xacebad03, short_gen398, long_gen398, 0xcb0bfa11 },
{ 0xacebad03, short_gen399, long_gen399, 0xcb0bfa11 },
{ 0xacebad03, short_gen400, long_gen400, 0xcb0bfa11 },
{ 0xacebad03, short_gen401, long_gen401, 0xcb0bfa11 },
{ 0xacebad03, short_gen402, long_gen402, 0xcb0bfa11 },
{ 0xacebad03, short_gen403, long_gen403, 0xcb0bfa11 },
{ 0xacebad03, short_gen404, long_gen404, 0xcb0bfa11 },
{ 0xacebad03, short_gen405, long_gen405, 0xcb0bfa11 },
{ 0xacebad03, short_gen406, long_gen406, 0xcb0bfa11 },
{ 0xacebad03, short_gen407, long_gen407, 0xcb0bfa11 },
{ 0xacebad03, short_gen408, long_gen408, 0xcb0bfa11 },
{ 0xacebad03, short_gen409, long_gen409, 0xcb0bfa11 },
{ 0xacebad03, short_gen410, long_gen410, 0xcb0bfa11 },
{ 0xacebad03, short_gen411, long_gen411, 0xcb0bfa11 },
{ 0xacebad03, short_gen412, long_gen412, 0xcb0bfa11 },
{ 0xacebad03, short_gen413, long_gen413, 0xcb0bfa11 },
{ 0xacebad03, short_gen414, long_gen414, 0xcb0bfa11 },
{ 0xacebad03, short_gen415, long_gen415, 0xcb0bfa11 },
{ 0xacebad03, short_gen416, long_gen416, 0xcb0bfa11 },
{ 0xacebad03, short_gen417, long_gen417, 0xcb0bfa11 },
{ 0xacebad03, short_gen418, long_gen418, 0xcb0bfa11 },
{ 0xacebad03, short_gen419, long_gen419, 0xcb0bfa11 },
{ 0xacebad03, short_gen420, long_gen420, 0xcb0bfa11 },
{ 0xacebad03, short_gen421, long_gen421, 0xcb0bfa11 },
{ 0xacebad03, short_gen422, long_gen422, 0xcb0bfa11 },
{ 0xacebad03, short_gen423, long_gen423, 0xcb0bfa11 },
{ 0xacebad03, short_gen424, long_gen424, 0xcb0bfa11 },
{ 0xacebad03, short_gen425, long_gen425, 0xcb0bfa11 },
{ 0xacebad03, short_gen426, long_gen426, 0xcb0bfa11 },
{ 0xacebad03, short_gen427, long_gen427, 0xcb0bfa11 },
{ 0xacebad03, short_gen428, long_gen428, 0xcb0bfa11 },
{ 0xacebad03, short_gen429, long_gen429, 0xcb0bfa11 },
{ 0xacebad03, short_gen430, long_gen430, 0xcb0bfa11 },
{ 0xacebad03, short_gen431, long_gen431, 0xcb0bfa11 },
{ 0xacebad03, short_gen432, long_gen432, 0xcb0bfa11 },
{ 0xacebad03, short_gen433, long_gen433, 0xcb0bfa11 },
{ 0xacebad03, short_gen434, long_gen434, 0xcb0bfa11 },
{ 0xacebad03, short_gen435, long_gen435, 0xcb0bfa11 },
{ 0xacebad03, short_gen436, long_gen436, 0xcb0bfa11 },
{ 0xacebad03, short_gen437, long_gen437, 0xcb0bfa11 },
{ 0xacebad03, short_gen438, long_gen438, 0xcb0bfa11 },
{ 0xacebad03, short_gen439, long_gen439, 0xcb0bfa11 },
{ 0xacebad03, short_gen440, long_gen440, 0xcb0bfa11 },
{ 0xacebad03, short_gen441, long_gen441, 0xcb0bfa11 },
{ 0xacebad03, short_gen442, long_gen442, 0xcb0bfa11 },
{ 0xacebad03, short_gen443, long_gen443, 0xcb0bfa11 },
{ 0xacebad03, short_gen444, long_gen444, 0xcb0bfa11 },
{ 0xacebad03, short_gen445, long_gen445, 0xcb0bfa11 },
{ 0xacebad03, short_gen446, long_gen446, 0xcb0bfa11 },
{ 0xacebad03, short_gen447, long_gen447, 0xcb0bfa11 },
{ 0xacebad03, short_gen448, long_gen448, 0xcb0bfa11 },
{ 0xacebad03, short_gen449, long_gen449, 0xcb0bfa11 },
{ 0xacebad03, short_gen450, long_gen450, 0xcb0bfa11 },
{ 0xacebad03, short_gen451, long_gen451, 0xcb0bfa11 },
{ 0xacebad03, short_gen452, long_gen452, 0xcb0bfa11 },
{ 0xacebad03, short_gen453, long_gen453, 0xcb0bfa11 },
{ 0xacebad03, short_gen454, long_gen454, 0xcb0bfa11 },
{ 0xacebad03, short_gen455, long_gen455, 0xcb0bfa11 },
{ 0xacebad03, short_gen456, long_gen456, 0xcb0bfa11 },
{ 0xacebad03, short_gen457, long_gen457, 0xcb0bfa11 },
{ 0xacebad03, short_gen458, long_gen458, 0xcb0bfa11 },
{ 0xacebad03, short_gen459, long_gen459, 0xcb0bfa11 },
{ 0xacebad03, short_gen460, long_gen460, 0xcb0bfa11 },
{ 0xacebad03, short_gen461, long_gen461, 0xcb0bfa11 },
{ 0xacebad03, short_gen462, long_gen462, 0xcb0bfa11 },
{ 0xacebad03, short_gen463, long_gen463, 0xcb0bfa11 },
{ 0xacebad03, short_gen464, long_gen464, 0xcb0bfa11 },
{ 0xacebad03, short_gen465, long_gen465, 0xcb0bfa11 },
{ 0xacebad03, short_gen466, long_gen466, 0xcb0bfa11 },
{ 0xacebad03, short_gen467, long_gen467, 0xcb0bfa11 },
{ 0xacebad03, short_gen468, long_gen468, 0xcb0bfa11 },
{ 0xacebad03, short_gen469, long_gen469, 0xcb0bfa11 },
{ 0xacebad03, short_gen470, long_gen470, 0xcb0bfa11 },
{ 0xacebad03, short_gen471, long_gen471, 0xcb0bfa11 },
{ 0xacebad03, short_gen472, long_gen472, 0xcb0bfa11 },
{ 0xacebad03, short_gen473, long_gen473, 0xcb0bfa11 },
{ 0xacebad03, short_gen474, long_gen474, 0xcb0bfa11 },
{ 0xacebad03, short_gen475, long_gen475, 0xcb0bfa11 },
{ 0xacebad03, short_gen476, long_gen476, 0xcb0bfa11 },
{ 0xacebad03, short_gen477, long_gen477, 0xcb0bfa11 },
{ 0xacebad03, short_gen478, long_gen478, 0xcb0bfa11 },
{ 0xacebad03, short_gen479, long_gen479, 0xcb0bfa11 },
{ 0xacebad03, short_gen480, long_gen480, 0xcb0bfa11 },
{ 0xacebad03, short_gen481, long_gen481, 0xcb0bfa11 },
{ 0xacebad03, short_gen482, long_gen482, 0xcb0bfa11 },
{ 0xacebad03, short_gen483, long_gen483, 0xcb0bfa11 },
{ 0xacebad03, short_gen484, long_gen484, 0xcb0bfa11 },
{ 0xacebad03, short_gen485, long_gen485, 0xcb0bfa11 },
{ 0xacebad03, short_gen486, long_gen486, 0xcb0bfa11 },
{ 0xacebad03, short_gen487, long_gen487, 0xcb0bfa11 },
{ 0xacebad03, short_gen488, long_gen488, 0xcb0bfa11 },
{ 0xacebad03, short_gen489, long_gen489, 0xcb0bfa11 },
{ 0xacebad03, short_gen490, long_gen490, 0xcb0bfa11 },
{ 0xacebad03, short_gen491, long_gen491, 0xcb0bfa11 },
{ 0xacebad03, short_gen492, long_gen492, 0xcb0bfa11 },
{ 0xacebad03, short_gen493, long_gen493, 0xcb0bfa11 },
{ 0xacebad03, short_gen494, long_gen494, 0xcb0bfa11 },
{ 0xacebad03, short_gen495, long_gen495, 0xcb0bfa11 },
{ 0xacebad03, short_gen496, long_gen496, 0xcb0bfa11 },
{ 0xacebad03, short_gen497, long_gen497, 0xcb0bfa11 },
{ 0xacebad03, short_gen498, long_gen498, 0xcb0bfa11 },
{ 0xacebad03, short_gen499, long_gen499, 0xcb0bfa11 },
{ 0xacebad03, short_gen500, long_gen500, 0xcb0bfa11 },
{ 0xacebad03, short_gen501, long_gen501, 0xcb0bfa11 },
{ 0xacebad03, short_gen502, long_gen502, 0xcb0bfa11 },
{ 0xacebad03, short_gen503, long_gen503, 0xcb0bfa11 },
{ 0xacebad03, short_gen504, long_gen504, 0xcb0bfa11 },
{ 0xacebad03, short_gen505, long_gen505, 0xcb0bfa11 },
{ 0xacebad03, short_gen506, long_gen506, 0xcb0bfa11 },
{ 0xacebad03, short_gen507, long_gen507, 0xcb0bfa11 },
{ 0xacebad03, short_gen508, long_gen508, 0xcb0bfa11 },
{ 0xacebad03, short_gen509, long_gen509, 0xcb0bfa11 },
{ 0xacebad03, short_gen510, long_gen510, 0xcb0bfa11 },
{ 0xacebad03, short_gen511, long_gen511, 0xcb0bfa11 },
{ 0xacebad03, short_gen512, long_gen512, 0xcb0bfa11 },
{ 0xacebad03, short_gen513, long_gen513, 0xcb0bfa11 },
{ 0xacebad03, short_gen514, long_gen514, 0xcb0bfa11 },
{ 0xacebad03, short_gen515, long_gen515, 0xcb0bfa11 },
{ 0xacebad03, short_gen516, long_gen516, 0xcb0bfa11 },
{ 0xacebad03, short_gen517, long_gen517, 0xcb0bfa11 },
{ 0xacebad03, short_gen518, long_gen518, 0xcb0bfa11 },
{ 0xacebad03, short_gen519, long_gen519, 0xcb0bfa11 },
{ 0xacebad03, short_gen520, long_gen520, 0xcb0bfa11 },
{ 0xacebad03, short_gen521, long_gen521, 0xcb0bfa11 },
{ 0xacebad03, short_gen522, long_gen522, 0xcb0bfa11 },
{ 0xacebad03, short_gen523, long_gen523, 0xcb0bfa11 },
{ 0xacebad03, short_gen524, long_gen524, 0xcb0bfa11 },
{ 0xacebad03, short_gen525, long_gen525, 0xcb0bfa11 },
{ 0xacebad03, short_gen526, long_gen526, 0xcb0bfa11 },
{ 0xacebad03, short_gen527, long_gen527, 0xcb0bfa11 },
{ 0xacebad03, short_gen528, long_gen528, 0xcb0bfa11 },
{ 0xacebad03, short_gen529, long_gen529, 0xcb0bfa11 },
{ 0xacebad03, short_gen530, long_gen530, 0xcb0bfa11 },
{ 0xacebad03, short_gen531, long_gen531, 0xcb0bfa11 },
{ 0xacebad03, short_gen532, long_gen532, 0xcb0bfa11 },
{ 0xacebad03, short_gen533, long_gen533, 0xcb0bfa11 },
{ 0xacebad03, short_gen534, long_gen534, 0xcb0bfa11 },
{ 0xacebad03, short_gen535, long_gen535, 0xcb0bfa11 },
{ 0xacebad03, short_gen536, long_gen536, 0xcb0bfa11 },
{ 0xacebad03, short_gen537, long_gen537, 0xcb0bfa11 },
{ 0xacebad03, short_gen538, long_gen538, 0xcb0bfa11 },
{ 0xacebad03, short_gen539, long_gen539, 0xcb0bfa11 },
{ 0xacebad03, short_gen540, long_gen540, 0xcb0bfa11 },
{ 0xacebad03, short_gen541, long_gen541, 0xcb0bfa11 },
{ 0xacebad03, short_gen542, long_gen542, 0xcb0bfa11 },
{ 0xacebad03, short_gen543, long_gen543, 0xcb0bfa11 },
{ 0xacebad03, short_gen544, long_gen544, 0xcb0bfa11 },
{ 0xacebad03, short_gen545, long_gen545, 0xcb0bfa11 },
{ 0xacebad03, short_gen546, long_gen546, 0xcb0bfa11 },
{ 0xacebad03, short_gen547, long_gen547, 0xcb0bfa11 },
{ 0xacebad03, short_gen548, long_gen548, 0xcb0bfa11 },
{ 0xacebad03, short_gen549, long_gen549, 0xcb0bfa11 },
{ 0xacebad03, short_gen550, long_gen550, 0xcb0bfa11 },
{ 0xacebad03, short_gen551, long_gen551, 0xcb0bfa11 },
{ 0xacebad03, short_gen552, long_gen552, 0xcb0bfa11 },
{ 0xacebad03, short_gen553, long_gen553, 0xcb0bfa11 },
{ 0xacebad03, short_gen554, long_gen554, 0xcb0bfa11 },
{ 0xacebad03, short_gen555, long_gen555, 0xcb0bfa11 },
{ 0xacebad03, short_gen556, long_gen556, 0xcb0bfa11 },
{ 0xacebad03, short_gen557, long_gen557, 0xcb0bfa11 },
{ 0xacebad03, short_gen558, long_gen558, 0xcb0bfa11 },
{ 0xacebad03, short_gen559, long_gen559, 0xcb0bfa11 },
{ 0xacebad03, short_gen560, long_gen560, 0xcb0bfa11 },
{ 0xacebad03, short_gen561, long_gen561, 0xcb0bfa11 },
{ 0xacebad03, short_gen562, long_gen562, 0xcb0bfa11 },
{ 0xacebad03, short_gen563, long_gen563, 0xcb0bfa11 },
{ 0xacebad03, short_gen564, long_gen564, 0xcb0bfa11 },
{ 0xacebad03, short_gen565, long_gen565, 0xcb0bfa11 },
{ 0xacebad03, short_gen566, long_gen566, 0xcb0bfa11 },
{ 0xacebad03, short_gen567, long_gen567, 0xcb0bfa11 },
{ 0xacebad03, short_gen568, long_gen568, 0xcb0bfa11 },
{ 0xacebad03, short_gen569, long_gen569, 0xcb0bfa11 },
{ 0xacebad03, short_gen570, long_gen570, 0xcb0bfa11 },
{ 0xacebad03, short_gen571, long_gen571, 0xcb0bfa11 },
{ 0xacebad03, short_gen572, long_gen572, 0xcb0bfa11 },
{ 0xacebad03, short_gen573, long_gen573, 0xcb0bfa11 },
{ 0xacebad03, short_gen574, long_gen574, 0xcb0bfa11 },
{ 0xacebad03, short_gen575, long_gen575, 0xcb0bfa11 },
{ 0xacebad03, short_gen576, long_gen576, 0xcb0bfa11 },
{ 0xacebad03, short_gen577, long_gen577, 0xcb0bfa11 },
{ 0xacebad03, short_gen578, long_gen578, 0xcb0bfa11 },
{ 0xacebad03, short_gen579, long_gen579, 0xcb0bfa11 },
{ 0xacebad03, short_gen580, long_gen580, 0xcb0bfa11 },
{ 0xacebad03, short_gen581, long_gen581, 0xcb0bfa11 },
{ 0xacebad03, short_gen582, long_gen582, 0xcb0bfa11 },
{ 0xacebad03, short_gen583, long_gen583, 0xcb0bfa11 },
{ 0xacebad03, short_gen584, long_gen584, 0xcb0bfa11 },
{ 0xacebad03, short_gen585, long_gen585, 0xcb0bfa11 },
{ 0xacebad03, short_gen586, long_gen586, 0xcb0bfa11 },
{ 0xacebad03, short_gen587, long_gen587, 0xcb0bfa11 },
{ 0xacebad03, short_gen588, long_gen588, 0xcb0bfa11 },
{ 0xacebad03, short_gen589, long_gen589, 0xcb0bfa11 },
{ 0xacebad03, short_gen590, long_gen590, 0xcb0bfa11 },
{ 0xacebad03, short_gen591, long_gen591, 0xcb0bfa11 },
{ 0xacebad03, short_gen592, long_gen592, 0xcb0bfa11 },
{ 0xacebad03, short_gen593, long_gen593, 0xcb0bfa11 },
{ 0xacebad03, short_gen594, long_gen594, 0xcb0bfa11 },
{ 0xacebad03, short_gen595, long_gen595, 0xcb0bfa11 },
{ 0xacebad03, short_gen596, long_gen596, 0xcb0bfa11 },
{ 0xacebad03, short_gen597, long_gen597, 0xcb0bfa11 },
{ 0xacebad03, short_gen598, long_gen598, 0xcb0bfa11 },
{ 0xacebad03, short_gen599, long_gen599, 0xcb0bfa11 },
{ 0xacebad03, short_gen600, long_gen600, 0xcb0bfa11 },
{ 0xacebad03, short_gen601, long_gen601, 0xcb0bfa11 },
{ 0xacebad03, short_gen602, long_gen602, 0xcb0bfa11 },
{ 0xacebad03, short_gen603, long_gen603, 0xcb0bfa11 },
{ 0xacebad03, short_gen604, long_gen604, 0xcb0bfa11 },
{ 0xacebad03, short_gen605, long_gen605, 0xcb0bfa11 },
{ 0xacebad03, short_gen606, long_gen606, 0xcb0bfa11 },
{ 0xacebad03, short_gen607, long_gen607, 0xcb0bfa11 },
{ 0xacebad03, short_gen608, long_gen608, 0xcb0bfa11 },
{ 0xacebad03, short_gen609, long_gen609, 0xcb0bfa11 },
{ 0xacebad03, short_gen610, long_gen610, 0xcb0bfa11 },
{ 0xacebad03, short_gen611, long_gen611, 0xcb0bfa11 },
{ 0xacebad03, short_gen612, long_gen612, 0xcb0bfa11 },
{ 0xacebad03, short_gen613, long_gen613, 0xcb0bfa11 },
{ 0xacebad03, short_gen614, long_gen614, 0xcb0bfa11 },
{ 0xacebad03, short_gen615, long_gen615, 0xcb0bfa11 },
{ 0xacebad03, short_gen616, long_gen616, 0xcb0bfa11 },
{ 0xacebad03, short_gen617, long_gen617, 0xcb0bfa11 },
{ 0xacebad03, short_gen618, long_gen618, 0xcb0bfa11 },
{ 0xacebad03, short_gen619, long_gen619, 0xcb0bfa11 },
{ 0xacebad03, short_gen620, long_gen620, 0xcb0bfa11 },
{ 0xacebad03, short_gen621, long_gen621, 0xcb0bfa11 },
{ 0xacebad03, short_gen622, long_gen622, 0xcb0bfa11 },
{ 0xacebad03, short_gen623, long_gen623, 0xcb0bfa11 },
{ 0xacebad03, short_gen624, long_gen624, 0xcb0bfa11 },
{ 0xacebad03, short_gen625, long_gen625, 0xcb0bfa11 },
{ 0xacebad03, short_gen626, long_gen626, 0xcb0bfa11 },
{ 0xacebad03, short_gen627, long_gen627, 0xcb0bfa11 },
{ 0xacebad03, short_gen628, long_gen628, 0xcb0bfa11 },
{ 0xacebad03, short_gen629, long_gen629, 0xcb0bfa11 },
{ 0xacebad03, short_gen630, long_gen630, 0xcb0bfa11 },
{ 0xacebad03, short_gen631, long_gen631, 0xcb0bfa11 },
{ 0xacebad03, short_gen632, long_gen632, 0xcb0bfa11 },
{ 0xacebad03, short_gen633, long_gen633, 0xcb0bfa11 },
{ 0xacebad03, short_gen634, long_gen634, 0xcb0bfa11 },
{ 0xacebad03, short_gen635, long_gen635, 0xcb0bfa11 },
{ 0xacebad03, short_gen636, long_gen636, 0xcb0bfa11 },
{ 0xacebad03, short_gen637, long_gen637, 0xcb0bfa11 },
{ 0xacebad03, short_gen638, long_gen638, 0xcb0bfa11 },
{ 0xacebad03, short_gen639, long_gen639, 0xcb0bfa11 },
{ 0xacebad03, short_gen640, long_gen640, 0xcb0bfa11 },
{ 0xacebad03, short_gen641, long_gen641, 0xcb0bfa11 },
{ 0xacebad03, short_gen642, long_gen642, 0xcb0bfa11 },
{ 0xacebad03, short_gen643, long_gen643, 0xcb0bfa11 },
{ 0xacebad03, short_gen644, long_gen644, 0xcb0bfa11 },
{ 0xacebad03, short_gen645, long_gen645, 0xcb0bfa11 },
{ 0xacebad03, short_gen646, long_gen646, 0xcb0bfa11 },
{ 0xacebad03, short_gen647, long_gen647, 0xcb0bfa11 },
{ 0xacebad03, short_gen648, long_gen648, 0xcb0bfa11 },
{ 0xacebad03, short_gen649, long_gen649, 0xcb0bfa11 },
{ 0xacebad03, short_gen650, long_gen650, 0xcb0bfa11 },
{ 0xacebad03, short_gen651, long_gen651, 0xcb0bfa11 },
{ 0xacebad03, short_gen652, long_gen652, 0xcb0bfa11 },
{ 0xacebad03, short_gen653, long_gen653, 0xcb0bfa11 },
{ 0xacebad03, short_gen654, long_gen654, 0xcb0bfa11 },
{ 0xacebad03, short_gen655, long_gen655, 0xcb0bfa11 },
{ 0xacebad03, short_gen656, long_gen656, 0xcb0bfa11 },
{ 0xacebad03, short_gen657, long_gen657, 0xcb0bfa11 },
{ 0xacebad03, short_gen658, long_gen658, 0xcb0bfa11 }
};
#endif

#if MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG_GENERIC
static const char short_spc0[] = "**CreateFileMapping %d";
static const char long_spc0[]  = "CreateFileMapping failed, error %d";
static const char short_spc1[] = "**CreateThread %d";
static const char long_spc1[]  = "CreateThread failed, error %d";
static const char short_spc2[] = "**FindWindowEx %d";
static const char long_spc2[]  = "FindWindowEx failed, error %d";
static const char short_spc3[] = "**MPIDI_CH3I_SHM_Attach_to_mem %d";
static const char long_spc3[]  = "MPIDI_CH3I_SHM_Attach_to_mem failed, error %d";
static const char short_spc4[] = "**MapViewOfFileEx %d";
static const char long_spc4[]  = "MapViewOfFileEx failed, error %d";
static const char short_spc5[] = "**OpenProcess %d %d";
static const char long_spc5[]  = "OpenProcess failed for process %d, error %d";
static const char short_spc6[] = "**arg %s";
static const char long_spc6[]  = "Invalid argument %s";
static const char short_spc7[] = "**argarrayneg %s %d %d";
static const char long_spc7[]  = "Negative value in array %s[%d] (value is %d)";
static const char short_spc8[] = "**argerrcode %d";
static const char long_spc8[]  = "Invalid error code %d";
static const char short_spc9[] = "**argneg %s %d";
static const char long_spc9[]  = "Invalid value for %s, must be non-negative but is %d";
static const char short_spc10[] = "**argnonpos %s %d";
static const char long_spc10[]  = "Invalid value for %s; must be positive but is %d";
static const char short_spc11[] = "**argrange %s %d %d";
static const char long_spc11[]  = "Argument %s has value %d but must be within [0,%d]";
static const char short_spc12[] = "**attach_to_mem %d";
static const char long_spc12[]  = "attach to shared memory returned error %d";
static const char short_spc13[] = "**bad_conn %p %p";
static const char long_spc13[]  = "bad conn structure pointer (%p != %p)";
static const char short_spc14[] = "**bad_sock %d %d";
static const char long_spc14[]  = "bad sock (%d != %d)";
static const char short_spc15[] = "**badpacket %d";
static const char long_spc15[]  = "Received a packet of unknown type (%d)";
static const char short_spc16[] = "**boot_attach %s";
static const char long_spc16[]  = "failed to attach to a bootstrap queue - %s";
static const char short_spc17[] = "**bootqmsg %d";
static const char long_spc17[]  = "invalid bootstrap queue message size (%d bytes)";
static const char short_spc18[] = "**bsendbufsmall %d %d";
static const char long_spc18[]  = "Buffer size of %d is smaller than MPI_BSEND_OVERHEAD (%d)";
static const char short_spc19[] = "**bufbsend %d %d";
static const char long_spc19[]  = "Insufficient space in Bsend buffer; requested %d; total buffer size is %d";
static const char short_spc20[] = "**business_card %s";
static const char long_spc20[]  = "Invalid business card (%s)";
static const char short_spc21[] = "**ca_guids %s";
static const char long_spc21[]  = "unable to get the infiniband guids - %s";
static const char short_spc22[] = "**cartcoordinvalid %d %d %d";
static const char long_spc22[]  = " Cartesian coordinate for the %d coordinate is %d but must be between 0 and %d";
static const char short_spc23[] = "**cartdim %d %d";
static const char long_spc23[]  = "Size of the communicator (%d) is smaller than the size of the Cartesian topology (%d)";
static const char short_spc24[] = "**ch3|badca %d";
static const char long_spc24[]  = "specified completion action in not known (%d)";
static const char short_spc25[] = "**ch3|badmsgtype %d";
static const char long_spc25[]  = "request contained an invalid message type (%d)";
static const char short_spc26[] = "**ch3|badreqtype %d";
static const char long_spc26[]  = "request contained an invalid request type (%d)";
static const char short_spc27[] = "**ch3|loadrecviov %s";
static const char long_spc27[]  = "failure occurred while loading the receive I/O vector (%s)";
static const char short_spc28[] = "**ch3|postrecv %s";
static const char long_spc28[]  = "failure occurred while posting a receive for message data (%s)";
static const char short_spc29[] = "**ch3|recvdata %s";
static const char long_spc29[]  = "failure occurred while attempting to receive message data (%s)";
static const char short_spc30[] = "**ch3|sock|badbuscard %s";
static const char long_spc30[]  = "[ch3:sock] GetHostAndPort - Invalid business card (%s)";
static const char short_spc31[] = "**ch3|sock|badhost %s %d %s";
static const char long_spc31[]  = "[ch3:sock] invalid host description, %s:%d (%s)";
static const char short_spc32[] = "**ch3|sock|badpacket %d";
static const char long_spc32[]  = "[ch3:sock] received packet of unknown type (%d)";
static const char short_spc33[] = "**ch3|sock|connfailed %d %d";
static const char long_spc33[]  = "[ch3:sock] failed to connnect to remote process %d:%d";
static const char short_spc34[] = "**ch3|sock|connrefused %s %d %s";
static const char long_spc34[]  = "[ch3:sock] failed to connect to process %s:%d (%s)";
static const char short_spc35[] = "**ch3|sock|conparent %s";
static const char long_spc35[]  = "[ch3:sock] spawned group unable to connect back to the parent on port <%s>";
static const char short_spc36[] = "**ch3|sock|failure %d";
static const char long_spc36[]  = "[ch3:sock] unknown failure, sock_errno=%d";
static const char short_spc37[] = "**ch3|sock|pmi_finalize %d";
static const char long_spc37[]  = "PMI_Finalize failed, error %d";
static const char short_spc38[] = "**ch3|sock|postconnect %d %d %s";
static const char long_spc38[]  = "[ch3:sock] rank %d unable to connect to rank %d using business card <%s>";
static const char short_spc39[] = "**ch3|unknownpkt %d";
static const char long_spc39[]  = "received unknown packet type (type=%d)";
static const char short_spc40[] = "**commperm %s";
static const char long_spc40[]  = "Cannot free permanent communicator %s";
static const char short_spc41[] = "**connfailed %d %d";
static const char long_spc41[]  = "Failed to connect to remote process %d-%d";
static const char short_spc42[] = "**connrefused %d %d %s";
static const char long_spc42[]  = "Connection refused for process group %d, rank %d, business card <%s>";
static const char short_spc43[] = "**countneg %d";
static const char long_spc43[]  = "Negative count, value is %d";
static const char short_spc44[] = "**darrayblock %d";
static const char long_spc44[]  = " Value of m in block(m) distribution is %d must must be positive";
static const char short_spc45[] = "**darrayblock2 %d %d";
static const char long_spc45[]  = "m * nprocs is %d but must equal the array size %d and is not valid for block(m) distribution";
static const char short_spc46[] = "**darraycyclic %d";
static const char long_spc46[]  = "Value of m is %d but must be positive for a cyclic(m) distribution";
static const char short_spc47[] = "**darraydist %d %d";
static const char long_spc47[]  = "For MPI_DISTRIBUTE_NONE, the value of array_of_psizes[%d] is %d but must have value 1";
static const char short_spc48[] = "**dims %d";
static const char long_spc48[]  = "Invalid dimension argument (value is %d)";
static const char short_spc49[] = "**dimsmany %d %d";
static const char long_spc49[]  = "Number of dimensions %d is too large (maximum is %d)";
static const char short_spc50[] = "**duphandle %s %d";
static const char long_spc50[]  = "unable to duplicate a handle, %s (errno %d)";
static const char short_spc51[] = "**dupprocesses %d";
static const char long_spc51[]  = "Local and remote groups in MPI_Intercomm_create must not contain the same processes; both contain process %d";
static const char short_spc52[] = "**edgeoutrange %d %d %d";
static const char long_spc52[]  = "Edge index edges[%d] is %d but must be nonnegative and less than %d";
static const char short_spc53[] = "**fail %d";
static const char long_spc53[]  = "generic failure with errno = %d";
static const char short_spc54[] = "**fail %s";
static const char long_spc54[]  = "%s";
static const char short_spc55[] = "**fail %s %d";
static const char long_spc55[]  = "%s (errno %d)";
static const char short_spc56[] = "**fileaccess %s";
static const char long_spc56[]  = "Access denied to file %s";
static const char short_spc57[] = "**filenamedir %s";
static const char long_spc57[]  = "Invalid or missing directory %s";
static const char short_spc58[] = "**filenamelong %s %d";
static const char long_spc58[]  = "Pathname %s too long (%d characters)";
static const char short_spc59[] = "**filenoexist %s";
static const char long_spc59[]  = "File %s does not exist";
static const char short_spc60[] = "**filerdonly %s";
static const char long_spc60[]  = "Read-only file or filesystem name %s";
static const char short_spc61[] = "**ftok %s %d %d";
static const char long_spc61[]  = "failed to create a sysv key from the file '%s' and id %d, error %d";
static const char short_spc62[] = "**ftruncate %s %d %d";
static const char long_spc62[]  = "unable to resize the shared memory object %s to size %d (errno %d)";
static const char short_spc63[] = "**gethostbyname %s %d";
static const char long_spc63[]  = "gethostbyname failed, %s (errno %d)";
static const char short_spc64[] = "**getinfo %s %d";
static const char long_spc64[]  = "getinfo failed, %s (errno %d)";
static const char short_spc65[] = "**groupnotincomm %d";
static const char long_spc65[]  = "Rank %d of the specified group is not a member of this communicator";
static const char short_spc66[] = "**hostlookup %d %d %s";
static const char long_spc66[]  = "Host lookup failed for process group %d, rank %d, business card <%s>";
static const char short_spc67[] = "**ibu_op %d";
static const char long_spc67[]  = "invalid infiniband operation (%d)";
static const char short_spc68[] = "**ibu_wait %d";
static const char long_spc68[]  = "ibu_wait failed (errno %d)";
static const char short_spc69[] = "**indexneg %d %d";
static const char long_spc69[]  = "Index value for index[%d] is %d but must be nonnegative";
static const char short_spc70[] = "**indexnonmonotone %d %d %d";
static const char long_spc70[]  = "Index values in graph topology must be monotone nondecreasing but index[%d] is %d but the next index value is %d";
static const char short_spc71[] = "**infonkey %d %d";
static const char long_spc71[]  = "Requested key %d but this MPI_Info only has %d keys";
static const char short_spc72[] = "**infonokey %s";
static const char long_spc72[]  = "MPI_Info key %s is not defined ";
static const char short_spc73[] = "**intern %s";
static const char long_spc73[]  = "Internal MPI error!  %s";
static const char short_spc74[] = "**invalid_handle %d";
static const char long_spc74[]  = "invalid handle (%d)";
static const char short_spc75[] = "**invalid_listener %p";
static const char long_spc75[]  = "invalid listener (%p)";
static const char short_spc76[] = "**invalid_refcount %d";
static const char long_spc76[]  = "invalid reference count (%d)";
static const char short_spc77[] = "**io %s";
static const char long_spc77[]  = "Other I/O error %s";
static const char short_spc78[] = "**iocp %s %d";
static const char long_spc78[]  = "unable to create an I/O copmletion port, %s (erron %d)";
static const char short_spc79[] = "**iov_offset %d %d";
static const char long_spc79[]  = "invalid iov offset (%d > %d)";
static const char short_spc80[] = "**listen %s %d";
static const char long_spc80[]  = "listen failed, %s (errno %d)";
static const char short_spc81[] = "**mmap %d";
static const char long_spc81[]  = " unable to map memory, mmap failed (errno %d)";
static const char short_spc82[] = "**mpi_abort %C %d";
static const char long_spc82[]  = "MPI_Abort(%C, errorcode=%d) failed";
static const char short_spc83[] = "**mpi_accumulate %p %d %D %d %d %d %D %O %W";
static const char long_spc83[]  = "MPI_Accumulate(origin_addr=%p, origin_count=%d, %D, target_rank=%d, target_disp=%d, target_count=%d, %D, %O, %W) failed";
static const char short_spc84[] = "**mpi_add_error_class %p";
static const char long_spc84[]  = "MPI_Add_error_class(errorclass=%p) failed";
static const char short_spc85[] = "**mpi_add_error_code %d %p";
static const char long_spc85[]  = "MPI_Add_error_code(errorclass=%d, errorcode=%p) failed";
static const char short_spc86[] = "**mpi_add_error_string %d %s";
static const char long_spc86[]  = "MPI_Add_error_string(code=%d, str=\"%s\") failed";
static const char short_spc87[] = "**mpi_address %p %p";
static const char long_spc87[]  = "MPI_Address(location=%p, address=%p) failed";
static const char short_spc88[] = "**mpi_allgather %p %d %D %p %d %D %C";
static const char long_spc88[]  = "MPI_Allgather(sbuf=%p, scount=%d, %D, rbuf=%p, rcount=%d, %D, %C) failed";
static const char short_spc89[] = "**mpi_allgatherv %p %d %D %p %p %p %D %C";
static const char long_spc89[]  = "MPI_Allgatherv(sbuf=%p, scount=%d, %D, rbuf=%p, rcounts=%p, displs=%p, %D, %C) failed";
static const char short_spc90[] = "**mpi_alloc_mem %d %I %p";
static const char long_spc90[]  = "MPI_Alloc_mem(size=%d, %I, baseptr=%p) failed";
static const char short_spc91[] = "**mpi_allreduce %p %p %d %D %O %C";
static const char long_spc91[]  = "MPI_Allreduce(sbuf=%p, rbuf=%p, count=%d, %D, %O, %C) failed";
static const char short_spc92[] = "**mpi_alltoall %p %d %D %p %d %D %C";
static const char long_spc92[]  = "MPI_Alltoall(sbuf=%p, scount=%d, %D, rbuf=%p, rcount=%d, %D, %C) failed";
static const char short_spc93[] = "**mpi_alltoallv %p %p %p %D %p %p %p %D %C";
static const char long_spc93[]  = "MPI_Alltoallv(sbuf=%p, scnts=%p, sdispls=%p, %D, rbuf=%p, rcnts=%p, rdispls=%p, %D, %C) failed";
static const char short_spc94[] = "**mpi_alltoallw %p %p %p %p %p %p %p %p %C";
static const char long_spc94[]  = "MPI_Alltoallw(sbuf=%p, scnts=%p, sdispls=%p, stypes=%p, rbuf=%p, rcnts=%p, rdispls=%p, rtypes=%p, %C) failed";
static const char short_spc95[] = "**mpi_attr_delete %C %d";
static const char long_spc95[]  = "MPI_Attr_delete(%C, keyval=%d) failed";
static const char short_spc96[] = "**mpi_attr_get %C %d %p %p";
static const char long_spc96[]  = "MPI_Attr_get(%C, keyval=%d, attr_value=%p, flag=%p) failed";
static const char short_spc97[] = "**mpi_attr_put %C %d %p";
static const char long_spc97[]  = "MPI_Attr_put(%C, keyval=%d, attr_value=%p) failed";
static const char short_spc98[] = "**mpi_barrier %C";
static const char long_spc98[]  = "MPI_Barrier(%C) failed";
static const char short_spc99[] = "**mpi_bcast %p %d %D %d %C";
static const char long_spc99[]  = "MPI_Bcast(buf=%p, count=%d, %D, root=%d, %C) failed";
static const char short_spc100[] = "**mpi_bsend %p %d %D %d %d %C";
static const char long_spc100[]  = "MPI_Bsend(buf=%p, count=%d, %D, dest=%d, tag=%d, %C) failed";
static const char short_spc101[] = "**mpi_bsend_init %p %d %D %d %d %C %p";
static const char long_spc101[]  = "MPI_Bsend_init(buf=%p, count=%d, %D, dest=%d, tag=%d, %C, request=%p) failed";
static const char short_spc102[] = "**mpi_buffer_attach %p %d";
static const char long_spc102[]  = "MPI_Buffer_attach(buf=%p, size=%d) failed";
static const char short_spc103[] = "**mpi_buffer_detach %p %p";
static const char long_spc103[]  = "MPI_Buffer_detach(buf=%p, size=%p) failed";
static const char short_spc104[] = "**mpi_cancel %p";
static const char long_spc104[]  = "MPI_Cancel(request=%p) failed";
static const char short_spc105[] = "**mpi_cart_coords %C %d %d %p";
static const char long_spc105[]  = "MPI_Cart_coords(%C, rank=%d, maxdims=%d, coords=%p) failed";
static const char short_spc106[] = "**mpi_cart_create %C %d %p %p %d %p";
static const char long_spc106[]  = "MPI_Cart_create(%C, ndims=%d, dims=%p, periods=%p, reorder=%d, comm_cart=%p) failed";
static const char short_spc107[] = "**mpi_cart_get %C %d %p %p %p";
static const char long_spc107[]  = "MPI_Cart_get(%C, maxdims=%d, dims=%p, periods=%p, coords=%p) failed";
static const char short_spc108[] = "**mpi_cart_map %C %d %p %p %p";
static const char long_spc108[]  = "MPI_Cart_map(%C, ndims=%d, dims=%p, periods=%p, newrank=%p) failed";
static const char short_spc109[] = "**mpi_cart_rank %C %p %p";
static const char long_spc109[]  = "MPI_Cart_rank(%C, coords=%p, rank=%p) failed";
static const char short_spc110[] = "**mpi_cart_shift %C %d %d %p %p";
static const char long_spc110[]  = "MPI_Cart_shift(%C, direction=%d, displ=%d, source=%p, dest=%p) failed";
static const char short_spc111[] = "**mpi_cart_sub %C %p %p";
static const char long_spc111[]  = "MPI_Cart_sub(%C, remain_dims=%p, comm_new=%p) failed";
static const char short_spc112[] = "**mpi_cartdim_get %C %p";
static const char long_spc112[]  = "MPI_Cartdim_get(%C, ndims=%p) failed";
static const char short_spc113[] = "**mpi_close_port %s";
static const char long_spc113[]  = "MPI_Close_port(port=\"%s\") failed";
static const char short_spc114[] = "**mpi_comm_accept %s %I %d %C %p";
static const char long_spc114[]  = "MPI_Comm_accept(port=\"%s\", %I, root=%d, %C, newcomm=%p) failed";
static const char short_spc115[] = "**mpi_comm_call_errhandler %C %d";
static const char long_spc115[]  = "MPI_Comm_call_errhandler(%C, errorcode=%d) failed";
static const char short_spc116[] = "**mpi_comm_compare %C %C %p";
static const char long_spc116[]  = "MPI_Comm_compare(%C, %C, result=%p) failed";
static const char short_spc117[] = "**mpi_comm_connect %s %I %d %C %p";
static const char long_spc117[]  = "MPI_Comm_connect(port=\"%s\", %I, root=%d, %C, newcomm=%p) failed";
static const char short_spc118[] = "**mpi_comm_create %C %G %p";
static const char long_spc118[]  = "MPI_Comm_create(%C, %G, new_comm=%p) failed";
static const char short_spc119[] = "**mpi_comm_create_errhandler %p %p";
static const char long_spc119[]  = "MPI_Comm_create_errhandler(function=%p, errhandler=%p) failed";
static const char short_spc120[] = "**mpi_comm_create_keyval %p %p %p %p";
static const char long_spc120[]  = "MPI_Comm_create_keyval(comm_copy_attr_fn=%p, comm_delete_attr_fn=%p, comm_keyval=%p, extra_state=%p) failed";
static const char short_spc121[] = "**mpi_comm_delete_attr %C %d";
static const char long_spc121[]  = "MPI_Comm_delete_attr(%C, comm_keyval=%d) failed";
static const char short_spc122[] = "**mpi_comm_disconnect %p";
static const char long_spc122[]  = "MPI_Comm_disconnect(comm=%p) failed";
static const char short_spc123[] = "**mpi_comm_dup %C %p";
static const char long_spc123[]  = "MPI_Comm_dup(%C, new_comm=%p) failed";
static const char short_spc124[] = "**mpi_comm_free %p";
static const char long_spc124[]  = "MPI_Comm_free(comm=%p) failed";
static const char short_spc125[] = "**mpi_comm_free_keyval %p";
static const char long_spc125[]  = "MPI_Comm_free_keyval(comm_keyval=%p) failed";
static const char short_spc126[] = "**mpi_comm_get_attr %C %d %p %p";
static const char long_spc126[]  = "MPI_Comm_get_attr(%C, comm_keyval=%d, attribute_val=%p, flag=%p) failed";
static const char short_spc127[] = "**mpi_comm_get_errhandler %C %p";
static const char long_spc127[]  = "MPI_Comm_get_errhandler(%C, errhandler=%p) failed";
static const char short_spc128[] = "**mpi_comm_get_name %C %p %p";
static const char long_spc128[]  = "MPI_Comm_get_name(%C, comm_name=%p, resultlen=%p) failed";
static const char short_spc129[] = "**mpi_comm_get_parent %p";
static const char long_spc129[]  = "MPI_Comm_get_parent(comm=%p) failed";
static const char short_spc130[] = "**mpi_comm_group %C %p";
static const char long_spc130[]  = "MPI_Comm_group(%C, group=%p) failed";
static const char short_spc131[] = "**mpi_comm_join %d %p";
static const char long_spc131[]  = "MPI_Comm_join(fd=%d, intercomm=%p) failed";
static const char short_spc132[] = "**mpi_comm_rank %C %p";
static const char long_spc132[]  = "MPI_Comm_rank(%C, rank=%p) failed";
static const char short_spc133[] = "**mpi_comm_remote_group %C %p";
static const char long_spc133[]  = "MPI_Comm_remote_group(%C, group=%p) failed";
static const char short_spc134[] = "**mpi_comm_remote_size %C %p";
static const char long_spc134[]  = "MPI_Comm_remote_size(%C, size=%p) failed";
static const char short_spc135[] = "**mpi_comm_set_attr %C %d %p";
static const char long_spc135[]  = "MPI_Comm_set_attr(%C, comm_keyval=%d, attribute_val=%p) failed";
static const char short_spc136[] = "**mpi_comm_set_errhandler %C %E";
static const char long_spc136[]  = "MPI_Comm_set_errhandler(%C, %E) failed";
static const char short_spc137[] = "**mpi_comm_set_name %C %s";
static const char long_spc137[]  = "MPI_Comm_set_name(%C, comm_name=%s) failed";
static const char short_spc138[] = "**mpi_comm_size %C %p";
static const char long_spc138[]  = "MPI_Comm_size(%C, size=%p) failed";
static const char short_spc139[] = "**mpi_comm_spawn %s %p %d %I %d %C %p %p";
static const char long_spc139[]  = "MPI_Comm_spawn(cmd=\"%s\", argv=%p, maxprocs=%d, %I, root=%d, %C, intercomm=%p, errors=%p) failed";
static const char short_spc140[] = "**mpi_comm_spawn_multiple %d %p %p %p %p %d %C %p %p";
static const char long_spc140[]  = "MPI_Comm_spawn_multiple(count=%d, cmds=%p, argvs=%p, maxprocs=%p, infos=%p, root=%d, %C, intercomm=%p, errors=%p) failed";
static const char short_spc141[] = "**mpi_comm_split %C %d %d %p";
static const char long_spc141[]  = "MPI_Comm_split(%C, color=%d, key=%d, new_comm=%p) failed";
static const char short_spc142[] = "**mpi_comm_test_inter %C %p";
static const char long_spc142[]  = "MPI_Comm_test_inter(%C, flag=%p) failed";
static const char short_spc143[] = "**mpi_dims_create %d %d %p";
static const char long_spc143[]  = "MPI_Dims_create(nnodes=%d, ndims=%d, dims=%p) failed";
static const char short_spc144[] = "**mpi_errhandler_create %p %p";
static const char long_spc144[]  = "MPI_Errhandler_create(function=%p, errhandler=%p) failed";
static const char short_spc145[] = "**mpi_errhandler_free %p";
static const char long_spc145[]  = "MPI_Errhandler_free(errhandler=%p) failed";
static const char short_spc146[] = "**mpi_errhandler_get %C %p";
static const char long_spc146[]  = "MPI_Errhandler_get(%C, errhandler=%p) failed";
static const char short_spc147[] = "**mpi_errhandler_set %C %E";
static const char long_spc147[]  = "MPI_Errhandler_set(%C, %E) failed";
static const char short_spc148[] = "**mpi_error_class %d %p";
static const char long_spc148[]  = "MPI_Error_class(errorcode=%d, errorclass=%p) failed";
static const char short_spc149[] = "**mpi_error_string %d %s %p";
static const char long_spc149[]  = "MPI_Error_string(errorcode=%d, string=%s, resultlen=%p) failed";
static const char short_spc150[] = "**mpi_exscan %p %p %d %D %O %C";
static const char long_spc150[]  = "MPI_Exscan(sbuf=%p, rbuf=%p, count=%d, %D, %O, %C) failed";
static const char short_spc151[] = "**mpi_file_call_errhandler %F %d";
static const char long_spc151[]  = "MPI_File_call_errhandler(%F, errorcode=%d) failed";
static const char short_spc152[] = "**mpi_file_create_errhandler %p %p";
static const char long_spc152[]  = "MPI_File_create_errhandler(function=%p, errhandler=%p) failed";
static const char short_spc153[] = "**mpi_file_get_errhandler %F %p";
static const char long_spc153[]  = "MPI_File_get_errhandler(%F, errhandler=%p) failed";
static const char short_spc154[] = "**mpi_file_set_errhandler %F %E";
static const char long_spc154[]  = "MPI_File_set_errhandler(%F, %E) failed";
static const char short_spc155[] = "**mpi_finalized %p";
static const char long_spc155[]  = "MPI_Finalized(flag=%p) failed";
static const char short_spc156[] = "**mpi_free_mem %p";
static const char long_spc156[]  = "MPI_Free_mem(base=%p) failed";
static const char short_spc157[] = "**mpi_gather %p %d %D %p %d %D %d %C";
static const char long_spc157[]  = "MPI_Gather(sbuf=%p, scount=%d, %D, rbuf=%p, rcount=%d, %D, root=%d, %C) failed";
static const char short_spc158[] = "**mpi_gatherv %p %d %D %p %p %p %D %d %C";
static const char long_spc158[]  = "MPI_Gatherv failed(sbuf=%p, scount=%d, %D, rbuf=%p, rcnts=%p, displs=%p, %D, root=%d, %C) failed";
static const char short_spc159[] = "**mpi_get %p %d %D %d %d %d %D %W";
static const char long_spc159[]  = "MPI_Get(origin_addr=%p, origin_count=%d, %D, target_rank=%d, target_disp=%d, target_count=%d, %D, %W) failed";
static const char short_spc160[] = "**mpi_get_address %p %p";
static const char long_spc160[]  = "MPI_Get_address(location=%p, address=%p) failed";
static const char short_spc161[] = "**mpi_get_count %p %D %p";
static const char long_spc161[]  = "MPI_Get_count(status=%p, %D, count=%p) failed";
static const char short_spc162[] = "**mpi_get_elements %p %D %p";
static const char long_spc162[]  = "MPI_Get_elements(status=%p, %D, elements=%p) failed";
static const char short_spc163[] = "**mpi_get_processor_name %p %p";
static const char long_spc163[]  = "MPI_Get_processor_name(name=%p, resultlen=%p) failed";
static const char short_spc164[] = "**mpi_get_version %p %p";
static const char long_spc164[]  = "MPI_Get_version(version=%p, subversion=%p) failed";
static const char short_spc165[] = "**mpi_graph_create %C %d %p %p %d %p";
static const char long_spc165[]  = "MPI_Graph_create(%C, nnodes=%d, index=%p, edges=%p, reorder=%d, comm_graph=%p) failed";
static const char short_spc166[] = "**mpi_graph_get %C %d %d %p %p";
static const char long_spc166[]  = "MPI_Graph_get(%C, maxindex=%d, maxedges=%d, index=%p, edges=%p) failed";
static const char short_spc167[] = "**mpi_graph_map %C %d %p %p %p";
static const char long_spc167[]  = "MPI_Graph_map(%C, nnodes=%d, index=%p, edges=%p, newrank=%p) failed";
static const char short_spc168[] = "**mpi_graph_neighbors %C %d %d %p";
static const char long_spc168[]  = "MPI_Graph_neighbors(%C, rank=%d, maxneighbors=%d, neighbors=%p) failed";
static const char short_spc169[] = "**mpi_graph_neighbors_count %C %d %p";
static const char long_spc169[]  = "MPI_Graph_neighbors_count(%C, rank=%d, nneighbors=%p) failed";
static const char short_spc170[] = "**mpi_graphdims_get %C %p %p";
static const char long_spc170[]  = "MPI_Graphdims_get(%C, nnodes=%p, nedges=%p) failed";
static const char short_spc171[] = "**mpi_grequest_complete %R";
static const char long_spc171[]  = "MPI_Grequest_complete(%R) failed";
static const char short_spc172[] = "**mpi_grequest_start %p %p %p %p %p";
static const char long_spc172[]  = "MPI_Grequest_start(query_fn=%p, free_fn=%p, cancel_fn=%p, extra_state=%p, request=%p) failed";
static const char short_spc173[] = "**mpi_group_compare %G %G %p";
static const char long_spc173[]  = "MPI_Group_compare(%G, %G, result=%p) failed";
static const char short_spc174[] = "**mpi_group_difference %G %G %p";
static const char long_spc174[]  = "MPI_Group_difference(%G, %G, new_group=%p) failed";
static const char short_spc175[] = "**mpi_group_excl %G %d %p %p";
static const char long_spc175[]  = "MPI_Group_excl(%G, n=%d, ranks=%p, new_group=%p) failed";
static const char short_spc176[] = "**mpi_group_free %p";
static const char long_spc176[]  = "MPI_Group_free(group=%p) failed";
static const char short_spc177[] = "**mpi_group_incl %G %d %p %p";
static const char long_spc177[]  = "MPI_Group_incl(%G, n=%d, ranks=%p, new_group=%p) failed";
static const char short_spc178[] = "**mpi_group_intersection %G %G %p";
static const char long_spc178[]  = "MPI_Group_intersection(%G, %G, new_group=%p) failed";
static const char short_spc179[] = "**mpi_group_range_excl %G %d %p %p";
static const char long_spc179[]  = "MPI_Group_range_excl(%G, n=%d, ranges=%p, new_group=%p) failed";
static const char short_spc180[] = "**mpi_group_range_incl %G %d %p %p";
static const char long_spc180[]  = "MPI_Group_range_incl(%G, n=%d, ranges=%p, new_group=%p) failed";
static const char short_spc181[] = "**mpi_group_rank %G %p";
static const char long_spc181[]  = "MPI_Group_rank(%G, rank=%p) failed";
static const char short_spc182[] = "**mpi_group_size %G %p";
static const char long_spc182[]  = "MPI_Group_size(%G, size=%p) failed";
static const char short_spc183[] = "**mpi_group_translate_ranks %G %d %p %G %p";
static const char long_spc183[]  = "MPI_Group_translate_ranks(%G, n=%d, ranks1=%p, %G, ranks2=%p) failed";
static const char short_spc184[] = "**mpi_group_union %G %G %p";
static const char long_spc184[]  = "MPI_Group_union(%G, %G, new_group=%p) failed";
static const char short_spc185[] = "**mpi_ibsend %p %d %D %d %d %C %p";
static const char long_spc185[]  = "MPI_Ibsend(buf=%p, count=%d, %D, dest=%d, tag=%d, %C, request=%p) failed";
static const char short_spc186[] = "**mpi_info_create %p";
static const char long_spc186[]  = "MPI_Info_create(info=%p) failed";
static const char short_spc187[] = "**mpi_info_delete %I %s";
static const char long_spc187[]  = "MPI_Info_delete(%I, key=%s) failed";
static const char short_spc188[] = "**mpi_info_dup %I %p";
static const char long_spc188[]  = "MPI_Info_dup(%I, newinfo=%p) failed";
static const char short_spc189[] = "**mpi_info_free %p";
static const char long_spc189[]  = "MPI_Info_free(info=%p) failed";
static const char short_spc190[] = "**mpi_info_get %I %s %d %p %p";
static const char long_spc190[]  = "MPI_Info_get(%I, key=%s, valuelen=%d, value=%p, flag=%p) failed";
static const char short_spc191[] = "**mpi_info_get_nkeys %I %p";
static const char long_spc191[]  = "MPI_Info_get_nkeys(%I, nkeys=%p) failed";
static const char short_spc192[] = "**mpi_info_get_nthkey %I %d %p";
static const char long_spc192[]  = "MPI_Info_get_nthkey(%I, n=%d, key=%p) failed";
static const char short_spc193[] = "**mpi_info_get_valuelen %I %s %p %p";
static const char long_spc193[]  = "MPI_Info_get_valuelen(%I, key=%s, valuelen=%p, flag=%p) failed";
static const char short_spc194[] = "**mpi_info_set %I %s %s";
static const char long_spc194[]  = "MPI_Info_set(%I, key=%s, value=%s) failed";
static const char short_spc195[] = "**mpi_init %p %p";
static const char long_spc195[]  = "MPI_Init(argc_p=%p, argv_p=%p) failed";
static const char short_spc196[] = "**mpi_init_thread %p %p %d %p";
static const char long_spc196[]  = "MPI_Init_thread(argc_p=%p, argv_p=%p, required=%d, provided=%p)";
static const char short_spc197[] = "**mpi_initialized %p";
static const char long_spc197[]  = "MPI_Initialized(flag=%p) failed";
static const char short_spc198[] = "**mpi_intercomm_create %C %d %C %d %d %p";
static const char long_spc198[]  = "MPI_Intercomm_create(%C, local_leader=%d, %C, remote_leader=%d, tag=%d, newintercomm=%p) failed";
static const char short_spc199[] = "**mpi_intercomm_merge %C %d %p";
static const char long_spc199[]  = "MPI_Intercomm_merge(%C, high=%d, newintracomm=%p) failed";
static const char short_spc200[] = "**mpi_iprobe %d %d %C %p %p";
static const char long_spc200[]  = "MPI_Iprobe(src=%d, tag=%d, %C, flag=%p, status=%p) failed";
static const char short_spc201[] = "**mpi_irecv %p %d %D %d %d %C %p";
static const char long_spc201[]  = "MPI_Irecv(buf=%p, count=%d, %D, src=%d, tag=%d, %C, request=%p) failed";
static const char short_spc202[] = "**mpi_irsend %p %d %D %d %d %C %p";
static const char long_spc202[]  = "MPI_Irsend(buf=%p, count=%d, %D, dest=%d, tag=%d, %C, request=%p) failed";
static const char short_spc203[] = "**mpi_is_thread_main %p";
static const char long_spc203[]  = "MPI_Is_thread_main(flag=%p) failed";
static const char short_spc204[] = "**mpi_isend %p %d %D %d %d %C %p";
static const char long_spc204[]  = "MPI_Isend(buf=%p, count=%d, %D, dest=%d, tag=%d, %C, request=%p) failed";
static const char short_spc205[] = "**mpi_issend %p %d %D %d %d %C %p";
static const char long_spc205[]  = "MPI_Issend(buf=%p, count=%d, %D, dest=%d, tag=%d, %C, request=%p) failed";
static const char short_spc206[] = "**mpi_keyval_create %p %p %p %p";
static const char long_spc206[]  = "MPI_Keyval_create(copy_fn=%p, del_fn=%p, keyval=%p, extra=%p) failed";
static const char short_spc207[] = "**mpi_keyval_free %p";
static const char long_spc207[]  = "MPI_Keyval_free(keyval=%p) failed";
static const char short_spc208[] = "**mpi_lookup_name %s %I %p";
static const char long_spc208[]  = "MPI_Lookup_name(service=\"%s\", %I, port=%p) failed";
static const char short_spc209[] = "**mpi_op_create %p %d %p";
static const char long_spc209[]  = "MPI_Op_create(fn=%p, commute=%d, op=%p) failed";
static const char short_spc210[] = "**mpi_op_free %p";
static const char long_spc210[]  = "MPI_Op_free(op=%p) failed";
static const char short_spc211[] = "**mpi_open_port %I %p";
static const char long_spc211[]  = "MPI_Open_port(%I, port=%p) failed";
static const char short_spc212[] = "**mpi_pack %p %d %D %p %d %p %C";
static const char long_spc212[]  = "MPI_Pack(inbuf=%p, incount=%d, %D, outbuf=%p, outcount=%d, position=%p, %C) failed";
static const char short_spc213[] = "**mpi_pack_external %s %p %d %D %p %d %p";
static const char long_spc213[]  = "MPI_Pack_external(datarep=%s, inbuf=%p, incount=%d, %D, outbuf=%p, outcount=%d, position=%p) failed";
static const char short_spc214[] = "**mpi_pack_external_size %s %d %D %p";
static const char long_spc214[]  = "MPI_Pack_external_size(datarep=%s, incount=%d, %D, size=%p) failed";
static const char short_spc215[] = "**mpi_pack_size %d %D %C %p";
static const char long_spc215[]  = "MPI_Pack_size(count=%d, %D, %C, size=%p) failed";
static const char short_spc216[] = "**mpi_pcontrol %d";
static const char long_spc216[]  = "MPI_Pcontrol(level=%d) failed";
static const char short_spc217[] = "**mpi_probe %d %d %C %p";
static const char long_spc217[]  = "MPI_Probe(src=%d, tag=%d, %C, status=%p) failed";
static const char short_spc218[] = "**mpi_publish_name %s %I %s";
static const char long_spc218[]  = "MPI_Publish_name(service=\"%s\", %I, port=\"%s\") failed";
static const char short_spc219[] = "**mpi_put %p %d %D %d %d %d %D %W";
static const char long_spc219[]  = "MPI_Put(origin_addr=%p, origin_count=%d, %D, target_rank=%d, target_disp=%d, target_count=%d, %D, %W) failed";
static const char short_spc220[] = "**mpi_query_thread %p";
static const char long_spc220[]  = "MPI_Query_thread(provided=%p) failed";
static const char short_spc221[] = "**mpi_recv %p %d %D %d %d %C %p";
static const char long_spc221[]  = "MPI_Recv(buf=%p, count=%d, %D, src=%d, %C, status=%p) failed";
static const char short_spc222[] = "**mpi_recv_init %p %d %D %d %d %C %p";
static const char long_spc222[]  = "MPI_Recv_init(buf=%p, count=%d, %D, src=%d, tag=%d, %C, request=%p) failed";
static const char short_spc223[] = "**mpi_reduce %p %p %d %D %O %d %C";
static const char long_spc223[]  = "MPI_Reduce(sbuf=%p, rbuf=%p, count=%d, %D, %O, root=%d, %C) failed";
static const char short_spc224[] = "**mpi_reduce_scatter %p %p %p %D %O %C";
static const char long_spc224[]  = "MPI_Reduce_scatter(sbuf=%p, rbuf=%p, rcnts=%p, %D, %O, %C) failed";
static const char short_spc225[] = "**mpi_register_datarep %s %p %p %p %p";
static const char long_spc225[]  = "MPI_Register_datarep(datarep=%s, read_conversion_fn=%p, write_conversion_fn=%p, dtype_file_extent_fn=%p, extra_state=%p) failed";
static const char short_spc226[] = "**mpi_request_free %p";
static const char long_spc226[]  = "MPI_Request_free(request=%p) failed";
static const char short_spc227[] = "**mpi_request_get_status %R %p %p";
static const char long_spc227[]  = "MPI_Request_get_status(%R, flag=%p, status=%p) failed";
static const char short_spc228[] = "**mpi_rsend %p %d %D %d %d %C";
static const char long_spc228[]  = "MPI_Rsend(buf=%p, count=%d, %D, src=%d, tag=%d, %C) failed";
static const char short_spc229[] = "**mpi_rsend_init %p %d %D %d %d %C %p";
static const char long_spc229[]  = "MPI_Rsend_init(buf=%p, count=%d, %D, dest=%d, tag=%d, %C, request=%p) failed";
static const char short_spc230[] = "**mpi_scan %p %p %d %D %O %C";
static const char long_spc230[]  = "MPI_Scan(sbuf=%p, rbuf=%p, count=%d, %D, %O, %C) failed";
static const char short_spc231[] = "**mpi_scatter %p %d %D %p %d %D %d %C";
static const char long_spc231[]  = "MPI_Scatter(sbuf=%p, scount=%d, %D, rbuf=%p, rcount=%d, %D, root=%d, %C) failed";
static const char short_spc232[] = "**mpi_scatterv %p %p %p %D %p %d %D %d %C";
static const char long_spc232[]  = "MPI_Scatterv(sbuf=%p, scnts=%p, displs=%p, %D, rbuf=%p, rcount=%d, %D, root=%d, %C) failed";
static const char short_spc233[] = "**mpi_send %p %d %D %d %d %C";
static const char long_spc233[]  = "MPI_Send(buf=%p, count=%d, %D, dest=%d, tag=%d, %C) failed";
static const char short_spc234[] = "**mpi_send_init %p %d %D %d %d %C %p";
static const char long_spc234[]  = "MPI_Send_init(buf=%p, count=%d, %D, dest=%d, tag=%d, %C, request=%p) failed";
static const char short_spc235[] = "**mpi_sendrecv %p %d %D %d %d %p %d %D %d %d %C %p";
static const char long_spc235[]  = "MPI_Sendrecv(sbuf=%p, scount=%d, %D, dest=%d, stag=%d, rbuf=%p, rcount=%d, %D, src=%d, rtag=%d, %C, status=%p) failed";
static const char short_spc236[] = "**mpi_sendrecv_replace %p %d %D %d %d %d %d %C %p";
static const char long_spc236[]  = "MPI_Sendrecv_replace(buf=%p, count=%d, %D, dest=%d, stag=%d, src=%d, rtag=%d, %C, status=%p) failed";
static const char short_spc237[] = "**mpi_ssend %p %d %D %d %d %C";
static const char long_spc237[]  = "MPI_Ssend(buf=%p, count=%d, %D, dest=%d, tag=%d, %C) failed";
static const char short_spc238[] = "**mpi_ssend_init %p %d %D %d %d %C %p";
static const char long_spc238[]  = "MPI_Ssend_init(buf=%p, count=%d, %D, dest=%d, tag=%d, %C, request=%p) failed";
static const char short_spc239[] = "**mpi_start %p";
static const char long_spc239[]  = "MPI_Start(request=%p) failed";
static const char short_spc240[] = "**mpi_startall %d %p";
static const char long_spc240[]  = "MPI_Startall(count=%d, req_array=%p) failed";
static const char short_spc241[] = "**mpi_status_set_cancelled %p %d";
static const char long_spc241[]  = "MPI_Status_set_cancelled(status=%p, flag=%d) failed";
static const char short_spc242[] = "**mpi_status_set_elements %p %D %d";
static const char long_spc242[]  = "MPI_Status_set_elements(status=%p, %D, count=%d) failed";
static const char short_spc243[] = "**mpi_test %p %p %p";
static const char long_spc243[]  = "MPI_Test(request=%p, flag=%p, status=%p) failed";
static const char short_spc244[] = "**mpi_test_cancelled %p %p";
static const char long_spc244[]  = "MPI_Test_cancelled(status=%p, flag=%p) failed";
static const char short_spc245[] = "**mpi_testall %d %p %p %p";
static const char long_spc245[]  = "MPI_Testall(count=%d, req_array=%p, flag=%p, status_array=%p) failed";
static const char short_spc246[] = "**mpi_testany %d %p %p %p %p";
static const char long_spc246[]  = "MPI_Testany(count=%d, req_array=%p, index=%p, flag=%p, status=%p) failed";
static const char short_spc247[] = "**mpi_testsome %d %p %p %p %p";
static const char long_spc247[]  = "MPI_Testsome(count=%d, req_array=%p, out_count=%p, indices=%p, status_array=%p) failed";
static const char short_spc248[] = "**mpi_topo_test %C %p";
static const char long_spc248[]  = "MPI_Topo_test(%C, topo_type=%p) failed";
static const char short_spc249[] = "**mpi_type_commit %p";
static const char long_spc249[]  = "MPI_Type_commit(datatype_p=%p) failed";
static const char short_spc250[] = "**mpi_type_contiguous %d %D %p";
static const char long_spc250[]  = "MPI_Type_contiguous(count=%d, %D, new_type_p=%p) failed";
static const char short_spc251[] = "**mpi_type_create_darray %d %d %d %p %p %p %p %d %D %p";
static const char long_spc251[]  = "MPI_Type_create_darray(size=%d, rank=%d, ndims=%d, array_of_gsizes=%p, array_of_distribs=%p, array_of_dargs=%p, array_of_psizes=%p, order=%d, %D, newtype=%p) failed";
static const char short_spc252[] = "**mpi_type_create_hindexed %d %p %p %D %p";
static const char long_spc252[]  = "MPI_Type_create_hindexed(count=%d, array_of_blocklengths=%p, array_of_displacements=%p, %D, newtype=%p) failed";
static const char short_spc253[] = "**mpi_type_create_hvector %d %d %d %D %p";
static const char long_spc253[]  = "MPI_Type_create_hvector(count=%d, blocklength=%d, stride=%d, %D, newtype=%p) failed";
static const char short_spc254[] = "**mpi_type_create_indexed_block %d %d %p %D %p";
static const char long_spc254[]  = "MPI_Type_create_indexed_block(count=%d, blocklength=%d, array_of_displacements=%p, %D, newtype=%p) failed";
static const char short_spc255[] = "**mpi_type_create_keyval %p %p %p %p";
static const char long_spc255[]  = "MPI_Type_create_keyval(type_copy_attr_fn=%p, type_delete_attr_fn=%p, type_keyval=%p, extra_state=%p) failed";
static const char short_spc256[] = "**mpi_type_create_resized %D %d %d %p";
static const char long_spc256[]  = "MPI_Type_create_resized(%D, lb=%d, extent=%d, newtype=%p) failed";
static const char short_spc257[] = "**mpi_type_create_struct %d %p %p %p %p";
static const char long_spc257[]  = "MPI_Type_create_struct(count=%d, array_of_blocklengths=%p, array_of_displacements=%p, array_of_types=%p, newtype=%p) failed";
static const char short_spc258[] = "**mpi_type_create_subarray %d %p %p %p %d %D %p";
static const char long_spc258[]  = "MPI_Type_create_subarray(ndims=%d, array_of_sizes=%p, array_of_subsizes=%p, array_of_starts=%p, order=%d, %D, newtype=%p) failed";
static const char short_spc259[] = "**mpi_type_delete_attr %D %d";
static const char long_spc259[]  = "MPI_Type_delete_attr(%D, type_keyval=%d) failed";
static const char short_spc260[] = "**mpi_type_dup %D %p";
static const char long_spc260[]  = "MPI_Type_dup(%D, newtype=%p) failed";
static const char short_spc261[] = "**mpi_type_extent %D %p";
static const char long_spc261[]  = "MPI_Type_extent(%D, extent=%p) failed";
static const char short_spc262[] = "**mpi_type_free %p";
static const char long_spc262[]  = "MPI_Type_free(datatype_p=%p) failed";
static const char short_spc263[] = "**mpi_type_free_keyval %p";
static const char long_spc263[]  = "MPI_Type_free_keyval(type_keyval=%p) failed";
static const char short_spc264[] = "**mpi_type_get_attr %D %d %p %p";
static const char long_spc264[]  = "MPI_Type_get_attr(%D, type_keyval=%d, attribute_val=%p, flag=%p) failed";
static const char short_spc265[] = "**mpi_type_get_contents %D %d %d %d %p %p %p";
static const char long_spc265[]  = "MPI_Type_get_contents(%D, max_integers=%d, max_addresses=%d, max_datatypes=%d, array_of_integers=%p, array_of_addresses=%p, array_of_datatypes=%p) failed";
static const char short_spc266[] = "**mpi_type_get_envelope %D %p %p %p %p";
static const char long_spc266[]  = "MPI_Type_get_envelope(%D, num_integers=%p, num_addresses=%p, num_datatypes=%p, combiner=%p) failed";
static const char short_spc267[] = "**mpi_type_get_extent %D %p %p";
static const char long_spc267[]  = "MPI_Type_get_extent(%D, lb=%p, extent=%p) failed";
static const char short_spc268[] = "**mpi_type_get_name %D %p %p";
static const char long_spc268[]  = "MPI_Type_get_name(%D, type_name=%p, resultlen=%p) failed";
static const char short_spc269[] = "**mpi_type_get_true_extent %D %p %p";
static const char long_spc269[]  = "MPI_Type_get_true_extent(%D, lb=%p, true_extent=%p) failed";
static const char short_spc270[] = "**mpi_type_hindexed %d %p %p %D %p";
static const char long_spc270[]  = "MPI_Type_hindexed(count=%d, blocklens=%p, indices=%p, %D, new_type_p=%p) failed";
static const char short_spc271[] = "**mpi_type_hvector %d %d %d %D %p";
static const char long_spc271[]  = "MPI_Type_hvector(count=%d, blocklen=%d, stride=%d, %D, new_type_p=%p) failed";
static const char short_spc272[] = "**mpi_type_indexed %d %p %p %D %p";
static const char long_spc272[]  = "MPI_Type_indexed(count=%d, blocklens=%p, indices=%p, %D, new_type_p=%p) failed";
static const char short_spc273[] = "**mpi_type_lb %D %p";
static const char long_spc273[]  = "MPI_Type_lb(%D, displacement=%p) failed";
static const char short_spc274[] = "**mpi_type_match_size %d %d %p";
static const char long_spc274[]  = "MPI_Type_match_size(typeclass=%d, size=%d, datatype=%p) failed";
static const char short_spc275[] = "**mpi_type_set_attr %D %d %p";
static const char long_spc275[]  = "MPI_Type_set_attr(%D, type_keyval=%d, attribute_val=%p) failed";
static const char short_spc276[] = "**mpi_type_set_name %D %s";
static const char long_spc276[]  = "MPI_Type_set_name(%D, type_name=%s) failed";
static const char short_spc277[] = "**mpi_type_size %D %p";
static const char long_spc277[]  = "MPI_Type_size(%D) failed";
static const char short_spc278[] = "**mpi_type_struct %d %p %p %p %p";
static const char long_spc278[]  = "MPI_Type_struct(count=%d, blocklens=%p, indices=%p, old_types=%p, new_type_p=%p) failed";
static const char short_spc279[] = "**mpi_type_ub %D %p";
static const char long_spc279[]  = "MPI_Type_ub(%D, displacement=%p) failed";
static const char short_spc280[] = "**mpi_type_vector %d %d %d %D %p";
static const char long_spc280[]  = "MPI_Type_vector(count=%d, blocklength=%d, stride=%d, %D, new_type_p=%p) failed";
static const char short_spc281[] = "**mpi_unpack %p %d %p %p %d %D %C";
static const char long_spc281[]  = "MPI_Unpack(inbuf=%p, insize=%d, position=%p, outbuf=%p, outcount=%d, %D, %C) failed";
static const char short_spc282[] = "**mpi_unpack_external %s %p %d %p %p %d %D";
static const char long_spc282[]  = "MPI_Unpack_external(datarep=%s, inbuf=%p, insize=%d, position=%p, outbuf=%p, outcount=%d, %D) failed";
static const char short_spc283[] = "**mpi_unpublish_name %s %I %s";
static const char long_spc283[]  = "MPI_Unpublish_name(service=\"%s\", %I, port=\"%s\") failed";
static const char short_spc284[] = "**mpi_wait %p %p";
static const char long_spc284[]  = "MPI_Wait(request=%p, status%p) failed";
static const char short_spc285[] = "**mpi_waitall %d %p %p";
static const char long_spc285[]  = "MPI_Waitall(count=%d, req_array=%p, status_array=%p) failed";
static const char short_spc286[] = "**mpi_waitany %d %p %p %p";
static const char long_spc286[]  = "MPI_Waitany(count=%d, req_array=%p, index=%p, status=%p) failed";
static const char short_spc287[] = "**mpi_waitsome %d %p %p %p %p";
static const char long_spc287[]  = "MPI_Waitsome(count=%d, req_array=%p, out_count=%p, indices=%p, status_array=%p) failed";
static const char short_spc288[] = "**mpi_win_call_errhandler %W %d";
static const char long_spc288[]  = "MPI_Win_call_errhandler(%W, errorcode=%d) failed";
static const char short_spc289[] = "**mpi_win_complete %W";
static const char long_spc289[]  = "MPI_Win_complete(%W) failed";
static const char short_spc290[] = "**mpi_win_create %p %d %d %I %C %p";
static const char long_spc290[]  = "MPI_Win_create(base=%p, size=%d, disp_unit=%d, %I, %C, win=%p) failed";
static const char short_spc291[] = "**mpi_win_create_errhandler %p %p";
static const char long_spc291[]  = "MPI_Win_create_errhandler(function=%p, errhandler=%p) failed";
static const char short_spc292[] = "**mpi_win_create_keyval %p %p %p %p";
static const char long_spc292[]  = "MPI_Win_create_keyval(win_copy_attr_fn=%p, win_delete_attr_fn=%p, win_keyval=%p, extra_state=%p) failed";
static const char short_spc293[] = "**mpi_win_delete_attr %W %d";
static const char long_spc293[]  = "MPI_Win_delete_attr(%W, win_keyval=%d) failed";
static const char short_spc294[] = "**mpi_win_fence %A %W";
static const char long_spc294[]  = "MPI_Win_fence(%A, %W) failed";
static const char short_spc295[] = "**mpi_win_free %p";
static const char long_spc295[]  = "MPI_Win_free(win=%p) failed";
static const char short_spc296[] = "**mpi_win_free_keyval %p";
static const char long_spc296[]  = "MPI_Win_free_keyval(win_keyval=%p) failed";
static const char short_spc297[] = "**mpi_win_get_attr %W %d %p %p";
static const char long_spc297[]  = "MPI_Win_get_attr(%W, win_keyval=%d, attribute_val=%p, flag=%p) failed";
static const char short_spc298[] = "**mpi_win_get_errhandler %W %p";
static const char long_spc298[]  = "MPI_Win_get_errhandler(%W, errhandler=%p) failed";
static const char short_spc299[] = "**mpi_win_get_group %W %p";
static const char long_spc299[]  = "MPI_Win_get_group(%W, group=%p) failed";
static const char short_spc300[] = "**mpi_win_get_name %W %p %p";
static const char long_spc300[]  = "MPI_Win_get_name(%W, win_name=%p, resultlen=%p) failed";
static const char short_spc301[] = "**mpi_win_lock %d %d %A %W";
static const char long_spc301[]  = "MPI_Win_lock(lock_type=%d, rank=%d, %A, %W) failed";
static const char short_spc302[] = "**mpi_win_post %G %A %W";
static const char long_spc302[]  = "MPI_Win_post(%G, %A, %W) failed";
static const char short_spc303[] = "**mpi_win_set_attr %W %d %p";
static const char long_spc303[]  = "MPI_Win_set_attr(%W, win_keyval=%d, attribute_val=%p) failed";
static const char short_spc304[] = "**mpi_win_set_errhandler %W %E";
static const char long_spc304[]  = "MPI_Win_set_errhandler(%W, %E) failed";
static const char short_spc305[] = "**mpi_win_set_name %W %s";
static const char long_spc305[]  = "MPI_Win_set_name(%W, win_name=%s) failed";
static const char short_spc306[] = "**mpi_win_start %G %A %W";
static const char long_spc306[]  = "MPI_Win_start(%G, %A, %W) failed";
static const char short_spc307[] = "**mpi_win_test %W %p";
static const char long_spc307[]  = "MPI_Win_test(%W, flag=%p) failed";
static const char short_spc308[] = "**mpi_win_unlock %d %W";
static const char long_spc308[]  = "MPI_Win_unlock(rank=%d, %W) failed";
static const char short_spc309[] = "**mpi_win_wait %W";
static const char long_spc309[]  = "MPI_Win_wait(%W) failed";
static const char short_spc310[] = "**mq_close %d";
static const char long_spc310[]  = "failed to close a posix message queue, error %d";
static const char short_spc311[] = "**mq_open %d";
static const char long_spc311[]  = "failed to open a posix message queue, error %d";
static const char short_spc312[] = "**mq_receive %d";
static const char long_spc312[]  = "failed to receive a posix message queue message, error %d";
static const char short_spc313[] = "**mq_send %d";
static const char long_spc313[]  = "failed to send a posix message queue message, error %d";
static const char short_spc314[] = "**msgctl %d";
static const char long_spc314[]  = "msgctl returned %d";
static const char short_spc315[] = "**msgget %d";
static const char long_spc315[]  = "msgget returned %d";
static const char short_spc316[] = "**msgrcv %d";
static const char long_spc316[]  = "msgrcv returned %d";
static const char short_spc317[] = "**msgsnd %d";
static const char long_spc317[]  = "msgsnd returned %d";
static const char short_spc318[] = "**namepubfile %s %s %s";
static const char long_spc318[]  = "Unable to publish service name %s using file %s for reason %s";
static const char short_spc319[] = "**namepubnotpub %s";
static const char long_spc319[]  = "Lookup failed for service name %s";
static const char short_spc320[] = "**nomem %d";
static const char long_spc320[]  = "Out of memory (unable to allocate %d bytes)";
static const char short_spc321[] = "**nomem %s";
static const char long_spc321[]  = "Out of memory (unable to allocate a '%s')";
static const char short_spc322[] = "**nomem %s %d";
static const char long_spc322[]  = "Out of memory (unable to allocate a '%s' of size %d)";
static const char short_spc323[] = "**notsame %s %s";
static const char long_spc323[]  = "Inconsistent arguments %s to collective routine %s";
static const char short_spc324[] = "**nulledge %d %d";
static const char long_spc324[]  = "Edge for node %d (entry edges[%d]) is to itself";
static const char short_spc325[] = "**nullptr %s";
static const char long_spc325[]  = "Null pointer in parameter %s";
static const char short_spc326[] = "**nullptrtype %s";
static const char long_spc326[]  = "Null %s pointer";
static const char short_spc327[] = "**open %s %d %d";
static const char long_spc327[]  = "open(%s) failed for process %d, error %d";
static const char short_spc328[] = "**opnotpredefined %d";
static const char long_spc328[]  = "only predefined ops are valid (op = %d)";
static const char short_spc329[] = "**opundefined %s";
static const char long_spc329[]  = "MPI_Op %s operation not defined for this datatype ";
static const char short_spc330[] = "**opundefined_rma %d";
static const char long_spc330[]  = "RMA target received unknown RMA operation type %d";
static const char short_spc331[] = "**pd_alloc %s";
static const char long_spc331[]  = "unable to allocate a protection domain - %s";
static const char short_spc332[] = "**pglookup %s";
static const char long_spc332[]  = "unable to find the process group structure with id <%s>";
static const char short_spc333[] = "**pkt_ptr %p %p";
static const char long_spc333[]  = "invalid shm queue packet pointer (%p != %p)";
static const char short_spc334[] = "**pmi_barrier %d";
static const char long_spc334[]  = "PMI_Barrier returned %d";
static const char short_spc335[] = "**pmi_finalize %d";
static const char long_spc335[]  = "PMI_Finalize returned %d";
static const char short_spc336[] = "**pmi_get_id %d";
static const char long_spc336[]  = "PMI_Get_id returned %d";
static const char short_spc337[] = "**pmi_get_id_length_max %d";
static const char long_spc337[]  = "PMI_Get_id_length_max returned %d";
static const char short_spc338[] = "**pmi_get_rank %d";
static const char long_spc338[]  = "PMI_Get_rank returned %d";
static const char short_spc339[] = "**pmi_get_size %d";
static const char long_spc339[]  = "PMI_Get_size returned %d";
static const char short_spc340[] = "**pmi_init %d";
static const char long_spc340[]  = "PMI_Init returned %d";
static const char short_spc341[] = "**pmi_kvs_commit %d";
static const char long_spc341[]  = "PMI_KVS_Commit returned %d";
static const char short_spc342[] = "**pmi_kvs_create %d";
static const char long_spc342[]  = "PMI_KVS_Create returned %d";
static const char short_spc343[] = "**pmi_kvs_get %d";
static const char long_spc343[]  = "PMI_KVS_Get returned %d";
static const char short_spc344[] = "**pmi_kvs_get_key_length_max %d";
static const char long_spc344[]  = "PMI_KVS_Get_key_length_max returned %d";
static const char short_spc345[] = "**pmi_kvs_get_my_name %d";
static const char long_spc345[]  = "PMI_KVS_Get_my_name returned %d";
static const char short_spc346[] = "**pmi_kvs_get_name_length_max %d";
static const char long_spc346[]  = "PMI_KVS_Get_name_length_max returned %d";
static const char short_spc347[] = "**pmi_kvs_get_parent %d";
static const char long_spc347[]  = "unable to get the PARENT_ROOT_PORT_NAME from the keyval space (pmi_error %d)";
static const char short_spc348[] = "**pmi_kvs_get_value_length_max %d";
static const char long_spc348[]  = "PMI_KVS_Get_value_length_max returned %d";
static const char short_spc349[] = "**pmi_kvs_put %d";
static const char long_spc349[]  = "PMI_KVS_Put returned %d";
static const char short_spc350[] = "**pmi_spawn_multiple %d";
static const char long_spc350[]  = "PMI_Spawn_multiple returned %d";
static const char short_spc351[] = "**post_connect %s";
static const char long_spc351[]  = "%s failed in VC_post_connect";
static const char short_spc352[] = "**rangedup %d %d %d";
static const char long_spc352[]  = "The range array specifies duplicate entries; process %d specified in range array %d was previously specified in range array %d";
static const char short_spc353[] = "**rangeendinvalid %d %d %d";
static const char long_spc353[]  = "The %dth element of a range array ends at %d but must be nonnegative and less than %d";
static const char short_spc354[] = "**rangestartinvalid %d %d %d";
static const char long_spc354[]  = "The %dth element of a range array starts at %d but must be nonnegative and less than %d";
static const char short_spc355[] = "**rank %d %d";
static const char long_spc355[]  = "Invalid rank has value %d but must be nonnegative and less than %d";
static const char short_spc356[] = "**rankarray %d %d %d";
static const char long_spc356[]  = "Invalid rank in rank array at index %d; value is %d but must be in the range 0 to %d";
static const char short_spc357[] = "**rankdup %d %d %d";
static const char long_spc357[]  = "Duplicate ranks in rank array at index %d, has value %d which is also the value at index %d";
static const char short_spc358[] = "**ranklocal %d %d";
static const char long_spc358[]  = "Error specifying local_leader; rank given was %d but must be in the range 0 to %d";
static const char short_spc359[] = "**rankremote %d %d";
static const char long_spc359[]  = "Error specifying remote_leader; rank given was %d but must be in the range 0 to %d";
static const char short_spc360[] = "**rmasize %d";
static const char long_spc360[]  = "Invalid size argument in RMA call (value is %d)";
static const char short_spc361[] = "**root %d";
static const char long_spc361[]  = "Invalid root (value given was %d)";
static const char short_spc362[] = "**rsendnomatch %d %d";
static const char long_spc362[]  = "Ready send from source %d and with tag %d had no matching receive";
static const char short_spc363[] = "**shm_op %d";
static const char long_spc363[]  = "invalid shm operation (%d)";
static const char short_spc364[] = "**shm_open %s %d";
static const char long_spc364[]  = "unable to open shared memory object %s (errno %d)";
static const char short_spc365[] = "**shm_unlink %s %d";
static const char long_spc365[]  = "failed to unlink shared memory object %s, error %d";
static const char short_spc366[] = "**shmat %d";
static const char long_spc366[]  = "shmat failed, error %d";
static const char short_spc367[] = "**shmctl %d %d";
static const char long_spc367[]  = "failed to mark the sysv segment %d for removal, error %d";
static const char short_spc368[] = "**shmget %d";
static const char long_spc368[]  = "shmget failed, error %d";
static const char short_spc369[] = "**shmhost %s %s";
static const char long_spc369[]  = "process not on the same host (%s != %s)";
static const char short_spc370[] = "**shmq_index %d %d";
static const char long_spc370[]  = "invalid shm queue index (%d > %d)";
static const char short_spc371[] = "**shmsize %d";
static const char long_spc371[]  = "Shared memory subsystem unable to allocate a block of the requested size, %d bytes";
static const char short_spc372[] = "**snprintf %d";
static const char long_spc372[]  = "snprintf returned %d";
static const char short_spc373[] = "**sock_byname %s %d";
static const char long_spc373[]  = "gethostbyname failed, %s (errno %d)";
static const char short_spc374[] = "**sock_connect %s %d %s %d";
static const char long_spc374[]  = "unable to connect to %s on port %d, %s (errno %d)";
static const char short_spc375[] = "**sock_create %s %d";
static const char long_spc375[]  = "unable to create a socket, %s (errno %d)";
static const char short_spc376[] = "**sock_gethost %s %d";
static const char long_spc376[]  = "gethostname failed, %s (errno %d)";
static const char short_spc377[] = "**socket %s %d";
static const char long_spc377[]  = "WSASocket failed, %s (errno %d)";
static const char short_spc378[] = "**sock|badiovn %d %d %d";
static const char long_spc378[]  = "size of iov is invalid (set=%d,sock=%d,iov_n=%d)";
static const char short_spc379[] = "**sock|badlen %d %d %d %d";
static const char long_spc379[]  = "bad length parameter(s) (set=%d,sock=%d,min=%d,max=%d)";
static const char short_spc380[] = "**sock|badport %d";
static const char long_spc380[]  = "port number is out of range (sock=%d)";
static const char short_spc381[] = "**sock|close_cancel %d %d";
static const char long_spc381[]  = "operation cancelled because sock was closed locally (set=%d,sock=%d)";
static const char short_spc382[] = "**sock|closing %d %d";
static const char long_spc382[]  = "sock is in the process of being closed locally (set=%d,sock=%d)";
static const char short_spc383[] = "**sock|closing_already %d %d";
static const char long_spc383[]  = "a close operation is already post (set=%d,sock=%d)";
static const char short_spc384[] = "**sock|connclosed %d %d";
static const char long_spc384[]  = "connection closed by peer (set=%d,sock=%d)";
static const char short_spc385[] = "**sock|listener_bad_sock %d %d";
static const char long_spc385[]  = " supplied sock is not a listener (set=%d,sock=%d)";
static const char short_spc386[] = "**sock|listener_bad_state %d %d %d";
static const char long_spc386[]  = " supplied listener sock is in a bad state (set=%d,sock=%d,state=%d)";
static const char short_spc387[] = "**sock|listener_read %d %d";
static const char long_spc387[]  = "read operation not allowed on a listener (set=%d,sock=%d)";
static const char short_spc388[] = "**sock|listener_write %d %d";
static const char long_spc388[]  = "write operation not allowed on a listener (set=%d,sock=%d)";
static const char short_spc389[] = "**sock|notconnected %d %d";
static const char long_spc389[]  = "sock is not connected (set=%d,sock=%d)";
static const char short_spc390[] = "**sock|osnomem %s %d %d";
static const char long_spc390[]  = "operating system routine %s failed due to lack of memory (set=%d,sock=%d)";
static const char short_spc391[] = "**sock|poll|accept %d %s";
static const char long_spc391[]  = "accept failed to acquire a new socket (errno=%d:%s)";
static const char short_spc392[] = "**sock|poll|badbuf %d %d %d %s";
static const char long_spc392[]  = "the supplied buffer contains invalid memory (set=%d,sock=%d,errno=%d:%s)";
static const char short_spc393[] = "**sock|poll|badhandle %d %d %d";
static const char long_spc393[]  = "sock contains an invalid file descriptor (set=%d,sock=%d,fd=%d)";
static const char short_spc394[] = "**sock|poll|badhandle %d %d %d %d";
static const char long_spc394[]  = "sock contains an invalid file descriptor (set=%d,sock=%d,fd=%d:%d)";
static const char short_spc395[] = "**sock|poll|bind %d %d %s";
static const char long_spc395[]  = "unable to bind socket to port (port=%d,errno=%d:%s)";
static const char short_spc396[] = "**sock|poll|connclosed %d %d %d %s";
static const char long_spc396[]  = "connection closed by peer (set=%d,sock=%d,errno=%d:%s)";
static const char short_spc397[] = "**sock|poll|connfailed %d %d %d %s";
static const char long_spc397[]  = "connection failure (set=%d,sock=%d,errno=%d:%s)";
static const char short_spc398[] = "**sock|poll|connrefused %d %d %s";
static const char long_spc398[]  = "connection refused (set=%d,sock=%d,host=%s)";
static const char short_spc399[] = "**sock|poll|eqfail %d %d %d";
static const char long_spc399[]  = "fatal error: failed to enqueue an event; event was lost (set=%d,sock=%d,op=%d)";
static const char short_spc400[] = "**sock|poll|getport %d %s";
static const char long_spc400[]  = "failed to obtain port number of listener (errno=%d:%s)";
static const char short_spc401[] = "**sock|poll|hostres %d %d %s";
static const char long_spc401[]  = "unable to resolve host name to an address (set=%d,sock=%d,host=%s)";
static const char short_spc402[] = "**sock|poll|listen %d %s";
static const char long_spc402[]  = "listen() failed (errno=%d:%s)";
static const char short_spc403[] = "**sock|poll|nodelay %d %s";
static const char long_spc403[]  = "unable to set TCP no delay attribute on socket (errno=%d:%s)";
static const char short_spc404[] = "**sock|poll|nonblock %d %s";
static const char long_spc404[]  = "unable to set socket to nonblocking (errno=%d:%s)";
static const char short_spc405[] = "**sock|poll|oserror %d %d %d %s";
static const char long_spc405[]  = "unexpected operating system error (set=%d,sock=%d,errno=%d:%s)";
static const char short_spc406[] = "**sock|poll|oserror %d %s";
static const char long_spc406[]  = "unexpected operating system error (errno=%d:%s)";
static const char short_spc407[] = "**sock|poll|oserror %s %d %d %d %s";
static const char long_spc407[]  = "unpected operating system error from %s (set=%d,sock=%d,errno=%d:%s)";
static const char short_spc408[] = "**sock|poll|osnomem %d %d %d %s";
static const char long_spc408[]  = "operating system routine failed due to lack of memory (set=%d,sock=%d,errno=%d:%s)";
static const char short_spc409[] = "**sock|poll|pipe %d %s";
static const char long_spc409[]  = "unable to allocate pipe to wakeup a blocking poll() (errno=%d:%s)";
static const char short_spc410[] = "**sock|poll|pipenonblock %d %s";
static const char long_spc410[]  = "unable to set wakeup pipe to nonblocking (errno=%d:%s)";
static const char short_spc411[] = "**sock|poll|reuseaddr %d %s";
static const char long_spc411[]  = "unable to set reuseaddr attribute on socket (errno=%d:%s)";
static const char short_spc412[] = "**sock|poll|socket %d %s";
static const char long_spc412[]  = "unable to obtain new socket (errno=%d:%s)";
static const char short_spc413[] = "**sock|poll|unhandledstate %d";
static const char long_spc413[]  = "encountered an unexpected state (%d)";
static const char short_spc414[] = "**sock|poll|unhandledtype %d";
static const char long_spc414[]  = "encountered an unexpected sock type (%d)";
static const char short_spc415[] = "**sock|reads %d %d";
static const char long_spc415[]  = "attempt to perform multiple simultaneous reads (set=%d,sock=%d)";
static const char short_spc416[] = "**sock|writes %d %d";
static const char long_spc416[]  = "attempt to perform multiple simultaneous writes (set=%d,sock=%d)";
static const char short_spc417[] = "**stride %d %d %d";
static const char long_spc417[]  = "Range (start = %d, end = %d, stride = %d) does not terminate";
static const char short_spc418[] = "**tag %d";
static const char long_spc418[]  = "Invalid tag, value is %d";
static const char short_spc419[] = "**topotoolarge %d %d";
static const char long_spc419[]  = "Topology size %d is larger than communicator size (%d)";
static const char short_spc420[] = "**truncate %d %d";
static const char long_spc420[]  = "Message truncated; %d bytes received but buffer size is %d";
static const char short_spc421[] = "**truncate %d %d %d %d";
static const char long_spc421[]  = "Message from rank %d and tag %d truncated; %d bytes received but buffer size is %d";
static const char short_spc422[] = "**typematchsize %s %d";
static const char long_spc422[]  = "No MPI datatype available for typeclass %s and size %d";
static const char short_spc423[] = "**typenamelen %d";
static const char long_spc423[]  = " Specified datatype name is too long (%d characters)";
static const char short_spc424[] = "**vc_state %d";
static const char long_spc424[]  = "invalid vc state (%d)";
static const char short_spc425[] = "**winunlockrank %d %d";
static const char long_spc425[]  = "Invalid rank argument %d, should be %d";
static const char short_spc426[] = "**wsasock %s %d";
static const char long_spc426[]  = "WSAStartup failed, %s (errno %d)";

static const int specific_msgs_len = 427;
static const msgpair specific_err_msgs[] = {
{ 0xacebad03, short_spc0, long_spc0, 0xcb0bfa11 },
{ 0xacebad03, short_spc1, long_spc1, 0xcb0bfa11 },
{ 0xacebad03, short_spc2, long_spc2, 0xcb0bfa11 },
{ 0xacebad03, short_spc3, long_spc3, 0xcb0bfa11 },
{ 0xacebad03, short_spc4, long_spc4, 0xcb0bfa11 },
{ 0xacebad03, short_spc5, long_spc5, 0xcb0bfa11 },
{ 0xacebad03, short_spc6, long_spc6, 0xcb0bfa11 },
{ 0xacebad03, short_spc7, long_spc7, 0xcb0bfa11 },
{ 0xacebad03, short_spc8, long_spc8, 0xcb0bfa11 },
{ 0xacebad03, short_spc9, long_spc9, 0xcb0bfa11 },
{ 0xacebad03, short_spc10, long_spc10, 0xcb0bfa11 },
{ 0xacebad03, short_spc11, long_spc11, 0xcb0bfa11 },
{ 0xacebad03, short_spc12, long_spc12, 0xcb0bfa11 },
{ 0xacebad03, short_spc13, long_spc13, 0xcb0bfa11 },
{ 0xacebad03, short_spc14, long_spc14, 0xcb0bfa11 },
{ 0xacebad03, short_spc15, long_spc15, 0xcb0bfa11 },
{ 0xacebad03, short_spc16, long_spc16, 0xcb0bfa11 },
{ 0xacebad03, short_spc17, long_spc17, 0xcb0bfa11 },
{ 0xacebad03, short_spc18, long_spc18, 0xcb0bfa11 },
{ 0xacebad03, short_spc19, long_spc19, 0xcb0bfa11 },
{ 0xacebad03, short_spc20, long_spc20, 0xcb0bfa11 },
{ 0xacebad03, short_spc21, long_spc21, 0xcb0bfa11 },
{ 0xacebad03, short_spc22, long_spc22, 0xcb0bfa11 },
{ 0xacebad03, short_spc23, long_spc23, 0xcb0bfa11 },
{ 0xacebad03, short_spc24, long_spc24, 0xcb0bfa11 },
{ 0xacebad03, short_spc25, long_spc25, 0xcb0bfa11 },
{ 0xacebad03, short_spc26, long_spc26, 0xcb0bfa11 },
{ 0xacebad03, short_spc27, long_spc27, 0xcb0bfa11 },
{ 0xacebad03, short_spc28, long_spc28, 0xcb0bfa11 },
{ 0xacebad03, short_spc29, long_spc29, 0xcb0bfa11 },
{ 0xacebad03, short_spc30, long_spc30, 0xcb0bfa11 },
{ 0xacebad03, short_spc31, long_spc31, 0xcb0bfa11 },
{ 0xacebad03, short_spc32, long_spc32, 0xcb0bfa11 },
{ 0xacebad03, short_spc33, long_spc33, 0xcb0bfa11 },
{ 0xacebad03, short_spc34, long_spc34, 0xcb0bfa11 },
{ 0xacebad03, short_spc35, long_spc35, 0xcb0bfa11 },
{ 0xacebad03, short_spc36, long_spc36, 0xcb0bfa11 },
{ 0xacebad03, short_spc37, long_spc37, 0xcb0bfa11 },
{ 0xacebad03, short_spc38, long_spc38, 0xcb0bfa11 },
{ 0xacebad03, short_spc39, long_spc39, 0xcb0bfa11 },
{ 0xacebad03, short_spc40, long_spc40, 0xcb0bfa11 },
{ 0xacebad03, short_spc41, long_spc41, 0xcb0bfa11 },
{ 0xacebad03, short_spc42, long_spc42, 0xcb0bfa11 },
{ 0xacebad03, short_spc43, long_spc43, 0xcb0bfa11 },
{ 0xacebad03, short_spc44, long_spc44, 0xcb0bfa11 },
{ 0xacebad03, short_spc45, long_spc45, 0xcb0bfa11 },
{ 0xacebad03, short_spc46, long_spc46, 0xcb0bfa11 },
{ 0xacebad03, short_spc47, long_spc47, 0xcb0bfa11 },
{ 0xacebad03, short_spc48, long_spc48, 0xcb0bfa11 },
{ 0xacebad03, short_spc49, long_spc49, 0xcb0bfa11 },
{ 0xacebad03, short_spc50, long_spc50, 0xcb0bfa11 },
{ 0xacebad03, short_spc51, long_spc51, 0xcb0bfa11 },
{ 0xacebad03, short_spc52, long_spc52, 0xcb0bfa11 },
{ 0xacebad03, short_spc53, long_spc53, 0xcb0bfa11 },
{ 0xacebad03, short_spc54, long_spc54, 0xcb0bfa11 },
{ 0xacebad03, short_spc55, long_spc55, 0xcb0bfa11 },
{ 0xacebad03, short_spc56, long_spc56, 0xcb0bfa11 },
{ 0xacebad03, short_spc57, long_spc57, 0xcb0bfa11 },
{ 0xacebad03, short_spc58, long_spc58, 0xcb0bfa11 },
{ 0xacebad03, short_spc59, long_spc59, 0xcb0bfa11 },
{ 0xacebad03, short_spc60, long_spc60, 0xcb0bfa11 },
{ 0xacebad03, short_spc61, long_spc61, 0xcb0bfa11 },
{ 0xacebad03, short_spc62, long_spc62, 0xcb0bfa11 },
{ 0xacebad03, short_spc63, long_spc63, 0xcb0bfa11 },
{ 0xacebad03, short_spc64, long_spc64, 0xcb0bfa11 },
{ 0xacebad03, short_spc65, long_spc65, 0xcb0bfa11 },
{ 0xacebad03, short_spc66, long_spc66, 0xcb0bfa11 },
{ 0xacebad03, short_spc67, long_spc67, 0xcb0bfa11 },
{ 0xacebad03, short_spc68, long_spc68, 0xcb0bfa11 },
{ 0xacebad03, short_spc69, long_spc69, 0xcb0bfa11 },
{ 0xacebad03, short_spc70, long_spc70, 0xcb0bfa11 },
{ 0xacebad03, short_spc71, long_spc71, 0xcb0bfa11 },
{ 0xacebad03, short_spc72, long_spc72, 0xcb0bfa11 },
{ 0xacebad03, short_spc73, long_spc73, 0xcb0bfa11 },
{ 0xacebad03, short_spc74, long_spc74, 0xcb0bfa11 },
{ 0xacebad03, short_spc75, long_spc75, 0xcb0bfa11 },
{ 0xacebad03, short_spc76, long_spc76, 0xcb0bfa11 },
{ 0xacebad03, short_spc77, long_spc77, 0xcb0bfa11 },
{ 0xacebad03, short_spc78, long_spc78, 0xcb0bfa11 },
{ 0xacebad03, short_spc79, long_spc79, 0xcb0bfa11 },
{ 0xacebad03, short_spc80, long_spc80, 0xcb0bfa11 },
{ 0xacebad03, short_spc81, long_spc81, 0xcb0bfa11 },
{ 0xacebad03, short_spc82, long_spc82, 0xcb0bfa11 },
{ 0xacebad03, short_spc83, long_spc83, 0xcb0bfa11 },
{ 0xacebad03, short_spc84, long_spc84, 0xcb0bfa11 },
{ 0xacebad03, short_spc85, long_spc85, 0xcb0bfa11 },
{ 0xacebad03, short_spc86, long_spc86, 0xcb0bfa11 },
{ 0xacebad03, short_spc87, long_spc87, 0xcb0bfa11 },
{ 0xacebad03, short_spc88, long_spc88, 0xcb0bfa11 },
{ 0xacebad03, short_spc89, long_spc89, 0xcb0bfa11 },
{ 0xacebad03, short_spc90, long_spc90, 0xcb0bfa11 },
{ 0xacebad03, short_spc91, long_spc91, 0xcb0bfa11 },
{ 0xacebad03, short_spc92, long_spc92, 0xcb0bfa11 },
{ 0xacebad03, short_spc93, long_spc93, 0xcb0bfa11 },
{ 0xacebad03, short_spc94, long_spc94, 0xcb0bfa11 },
{ 0xacebad03, short_spc95, long_spc95, 0xcb0bfa11 },
{ 0xacebad03, short_spc96, long_spc96, 0xcb0bfa11 },
{ 0xacebad03, short_spc97, long_spc97, 0xcb0bfa11 },
{ 0xacebad03, short_spc98, long_spc98, 0xcb0bfa11 },
{ 0xacebad03, short_spc99, long_spc99, 0xcb0bfa11 },
{ 0xacebad03, short_spc100, long_spc100, 0xcb0bfa11 },
{ 0xacebad03, short_spc101, long_spc101, 0xcb0bfa11 },
{ 0xacebad03, short_spc102, long_spc102, 0xcb0bfa11 },
{ 0xacebad03, short_spc103, long_spc103, 0xcb0bfa11 },
{ 0xacebad03, short_spc104, long_spc104, 0xcb0bfa11 },
{ 0xacebad03, short_spc105, long_spc105, 0xcb0bfa11 },
{ 0xacebad03, short_spc106, long_spc106, 0xcb0bfa11 },
{ 0xacebad03, short_spc107, long_spc107, 0xcb0bfa11 },
{ 0xacebad03, short_spc108, long_spc108, 0xcb0bfa11 },
{ 0xacebad03, short_spc109, long_spc109, 0xcb0bfa11 },
{ 0xacebad03, short_spc110, long_spc110, 0xcb0bfa11 },
{ 0xacebad03, short_spc111, long_spc111, 0xcb0bfa11 },
{ 0xacebad03, short_spc112, long_spc112, 0xcb0bfa11 },
{ 0xacebad03, short_spc113, long_spc113, 0xcb0bfa11 },
{ 0xacebad03, short_spc114, long_spc114, 0xcb0bfa11 },
{ 0xacebad03, short_spc115, long_spc115, 0xcb0bfa11 },
{ 0xacebad03, short_spc116, long_spc116, 0xcb0bfa11 },
{ 0xacebad03, short_spc117, long_spc117, 0xcb0bfa11 },
{ 0xacebad03, short_spc118, long_spc118, 0xcb0bfa11 },
{ 0xacebad03, short_spc119, long_spc119, 0xcb0bfa11 },
{ 0xacebad03, short_spc120, long_spc120, 0xcb0bfa11 },
{ 0xacebad03, short_spc121, long_spc121, 0xcb0bfa11 },
{ 0xacebad03, short_spc122, long_spc122, 0xcb0bfa11 },
{ 0xacebad03, short_spc123, long_spc123, 0xcb0bfa11 },
{ 0xacebad03, short_spc124, long_spc124, 0xcb0bfa11 },
{ 0xacebad03, short_spc125, long_spc125, 0xcb0bfa11 },
{ 0xacebad03, short_spc126, long_spc126, 0xcb0bfa11 },
{ 0xacebad03, short_spc127, long_spc127, 0xcb0bfa11 },
{ 0xacebad03, short_spc128, long_spc128, 0xcb0bfa11 },
{ 0xacebad03, short_spc129, long_spc129, 0xcb0bfa11 },
{ 0xacebad03, short_spc130, long_spc130, 0xcb0bfa11 },
{ 0xacebad03, short_spc131, long_spc131, 0xcb0bfa11 },
{ 0xacebad03, short_spc132, long_spc132, 0xcb0bfa11 },
{ 0xacebad03, short_spc133, long_spc133, 0xcb0bfa11 },
{ 0xacebad03, short_spc134, long_spc134, 0xcb0bfa11 },
{ 0xacebad03, short_spc135, long_spc135, 0xcb0bfa11 },
{ 0xacebad03, short_spc136, long_spc136, 0xcb0bfa11 },
{ 0xacebad03, short_spc137, long_spc137, 0xcb0bfa11 },
{ 0xacebad03, short_spc138, long_spc138, 0xcb0bfa11 },
{ 0xacebad03, short_spc139, long_spc139, 0xcb0bfa11 },
{ 0xacebad03, short_spc140, long_spc140, 0xcb0bfa11 },
{ 0xacebad03, short_spc141, long_spc141, 0xcb0bfa11 },
{ 0xacebad03, short_spc142, long_spc142, 0xcb0bfa11 },
{ 0xacebad03, short_spc143, long_spc143, 0xcb0bfa11 },
{ 0xacebad03, short_spc144, long_spc144, 0xcb0bfa11 },
{ 0xacebad03, short_spc145, long_spc145, 0xcb0bfa11 },
{ 0xacebad03, short_spc146, long_spc146, 0xcb0bfa11 },
{ 0xacebad03, short_spc147, long_spc147, 0xcb0bfa11 },
{ 0xacebad03, short_spc148, long_spc148, 0xcb0bfa11 },
{ 0xacebad03, short_spc149, long_spc149, 0xcb0bfa11 },
{ 0xacebad03, short_spc150, long_spc150, 0xcb0bfa11 },
{ 0xacebad03, short_spc151, long_spc151, 0xcb0bfa11 },
{ 0xacebad03, short_spc152, long_spc152, 0xcb0bfa11 },
{ 0xacebad03, short_spc153, long_spc153, 0xcb0bfa11 },
{ 0xacebad03, short_spc154, long_spc154, 0xcb0bfa11 },
{ 0xacebad03, short_spc155, long_spc155, 0xcb0bfa11 },
{ 0xacebad03, short_spc156, long_spc156, 0xcb0bfa11 },
{ 0xacebad03, short_spc157, long_spc157, 0xcb0bfa11 },
{ 0xacebad03, short_spc158, long_spc158, 0xcb0bfa11 },
{ 0xacebad03, short_spc159, long_spc159, 0xcb0bfa11 },
{ 0xacebad03, short_spc160, long_spc160, 0xcb0bfa11 },
{ 0xacebad03, short_spc161, long_spc161, 0xcb0bfa11 },
{ 0xacebad03, short_spc162, long_spc162, 0xcb0bfa11 },
{ 0xacebad03, short_spc163, long_spc163, 0xcb0bfa11 },
{ 0xacebad03, short_spc164, long_spc164, 0xcb0bfa11 },
{ 0xacebad03, short_spc165, long_spc165, 0xcb0bfa11 },
{ 0xacebad03, short_spc166, long_spc166, 0xcb0bfa11 },
{ 0xacebad03, short_spc167, long_spc167, 0xcb0bfa11 },
{ 0xacebad03, short_spc168, long_spc168, 0xcb0bfa11 },
{ 0xacebad03, short_spc169, long_spc169, 0xcb0bfa11 },
{ 0xacebad03, short_spc170, long_spc170, 0xcb0bfa11 },
{ 0xacebad03, short_spc171, long_spc171, 0xcb0bfa11 },
{ 0xacebad03, short_spc172, long_spc172, 0xcb0bfa11 },
{ 0xacebad03, short_spc173, long_spc173, 0xcb0bfa11 },
{ 0xacebad03, short_spc174, long_spc174, 0xcb0bfa11 },
{ 0xacebad03, short_spc175, long_spc175, 0xcb0bfa11 },
{ 0xacebad03, short_spc176, long_spc176, 0xcb0bfa11 },
{ 0xacebad03, short_spc177, long_spc177, 0xcb0bfa11 },
{ 0xacebad03, short_spc178, long_spc178, 0xcb0bfa11 },
{ 0xacebad03, short_spc179, long_spc179, 0xcb0bfa11 },
{ 0xacebad03, short_spc180, long_spc180, 0xcb0bfa11 },
{ 0xacebad03, short_spc181, long_spc181, 0xcb0bfa11 },
{ 0xacebad03, short_spc182, long_spc182, 0xcb0bfa11 },
{ 0xacebad03, short_spc183, long_spc183, 0xcb0bfa11 },
{ 0xacebad03, short_spc184, long_spc184, 0xcb0bfa11 },
{ 0xacebad03, short_spc185, long_spc185, 0xcb0bfa11 },
{ 0xacebad03, short_spc186, long_spc186, 0xcb0bfa11 },
{ 0xacebad03, short_spc187, long_spc187, 0xcb0bfa11 },
{ 0xacebad03, short_spc188, long_spc188, 0xcb0bfa11 },
{ 0xacebad03, short_spc189, long_spc189, 0xcb0bfa11 },
{ 0xacebad03, short_spc190, long_spc190, 0xcb0bfa11 },
{ 0xacebad03, short_spc191, long_spc191, 0xcb0bfa11 },
{ 0xacebad03, short_spc192, long_spc192, 0xcb0bfa11 },
{ 0xacebad03, short_spc193, long_spc193, 0xcb0bfa11 },
{ 0xacebad03, short_spc194, long_spc194, 0xcb0bfa11 },
{ 0xacebad03, short_spc195, long_spc195, 0xcb0bfa11 },
{ 0xacebad03, short_spc196, long_spc196, 0xcb0bfa11 },
{ 0xacebad03, short_spc197, long_spc197, 0xcb0bfa11 },
{ 0xacebad03, short_spc198, long_spc198, 0xcb0bfa11 },
{ 0xacebad03, short_spc199, long_spc199, 0xcb0bfa11 },
{ 0xacebad03, short_spc200, long_spc200, 0xcb0bfa11 },
{ 0xacebad03, short_spc201, long_spc201, 0xcb0bfa11 },
{ 0xacebad03, short_spc202, long_spc202, 0xcb0bfa11 },
{ 0xacebad03, short_spc203, long_spc203, 0xcb0bfa11 },
{ 0xacebad03, short_spc204, long_spc204, 0xcb0bfa11 },
{ 0xacebad03, short_spc205, long_spc205, 0xcb0bfa11 },
{ 0xacebad03, short_spc206, long_spc206, 0xcb0bfa11 },
{ 0xacebad03, short_spc207, long_spc207, 0xcb0bfa11 },
{ 0xacebad03, short_spc208, long_spc208, 0xcb0bfa11 },
{ 0xacebad03, short_spc209, long_spc209, 0xcb0bfa11 },
{ 0xacebad03, short_spc210, long_spc210, 0xcb0bfa11 },
{ 0xacebad03, short_spc211, long_spc211, 0xcb0bfa11 },
{ 0xacebad03, short_spc212, long_spc212, 0xcb0bfa11 },
{ 0xacebad03, short_spc213, long_spc213, 0xcb0bfa11 },
{ 0xacebad03, short_spc214, long_spc214, 0xcb0bfa11 },
{ 0xacebad03, short_spc215, long_spc215, 0xcb0bfa11 },
{ 0xacebad03, short_spc216, long_spc216, 0xcb0bfa11 },
{ 0xacebad03, short_spc217, long_spc217, 0xcb0bfa11 },
{ 0xacebad03, short_spc218, long_spc218, 0xcb0bfa11 },
{ 0xacebad03, short_spc219, long_spc219, 0xcb0bfa11 },
{ 0xacebad03, short_spc220, long_spc220, 0xcb0bfa11 },
{ 0xacebad03, short_spc221, long_spc221, 0xcb0bfa11 },
{ 0xacebad03, short_spc222, long_spc222, 0xcb0bfa11 },
{ 0xacebad03, short_spc223, long_spc223, 0xcb0bfa11 },
{ 0xacebad03, short_spc224, long_spc224, 0xcb0bfa11 },
{ 0xacebad03, short_spc225, long_spc225, 0xcb0bfa11 },
{ 0xacebad03, short_spc226, long_spc226, 0xcb0bfa11 },
{ 0xacebad03, short_spc227, long_spc227, 0xcb0bfa11 },
{ 0xacebad03, short_spc228, long_spc228, 0xcb0bfa11 },
{ 0xacebad03, short_spc229, long_spc229, 0xcb0bfa11 },
{ 0xacebad03, short_spc230, long_spc230, 0xcb0bfa11 },
{ 0xacebad03, short_spc231, long_spc231, 0xcb0bfa11 },
{ 0xacebad03, short_spc232, long_spc232, 0xcb0bfa11 },
{ 0xacebad03, short_spc233, long_spc233, 0xcb0bfa11 },
{ 0xacebad03, short_spc234, long_spc234, 0xcb0bfa11 },
{ 0xacebad03, short_spc235, long_spc235, 0xcb0bfa11 },
{ 0xacebad03, short_spc236, long_spc236, 0xcb0bfa11 },
{ 0xacebad03, short_spc237, long_spc237, 0xcb0bfa11 },
{ 0xacebad03, short_spc238, long_spc238, 0xcb0bfa11 },
{ 0xacebad03, short_spc239, long_spc239, 0xcb0bfa11 },
{ 0xacebad03, short_spc240, long_spc240, 0xcb0bfa11 },
{ 0xacebad03, short_spc241, long_spc241, 0xcb0bfa11 },
{ 0xacebad03, short_spc242, long_spc242, 0xcb0bfa11 },
{ 0xacebad03, short_spc243, long_spc243, 0xcb0bfa11 },
{ 0xacebad03, short_spc244, long_spc244, 0xcb0bfa11 },
{ 0xacebad03, short_spc245, long_spc245, 0xcb0bfa11 },
{ 0xacebad03, short_spc246, long_spc246, 0xcb0bfa11 },
{ 0xacebad03, short_spc247, long_spc247, 0xcb0bfa11 },
{ 0xacebad03, short_spc248, long_spc248, 0xcb0bfa11 },
{ 0xacebad03, short_spc249, long_spc249, 0xcb0bfa11 },
{ 0xacebad03, short_spc250, long_spc250, 0xcb0bfa11 },
{ 0xacebad03, short_spc251, long_spc251, 0xcb0bfa11 },
{ 0xacebad03, short_spc252, long_spc252, 0xcb0bfa11 },
{ 0xacebad03, short_spc253, long_spc253, 0xcb0bfa11 },
{ 0xacebad03, short_spc254, long_spc254, 0xcb0bfa11 },
{ 0xacebad03, short_spc255, long_spc255, 0xcb0bfa11 },
{ 0xacebad03, short_spc256, long_spc256, 0xcb0bfa11 },
{ 0xacebad03, short_spc257, long_spc257, 0xcb0bfa11 },
{ 0xacebad03, short_spc258, long_spc258, 0xcb0bfa11 },
{ 0xacebad03, short_spc259, long_spc259, 0xcb0bfa11 },
{ 0xacebad03, short_spc260, long_spc260, 0xcb0bfa11 },
{ 0xacebad03, short_spc261, long_spc261, 0xcb0bfa11 },
{ 0xacebad03, short_spc262, long_spc262, 0xcb0bfa11 },
{ 0xacebad03, short_spc263, long_spc263, 0xcb0bfa11 },
{ 0xacebad03, short_spc264, long_spc264, 0xcb0bfa11 },
{ 0xacebad03, short_spc265, long_spc265, 0xcb0bfa11 },
{ 0xacebad03, short_spc266, long_spc266, 0xcb0bfa11 },
{ 0xacebad03, short_spc267, long_spc267, 0xcb0bfa11 },
{ 0xacebad03, short_spc268, long_spc268, 0xcb0bfa11 },
{ 0xacebad03, short_spc269, long_spc269, 0xcb0bfa11 },
{ 0xacebad03, short_spc270, long_spc270, 0xcb0bfa11 },
{ 0xacebad03, short_spc271, long_spc271, 0xcb0bfa11 },
{ 0xacebad03, short_spc272, long_spc272, 0xcb0bfa11 },
{ 0xacebad03, short_spc273, long_spc273, 0xcb0bfa11 },
{ 0xacebad03, short_spc274, long_spc274, 0xcb0bfa11 },
{ 0xacebad03, short_spc275, long_spc275, 0xcb0bfa11 },
{ 0xacebad03, short_spc276, long_spc276, 0xcb0bfa11 },
{ 0xacebad03, short_spc277, long_spc277, 0xcb0bfa11 },
{ 0xacebad03, short_spc278, long_spc278, 0xcb0bfa11 },
{ 0xacebad03, short_spc279, long_spc279, 0xcb0bfa11 },
{ 0xacebad03, short_spc280, long_spc280, 0xcb0bfa11 },
{ 0xacebad03, short_spc281, long_spc281, 0xcb0bfa11 },
{ 0xacebad03, short_spc282, long_spc282, 0xcb0bfa11 },
{ 0xacebad03, short_spc283, long_spc283, 0xcb0bfa11 },
{ 0xacebad03, short_spc284, long_spc284, 0xcb0bfa11 },
{ 0xacebad03, short_spc285, long_spc285, 0xcb0bfa11 },
{ 0xacebad03, short_spc286, long_spc286, 0xcb0bfa11 },
{ 0xacebad03, short_spc287, long_spc287, 0xcb0bfa11 },
{ 0xacebad03, short_spc288, long_spc288, 0xcb0bfa11 },
{ 0xacebad03, short_spc289, long_spc289, 0xcb0bfa11 },
{ 0xacebad03, short_spc290, long_spc290, 0xcb0bfa11 },
{ 0xacebad03, short_spc291, long_spc291, 0xcb0bfa11 },
{ 0xacebad03, short_spc292, long_spc292, 0xcb0bfa11 },
{ 0xacebad03, short_spc293, long_spc293, 0xcb0bfa11 },
{ 0xacebad03, short_spc294, long_spc294, 0xcb0bfa11 },
{ 0xacebad03, short_spc295, long_spc295, 0xcb0bfa11 },
{ 0xacebad03, short_spc296, long_spc296, 0xcb0bfa11 },
{ 0xacebad03, short_spc297, long_spc297, 0xcb0bfa11 },
{ 0xacebad03, short_spc298, long_spc298, 0xcb0bfa11 },
{ 0xacebad03, short_spc299, long_spc299, 0xcb0bfa11 },
{ 0xacebad03, short_spc300, long_spc300, 0xcb0bfa11 },
{ 0xacebad03, short_spc301, long_spc301, 0xcb0bfa11 },
{ 0xacebad03, short_spc302, long_spc302, 0xcb0bfa11 },
{ 0xacebad03, short_spc303, long_spc303, 0xcb0bfa11 },
{ 0xacebad03, short_spc304, long_spc304, 0xcb0bfa11 },
{ 0xacebad03, short_spc305, long_spc305, 0xcb0bfa11 },
{ 0xacebad03, short_spc306, long_spc306, 0xcb0bfa11 },
{ 0xacebad03, short_spc307, long_spc307, 0xcb0bfa11 },
{ 0xacebad03, short_spc308, long_spc308, 0xcb0bfa11 },
{ 0xacebad03, short_spc309, long_spc309, 0xcb0bfa11 },
{ 0xacebad03, short_spc310, long_spc310, 0xcb0bfa11 },
{ 0xacebad03, short_spc311, long_spc311, 0xcb0bfa11 },
{ 0xacebad03, short_spc312, long_spc312, 0xcb0bfa11 },
{ 0xacebad03, short_spc313, long_spc313, 0xcb0bfa11 },
{ 0xacebad03, short_spc314, long_spc314, 0xcb0bfa11 },
{ 0xacebad03, short_spc315, long_spc315, 0xcb0bfa11 },
{ 0xacebad03, short_spc316, long_spc316, 0xcb0bfa11 },
{ 0xacebad03, short_spc317, long_spc317, 0xcb0bfa11 },
{ 0xacebad03, short_spc318, long_spc318, 0xcb0bfa11 },
{ 0xacebad03, short_spc319, long_spc319, 0xcb0bfa11 },
{ 0xacebad03, short_spc320, long_spc320, 0xcb0bfa11 },
{ 0xacebad03, short_spc321, long_spc321, 0xcb0bfa11 },
{ 0xacebad03, short_spc322, long_spc322, 0xcb0bfa11 },
{ 0xacebad03, short_spc323, long_spc323, 0xcb0bfa11 },
{ 0xacebad03, short_spc324, long_spc324, 0xcb0bfa11 },
{ 0xacebad03, short_spc325, long_spc325, 0xcb0bfa11 },
{ 0xacebad03, short_spc326, long_spc326, 0xcb0bfa11 },
{ 0xacebad03, short_spc327, long_spc327, 0xcb0bfa11 },
{ 0xacebad03, short_spc328, long_spc328, 0xcb0bfa11 },
{ 0xacebad03, short_spc329, long_spc329, 0xcb0bfa11 },
{ 0xacebad03, short_spc330, long_spc330, 0xcb0bfa11 },
{ 0xacebad03, short_spc331, long_spc331, 0xcb0bfa11 },
{ 0xacebad03, short_spc332, long_spc332, 0xcb0bfa11 },
{ 0xacebad03, short_spc333, long_spc333, 0xcb0bfa11 },
{ 0xacebad03, short_spc334, long_spc334, 0xcb0bfa11 },
{ 0xacebad03, short_spc335, long_spc335, 0xcb0bfa11 },
{ 0xacebad03, short_spc336, long_spc336, 0xcb0bfa11 },
{ 0xacebad03, short_spc337, long_spc337, 0xcb0bfa11 },
{ 0xacebad03, short_spc338, long_spc338, 0xcb0bfa11 },
{ 0xacebad03, short_spc339, long_spc339, 0xcb0bfa11 },
{ 0xacebad03, short_spc340, long_spc340, 0xcb0bfa11 },
{ 0xacebad03, short_spc341, long_spc341, 0xcb0bfa11 },
{ 0xacebad03, short_spc342, long_spc342, 0xcb0bfa11 },
{ 0xacebad03, short_spc343, long_spc343, 0xcb0bfa11 },
{ 0xacebad03, short_spc344, long_spc344, 0xcb0bfa11 },
{ 0xacebad03, short_spc345, long_spc345, 0xcb0bfa11 },
{ 0xacebad03, short_spc346, long_spc346, 0xcb0bfa11 },
{ 0xacebad03, short_spc347, long_spc347, 0xcb0bfa11 },
{ 0xacebad03, short_spc348, long_spc348, 0xcb0bfa11 },
{ 0xacebad03, short_spc349, long_spc349, 0xcb0bfa11 },
{ 0xacebad03, short_spc350, long_spc350, 0xcb0bfa11 },
{ 0xacebad03, short_spc351, long_spc351, 0xcb0bfa11 },
{ 0xacebad03, short_spc352, long_spc352, 0xcb0bfa11 },
{ 0xacebad03, short_spc353, long_spc353, 0xcb0bfa11 },
{ 0xacebad03, short_spc354, long_spc354, 0xcb0bfa11 },
{ 0xacebad03, short_spc355, long_spc355, 0xcb0bfa11 },
{ 0xacebad03, short_spc356, long_spc356, 0xcb0bfa11 },
{ 0xacebad03, short_spc357, long_spc357, 0xcb0bfa11 },
{ 0xacebad03, short_spc358, long_spc358, 0xcb0bfa11 },
{ 0xacebad03, short_spc359, long_spc359, 0xcb0bfa11 },
{ 0xacebad03, short_spc360, long_spc360, 0xcb0bfa11 },
{ 0xacebad03, short_spc361, long_spc361, 0xcb0bfa11 },
{ 0xacebad03, short_spc362, long_spc362, 0xcb0bfa11 },
{ 0xacebad03, short_spc363, long_spc363, 0xcb0bfa11 },
{ 0xacebad03, short_spc364, long_spc364, 0xcb0bfa11 },
{ 0xacebad03, short_spc365, long_spc365, 0xcb0bfa11 },
{ 0xacebad03, short_spc366, long_spc366, 0xcb0bfa11 },
{ 0xacebad03, short_spc367, long_spc367, 0xcb0bfa11 },
{ 0xacebad03, short_spc368, long_spc368, 0xcb0bfa11 },
{ 0xacebad03, short_spc369, long_spc369, 0xcb0bfa11 },
{ 0xacebad03, short_spc370, long_spc370, 0xcb0bfa11 },
{ 0xacebad03, short_spc371, long_spc371, 0xcb0bfa11 },
{ 0xacebad03, short_spc372, long_spc372, 0xcb0bfa11 },
{ 0xacebad03, short_spc373, long_spc373, 0xcb0bfa11 },
{ 0xacebad03, short_spc374, long_spc374, 0xcb0bfa11 },
{ 0xacebad03, short_spc375, long_spc375, 0xcb0bfa11 },
{ 0xacebad03, short_spc376, long_spc376, 0xcb0bfa11 },
{ 0xacebad03, short_spc377, long_spc377, 0xcb0bfa11 },
{ 0xacebad03, short_spc378, long_spc378, 0xcb0bfa11 },
{ 0xacebad03, short_spc379, long_spc379, 0xcb0bfa11 },
{ 0xacebad03, short_spc380, long_spc380, 0xcb0bfa11 },
{ 0xacebad03, short_spc381, long_spc381, 0xcb0bfa11 },
{ 0xacebad03, short_spc382, long_spc382, 0xcb0bfa11 },
{ 0xacebad03, short_spc383, long_spc383, 0xcb0bfa11 },
{ 0xacebad03, short_spc384, long_spc384, 0xcb0bfa11 },
{ 0xacebad03, short_spc385, long_spc385, 0xcb0bfa11 },
{ 0xacebad03, short_spc386, long_spc386, 0xcb0bfa11 },
{ 0xacebad03, short_spc387, long_spc387, 0xcb0bfa11 },
{ 0xacebad03, short_spc388, long_spc388, 0xcb0bfa11 },
{ 0xacebad03, short_spc389, long_spc389, 0xcb0bfa11 },
{ 0xacebad03, short_spc390, long_spc390, 0xcb0bfa11 },
{ 0xacebad03, short_spc391, long_spc391, 0xcb0bfa11 },
{ 0xacebad03, short_spc392, long_spc392, 0xcb0bfa11 },
{ 0xacebad03, short_spc393, long_spc393, 0xcb0bfa11 },
{ 0xacebad03, short_spc394, long_spc394, 0xcb0bfa11 },
{ 0xacebad03, short_spc395, long_spc395, 0xcb0bfa11 },
{ 0xacebad03, short_spc396, long_spc396, 0xcb0bfa11 },
{ 0xacebad03, short_spc397, long_spc397, 0xcb0bfa11 },
{ 0xacebad03, short_spc398, long_spc398, 0xcb0bfa11 },
{ 0xacebad03, short_spc399, long_spc399, 0xcb0bfa11 },
{ 0xacebad03, short_spc400, long_spc400, 0xcb0bfa11 },
{ 0xacebad03, short_spc401, long_spc401, 0xcb0bfa11 },
{ 0xacebad03, short_spc402, long_spc402, 0xcb0bfa11 },
{ 0xacebad03, short_spc403, long_spc403, 0xcb0bfa11 },
{ 0xacebad03, short_spc404, long_spc404, 0xcb0bfa11 },
{ 0xacebad03, short_spc405, long_spc405, 0xcb0bfa11 },
{ 0xacebad03, short_spc406, long_spc406, 0xcb0bfa11 },
{ 0xacebad03, short_spc407, long_spc407, 0xcb0bfa11 },
{ 0xacebad03, short_spc408, long_spc408, 0xcb0bfa11 },
{ 0xacebad03, short_spc409, long_spc409, 0xcb0bfa11 },
{ 0xacebad03, short_spc410, long_spc410, 0xcb0bfa11 },
{ 0xacebad03, short_spc411, long_spc411, 0xcb0bfa11 },
{ 0xacebad03, short_spc412, long_spc412, 0xcb0bfa11 },
{ 0xacebad03, short_spc413, long_spc413, 0xcb0bfa11 },
{ 0xacebad03, short_spc414, long_spc414, 0xcb0bfa11 },
{ 0xacebad03, short_spc415, long_spc415, 0xcb0bfa11 },
{ 0xacebad03, short_spc416, long_spc416, 0xcb0bfa11 },
{ 0xacebad03, short_spc417, long_spc417, 0xcb0bfa11 },
{ 0xacebad03, short_spc418, long_spc418, 0xcb0bfa11 },
{ 0xacebad03, short_spc419, long_spc419, 0xcb0bfa11 },
{ 0xacebad03, short_spc420, long_spc420, 0xcb0bfa11 },
{ 0xacebad03, short_spc421, long_spc421, 0xcb0bfa11 },
{ 0xacebad03, short_spc422, long_spc422, 0xcb0bfa11 },
{ 0xacebad03, short_spc423, long_spc423, 0xcb0bfa11 },
{ 0xacebad03, short_spc424, long_spc424, 0xcb0bfa11 },
{ 0xacebad03, short_spc425, long_spc425, 0xcb0bfa11 },
{ 0xacebad03, short_spc426, long_spc426, 0xcb0bfa11 }
};
#endif

#if MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG_NONE
#define MPIR_MAX_ERROR_CLASS_INDEX 54
static int class_to_index[] = {
641,42,113,126,642,104,548,564,164,504,
645,123,9,651,647,509,197,196,195,557,
138,139,145,112,120,143,144,137,174,175,
181,180,202,483,8,499,149,148,534,151,
152,567,635,121,150,654,27,236,230,560,
563,562,561,19};
#endif
