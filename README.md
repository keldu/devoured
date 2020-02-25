# devoured  
## not working - check feature table below  

A wrapper around bad binaries, which are not handling SIGTERM properly, are crashing when stdin is closed or don't have a proper management console. My motivation are some bad binaries which make administration a little bit difficult with some strange behaviour.  

It could be also used as a general supervisor and maybe as a test suite for submitted programming exercises, because you can extract output and inject input.  
Test suite would need more supervisor features. Mainly some kind of possibility to create services through the daemon control socket instead of config files.  

This will be a service which accepts custom commands like this one  
Sending single commands:  
`devoured -t $target -c "motd This is the message of the day"`  
Send an alias command  
`devoured -t $target -a "motd_one"`  
Checking the status.  
`devoured -s` or `devoured --status`  
for an interactive shell.  
`devoured`, `devoured -i` or `devoured --interactive`  
Starting the daemon with.  
`devoured -d`  
The scheme how to start the services may be based on something like  
`devoured -m "start" -t $target`  

Currently not working, but the general layout is done. Just have to write a proper management socket for devoured.  

### List of features and their status for basic functionality  

| Feature	| Status	|
|-----------|-----------|
| Network	| :heavy_check_mark: |
| Forking	| :heavy_check_mark: |
| Daemon	| :heavy_check_mark: |
| Status	| :heavy_check_mark: |
| Manage	|			|
| Service Configuration |		|
| Command	|			|
| Alias		|			|
| Interactive |			|
