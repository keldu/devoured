# devoured
A wrapper around bad binaries, which are not handling SIGTERM properly, are crashing when stdin is closed or don't have a proper management console. My motivation is Terraria which makes makes administration a little bit difficult with some strange behaviour.  

This will be a service which accepts custom commands like this one  
Sending single commands:  
`devoured -t terraria -c "motd This is the message of the day"`  
Send an alias command  
`devoured -t terraria -a "motd_one"`  
Checking the status.  
`devoured -s` or `devoured --status`  
for an interactive shell.  
`devoured`, `devoured -i` or `devoured --interactive`  
Starting the daemon with.  
`devoured -d`  
The scheme how to start the services may be based on something like  
`devoured -m "start" -t terraria`  

Currently not working, but the general layout is done. Just have to write a proper management socket for devoured.  

### List of features and their status  

| Feature	| Status	|
|-----------|-----------|
| Network	| :heavy_check_mark: |
| Forking	| :heavy_check_mark: |
| Daemon	| :heavy_check_mark: |
| Status	|			|
| Spawn		|			|
| Service Configuration |		|
| Command	|			|
| Alias		|			|
| Interactive |			|
