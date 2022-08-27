/*
  WebSocket connection Script
  Uses standard W3C WebSocket API, not socket.io API
  Connects to a local websocket server

  created 7 Jan 2021
  modified 17 Jan 2021
  by Tom Igoe
*/
let server;
let serverURL;

let socket;
// variables for the DOM elements:
let serverText;
let endpointMenu;
let urlSpan;
let incomingSpan;
let outgoingText;
let connectionSpan;
let connectButton;

function setup() {
  // Get all the DOM elements that need listeners:
  serverText = document.getElementById('server-box');
  endpointMenu = document.getElementById('endpoint');
  urlSpan = document.getElementById('full-url');
  incomingSpan = document.getElementById('incoming');
  outgoingText = document.getElementById('outgoing');
  connectionSpan = document.getElementById('connection');
  connectButton = document.getElementById('connectButton');

  // set the listeners:
  serverText.addEventListener('change', setServer);
  outgoingText.addEventListener('change', sendMessage);
  connectButton.addEventListener('click', changeConnection);

  // Initial server and endpoint
  serverText.value = 'ws://localhost:18080/';
  endpointMenu.value = 'json';
  server = serverText.value;
  serverURL = server + endpointMenu.value;
  urlSpan.innerHTML = serverURL;
  openSocket(serverURL);
}

function setServer() {
  console.log("enter setve");
  server = serverText.value;
  serverURL = server + endpointMenu.value;
}

function openSocket(url) {
  // open the socket:
  // alert("Connecting to server now");
  socket = new WebSocket(url);
  socket.addEventListener('open', openConnection);
  socket.addEventListener('close', closeConnection);
  socket.addEventListener('message', readIncomingMessage);
}


function changeConnection(event) {
  console.log("clickety state=" + socket.readyState + " vs closed=" + WebSocket.CLOSED);
  // open the connection if it's closed, or close it if open:
  if (socket.readyState === WebSocket.CLOSED) {
    console.log("opening it");
    openSocket(serverURL);
  } else {
    console.log("closing it");
    socket.close();
  }
}

function openConnection() {
  // display the change of state:
  serverText.value = server;
  urlSpan.innerHTML = serverURL;
  connectionSpan.innerHTML = "true";
  connectButton.value = "Disconnect";
}

function closeConnection() {
  // display the change of state:
  urlSpan.innerHTML = "";
  connectionSpan.innerHTML = "false";
  connectButton.value = "Connect";
}

function readIncomingMessage(event) {
  console.log("got incoming=" + event.data + "<<");
  // display the incoming message:
  incomingSpan.innerHTML = event.data;
}

function sendMessage() {
  console.log("enter sendmsg state=" + socket.readyState + " vs open=" + WebSocket.OPEN);
  //if the socket's open, send a message:
  if (socket.readyState === WebSocket.OPEN) {
    // winl = outgoingText.value +"\n";
    winl = outgoingText.value;
    // winl = "; come\n(+ 4 4); moroeco";
    console.log("gonna send this=" + winl + "<<");
    socket.send(winl);
  }
}

// add a listener for the page to load:
window.addEventListener('load', setup);
