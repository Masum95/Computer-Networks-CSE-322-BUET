
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include "Util.h"
using namespace std;

int sockfd;
map<string, Edge> neighbourList;
map<string, RoutingTableEntry> routingTableMap;
string myRouterIPAddress;

void initializeRouter(string myIPAddress, string topoFileName)
{
	set<string> vertexSet, neighborSet;
	ifstream topoFile(topoFileName);
	string addrOfRouter1, addrOfRouter2, nbrIP;
	int linkCost;
	while (topoFile >> addrOfRouter1 >> addrOfRouter2 >> linkCost)
	{
		vertexSet.insert(addrOfRouter1);
		vertexSet.insert(addrOfRouter2);
		if (addrOfRouter1 == myIPAddress || addrOfRouter2 == myIPAddress)
		{
			Edge myNeighbor;
			if (addrOfRouter1 == myIPAddress)
				nbrIP = addrOfRouter2;
			else
				nbrIP = addrOfRouter1;
			myNeighbor.setEdge(nbrIP, linkCost);
			neighbourList[nbrIP] = myNeighbor;
			neighborSet.insert(nbrIP);
			routingTableMap[nbrIP] = RoutingTableEntry(nbrIP, nbrIP, linkCost);
		}
	}
	topoFile.close();
	set<string>::iterator it;

	for (it = vertexSet.begin(); it != vertexSet.end(); it++)
	{
		if (*it == myRouterIPAddress)
			routingTableMap[*it] = RoutingTableEntry(*it, *it, 0);
		else if (neighborSet.find(*it) == neighborSet.end())
		{ // if not found in neighbor list
			cout << *it << endl;
			routingTableMap[*it] = RoutingTableEntry(*it, "-", INF);
		}
	}
}

void printMyRoutingTable()
{
	cout << "\t------\t" << myRouterIPAddress << "\t------\t" << endl;
	cout << "Destination  \tNext Hop \tCost" << endl;
	cout << "-------------\t-------------\t-----" << endl;
	map<string, RoutingTableEntry>::iterator it;
	for (it = routingTableMap.begin(); it != routingTableMap.end(); it++)
	{
		//printf("%-16s%-16s%-16d",(it->first).c_str(),((it->second).nextHop).c_str(),(it->second).cost);
		cout << it->first << "\t" << (it->second).nextHop << "\t" << (it->second).cost << endl;
	}
	cout << "--------------------------------------" << endl;
}

string makeIP(string str)
{
	int ipSegment[4];
	for (int i = 0; i < 4; i++)
	{
		unsigned char ch = str[i];
		ipSegment[i] = ch;
	}
	string ip = to_string(ipSegment[0]) + "." + to_string(ipSegment[1]) + "." + to_string(ipSegment[2]) + "." + to_string(ipSegment[3]);
	return ip;
}

char *receiveMessage()
{
	struct sockaddr_in routerAddress;
	char *buffer = new char[1024];
	socklen_t addrlen;

	int bytes_received = recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr *)&routerAddress, &addrlen);
	//if(bytes_received==-1) return NULL;
	return buffer;
}

void forwardMsg(string msg, int length, string destIP)
{
	bool msgSent = false;
	RoutingTableEntry rt = routingTableMap[destIP];
	if (rt.nextHop != UNDEFINED || rt.cost != INF)
	{
		string frwdMsg = string(FORWARD) + " " + destIP + " " + to_string(length) + " " + msg;
		msgSent = sendData(frwdMsg, rt.nextHop, sockfd);
		if (msgSent)
			cout << " packet forwarded to " << rt.nextHop << "  (printed by " << myRouterIPAddress << " )" << endl;
		else
			cout << "Message couldn't be sent. Error occured" << endl;
	}
	else
	{
		cout << "No Next Hop to forward. Message Dropped..." << endl;
	}
}

void processSENDCommmand(char *buffer)
{
	string data(buffer);
	string srcIP = makeIP(data.substr(4, 4));
	string destIP = makeIP(data.substr(8, 4));
	int length = data[12];
	char ara[length + 1];
	for (int i = 0; i < length; i++)
	{
		ara[i] = buffer[14 + i];
	}
	ara[length] = '\0';
	string msg(ara);
	if (destIP == myRouterIPAddress)
	{
		cout << msg << " packet reached to destination (printed by " << myRouterIPAddress << " )" << endl;
	}
	else
	{
		forwardMsg(msg, length, destIP);
	}
}

void processFRWDCommand(char *buffer)
{
	string data(buffer);
	vector<string> msgParts = stringSplit(data, ' ');
	string destIP = msgParts[1];
	int length = stoi(msgParts[2]);
	string msg = mergeVectorToString(vector<string>(msgParts.begin() + 3, msgParts.end()), ' ');
	if (destIP == myRouterIPAddress)
	{
		cout << msg << " packet reached to destination (printed by " << myRouterIPAddress << " )" << endl;
	}
	else
	{
		forwardMsg(msg, length, destIP);
	}
}

// format to conver myRouting table to sting packet
// rtbl myIPAddress dest-nexthop-cost dest-nexthop-cost....
string myTableToPacket()
{
	string packet = "rtbl " + myRouterIPAddress + " ";
	map<string, RoutingTableEntry>::iterator it;
	for (it = routingTableMap.begin(); it != routingTableMap.end(); it++)
	{
		packet += it->first + "/" + (it->second).nextHop + "/" + to_string((it->second).cost) + " ";
	}
	return packet;
}

void broadCastMyTableToNeighbor()
{
	string tablePacket = myTableToPacket();
	map<string, Edge>::iterator it;
	for (it = neighbourList.begin(); it != neighbourList.end(); it++)
	{

		if ((it->second).cost != INF)
		{
			sendData(tablePacket, it->first, sockfd);
		}
	}
}

void processCOST_UPDATECommand(char *buffer)
{
	string data(buffer);
	string ip1 = makeIP(data.substr(4, 4));
	string ip2 = makeIP(data.substr(8, 4));
	int newCost = data[12];
	string neighborIP;
	if (ip1 == myRouterIPAddress)
		neighborIP = ip2;
	else
		neighborIP = ip1;
	if (neighbourList[neighborIP].ttl != 0)
	{
		routingTableMap[neighborIP] = RoutingTableEntry(neighborIP, neighborIP, newCost);
	}
	neighbourList[neighborIP].cost = newCost;
	broadCastMyTableToNeighbor();
}

void updateLinkStatus()
{
	map<string, Edge>::iterator it;
	for (it = neighbourList.begin(); it != neighbourList.end(); it++)
	{
		if ((it->second).ttl != 0)
		{
			(it->second).ttl--;
		}

		if ((it->second).ttl == 0) // If i didn't receive table for 3 consecutive cycle
		{
			if (routingTableMap[it->first].nextHop == it->first)  // if neighbor is reached via this edge, make the cost of edge infintiy.
																  // if neighbor is reached via some other node, don't alter routing table 
				routingTableMap[it->first] = RoutingTableEntry(it->first, "-", INF);
		}
	}
}

void processTABLE_RECEIVECommand(string data)
{
	vector<string> msgParts = stringSplit(data, ' ');
	string neighborIPAddress = msgParts[1];

	if (neighbourList[neighborIPAddress].ttl == 0) // if the edge has been down 
	{
		routingTableMap[neighborIPAddress] = RoutingTableEntry(neighborIPAddress, neighborIPAddress, neighbourList[neighborIPAddress].cost);
	}
	neighbourList[neighborIPAddress].ttl = MAX_TTL;
	for (int i = 2; i < msgParts.size(); i++)
	{
		vector<string> tableRow = stringSplit(msgParts[i], '/');
		string dest = tableRow[0];
		string nextHop = tableRow[1];
		int costFromNeighbor = stoi(tableRow[2]);
		if (nextHop == myRouterIPAddress || dest == myRouterIPAddress)
			continue;
		string resNextHop; // which nextHop to follow to reache the goal
		if(routingTableMap[neighborIPAddress].cost<neighbourList[neighborIPAddress].cost){  // is cost to reach the node, stored in routing table, is less than the direct cost?
																							// cost in routing table may not be less than the direct edge cost in the case when
																							// some of the edge of the previous optimal path has been down , and then the neighbor gets unreachable via that route
																							// So then direct edge cost is less than the routing table cost

			resNextHop = routingTableMap[neighborIPAddress].nextHop;
		}else{
			resNextHop = neighborIPAddress;
		}
		int resCost = addEdgeCost(min(routingTableMap[neighborIPAddress].cost,neighbourList[neighborIPAddress].cost), costFromNeighbor);

		if (routingTableMap[dest].cost > resCost || routingTableMap[dest].nextHop == neighborIPAddress) // second condition checks if the neighbor is the only way to reach the destination ,
																										// then update the cost even if it is greater than the routing table cost
																										// this scenario may arise when cost between neighbor and goal has been increased 
																										// so cost from this current node must be forcefully updated. That is previous less cost to reach the goal
																										// is now invalid as the path cost has been modified
		{
			resNextHop = (resCost == INF) ? UNDEFINED : resNextHop;
			routingTableMap[dest].cost = resCost;
			routingTableMap[dest].nextHop = resNextHop;
		}
	}
}

void receiveCommands()
{
	while (1)
	{
		char *buffer = receiveMessage();
		string data(buffer);
		
		if (data != "") // not empty message
		{

			string header = data.substr(0, 4);
			if (header.find("clk") != string::npos)
			{
				updateLinkStatus();
				broadCastMyTableToNeighbor();
				//printMyRoutingTable();
			}
			else if (header == SEND)
			{
				processSENDCommmand(buffer);
			}
			else if (header == COST_UPDATE)
			{
				processCOST_UPDATECommand(buffer);
			}
			else if (header == FORWARD)
			{
				processFRWDCommand(buffer);
			}
			else if (header == SHOW_TABLE)
			{
				printMyRoutingTable();
			}
			else if (header == TABLE_RECEIVE)
			{
				processTABLE_RECEIVECommand(data);
			}
		}
	}
}

int main(int argc, char const *argv[])
{
	if (argc != 3)
	{
		cout << "router : " << argv[1] << "<ip address>\n";
		exit(1);
	}

	myRouterIPAddress = argv[1];
	initializeRouter(argv[1], argv[2]);
	printMyRoutingTable();
	struct sockaddr_in client_address;

	client_address.sin_family = AF_INET;
	client_address.sin_port = htons(4747);
	inet_pton(AF_INET, myRouterIPAddress.c_str(), &client_address.sin_addr);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	bind(sockfd, (struct sockaddr *)&client_address, sizeof(sockaddr_in));
	receiveCommands();
}

