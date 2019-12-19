#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

void error(const char *msg) { perror(msg); exit(0); } // Error function used for reporting issues
int getSubstring(char* substring, char fullString[100000]);
int checkKeyRatio(char k[100000], char p[100000], char kFile[256]);
int validateChars(char p[100000]);
int isNewline(char arr[1001]);

int substrIdx = 0; // Global variable to track substring index starting point

int main(int argc, char *argv[])
{
	int socketFD, portNumber, keyInput, charsRead, stringLength, isValid, dataSize, i;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[100000];
	char plainText[100000], encipheredText[100000], keyString[100000];
	char sendChar[1001], receiveChar[1001];
	char* b = buffer;
	char* s = sendChar;
	int sendCharLength = 0;
	int newline = 0;

    char *hostName = "localhost";
	char *socketName = "otp_enc";
	FILE *key, *plaintext, *ciphertext;
	size_t len = 100000;
	size_t nread;
    
	// Check argument length
	if (argc < 3) { fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); exit(2); } // Check usage & args

	// Open key file and check for open success
	key = fopen(argv[2], "r");
	if (key == NULL) {
		perror("fopen");
		exit(1);
	}

	// Open plaintext file and check for open success
	plaintext = fopen(argv[1], "r");
	if (plaintext == NULL) {
		perror("fopen");
		exit(1);
	}

	// Read key from file
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
	nread = getline(&b, &len, key); // Read from key file into buffer variable
	strcpy(keyString, buffer); // Save buffer value to key variable

	// Read plaintext from file
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
	nread = getline(&b, &len, plaintext); // Read from plaintext file into buffer variable
	strcpy(plainText, buffer); // Save buffer value to plaintext variable
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
	
	// Validate key/plaintext ratio
	isValid = checkKeyRatio(keyString, plainText, argv[2]); // Check ratio of key and plaintext

	// Validate plaintext characters
	if (isValid == 1) {
		isValid = validateChars(plainText); // Validate characters
	}

	// End program if invalid argument passed
	if (isValid == 0) {
		exit(1);	
	}
	else {
		// Set up the server address struct
		memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
		portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
		serverAddress.sin_family = AF_INET; // Create a network-capable socket
		serverAddress.sin_port = htons(portNumber); // Store the port number
		serverHostInfo = gethostbyname(hostName); // Convert the machine name into a special form of address
		if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
		memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address
		
		substrIdx = 0; // Reset substring variable

		// Set up the socket
		socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
		if (socketFD < 0) {
			error("CLIENT: ERROR opening socket");
		}
		
		// Connect to server
		if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) { // Connect socket to address
			error("Error: connecting");
		}

		// Send name of client for server validaton
		keyInput = send(socketFD, socketName, sizeof(socketName), 0); // Write to the server
		if (keyInput < 0) { error("CLIENT: ERROR writing to socket"); } // Output if error writing
		if (keyInput < sendCharLength) { printf("CLIENT: WARNING: Not all data written to socket!\n"); fflush(stdout); } // Output if error with data sent

		// Receive name back from server
		memset(receiveChar, '\0', sizeof(receiveChar)); // Clear out the receiveChar array
		charsRead = recv(socketFD, receiveChar, sizeof(receiveChar), 0); // Read the server's message from the socket

		// Continue if correct server was called
		if (strcmp(receiveChar, socketName) == 0) {
			stringLength = strlen(keyString); // Set length variable for full string
			
			// Slice key into substring(s) with max length 1000 and send to server
			do { // Loop until last substring
				memset(sendChar, '\0', sizeof(sendChar)); // Reset sendChar variable
				sendCharLength = getSubstring(s, keyString); // Get substring of key with max 1000 characters

				// Send data size and substring to server
				keyInput = send(socketFD, &sendCharLength, sizeof(int), 0); // Write to the server
				keyInput = send(socketFD, sendChar, sendCharLength, 0); // Write to the server
				if (keyInput < 0) { error("CLIENT: ERROR writing to socket"); } // Output if error writing
				
				stringLength = stringLength - keyInput; // Decrement stringLength counter				
			} while(stringLength > 0);

			substrIdx = 0; // Reset substring variable	
			fclose(key); // Close key file

			stringLength = strlen(plainText); // Set length variable for full string
			
			// Slice plaintext into substring(s) with max length 1000 and send to server
			do { // Loop until last substring
				memset(sendChar, '\0', sizeof(sendChar)); // Reset sendChar variable
				sendCharLength = getSubstring(s, plainText); // Get substring of key with max 1000 characters

				// Send data size and substring to server
				keyInput = send(socketFD, &sendCharLength, sizeof(int), 0); // Write to the server
				keyInput = send(socketFD, sendChar, sendCharLength, 0); // Write to the server
				if (keyInput < 0) { error("CLIENT: ERROR writing to socket"); } // Output if error writing

				stringLength = stringLength - keyInput; // Decrement stringLength counter
			} while(stringLength > 0);

			fclose(plaintext); // Close plaintext file	

			// Loop until end of ciphertext found
			do {
				// Receive size of incoming data and key substring
				charsRead = recv(socketFD, &dataSize, sizeof(int), 0); // Read the client's message from the socket
				memset(receiveChar, '\0', sizeof(receiveChar)); // Clear out the buffer array
				charsRead = recv(socketFD, receiveChar, dataSize, 0); // Read the server's message from the socket
				if (charsRead < 0) { error("ERROR reading from socket"); }; // Error if no data received

				strcat(encipheredText, receiveChar); // Concatenate buffer to ciphertext variable
				newline = isNewline(receiveChar); // Check for end of string
			} while(newline == 0);

			// Print enciphered text to standard output
			printf("%s", encipheredText);
			fflush(stdout);
		}
		
		close(socketFD); // Close the socket
		exit(0);
	}
	
	return 0;
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

// Checks ratio of key to file input
// Exits program if ratio does not meet requirements
int checkKeyRatio(char k[100000], char p[100000], char kFile[256]) {
	// Error if key length shorter than plaintext to be encrypted
	if (strlen(k) < strlen(p)) {
		fprintf(stderr, "Error: key \'%s\' is too short\n", kFile);
		fflush(stdout);
		return 0;
	}

	return 1;
}

// Validates input file
// Exits program if bad character is contained in file
// Bad character includes any non-capital letter or space character
int validateChars(char p[100000]) {
	int i, c;

	// Error if non-valid character found
	for (i = 0; i < strlen(p); i++) {
		c = p[i];
		if (((c < 65) && (c != 32) && (c != 10)) || ( c > 90)) {
			fprintf(stderr, "otp_enc error: input contains bad characters\n");
			fflush(stdout);
			return 0;
		}
	}

	return 1;
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