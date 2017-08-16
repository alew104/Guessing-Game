//lewa1 (Alex Lew)
// ports: 10,800 - 10,899
// ip:
// This is the client program for a guessing game

#include <iostream>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> // size_t, ssize_t
#include <sys/socket.h> // socket funcs
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h> // htons, inet_pton

using namespace std;

const int PORTMAX = 10899; // max port
const int PORTMIN = 10800; // min port

long getGuess();
void sendString(string s, int sock);
void sendLong(long l, int sock);
long recvLong(int sock);
string recvString(int sock);



int main(int argc, char * argv[]) {

	// getting args
	if (argc != 3)
	{
		cerr << "Please input both the IP Address and the port in the arguments as 1 and 2 respectively.";
		exit(-1);
	}


	unsigned short servPort = (unsigned short)strtoul(argv[2], NULL, 0);
	if (servPort > PORTMAX || servPort < PORTMIN)
	{
		cerr << "Invalid Ports, please use a portnumber between 10800 and 10899 inclusive.";
		exit(-1);
	}

	bool end = false;

	unsigned long servIP;
	int status = inet_pton(AF_INET, argv[1], &servIP);
	if (status <= 0) {
		cerr << "Error with pton";
		exit(-1);
	}
	struct sockaddr_in servAddr;
	servAddr.sin_family = AF_INET; // always AF_INET
	servAddr.sin_addr.s_addr = servIP;
	servAddr.sin_port = htons(servPort);

	//initializing the socket & checking if there is an error
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //creating socket
	if (sock < 0) {
		cerr << "Error with socket" << endl;
		exit(-1);
	}

	//connecting to ipAddress with given socket
	status = connect(sock, (struct sockaddr*) &servAddr, sizeof(servAddr));
	if (status < 0) {
		cerr << "Connection Error" << endl;
		exit(-1);
	}

	// success text
	cout << "Connection Successful." << endl;
	cout << "Welcome to the guessing game. Guess a number between 0-9999." << endl;
	cout << "The difference is the sum of the difference between each digit." << endl;

	// get username
	string name = "";
	cout << "Enter your name: " << endl;
	cin >> name;
    sendString(name, sock);

	// game operations
	do {
        long guess = getGuess();
        //cerr << "Sending Guess. " << guess << endl;
        sendLong(guess, sock);
        long result = recvLong(sock);
        //cerr << "Result of guess: " << result << endl;
        if (result == 1){
            //cerr << "Your guess was right!" << endl;
			long difference = recvLong(sock);
			cerr << "The difference is: " << difference << endl;
            end = true;
        } else {
            cerr << "Guess was wrong." << endl;
			long difference = recvLong(sock);
			cerr << "The difference is: " << difference << endl;
        }
	} while (!end);

	// receive victory message
	string victory = recvString(sock);
	// output victory message
	long numRounds = recvLong(sock);
	// receive leaderboard
	cerr << victory << " It took you " << to_string(numRounds) << " turns to guess the number!" << endl;
	// output leaderboard
	string leaderboard = recvString(sock);
	cerr << endl << leaderboard << endl;

	status = close(sock);
	if (status < 0) {
		cerr << "Error with close" << endl;
		exit(-1);
	}

	return 0;
}

// guess loop
// Validates input and only exits loop until the guess is valid.
long getGuess() {
	long input = -1;
    while (input < 0 || input > 9999){
        cout << "Enter a guess: " << endl;
        cin >> input;
        if (input < 0 || input > 9999){
            cerr << "Your guess is invalid. Please enter a number between 0 "
                << "and 9999 inclusive"
                << endl;
        }
    }
	return input;
}

//send string
// @param sock The socket passed to allow sending
// @param The string to send to the server
void sendString(string s, int sock) {
	long size = (s.length() + 1);
	char msg[size];
	strcpy(msg, s.c_str());

	sendLong(size, sock);

	int bytesSent = send(sock, (void *)msg, size, 0);
	if (bytesSent != size) {
		cerr << "error sending string" << endl;
		exit(-1);
	}
}

// send long
// @param sock The socket passed to allow sending
// @param l The number to send to the server
void sendLong(long l, int sock) {
	long convert = htonl(l);
    //cerr << "Sending long " << convert << endl;
	int bytesSent = send(sock, (void *)&convert, sizeof(long), 0);
	if (bytesSent != sizeof(long)) {
		cerr << "error sending long" << endl;
		exit(-1);
	}
}

// receive long
// @param sock The socket passed to allow receiving
// @return The long received from the server
long recvLong(int sock) {
	int bytesLeft = sizeof(long);
	long reception;
	char *bp = (char*)&reception;
	while (bytesLeft > 0) {
		int bytesRecv = recv(sock, (void *)bp, bytesLeft, 0);
		if (bytesRecv <= 0) {
			cerr << "error receiving long" << endl;
			exit(-1);
		}
		bytesLeft = bytesLeft - bytesRecv;
		bp = bp + bytesRecv;
	}
	return ntohl(reception);
}

// receive string
// @param sock The socket passed to allow receiving
// @return The string received from the server
string recvString(int sock) {
	int bytesLeft = recvLong(sock);
	char buffer[bytesLeft];
	char *bp = buffer;
	while (bytesLeft > 0) {
		int bytesRecv = recv(sock, (void *)bp, bytesLeft, 0);
		if (bytesRecv <= 0) {
			cerr << "error receiving string" << endl;
			exit(-1);
		}
		bytesLeft = bytesLeft - bytesRecv;
		bp = bp + bytesRecv;
	}
	return string(buffer);
}
