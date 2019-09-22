# rcom
rcom is light-weight libary for inter-node communication 


All apps run as separate processes. They communicate with each other using one or more of the communication links discussed below. 

One specific app, rcregistry, maintains the list of all running apps and updates the connections when the apps start and stop. The rcregistry app should be launched before any other app. 


There are four type of communication between nodes:
* short real-time messages (datahubs and datalinks)
* buffered, non-realtime messages (messagehubs and messagelinks)
* one-shot, request-response exchange (services and clients)
* continuous (video) streams (streamer and streamer link)

Data messages are implemented on top of UDP and are therefore limited in size (1.5kB). Messagehub use WebSockets to pass data. Services use classical HTTP requests. Streamers use HTTP request and return the data using the multipart/x-mixed-replace format.

In addition, to those four basic link types, the rcgen utility (discussed below) recognizes the "controller" type. A controller is a messagehub that expects a command/response interaction.

The rcom library does not have a "bus" type of communication but it can be built quite easily using a messagehub. 

Although the connections are agnostic about the content of the messages most messages are encoded in JSON.  




There are several utilities:
* rcregistry: 
* rcom: The application offers a number of utility functions, inluding querying the registry, listening to the message of nodes.
* rclaunch: 
* rcgen: takes a description of an app as input and generates C code that provides a skeleton for the app.


## rcgen

  $ rcgen code output-file [input-file]

  $ rcgen cmakelists [output-file] [input-file]


|   |   |   |
|---|---|---|
|name|required| - |Base name for the code generator|
|init|optionnal|\<name\>_init|Name of the init function|
|cleanup|optionnal|\<name\>_init|Name of the cleanup function|
|header|optionnal|\<name\>.h|Name of the cleanup function|




