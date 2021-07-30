// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

'use strict'

import { anHourFromNow } from 'azure-iot-common';
import { SharedAccessSignature } from 'azure-iothub';

//
// isEmpty check if object is empty.
//
function checkIsEmpty(obj) {
    for(let key in obj) {
        if(obj.hasOwnProperty(key)) {
            return false;
        }
    }

    return true;
}

//
// CommandTestData stores the expected test data for a particular test run.  ALL tests - telemetry, properties, and commands -
// are initiated by a command that this test framework invokes on the test executable.
//
class CommandTestData {
    protected payload:Object
    protected properties:Object

    constructor(methodName:string) {
        this.payload = {};
        this.payload["method"] = methodName
        this.properties = {}
    }

    Serialize(): string {
        return JSON.stringify(this.payload)
    }

    GetMethod(): string {
        return this.payload["method"];
    }

    GetProperties(): Object {
        return this.properties
    }

    GetDataExpected(): string {
        return JSON.stringify(this.payload)
    }
}

//
// CommandTestData_EchoCommands is echo
//
class CommandTestData_EchoCommands extends CommandTestData {
    constructor(payload:string) {
        super("echo")
        this.payload["payload"] = payload
        this.properties = {}
    }
}

//
// CommandTestData_ExitCommands is used for exiting
//
class CommandTestData_ExitCommands extends CommandTestData {
    constructor() {
        super("exit")
    }
}

//
// CommandTestData_SendTelemetryCommands is used for testing SendTelemetry
//
class CommandTestData_SendTelemetryCommands extends CommandTestData {
    constructor(payload:string, properties) {
        super("send_telemetry")
        this.payload["payload"] = payload
        if (!checkIsEmpty(properties)) {
            this.payload["properties"] = properties
        }
        this.properties = properties
    }

    GetDataExpected(): string {
        let message = ""

        if (!checkIsEmpty(this.payload["payload"])) {
            message += this.payload["payload"]
        }

        if (!checkIsEmpty(this.properties)) {
            message += JSON.stringify(this.properties)
        }

        return message
    }
}

//
// CommandTestData_DeviceProvisioningCommands is used for testing Device Provisioning
//
class CommandTestData_DeviceProvisioningCommands extends CommandTestData {
    protected result:Object;

    constructor(serviceEndpoint:string, id_scope:string, enrollmentForSymKeyDevice:any) {
        super("device_provisioning")
        this.payload["id_scope"] = id_scope
        this.payload["service_endpoint"] = serviceEndpoint;
        this.payload["registration_id"] = enrollmentForSymKeyDevice.registrationId
        this.payload["symmetric_key"] = enrollmentForSymKeyDevice.attestation.symmetricKey.primaryKey
        this.result = {}
        this.result["device"] = enrollmentForSymKeyDevice.deviceId
        this.result["status"] = "OK"
    }

    GetDataExpected(): string {
        return JSON.stringify(this.result)
    }
}

//
// CommandTestData_ReportedPropertiesCommands is used for testing Device twin reported properties
//
class CommandTestData_ReportedPropertiesCommands extends CommandTestData {
    constructor(payload:string) {
        super("reported_properties")
        this.payload["payload"] = payload
    }

    GetDataExpected(): string {
        return JSON.stringify(this.payload)
    }
}

//
// CommandTestData_VerifyDesiredPropertiesCommands is used for testing Device twin desired properties
//
class CommandTestData_VerifyDesiredPropertiesCommands extends CommandTestData {
    protected result:Object;

    constructor(desired_property_key:string, desired_property_value:string) {
        super("verify_desired_properties")
        this.payload["desired_property_key"] = desired_property_key;
        this.payload["desired_property_value"] = desired_property_value;
        this.result = {}
        this.result[desired_property_key] = desired_property_value
    }

    GetDataExpected(): string {
        return JSON.stringify(this.result)
    }
}

//
// CommandTestData_GetTwinPropertiesCommands is used for testing Device twin properties
//
class CommandTestData_GetTwinPropertiesCommands extends CommandTestData {
    protected result:string;

    constructor(twin:string) {
        super("get_twin_properties")
        this.result = twin
    }

    GetDataExpected(): string {
        return this.result
    }
}

//
// CommandTestData_WritablePropertiesResponseCommands is used for testing Device writable property response
//
class CommandTestData_WritablePropertiesResponseCommands extends CommandTestData {
    constructor(desired_property_key:string, desired_property_value:string) {
        super("get_writable_properties_response")
        this.payload["desired_property_key"] = desired_property_key;
        this.payload["desired_property_value"] = desired_property_value;
    }
}

//
// CommandTestData_ReportComponentPropertiesCommands is used for testing reported properties for component
//
class CommandTestData_ReportComponentPropertiesCommands extends CommandTestData {

    constructor(component:string, desired_property_key:string, desired_property_value:string) {
        super("report_component_properties")
        this.payload["desired_property_key"] = desired_property_key;
        this.payload["desired_property_value"] = desired_property_value;
        this.payload["component"] = component;
    }
}

export {
    CommandTestData,
    CommandTestData_EchoCommands,
    CommandTestData_ExitCommands,
    CommandTestData_SendTelemetryCommands,
    CommandTestData_DeviceProvisioningCommands,
    CommandTestData_ReportedPropertiesCommands,
    CommandTestData_VerifyDesiredPropertiesCommands,
    CommandTestData_GetTwinPropertiesCommands,
    CommandTestData_WritablePropertiesResponseCommands,
    CommandTestData_ReportComponentPropertiesCommands
};
