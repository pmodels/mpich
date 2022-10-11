# Design Documentation

- Architecture
	- [Atomic Operations](Atomic_Operations.md)
	- [Checkpointing Implementation](Checkpointing_implementation.md)
	- [Checkpointing Notes](Checkpointing.md)
	- [Communicators and Context IDs](Communicators_and_Context_IDs.md)
	- [DAME](DAME.md)
	- [DTPools](DTPools.md)
	- [Fault Tolerance](Fault_Tolerance.md)
	- [Function Name Prefix Convention](Function_Name_Prefix_Convention.md)
	- [Include Files in MPICH](Include_Files_in_MPICH.md)
	- [Internal Instrumentation](Internal_Instrumentation.md)
	- [KVS Caching](KVS_Caching.md)
	- [Local and Global MPI Process IDs](Local_and_Global_MPI_Process_IDs.md)
	- [MPICH Architecture Overview](MPICH_Architecture.md)
	- [Parameters in MPICH](Parameters_in_MPICH.md)
	- [Process Groups and Virtual Connections](Process_Groups_and_Virtual_Connections.md)
	- [Processing MPI Datatypes](Processing_MPI_Datatypes.md)
	- [Reference Counting](Reference_Counting.md)
	- [Reporting and Returning Error Codes](Reporting_And_Returning_Error_Codes.md)
	- [Sharing Blocking Resources](Sharing_Blocking_Resources.md)
	- [The Progress Engine](The_Progress_Engine.md)

- CH4
    - [Overall Design](CH4_Overall_Design.md)
    - [Process Address Translation](CH4_Process_Address_Translation.md)

- CH3
	- [Channels](CH3_And_Channels.md)
	- [VC Protocol](CH3_VC_protocol.md)
        - [Object Model](CH3_Object_Model.md)
        - [Netmod Packet Types](CH3_Netmod_Packet_Types.md)
	- [Changes and ADI3](Changes_to_CH3_and_ADI3.md)

- Collectives
	- [Collective Selection Framework](Collective_Selection_Framework.md)
	- [Collectives Framework](Collectives_framework.md)
	- [Overriding Collective Functions](Overriding_Collective_Functions.md)

- Debugging
	- [Debug Event Logging](Debug_Event_Logging.md)
	- [Debugger Message Queue Access](Debugger_Message_Queue_Access.md)
	- [Message Queue Debugging](Message_Queue_Debugging.md)
	- [Support for Debugging Memory Allocation](Support_for_Debugging_Memory_Allocation.md)

- Hydra
    - [Hydra Process Management Framework](Hydra_Process_Management_Framework.md)

- Misc
	- [Proposed MPIEXEC Extensions](Proposed_MPIEXEC_Extensions.md)
	- [MPICH Parameters and Instrumentation Tool Interface](Tool_Interfaces.md)
	- [Best Practices](Best_Practices_MS.md)

- Nemesis
	- [Dynamic Processes and Connections](Nemesis_Dynamic_Processes_and_Connections.md)
	- [Network Module API](Nemesis_Network_Module_API.md)
	- [Runtime Netmod Selection](Nemesis_Runtime_Netmod_Selection.md)
	- [TCP Netmod State Machine](Nemesis_tcp_netmod_state_machine.md)

- PMI
    - [Process-Management Interface] (PMI.md)
    - PMI-v2 notes (outdated)
	- [Specification](PMI-2_Specification.md)
	- [API](PMI_v2_API.md)
	- [Design Thoughts](PMI_v2_Design_Thoughts.md)
	- [Wire Protocol](PMI_v2_Wire_Protocol.md)

- RMA
	- [ADI Interface](ADI_RMA_Interface.md)
	- [New Dessign](New_RMA_Design.md)
	- [Implementation](RMA_Implementation.md)
	- [Old RMA Design](RMA_Design.md)

- Sockets
	- [Establishing Socket Connections](Establishing_Socket_Connections.md)
	- [Socket Connection Protocol](Sock_conn_protocol.md)
	- [Socket Thread Safety Mechanism](Sock_Thread_Safety_Mechanism.md)

- Threads
	- [Making MPICH Thread Safe](Making_MPICH_Thread_Safe.md)
	- [Thread Safety Chart](Thread_Safety.md)
	- [Thread Private Storage](Thread_Private_Storage.md)
