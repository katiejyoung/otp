#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues
int isNewline(char arr[1001]);
char* toCipher(char key[100000], char plaintext[100000]);
int addChars(char keyChar, char ptChar);
int getSubstring(char* substring, char fullString[100000]);

int substrIdx = 0; // Global variable to track substring index starting point

int main(int argc, char *argv[])
{
	// Original child process
	pid_t childPID = -5; 
    int childExitStatus = -5;

	// Background child process
	pid_t childPID_Background = -5;
    int childExitStatus_Background = -5;
    int childProcess = -5; // Tracker for element number in childPID array

	int listenSocketFD, establishedConnectionFD, portNumber, charsRead, stringLength, keyInput, dataSize;
	socklen_t sizeOfClientInfo;
	char key[100000], plaintext[100000], ciphertext[100000];
	char sendChar[1001], buffer[1001];
	char* s = sendChar;
	struct sockaddr_in serverAddress, clientAddress;
	int newline = 0;
	int sendCharLength = 0;
	char *socketName = "otp_enc";

	// Check argument length
	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); }

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("ERROR opening socket");

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) { // Connect socket to port
		perror("Error: on binding");
		exit(2);
	}

	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

	// Accept a connection, blocking if one is not available until one connects
    while (1) {
		memset(key, '\0', sizeof(key)); // Clear out the key array
		memset(plaintext, '\0', sizeof(plaintext)); // Clear out the plaintext array
		memset(ciphertext, '\0', sizeof(ciphertext)); // Clear out the ciphertext array
		sendCharLength = 0; // Reset length counter
		substrIdx = 0; // Reset substring length tracker
		dataSize = 0; // Reset data size tracker

		// Check for terminated child process
        childPID_Background = waitpid(-1, &childExitStatus_Background, WNOHANG);

        // If background process terminated
        if (childPID_Background > 0) {
            // Decrement child process count
            childProcess = childProcess - 1;
		}

		// Accept connection to client
        sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
        establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
        if (establishedConnectionFD < 0) error("ERROR on accept");

		// Create child process
		childProcess = childProcess + 1; // Increment child process counter for main process
        childPID = fork();

		 // Evaluate childPID creation status
		switch (childPID)
		{
			case -1: // Error creating child process
				perror("Error creating new child process"); 
				childProcess = childProcess - 1; // Decrement fork counter if process failed
				exit(1);
				break;
			case 0: // Child process created successfully
				// Read name of client connection
				memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
				charsRead = recv(establishedConnectionFD, buffer, sizeof(buffer), 0); // Read the client's message from the socket

				// Return name of server connection
				keyInput = send(establishedConnectionFD, socketName, sizeof(socketName), 0); // Write to the client
				if (keyInput < 0) { error("ERROR writing to socket"); } // Output if error writing
				if (keyInput < sendCharLength) { printf("WARNING: Not all data written to socket!\n"); fflush(stdout); } // Output if error with data sent

				// Only proceed if client connection name is correct
				if (strcmp(buffer, socketName) == 0) {
					// Retrieve key
					do { // Loop until end of key found
						// Receive size of incoming data and key substring
						charsRead = recv(establishedConnectionFD, &dataSize, sizeof(int), 0); // Read the client's message from the socket
						memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
						charsRead = recv(establishedConnectionFD, buffer, dataSize, 0); // Read the client's message from the socket
						if (charsRead < 0) { error("ERROR reading from socket"); }; // Error if no data received

						strcat(key, buffer); // Concatenate buffer to plaintext variable
						newline = isNewline(buffer); // Check for end of string
					} while(newline == 0);

					newline = 0; // Reset newline
					dataSize = 0; // Reset data size

					// Retrieve plaintext
					do {
						// Receive size of incoming data and key substring
						charsRead = recv(establishedConnectionFD, &dataSize, sizeof(int), 0); // Read the client's message from the socket
						memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
						charsRead = recv(establishedConnectionFD, buffer, dataSize, 0); // Read the client's message from the socket
						if (charsRead < 0) { error("ERROR reading from socket"); }; // Error if no data received

						strcat(plaintext, buffer); // Concatenate buffer to key variable		
						newline = isNewline(buffer); // Check for end of string
					} while(newline == 0);

					// Remove trailing newlines from key and plaintext
					key[strcspn(key, "\n")] = 0;
					plaintext[strcspn(plaintext, "\n")] = 0;
					
					// Encipher plaintext using key then re-introduce newline
					strcpy(ciphertext, toCipher(key, plaintext)); 
					strcat(ciphertext, "\n");
					
					stringLength = strlen(ciphertext);
					// Slice ciphertext into substring(s) with max length 1000 and send to client
					do { // Loop until last substring
						memset(sendChar, '\0', sizeof(sendChar)); // Reset sendChar variable
						sendCharLength = getSubstring(s, ciphertext); // Get substring of key with max 1000 characters

						// Send data size and substring to server
						keyInput = send(establishedConnectionFD, &sendCharLength, sizeof(int), 0); // Write to the server
						keyInput = send(establishedConnectionFD, sendChar, sendCharLength, 0); // Write to the client
						if (keyInput < 0) { error("ERROR writing to socket"); } // Output if error writing

						stringLength = stringLength - keyInput; // Decrement string length
					} while(stringLength > 0);

					close(establishedConnectionFD); // Close the existing socket which is connected to the client
					exit(0); // Exit child process
				}
				else {
					perror("Error: Connection rejected");
					close(establishedConnectionFD); // Close the existing socket which is connected to the client
					exit(2); // Exit child process
				}
				
				break;
			default:
				// Wait until system can handle more child processes (max 5)
				while (childProcess >= 5) {
        			// Check for terminated child process
					childPID_Background = waitpid(-1, &childExitStatus_Background, WNOHANG);

					// If background process terminated
					if (childPID_Background > 0) {
						// Decrement child process count
						childProcess = childProcess - 1;
					}
				}
				
				break;
		}
			
	}

	close(listenSocketFD); // Close the listening socket
	return 0; 
}

// Detects newline characters in passed char array
// Returns 1 if newline found
// Returns 0 if no newline found
int isNewline(char arr[1001]) {
	int i;

	// Loop through buffer input
	for (i = 0 ; i < 1000; i++) {
		// Break if end condition met
		if (arr[i] == '\n') {
			return 1;
		}
	}

	// Return 0 if no newline found
	return 0;
}

// Enciphers message using passed key
// Returns enciphered message
char* toCipher(char key[100000], char plaintext[100000]) {
	static char cipher[100000]; // Ciphertext char array
	int msgASCII, keyASCII; // ASCII values of characters
	int i; // Iterator
	int asciiInt;

	// Iterate through plaintext and use key to encipher
	for (i = 0; i < strlen(plaintext); i++) {
		asciiInt = addChars(key[i], plaintext[i]); // Convert to encoded ASCII number
		cipher[i] = asciiInt; // Save to character array
	}

	return cipher;
}

// Calculates encipherment value of two characters
// Returns modulus of ASCII integer sum
int addChars(char keyChar, char ptChar) {
	// Convert characters to integer
	int k = keyChar;
	int p = ptChar;

	if (k == 32) {
		k = 91;
	}
	if (p == 32) {
		p = 91;
	}

	k = k - 65;
	p = p - 65;

	int sum = k + p; // Add integers

	sum = sum % 27; // Find modulus
	sum = sum + 65; // Increase to range


	// Convert to space char if applicable
	if (sum == 91) {
		sum = 32;
	}

	return sum;
}

// Generates substring of given string with max 1000 characters
// Returns size of substring
int getSubstring(char* substring, char fullString[100000]) {
	int i;

	// Copy up to 1000 characters to substring
	for (i = 0; i < 1000; i++) {
		substring[i] = fullString[substrIdx];
		substrIdx = substrIdx + 1; // Increment substring index variable

		// Return if \n reached before reaching 1000
		if (fullString[substrIdx - 1] == '\n') {
			i = i + 1; // Increment i for newline
			return i;
		}
	}

	// Return if 1000 reached before newline
	return i;
}