/*
  WebSocket connection Script
  Uses standard W3C WebSocket API, not socket.io API
  Connects to a local websocket server

  created 7 Jan 2021
  modified 17 Jan 2021
  by Tom Igoe

  Revised and adapted for the CogServer, Linas Vepstas August 2022
*/
let server;
let endpoint;
let serverURL;
let errorState = "";

let socket;
// variables for the DOM elements:
let serverText;
let endpointMenu;
let endpointType;
let urlSpan;
let connectionSpan;
let connectButton;
let replySpan;
let outgoingText;

function setup()
{
  // Get all the DOM elements that need listeners.
  serverText = document.getElementById('server-box');
  endpointMenu = document.getElementById('endpoint-menu');
  endpointType = document.getElementById('endpoint');
  urlSpan = document.getElementById('full-url');
  replySpan = document.getElementById('reply');
  outgoingText = document.getElementById('outgoing');
  connectionSpan = document.getElementById('connection');
  connectButton = document.getElementById('connectButton');

  // Set the listeners.
  serverText.addEventListener('change', setServer);
  outgoingText.addEventListener('change', sendMessage);
  connectButton.addEventListener('click', changeConnection);

  // Initial server and endpoint.
  server = 'ws://localhost:18080/';
  endpoint = 'json';
  serverURL = server + endpoint;

  // DOM elements
  serverText.value = server;
  endpointMenu.value = endpoint;
  urlSpan.innerHTML = serverURL;
  openSocket(serverURL);
}

// Called when user enters a new CogServer base URL
function setServer() {
  server = serverText.value;
  endpoint = endpointMenu.value;
  serverURL = server + endpoint;
  console.log("Enter setServer, new url=" + serverURL);
}

function openSocket(url)
{
  // Open the socket.
  socket = new WebSocket(url);
  socket.addEventListener('open', openConnection);
  socket.addEventListener('close', closeConnection);
  socket.addEventListener('message', readReplyMessage);
  socket.addEventListener('error', reportError);
}


function changeConnection(event)
{
  console.log("button click; socket state=" + socket.readyState + " vs closed=" + WebSocket.CLOSED);
  // Open the connection if it's closed, or close it if open.
  if (socket.readyState === WebSocket.CLOSED) {
    server = serverText.value;
    endpoint = endpointMenu.value;
    serverURL = server + endpoint;
    console.log("Opening socket connection to " + serverURL);
    openSocket(serverURL);
  } else {
    console.log("close socket");
    socket.close();
  }
}

function openConnection()
{
  errorState = "";
  // Display the change of state:
  serverText.value = server;
  urlSpan.innerHTML = serverURL;
  connectionSpan.innerHTML = "true";
  connectButton.value = "Disconnect";

  if (endpoint == 'json')
    endpointType.innerHTML = "JSON";
  else if (endpoint == 'scm')
    endpointType.innerHTML = "Guile Scheme";
  else if (endpoint == 'python')
    endpointType.innerHTML = "Python";
  else if (endpoint == 'sexpr')
    endpointType.innerHTML = "S-Expressions";
}

function closeConnection()
{
  // Display the change of state:
  urlSpan.innerHTML = "none";
  connectionSpan.innerHTML = "false" + errorState;
  connectButton.value = "Connect";
  endpointType.innerHTML = "none";
  replySpan.innerHTML = "";
}

function reportError(event)
{
  console.log("oh nooo=" + event.data + "<<");
  errorState = "; Unable to connect to \'" + serverURL + "\'";
}

// Display the reply message
function readReplyMessage(event)
{
  console.log("got reply=" + event.data + "<<");
  replySpan.innerHTML = event.data;
}

function sendMessage()
{
  console.log("enter sendmsg; socket state=" + socket.readyState + " vs open=" + WebSocket.OPEN);
  // If the socket's open, send a message:
  if (socket.readyState === WebSocket.OPEN) {
    winl = outgoingText.value;
    console.log("going to send this" + winl + "<<");
    socket.send(winl);
  }
}

// add a listener for the page to load:
window.addEventListener('load', setup);
