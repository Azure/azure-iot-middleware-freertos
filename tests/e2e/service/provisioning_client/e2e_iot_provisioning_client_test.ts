// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// These TypeScript tests are the test driver for E2E tests, acting as service application
// calling into an EXE hosting the C Azure IoT device SDK.
// See ../../readme.md for a full architectural review of how it works.
'use strict'

import { CommandTestData_DeviceProvisioningCommands } from '../common/e2e_test_commands';
const e2eTestCore = require ('../common/e2e_test_core');
// Time, in 60s, for event hub partition polling.
const eventHubTimeout:number = 60000;

// testHubInfo contains the settings for our test device on the IoTHub.
let testHubInfo:any = null;
// testEnrollmentInfo contains the settings for our test Enrollment on the Device Provisioning.
let testEnrollmentInfo:any = null;

//
// testSetup creates a device for test scenario and creates the test process that hosts the C SDK
//
function testSetup(done) {
    if (argv.useDirectMethods) {
        e2eTestCore.setDirectMethodToExecuteCommand();
    }

    if (argv.deviceId == null) {
        e2eTestCore.createTestDeviceAndTestProcess(testHubConnectionString, argv.testexe, (err, newDeviceInfo) => {
            if (err) {
                done(err)
            }
            else {
                // Store information about this device for later
                testHubInfo = newDeviceInfo
                e2eTestCore.createProvisioningEnrollment(testProvisioningConnectionString, "azure_services_port_e2e_", (err, newEnrollmentInfo) => {
                    // Store information about this device for later
                    testEnrollmentInfo = newEnrollmentInfo
                    done(err)
                });
            }
        })
    }
    else {
        testHubInfo = {
            deviceId: argv.deviceId
        }

        testEnrollmentInfo = {
            deviceId: argv.deviceId,
            registrationId: argv.deviceId,
            attestation: {
                symmetricKey: {
                    primaryKey: argv.deviceSymKey
                }
            }
        }

        if (argv.deviceSymKey == null) {
            done(Error ("invalid argument: deviceSymKey can't be null"))
        }
        else {
            done();
        }
    }
}

//
// testCleanup deletes the device and forces the child test process, if still running, to terminate.
//
function testCleanup(done) {
    if (argv.deviceId == null) {
        e2eTestCore.deleteDeviceAndEndProcess(testHubConnectionString, testHubInfo, (err) => {
            if (testEnrollmentInfo) {
                e2eTestCore.deleteProvisioningEnrollment(testProvisioningConnectionString, testEnrollmentInfo.registrationId, (err2) => {
                    e2eTestCore.deleteDevice(testHubConnectionString, testEnrollmentInfo.deviceId, (e) => {
                        if (err) {
                            done(err)
                        }
                        else if (err2) {
                            done(err2)
                        }
                        else {
                            done()
                        }
                    });
                });
            }
            else {
                done(err)
            }
        });
    }
}

//
// getTestHubConnectionString returns hub's connection string, from either command line or environment
//
function getTestHubConnectionString(): string {
    if (argv.hubConnectionString != null) {
        return argv.hubConnectionString
    }
    else if (process.env.IOTHUB_CONNECTION_STRING != null) {
        return process.env.IOTHUB_CONNECTION_STRING
    }
    else {
        throw "Hub connection string must be configured in the command line or environment variable IOTHUB_CONNECTION_STRING"
    }
}

//
// getTestProvisioningConnectionString returns provisioning's connection string, from either command line or environment
//
function getTestProvisioningConnectionString(): string {
    if (argv.provConnectionString != null) {
        return argv.provConnectionString
    }
    else if (process.env.IOT_PROVISIONING_CONNECTION_STRING != null) {
        return process.env.IOT_PROVISIONING_CONNECTION_STRING
    }
    else {
        throw "Provisioning connection string must be configured in the command line or environment variable IOT_PROVISIONING_CONNECTION_STRING"
    }
}

// Note: provisioning service should be link to the iothub whose connection string is passed, required to do proper cleanup
let argv = require('yargs')
    .usage('Usage: $0 --hubConnectionString <HUB CONNECTION STRING> --provConnectionString <PROVISIONING CONNECTION STRING> --testexe <PATH OF EXECUTABLE TO HOST C SDK TESTS>')
    .option('hubConnectionString', {
        alias: 'c',
        describe: 'The connection string for the *IoTHub* instance to run tests against',
        type: 'string',
        demandOption: false
    })
    .option('provConnectionString', {
        alias: 'p',
        describe: 'The connection string for the *Provisioning* service to run tests against',
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
    .option('idScope', {
        alias: 's',
        describe: 'Id Scope for the Provisioning service',
        type: 'string',
        demandOption: true
    })
    .option('deviceSymKey', {
        alias: 'k',
        describe: 'Device symmetric key for debug',
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

// testProvisioningConnectionString is the Provisioning connection string used to create our test enrollment
const testProvisioningConnectionString:string = getTestProvisioningConnectionString()

//
// The global `before` clause is invoked before other tests are invoked
//
before("Initial test device and test child  process creation", testSetup)
//
// The global `after` clause is invoked as the final test
//
after("Free resources on test completion", testCleanup)

//
// Main tests
//
describe("mainTest", () => {
    afterEach(function() {
        let exitStatus = e2eTestCore.getDeviceExitStatus();
        if (exitStatus != 0) {
            throw new Error(`device exited with status : ${exitStatus}`)
        }
    });

    it("Get Device Provisioning data test", async function() {
        let serviceEndpoint = e2eTestCore.getHubnameFromConnectionString(testProvisioningConnectionString);
        const device_provisioning = new CommandTestData_DeviceProvisioningCommands(serviceEndpoint, argv.idScope, testEnrollmentInfo);
        let expectedMessage = await e2eTestCore.runTestCommand(testHubConnectionString, testHubInfo.deviceId, device_provisioning);
        let result = await e2eTestCore.verifyTelemetryMessageWithTimeout(testHubConnectionString, testHubInfo.deviceId, expectedMessage, eventHubTimeout);
    })
})
