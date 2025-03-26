@minLength(4)
param prefix string
param acrName string
param imageName string

// var loadBalancerName = '${prefix}-lb'

resource acrResource 'Microsoft.ContainerRegistry/registries@2024-11-01-preview' existing = {
  name: acrName
  scope: resourceGroup()
}

// resource vnet 'Microsoft.Network/virtualNetworks@2024-05-01' = {
//   name: '${prefix}-vnet'
//   location: resourceGroup().location
//   properties: {
//     addressSpace: {
//       addressPrefixes: [
//         '10.0.0.0/16'
//       ]
//     }
//     subnets: [
//       {
//         name: '${prefix}-subnet'
//         properties: {
//           networkSecurityGroup: {
//             id: networkSecurityGroup.id
//           }
//           addressPrefix: '10.0.0.0/24'
//           delegations: [
//             {
//               name: 'acidelegation'
//               properties: {
//                 serviceName: 'Microsoft.ContainerInstance/containerGroups'
//               }
//             }
//           ]
//         }
//       }
//     ]
//   }
// }

// resource publicIp 'Microsoft.Network/publicIPAddresses@2024-05-01' = {
//   name: '${prefix}-public-ip'
//   location: resourceGroup().location
//   properties: {
//     publicIPAllocationMethod: 'Dynamic'
//   }
// }

// resource networkSecurityGroup 'Microsoft.Network/networkSecurityGroups@2024-05-01' = {
//   name: '${prefix}-nsg'
//   location: resourceGroup().location
//   properties: {
//     securityRules: [
//       {
//         name: 'AllowInboundFromLocal'
//         properties: {
//           protocol: '*'
//           sourcePortRange: '*'
//           destinationPortRange: '*'
//           sourceAddressPrefix: '24.170.235.62'
//           destinationAddressPrefix: '*'
//           access: 'Allow'
//           priority: 200
//           direction: 'Inbound'
//         }
//       }
//       {
//         name: 'AllowAzurePortalInbound'
//         properties: {
//           protocol: '*'
//           sourcePortRange: '*'
//           destinationPortRange: '*'
//           sourceAddressPrefix: 'AzurePortal'
//           destinationAddressPrefix: '*'
//           access: 'Allow'
//           priority: 100
//           direction: 'Inbound'
//         }
//       }
//       {
//         name: 'AllowAzureCloudInbound'
//         properties: {
//           protocol: '*'
//           sourcePortRange: '*'
//           destinationPortRange: '*'
//           sourceAddressPrefix: 'AzureCloud'
//           destinationAddressPrefix: '*'
//           access: 'Allow'
//           priority: 101
//           direction: 'Inbound'
//         }
//       }
//       {
//         name: 'AllowUDPInbound'
//         properties: {
//           protocol: 'Udp'
//           sourcePortRange: '*'
//           destinationPortRange: '29070'
//           sourceAddressPrefix: '*'
//           destinationAddressPrefix: '*'
//           access: 'Allow'
//           priority: 102
//           direction: 'Inbound'
//         }
//       }
//       {
//         name: 'AllowUDPOutbound'
//         properties: {
//           protocol: 'Udp'
//           sourcePortRange: '*'
//           destinationPortRange: '29070'
//           sourceAddressPrefix: '*'
//           destinationAddressPrefix: '*'
//           access: 'Allow'
//           priority: 103
//           direction: 'Outbound'
//         }
//       }
//     ]
//   }
// }

resource containerInstanceResource 'Microsoft.ContainerInstance/containerGroups@2024-10-01-preview' = {
  name: '${prefix}-app'
  location: resourceGroup().location
  identity: {
    type: 'SystemAssigned'
  }
  properties: {
    restartPolicy: 'OnFailure'
    osType: 'Linux'
    // subnetIds: [
    //   {
    //     name: vnet.properties.subnets[0].name
    //     id: vnet.properties.subnets[0].id
    //   }
    // ]
    containers: [
      {
        name: '${prefix}-container-1'
        properties: {
          image: '${acrResource.properties.loginServer}/${imageName}'
          ports: [
            {
              port: 29070
              protocol: 'udp'
            }
          ]
          resources: {
            limits: {
              cpu: 1
              memoryInGB: 1
            }
            requests: {
              cpu: 1
              memoryInGB: 1
            }
          }
        }
      }
    ]
    imageRegistryCredentials: [
      {
        server: acrResource.properties.loginServer
        username: acrResource.listCredentials().username
        password: acrResource.listCredentials().passwords[0].value
      }
    ]
    ipAddress: {
      type: 'public'
      ports: [
        {
          port: 29070
          protocol: 'UDP'
        }
      ]
    }
  }
}
