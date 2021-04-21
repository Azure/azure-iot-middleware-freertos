// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// These TypeScript tests are the test driver for E2E tests, acting as service application
// calling into an EXE hosting the C Azure IoT device SDK.
// See ../../readme.md for a full architectural review of how it works.
'use strict'

const iothubTestCore = require ('../common/e2e_test_core');
const e2eTestCommands = require ('../common/e2e_test_commands');

// testDeviceInfo contains the settings for our test device on the IoTHub.
let testDeviceInfo:any = null;


//
// testSetup creates a device for test scenario and creates the test process that hosts the C SDK
//
function testSetup(done) {
    if (argv.useDirectMethods) {
        iothubTestCore.setDirectMethodToExecuteCommand();
    }
    
    iothubTestCore.createTestDeviceAndTestProcess(testHubConnectionString, argv.testexe, (err, newDeviceInfo) => {
        if (err) {
            done(err)
        }
        else {
            // Store information about this device for later
            testDeviceInfo = newDeviceInfo
            done(err)
        }
    })
}

//
// testCleanup deletes the device and forces the child test process, if still running, to terminate.
//
function testCleanup(done) {
    iothubTestCore.deleteDeviceAndEndProcess(testHubConnectionString, testDeviceInfo, done);
}

//
// getTestHubConnectionString returns hub's connection string, from either command line or environment
//
function getTestHubConnectionString(): string {
    if (argv.connectionString != null) {
        return argv.connectionString
    }
    else if (process.env.IOTHUB_CONNECTION_STRING != null) {
        return process.env.IOTHUB_CONNECTION_STRING
    }
    else {
        throw "Hub connection string must be configured in the command line or environment variable IOTHUB_CONNECTION_STRING"
    }
}

let argv = require('yargs')
    .usage('Usage: $0 --connectionString <HUB CONNECTION STRING> --testexe <PATH OF EXECUTABLE TO HOST C SDK TESTS>')
    .option('connectionString', {
        alias: 'c',
        describe: 'The connection string for the *IoTHub* instance to run tests against',
        type: 'string',
        demandOption: false
    })
    .option('testexe', {
        alias: 'e',
        describe: 'The full path of the application to execute',
        type: 'string',
        demandOption: true
    })
    .option('deviceId', {
        alias: 'd',
        describe: 'The device id already created for debug',
        type: 'string',
        demandOption: false
    })
    .option('useDirectMethods', {
        alias: 'dm',
        describe: 'Use direct method to execute command',
        type: 'boolean',
        default: false,
        demandOption: false
    })
    .argv;

// testHubConnectionString is the Hub connection string used to create our test device against
const testHubConnectionString:string = getTestHubConnectionString()

if (argv.deviceId == null) {
    //
    // The global `before` clause is invoked before other tests are invoked
    //
    before("Initial test device and test child  process creation", testSetup)

    //
    // The global `after` clause is invoked as the final test
    //
    after("Free resources on test completion", testCleanup)
}
else {
    testDeviceInfo = {
        deviceId: argv.deviceId
    }

    if (argv.useDirectMethods) {
        iothubTestCore.setDirectMethodToExecuteCommand();
    }
}

//
// Main tests
//
describe("mainTest", () => {
    afterEach(function() {
        let exitStatus = iothubTestCore.getDeviceExitStatus();
        if (exitStatus != 0) {
            throw new Error(`device exited with status : ${exitStatus}`);
        }
    });

    it("Echo payload", async function() {
        const echoCmd = new e2eTestCommands.CommandTestData_EchoCommands("hello");
        let expectedMessage = await iothubTestCore.runTestCommand(testHubConnectionString, testDeviceInfo.deviceId, echoCmd);
        let rc = await iothubTestCore.verifyTelemetryMessage(testHubConnectionString, testDeviceInfo.deviceId, expectedMessage);
    })

    it("SendTelemetry without payload", async function() {
        let properties = {};
        properties["propertyA"] = "valueA";
        properties["propertyB"] = "valueB";
        const telemetry_cmd = new e2eTestCommands.CommandTestData_SendTelemetryCommands("", properties);
        let expectedMessage = await iothubTestCore.runTestCommand(testHubConnectionString, testDeviceInfo.deviceId, telemetry_cmd);
        let rc = await iothubTestCore.verifyTelemetryMessage(testHubConnectionString, testDeviceInfo.deviceId, expectedMessage);
    })

    it("SendTelemetry without properties", async function() {
        const payload = [(Math.floor(Math.random() * 10000000000))];
        let properties = {};
        const telemetry_cmd = new e2eTestCommands.CommandTestData_SendTelemetryCommands(JSON.stringify(payload), properties);
        let expectedMessage = await iothubTestCore.runTestCommand(testHubConnectionString, testDeviceInfo.deviceId, telemetry_cmd);
        let rc = await iothubTestCore.verifyTelemetryMessage(testHubConnectionString, testDeviceInfo.deviceId, expectedMessage);
    })

    it("SendTelemetry with properties", async function() {
        const payload = [(Math.floor(Math.random() * 10000000000))];
        let properties = {};
        properties["propertyA"] = "valueA";
        properties["propertyB"] = "valueB";
        const telemetry_cmd = new e2eTestCommands.CommandTestData_SendTelemetryCommands(JSON.stringify(payload), properties);
        let expectedMessage = await iothubTestCore.runTestCommand(testHubConnectionString, testDeviceInfo.deviceId, telemetry_cmd);
        let rc = await iothubTestCore.verifyTelemetryMessage(testHubConnectionString, testDeviceInfo.deviceId, expectedMessage);
    })

    /* Note: Removing this test, currenlty we are only supporting topic to be in one packet boundary.
    it("SendTelemetry with large length properties", function(done) {
        const payload = [(Math.floor(Math.random() * 10000000000))]
        let properties = {}
        var characters = "abcdefghijklmnopqrstuvwxyz"
        // The maximum length of property name and value is 1600 to make sure the length of C2D MQTT packet is less than 5000.
        // It is the limitation of Azure RTOS TLS.
        var random_range = 800
        var property_name_length = 800 + Math.floor(Math.random() * random_range)
        var property_value_length = 800 + Math.floor(Math.random() * random_range)
        var property_name = ""
        var property_value = ""
        var i;
        for (i = 0; i < property_name_length; i++) {
            property_name += characters.charAt(Math.floor(Math.random() * characters.length))
        }
        for (i = 0; i < property_value_length; i++) {
            property_value += characters.charAt(Math.floor(Math.random() * characters.length))
        }
        properties[property_name] = property_value

        const telemetry_cmd = new e2eTestCommands.CommandTestData_SendTelemetryCommands(JSON.stringify(payload),
                                                                                        properties)
        let expectedMessage:Promise<string> = iothubTestCore.runTestCommand(testHubConnectionString, testDeviceInfo.deviceId, telemetry_cmd)

        expectedMessage.then(function(message) {
            iothubTestCore.verifyTelemetryMessage(testHubConnectionString, testDeviceInfo.deviceId, message).then(function(result) {
                done(result)
            }).catch(function(error) {
                done(error)
            });
        }).catch(function(error) {
            console.log(`Failed to send message with error: ${error}`)
            done(error)
        });
    }) */

    
    it("Device twin SendReported properties", async function() {
        const payload = { "temp" : (Math.floor(Math.random() * 10000000000).toString())}
        const reported_properties_cmd = new e2eTestCommands.CommandTestData_ReportedPropertiesCommands(JSON.stringify(payload));
        let expectedMessage = await iothubTestCore.runTestCommand(testHubConnectionString, testDeviceInfo.deviceId, reported_properties_cmd);
        let result = await iothubTestCore.verifyTelemetryMessage(testHubConnectionString, testDeviceInfo.deviceId, expectedMessage);
        let rc = iothubTestCore.verifyReportedProperties(testHubConnectionString, testDeviceInfo.deviceId, payload["temp"]);
    })

    
    it("Device twin Receive Desired properties", async function() {
        const payload = { "temp" : (Math.floor(Math.random() * 10000000000).toString())};
        const desired_properties_cmd = new e2eTestCommands.CommandTestData_VerifyDesiredPropertiesCommands("temp", payload["temp"]);
        let twin = await iothubTestCore.postDesiredProperty(testHubConnectionString, testDeviceInfo.deviceId, "temp", payload["temp"]);
        let expectedMessage = await iothubTestCore.runTestCommand(testHubConnectionString, testDeviceInfo.deviceId, desired_properties_cmd);
        let rc = await iothubTestCore.verifyTelemetryMessage(testHubConnectionString, testDeviceInfo.deviceId, expectedMessage);    
    })

    it("Device twin Receive Full twin properties", async function() {
        const payload = { "temp" : (Math.floor(Math.random() * 10000000000).toString())}
        let twin = await iothubTestCore.getTwinProperties(testHubConnectionString, testDeviceInfo.deviceId);
        twin.replace("\\", "");
        const get_twin_cmd = new e2eTestCommands.CommandTestData_GetTwinPropertiesCommands(twin);
        let expectedMessage = await iothubTestCore.runTestCommand(testHubConnectionString, testDeviceInfo.deviceId, get_twin_cmd);
        let rc = await iothubTestCore.verifyTelemetryMessage(testHubConnectionString, testDeviceInfo.deviceId, expectedMessage);
    })
})
