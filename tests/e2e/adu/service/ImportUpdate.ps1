#
# Device Update for IoT Hub
# Sample PowerShell script for creating and importing a complex update with mixed installation steps.
# Copyright (c) Microsoft Corporation.
#

#Requires -Version 5.0
#Requires -Modules Az.Accounts, Az.Storage

<#
    .SYNOPSIS
        Create a sample update with mix of inline and reference install steps, and importing it to Device Update for IoT Hub
        through REST API. This sample update contain fake files and cannot be actually installed onto a device.

    .EXAMPLE
        PS > Import-Module AduImportUpdate.psm1
        PS >
        PS > $BlobContainer = Get-AduAzBlobContainer -SubscriptionId $subscriptionId -ResourceGroupName $resourceGroupName -StorageAccountName $storageAccount -ContainerName $BlobContainerName
        PS >
        PS > $token = Get-MsalToken -ClientId $clientId -TenantId $tenantId -Scopes 'https://api.adu.microsoft.com/user_impersonation' -Authority https://login.microsoftonline.com/$tenantId/v2.0 -Interactive -DeviceCode
        PS >
        PS > ImportSampleComplexUpdate.ps1 -AccountEndpoint sampleaccount.api.adu.microsoft.com -InstanceId sampleinstance `
                                           -Container $BlobContainer `
                                           -AuthorizationToken $token `
                                           -UpdateVersion 1.0
#>
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

    # Azure Storage Blob container for staging update artifacts.
    [Parameter(Mandatory=$true)]
    [ValidateNotNull()]
    [Microsoft.WindowsAzure.Commands.Common.Storage.ResourceModel.AzureStorageContainer] $BlobContainer,

    # Azure Active Directory OAuth authorization token.
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $AuthorizationToken,

    # Version of update to create and import. For e.g., 1.0
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $UpdateVersion
)

Import-Module $PSScriptRoot\AduImportUpdate.psm1 -ErrorAction Stop
Import-Module $PSScriptRoot\AduRestApi.psm1 -ErrorAction Stop

# We will use arbitrary files as update payload files.
$childFile = "$env:TEMP/childFile.bin.txt"
$parentFile = "$env:TEMP/parentFile.bin.txt"
"This is a child update payload file." | Out-File $childFile -Force -Encoding utf8
"This is a parent update payload file." | Out-File $parentFile -Force -Encoding utf8

# ------------------------------
# Create a child update
# ------------------------------
Write-Host 'Preparing child update ...'

$microphoneUpdateId = New-AduUpdateId -Provider Contoso -Name Microphone -Version $UpdateVersion
$microphoneCompat = New-AduUpdateCompatibility -DeviceManufacturer Contoso -DeviceModel Microphone
$microphoneInstallStep = New-AduInstallationStep -Handler 'microsoft/swupdate:1' -Files $childFile
$microphoneUpdate = New-AduImportUpdateInput -UpdateId $microphoneUpdateId `
                                             -IsDeployable $false `
                                             -Compatibility $microphoneCompat `
                                             -InstallationSteps $microphoneInstallStep `
                                             -BlobContainer $BlobContainer -ErrorAction Stop -Verbose:$VerbosePreference

# ------------------------------
# Create another child update
# ------------------------------
Write-Host 'Preparing another child update ...'

$speakerUpdateId = New-AduUpdateId -Provider Contoso -Name Speaker -Version $UpdateVersion
$speakerCompat = New-AduUpdateCompatibility -DeviceManufacturer Contoso -DeviceModel Speaker
$speakerInstallStep = New-AduInstallationStep -Handler 'microsoft/swupdate:1' -Files $childFile
$speakerUpdate = New-AduImportUpdateInput -UpdateId $speakerUpdateId `
                                          -IsDeployable $false `
                                          -Compatibility $speakerCompat `
                                          -InstallationSteps $speakerInstallStep `
                                          -BlobContainer $BlobContainer -ErrorAction Stop -Verbose:$VerbosePreference

# ------------------------------------------------------------
# Create the parent update which parents the child update above
# ------------------------------------------------------------
Write-Host 'Preparing parent update ...'

$parentUpdateId = New-AduUpdateId -Provider Contoso -Name Toaster -Version $UpdateVersion
$parentCompat = New-AduUpdateCompatibility -DeviceManufacturer Contoso -DeviceModel Toaster
$parentSteps = @()
$parentSteps += New-AduInstallationStep -Handler 'microsoft/script:1' -Files $parentFile -HandlerProperties @{ 'arguments'='--pre'} -Description 'Pre-install script'
$parentSteps += New-AduInstallationStep -UpdateId $microphoneUpdateId -Description 'Microphone Firmware'
$parentSteps += New-AduInstallationStep -UpdateId $speakerUpdateId -Description 'Speaker Firmware'
$parentSteps += New-AduInstallationStep -Handler 'microsoft/script:1' -Files $parentFile -HandlerProperties @{ 'arguments'='--post'} -Description 'Post-install script'

$parentUpdate = New-AduImportUpdateInput -UpdateId $parentUpdateId `
                                         -Compatibility $parentCompat `
                                         -InstallationSteps $parentSteps `
                                         -BlobContainer $BlobContainer -ErrorAction Stop -Verbose:$VerbosePreference

# -----------------------------------------------------------
# Call ADU Import Update REST API to import the updates above
# -----------------------------------------------------------
Write-Host 'Importing update ...'

$operationId = Start-AduImportUpdate -AccountEndpoint $AccountEndpoint -InstanceId $InstanceId -AuthorizationToken $AuthorizationToken `
                                     -ImportUpdateInput $parentUpdate, $microphoneUpdate, $speakerUpdate -ErrorAction Stop -Verbose:$VerbosePreference

Wait-AduUpdateOperation -AccountEndpoint $AccountEndpoint -InstanceId $InstanceId -AuthorizationToken $AuthorizationToken `
                        -OperationId $operationId -Timeout (New-TimeSpan -Minutes 15) -Verbose:$VerbosePreference

Remove-Item $childFile -Force -ErrorAction SilentlyContinue | Out-Null
Remove-Item $parentFile -Force -ErrorAction SilentlyContinue | Out-Null
