/**********************************
 * FILE NAME: MP2Node.h
 *
 * DESCRIPTION: MP2Node class header file
 **********************************/

#ifndef MP2NODE_H_
#define MP2NODE_H_

/**
 * Header files
 */
#include "stdincludes.h"
#include "EmulNet.h"
#include "Node.h"
#include "HashTable.h"
#include "Log.h"
#include "Params.h"
#include "Message.h"
#include "Queue.h"

// Macros
#define QUORUM_CNT 2
#define TIMEOUT 20

/**
 * CLASS NAME: MP2Node
 *
 * DESCRIPTION: This class encapsulates all the key-value store functionality
 * 				including:
 * 				1) Ring
 * 				2) Stabilization Protocol
 * 				3) Server side CRUD APIs
 * 				4) Client side CRUD APIs
 */
class MP2Node {
private:
	// Vector holding the next two neighbors in the ring who have my replicas
	vector<Node> hasMyReplicas;
	// Vector holding the previous two neighbors in the ring whose replicas I have
	vector<Node> haveReplicasOf;
	// Ring
	vector<Node> ring;
	// Hash Table
	HashTable * ht;
	// Member representing this member
	Member *memberNode;
	// Params object
	Params *par;
	// Object of EmulNet
	EmulNet * emulNet;
	// Object of Log
	Log * log;
	// 	ID:No of Success replies:
	//key: value
	//vector<int, int> success_map;
	map <int, int> quorum_map;			//id : quorum cnt
	map <int, long> request_time_map;	//id vs time
	map <int, Message*> message_cache;	//id : msg ptr
	map <int, string> read_cache;
	//vector <Message> message_cache;

public:
	MP2Node(Member *memberNode, Params *par, EmulNet *emulNet, Log *log, Address *addressOfMember);
	Member * getMemberNode() {
		return this->memberNode;
	}

	// ring functionalities
	void updateRing();
	vector<Node> getMembershipList();
	size_t hashFunction(string key);
	void findNeighbors();

	// client side CRUD APIs
	void clientCreate(string key, string value);
	void clientRead(string key);
	void clientUpdate(string key, string value);
	void clientDelete(string key);

	// receive messages from Emulnet
	bool recvLoop();
	static int enqueueWrapper(void *env, char *buff, int size);

	// handle messages from receiving queue
	void checkMessages();

	// coordinator dispatches messages to corresponding nodes
	void dispatchMessages(Message message);

	// find the addresses of nodes that are responsible for a key
	vector<Node> findNodes(string key);

	// server
	bool createKeyValue(string key, string value, ReplicaType replica);
	string readKey(string key);
	bool updateKeyValue(string key, string value, ReplicaType replica);
	bool deletekey(string key);

	// stabilization protocol - handle multiple failures
	void stabilizationProtocol();


	//Helper Functions
	void dissectMsg( char* data,  int& inID, Address& inAddr, MessageType& type,
					string& iKey, string& iValue, ReplicaType replica, int size);
	ReplicaType getReplicaType (string ikey);
	void sortArchives();
	void archiveAdd(Message& imsg);
	void logTrans(MessageType type, bool isCoordinator, int transID,
		string key, string value, bool success);

	//message handlers
	void switchBoard(Message& imsg);
	void handle_create( Message& imsg);
	void handle_read( Message& imsg);
	void handle_update( Message& imsg);
	void handle_delete( Message& imsg);
	void handle_reply( Message& imsg);
	void handle_readreply( Message& imsg);

	~MP2Node();
};

#endif /* MP2NODE_H_ */
