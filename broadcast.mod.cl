#define NO 0
#define YES 1

#define REG_STATE 0

#define STATE_INIT 1
#define STATE_IDLE 0
#define STATE_DONE -1

#define MESSAGE_BRD 0

__kernel void broadcast(	__global int *links,
				__global int *states,
				__global int *messages_in,
				__global int *messages_out,
				const unsigned int messtypes,
				const unsigned int registers,
				const unsigned int nodes)
{
	//Get our global thread ID            
	int nid = get_global_id(0);

	// Counters
	int i,j;

	//Make sure we do not go out of bounds
	if (nid < nodes) {
                                          
		//Start the entity work               
		int mystate = states[nid*registers+REG_STATE];

		// I am the initiator
		if (mystate == STATE_INIT) {

			// Check if there is the spoutaneuos impulse (ie message from self)
			if (messages_in[(nid*nodes+nid)*messtypes+MESSAGE_BRD] == YES) {

				// Send the broadcast to the others			
				for (i=0;i<nodes;i++) {
					if ((i!=nid)&&(links[nid*nodes+i] == YES)) {
						messages_out[(nid*nodes+i)*messtypes+MESSAGE_BRD]=YES;
					}
				}

				states[nid*registers+REG_STATE]=STATE_DONE;
			}

			// The other messages are not important since the initator action will be null so remove them

		} else if (mystate == STATE_IDLE) {

			// Check for messages
			int messck=0;
			for (i=0;i<nodes;i++) {
				if ((i!=nid)&&(messages_in[(i*nodes+nid)*messtypes+MESSAGE_BRD] == YES)) {
					messck=1;
				}
			}

			if (messck==1) {
				for (i=0;i<nodes;i++) {
					if (links[nid*nodes+i] == YES) {
						messages_out[(nid*nodes+i)*messtypes+MESSAGE_BRD]=YES;
					}
				}
				states[nid*registers+REG_STATE]=STATE_DONE;
			}
		}

		for (i=0;i<nodes;i++) {
			if (messages_in[(i*nodes+nid)*messtypes+MESSAGE_BRD] == YES) {
				messages_in[(i*nodes+nid)*messtypes+MESSAGE_BRD]=NO;
			}
		}
	}
}

