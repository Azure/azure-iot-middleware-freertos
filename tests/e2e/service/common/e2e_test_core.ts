// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

//
//  iothub_test_core.ts implements fundamental logic for the C SDK E2E test framework.
//
'use strict'

import { CommandTestData, CommandTestData_ExitCommands } from "./e2e_test_commands";
import { DeviceMethodParams } from "azure-iothub";

const { exec } = require('child_process');
const serviceSDK = require('azure-iothub');
const iothubRegistry  = require('azure-iothub').Registry;
const uuid = require('uuid');
const terminate = require('terminate');
const anHourFromNow = require('azure-iot-common').anHourFromNow;
const serviceSas = require('azure-iothub').SharedAccessSignature;
const Message =  require('azure-iot-common').Message;
const EventHubClient = require('@azure/event-hubs').EventHubClient;
const EventPosition = require('@azure/event-hubs').EventPosition;
const ProvisioningServiceClient = require('azure-iot-provisioning-service').ProvisioningServiceClient;

// Time, in ms, for test framework to sleep after the process is created and before continuing next tests.
const sleepAfterProcessCreation:number = 5000;
// Time, 5s, for test framework to sleep before terminating test process.
const sleepBeforeTermination:number = 5000;
const deviceConnectedMessage = "\"CONNECTED\"";
// Time, in 40s, for event hub partition polling.
const eventHubPartitionTimeout:number = 40000;

// "Handle" to test executable.  Because this is global, means that there
// can only be one sub-process test executable per suite.
let test_exe = null
let service_client = null
let eventhub_client = null
let use_direct_method_to_execute = false
// device state
let device_exit_status = 0;

//
// executeTestProcess creates a worker process that hosts the C SDK test framework
// This also redirects stdout/stderr from the child process to the console
//
function executeTestProcess(test_process:string, cmd_args:string, resultCallback) {
    // Start the process
    let command_line:string = test_process + ` ` + cmd_args + ``
    //console.log(`Invoking exec(${command_line})`)
    test_exe = exec(command_line);

    test_exe.stdout.on('data', (data:string) => {
        console.log("APP STDOUT: " + data)
    })

    test_exe.stderr.on('data', (data:string) => {
        console.log("APP STDERR:" + data)
    })

    test_exe.on('exit', (code:number, signal:string) => {
        let result:number

        if (code) {
            console.warn(`process return non zero exit code error: ${code}`);
            result = 1;
        }
        else {
            console.log(`test process has cleanly exited`)
            result = 0;
        }

        return resultCallback(result);
    });
}

//
// terminateTestProcess will terminate (killproc) the test child process, if it is active.
//
function terminateTestProcessIfNecessary(done) {
    if (test_exe != null) {
        terminate(test_exe.pid, function(err, result) {
            if (err) {
                // Terminating the test process is a best effort action.  Its failure will not
                // cause an otherwise successful test run to be marked as failing.
                console.warn(`Unable to terminate test process ID=<${test_exe.pid}>`)
                done()
            }
            else {
                console.log(`Successfully terminated process`)
                done()
            }
        })
    }
}

//
// createTestDevice will create a new device on IoThub for these tests.  The name of the device
// is randomly determined, but will be prefixed with the specified testDeviceNamePrefix to make
// it easier for cleaning up test suites under development.  On device creation, the passed
// resultCallback function will be invoked.
//
function createTestDevice(hubConnectionString:string, testDeviceNamePrefix:string, resultCallback:any) {
    let registry = iothubRegistry.fromConnectionString(hubConnectionString)

    // device side limitation
    let testDeviceName:string = (testDeviceNamePrefix + uuid.v4()).substring(0, 32)

    let new_device =
    {
        deviceId: testDeviceName,
        status: 'enabled',
        authentication: {
            symmetricKey: {
                primaryKey: new Buffer(uuid.v4()).toString('base64'),
                secondaryKey: new Buffer(uuid.v4()).toString('base64')
            }
        }
    }

    registry.create(new_device, resultCallback)
}

//
// Removes device of given name from IoTHub
//
function deleteDevice(hubConnectionString:string, testDeviceName:string, resultCallback:any) {
    console.log(`About to delete device <${testDeviceName}>`)
    let registry = iothubRegistry.fromConnectionString(hubConnectionString)
    registry.delete(testDeviceName, resultCallback)
}

//
// getHubnameFromConnectionString retrieves the hostname from a given hub connection string
//
function getHubnameFromConnectionString(hubConnectionString:string) {
    // For given hub connection string, remove the HostName=<USER_SPECIFIED_VALUE>;Etc. so it's just the <sUSER_SPECIFIED_VALUE>
    return (hubConnectionString.replace("HostName=","")).replace(/;.*/,"")
}

//
// getConnectionStringFromDeviceInfo returns a connection string for a given device
// based on the hub's connection string and the device's info structure.
//
function getConnectionStringFromDeviceInfo(hubConnectionString:string, deviceInfo) {
    // The deviceInfo doesn't contain the hub hostname, so need to parse it from initial connection string
    let hubHostName = getHubnameFromConnectionString(hubConnectionString)
    return ("HostName=" + hubHostName + ";DeviceId=" + deviceInfo.deviceId + ";SharedAccessKey=" + deviceInfo.authentication.symmetricKey.primaryKey)
}

//
// getTestProcessArgs returns arguments required for Test process.
//
function getTestProcessArgs(hubConnectionString:string, deviceInfo) {
    // Start the process
    let hubHostName = getHubnameFromConnectionString(hubConnectionString)
    let test_process_args = "'" + hubHostName + "' '" + deviceInfo.deviceId + "' '' '" + deviceInfo.authentication.symmetricKey.primaryKey + "'";
    return test_process_args;
}

//
// getIoTHubServiceClient returns global IoTHub Service client.
//
function getIoTHubServiceClient(testHubConnectionString:string) {
    if (service_client == null) {
        service_client = serviceSDK.Client.fromConnectionString(testHubConnectionString);
    }

    return service_client;
}

//
// getEventHubClient returns global Event Hub client.
//
function getEventHubClient(testHubConnectionString:string) {
    if (eventhub_client == null) {
        eventhub_client = EventHubClient.createFromIotHubConnectionString(testHubConnectionString);
    }

    return eventhub_client;
}

//
// invokeTestCommand invokes command by sending C2D message.
//
function invokeTestCommand(client, deviceId:string, properties:Object, payload:string, done) {
    let message = new Message(payload);
    
    client.send(deviceId, message, (err) => {
        console.log('Message sent ' + payload);
        if (!isEmpty(properties)) {
            console.log('Properties sent' + JSON.stringify(properties));
        }
        done(err)
    });
}

//
// invokeTestCommandUsingDM invokes command by sending Direct method message.
//
function invokeTestCommandUsingDM(client, deviceId:string, methodName:string, properties:Object, payload:string, done) {
    const methodParams: DeviceMethodParams = {
        methodName: methodName,
        payload: JSON.parse(payload),
        connectTimeoutInSeconds: 30,
        responseTimeoutInSeconds: 60
      };
    
    client.invokeDeviceMethod(deviceId, methodParams, (err, result, response) => {
        if (err) {
            done(err);
        }
        else if (result.status != 0) {
            throw new Error("failed to execute : " + JSON.stringify(result.payload)) 
        }
        else { 
            done();
        }
      });
}

//
// closeServiceEventHubClients closes global EventHub client and IOTHub Service client.
//
function closeServiceEventHubClients(done) {
    let eventHubErr = null;
    let serviceErr = null;

    if (!service_client && !eventhub_client) {
      done();
    }

    if (eventhub_client) {
        eventhub_client.then( function(client) {
            client.close().then(function () {
                eventHubErr = serviceErr;
                eventhub_client = null;
                if (!service_client) {
                  done(eventHubErr);
                }
              }).catch(function (err) {
                eventHubErr = err;
                eventhub_client = null;
                if (!service_client) {
                  done(eventHubErr);
                }
              });
        });
    }

    if (service_client) {
        service_client.close( function (err) {
        serviceErr = err || eventHubErr;
        service_client = null;
        if (!eventhub_client) {
            done(serviceErr);
        }});
      }
}

//
// isEmpty check if object is empty.
//
function isEmpty(obj) {
    for(let key in obj) {
        if(obj.hasOwnProperty(key)) {
            return false;
        }
    }

    return true;
}

//
// runTestCommand invokes the test command on the test exe using Direct Method or C2D message
//
function runTestCommand(testHubConnectionString:string, deviceId:string, testData:CommandTestData): Promise<string> {
    const client = getIoTHubServiceClient(testHubConnectionString);

    let result = new Promise<string>(function(resolve, reject) {
        if (use_direct_method_to_execute) {
            invokeTestCommandUsingDM(client, deviceId, testData.GetMethod(), testData.GetProperties(), testData.Serialize(), function(err) {
                if (err) {
                    console.log(('Message sent failed rejecting promise'))
                    reject(err)
                }
                else {
                    console.log(('Message sent completing promise'))
                    resolve(testData.GetDataExpected())
                }
            });
        }
        else {
            invokeTestCommand(client, deviceId, testData.GetProperties(), testData.Serialize(), function(err) {
                if (err) {
                    console.log(('Message sent failed rejecting promise'))
                    reject(err)
                }
                else {
                    console.log(('Message sent completing promise'))
                    resolve(testData.GetDataExpected())
                }
            });
        }
    });

    return result;
}

//
// runTestCommand invokes the test command on the test exe using Direct method message
//
function runTestCommandUsingDM(testHubConnectionString:string, deviceId:string, testData:CommandTestData): Promise<string> {
    const client = getIoTHubServiceClient(testHubConnectionString);

    let result = new Promise<string>(function(resolve, reject) {
        invokeTestCommandUsingDM(client, deviceId, testData.GetMethod(), testData.GetProperties(), testData.Serialize(), function(err) {
            if (err) {
                console.log(('Message sent failed rejecting promise'))
                reject(err)
            }
            else {
                console.log(('Message sent completing promise'))
                resolve(testData.GetDataExpected())
            }
        });
    });

    return result;
}

//
// getEventHubDecodedMessage routine decodes the eventHub AMQ message to string
//
function getEventHubDecodedMessage(eventData): string {
    let message = ""

    if (!isEmpty(eventData.body)) {
        message += JSON.stringify(eventData.body)
    }

    if (!isEmpty(eventData.applicationProperties)) {
        message += JSON.stringify(eventData.applicationProperties)
    }

    return message
}

//
// verifyTelemetryMessage verify telemetry message in event hub messages
//
function verifyTelemetryMessage(testHubConnectionString:string, deviceId:string, message:string): Promise<number> {
    return verifyTelemetryMessageWithTimeout(testHubConnectionString, deviceId, message, eventHubPartitionTimeout);
}

//
// verifyTelemetryMessageWithTimeout verify telemetry message in event hub messages with user specified timeout
//
function verifyTelemetryMessageWithTimeout(testHubConnectionString:string, deviceId:string, message:string, timeout:number): Promise<number> {
    let result = new Promise<number> (function(resolve, reject) {
        const startAfterTime = Date.now() - 5000

        getEventHubClient(testHubConnectionString).then(function(client) {
            let found = 0;
            let timmer = null;

            const onEventHubMessage = function (eventData) {
                if (eventData.annotations['iothub-connection-device-id'] === deviceId) {
                    let eventMsg = getEventHubDecodedMessage(eventData);
                    console.log('eventMsg: ' + eventMsg);
                    if (found === 0 && message === eventMsg) {
                        console.log(('found message resolving the promise'));
                        found = 1;
                        client.close().then(function() {
                            console.log(('Resolving message verification promise'))
                            clearTimeout(timmer)
                            resolve(0)
                        });
                    }
                }
                else {
                    console.log('Incoming device id is: ' + eventData.annotations['iothub-connection-device-id'])
                }
            };

            const onEventHubError = function (err) {
                console.log(('Error from Event Hub Client Receiver: ' + err.toString()));
            };

            // Listing for message in all paritions
            client.getPartitionIds().then(function(partitionIds) {
                partitionIds.forEach(function (partitionId) {
                    client.receive(partitionId, onEventHubMessage, onEventHubError, { eventPosition: EventPosition.fromEnqueuedTime(startAfterTime) })
                });
            });

            // timeout parition after eventHubPartitionTimeout
            // and close connection if result not found
            timmer = setTimeout(function() {
                if (!found) {
                    console.log(('Event hub timeout triggered'))
                    client.close().then(function() {
                        if (!found) {
                            console.log(('rejecting the verification promise'))
                            reject(Error ('timeout'))
                        }
                    }).catch (function (error) {
                        if (!found) {
                            console.log(('rejecting the verification promise'))
                            reject(error)
                        }
                    });
                }
            }, timeout)

        }).catch(function(error) {
            console.log(('rejecting the verification promise as eventhub promise failed'))
            reject(error)
        });
    });

    return result;
}

//
// createTestDeviceAndTestProcess creates a device for test scenario and creates the test process that hosts the C SDK
//
function createTestDeviceAndTestProcess(testHubConnectionString:string, testexe:string, done) {
    // First, we create a new test device on IoTHub.  This will be destroyed on test tear down.
    createTestDevice(testHubConnectionString, "azure_services_port_e2e_", function(err, newDeviceInfo) {
        if (err) {
            console.log(`createTestDevice fails, error=<${err}>`)
            done(err, null)
        }
        else {
            const testDeviceConnectionString:string = getConnectionStringFromDeviceInfo(testHubConnectionString, newDeviceInfo)
            //console.log(`Successfully new device with connectionString=<${testDeviceConnectionString}>`)

            // Creates the test process, providing connection string of the test created device it should associate with
            //console.log(`Invoking test executable ${testDeviceConnectionString}`)
            let test_args:string = getTestProcessArgs(testHubConnectionString, newDeviceInfo);
            executeTestProcess(testexe, test_args, function processCompleteCallback(result:number) {
                console.log(`Test process has completed execution with exit status ${result}`)
                device_exit_status = result
            })

            // Now we need to wait until the test EXE is connected.
            verifyTelemetryMessage(testHubConnectionString, newDeviceInfo.deviceId, deviceConnectedMessage).then(function(result) {
                done(result, newDeviceInfo)
            }).catch(function(error) {
                done(error, null)
            });
        }
    })
}

//
// deleteDeviceAndEndProcess deletes the device and forces the child test process, if still running, to terminate.
//
function deleteDeviceAndEndProcess(testHubConnectionString:string, testDeviceInfo:any, done) {
    if (testDeviceInfo != null) {
        console.log(`Sending command to test EXE to gracefully shutdown`)

        // Send exit command
        const exitCmd = new CommandTestData_ExitCommands()
        runTestCommand(testHubConnectionString, testDeviceInfo.deviceId, exitCmd).then(() => {});

        deleteDevice(testHubConnectionString, testDeviceInfo.deviceId, () => {
            // Always attempt to terminate the test process, regardless of whether device deletion succeeded or failed
            closeServiceEventHubClients(function(err) {
                setTimeout(function() {
                    terminateTestProcessIfNecessary(done);
                }, sleepBeforeTermination);
            });
        });
    }
    else {
        done()
    }
}

function getDeviceExitStatus() {
    return device_exit_status;
}


//
// createProvisioningEnrollment will create a new enrollment on DPS service for these tests.  The name of the enrollment
// is randomly determined, but will be prefixed with the specified testEnrollmentNamePrefix to make
// it easier for cleaning up test suites under development.  On enrollment creation, the passed
// resultCallback function will be invoked.
//
function createProvisioningEnrollment(provisioningConnectionString:string, testEnrollmentNamePrefix:string, resultCallback:any) {
    let provisioningServiceClient = ProvisioningServiceClient.fromConnectionString(provisioningConnectionString);

    // device side limitation
    let reg_id:string = (testEnrollmentNamePrefix + uuid.v4()).substring(0, 32)

    let enrollment =
    {
        registrationId: reg_id,
        deviceId: reg_id,
        attestation: {
            type: 'symmetricKey',
            symmetricKey: {
              primaryKey: new Buffer(uuid.v4()).toString('base64'),
              secondaryKey: new Buffer(uuid.v4()).toString('base64'),
            }
          },
          provisioningStatus: "enabled",
    }

    provisioningServiceClient.createOrUpdateIndividualEnrollment(enrollment, (err) => {
        if (err) {
            resultCallback(err, null);
        }
        else {
            resultCallback(err, enrollment)
        }
    });
}

//
// Removes enrollment of given name from Enrollment
//
function deleteProvisioningEnrollment(provisioningConnectionString:string, reg_id:string, resultCallback:any) {
    console.log(`About to delete enrollment <${reg_id}>`)
    let provisioningServiceClient = ProvisioningServiceClient.fromConnectionString(provisioningConnectionString);
    provisioningServiceClient.deleteIndividualEnrollment(reg_id, resultCallback)
}

function setDirectMethodToExecuteCommand() {
    use_direct_method_to_execute = true;
}

function verifyReportedProperties(connectionString:string, deviceId:string, value:string) : Promise<string> {
    var registry = iothubRegistry.fromConnectionString(connectionString);
    var rv = registry.getTwin(deviceId);

    let result = new Promise<string> ((resolve, reject) => {
        rv.then((httResponse:any) => {
            let twin = httResponse.responseBody;
            console.log(JSON.stringify(twin));
            var reported_property_str = JSON.stringify(twin.properties.reported);

            if (reported_property_str.includes(value)) {
                resolve(JSON.stringify(twin));
            }
            else {
                reject(new Error("failed to find reported value :" + value))
            }
        }).catch((err:any) => {
            reject(err)
        });
    });

    return result;
}

function postDesiredProperty(connectionString:string, deviceId:string, key:string, value:string) : Promise<string> {
    var registry = iothubRegistry.fromConnectionString(connectionString);
    var rv = registry.getTwin(deviceId);
    let result = new Promise<string> ((resolve, reject) => {
        rv.then((httResponse:any) => {
            let twin = httResponse.responseBody;
            let properties = {};
            properties[key] = value;

            var twinPatch = {
                properties: {
                  desired : properties
                }
            };
            
            twin.update(twinPatch, (err:any, newTwin:any) => {
                JSON.stringify(newTwin)
                if (err) {
                    reject(err)
                }
                else {
                    resolve(JSON.stringify(newTwin));
                }
            });
        }).catch((err:any) => {
            reject(err)
        });
    });

    return result;
}

function getTwinProperties(connectionString:string, deviceId:string) : Promise<string> {
    var registry = iothubRegistry.fromConnectionString(connectionString);
    var rv = registry.getTwin(deviceId);
    let result = new Promise<string> ((resolve, reject) => {
        rv.then((httResponse:any) => {
            let twin = httResponse.responseBody.properties
            delete twin.desired["$metadata"]
            delete twin.reported["$metadata"]
            resolve(JSON.stringify(twin))
        }).catch((err:any) => {
            reject(err)
        });
    });

    return result;
}

module.exports = {
    getConnectionStringFromDeviceInfo : getConnectionStringFromDeviceInfo,
    getHubnameFromConnectionString : getHubnameFromConnectionString,
    createTestDevice : createTestDevice,
    executeTestProcess : executeTestProcess,
    deleteDevice : deleteDevice,
    terminateTestProcessIfNecessary : terminateTestProcessIfNecessary,
    getTestProcessArgs : getTestProcessArgs,
    getIoTHubServiceClient : getIoTHubServiceClient,
    getEventHubClient : getEventHubClient,
    closeServiceEventHubClients : closeServiceEventHubClients,
    runTestCommand : runTestCommand,
    setDirectMethodToExecuteCommand : setDirectMethodToExecuteCommand,
    verifyTelemetryMessage : verifyTelemetryMessage,
    createTestDeviceAndTestProcess : createTestDeviceAndTestProcess,
    deleteDeviceAndEndProcess : deleteDeviceAndEndProcess,
    getDeviceExitStatus : getDeviceExitStatus,
    createProvisioningEnrollment : createProvisioningEnrollment,
    deleteProvisioningEnrollment : deleteProvisioningEnrollment,
    verifyTelemetryMessageWithTimeout : verifyTelemetryMessageWithTimeout,
    verifyReportedProperties : verifyReportedProperties,
    postDesiredProperty : postDesiredProperty,
    getTwinProperties : getTwinProperties
}