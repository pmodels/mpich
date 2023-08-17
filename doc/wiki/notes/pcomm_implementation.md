# Partitioned Communication Implementation notes


## Overarching ideas

Both the sender and the receiver have to agree on a number of messages that will be sent (named `msg_part`), which comes from algorithmic choices.
The sender sends the RTS with the partition information and then the receiver decides on the number of msgs and reply to the sender with the CTS.
Then multiple iterations of `start`-`pready`/`parrived`-`wait` can happen in a row.
Every iteration must start with a CTS send/reception, which guarantees that data will not been sent too soon or too late.

To track progress we have three main counters:
- `cc` the completion counter of the request. It dictates completion for the user (i.e. `MPI_Wait` completion). The value is set during the `MPI_Start` (or at reception of the CTS) and decreased during the callback of the child requests.
- `cc_part` the completion counter per partition, keeps track of the different status of the partitions
- `cc_send` for the sender only, the counter of the number of send requests initiated (which allows us to reset some values before starting the last send request).

## software path

### Request creation

**sender**: the creation does the following operations:
- set `msg_part` to `-1`
- set `cc_send` to `0`
- allocate the `cc_part` array and set the initial value to all the counters
- send the RTS

**receiver**: the creation on the receive side is different as most of the needed information have to come from the RTS. 
First we try to see if we have received an unexpected request.
If so then we have already some of the RTS information and we save them: the total datasize `dsize`, the number of msgs `msg_part`, and the pointer to the sender's request `peer_req_ptr`.
Then we set the status of the request and allocate the `cc_part` array in `MPIDIG_part_rreq_matched`.

If no unexpected request has been received then we simply queue one request that will be filled at reception of the RTS.

### Start
The whole start function happens within a `lock` region on the VCI `0`.

**sender**: on the sender side the start operation activates the request and then sets the `cc` value to `max(1,msg_part)`.
If we haven't received the CTS, `msg_part=-1`, if we have received it then we get the correct value. 

**receiver**: for the receiver we set the `cc_value` to `1` as a temporary choice to avoid completion.
Then if the request has been matched (i.e. RTS has been received) we
- reset the `cc_part` values
- set the value of `cc` to `1`
- issue the CTS

### Callbacks
the callbacks execution happens within a locked region (**TODO: check that!!**)

#### Sender side - Clear To Send message (CTS)

The role of the CTS is to synchronize both the sender and the receiver when performing multiple iterations.
It avoids the situation when the sender sends messages from the next iteration while the receiver is still busy with the previous iteation. 
Also the CTS carries the information of the number of messages which is blocking to perform the first iteration.

The reception of the first CTS on the sender side performs the following steps:
- change `peer_req_ptr` from null to the address of the receiver's request
- assign the value to `msg_part` which contains the number of partitions to actually be used at communication time
- if the request is already active, we set the `cc` value to the number of msgs because we didn't have that information at `start` time.

For every CTS received we do the following operations:
- set `cc_send` from `0` to `msg_part` which tracks the number of messages to be send. the value `0` is obtained either from the completion of the previous iteration or from the initialization.
- decrease the value of `cc_part` to notify reception of the CTS
- if the request is started then issue the data for the partitions that are ready

#### Sender side - issue the data
When we issue the data we also have to reset anything that will be used upon reception of the next CTS such as the completion values `cc_part`.
The reason is that the send of data is asynchronous and while those complete we might receive the next CTS (if the reception has been completed on the receive side before the send completes).

Because we do not assume that one message actually send will only affect one partition (in the general case we might have one partition sent by multiple messages), we cannot reset the value in a "per-message" basis.
The only choice is to do it when we issue the last message. 
At that time, all the partitions are marked ready and the value of `cc_send` is `0`. Which indicates that no further message will be send.
We then reset the value of `cc_part` so that it's ready to be used upon reception of the CTS.

#### Receiver side - reception of data

