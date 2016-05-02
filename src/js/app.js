var keys="notset";

// Function to send a message to the Pebble using AppMessage API
// We are currently only sending a message using the "status" appKey defined in appinfo.json/Settings
function sendStatus(status) {
	Pebble.sendAppMessage({"status": status}, messageSuccessHandler, messageFailureHandler);
}

// Called when the message send attempt succeeds
function messageSuccessHandler() {
  console.log("Message send succeeded.");  
}

// Called when the message send attempt fails
function messageFailureHandler() {
  console.log("Message send failed.");
  sendStatus();
}

// Called when JS is ready
Pebble.addEventListener("ready", function(e) {
  console.log("JS is ready!");
  
  keys=localStorage.getItem('keys');
  console.log("Got key:"+keys);
  if (!keys)
    keys="notset";
});

Pebble.addEventListener('showConfiguration', function() {
  var url = 'http://charcole.github.io/PebbleIFTTT/index.html';
  console.log('Showing configuration page: ' + url);

  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
  var configData = JSON.parse(decodeURIComponent(e.response));
  console.log('Configuration page returned: ' + JSON.stringify(configData));

  keys=configData.keybox;
  localStorage.setItem('keys', keys);
  
  var dict = {};
  dict.menu=configData.menu;
  
  // Send to watchapp
  Pebble.sendAppMessage(dict, function() {
    console.log('Send successful: ' + JSON.stringify(dict));
  }, function() {
    console.log('Send failed!');
  });
});

// Called when incoming message from the Pebble is received
// We are currently only checking the "message" appKey defined in appinfo.json/Settings
Pebble.addEventListener("appmessage", function(e) {
  if ('cmd' in e.payload)
  {
    console.log("Received Message: " + e.payload.cmd);
    var oReq = new XMLHttpRequest();
    oReq.onreadystatechange = function() {
      if (oReq.readyState == 4) {
        if (oReq.status == 200)
        {
          sendStatus(0);
        }
        else
        {
          if (oReq.status==401)
            sendStatus(2);
          else
            sendStatus(1);  
        }
      }
    };
    oReq.open("post", "https://maker.ifttt.com/trigger/"+e.payload.cmd+"/with/key/"+keys);
    oReq.send();
  }
});