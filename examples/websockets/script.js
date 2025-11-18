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
  // For textarea, listen to keydown to catch Enter key (but not Shift+Enter)
  outgoingText.addEventListener('keydown', function(event) {
    if (event.key === 'Enter' && !event.shiftKey) {
      event.preventDefault(); // Prevent default newline
      sendMessage();
    }
  });
  connectButton.addEventListener('click', changeConnection);

  // Initial server and endpoint - use current hostname.
  server = `ws://${window.location.hostname}:18080/`;
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
  connectionSpan.className = "connected";
  connectButton.value = "Disconnect";

  if (endpoint == 'json')
    endpointType.innerHTML = "JSON";
  else if (endpoint == 'scm')
    endpointType.innerHTML = "Guile Scheme";
  else if (endpoint == 'py')
    endpointType.innerHTML = "Python";
  else if (endpoint == 'sexpr')
    endpointType.innerHTML = "S-Expressions";
}

function closeConnection()
{
  // Display the change of state:
  urlSpan.innerHTML = "none";
  connectionSpan.innerHTML = "false" + errorState;
  connectionSpan.className = "";
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
  // Append each message chunk (don't replace) so errors aren't lost
  replySpan.innerHTML += event.data;
}

function sendMessage(keepCommand)
{
  console.log("enter sendmsg; socket state=" + socket.readyState + " vs open=" + WebSocket.OPEN);
  // If the socket's open, send a message:
  if (socket.readyState === WebSocket.OPEN) {
    // Clear previous results before sending new command
    replySpan.innerHTML = "";
    winl = outgoingText.value;
    console.log("going to send this" + winl + "<<");
    socket.send(winl);
    // Clear the textarea after sending (unless keepCommand is true)
    if (!keepCommand) {
      outgoingText.value = '';
    }
  }
}

// Function to run a specific command with the correct endpoint
function runCommand(command, requiredEndpoint)
{
  console.log("Running command: " + command + " with endpoint: " + requiredEndpoint);

  // Always set the command in the textarea first so user can see it
  outgoingText.value = command;

  // Check if we need to switch endpoints
  if (endpoint !== requiredEndpoint) {
    // Update the endpoint
    endpoint = requiredEndpoint;
    endpointMenu.value = requiredEndpoint;

    // If connected, disconnect first to reconnect with new endpoint
    if (socket && socket.readyState === WebSocket.OPEN) {
      console.log("Switching endpoint, disconnecting first");
      socket.close();
      // Wait a bit for close to complete, then reconnect
      setTimeout(function() {
        // Re-set the command since connectAndSend might clear it
        outgoingText.value = command;
        connectAndSend();
      }, 500);
    } else {
      // Not connected, just connect and send
      connectAndSend();
    }
  } else if (socket && socket.readyState === WebSocket.OPEN) {
    // Already connected to the right endpoint, just send
    sendMessage(true); // Keep command in textarea
  } else {
    // Right endpoint but not connected
    connectAndSend();
  }
}

// Helper function to connect and send after connection
function connectAndSend()
{
  server = serverText.value;
  endpoint = endpointMenu.value;
  serverURL = server + endpoint;
  console.log("Connecting to " + serverURL + " to send command");

  // Save the command before connecting since sendMessage will clear it
  var commandToSend = outgoingText.value;

  // Create a one-time listener for the open event
  var tempSocket = new WebSocket(serverURL);
  tempSocket.addEventListener('open', function() {
    socket = tempSocket;
    openConnection();
    // Restore the command and send it after a short delay to ensure connection is ready
    setTimeout(function() {
      outgoingText.value = commandToSend;
      sendMessage(true); // Keep command in textarea
    }, 100);
  });
  tempSocket.addEventListener('close', closeConnection);
  tempSocket.addEventListener('message', readReplyMessage);
  tempSocket.addEventListener('error', reportError);
}

// add a listener for the page to load:
window.addEventListener('load', setup);
