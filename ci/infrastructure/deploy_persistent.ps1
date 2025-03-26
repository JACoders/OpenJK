$ErrorActionPreference = 'Stop'

$resourceGroup = 'jkaded-persistent-rg'

$deploymentPrefix = 'jkaded'
$imageName = 'jka_ded'
$imageTag = 'latest'
$acrName = "${deploymentPrefix}acr"
$acrResourceGroup = "jk"

# Create the rg
az group create `
    --name $resourceGroup `
    --location 'eastus';

if ($LASTEXITCODE) {
    exit $LASTEXITCODE;
}

# Deploy ACR bicep
az deployment group create `
    --resource-group $resourceGroup `
    --template-file "$PSScriptRoot/acr.bicep" `
    --parameters acrName=$acrName;

if ($LASTEXITCODE) {
    exit $LASTEXITCODE;
}

# Authenticate to the ACR
az acr login --name $acrName

if ($LASTEXITCODE) {
    exit $LASTEXITCODE;
}

# Build & tag image
docker build -t $imageName .

if ($LASTEXITCODE) {
    exit $LASTEXITCODE;
}

$acrFullImageName = "${acrName}.azurecr.io/${imageName}:${imageTag}";

docker tag $imageName $acrFullImageName;
docker push $acrFullImageName;