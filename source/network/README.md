# network with the magic of async  

## Notes and design thoughts for and from the developer  
So generally Capn'Proto really is successfull in doing the stuff it wants to do, but I'd rather have a one simpler and more restricted implementation. I really don't need any functor for Promise::then(). std::function is enough for my use cases and I can be more sure that my implementation is correct.  

Stuff like attachment management is really neat though.  

When a promise endpoint is ready ( e.g. from EventPoll ) it should resolve that chain.
Plus why do I event really need events?  
I think it's for interfacing with EventLoop tbh, but I am not too sure.  

So a promise holds a PromiseNode which in turn can have either another PromiseNode or nothing at all.
This PromiseNode can be fulfilled, broken or still waiting.  

Personally I would use PromiseFulfiller for fulfillment of promises in general and keep the Promise as the endpoint.  

Each PromiseNode needs to know its owning PromiseNode.  
