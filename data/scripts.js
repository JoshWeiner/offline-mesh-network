const title = "Josh's Chat App";
let displayTitle = new Array(title.length).fill("0");
let index = 0;
var socket = new WebSocket('ws://192.168.4.1:1337');

socket.onmessage = function(event) {
    console.log('Message from server:', event.data);
};

function sendMessage() {
    var message = document.getElementById('messageInput').value;
    socket.send(message);
}

function randomChar() {
    const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
    return chars.charAt(Math.floor(Math.random() * chars.length));
}

function updateTitle() {
    for (let i = index; i < displayTitle.length; i++) {
        displayTitle[i] = randomChar();
    }
    document.getElementById('chatTitle').innerText = displayTitle.join('');
}

function updateTitle() {
    for (let i = index; i < displayTitle.length; i++) {
        displayTitle[i] = randomChar();
    }
    document.getElementById('chatTitle').innerText = displayTitle.join('');
    if (index < title.length) {
        setTimeout(updateTitle, 20); // Reduced time for changing random characters for a smoother effect
    }
}

function decodeTitle() {
    if (index < title.length) {
        displayTitle[index] = title.charAt(index);
        index++;
        setTimeout(decodeTitle, 200); 
    }
}

function enterChatRoom() {
    const name = document.getElementById('nameField').value;
    if (name) {
        localStorage.setItem('username', name);
        window.location.href = 'chat.html';
    } else {
        console.log("Please enter your name.");
    }
}

enterButton.addEventListener("click", async function (e) {
    e.preventDefault();
    enterChatRoom();    
  });

updateTitle();
decodeTitle();
