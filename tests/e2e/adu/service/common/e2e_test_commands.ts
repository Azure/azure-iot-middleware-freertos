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
// CommandTestData_SendInitADUState is used for exiting
//
class CommandTestData_SendInitADUState extends CommandTestData {
  constructor() {
      super("send_init_adu_state")
  }
}

//
// CommandTestData_GetADUTwinPropertyCommands is used for exiting
//
class CommandTestData_GetADUTwinPropertyCommands extends CommandTestData {
  constructor() {
      super("get_adu_twin")
  }
}

//
// CommandTestData_ApplyUpdate is used for exiting
//
class CommandTestData_ApplyUpdate extends CommandTestData {
  constructor() {
      super("apply_update")
  }
}

//
// CommandTestData_VerifyFinalADUState is used for exiting
//
class CommandTestData_VerifyFinalADUState extends CommandTestData {
  constructor() {
      super("verify_final_state")
  }
}

export {
    CommandTestData,
    CommandTestData_EchoCommands,
    CommandTestData_ExitCommands,
    CommandTestData_SendInitADUState,
    CommandTestData_GetADUTwinPropertyCommands,
    CommandTestData_ApplyUpdate,
    CommandTestData_VerifyFinalADUState,
};
