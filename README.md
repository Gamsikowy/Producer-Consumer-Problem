# Producer-Consumer-Problem
## Program description:
Implementation of the solution to the producer-consumer problem in a version that allows simultaneous work of many producers and many consumers. The exchange of information between the processes is carried out with the use of shared memory blocks. Individual producers and consumers are independent processes,
which can be activated at any time. Processing time is variable. For each producer, the number of products that are to be produced and added to the buffer is given during the start-up. After placing in
the number of products in the buffer, the manufacturer's process ends. Consumers remain active as long as there is at least one active producer in the system or as long as there are items ready to be processed in the buffer. Otherwise, they quit their job. In particular, starting the consumer before starting any
manufacturer causes its operation to end immediately.
## Compilation method:
We need the -lrt and -pthread libraries to activate the process. Programs should be activated in separate terminals or using the tmux utility. Example activation:
```
gcc producer.c -lrt -lpthread -o p.out
./p.out 5
```
```
gcc consumer.c -lrt -lpthread -o c.out
./c.out
```
