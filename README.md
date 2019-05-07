# GRASS

Baby Hackers - Software security class project 2019
 
 ```
    ____  __                            __           _  __     __               __  
   / __ \/ /__  ____ _________     ____/ /___  ____ ( )/ /_   / /_  ____ ______/ /__
  / /_/ / / _ \/ __ `/ ___/ _ \   / __  / __ \/ __ \|// __/  / __ \/ __ `/ ___/ //_/
 / ____/ /  __/ /_/ (__  )  __/  / /_/ / /_/ / / / / / /_   / / / / /_/ / /__/ ,<   
/_/   /_/\___/\__,_/____/\___/   \__,_/\____/_/ /_/  \__/  /_/ /_/\__,_/\___/_/|_|  
                                                                                    
```     

# How to run?
(just don't)

`make` in the project-code directory

`./bin/server/server` to exec server

`./bin/client/client 127.0.0.1 1337` to execute client


# Our approach
In order to prevent any direct access to the OS, we decided to implement GRASS through an interposition layer:
* From security perspectives: less syscalls, binaries can't be executed, etc.
* From privacy perspectives: more difficult to access other clients folders thanks to compartmentalization

In practive, we used recursive structures to implement a folder architecture for each client. Each client has its own folder architecture, enabling navigation through directories using list of pointers and other innovative structures. 

However, when practice meets theory things have a tendency to get awfully complicated. For instance, reimplementing the grep command using our architecture appears is a project in itself and we circumvented it in a... Stylish way.

### Team
[Hugo Roussel](https://github.com/hugoroussel) / [Marion Le Tilly](https://github.com/rionma) / [Zoé Baraschi](https://github.com/baraschi)
