let db;
const request = indexedDB.open("ChatAppDB", 1);
var socket = new WebSocket('ws://192.168.4.1:1337');

request.onerror = function(event) {
    console.error("An error occurred with IndexedDB:", event.target.errorCode);
};

request.onsuccess = function(event) {
    db = event.target.result;
};

request.onupgradeneeded = function(event) {
    let db = event.target.result;
    let userStore = db.createObjectStore("User", { keyPath: "ip" });
    userStore.createIndex("name", "name", { unique: false });
    let chatStore = db.createObjectStore("Chat", { autoIncrement: true });
    chatStore.createIndex("message", "message", { unique: false });
};

function updateScroll() {
    var chatWindow = document.getElementById("chatWindow");
    chatWindow.scrollTop = chatWindow.scrollHeight;
}


function encryptData(data) {
    let stringData = JSON.stringify(data);
    return btoa(stringData);
}

function decryptData(data) {
    return decodeURIComponent(atob(data));
}

function handleKeyPress(event) {
    if (event.key === 'Enter') {
        sendMessage();
    }
}

function sendMessage() {
    let message = document.getElementById('messageInput').value;
    let username = localStorage.getItem('username');
    let timestamp = new Date().toISOString();
    document.getElementById('messageInput').value = '';
    let messageData = {
        user: username,
        message: message,
        timestamp: timestamp
    };
    let encryptedData = encryptData(messageData);
    if (socket && socket.readyState === WebSocket.OPEN) {
        openDatabase().then(db => {
            const transaction = db.transaction(["Chat"], "readwrite");
            const store = transaction.objectStore("Chat");
            store.add(encryptedData);
        });
        socket.send(encryptedData);
    } else {
        // For now testing, delete later
        openDatabase().then(db => {
            const transaction = db.transaction(["Chat"], "readwrite");
            const store = transaction.objectStore("Chat");
            store.add(encryptedData);
        });
        console.log("WebSocket is not connected.");
    }
    updateScroll();
}

function openDatabase() {
    return new Promise((resolve, reject) => {
        const request = indexedDB.open("ChatAppDB", 1);

        request.onerror = (event) => {
            reject('Database error: ' + event.target.errorCode);
        };

        request.onsuccess = (event) => {
            resolve(event.target.result);
        };
    });
}

function fetchAndRenderMessages() {
    openDatabase().then(db => {
        const transaction = db.transaction(["Chat"], "readonly");
        const store = transaction.objectStore("Chat");
        const request = store.getAll();

        request.onerror = (event) => {
            console.error("Error fetching messages:", event.target.errorCode);
        };

        request.onsuccess = (event) => {
            let messages = event.target.result;

            //console.log("messages: ", messages);
            let decryptedMessages = messages.map(message => {
                const decryptedMessage = JSON.parse(decryptData(message));
                return {
                    ...decryptedMessage,
                    message: decryptedMessage,
                    timestamp: new Date(decryptedMessage.timestamp)
                };
            });

            console.log("decrypted: ", decryptedMessages);

            decryptedMessages.sort((a, b) => a.timestamp - b.timestamp);

            decryptedMessages.forEach(msg => {
                renderMessage(msg);
            });

            updateScroll();
        };
    });
}

function formatTimestamp(timestamp) {
    const messageDate = new Date(timestamp);
    const now = new Date();
    let formattedTimestamp;

    if (messageDate.toDateString() === now.toDateString()) {
        formattedTimestamp = messageDate.toLocaleTimeString('en-US', {
            hour: '2-digit',
            minute: '2-digit',
            hour12: true
        });
    } else {
        formattedTimestamp = messageDate.toLocaleDateString('en-US', {
            month: 'short',
            day: 'numeric'
        });
    }

    return formattedTimestamp;
}

function renderMessage(messageData) {
    const username = messageData.message.user;
    const local_user = localStorage.getItem('username');
    const msg = messageData.message.message;
    const timestamp = messageData.timestamp;
    const messageElement = document.createElement("div");
    const is_local = username == local_user;
    messageElement.className = is_local ? 'message my-user' : 'message other-user';
    messageElement.innerHTML = `
        <div class="message-info">${is_local ? "You" : username} ${formatTimestamp(timestamp)}</div>
        <div class="message-body">${msg}</div>
    `;
    // console.log(messageElement);
    document.getElementById("chatWindow").appendChild(messageElement);
}

socket.onmessage = function(event) {
    console.log("Message!");
    let decryptedData = JSON.parse(decryptData(event.data));
    renderMessage({
        message: decryptedData,
        timestamp: new Date(decryptedData.timestamp)
    });
    openDatabase().then(db => {
        const transaction = db.transaction(["Chat"], "readwrite");
        const store = transaction.objectStore("Chat");
        store.add(event.data);
    });
    updateScroll();
}

window.onload = () => {
    fetchAndRenderMessages();
    updateScroll();
};