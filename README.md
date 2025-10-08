# TCP/UDP Publish-Subscribe System

A client-server application implementing a publish-subscribe messaging system that bridges UDP and TCP protocols. The server receives messages from UDP clients (provided by test scripts) and forwards them to TCP subscribers based on topic matching with wildcard support.

## Features

- **Protocol Bridge**: Receives UDP messages and forwards them to TCP subscribers
- **Topic-Based Routing**: Subscribers receive only messages matching their subscribed topics
- **Wildcard Support**: 
  - `*` - matches any number of words in a topic path
  - `+` - matches exactly one word in a topic path
- **Message Types**: Supports INT, SHORT_REAL, FLOAT, and STRING data types
- **Connection Management**: Handles client reconnections and maintains subscription state
- **Nagle's Algorithm Disabled**: Ensures low-latency message delivery for TCP connections

## Architecture

### Server (`server.cpp`)
- Listens on both UDP and TCP sockets
- Manages multiple TCP client connections using `poll()`
- Parses UDP messages and formats them for TCP clients
- Maintains client subscriptions and connection state
- Implements topic matching with wildcard patterns

### Subscriber (`subscriber.cpp`)
- Connects to the server via TCP
- Sends subscription/unsubscription requests
- Receives and displays formatted messages
- Handles graceful disconnection

## Message Format

### UDP Messages (from test.py to server)
```
[Topic: 50 bytes][Type: 1 byte][Payload: up to 1500 bytes]
```

**Supported Types:**
- `0` - INT: Signed 32-bit integer
- `1` - SHORT_REAL: Fixed-point number (precision: 0.01)
- `2` - FLOAT: Double with variable precision
- `3` - STRING: Null-terminated string

### TCP Messages (server to subscriber)
```
IP:PORT - TOPIC - TYPE - VALUE
```

Example: `192.168.1.100:5000 - sensors/temperature - FLOAT - 23.5600`

## Building

```bash
make
```

## Usage

### Starting the Server
```bash
./server <PORT>
```

Example:
```bash
./server 12345
```

### Running the UDP Test Client (provided by course) - having 12345 as port and 127.0.0.1 as IP_Server
```bash
python3 test.py
```

Example:
```bash
python3 test.py
```

### Starting a Subscriber
```bash
./subscriber <CLIENT_ID> <SERVER_IP> <SERVER_PORT>
```

Example:
```bash
./subscriber client1 127.0.0.1 12345
```

### Subscriber Commands

#### Subscribe to a topic
```
subscribe <TOPIC>
```

Examples:
```
subscribe sensors/temperature
subscribe sensors/*/humidity
subscribe +/livingroom/+
```

#### Unsubscribe from a topic
```
unsubscribe <TOPIC>
```

Example:
```
unsubscribe sensors/temperature
```

#### Disconnect
```
exit
```

### Server Commands

#### Shutdown server
```
exit
```

## Topic Matching Examples

Given UDP topic: `home/livingroom/temperature`

| Subscriber Topic | Matches? |
|-----------------|----------|
| `home/livingroom/temperature` | ✓ Yes |
| `home/*/temperature` | ✓ Yes |
| `home/+/temperature` | ✓ Yes |
| `*/temperature` | ✓ Yes |
| `home/*` | ✓ Yes |
| `home/+/+` | ✓ Yes |
| `home/bedroom/temperature` | ✗ No |
| `home/temperature` | ✗ No |

## Implementation Details

### Key Features

- **Non-blocking I/O**: Uses `poll()` for efficient multiplexing
- **Reliable Message Delivery**: Custom send/receive functions ensure complete message transmission
- **Topic Parsing**: Implements wildcard matching for flexible subscription patterns
- **Client State Management**: Tracks connected clients and their subscriptions
- **Reconnection Support**: Clients can reconnect and resume their subscriptions

### Data Structures

```cpp
struct udp_message {
    char topic[50];
    uint8_t type;
    char payload[1500];
};

struct tcp_client {
    char id[11];
    vector<string> topics;
    int sockfd;
};
```

### Network Considerations

- TCP connections disable Nagle's algorithm (`TCP_NODELAY`) for reduced latency
- All multi-byte integers use network byte order (big-endian)
- Message length prefixes ensure reliable framing

## Error Handling

- Server detects client disconnections and maintains subscription state
- Prevents duplicate client IDs from connecting simultaneously
- Handles partial sends/receives with retry logic
- Ignores `SIGPIPE` signal to prevent crashes on broken connections

## Limitations

- Maximum topic length: 50 characters
- Maximum client ID length: 10 characters
- Maximum payload size: 1500 bytes
- No message persistence or store-and-forward functionality

## Testing

The project was designed to work with a `test.py` script provided by the course that simulates UDP clients sending messages with various topics and data types. The server receives these UDP messages and distributes them to subscribed TCP clients based on topic matching rules.
