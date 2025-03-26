$ErrorActionPreference = 'Stop'

$resourceGroup = 'jka-ded-rg'
$acrResourceGroup = 'jkaded-persistent-rg'

$deploymentPrefix = 'jkaded'
$imageName = 'jkded'
$imageTag = 'latest'
$acrName = "${deploymentPrefix}acr"

# Create the rg
az group create `
    --name $resourceGroup `
    --location 'eastus';

az deployment group create --resource-group $resourceGroup `
    --template-file "$PSScriptRoot/app.bicep" `
    --parameters `
        imageName=${imageName}:${imageTag} `
        prefix=$deploymentPrefix `
        acrName=$acrName `
        acrResourceGroup=$acrResourceGroup;