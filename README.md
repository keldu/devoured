# devoured
A wrapper around the bad binaries, which are not handling SIGTERM properly, are crashing when stdin is closed or don't have a proper management console. My motivation is Terraria which makes makes administration a little bit difficult with some strange behaviour.  

This will be a service which accepts custom commands like this one  
`devoured -t terraria -c "motd This is the message of the day" `  
Currently not working, but the general layout is done. Just have to write a proper management socket for devoured.  
