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
Starting the service.  
`devoured -d -f config.toml -t terraria`  
would run the service with the specified config file.  

Currently not working, but the general layout is done. Just have to write a proper management socket for devoured.  
