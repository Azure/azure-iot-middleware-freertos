# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

#Requires -Version 5.0

[CmdletBinding()]
Param(
    # ADU account endpoint, e.g. sampleaccount.api.adu.microsoft.com
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $AccountEndpoint,

    # ADU Instance Id.
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $InstanceId,

    # AD Client ID.
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $AzureAdClientId,

    # AD Tenant ID.
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $AzureAdTenantId,

    # AD Application Secret.
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $AzureAdApplicationSecret,

    # Azure Subscription ID.
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $AzureSubscriptionId,

    # Azure Resource Group Name.
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $AzureResourceGroupName,

    # Azure Storage Account Name.
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $AzureStorageAccountName,

    # Azure Blob Container Name.
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $AzureBlobContainerName,

    # Version of update to create and import. For e.g., 1.0
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $UpdateVersion,

    # Group ID to deploy to
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $GroupID,

    # Update provider
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $UpdateProvider,

    # Update name
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $UpdateName
)

Install-Module MSAL.PS -Force
Install-Module Az.Accounts -Force
Install-Module Az.Storage -Force
Import-Module ../e2e_build/iot-hub-device-update/tools/AduCmdlets/AduUpdate.psm1 -ErrorAction Stop -Force
Import-Module ../e2e_build/iot-hub-device-update/tools/AduCmdlets/AduImportUpdate.psm1 -ErrorAction Stop -Force
Import-Module ../e2e_build/iot-hub-device-update/tools/AduCmdlets/AduRestApi.psm1 -ErrorAction Stop -Force

$accountSecret = ConvertTo-SecureString -String $AzureAdApplicationSecret -AsPlainText -Force
$accountCredential = [System.Management.Automation.PSCredential]::new($AzureAdClientId, $accountSecret)
Connect-AzAccount -ServicePrincipal -Credential $accountCredential -Tenant $AzureAdTenantId -ErrorAction Stop
$subscription = Get-AzSubscription -SubscriptionId $AzureSubscriptionId -ErrorAction Stop
Set-AzContext $subscription -ErrorAction Stop | Out-Null
$account = Get-AzStorageAccount -ResourceGroupName $AzureResourceGroupName -Name $AzureStorageAccountName -ErrorAction Stop
$container = Get-AzStorageContainer -Name $AzureBlobContainerName -Context $account.Context -ErrorAction Stop

# Create new update for v1.1
$updateId = New-AduUpdateId -Provider ADU-E2E-Tests -Name Linux-E2E-Update -Version 1.1
$compat = New-AduUpdateCompatibility -Properties @{ deviceManufacturer = 'PC'; deviceModel = 'Linux-E2E' }
$installStep = New-AduInstallationStep -Handler 'microsoft/swupdate:1'-HandlerProperties @{ installedCriteria = '1.1' } -Files '..\e2e_build\azure_iot_e2e_adu_tests_v1.1'
$updateInput = New-AduImportUpdateInput -UpdateId $updateId `
                                         -Compatibility $compat `
                                         -InstallationSteps $installStep `
                                         -BlobContainer $container `
                                         -ErrorAction Stop `
                                         -Verbose:$VerbosePreference

$secureStringAuth = ConvertTo-SecureString $AzureAdApplicationSecret -AsPlainText -Force

$AuthorizationToken = Get-MsalToken `
            -ClientId $AzureAdClientId `
            -TenantId $AzureAdTenantId `
            -Scope 'https://api.adu.microsoft.com/.default' `
            -Authority "https://login.microsoftonline.com/$AzureAdTenantId/v2.0" `
            -ClientSecret $secureStringAuth `
            -ErrorAction Stop

# Import update into blob storage for deployment
$operationId = Start-AduImportUpdate -AccountEndpoint $AccountEndpoint `
                                     -InstanceId $InstanceId `
                                     -AuthorizationToken $AuthorizationToken.AccessToken `
                                     -ImportUpdateInput $updateInput `
                                     -Verbose:$VerbosePreference

Wait-AduUpdateOperation -AccountEndpoint $AccountEndpoint `
                        -InstanceId $InstanceId `
                        -AuthorizationToken $AuthorizationToken.AccessToken `
                        -OperationId $operationId `
                        -Timeout (New-TimeSpan -Minutes 15) `
                        -Verbose:$VerbosePreference


# Deploy the update to the device group
$CurrentTime = [DateTime]::UtcNow.ToString('u')
$DeploymentId = [guid]::NewGuid().toString()

$deploymentUri = "https://$AccountEndpoint/deviceUpdate/$InstanceId/management/groups/$GroupID/deployments/$DeploymentId/?api-version=2021-06-01-preview"
$headers = @{'Authorization' = "Bearer $($AuthorizationToken.AccessToken)"}
$headers.Add('Content-Type', 'application/json')
$payload = @"
{
  "deploymentId": "$DeploymentId",
  "startDateTime": "$CurrentTime",
  "updateId": {
    "provider": "$UpdateProvider",
    "name": "$UpdateName",
    "version": "$UpdateVersion"
  },
  "groupId": "$GroupID"
}
"@

Write-Host($deploymentUri)
Write-Host($headers)
Write-Host($payload)

$response = Invoke-WebRequest -Uri $deploymentUri -Method PUT -Headers $headers -Body $payload -UseBasicParsing -Verbose:$VerbosePreference

Write-Host($response)

# Get the status of the deployment
$deploymenetStatusUri = "https://$AccountEndpoint/deviceupdate/$InstanceId/management/groups/$groupId/deployments/$deploymentId/status?api-version=2021-06-01-preview"

$statusResponse = Invoke-WebRequest -Uri $deploymenetStatusUri -Method GET -Headers $headers -UseBasicParsing -Verbose:$VerbosePreference

Write-Host($statusResponse)
