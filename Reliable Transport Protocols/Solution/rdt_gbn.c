#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ******************************************************************
ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: SLIGHTLY MODIFIED
FROM VERSION 1.1 of J.F.Kurose

This code should be used for PA2, unidirectional or bidirectional
data transfer protocols (from A to B. Bidirectional transfer of data
is for extra credit and is not required).  Network properties:
- one way network delay averages five time units (longer if there
are other messages in the channel for GBN), but can be larger
- packets can be corrupted (either the header or the data portion)
or lost, according to user-defined probabilities
- packets will be delivered in the order in which they were sent
(although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 0 /* change to 1 if you're doing extra credit */
/* and write a routine called B_output */

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg
{
	char data[20];
};

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt
{
	int seqnum;
	int acknum;
	int checksum;
	char payload[20];
};

/********* FUNCTION PROTOTYPES. DEFINED IN THE LATER PART******************/
void starttimer(int AorB, float increment);
void stoptimer(int AorB);
void tolayer3(int AorB, struct pkt packet);
void tolayer5(int AorB, char datasent[20]);

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* called from layer 5, passed the data to be sent to other side */
/* replace
AorB -> whichSide
*/
#define DEBUG 1
#define MAX_SEQ 7
#define WINDOW_SIZE 7
#define MAX_BUF 50
#define TIME_OUT 16
#define INC(a) ( a = (a+1)%(MAX_SEQ+1))
#define DEC(a) ((a+MAX_SEQ)%(MAX_SEQ+1))
#define INC_BY_WINDOW(a) (a = (a+1)%(WINDOW_SIZE))

int expected_ackFor_A;
int sendingSeqFor_A;
int packet_in_A_buffer;
int expected_frameFor_B;
struct pkt window_buffer[WINDOW_SIZE];
/* print packet content */
void print_packet(int whichSide,char* action,struct pkt packet)
{
	printf("[%d] %s: ", whichSide, action);
	printf("seq = %d ack = %d checksum = %x ", packet.seqnum, packet.acknum, packet.checksum);
	int i;
	for (i = 0; i < 20; i++)
		putchar(packet.payload[i]);
	putchar('\n');
}

/* compute checksum */
int compute_checkSum(struct pkt packet)
{
	int sum = 0, i = 0;
	sum += packet.seqnum;
	sum += packet.acknum;
	for (i = 0; i < 20; i ++) {
		sum += packet.payload[i];
	}
	return sum;
}

/* check if a <= b < c circularly */
int isBetween(int a,int b,int c)
{
	if ((a <= b && b < c)
		|| (c < a && a <= b)
		|| (b < c && c < a))
		return 1;
	else
		return 0;
}

int window_buffer_tail = 0;
/* construct a packet */
int window_buffer_head = 0;
struct pkt createPacket(struct msg message)
{
	struct pkt packet;
	memcpy(packet.payload, message.data, sizeof(message.data));
	packet.seqnum = sendingSeqFor_A;
	packet.acknum = -1;
	packet.checksum = compute_checkSum(packet);
	window_buffer[window_buffer_tail] = packet;
	window_buffer_tail = (window_buffer_tail + 1)% WINDOW_SIZE;
	INC(sendingSeqFor_A);
	packet_in_A_buffer++;
	return packet;
}

struct pkt copyPacket(struct pkt toBeCopied)
{
	struct pkt packet;
	memcpy(packet.payload, toBeCopied.payload, sizeof(toBeCopied.payload));
	packet.seqnum = toBeCopied.seqnum;
	packet.acknum = toBeCopied.acknum;
	packet.checksum = toBeCopied.checksum;
	return packet;
}

/* multitimer:
* start a timer for each packet using one timer
*/
struct pkt_timer{
	float timer_expire;
	int seqNum;
};

struct pkt_timer makeTimer(int seqNum,float expire)
{
	struct pkt_timer tmpTimer;
	tmpTimer.timer_expire = expire;
	tmpTimer.seqNum = seqNum;
	return tmpTimer;
}
#define TIMER_QUEUE_SIZE (WINDOW_SIZE + 2)  // 1 more for ack_timer
struct pkt_timer timers_queue[TIMER_QUEUE_SIZE];
int timers_running = 0;
int timerQueue_head = 0;
int timerQueue_tail = 0;
float time = 0.0;

/* call this function after the first timer goes off or was be closed */

/* start a timer for a packet */
void start_mytimer(int seqnum)
{
	/* bound check */
	//printf("%d-------------%d\n",timerQueue_head,timerQueue_tail );
	if (timerQueue_head == timerQueue_tail + 1) {
		printf("Warning: you can't create more than %d timers.\n", WINDOW_SIZE);
		return;
	}
	//printf("this %d timer is being onned -----------\n",seqnum );

	if (timers_running == 0) {    /* if timers isn't running, start the timer right now */
	timers_running = 1;
	starttimer(0, TIME_OUT);
	} else {                    /* else, add this timer into the queue */
	timers_queue[timerQueue_tail] = makeTimer(seqnum, time+TIME_OUT);
	timerQueue_tail = (timerQueue_tail +1)%TIMER_QUEUE_SIZE;
}
	//printf("%d-------------%d\n",timerQueue_head,timerQueue_tail );

}

/* stop the first timer */
void stop_mytimer(int seqnum)
{
	/* bound check */
	//	printf("%d-------------%d\n",timerQueue_head,timerQueue_tail );

	if (timers_running == 0) {
		printf("Warning: you are trying to stop a timer isn't running.\n");
		return;
	}
	/* stop the first timer */
	//printf("this %d timer is being offed -----------\n",seqnum );
	stoptimer(0);
	timers_running = 0;
	/* if there is more timer, run it right now */
	if (timerQueue_head != timerQueue_tail) {

		timers_running = 1;
		float increment = timers_queue[timerQueue_head].timer_expire - time;
		timerQueue_head = (timerQueue_head +1)%TIMER_QUEUE_SIZE;
		starttimer(0, increment);
	}
		//printf("%d-------------%d\n",timerQueue_head,timerQueue_tail );

}


/* queue:
* when message is out of the sender's window, put the messge in queue
*/
int msgQueue_head= 0;
int msgQueue_tail = 0;
struct msg msgQueue_buffer[MAX_BUF];

/* check if queue is empty */
int isMsgQueueEmpty() {
	return (msgQueue_head == msgQueue_tail) ? 1:0;
}

int isMsgQueueFull() {
	return (msgQueue_head == (msgQueue_tail + 1)%MAX_BUF) ? 1:0;
}
/* put message in queue */

void pushMsg(struct msg message)
{
	/* bound check */
	if (isMsgQueueFull()) {
		printf("Warning: there is no avaliable space in queue.\n");
		return;
	}
	msgQueue_buffer[msgQueue_tail] = message;
	INC(msgQueue_tail);
}

/* get messsage out of queue */
struct msg popMsg()
{
	/* bound check */
	if (isMsgQueueEmpty()) {
		printf("Warning: no packet in queue.\n");
		return;
	}
	struct msg message = msgQueue_buffer[msgQueue_head];
	INC(msgQueue_head);
	return message;
}


/* called from layer 5, passed the data to be sent to other side */

int numberOfAcked = 0;
/* called from layer 3, when a packet arrives for layer 4 */


/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
	if (packet_in_A_buffer < WINDOW_SIZE) {
		/* construct a packet */
		struct pkt packet = createPacket(message);
		tolayer3(0, packet);
		start_mytimer( packet.seqnum);
		/* debug output */
		if (DEBUG)
			print_packet(0, "Send", packet);
	} else {
		pushMsg( message);
	}

}

void B_output(struct msg message)
{
	
}

//changed 
// ack printing 
// seqnum er start
int how = 20;
void retransmitWindow(int isTimer_Interrupt){
	if(how == 100) return;
	how++;
	//printf("in retransmitWindow\n");
	if(!isTimer_Interrupt && timers_running !=0 ){
		stoptimer(0);
	}
		timers_running = 0;

	int seqnum;
	for (seqnum = window_buffer_head;  ; seqnum = (seqnum+1)%WINDOW_SIZE) {

		if (seqnum != window_buffer_head)
			stop_mytimer( seqnum);
		struct pkt pktToBeSent = copyPacket( window_buffer[seqnum]);
		pktToBeSent.acknum = -1;
		pktToBeSent.checksum = compute_checkSum(pktToBeSent);
		tolayer3(0, pktToBeSent);
		start_mytimer( seqnum);
		if(seqnum == (window_buffer_tail -1 +WINDOW_SIZE)%(WINDOW_SIZE))
			break;
		
	}
}
/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
	int tmp = numberOfAcked;
	if (DEBUG)
		print_packet(0,"Recieved", packet);
	//return;
	if(compute_checkSum(packet) != packet.checksum){
		return;
	}
	//printf("expected %d and recvd %d\n",expected_ackFor_A, packet.acknum );
	if(packet.acknum == DEC(expected_ackFor_A)){
		retransmitWindow(0);
		return;
	}
		while (isBetween(expected_ackFor_A, packet.acknum, sendingSeqFor_A)) {
			if (DEBUG)
				print_packet(0, "Acknowledged", window_buffer[window_buffer_head]);
			stop_mytimer( expected_ackFor_A);
			INC(expected_ackFor_A);
			INC_BY_WINDOW(window_buffer_head);
			packet_in_A_buffer--;
			numberOfAcked++;
		}
		while (packet_in_A_buffer < WINDOW_SIZE && !isMsgQueueEmpty()) {
			struct msg message = popMsg();
			struct pkt packet = createPacket(message);
			tolayer3(0, packet);

			start_mytimer( packet.seqnum);

			if (DEBUG)
				print_packet(0, "Send", packet);
		}
	
	
	

	if(tmp != numberOfAcked){
		printf("----------Total number of Acknowledged packet so far on %d side => %d ------------------\n",0,numberOfAcked );
	}
}
/* called when A's timer goes off */
void A_timerinterrupt()
{

	retransmitWindow(1);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	packet_in_A_buffer = 0;
	sendingSeqFor_A = 0;
	expected_ackFor_A = 0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{

	struct pkt replyPacket;
	print_packet(1,"received",packet);
	if(compute_checkSum(packet) != packet.checksum ||  expected_frameFor_B != packet.seqnum){
		replyPacket.acknum = DEC(expected_frameFor_B);

	}else {
		struct msg message;
		memcpy(message.data, packet.payload, sizeof(packet.payload));
		tolayer5(1, message.data);
		INC(expected_frameFor_B);
		replyPacket.acknum = packet.seqnum;

		if (DEBUG)
			print_packet(1, "Accepted", packet);
	}
	replyPacket.checksum = compute_checkSum(replyPacket);
	tolayer3(1, replyPacket);
		print_packet(1,"send",replyPacket);



}

/* called when B's timer goes off */
void B_timerinterrupt()
{
	
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	expected_frameFor_B = 0;

}

/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
- emulates the tranmission and delivery (possibly with bit-level corruption
and packet loss) of packets across the layer 3/4 interface
- handles the starting/stopping of a timer, and generates timer
interrupts (resulting in calling students timer handler).
- generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/

struct event
{
	float evtime;       /* event time */
	int evtype;         /* event type code */
	int eventity;       /* entity where event occurs */
	struct pkt *pktptr; /* ptr to packet (if any) assoc w/ this event */
	struct event *prev;
	struct event *next;
};
struct event *evlist = NULL; /* the event list */

/* possible events: */
#define TIMER_INTERRUPT 0
#define FROM_LAYER5 1
#define FROM_LAYER3 2

#define OFF 0
#define ON 1
#define A 0
#define B 1

int TRACE = 1;     /* for my debugging */
int nsim = 0;      /* number of messages from 5 to 4 so far */
int nsimmax = 0;   /* number of msgs to generate, then stop */
float lossprob;    /* probability that a packet is dropped  */
float corruptprob; /* probability that one bit is packet is flipped */
float lambda;      /* arrival rate of messages from layer 5 */
int ntolayer3;     /* number sent into layer 3 */
int nlost;         /* number lost in media */
int ncorrupt;      /* number corrupted by media*/

void init();
void generate_next_arrival(void);
void insertevent(struct event *p);

int main()
{
	struct event *eventptr;
	struct msg msg2give;
	struct pkt pkt2give;

	int i, j;
	char c;

	init();
	A_init();
	B_init();

	while (1)
	{
		eventptr = evlist; /* get next event to simulate */
		if (eventptr == NULL)
			goto terminate;
		evlist = evlist->next; /* remove this event from event list */
		if (evlist != NULL)
			evlist->prev = NULL;
		if (TRACE >= 2)
		{
			printf("\nEVENT time: %f,", eventptr->evtime);
			printf("  type: %d", eventptr->evtype);
			if (eventptr->evtype == 0)
				printf(", timerinterrupt  ");
			else if (eventptr->evtype == 1)
				printf(", fromlayer5 ");
			else
				printf(", fromlayer3 ");
			printf(" entity: %d\n", eventptr->eventity);
		}
		time = eventptr->evtime; /* update time to next event time */
		if (eventptr->evtype == FROM_LAYER5)
		{
			if (nsim < nsimmax)
			{
				if (nsim + 1 < nsimmax)
				generate_next_arrival(); /* set up future arrival */
				/* fill in msg to give with string of same letter */
					j = nsim % 26;
				for (i = 0; i < 20; i++)
					msg2give.data[i] = 97 + j;
				msg2give.data[19] = 0;
				if (TRACE > 2)
				{
					printf("          MAINLOOP: data given to student: ");
					for (i = 0; i < 20; i++)
						printf("%c", msg2give.data[i]);
					printf("\n");
				}
				nsim++;
				if (eventptr->eventity == A)
					A_output(msg2give);
				else
					B_output(msg2give);
			}
		}
		else if (eventptr->evtype == FROM_LAYER3)
		{
			pkt2give.seqnum = eventptr->pktptr->seqnum;
			pkt2give.acknum = eventptr->pktptr->acknum;
			pkt2give.checksum = eventptr->pktptr->checksum;
			for (i = 0; i < 20; i++)
				pkt2give.payload[i] = eventptr->pktptr->payload[i];
			if (eventptr->eventity == A) /* deliver packet by calling */
			A_input(pkt2give); /* appropriate entity */
			else
				B_input(pkt2give);
			free(eventptr->pktptr); /* free the memory for packet */
		}
		else if (eventptr->evtype == TIMER_INTERRUPT)
		{
			if (eventptr->eventity == A)
				A_timerinterrupt();
			else
				B_timerinterrupt();
		}
		else
		{
			printf("INTERNAL PANIC: unknown event type \n");
		}
		free(eventptr);
	}

	terminate:
	printf(
		" Simulator terminated at time %f\n after sending %d msgs from layer5\n",
		time, nsim);
}

	void init() /* initialize the simulator */
{
	int i;
	float sum, avg;
	float jimsrand();

	printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
	printf("Enter the number of messages to simulate: ");
	scanf("%d",&nsimmax);
	printf("Enter  packet loss probability [enter 0.0 for no loss]:");
	scanf("%f",&lossprob);
	printf("Enter packet corruption probability [0.0 for no corruption]:");
	scanf("%f",&corruptprob);
	printf("Enter average time isBetween messages from sender's layer5 [ > 0.0]:");
	scanf("%f",&lambda);
	printf("Enter TRACE:");
	scanf("%d",&TRACE);

		srand(9999); /* init random number generator */
		sum = 0.0;   /* test random number generator for students */
	for (i = 0; i < 1000; i++)
		sum = sum + jimsrand(); /* jimsrand() should be uniform in [0,1] */
		avg = sum / 1000.0;
	if (avg < 0.25 || avg > 0.75)
	{
		printf("It is likely that random number generation on your machine\n");
		printf("is different from what this emulator expects.  Please take\n");
		printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
		exit(1);
	}

	ntolayer3 = 0;
	nlost = 0;
	ncorrupt = 0;

		time = 0.0;              /* initialize time to 0.0 */
		generate_next_arrival(); /* initialize event list */
}

	/****************************************************************************/
	/* jimsrand(): return a float in range [0,1].  The routine below is used to */
	/* isolate all random number generation in one location.  We assume that the*/
	/* system-supplied rand() function return an int in therange [0,mmm]        */
	/****************************************************************************/
float jimsrand(void)
{
	double mmm = RAND_MAX;
		float x;                 /* individual students may need to change mmm */
		x = rand() / mmm;        /* x should be uniform in [0,1] */
	return (x);
}

	/********************* EVENT HANDLINE ROUTINES *******/
	/*  The next set of routines handle the event list   */
	/*****************************************************/

void generate_next_arrival(void)
{
	double x, log(), ceil();
	struct event *evptr;
	float ttime;
	int tempint;

	if (TRACE > 2)
		printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

		x = lambda * jimsrand() * 2; /* x is uniform on [0,2*lambda] */
		/* having mean of lambda        */
	evptr = (struct event *)malloc(sizeof(struct event));
	evptr->evtime = time + x;
	evptr->evtype = FROM_LAYER5;
	if (BIDIRECTIONAL && (jimsrand() > 0.5))
		evptr->eventity = B;
	else
		evptr->eventity = A;
	insertevent(evptr);
}

void insertevent(struct event *p)
{
	struct event *q, *qold;

	if (TRACE > 2)
	{
		printf("            INSERTEVENT: time is %lf\n", time);
		printf("            INSERTEVENT: future time will be %lf\n", p->evtime);
	}
		q = evlist;      /* q 2s to header of list in which p struct inserted */
		if (q == NULL)   /* list is empty */
	{
		evlist = p;
		p->next = NULL;
		p->prev = NULL;
	}
	else
	{
		for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next)
			qold = q;
			if (q == NULL)   /* end of list */
		{
			qold->next = p;
			p->prev = qold;
			p->next = NULL;
		}
			else if (q == evlist)     /* front of list */
		{
			p->next = evlist;
			p->prev = NULL;
			p->next->prev = p;
			evlist = p;
		}
			else     /* middle of list */
		{
			p->next = q;
			p->prev = q->prev;
			q->prev->next = p;
			q->prev = p;
		}
	}
}

void printevlist(void)
{
	struct event *q;
	int i;
	printf("--------------\nEvent List Follows:\n");
	for (q = evlist; q != NULL; q = q->next)
	{
		printf("Event time: %f, type: %d entity: %d\n", q->evtime, q->evtype,
			q->eventity);
	}
	printf("--------------\n");
}

	/********************** Student-callable ROUTINES ***********************/

	/* called by students routine to cancel a previously-started timer */
	void stoptimer(int whichSide /* A or B is trying to stop timer */)
{
	struct event *q, *qold;

	if (TRACE > 2)
		printf("          STOP TIMER: stopping timer at %f\n", time);
		/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
	for (q = evlist; q != NULL; q = q->next)
		if ((q->evtype == TIMER_INTERRUPT && q->eventity == whichSide))
		{
			/* remove this event */
			if (q->next == NULL && q->prev == NULL)
			evlist = NULL;          /* remove first and only event on list */
			else if (q->next == NULL) /* end of list - there is one in front */
				q->prev->next = NULL;
			else if (q == evlist)   /* front of list - there must be event after */
			{
				q->next->prev = NULL;
				evlist = q->next;
			}
			else     /* middle of list */
			{
				q->next->prev = q->prev;
				q->prev->next = q->next;
			}
			free(q);
			return;
		}
		printf("Warning: unable to cancel your timer. It wasn't running.\n");
	}

	void starttimer(int whichSide /* A or B is trying to start timer */, float increment)
	{
		struct event *q;
		struct event *evptr;

		if (TRACE > 2)
			printf("          START TIMER: starting timer at %f\n", time);
		/* be nice: check to see if timer is already started, if so, then  warn */
		/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
		for (q = evlist; q != NULL; q = q->next)
			if ((q->evtype == TIMER_INTERRUPT && q->eventity == whichSide))
			{
				printf("Warning: attempt to start a timer that is already started\n");
				return;
			}

		/* create future event for when timer goes off */
			evptr = (struct event *)malloc(sizeof(struct event));
			evptr->evtime = time + increment;
			evptr->evtype = TIMER_INTERRUPT;
			evptr->eventity = whichSide;
			insertevent(evptr);
		}

	/************************** TOLAYER3 ***************/
		void tolayer3(int whichSide, struct pkt packet)
		{
			struct pkt *mypktptr;
			struct event *evptr, *q;
			float lastime, x;
			int i;

			ntolayer3++;

		/* simulate losses: */
			if (jimsrand() < lossprob)
			{
				nlost++;
				if (TRACE > 0)
					printf("          TOLAYER3: packet being lost\n");
				return;
			}

		/* make a copy of the packet student just gave me since he/she may decide */
		/* to do something with the packet after we return back to him/her */
			mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
			mypktptr->seqnum = packet.seqnum;
			mypktptr->acknum = packet.acknum;
			mypktptr->checksum = packet.checksum;
			for (i = 0; i < 20; i++)
				mypktptr->payload[i] = packet.payload[i];
			if (TRACE > 2)
			{
				printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
					mypktptr->acknum, mypktptr->checksum);
				for (i = 0; i < 20; i++)
					printf("%c", mypktptr->payload[i]);
				printf("\n");
			}

		/* create future event for arrival of packet at the other side */
			evptr = (struct event *)malloc(sizeof(struct event));
		evptr->evtype = FROM_LAYER3;      /* packet will pop out from layer3 */
		evptr->eventity = (whichSide + 1) % 2; /* event occurs at other entity */
		evptr->pktptr = mypktptr;         /* save ptr to my copy of packet */
		/* finally, compute the arrival time of packet at the other end.
		medium can not reorder, so make sure packet arrives isBetween 1 and 10
		time units after the latest arrival time of packets
		currently in the medium on their way to the destination */
			lastime = time;
		/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
			for (q = evlist; q != NULL; q = q->next)
				if ((q->evtype == FROM_LAYER3 && q->eventity == evptr->eventity))
					lastime = q->evtime;
				evptr->evtime = lastime + 1 + 9 * jimsrand();

		/* simulate corruption: */
				if (jimsrand() < corruptprob)
				{
					ncorrupt++;
					if ((x = jimsrand()) < .75)
			mypktptr->payload[0] = 'Z'; /* corrupt payload */
						else if (x < .875)
							mypktptr->seqnum = 999999;
						else
							mypktptr->acknum = 999999;
						if (TRACE > 0)
							printf("          TOLAYER3: packet being corrupted\n");
					}

					if (TRACE > 2)
						printf("          TOLAYER3: scheduling arrival on other side\n");
					insertevent(evptr);
				}

				void tolayer5(int whichSide, char datasent[20])
				{
					int i;
					if (TRACE > 2)
					{
						printf("          TOLAYER5: data received: ");
						for (i = 0; i < 20; i++)
							printf("%c", datasent[i]);
						printf("\n");
					}
				}
