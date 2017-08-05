/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
	/*
	 * This function is partially implemented and may require changes
	 */
	//int id = *(int*)(&memberNode->addr.addr);
	//int port = *(short*)(&memberNode->addr.addr[4]);

	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
    // node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1;
    initMemberListTable(memberNode);

    return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	MessageHdr *msg;
#ifdef DEBUGLOG
    static char s[1024];
#endif

    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
        // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
#endif
        memberNode->inGroup = true;
    }
    else {
        size_t msgsize = sizeof(MessageHdr) + sizeof(joinaddr->addr) + sizeof(long) + 1;
        msg = (MessageHdr *) malloc(msgsize * sizeof(char));

        // create JOINREQ message: format of data is {struct Address myaddr}
        msg->msgType = JOINREQ;
        memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));
        memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, msgsize);

        free(msg);
    }

    return 1;

}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode(){
   /*
    * Your code goes here
    */
	return 0;
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
    	return;
    }

    // Check my messages
    checkMessages();

    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
    	return;
    }

    // ...then jump in and share your responsibilites!
    nodeLoopOps();

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 *
 * Inputs ; env - pointer to a member node
 * 			data - pointer to the message entry
 *			size - Size of message
 *
 * Return Value : True if
 * 				  False if
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
	 //Local Variables
	 MsgTypes msg_type;
	 //Get the type of message & forward to right function
	 msg_type = ((MessageHdr *) data)->msgType;

	 //update timestamp on receive
	 memberNode->memberList[0].heartbeat = ++memberNode->heartbeat;
	 memberNode->memberList[0].timestamp = par->getcurrtime();

	 //Forward to proper message handler
	 switch (msg_type){

		 case JOINREQ :
		 	handle_request (data, size);
			//update personal heartbeat
			memberNode->heartbeat ++;
			return true;
		 case JOINREP :
		 	handle_reply (data, size);
			//update personal heartbeat
			memberNode->heartbeat ++;
			return true;
		 case GOSSIP :
		 	handle_gossip_in (data, size);
			//update personal heartbeat
			memberNode->heartbeat ++;
			return true;
		 default :
		  	return false;
	 }
	 return false;

}

 /**
  * FUNCTION NAME: nodeLoopOps
  *
  * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
  * 				the nodes
  *
  *
  * Inputs ; non- uses object that calls it
  *
  *
  * Return Value : none
  *
  *
  * Functionality - Checks if any nodes have failed & then gossips
  *
  */
void MP1Node::nodeLoopOps() {
	//Local variables
	size_t i;
	long cur_time, node_time ;
	Address removed_addr;
	vector<MemberListEntry>::iterator it = memberNode->memberList.begin();

	//print membership list

	//for each entry : Check timestamp & change heartbeat to invalid if necessary
	for (i=0; i<memberNode->memberList.size(); i++, it++){
		cur_time = par->getcurrtime();
		node_time = memberNode->memberList[i].timestamp;
		//Check timestamp
		if ( (cur_time- node_time)  > TREMOVE){
			//delet node & log its removal
			//get address from entry
			memcpy (&removed_addr.addr, &memberNode->memberList[i].id, sizeof(int));
			memcpy (&removed_addr.addr[4], &memberNode->memberList[i].port, sizeof(short));
			//log the removal
			#ifdef DEBUGLOG
			        log->logNodeRemove(&memberNode->addr, &removed_addr);
			#endif
			//delete entry
			memberNode->memberList.erase(it);
			//set i & iterator back to get next node
			i--;
			it--;
		}
		else if( (cur_time-node_time) >TFAIL){
			//mark node as failed
			memberNode->memberList[i].heartbeat =-1;
		}
	}
	//print membership list
	printML();
	//gossip th membership list
	gossip_out();
    return;
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialises the membership table
 *
 * Inputs ; memberNode - a pointer to the current member node
 *
 * Return Value : nothing
 *
 * Functionality - Updates membership list for a new heartbeat
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
	//add first value (self) to the membership list
	MemberListEntry my_entry;
	//convert address|port|heartbeat to membership entry
	addr2Entry(my_entry, memberNode->addr, memberNode->heartbeat,
			   par->getcurrtime());
	//store entry into list
	memberNode->memberList.push_back(my_entry);
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;
}




//////////////////////////// MESSAGE HANDLERS //////////////////////////////////
/**
 * FUNCTION NAME: handle_request
 *
 * DESCRIPTION: Handles a join request message
 *
 * Inputs : data - a pointer to the incoming message
 *			size - size of the incoming message
 *
 * Return Value : nothing
 */
void MP1Node:: handle_request (char* data, int size){
	//local variables
	int id;
	short port;
	long heartbeat;
	Address in_addr;
	MemberListEntry join_entry;

	//get details from message
	msg2var (data+sizeof(MessageHdr), id, port, heartbeat);
	//get address
	memcpy (&in_addr.addr, &id, sizeof(int));
	memcpy (&in_addr.addr[4], &port, sizeof(short));
	//conver to entry
	addr2Entry (join_entry, in_addr, heartbeat, par->getcurrtime());
	//store in membership list
	memberNode->memberList.push_back(join_entry);

	//send out reply with membership list to join node
	send_list (&in_addr, memberNode->memberList.size(), JOINREP);
	//update timestamp on send
	memberNode->memberList[0].heartbeat = ++memberNode->heartbeat;
	memberNode->memberList[0].timestamp = par->getcurrtime();
	//log the addition of a new node
	#ifdef DEBUGLOG
	        log->logNodeAdd(&memberNode->addr, &in_addr);
	#endif
}

/**
 * FUNCTION NAME: handle_reply
 *
 * DESCRIPTION: Handles a reply message from intoducer
 *
 * Inputs : data - a pointer to the incoming message
 *			size - size of the incoming message
 *
 * Return Value : nothing
 */
void MP1Node:: handle_reply  (char* data, int size){
	//Local Variables
	int id;
	size_t j;
	short port;
	long heartbeat, real_size, entries, entry_size, i;
	Address in_addr;
	MemberListEntry new_entry;
	char* entry_ptr = data + sizeof(MessageHdr);
	vector <MemberListEntry> ::iterator it;

	//add yourself to group
	memberNode->inGroup = true;

	//  |header|entry|entry|entry...
	//find the number of entries sent
	//Get the size without the header
	real_size = size - sizeof(MessageHdr);
	// entry = |Address|1|heartbeat
	entry_size = sizeof(Address) + 1 + sizeof (long);
	entries = real_size / entry_size;

	printAddress(&memberNode->addr);
	cout<< "Got a reply message with "<<entries<< " entries"<<endl;

	//For each entry sent
	for (i=0; i<entries-1; i++){
		//Convert to Variables
		msg2var(entry_ptr + (entry_size*i), id, port, heartbeat);
		//get address
		memcpy (&in_addr.addr, &id, sizeof(int));
		memcpy (&in_addr.addr[4], &port, sizeof(short));
		//Convert to Membership entry
		addr2Entry(new_entry, in_addr, heartbeat, par->getcurrtime());
		//push into membership list if not inside
		for (j =0; j<memberNode->memberList.size(); j++){
			if (memberNode->memberList[j].id == id){
				//update entry
				break;
			}

		}
		if (j ==memberNode->memberList.size())
			memberNode->memberList.push_back(new_entry);
	}

	//print out new membership list
	//printML();

}

/**
 * FUNCTION NAME: handle_gossip_in
 *
 * DESCRIPTION: Handles a received gossip message
 *
 * Inputs : data - a pointer to the incoming message
 *			size - size of the incoming message
 *
 * Return Value : nothing
 */
void MP1Node:: handle_gossip_in  (char* data, int size){
	//Local Variables
	size_t i;
	int id;
	short port;
	long entry_no, entry_size, entries, heartbeat, real_size;
	char * msg_ptr;
	MemberListEntry new_member;

	//find the number of entries
	real_size = size - sizeof(MessageHdr);
	entry_size = sizeof(Address) + 1 + sizeof(long);
	entries = real_size/entry_size;

	//msg_ptr = (char*) malloc (entry_size*entries);

	// printAddress(&memberNode->addr);
	// cout<< "Got a gossip with "<<entries<< " entries. "<< real_size<<"real_size. "
	// <<entry_size<<"entry size"<<endl;
	cout<< "Gossip in"<<endl;


	//get the start of the entry
	msg_ptr = data + sizeof(MessageHdr);

	//For each of the entries
	for (entry_no =0; entry_no<entries; entry_no++){
		//extract content fomr the entry
		msg2var(msg_ptr + (entry_size*entry_no), id, port, heartbeat);
		//find the MLE to update
		//go through membership list and chech for target id
		for (i=0; i<memberNode->memberList.size(); i++){
			if (memberNode->memberList[i].id == id)
				break;
		}
		//if entry does not exist
		if (i== memberNode->memberList.size()){
			//add entry to the list if node is alive
			if (heartbeat>=0){
				new_member = MemberListEntry(id, port, heartbeat, par->getcurrtime());
				memberNode->memberList.push_back(new_member);
			}
		}
		else{
			//check if heartbeat is newer
			if ( (memberNode->memberList[i].heartbeat<heartbeat) && (memberNode->memberList[i].heartbeat>=0)    ){
				//update heartbeat & timestamp
				memberNode->memberList[i].heartbeat = heartbeat;
				memberNode->memberList[i].timestamp = par->getcurrtime();

			}
		}

	}
}


////////////////////////////////////////////////////////////////////////////////





//////////////////////////// GOSSIP FUNCITONS //////////////////////////////////
/**
 * FUNCTION NAME: send_list
 *
 * DESCRIPTION: Sends this nodes membership ist to the requested node
 *
 * Inputs : to_addr - address to send list to
 *			no - no of entries to send (-1 means all)
 *
 * Return Value : nothing
 */
void MP1Node:: send_list (Address * to_addr, int no, MsgTypes type){
	//Local Variables
	long i, entries, msg_size, entry_size;
	char* message, *msg_ptr;
	MemberListEntry chosen_entry;
	Address entry_addr;



	//get total message size
	entries =memberNode->memberList.size();
	msg_size = sizeof(MessageHdr) + (entries * (sizeof(Address) + 1 + sizeof (long)));
	entry_size = sizeof(Address) + 1 + sizeof (long);
	//allocate space for the message on the heap
	message =  (char*) malloc (msg_size);

	//add header
	memcpy (message, (char*)&type , sizeof(MessageHdr));
	msg_ptr = message +sizeof(MessageHdr);

	//Assemble message
	for (i =0; i<entries && i < no ; i++){
		//Get Membership entry
		chosen_entry = memberNode->memberList[i];

		//if entry is invalid, skep
		if (chosen_entry.timestamp ==-1){
			msg_ptr += entry_size;
			continue;
		}
		//Get address from entry
		memcpy (&entry_addr.addr, &chosen_entry.id, sizeof(int));
		memcpy (&entry_addr.addr[4], &chosen_entry.port, sizeof(short));

		//put information at the back of the entry : |Address|1|Heartbeat
		memcpy (msg_ptr, &entry_addr, sizeof(Address));
		memcpy (msg_ptr + 1 + sizeof(Address), &chosen_entry.heartbeat, sizeof(long));

		//update pointer to next message location
		msg_ptr += entry_size;
	}

	//send message
	emulNet->ENsend(&memberNode->addr, to_addr, message, msg_size);

	//update own heartbeat
	memberNode->memberList[0].heartbeat = ++memberNode->heartbeat;

	//delete any variables
	delete message;
}


/**
 * FUNCTION NAME: gossip_out
 *
 * DESCRIPTION: sends the membership list to b nodes
 *
 * Inputs : none
 *
 * Return Value : nothing
 */
void MP1Node:: gossip_out (){
	//Local Variables
	long b, list_size, no;
	long sent_gossips=0;
	MemberListEntry chosen_entry;
	Address out_addr;
	vector <int> store;   //push no to the back when used
	vector <int> ::iterator it;


	list_size =memberNode->memberList.size();
	b =((list_size-1) /2) +1;;
	//Get b random membership list entries and send to them
	while (sent_gossips<b){
		//check numbers
		// if ((list_size<3) && (sent_gossips>0))
		// 	break;
		//Get a random entry number from 1-(size-1)
		no = rand() % (list_size-1);
		no++;
		// 0 1 2   size = 3  rand%2
		it = find (store.begin(), store.end(), no);
		if(it != store.end() && (!store.empty())){
			//already sent to that node
			continue;
		}

		//push to the back of store
		store.push_back(no);
		//Get the address from membership list
		chosen_entry = memberNode->memberList[no];
		memcpy (&out_addr.addr, &chosen_entry.id, sizeof(int));
		memcpy (&out_addr.addr[4], &chosen_entry.port, sizeof(short));

		//send the membership list to chosen address at entry
		if (chosen_entry.heartbeat >=0)
			send_list(&out_addr, list_size, GOSSIP) ;
		sent_gossips++;
	}
	// printAddress(&memberNode->addr);
	// cout<< " Sent out "<< sssent_gossips<< " entries"<<endl;

	//printML();

	//update timestamp on send
	memberNode->memberList[0].heartbeat = ++memberNode->heartbeat;
	memberNode->memberList[0].timestamp = par->getcurrtime();

}






////////////////////////////////////////////////////////////////////////////////





////////////////////////////// CONVERSION FUNCTIONS ////////////////////////////
/**
 * FUNCTION NAME: addr2Entry
 *
 * DESCRIPTION: Converts and address to a membership entry
 *
 * Inputs : entry- the membership entry to store into
 *			addr - the addr to get port and addr from
 *			heartbeat - heartbeat of entry
 *			local_time - local time entry was updated
 *
 * Return Value : nothing
 */
void MP1Node:: addr2Entry(MemberListEntry& entry, Address addr, long heartbeat,
	long local_time){
		//Local variables
		int id;
		short port;

		//Get port from address
		memcpy (&id, &addr.addr, sizeof(int));
		memcpy (&port, &addr.addr[4], sizeof(short));

		//Put in the entry
		entry.id = id;
		entry.port = port;
		entry.heartbeat = heartbeat;
		entry.timestamp = local_time;
}

/**
 * FUNCTION NAME: msg2var
 *
 * DESCRIPTION: Extracts useful information from a message
 *
 * Inputs : data - the messae pointer
 *			id - address found in the message
 *			port - port of entry
 *			heartbeat - heartbeat of entry
 *
 * Return Value : nothing
 */
void MP1Node:: msg2var(char* addr_ptr, int& id, short& port, long& heartbeat){
	//Local variables
	char * heartbeat_ptr;

	//Find start of heartbeat
	heartbeat_ptr = addr_ptr + sizeof(Address) +1;

	//Copy variables from message
	memcpy (&id, addr_ptr, sizeof(int));
	memcpy (&port, &addr_ptr[4], sizeof(short));
	memcpy (&heartbeat, heartbeat_ptr, sizeof(long));
}

////////////////////////////////////////////////////////////////////////////////


//Function that prints a node's membership list
void MP1Node::printML(){
	//local variables
	vector<MemberListEntry>::iterator it;

	cout<< "PRINTING MEMBERSHIP LIST FOR :"; printAddress(&memberNode->addr);
	cout<<" AT TIME: "<<par->getcurrtime()<<endl;

	//iterate through membership list and print: ID|PORT|HEARTBEAT|TIMESTAMP
	for (it = memberNode->memberList.begin(); it!=memberNode->memberList.end(); it++){
		//print values
		cout<< it->id<< "  |  "<<it->port<< "  |  "<<it->heartbeat<<"  |  "<<it->timestamp<<endl;
	}
}
