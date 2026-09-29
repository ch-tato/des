#include <arpa/inet.h>
#include <cstdint>
#include <exception>
#include <netinet/in.h>
#include <unistd.h>

#include <cctype>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "des.hpp"
#include "message_protocol.hpp"
#include "network.hpp"

using namespace std;



int main()
{
    cout << "=====================================\n";
    cout << "             DES SERVER\n";
    cout << "=====================================\n\n";

    int port;
    cout << "Enter port to listen on: ";
    cin >> port;
    cin.ignore();

    string key;
    while (true)
    {
        cout << "Enter shared DES key: ";
        getline(cin, key);
        if (key.size() == 8)
            break;
        cout << "  -> Key must be exactly 8 characters (64 bits). Try again.\n";
    }

    vector<uint8_t> keyBytes(key.begin(), key.end());
    des::Bits keyBits = des::bytesToBits(keyBytes);

    cout << "\n[1] Deriving 16 round keys (K1..K16) from the shared key...\n";
    des::KeySchedule ks = des::generateRoundKeys(keyBits, /*verbose=*/true);

    int serverSock, sock;
    try
    {
        cout << "\n[Setup] Listening on port " << port << " ...\n";
        serverSock = net::createServerSocket(port);
        string clientIP;
        sock = net::acceptClient(serverSock, clientIP);
        cout << "[Setup] Connection established from " << clientIP
             << ". 'exit'/'quit' ends the session for both sides.\n";
    }
    catch (const exception &e)
    {
        cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }

    int turnNumber = 0;
    while (true)
    {
        cout << "\n---------------------------------------\n";
        cout << "[Session] Waiting for sender's message...\n";
        ++turnNumber;
        try
        {
            chat::ReceivedMessage incoming = chat::receiveEncryptedMessage(sock, ks, turnNumber);
            if (incoming.isQuit)
            {
                cout << "\n[Session] Sender ended the conversation.\n";
                break;
            }
            cout << "\n[Session] Sender says: \"" << incoming.plaintext << "\"\n";
        }
        catch (const exception &e)
        {
            cout << "\n[Session] Connection closed unexpectedly (" << e.what() << ").\n";
            break;
        }

        cout << "\nEnter reply (or 'exit'/'quit' to end): ";
        string message;
        getline(cin, message);

        if (chat::isQuitCommand(message))
        {
            chat::sendQuitSignal(sock);
            cout << "\n[Session] You ended the conversation.\n";
            break;
        }

        chat::sendEncryptedMessage(sock, ks, message, turnNumber);
    }

    close(sock);
    close(serverSock);
    cout << "\n=====================================\n";
    cout << "               DONE\n";
    cout << "=====================================\n";
    return 0;
}