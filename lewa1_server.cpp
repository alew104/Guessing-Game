//lewa1 (Alex Lew)
// ports: 10,800 - 10,899
// ip: 10.124.72.20
// This is the server program for a guessing game

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <string>
#include <cmath>
#include <vector>
#include <pthread.h>
#include <algorithm>
#include <sys/types.h> // size_t, ssize_t
#include <sys/socket.h> // socket funcs
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h> // htons, inet_pton
#include <unistd.h>

using namespace std;

const int MAXCONCURRENT = 10; // max threads playing
const int PORTMAX = 10899; // max port
const int PORTMIN = 10800; // min port
vector<string> names;
vector<long> roundNums;



struct gameInfo {
	int sock;
};

pthread_mutex_t leaderboardLock;

void sendString(string s, gameInfo info);
void sendLong(long l, gameInfo info);
string recvString(gameInfo info);
long recvLong(gameInfo info);
string convertToFourDigits(long l);
int calculateDifference(long guess, long randNum);
void addToLeaderboard(int numRounds, string name);
string outputLeaderboard();
void sortLeaderboard();



void* game(void* i) {

	//get sock from struct
    gameInfo *info;
    info = (gameInfo*)i;

	// game variables
    int numRounds = 1;
    bool gameWon = false;

	// free resources after close
	pthread_detach(pthread_self());
	// get name
    string name = recvString(*info);
    cerr << "Your name is: " << name << endl;
	// new number to guess
	srand(time(NULL));
	long randNum = rand() % 9999;

	// debug message
    cerr << "****" << randNum << "****" << endl;

    do{
		// receive guess
        long guess = recvLong(*info);
        cerr << "Guess is " << guess << endl;
        if (guess != randNum){
            cerr << "Guess was wrong." << endl;
			numRounds = numRounds + 1;
			// send status
            sendLong(0, *info);
			// send difference
			sendLong(calculateDifference(guess, randNum), *info);
        } else {
			// send status
            sendLong(1, *info);
			// send difference
			sendLong(calculateDifference(guess, randNum), *info);
			// stop game loop
            gameWon = true;
        }
    } while(!gameWon);

	sendString("You won!", *info);
	sendLong(numRounds, *info);
	// send round count
	// lock board
	pthread_mutex_lock(&leaderboardLock);
	// add to leaderboard
	addToLeaderboard(numRounds, name);
	string leaderboard = outputLeaderboard();
	// unlock board
	pthread_mutex_unlock(&leaderboardLock);
	// send leaderboard
	cerr << leaderboard << endl;
	sendString(leaderboard, *info);

	cerr << "****Game Over****" << endl;
	// close socket
	close(info->sock);
	cerr << "****Socket Closed****" << endl;
	cerr << "****Closing Thread****" << endl;
	// free struct
	free(info);
	// close thread
	pthread_exit(NULL);

}


int main(int argc, char * argv[]) {

	// getting args
	if (argc != 2)
	{
		cerr << "Please input the port in the arguments." << endl;
		exit(-1);
	}


	unsigned short servPort = (unsigned short)strtoul(argv[1], NULL, 0);
	cerr << "Port is " << to_string(servPort) << endl;
	if (servPort > PORTMAX || servPort < PORTMIN)
	{
		cerr << "Invalid Ports, please use a port number between 10800 and 10899 inclusive." << endl;
		exit(-1);
	}

	// set connection information
	unsigned long servIP;
	int status;
	struct sockaddr_in servAddr;
	servAddr.sin_family = AF_INET; // always AF_INET
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(servPort);

	// initialize socket
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //creating socket
	if (sock < 0) {
		cerr << "Socket creation error" << endl;
		exit(-1);
	}

	// bind socket
	status = bind(sock, (struct sockaddr *) &servAddr, sizeof(servAddr));
	if (status < 0) {
		cerr << "Error with bind" << endl;
		exit(-1);
	}

	// begin listening
	status = listen(sock, MAXCONCURRENT);
	if (status < 0) {
		cerr << "Error with listen" << endl;
		exit(-1);
	}

	// continue until abort
	while (true) {
		cerr << "****Listening****" << endl;
        pthread_t tid;

		struct sockaddr_in clientAddr;
		socklen_t addrLen = sizeof(clientAddr);
		int clientSock = accept(sock, (struct sockaddr *) &clientAddr, &addrLen);
		cerr << "****NEW CLIENT****" << endl;
		if (clientSock < 0) {
			cerr << "Error with accept. Now exiting program. " << endl;
			close(clientSock);
			exit(-1);
		}
		gameInfo *info = new gameInfo;
		info->sock = clientSock;

        status = pthread_create(&tid, NULL, game, (void*)info);
        if(status){
            cerr<<"Error creating threads." <<endl;
            close(clientSock);
            exit(-1);
        }
        cerr << "****Client thread started.****" << endl;


	}
	return 0;
}

// send string
// @param s The string to sent to the client
// @param info The struct with the socket passed to allow receiving
void sendString(string s, gameInfo info) {
	long size = (s.length() + 1);
	char msg[size];
	strcpy(msg, s.c_str());

	sendLong(size, info);

	int bytesSent = send(info.sock, (void *)msg, size, 0);
	if (bytesSent != size) {
		cerr << "error sending string" << endl;
        cerr << "closing thread" << endl;
        pthread_exit(NULL);
	}
}

// send long
// @param l The number to sent to the client
// @param info The struct with the socket passed to allow receiving
void sendLong(long l, gameInfo info) {
	long convert = htonl(l);
	int bytesSent = send(info.sock, (void *)&convert, sizeof(long), 0);
	if (bytesSent != sizeof(long)) {
		cerr << "error sending long" << endl;
        cerr << "closing thread" << endl;
        pthread_exit(NULL);
	}
}

// receive long
// @param info The struct with the socket passed to allow receiving
long recvLong(gameInfo info) {
	int bytesLeft = sizeof(long);
	long reception;
	char *bp = (char*)&reception;
	while (bytesLeft > 0) {
		int bytesRecv = recv(info.sock, (void *)bp, bytesLeft, 0);
		if (bytesRecv <= 0) {
			cerr << "error receiving long" << endl;
            cerr << "closing thread" << endl;
            pthread_exit(NULL);
		}
		bytesLeft = bytesLeft - bytesRecv;
		bp = bp + bytesRecv;
	}
	long result = ntohl(reception);
	// << "Received Long " << reception << endl;
	//cerr << "Received conversion " << result << endl;
	return result;
}

// receive string
// @param info The struct with the socket passed to allow receiving
string recvString(gameInfo info) {
	int bytesLeft = recvLong(info);
	char buffer[bytesLeft];
	char *bp = buffer;
	while (bytesLeft > 0) {
		int bytesRecv = recv(info.sock, (void *)bp, bytesLeft, 0);
		if (bytesRecv <= 0) {
			cerr << "error receiving string" << endl;
            cerr << "closing thread" << endl;
            pthread_exit(NULL);
		}
		bytesLeft = bytesLeft - bytesRecv;
		bp = bp + bytesRecv;
	}
	return string(buffer);
}

// convert ints to strings for character analysis
string convertToFourDigits(long l) {
	string result = "";
	if (l < 10) {
		result = "000";
	}
	else if (l >= 10 && l < 100) {
		result = "00";
	}
	else if (l >= 100 && l < 1000) {
		result = "0";
	}
	//cerr << "String conversion result: " << result << to_string(l) << endl;
	return result + to_string(l);
}

// calculate differences
// @param guess The number guessed that was received from client
// @param randNum The correct number determined by the server
int calculateDifference(long guess, long randNum) {
	string guessStr = convertToFourDigits(guess);
	string randStr = convertToFourDigits(randNum);
	int result = 0;
	for (int i = 0; i < 4; i++) {
		char g = guessStr.at(i);
		char r = randStr.at(i);
		int rand = r - '0';
		int guess = g - '0';
		result = result + (max(rand, guess) - min(rand, guess));
	}
	//cerr << "Difference is " << result << endl;
	return result;
}

// add to leaderboard
// @param numRounds The number of rounds that the player took
// @param name The name of the player
void addToLeaderboard(int numRounds, string name) {
	// if empty merely populate and sort
	if (roundNums.size() < 3) {
		roundNums.push_back(numRounds);
		names.push_back(name);
		sortLeaderboard();
	}
	else {
		// if full, insert and resize to remain size 3
		for (int i = 0; i < roundNums.size(); i++) {
			if (numRounds < roundNums[i]) {
				roundNums.insert(roundNums.begin() + i, numRounds);
				roundNums.resize(3);
				names.insert(names.begin() + i, name);
				names.resize(3);
				i = roundNums.size();
			}
		}
	}
}

// use selection sort to sort the leaderboard
void sortLeaderboard() {
	for (int i = 0; i < roundNums.size() - 1; i++) {
		int iMin = i;
		for (int j = i + 1; j < roundNums.size(); j++) {
			if (roundNums[j] < roundNums[iMin]) {
				iMin = j;
			}
		}
		if (iMin != i) {
			int tempNum = roundNums[i];
			string tempName = names[i];
			roundNums[i] = roundNums[iMin];
			names[i] = names[iMin];
			roundNums[iMin] = tempNum;
			names[iMin] = tempName;
		}
	}
}

// create the string for leaderboard
string outputLeaderboard() {
	string result = "LEADERBOARD \n";
	for (int i = 0; i < roundNums.size(); i++) {
		result = result + names[i] + ": " + to_string(roundNums[i]) + "\n";
	}
	return result;
}
