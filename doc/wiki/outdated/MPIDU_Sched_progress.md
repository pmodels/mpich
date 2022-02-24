In MPICH, non-blocking calls are attached to a list called "schedule",
which is defined in `src/mpid/common/sched/mpid_sched.c`.

    struct MPIDU_Sched_state {
        struct MPIDU_Sched *head;
    };

    /* holds on to all incomplete schedules on which progress should be made */
    struct MPIDU_Sched_state all_schedules = {NULL};

The function [MPIDU_Sched_progress](MPIDU_Sched_progress "wikilink")
will make progress on all schedules, and is called by various functions
like `MPI_Wait` or `MPI_Send` in a different name `MPID_Progress_wait`

    #define MPID_Progress_wait(progress_state_)  MPIDI_CH3_Progress_wait(progress_state_)

## Reference

  - [Making_MPICH_Thread_Safe\#The_Progress_Engine](Making_MPICH_Thread_Safe#The_Progress_Engine "wikilink")