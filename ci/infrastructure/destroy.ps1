$resourceGroup = 'jka-ded-rg';

az group delete `
    --name $resourceGroup `
    --yes;