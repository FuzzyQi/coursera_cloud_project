/**********************************
 * FILE NAME: MP2Node.cpp
 *
 * DESCRIPTION: MP2Node class definition
 **********************************/
#include "MP2Node.h"

/**
 * constructor
 */
MP2Node::MP2Node(Member *memberNode, Params *par, EmulNet * emulNet, Log * log, Address * address) {
	this->memberNode = memberNode;
	this->par = par;
	this->emulNet = emulNet;
	this->log = log;
	ht = new HashTable();
	this->memberNode->addr = *address;
}

/**
 * Destructor
 */
MP2Node::~MP2Node() {
	delete ht;
	delete memberNode;
}

/**
 * FUNCTION NAME: updateRing
 *
 * DESCRIPTION: This function does the following:
 * 				1) Gets the current membership list from the Membership Protocol (MP1Node)
 * 				   The membership list is returned as a vector of Nodes. See Node class in Node.h
 * 				2) Constructs the ring based on the membership list
 * 				3) Calls the Stabilization Protocol
 */
void MP2Node::updateRing() {
	/*
	 * Implement this. Parts of it are already implemented
	 */
	vector<Node> curMemList;
	bool change = false;

	/*
	 *  Step 1. Get the current membership list from Membership Protocol / MP1
	 */
	curMemList = getMembershipList();

	/*
	 * Step 2: Construct the ring
	 */
	// Sort the list based on the hashCode
	sort(curMemList.begin(), curMemList.end());
	//update the ring if needed by comparing ring with curMemList
	// Compare each entry if necessary
	if (curMemList.size() != ring.size()){
		change = true;
		//copy over new data
		ring = curMemList;
	}

	/*
	 * Step 3: Run the stabilization protocol IF REQUIRED
	 */
	// Run stabilization protocol
	if (change && (!ht->isEmpty())){
		stabilizationProtocol();
	}
}

/**
 * FUNCTION NAME: getMemberhipList
 *
 * DESCRIPTION: This function goes through the membership list from the Membership protocol/MP1 and
 * 				i) generates the hash code for each member
 * 				ii) populates the ring member in MP2Node class
 * 				It returns a vector of Nodes. Each element in the vector contain the following fields:
 * 				a) Address of the node
 * 				b) Hash code obtained by consistent hashing of the Address
 */
vector<Node> MP2Node::getMembershipList() {
	unsigned int i;
	vector<Node> curMemList;
	for ( i = 0 ; i < this->memberNode->memberList.size(); i++ ) {
		Address addressOfThisMember;
		int id = this->memberNode->memberList.at(i).getid();
		short port = this->memberNode->memberList.at(i).getport();
		memcpy(&addressOfThisMember.addr[0], &id, sizeof(int));
		memcpy(&addressOfThisMember.addr[4], &port, sizeof(short));
		//check if member is alive
		if (memberNode->memberList.at(i).getheartbeat() == -1)
			continue;
		curMemList.emplace_back(Node(addressOfThisMember));
	}
	return curMemList;
}

/**
 * FUNCTION NAME: hashFunction
 *
 * DESCRIPTION: This functions hashes the key and returns the position on the ring
 * 				HASH FUNCTION USED FOR CONSISTENT HASHING
 *
 * RETURNS:
 * size_t position on the ring
 */
size_t MP2Node::hashFunction(string key) {
	std::hash<string> hashFunc;
	size_t ret = hashFunc(key);
	return ret%RING_SIZE;
}

/**
 * FUNCTION NAME: clientCreate
 *
 * DESCRIPTION: client side CREATE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientCreate(string key, string value) {
	/*
	 * IMPLELENTED
	 */
	 //local variables
	 vector <Node> r_nodes;
	 vector <Node> :: iterator ring_it;
	 //Create a create message to be sent to the servers
	 Message oMessage = Message (g_transID++, memberNode->addr,
		 							CREATE, key, value);
	 //update the ring
	 updateRing();
	 //find the replicas of this key
	 r_nodes = findNodes(key);
	 //add message to archives
	 archiveAdd(oMessage);
	 //send a message to all the replicas
	 for (auto& it : r_nodes)
	 	emulNet->ENsend (&memberNode->addr, &it.nodeAddress, oMessage.toString() );
}


/**
 * FUNCTION NAME: clientRead
 *
 * DESCRIPTION: client side READ API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientRead(string key){
	/*
	 * Implement this
	 */
	 //local variables
	 vector <Node> r_nodes;
	 vector <Node> :: iterator ring_it;
	 //Create a create message to be sent to the servers
	 Message oMessage = Message (g_transID++, memberNode->addr,
									READ, key);
	 //update the ring
	 updateRing();
	 //find the replicas of this key
	 r_nodes = findNodes(key);
	 //add message to archives
	 archiveAdd(oMessage);
	 //send a message to all the replicas
	 for (auto& it : r_nodes)
		emulNet->ENsend (&memberNode->addr, &it.nodeAddress, oMessage.toString() );

}

/**
 * FUNCTION NAME: clientUpdate
 *
 * DESCRIPTION: client side UPDATE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientUpdate(string key, string value){
	/*
	 * Implement this
	 */
	 //local variables
	 vector <Node> r_nodes;
	 vector <Node> :: iterator ring_it;
	 //Create an update message to be sent to the replicas
	 Message oMessage = Message (g_transID++, memberNode->addr,
									UPDATE, key, value, UNKNOWN);
	 //update the ring
	 updateRing();
	 //find the replicas of this key
	 r_nodes = findNodes(key);
	 //send a message to all the replicas
	 //add message to archives
	 archiveAdd(oMessage);
	 for (auto& it : r_nodes)
	 	emulNet->ENsend (&memberNode->addr, &it.nodeAddress, oMessage.toString() );
}

/**
 * FUNCTION NAME: clientDelete
 *
 * DESCRIPTION: client side DELETE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientDelete(string key){
	/*
	 * IMPLELENTED
	 */
	 //local variables
	 vector <Node> r_nodes;
	 vector <Node> :: iterator ring_it;
	 //Create a delete message to be sent to the servers
	 Message oMessage = Message (g_transID++, memberNode->addr,
		 	DELETE, key);
	 //update the ring
	 updateRing();
	 //find the replicas of this key
	 r_nodes = findNodes(key);
	 //add message to archives
	 archiveAdd(oMessage);
	 //send a message to all the replicas
	 for (auto& it : r_nodes)
	 	emulNet->ENsend (&memberNode->addr, &it.nodeAddress, oMessage.toString() );
}

/**
 * FUNCTION NAME: createKeyValue
 *
 * DESCRIPTION: Server side CREATE API
 * 			   	The function does the following:
 * 			   	1) Inserts key value into the local hash table
 * 			   	2) Return true or false based on success or failure
 */
bool MP2Node::createKeyValue(string key, string value, ReplicaType replica) {
	/*
	 * Implement this
	 */
	// Insert key, value, replicaType into the hash table
	//make a key value entry using the value and replica type
	Entry new_entry(value, par->getcurrtime(), replica);
	//create the entry in the hash table as a string
	return ht->create(key, new_entry.convertToString());
}

/**
 * FUNCTION NAME: readKey
 *
 * DESCRIPTION: Server side READ API
 * 			    This function does the following:
 * 			    1) Read key from local hash table
 * 			    2) Return value
 */
string MP2Node::readKey(string key) {
	/*
	 * Implement this
	 */
	// Read key from local hash table and return value
	return ht->read(key);
}

/**
 * FUNCTION NAME: updateKeyValue
 *
 * DESCRIPTION: Server side UPDATE API
 * 				This function does the following:
 * 				1) Update the key to the new value in the local hash table
 * 				2) Return true or false based on success or failure
 */
bool MP2Node::updateKeyValue(string key, string value, ReplicaType replica) {
	/*
	 * Implement this
	 */
	// Update key in local hash table and return true or false
	//get updated entry
	Entry updated_entry (value, par->getcurrtime(), replica);
	//update the hash table entry
	return ht->update (key, updated_entry.convertToString());
}

/**
 * FUNCTION NAME: deleteKey
 *
 * DESCRIPTION: Server side DELETE API
 * 				This function does the following:
 * 				1) Delete the key from the local hash table
 * 				2) Return true or false based on success or failure
 */
bool MP2Node::deletekey(string key) {
	/*
	 * Implement this
	 */
	// Delete the key from the local hash table
	return ht->deleteKey(key);
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: This function is the message handler of this node.
 * 				This function does the following:
 * 				1) Pops messages from the queue
 * 				2) Handles the messages according to message types
 */
void MP2Node::checkMessages() {
	/*
	 * Implement this. Parts of it are already implemented
	 */
	char * data;
	int size;

	/*
	 * Declare your local variables here
	 */

	// dequeue all messages and handle them
	while ( !memberNode->mp2q.empty() ) {
		/*
		 * Pop a message from the queue
		 */
		data = (char *)memberNode->mp2q.front().elt;
		size = memberNode->mp2q.front().size;
		memberNode->mp2q.pop();

		string message(data, data + size);

		/*
		 * Handle the message types here
		 */
		 //create messagge var called imsg
		 Message imsg(message);
		 //call message handler function
		 switchBoard(imsg);

	}

	/*
	 * This function should also ensure all READ and UPDATE operation
	 * get QUORUM replies
	 */
	 //sort out archived messages : checks for quorum
	 sortArchives();
}






/////////////////////////////////	MESSAGE HANDLERS 	///////////////////////
/**
 * FUNCTION NAME: handle_Create
 *
 * DESCRIPTION: Handles a request message
 *
 * Inputs : imsg - Message that came in
 *
 * Return Value : nothing
 *
 *Functionality : Creates a Key Value pair & logs a failure or success
 */
void MP2Node ::handle_create( Message& imsg){
	//local variables
	ReplicaType my_replica;
	vector <Node> replica_vect = findNodes(imsg.key);
	bool status = false;

	//get replica type of this node for this key
	my_replica = getReplicaType(imsg.key, memberNode->addr);
	//if unknoown, send to correct replicas
	if (my_replica == UNKNOWN){
		cout << "RESENDING, STALE MEMBERSHIP LIST"<<endl;
		emulNet->ENsend(&memberNode->addr, &replica_vect[0].nodeAddress,
			imsg.toString());
		return;
	}

	//Perfrom operation and send a reply to the coordinator
	//perofrm op & log if success or failure
	status = createKeyValue(imsg.key, imsg.value, my_replica);
	if (status){
		log->logCreateSuccess(&memberNode->addr, false,
		 				imsg.transID, imsg.key, imsg.value);
	}
	else{
		log->logCreateFail(&memberNode->addr, false,
		 				imsg.transID, imsg.key, imsg.value);
	}
	//send success to coord
	Message reply(imsg.transID, memberNode->addr, REPLY, status);
	emulNet->ENsend(&memberNode->addr, &imsg.fromAddr,
	 					reply.toString());
}


/**
 * FUNCTION NAME: handle_Read
 *
 * DESCRIPTION: Handles a read message
 *
 * Inputs : imsg - Message that came in
 *
 * Return Value : nothing
 *
 *Functionality : Reads and returns the value of a given key
 *					logs a failure or success
 */
void MP2Node ::handle_read( Message& imsg){
	//local variables
	string oValue;
	oValue = readKey(imsg.key);
	//Check if key exists
	if (oValue == ""){
		log->logReadFail(&memberNode->addr, false,
		 				imsg.transID, imsg.key);
	}
	else{
		//Get the entry, log it then send to coordinator
		Entry oEntry(oValue);
		log->logReadSuccess(&memberNode->addr, false,
		 				imsg.transID, imsg.key, oEntry.value);
		//send success to coord
		Message read_reply(imsg.transID, memberNode->addr, oValue);
		emulNet->ENsend(&memberNode->addr, &imsg.fromAddr,
		 					read_reply.toString());
	}
}


/**
 * FUNCTION NAME: handle_update
 *
 * DESCRIPTION: Handles an update message
 *
 * Inputs : imsg - Message that came in
 *
 * Return Value : nothing
 *
 *Functionality : Updates the value of a given key if it exists
 *					logs a failure or success
 *					Also processes update messages from primary replicas
 */
void MP2Node ::handle_update( Message& imsg){
	//local variables
	ReplicaType my_replica;
	vector <Node> replica_vect = findNodes(imsg.key);
	bool status = false;
	bool flag = false;

	//get replica type
	my_replica = getReplicaType(imsg.key, memberNode->addr);
	//if unknoown, it is from coordinator
		if (imsg.replica == UNKNOWN){
			//Update if key exists
			if (ht->hashTable.find(imsg.key) != ht->hashTable.end()){
				status = updateKeyValue(imsg.key, imsg.value, my_replica);
			}
			//log & send value on successful update
			if (status){
				log->logUpdateSuccess(&memberNode->addr, false, imsg.transID,
					imsg.key, imsg.value);
				Message reply(imsg.transID, memberNode->addr, REPLY, status);
				emulNet->ENsend(&memberNode->addr, &imsg.fromAddr,
				 					reply.toString());
			}
			//log faulure
			else{
				//key does not exist: failed update
				log->logUpdateFail(&memberNode->addr, false,
					imsg.transID,imsg.key, imsg.value);
			}
		}
		//It came from one of the primary replicas
		else{
			//Add to vector list if not already there
			for (auto& it : haveReplicasOf){
				//flag if you find it
				if (imsg.fromAddr == it.nodeAddress)
					flag = true;
			}
			if (!flag)
				haveReplicasOf.push_back(imsg.fromAddr);

			//if it does not exist, create it, else, update it
			if (ht->hashTable.find(imsg.key) == ht->hashTable.end()){
				createKeyValue(imsg.key, imsg.value, imsg.replica);
			}
			else{
				updateKeyValue(imsg.key, imsg.value, my_replica);
			}
		}
}


/**
 * FUNCTION NAME: handle_delete
 *
 * DESCRIPTION: Handles a delete message
 *
 * Inputs : imsg - Message that came in
 *
 * Return Value : nothing
 *
 *Functionality : Deletes a given key-value entry if it exists
 *					logs a failure or success
 */
void MP2Node ::handle_delete( Message& imsg){
	//local variables
	ReplicaType my_replica;
	vector <Node> replica_vect = findNodes(imsg.key);
	bool status = false;

	//get replica type of this node for this key
	my_replica = getReplicaType(imsg.key, memberNode->addr);
	//if unknoown, send to correct replicas
	if (my_replica == UNKNOWN){
		cout << "RESENDING, STALE MEMBERSHIP LIST"<<endl;
		emulNet->ENsend(&memberNode->addr, &replica_vect[0].nodeAddress,
			imsg.toString());
		return;
	}

	//Delete eht key, and log a failure or success
	status = deletekey(imsg.key);
	if (status){
		log->logDeleteSuccess(&memberNode->addr, false,
						imsg.transID, imsg.key);
	}
	else{
		cout<<"delete failed"<<endl;
		log->logDeleteFail(&memberNode->addr, false,
						imsg.transID, imsg.key);
	}
	//send success message to coord
	Message reply(imsg.transID, memberNode->addr, REPLY, status);
	emulNet->ENsend(&memberNode->addr, &imsg.fromAddr,
						reply.toString());
}


/**
 * FUNCTION NAME: handle_reply
 *
 * DESCRIPTION: Handles a reply message
 *
 * Inputs : imsg - Message that came in
 *
 * Return Value : nothing
 *
 *Functionality : Updates the quorum count for a success reply message
 */
void MP2Node ::handle_reply( Message& imsg){
	//If it was a success reply, update quorum count
	if (imsg.success){
		//update quorum cnt if already there
		if (quorum_map.find(imsg.transID) != quorum_map.end()){
			quorum_map[imsg.transID]++;
			//cout << "TRANSACTION SUCCESS"<<endl;
		}
		else
			cout << "ERROR : DELETED "<<endl;
	}
}

// Handles a readreply message
/**
 * FUNCTION NAME: handle_readreply
 *
 * DESCRIPTION: Handles a readreply message
 *
 * Inputs : imsg - Message that came in
 *
 * Return Value : nothing
 *
 *Functionality : Stores the read reply locally if it is the most up to date
 */
void MP2Node ::handle_readreply( Message& imsg){
	//Add to the cache if not already there
	if (read_cache.find(imsg.transID) ==read_cache.end() ){
		//archive the value in the read cache
		read_cache.emplace(make_pair(imsg.transID, imsg.value));

	}
	//Replace in cache only if it has a newer timestamp
	else{
		Entry i_entry(imsg.value);
		Entry str_entry(read_cache[imsg.transID]);
		if (i_entry.timestamp>str_entry.timestamp){
			//replace with the input message
			read_cache[imsg.transID] = imsg.value;
		}
	}
	//update quorum count for the original read message
	quorum_map[imsg.transID]++;
}
///////////////////////////////////////////////////////////////////////////////






/////////////////////////////////	HELPER FUNCTIONS	////////////////////////
//routes to correct handler
/**
 * FUNCTION NAME: switchBoard
 *
 * DESCRIPTION: Handles all input message
 *
 * Inputs : imsg - Message that came in
 *
 * Return Value : nothing
 *
 *Functionality : Calls the function that handles the specific message
 */
void MP2Node ::switchBoard(Message& imsg){
	//send to to message handler
	switch (imsg.type){
		case CREATE: return handle_create(imsg);

		case READ: return handle_read(imsg);

		case UPDATE: return handle_update(imsg);

		case DELETE: return handle_delete(imsg);

		case REPLY: return handle_reply(imsg);

		case READREPLY: return handle_readreply(imsg);
	}
}


//Function that adds a message to the archives
/**
 * FUNCTION NAME: archiveAdd
 *
 * DESCRIPTION: Stores a message locally with the local time
 *					& starts a quorum count
 *
 * Inputs : imsg - Message that came in
 *
 * Return Value : nothing
 *
 */
void MP2Node ::archiveAdd(Message& imsg){
	//local variables
	Message *archive_msg = NULL;
	//add to message archive
	archive_msg = new Message(imsg);
	message_cache.emplace(make_pair(imsg.transID, archive_msg));
	//Start the Quorum Count
	quorum_map.emplace(make_pair(imsg.transID, 0));
	//store current time
	request_time_map.emplace(make_pair(imsg.transID, par->getcurrtime()));
}


//function sorts through archived messages
/**
 * FUNCTION NAME: sortArchives
 *
 * DESCRIPTION: Checks if quorum has been reached for each message
 *					Also checks if any message requests have timed out
 *					Logs success or failure from the coordinators side
 *
 * Inputs : imsg - Message that came in
 *
 * Return Value : nothing
 */
void MP2Node ::sortArchives(){
	//local variables
	string val;
	long cur_time = par->getcurrtime();
	map <int, int> :: iterator quorum_it = quorum_map.begin();

	//for each message request waiting on replicas
	for (; quorum_it != quorum_map.end(); quorum_it++){
		Message target_msg = *message_cache[quorum_it->first];
		val = target_msg.value;
		//If read msg, get the value
		if (target_msg.type == READ){
			//get reply value from the read cache
			if (read_cache.find(quorum_it->first) != read_cache.end()){
				//if it exists
				Entry read_entry(read_cache[quorum_it->first]);
				val = read_entry.value;
			}
		}
		//check if quorum has been achieved & we've given enough time
		if ((quorum_it->second >= QUORUM_CNT) &&
				((cur_time -request_time_map[quorum_it->first] )>=WAITING)){
			//log as completed by coordinator
			logTrans(target_msg.type, true, quorum_it->first, target_msg.key,
				val, true);
			//remove message & counts from the map
			if (target_msg.type == READ){
				//remove from read cache
				read_cache.erase(quorum_it->first);
			}
			//free memory
			delete message_cache[quorum_it->first];
			message_cache.erase(quorum_it->first);
			request_time_map.erase(quorum_it->first);
			quorum_map.erase(quorum_it);
		}
		//if message has timed out
		else if ( (cur_time -request_time_map[quorum_it->first] )>=TIMEOUT){
			//log as failed & delete
			logTrans(target_msg.type, true, quorum_it->first, target_msg.key,
				val, false);
			if (target_msg.type == READ){
				//remove from read cache if necessary
				if (read_cache.find(quorum_it->first) != read_cache.end()){
					read_cache.erase(quorum_it->first);
				}
			}
			//free memory
			delete message_cache[quorum_it->first];
			message_cache.erase(quorum_it->first);
			request_time_map.erase(quorum_it->first);
			quorum_map.erase(quorum_it);
		}
	}
}


/**
 * FUNCTION NAME: logTrans
 *
 * DESCRIPTION: Logs any kind of message
 *
 * Inputs : type -type of message to log
 *			isCoordinator - if coordinator is logging
 *			transID  - the transaciton id
 *			key  - the key of the request
 *			value  - the value of the request
 *			success - if request was a success
 *
 * Return Value : nothing
 *
 */
void MP2Node ::logTrans(MessageType type, bool isCoordinator, int transID,
	string key, string value, bool success){

	switch(type){
		case CREATE:
				if (success){
					log->logCreateSuccess(&memberNode->addr, isCoordinator,
						transID, key, value);
				}
				else{
					log->logCreateFail(&memberNode->addr, isCoordinator,
						transID, key, value);
				}
				break;
		case READ:
				if (success){
					log->logReadSuccess(&memberNode->addr, isCoordinator,
						transID, key, value);
				}
				else{
					log->logReadFail(&memberNode->addr, isCoordinator,
						transID, key);
				}
				break;
		case UPDATE:
				if (success){
					log->logUpdateSuccess(&memberNode->addr, isCoordinator,
						transID, key, value);
				}
				else{
					log->logUpdateFail(&memberNode->addr, isCoordinator,
						transID, key, value);
				}
				break;
		case DELETE:
				if (success){
					log->logDeleteSuccess(&memberNode->addr, isCoordinator,
						transID, key);
				}
				else{
					log->logDeleteFail(&memberNode->addr, isCoordinator,
						transID, key);
				}
				break;
		default: break;

	}
}



//Finds the replica type of this node for a particular key
/**
 * FUNCTION NAME: logTrans
 *
 * DESCRIPTION: Logs any kind of message
 *
 * Inputs : key  - the key of the request
 *			addr  - the address to check
 *
 * Return Value : nothing
 *
 */
ReplicaType MP2Node ::getReplicaType (string ikey, Address addr){
	//local variables
	size_t i;
	vector<Node> inodes;
	//get all the nodes for this key
	inodes = findNodes(ikey);
	//Find out what replica addr is
	for (i=0; i<inodes.size(); i++){
		if (inodes[i].nodeAddress == addr){
			switch (i){
				case 0:	return PRIMARY;
				case 1:	return SECONDARY;
				case 2:	return TERTIARY;
			}
		}
	}
	return UNKNOWN;
}


////////////////////////////////////////////////////////////////////////////////

/**
 * FUNCTION NAME: findNodes
 *
 * DESCRIPTION: Find the replicas of the given keyfunction
 * 				This function is responsible for finding the replicas of a key
 */
vector<Node> MP2Node::findNodes(string key) {
	size_t pos = hashFunction(key);
	vector<Node> addr_vec;
	if (ring.size() >= 3) {
		// if pos <= min || pos > max, the leader is the min
		if (pos <= ring.at(0).getHashCode() || pos > ring.at(ring.size()-1).getHashCode()) {
			addr_vec.emplace_back(ring.at(0));
			addr_vec.emplace_back(ring.at(1));
			addr_vec.emplace_back(ring.at(2));
		}
		else {
			// go through the ring until pos <= node
			for (int i=1; i<ring.size(); i++){
				Node addr = ring.at(i);
				if (pos <= addr.getHashCode()) {
					addr_vec.emplace_back(addr);
					addr_vec.emplace_back(ring.at((i+1)%ring.size()));
					addr_vec.emplace_back(ring.at((i+2)%ring.size()));
					break;
				}
			}
		}
	}
	return addr_vec;
}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: Receive messages from EmulNet and push into the queue (mp2q)
 */
bool MP2Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), this->enqueueWrapper, NULL, 1, &(memberNode->mp2q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue of MP2Node
 */
int MP2Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}


/**
 * FUNCTION NAME: stabilizationProtocol
 *
 * DESCRIPTION: This runs the stabilization protocol in case of Node joins and leaves
 * 				It ensures that there always 3 copies of all keys in the DHT at all times
 * 				The function does the following:
 *				1) Ensures that there are three "CORRECT" replicas of all the keys in spite of failures and joins
 *				Note:- "CORRECT" replicas implies that every key is replicated in its two neighboring nodes in the ring
 */
void MP2Node::stabilizationProtocol() {
	/*
	 * IMPLELENTED
	 */
	 //local variables
	 vector <Node> replica_vect;
	 ReplicaType new_type;
	 string my_key;
	 bool flag;
	 map<string, string> :: iterator hash_it = ht->hashTable.begin();

	 //for each entry in my hash table: check if node was alwas primary
	 for (; hash_it!=ht->hashTable.end(); hash_it++){
		 my_key = hash_it->first;
		 //Get the previous replica type of this node for this key
		 Entry my_entry(hash_it->second);
		 replica_vect = findNodes(my_key);
		 new_type = getReplicaType(my_key, memberNode->addr);

		//if node was always primary, and is still primary
		 if ((my_entry.replica == PRIMARY) && (new_type==PRIMARY)){
			 //If any replicas have failed or joined
			if (!animateReplicas(hasMyReplicas, replica_vect)){
				//for each node, copy over, and send an update message
				for (auto& vect_it : replica_vect){
					new_type = getReplicaType(my_key, vect_it.nodeAddress);
					//Don't send message to node-self
					if (new_type != PRIMARY){
						//copy over to the right vector if not already there
						flag = false;
						for (auto& it : haveReplicasOf){
							//flag if you find it
							if (vect_it.nodeAddress == it.nodeAddress)
								flag = true;
						}
						if (!flag)
							hasMyReplicas.push_back(vect_it.nodeAddress);
						//send the message
						Message up_msg(g_transID++, memberNode->addr, UPDATE,
							 my_key, my_entry.value, new_type );
						emulNet->ENsend (&memberNode->addr, &vect_it.nodeAddress,
							 up_msg.toString() );
					}
				}
			}
		 }
		 else{
			 new_type = getReplicaType(my_key, memberNode->addr);
			 //If node is a new primary, populae right vector & send updates
			 if (new_type == PRIMARY){
				 hasMyReplicas.clear();
				 //Send message to all other replicas
				 for (auto& vect_it : replica_vect){
 					//send update message to new replica
 					new_type = getReplicaType(my_key, vect_it.nodeAddress);
					if (new_type != PRIMARY){
						//copy over to the relevant vector
	 					hasMyReplicas.push_back(vect_it.nodeAddress);
						Message up_msg(g_transID++, memberNode->addr, UPDATE,
	 						 my_key, my_entry.value, new_type );
	 					emulNet->ENsend (&memberNode->addr, &vect_it.nodeAddress,
	 						 up_msg.toString() );
					}
 				}
				//update type to primary
				updateKeyValue(my_key, my_entry.value, PRIMARY);
			 }
		 }
	 }
}


/**
 * FUNCTION NAME: animateReplicas
 *
 * DESCRIPTION: Checks if all the replicas in the old vector
 *					are in the new one.
 *					Clears the old vector if they differ
 *
 * Inputs : old_vect
 *			new_vect
 *
 * Return Value : same - boolean on if the vectors are the same
 *
 */
bool MP2Node ::animateReplicas(vector<Node>old_vect, vector<Node>new_vect){
	//local variables
	bool same = true;
	int same_cnt =0;
	vector <Node> :: iterator old_it = old_vect.begin();
	vector <Node> :: iterator new_it = new_vect.begin();


	//iterat over both vectors and make sure they are the same
	for (; old_it!=old_vect.end(); old_it++){
		//check if it is in the new oone
		for (new_it= new_vect.begin(); new_it!=new_vect.end(); new_it++){
			if (new_it->nodeAddress == old_it->nodeAddress)
				same_cnt++;
		}
	}
	//The old vector is cleared if they don't match exactly
	if (same_cnt !=2){
		cout<<"new set of neighbors"<<endl;
		old_vect.clear();
		same = false;
	}
	return same;
}
