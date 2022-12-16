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

# Errors in system routines will stop script execution
$errorActionPreference = "stop"
$VerbosePreference = "continue"

Install-Module MSAL.PS -Force
Install-Module Az.Accounts -Force
Install-Module Az.Storage -Force
Import-Module ../e2e_build/iot-hub-device-update/tools/AduCmdlets/AduUpdate.psm1 -ErrorAction Stop -Force
Import-Module ../e2e_build/iot-hub-device-update/tools/AduCmdlets/AduAzStorageBlobHelper.psm1 -ErrorAction Stop -Force
Import-Module ../e2e_build/iot-hub-device-update/tools/AduCmdlets/AduRestApi.psm1 -ErrorAction Stop -Force

Write-Host("Get auth token for ADU operations")
$secureStringAuth = ConvertTo-SecureString $AzureAdApplicationSecret -AsPlainText -Force
$AuthorizationToken = Get-MsalToken `
            -ClientId $AzureAdClientId `
            -TenantId $AzureAdTenantId `
            -Scope 'https://api.adu.microsoft.com/.default' `
            -Authority "https://login.microsoftonline.com/$AzureAdTenantId/v2.0" `
            -ClientSecret $secureStringAuth `
            -ErrorAction Stop

Write-Host("Get previous deployments")
$getUpdatesUri = "https://$AccountEndpoint/deviceupdate/$InstanceId/management/groups/$groupId/deployments/?api-version=2022-10-01"
$authHeaders = @{'Authorization' = "Bearer $($AuthorizationToken.AccessToken)"}
$authHeaders.Add('Content-Type', 'application/json')
$getResponse = Invoke-WebRequest -Uri $getUpdatesUri -Method GET -Headers $authHeaders -UseBasicParsing -Verbose:$VerbosePreference
Write-Host($getResponse)

$parsedJsonResponse = $getResponse | ConvertFrom-Json

foreach ($deployment in $parsedJsonResponse.value)
{
  Write-Host("Deleting previous deployment: $($deployment.deploymentId)")
  $deleteDeploymentUri = "https://$AccountEndpoint/deviceupdate/$InstanceId/management/groups/$groupId/deployments/$($deployment.deploymentId)/?api-version=2022-10-01"
  $deleteDeploymentResponse = Invoke-WebRequest -Uri $deleteDeploymentUri -Method DELETE -Headers $authHeaders -UseBasicParsing -Verbose:$VerbosePreference
}

Write-Host("Deleting previous 1.2 update")
$deleteUpdateUri = "https://$AccountEndpoint/deviceupdate/$InstanceId/updates/providers/$UpdateProvider/names/$UpdateName/versions/$UpdateVersion/?api-version=2022-10-01"
$deleteUpdateResponse = Invoke-WebRequest -Uri $deleteUpdateUri -Method DELETE -Headers $authHeaders -UseBasicParsing -Verbose:$VerbosePreference
$operationId = $deleteUpdateResponse.Headers["Operation-Location"].Split('/')[-1].Split('?')[0]
Wait-AduUpdateOperation -AccountEndpoint $AccountEndpoint `
                        -InstanceId $InstanceId `
                        -AuthorizationToken $AuthorizationToken.AccessToken `
                        -OperationId $operationId `
                        -Timeout (New-TimeSpan -Minutes 2) `
                        -Verbose:$VerbosePreference

# Set up Azure Account using the passed AAD secret for blob storage
$accountSecret = ConvertTo-SecureString -String $AzureAdApplicationSecret -AsPlainText -Force
$accountCredential = [System.Management.Automation.PSCredential]::new($AzureAdClientId, $accountSecret)
Connect-AzAccount -ServicePrincipal -Credential $accountCredential -Tenant $AzureAdTenantId -ErrorAction Stop
$subscription = Get-AzSubscription -SubscriptionId $AzureSubscriptionId -ErrorAction Stop
Set-AzContext $subscription -ErrorAction Stop | Out-Null
$account = Get-AzStorageAccount -ResourceGroupName $AzureResourceGroupName -Name $AzureStorageAccountName -ErrorAction Stop
$container = Get-AzStorageContainer -Name $AzureBlobContainerName -Context $account.Context -ErrorAction Stop

# Create new update for v1.2
$updateId = New-AduUpdateId -Provider ADU-E2E-Tests -Name Linux-E2E-Update -Version 1.2
$fileName = "azure_iot_e2e_adu_tests_v1.2"

Write-Host("Uploading update file(s) to Azure Blob Storage.")
$updateIdStr = "$($updateId.Provider).$($updateId.Name).$($updateId.Version)"

# Upload update to blob
# Place files within updateId subdirectory in case there are filenames conflict.
$blobName = "$updateIdStr/$fileName"
$importFileUrl = Copy-AduFileToAzBlobContainer -FilePath "..\e2e_build\$fileName" -BlobName $blobName -BlobContainer $container -ErrorAction Stop

# Create import manifest and upload to blob
az config set extension.use_dynamic_install=yes_without_prompt
$importMan = az iot du update init v5 --update-provider $updateId.Provider --update-name $updateId.Name --update-version $updateId.Version --compat deviceModel=Linux-E2E deviceManufacturer=PC --step handler=microsoft/swupdate:1 properties='{"installedCriteria":"1.2"}' --file path=../e2e_build/$fileName
$importManJsonFile = New-TemporaryFile
$importMan | Out-File $importManJsonFile -Encoding utf8

Get-Content $importManJsonFile | Write-Verbose

$importManJsonFile = Get-Item $importManJsonFile # refresh file properties
$importManJsonHash = Get-AduFileHashes -FilePath $importManJsonFile -ErrorAction Stop

$importManUrl = Copy-AduFileToAzBlobContainer -FilePath $importManJsonFile -BlobName "$updateIdStr/importmanifest.json" -BlobContainer $container -ErrorAction Stop

Write-Host("Preparing Import Update API request body.")

$importManMeta = [ordered] @{
    url = $importManUrl
    sizeInBytes = $importManJsonFile.Length
    hashes = $importManJsonHash
}

$updateInput = [ordered] @{
    importManifest = $importManMeta
    files = @(
      @{
        filename = $fileName
        url = $importFileUrl
      }
    )
}

Remove-Item $importManJsonFile

# Import update into ADU for deployment
Write-Host("Starting import")
$operationId = Start-AduImportUpdate -AccountEndpoint $AccountEndpoint `
                                     -InstanceId $InstanceId `
                                     -AuthorizationToken $AuthorizationToken.AccessToken `
                                     -ImportUpdateInput $updateInput `
                                     -Verbose:$VerbosePreference
Wait-AduUpdateOperation -AccountEndpoint $AccountEndpoint `
                        -InstanceId $InstanceId `
                        -AuthorizationToken $AuthorizationToken.AccessToken `
                        -OperationId $operationId `
                        -Timeout (New-TimeSpan -Minutes 5) `
                        -Verbose:$VerbosePreference


# Deploy the update to the device group
$currentTime = [DateTime]::UtcNow.ToString('u')
$deploymentId = [guid]::NewGuid().toString()
$deploymentUri = "https://$AccountEndpoint/deviceUpdate/$InstanceId/management/groups/$GroupID/deployments/$deploymentId/?api-version=2022-10-01"
$payload = @"
{
  "deploymentId": "$deploymentId",
  "startDateTime": "$currentTime",
  "update": {
    "updateId": {
      "provider": "$UpdateProvider",
      "name": "$UpdateName",
      "version": "$UpdateVersion"
    }
  },
  "groupId": "$GroupID"
}
"@

Write-Host($deploymentUri)
Write-Host($payload)

$response = Invoke-WebRequest -Uri $deploymentUri -Method PUT -Headers $authHeaders -Body $payload -UseBasicParsing -Verbose:$VerbosePreference

Write-Host($response)

# Get the status of the deployment
$deploymentStatusUri = "https://$AccountEndpoint/deviceupdate/$InstanceId/management/groups/$groupId/deployments/$deploymentId/status?api-version=2022-10-01"
$statusResponse = Invoke-WebRequest -Uri $deploymentStatusUri -Method GET -Headers $authHeaders -UseBasicParsing -Verbose:$VerbosePreference

Write-Host($statusResponse)
