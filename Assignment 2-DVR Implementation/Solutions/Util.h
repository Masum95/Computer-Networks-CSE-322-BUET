#include <bits/stdc++.h>
#define SEND "send"
#define FORWARD "frwd"
#define COST_UPDATE "cost"
#define SHOW_TABLE  "show"
#define TABLE_RECEIVE "rtbl"
#define UNDEFINED "-"
#define MAX_TTL 3
#define INF 9999

using namespace std;

class RoutingTableEntry
{
public:
  string destination;
  string nextHop;
  int cost;
  RoutingTableEntry(){}
  RoutingTableEntry(string destination,string nextHop,int cost){
    this->destination = destination;
    this->nextHop = nextHop;
    this->cost = cost;
  }
  RoutingTableEntry(const RoutingTableEntry &rt){
    destination = rt.destination;
    nextHop = rt.nextHop;
    cost = rt.cost;
  }
};

class Edge
{
public:
  string neighbor;
  int cost;
  int ttl;
  Edge(){}

  Edge(string nbr,int cost){
    neighbor = nbr , this->cost = cost , ttl = 3 ;
  }
  void setEdge(string nbr,int cost){
    neighbor = nbr , this->cost = cost , ttl = 3;
  }
};

vector<string> stringSplit(string data,char delim){
  stringstream ss(data);
  string token;
  vector<string> cont;

  while (getline(ss, token, delim)) {
    cont.push_back(token);
  }
  return cont;
}
int addEdgeCost(int c1,int c2){
  int res = c1 + c2;
  return res>=INF? INF: res;
}

string mergeVectorToString(vector<string> msgParts,char delim){
  string msg = "";
  for (int i=0;i<msgParts.size();i++) {
    msg += msgParts[i];
    if(i!=msgParts.size()-1) msg+= delim;
  }
  return msg;

}

bool sendData(string msg,string destination,int sockfd){
  struct sockaddr_in routerAddress;

  routerAddress.sin_family = AF_INET;
	routerAddress.sin_port = htons(4747);
	routerAddress.sin_addr.s_addr = inet_addr(destination.c_str());

  int sent_bytes = sendto(sockfd, msg.c_str(), 1024, 0, (struct sockaddr*) &routerAddress, sizeof(sockaddr_in));
  if(sent_bytes!=-1){
    return 1;
  }
  return 0;
}


