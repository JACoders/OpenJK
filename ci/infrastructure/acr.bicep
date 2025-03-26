@minLength(5)
param acrName string

resource acrResource 'Microsoft.ContainerRegistry/registries@2024-11-01-preview' = {
  name: acrName
  location: resourceGroup().location
  sku: {
    name: 'Basic'
  }
  properties: {
    adminUserEnabled: true
  }
}
