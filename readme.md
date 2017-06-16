#Token based hybrid network, Work in progress
The idea here is to build a protocol that can work over RS485 and any bus in general.
The network is set out to allow the following:
* Any node can broadcast information to all the other nodes directly.
* Define automatically a Master, or not have a Master at all.
##Definition of the protocol
To achieve this, I will take an approach that is a hybrid between Token ring and bus polling.
###Nodes
nodes are the pseudonym for the units that run a program that lets them participate in the bus with the protocol. Most probably, an MCU.
* Each node has a token input (TIP) pin and a token output (TOP) pin (later a single pin can fulfill both functions (TP)). Also a common bus pin named COM.
####States
* Each module has three states: Listening, Broadcasting and Connecting.
* On Listening state any normal node is on high impedance mode, reading the serial. On Listening state, the node is also reading the TIP pin. If TIP pin is set to 1, it switches to Broadcasting state.
* on Broadcasting state, the node sends all his available information if any, otherwise a "no information" header. Finishing this, it turns the TOP pin to 1, causing the following node in line to switch into Broadcasting state.
* when receiving any data, a node turns the TOP back to 0 if it was set to 1.
* every node has a unique ID, starting from 0. The node number 0 has a special role.
* When a node has just been powered up, it will be set to Connecting state. On connecting state, the node has no address, and has the TOP on state 0. It is listening to all the broadcasts, and keeps track of the highest address in the network. When it's own TIP goes high, it sets his own address to the highest + 1, writes a connected message to the bus, and sets his TOP to 1. This should cause a chain reaction where all the modules assign their addresses incrementally, if powered all at once.
* The TIP is pulled to the state 1, causing the first in line to start the chain reaction from 0. When a node is on Connecting mode, if it detects 1 on its TIP, and has not registered any address, it means that there is no lower module. It will set it's own address to 0. Maybe it is convenient if it waits about 10 bytes before doing so, in case the network is on some weird stuck state.
#### Token logic and the node 0
* Node number 0 has the role of originating a new token when the token has reached the last module in the token line. It reads the end of the token line when there is a message with disconnected header; or in other words, a timeout.
###Messages
* a message consists of the following components: <origin><header><payload>
* origin is a byte representing a unique identifier that indicates what module has broadcasted that message.
* header is a byte that indicates the mode of the upcoming data. It is divided in two nibbles: first nibble indicates the mode of the message (broadcasted, addressed, empty, offline). The second nibble indicates the length of the upcoming message in words. The maximum message length is 16*4 bytes. Maybe if the second nibble is F, the listeners will wait for a special termination byte.
* an offline header is equivalent to a disconnected module. The idea is that a module that is offline or not connected, will generate an offline header by not sending anything. You can also think of this as a timeout of the length of a single byte.

```
-----\         /---------------->
      \       /
    |-TI-----TO---|
    |    node     |
    |------COM----|
            |
------------|----------bus------->

```

