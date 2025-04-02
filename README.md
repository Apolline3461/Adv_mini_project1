# Chat Application

This is a simple chat application using Qt for the client and server. It allows multiple users to connect to a server and exchange messages in real time.
This application is designed to run on Linux.

## Special tips
- **Basic server commands**:
    - `SVR:who` → List connected users.
    - `SVR:rename $new_pseudo` → Change your username.
    - `SVR:disconnect` → Disconnect from the server.
- **Keyboard shortcuts**:
    - Press **Enter** to send a message.
    - Press **Escape** to exit and disconnect from the server.

## Installation & Running the Application

### 1. Build the Project
From the root of the repository, create a build directory and compile the project:
```bash
mkdir build
cd build
cmake ..
make
```

### 2. Start the Server
In a terminal, navigate to the `build/` directory and start the server:
```bash
./server
```
The server runs on port 8080

### 3. Start the Client
In another terminal, launch the client:
```bash
./client
```

You can launch multiple clients in separate terminals to simulate multiple users.

## Usage
- When launching the client, you will be prompted to enter a username.
- Type messages in the input field and press **Enter** to send them.
- Use the available server commands to interact with the chat.
- Press **Escape** to disconnect and exit the application.



