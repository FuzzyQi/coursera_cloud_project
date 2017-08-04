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
		cout<< "FAILED NODE IS : "; printAddress(&memberNode->addr);
		cout<<endl;
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
	int id = *(int*)(&memberNode->addr.addr);
	int port = *(short*)(&memberNode->addr.addr[4]);

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
		//printAddress(&memberNode->addr);
		//cout<<"	heartbeat :"<< memberNode->heartbeat <<endl;
		//cout<< memberNode->addr.getAddress()<< " - "<< (size_t) memberNode->heartbeat<<endl;

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
	return 1;
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
		//print that a node has failed
		cout<< "FAILED NODE IS : "; printAddress(&memberNode->addr);
		cout<<endl;
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
 *
 * Return Value : True if
 * 				  False if
 *
 *
 * Functionality - Function Responds to each kind of message type
 *
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
	/*
	 * Your code goes here : Implemented Functions
	 */
	 //Local Variables
	 MsgTypes message_typ;
	 char addr[6] = {0};
	 long heartbeat;

	 // Message Format : Header - Address - heartbeat
	 //Get Message type
	 message_typ = ((MessageHdr *) data)->msgType;
	 //cout<< "Message type: "<< (MsgTypes)message_typ<<".	";


	 //copy over address
	 memcpy ((char*)addr, (char*)(data + sizeof(MessageHdr)), sizeof(Address));


	 //copy over heartbeat
	 memcpy ((char*)&heartbeat,  (char*)(data + sizeof(MessageHdr) + sizeof(Address) + 1), sizeof(long));
	 //cout<<"	with heartbeat of: "<<(long)heartbeat<<endl;


	 //increment heartbeat counter of current node
	 memberNode->heartbeat++;

	 //Forward to proper message handler
	 switch (message_typ){

		 case JOINREQ :
		 	handle_request ( addr, heartbeat);
			//update personal heartbeat
			memberNode->heartbeat ++;
			return true;
		 case JOINREP :
		 	handle_reply (addr, heartbeat);
			//update personal heartbeat
			memberNode->heartbeat ++;
			return true;
		 case HEARTBEAT :
		 	handle_heartbeat (addr, heartbeat);
			//update personal heartbeat
			memberNode->heartbeat ++;
			return true;
		 default :
		  	return false;
	 }
	 return false;
}


//////////////////////  HELPER FUNCTIONS FOR RECVCALLBACK   ////////////////////
/**
 * FUNCTION NAME: handler Functions
 *
 * DESCRIPTION: Message handler for request messages
 *
 * Inputs ; addr -pointer to the requester node's address
 *			heartbeat - heartbeat of the requester node
 *
 *
 * Return Value : True if successful, false otherwise
 *
 * Functionality - Processes request messages
 * 				   Adds the nodes address and heartbeat to the membership list
 *				   Sends the membership list to the requester node & other nodes(gossip)
 *
 */
bool MP1Node ::handle_request(char *addr, long heartbeat){

	//Local Variables
	int other_id, intr_id;
	short other_port, intr_port;
	//time_t curr_time = time(0);
	vector<MemberListEntry>::iterator other_pos;

	//Get the id and port from the address pointer
	// id is an int from addr[0]  |  port is a short from addr[4]
	memcpy(&other_id, &addr[0], sizeof(int));
	memcpy(&other_port, &addr[4], sizeof(short));


	//Add Introducer to the membership list if not already there
	if (memberNode->memberList.empty()){
		//get Introducer id and port
		memcpy(&intr_id, &memberNode->addr.addr[0], sizeof(int));
		memcpy(&intr_port, &memberNode->addr.addr[4], sizeof(short));
		//put in membership list
		memberNode ->memberList.push_back(MemberListEntry(
				intr_id, intr_port, memberNode->heartbeat, (long) par->getcurrtime() ));
		//update entry iterator
		memberNode->myPos =memberNode->memberList.begin();
	}

	//add node to the membership list
	//create membership entry
	MemberListEntry new_MLE = MemberListEntry(other_id, other_port, heartbeat, (long) par->getcurrtime() );
	//add entry to the end of the list
	memberNode ->memberList.push_back(new_MLE);

	//cout << "size is now : "<< memberNode->memberList.size()<< endl;
	//cout << "about to send..."<<endl;
	cout << "RECEIVED REQUEST MESSAGE  "<< " AT :"; printAddress(&memberNode->addr);
	cout<<" FROM :"; printAddress((Address*)addr); cout<<endl;
	//Put node add in the log
	log->logNodeAdd(&memberNode->addr,(Address*)addr);

	//send the memberhsip to the new node
	member_unicast(addr, JOINREP);

	//send a multicast to members in the list
	//gossip_multicast();



	return true;

}



/**
 * FUNCTION NAME: handle_reply
 *
 * DESCRIPTION: Handles a reply message
 *
 * Inputs ; addr - address of heartbeat
 *			heartbeat - heartbeat at id
 *
 *
 * Return Value : True if successful, false otherwise
 *
 *
 * Functionality - Adds and address to membership list
 *
 */
//Functions that handle the reply messages from the introducer node
bool MP1Node ::handle_reply(char *addr, long heartbeat){
	//Local Variables
	int other_id, own_id;
	short other_port, own_port;

	//Get the id and port from the address pointer
	// id is an int from addr[0]  |  port is a short from addr[4]
	memcpy(&other_id, &addr[0], sizeof(int));
	memcpy(&other_port, &addr[4], sizeof(short));


	//if not already part of the group, make part & add to back of list
	if (!memberNode->inGroup){   // <-----This can't work
		//cout << "EXECUTED..."<<endl;
		//add yourself to group
		memberNode->inGroup = true;
		//Initialize membership list table
		initMemberListTable (memberNode);

		//get own id and port
		memcpy(&own_id, &memberNode->addr.addr[0], sizeof(int));
		memcpy(&own_port, &memberNode->addr.addr[4], sizeof(short));

		//put in membership list
		memberNode ->memberList.push_back(MemberListEntry(
				own_id, own_port, memberNode->heartbeat, (long) par->getcurrtime() ));
		//update entry iterator
		memberNode->myPos =memberNode->memberList.begin();
	}


	//Check if his address is in the membership list
	//		Add to the back of the membership list
	//		Beware of Duplicate for own node
	if (memberNode->memberList[0].id != other_id){
		MemberListEntry new_entry(other_id, other_port, heartbeat, (long) par->getcurrtime());
		memberNode->memberList.push_back (new_entry);
	}

	//update heartbeat timer of self
	//memberNode->heartbeat++;
	memberNode->memberList[0].heartbeat = ++memberNode->heartbeat;
	memberNode->memberList[0].timestamp = par->getcurrtime();
	// cout << "RECEIVED A REPLY MESSAGE at: "; printAddress(&memberNode->addr);
	// cout <<" FOR:"; printAddress((Address*)addr);cout<<endl;

	return true;
}


/**
 * FUNCTION NAME: handle_heartbeat
 *
 * DESCRIPTION: Handles a heartbeat message
 *
 * Inputs ; addr - address of heartbeat
 *			heartbeat - heartbeat at id
 *
 *
 * Return Value : True if successful, false otherwise
 *
 *
 * Functionality - Updates membership list for a new heartbeat
 *
 */
//handles when we get a heartbeat message
bool MP1Node ::handle_heartbeat(char *addr, long heartbeat){
	//Local Variables
	int target_id;
	short target_port;
	size_t i, list_size;
	//bool found;
	long timestamp;
	MemberListEntry new_member;

	//get list size
	list_size = memberNode->memberList.size();

	//Translate to id and port nos
	memcpy(&target_id, &addr[0], sizeof(int));
	memcpy(&target_port, &addr[4], sizeof(short));

	//Find Corresponding id in membership list
	i=0;
	//found =false;
	//go through membership list and chech for target id
	for (i=0; i<list_size; i++){
		if (memberNode->memberList[i].id == target_id)
			break;
	}
	timestamp = (long) par->getcurrtime();


	//if id is not in list, add to membership list
	if (i==list_size){
		//Create membership list entry
		new_member = MemberListEntry(target_id, target_port, heartbeat, timestamp );
		memberNode->memberList.push_back(new_member);
	}
	else{
		//check heartbeat & add to list if necessary
		if (heartbeat > memberNode->memberList[i].heartbeat){
			//replace heartbeat & update local time
			memberNode->memberList[i].heartbeat = heartbeat;
			memberNode->memberList[i].timestamp = timestamp;
		}

	}

	// cout<<"UPDATED HEARTBEAT AT:"; printAddress(&memberNode->addr);
	// cout << " FOR ADDRESS "; printAddress((Address*)addr);
	// cout<<endl;

	return true;
}






/**
 * FUNCTION NAME: member_unicast
 *
 * DESCRIPTION: Sends a membership table to a single recipient
 *
 * Inputs ; to_addr - address to send to
 *			msg_type - type of message to send
 *
 *
 * Return Value : True if successful, false otherwise
 *
 *
 * Functionality - Updates membership list for a new heartbeat
 *
 */
void MP1Node ::member_unicast (char* to_addr, MsgTypes msg_type){
	//local variables
	size_t msg_size, sends=0, entries=0;
	void * msg_ptr = NULL;
	//iterator for membersihp list
	vector<MemberListEntry>::iterator it;
	vector<MemberListEntry>::reverse_iterator itr;
	MemberListEntry cur_entry;
	Address entry_address;
	char entry_addr[6] = {0};



	//get size of the message
	msg_size = sizeof(MessageHdr) + sizeof(memberNode->addr) + sizeof(long) +1;
	//msg_size = sizeof(MessageHdr) + sizeof(MemberListEntry);
	msg_ptr = (void*) malloc (msg_size);

	//cout << "size of MLE is : "<<sizeof(MemberListEntry);
	//choose fromt or back iterator
	if (rand()%2 ==0){
		//start from front
		//for each member in the list
		for (it =memberNode->memberList.begin(); (it !=memberNode->memberList.end())
		 				&& (sends <=(memberNode->memberList.size()/2) ); it++){
			//get first member of membership list
			//create message : header | Address | heartbeat
			switch (msg_type) {
				case JOINREP:
					((MessageHdr*)msg_ptr)->msgType = JOINREP;
					break;
				case HEARTBEAT:
					((MessageHdr*)msg_ptr)->msgType = HEARTBEAT;
					break;
				default : return;
			}
			//if the entry has expired, try another
			if ((par->getcurrtime()- it->heartbeat) > TFAIL)
				continue;

			//get membership entry
			cur_entry = *it;

			//reconstruct address from membership entry
			translate_MLE ((Address*)&entry_addr, &cur_entry);
			//memcpy (&entry_addr, &cur_entry.id, 4);
			//memcpy (&entry_addr[4], &cur_entry.port, 2);
			//put address into message
			void* offset = (void*) ((long)msg_ptr + (long)sizeof(MessageHdr));
			memcpy ((char*)(offset), &entry_addr, sizeof(Address));

			//put in heartbeat
			memcpy ((char*) ((long)offset + (long)sizeof(Address) +1), &cur_entry.heartbeat, sizeof(long));
			//send message
			emulNet->ENsend(&memberNode->addr, (Address*)to_addr, (char*) msg_ptr, msg_size);
			sends++;
		}
	}
	else {
		//start from the back
		//cout<< " SENDING FROM BACK"<<endl;
		//for each member in the list
		for (itr =memberNode->memberList.rbegin(); (itr !=memberNode->memberList.rend())
		 				&& (sends <=(memberNode->memberList.size()/2) ); it++){
			//get first member of membership list
			//create message : header | Address | heartbeat
			switch (msg_type) {
				case JOINREP:
					((MessageHdr*)msg_ptr)->msgType = JOINREP;
					break;
				case HEARTBEAT:
					((MessageHdr*)msg_ptr)->msgType = HEARTBEAT;
					break;
				default : return;
			}

			//get membership entry
			cur_entry = *itr;

			//reconstruct address from membership entry
			translate_MLE ((Address*)&entry_addr, &cur_entry);
			//memcpy (&entry_addr, &cur_entry.id, 4);
			//memcpy (&entry_addr[4], &cur_entry.port, 2);
			//put address into message
			void* offset = (void*) ((long)msg_ptr + (long)sizeof(MessageHdr));
			memcpy ((char*)(offset), &entry_addr, sizeof(Address));

			//put in heartbeat
			memcpy ((char*) ((long)offset + (long)sizeof(Address) +1), &cur_entry.heartbeat, sizeof(long));
			//send message
			emulNet->ENsend(&memberNode->addr, (Address*)to_addr, (char*) msg_ptr, msg_size);
			sends++;
		}
		//cout<<"FINISHED SENDING FROM BACK"<<endl;

	}


	//free used variables
	free (msg_ptr);
	//increment node's heartbeat after every send
	memberNode->heartbeat++;
	if (msg_type == JOINREP)
		cout<< "NO OF ENTRIES SENT ON JOIN: "<< sends<<endl;

}



/**
 * FUNCTION NAME: gossip_multicast
 *
 * DESCRIPTION: Multicast membership list
 *
 * Inputs ; none, uses the object
 *
 *
 * Return Value : True if successful, false otherwise
 *
 *
 * Functionality - Sends Gossip messages to b of the N random nodes
 *
 */
bool MP1Node ::gossip_multicast(){
	//Local Variables

	size_t receiver_no, send_thresh, list_size, i, sent_entries=0;
	MemberListEntry chosen_entry;
	Address out_addr[sizeof(Address)];
	//vector to store chosen numbers
	vector<size_t> chosen_peers;
	//array of booleans to serve as bitmap
	//bool bitmap[memberNode->memberList.size()] = {false};
	vector <bool> bitmap;
	//initialise first element to checked
	//bitmap[0]= true;
	bitmap.push_back(true);
	for (i=1; i<memberNode->memberList.size(); i++){
		//set all to false
		bitmap.push_back(false);
	}

	//update your own entry before send
	memberNode->memberList[0].heartbeat = memberNode->heartbeat;
	memberNode->memberList[0].timestamp = par->getcurrtime();

	//choose b random nodes
	list_size = memberNode->memberList.size();
	// //set threshold to be n/2 +1 to achieve quorom
	 send_thresh = ((list_size-1) /2) +1;

	//while I have not sent to b random nodes
	//while (sent_entries < send_thresh){
	while (sent_entries < send_thresh){
		////// CHOOSE RANDOM PEER TO SEND TO //////////
		while (1){
			//Get a random number in range
			srand(time(0));
			size_t random = rand();
			receiver_no = random % (list_size);
			//if only 2 in group
			if (list_size<=2){
				if (list_size<2)
					return false;
				receiver_no =1;
				break;
			}
			//don't send to failed node
			if ( (par->getcurrtime() -memberNode->memberList[receiver_no].timestamp) >  TFAIL ){
				continue;
			}
			//don't send to yourself
			if (receiver_no ==0)
				continue;
			//If the number is new, break from loop
			if (bitmap[receiver_no] == false){
				break;
			}
		}
		//////////////////////////////////////////////


		//push chosen number to vector/bitmap
		bitmap[receiver_no] = true;
		//update the number of entries sent to
		sent_entries++;
		//Get the details for the MLE at receiver_no
		chosen_entry = memberNode->memberList[receiver_no];
		//get the address from the MLE
		translate_MLE ((Address*)&out_addr, &chosen_entry);
		if (memberNode->bFailed)
   	 		cout<< " SENDING MEMBER IS FAILED"<<endl;
		// cout<< "GOSSIPING FROM : "; printAddress(&memberNode->addr);
		// cout<< " TO: "; printAddress(out_addr); cout<<endl;

		//Send whole membership list to out address if T has not yet failed
		if (((long) par->getcurrtime() - memberNode->memberList[receiver_no].timestamp) < TFAIL ){
			member_unicast((char*)out_addr, HEARTBEAT);
		}

		//get the next node in line
		// receiver_no += jump_val;
		// receiver_no %=list_size;

	}
	cout<<"FINISHED GOSSIP..."<<endl;
	return true;
}


/**
 * FUNCTION NAME: translate_MLE
 *
 * DESCRIPTION: translates a membeship table entry to address
 *
 * Inputs ; conv_addr - address to store result
 *			conv_entry - hmembership table entry to convert
 *
 *
 * Return Value : True if successful, false otherwise
 *
 *
 * Functionality - Updates membership list for a new heartbeat
 *
 */
void MP1Node ::translate_MLE (Address* conv_addr, MemberListEntry* conv_entry ){
	//Local Variables
	char entry_addr[sizeof(Address)];

	//reconstruct address from membership entry
	memcpy (&entry_addr, &conv_entry->id, sizeof(int));
	memcpy (&entry_addr[sizeof(int)], &conv_entry->port, sizeof(short));
	//Copy to destination address
	memcpy (conv_addr, (char*)entry_addr, sizeof(Address));

}
//////////////////////////////////////////////////////////////////////////////////






/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
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


	/*
	 * Your code goes here
	 */
	 if (memberNode->bFailed)
	 	cout<< " SENDING MEMBER IS FAILED"<<endl;

	 //Local Variables
	 size_t i;
	 Address removed_addr;
	 vector<MemberListEntry>::iterator it = memberNode->memberList.begin();

	 //print membership list
	 printML();

	 //for each of the member entries
	 for (i=1; i<memberNode->memberList.size(); i++, it++){
	 //for (it =memberNode->memberList.begin(); it !=memberNode->memberList.end(); it++){
		 //Check if timestamp has passed Tfail+Tremove seconds
		 //if so - delete, if not - do nothing
		 if ( ((long)par->getcurrtime()-memberNode->memberList[i].timestamp) >= TREMOVE){
			 //remove entry
			 cout<< "FROM : "; printAddress(&memberNode->addr);
	 		 cout<< " REMOVED ID: " << memberNode->memberList[i].id <<endl;
			 //log removal of node
			 translate_MLE(&removed_addr, &memberNode->memberList[i]);
			 log->logNodeRemove(&memberNode->addr, &removed_addr );
			 memberNode->memberList.erase(it);

			 //set i & iterator back to get next node
			 i--;
			 it--;

		 }

	 }
	 //Gossip your membership list
	 gossip_multicast();
     return;
}



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
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{	/*\n*/ //- don't forget to add new line
    printf("%d.%d.%d.%d:%d ",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;
}
