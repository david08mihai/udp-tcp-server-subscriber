# UDP to TCP Messaging Server

This project implements a **publish-subscribe messaging system** using a server that receives messages via **UDP** and forwards them to connected **TCP subscribers**. The subscribers can dynamically subscribe/unsubscribe from topics (with wildcard support), and receive only the messages relevant to their interests.

---

## 📦 Components

- `server.cpp`: A TCP server that listens for UDP messages, parses and distributes them to the appropriate TCP clients.
- `subscriber.cpp`: A TCP client (subscriber) that connects to the server, subscribes/unsubscribes to topics, and receives messages.
- (A separate **UDP sender script** is used to simulate publishing messages — no dedicated UDP client exists in this repo.)

---

## 🚀 How It Works

### Server

- Listens for **UDP messages** from a publisher script.
- Parses UDP payloads and formats them based on type:
  - `INT`, `SHORT_REAL`, `FLOAT`, `STRING`.
- Matches incoming messages to client subscriptions using topic matching with wildcard support:
  - `+` matches one level (`news/+` matches `news/europe`).
  - `*` matches multiple levels (`alerts/*` matches `alerts/ro/fire`).
- Sends matched messages to all **connected TCP clients** subscribed to relevant topics.
- Maintains two mappings:
  - `socket → client ID`
  - `client ID → tcp_client { id, topics, socket }`
- Tracks disconnected clients and allows reconnections while preserving subscriptions.
- Uses `poll()` for multiplexed I/O handling over STDIN, UDP socket, and multiple TCP sockets.

### TCP Client (Subscriber)

- Connects to the server via TCP and sends its **unique ID** on startup.
- Supports the following commands via STDIN:
  - `subscribe <topic>`
  - `unsubscribe <topic>`
  - `exit`
- Commands are packed into a binary format:
  - 1 byte: command type (`0` = subscribe, `1` = unsubscribe, `2` = exit)
  - 1 byte: topic length
  - N bytes: topic string
- Sends a 2-byte prefix with total length before each command buffer.
- Listens for server messages:
  - Reads 4-byte length prefix.
  - Then receives the actual message and prints it to `stdout`.

---

## 🧪 Example Usage

### Start Server

```bash
./server <PORT>
```

### Start Subscriber
```bash
./subscriber <client_id> <server_ip> <server_port>
```
