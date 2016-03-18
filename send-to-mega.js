// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

'use strict';

var Client = require('azure-iothub').Client;
var Message = require('azure-iot-common').Message;

var connectionString = 'IOTHUB-CONN-STRING';
var targetDevice = 'Mega';

var client = Client.fromConnectionString(connectionString);

client.open(function (err) {
  if (err) {
    console.error('Could not connect: ' + err.message);
  } else {
    console.log('Client connected');

    // Create a message and send it to the IoT Hub every second
    var data = 'SURPRISE! NODE WORKS!';
    var message = new Message(data);
    console.log('Sent!');
    client.send(targetDevice, message, printResultFor('send'));
  }
});

// Helper function to print results in the console
function printResultFor(op) {
  return function printResult(err, res) {
    if (err) {
      console.log(op + ' error: ' + err.toString());
    } else {
      console.log(op + ' status: ' + res.constructor.name);
    }
    client.close();
  };
}
