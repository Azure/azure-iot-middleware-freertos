# How to set this up

Need to set up an Azure Active Directory application for the ADU instance and the container.

Directions here: https://learn.microsoft.com/en-us/azure/iot-hub-device-update/device-update-control-access#authenticate-to-device-update-rest-apis-for-publishing-and-management

PR to fix empty

https://github.com/Azure/iot-hub-device-update/pull/215

Make sure to add permissions in both Azure Storage and Azure Device Update to the application in Azure AD


## Needed Check List

[] Need to write e2e test binary.
[] Modify E2E test infrastructure to read twin and make sure it's on the correct version.
    [] Possibly think about having a scenario where the first exec is killed and boot up v1.1
[] 
