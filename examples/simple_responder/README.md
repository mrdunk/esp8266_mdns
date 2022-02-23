# simple_responder
This example is a very simple MDNS responder. It listens for queries for the local hostname (test.local in this case) and send an answer packet with the local IP address.
When sending the answer, it also checks to see if the querier has requested a unicast answer. If so, it gets the querier IP address from the getRemoteIP() call and then uses SendUnicast() to send the reply.