// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

//
//  e2e_test_cleanup.ts implements test resource cleanup script.
//
'use strict'

import { Registry } from "azure-iothub"

async function iothubRegistryCleanup(hubConnectionString:string, registryPrefix:string, expiryDate:Date) {
    let registry = await Registry.fromConnectionString(hubConnectionString);
    let response = await registry.list();
    let expiredDevicesDescription = [];
    let devices = response.responseBody;
    let newDeviceDefaultDate = new Date("0001-01-01T00:00:00Z");

    for(var device of devices) {
        let date = new Date(device.lastActivityTime)

        if ((date < expiryDate) && device.deviceId.startsWith(registryPrefix)) {

            /* if device was never active 0001-01-01T00:00:00Z */
            if (date.valueOf() == newDeviceDefaultDate.valueOf()) {
                /* Check twin properties last-update date */
                let twinResponse = await registry.getTwin(device.deviceId);
                let reportedPropertyDate = new Date(twinResponse.responseBody.properties.desired.$metadata.$lastUpdated);
                if (reportedPropertyDate > expiryDate) {
                    continue;
                }
            }

            expiredDevicesDescription.push({
                deviceId: device.deviceId
            });
        }
    }

    expiredDevicesDescription.forEach((des) => {
        console.log("Deleting device: " + des.deviceId)
    })

    await registry.removeDevices(expiredDevicesDescription, true);
}

let argv = require('yargs')
    .usage('Usage: $0 --connectionString <HUB CONNECTION STRING> --cleanupAfter <Days after Registry should be cleaned up>')
    .option('connectionString', {
        alias: 'c',
        describe: 'The connection string for the *IoTHub* instance to run tests against',
        type: 'string',
        demandOption: false
    })
    .option('cleanupAfter', {
        alias: 'e',
        describe: 'Days after DeviceId should be cleaned up',
        type: 'number',
        demandOption: false
    })
    .argv;

async function main() {
    let e2eTestDevicePrefix = "azure_services_port_e2e_"
    /* 86400000 is millisecond epoch in one day */
    let timeStamp:number = Date.now().valueOf() - 86400000 * argv.cleanupAfter;
    let expiryDate =  new Date(timeStamp);
    try {
        await iothubRegistryCleanup(argv.connectionString, e2eTestDevicePrefix, expiryDate);
    } catch (error) {
        console.log(error)
    }
}

main();
