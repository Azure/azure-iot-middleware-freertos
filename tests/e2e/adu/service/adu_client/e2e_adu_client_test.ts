// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// These TypeScript tests are the test driver for E2E tests, acting as service application
// calling into an EXE hosting the C Azure IoT device SDK.
// See ../../../readme.md for a full architectural review of how it works.
'use strict'

import assert from 'assert';

const iothubTestCore = require ('../common/e2e_test_core');
const e2eTestCommands = require ('../common/e2e_test_commands');

// testDeviceInfo contains the settings for our test device on the IoTHub.
let testDeviceInfo:any = null;

let aduDeviceStateIdle:number = 0;
let aduDeviceStateInProgress: number = 6;
let aduDeviceTwinAccepted: number = 200;


//
// testSetup creates a device for test scenario and creates the test process that hosts the C SDK
//
function testSetup(done) {
    if (argv.useCommands) {
        iothubTestCore.setCommandToExecuteCommand();
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
    else if (process.env.ADU_IOTHUB_CONNECTION_STRING != null) {
        return process.env.ADU_IOTHUB_CONNECTION_STRING
    }
    else {
        throw "Hub connection string must be configured in the command line or environment variable ADU_IOTHUB_CONNECTION_STRING"
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
    .option('useCommands', {
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
    before("Initial test device and test child process creation", function(done) {
      // Set up will wait for the ADU payload to arrive (10 minutes max)
      this.timeout(10 * 60 * 1000)
      testSetup(done)
    })

    //
    // The global `after` clause is invoked as the final test
    //
    after("Free resources on test completion", testCleanup)
}
else {
    testDeviceInfo = {
        deviceId: argv.deviceId
    }

    if (argv.useCommands) {
        iothubTestCore.setCommandToExecuteCommand();
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

    it("Send initial ADU state", async function() {
        const sendInitState = new e2eTestCommands.CommandTestData_SendInitADUState();
        let expectedMessage = await iothubTestCore.runTestCommand(testHubConnectionString, testDeviceInfo.deviceId, sendInitState);
        let rc = await iothubTestCore.verifyTelemetryMessage(testHubConnectionString, testDeviceInfo.deviceId, expectedMessage);
        let twin = await iothubTestCore.getTwinProperties(testHubConnectionString, testDeviceInfo.deviceId);
        console.log(twin)
        var twinObject = JSON.parse(twin);
        assert.ok(twinObject.reported.deviceUpdate);
        assert.ok(twinObject.reported.deviceUpdate.agent);
        assert.strictEqual(twinObject.reported.deviceUpdate.agent.state, aduDeviceStateIdle);
        assert.strictEqual(twinObject.reported.deviceUpdate.agent.installedUpdateId, "{\"provider\":\"ADU-E2E-Tests\",\"name\":\"Linux-E2E-Update\",\"version\":\"1.0\"}")
    })

    it("Get full ADU twin doc", async function() {
      const getADUTwin = new e2eTestCommands.CommandTestData_GetADUTwinPropertyCommands();
      let expectedMessage = await iothubTestCore.runTestCommand(testHubConnectionString, testDeviceInfo.deviceId, getADUTwin);
      let rc = await iothubTestCore.verifyTelemetryMessage(testHubConnectionString, testDeviceInfo.deviceId, expectedMessage);
    })

    it("Apply ADU update", async function() {
      const applyUpdate = new e2eTestCommands.CommandTestData_ApplyUpdate();
      let expectedMessage = await iothubTestCore.runTestCommand(testHubConnectionString, testDeviceInfo.deviceId, applyUpdate);
      let rc = await iothubTestCore.verifyTelemetryMessage(testHubConnectionString, testDeviceInfo.deviceId, expectedMessage);
      let twin = await iothubTestCore.getTwinProperties(testHubConnectionString, testDeviceInfo.deviceId);
      console.log(twin)
      var twinObject = JSON.parse(twin);
      assert.ok(twinObject.reported.deviceUpdate);
      assert.ok(twinObject.reported.deviceUpdate.agent);
      assert.strictEqual(twinObject.reported.deviceUpdate.agent.state, aduDeviceStateInProgress);
      assert.strictEqual(twinObject.reported.deviceUpdate.agent.installedUpdateId, "{\"provider\":\"ADU-E2E-Tests\",\"name\":\"Linux-E2E-Update\",\"version\":\"1.0\"}")
      assert.ok(twinObject.reported.deviceUpdate.service);
      assert.ok(twinObject.reported.deviceUpdate.service.ac);
      assert.strictEqual(twinObject.reported.deviceUpdate.service.ac, aduDeviceTwinAccepted);
    })

    it("Verify Final ADU State", async function() {
        const verifyFinalState = new e2eTestCommands.CommandTestData_VerifyFinalADUState();
        let expectedMessage = await iothubTestCore.runTestCommand(testHubConnectionString, testDeviceInfo.deviceId, verifyFinalState);
        let rc = await iothubTestCore.verifyTelemetryMessage(testHubConnectionString, testDeviceInfo.deviceId, expectedMessage);
        let twin = await iothubTestCore.getTwinProperties(testHubConnectionString, testDeviceInfo.deviceId);
        console.log(twin)
        var twinObject = JSON.parse(twin);
        assert.ok(twinObject.reported.deviceUpdate);
        assert.ok(twinObject.reported.deviceUpdate.agent);
        assert.strictEqual(twinObject.reported.deviceUpdate.agent.state, aduDeviceStateIdle);
        assert.strictEqual(twinObject.reported.deviceUpdate.agent.installedUpdateId, "{\"provider\":\"ADU-E2E-Tests\",\"name\":\"Linux-E2E-Update\",\"version\":\"1.1\"}")
    })

})
