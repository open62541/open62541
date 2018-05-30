open62541 Supported Features
============================

| __**Service**__             |                                 |                      | Comment              |
|:----------------------------|:--------------------------------|:--------------------:|:---------------------|
| Discovery Service Set       |                                 |                      |                      |
|                             | FindServers()                   |  :heavy_check_mark:  |                      |
|                             | FindServersOnNetwork()          |     :full_moon:      | master, Release 0.3  |
|                             | GetEndpoints()                  |  :heavy_check_mark:  |                      |
|                             | RegisterServer()                |  :heavy_check_mark:  |                      |
|                             | RegisterServer2()               |     :full_moon:      | master, Release 0.3  |
| Secure Channel Service Set  |                                 |                      |                      |
|                             | OpenSecureChannel()             |  :heavy_check_mark:  |                      |
|                             | CloseSecureChannel()            |  :heavy_check_mark:  |                      |
| Session Service Set         |                                 |                      |                      |
|                             | CreateSession()                 |  :heavy_check_mark:  |                      |
|                             | CloseSession()                  |  :heavy_check_mark:  |                      |
|                             | ActivateSession()               |  :heavy_check_mark:  |                      |
|                             | Cancel()                        |      :new_moon:      |                      |
| Node Management Service Set |                                 |                      |                      |
|                             | AddNodes()                      |  :heavy_check_mark:  |                      |
|                             | AddReferences()                 |  :heavy_check_mark:  |                      |
|                             | DeleteNodes()                   |  :heavy_check_mark:  |                      |
|                             | DeleteReferences()              |  :heavy_check_mark:  |                      |
| View Service Set            |                                 |                      |                      |
|                             | Browse()                        |  :heavy_check_mark:  |                      |
|                             | BrowseNext()                    |  :heavy_check_mark:  |                      |
|                             | TranslateBrowsePathsToNodeIds() |  :heavy_check_mark:  |                      |
|                             | RegisterNodes()                 |  :heavy_check_mark:  |                      |
|                             | UnregisterNodes()               |  :heavy_check_mark:  |                      |
| Query Service Set           |                                 |                      |                      |
|                             | QueryFirst()                    |      :new_moon:      |                      |
|                             | QueryNext()                     |      :new_moon:      |                      |
| Attribute Service Set       |                                 |                      |                      |
|                             | Read()                          |  :heavy_check_mark:  |                      |
|                             | Write()                         |  :heavy_check_mark:  |                      |
|                             | HistoryRead()                   | :waning_gibbous_moon: | [WIP](https://github.com/open62541/open62541/pull/1740), Release 0.4     |
|                             | HistoryUpdate()                 | :waning_gibbous_moon: | [WIP](https://github.com/open62541/open62541/pull/1740), Release 0.4     |
| Method Service Set          |                                 |                      |                      |
|                             | Call()                          |  :heavy_check_mark:  |                      |
| MonitoredItems Service Set  |                                 |                      |                      |
|                             | CreateMonitoredItems()          |  :heavy_check_mark:  |                      |
|                             | DeleteMonitoredItems()          |  :heavy_check_mark:  |                      |
|                             | ModifyMonitoredItems()          |  :heavy_check_mark:  |                      |
|                             | SetMonitoringMode()             |  :heavy_check_mark:  |                      |
|                             | SetTriggering()                 |      :new_moon:      |                      |
| Subscription Service Set    |                                 |                      |                      |
|                             | CreateSubscription()            |  :heavy_check_mark:  |                      |
|                             | ModifySubscription()            |  :heavy_check_mark:  |                      |
|                             | SetPublishingMode()             |  :heavy_check_mark:  |                      |
|                             | Publish()                       |  :heavy_check_mark:  |                      |
|                             | Republish()                     |  :heavy_check_mark:  |                      |
|                             | DeleteSubscriptions()           |  :heavy_check_mark:  |                      |
|                             | TransferSubscriptions()         |      :new_moon:      |                      |


|                                         |                      |                      |
|:----------------------------------------|:--------------------:|:---------------------|
| **Transport**                           |                      |                      |
| UA-TCP UA-SC UA Binary                  |  :heavy_check_mark:  | OPC.TCP - Binary     |
| SOAP-HTTP WS-SC UA Binary               |      :new_moon:      | HTTP/HTTPS - Binary  |
| SOAP-HTTP WS-SC UA XML                  |      :new_moon:      |                      |
| SOAP-HTTP WS-SC UA XML-UA Binary        |      :new_moon:      |                      |
| **Encryption**                          |                      |                      |
| None                                    |  :heavy_check_mark:  |                      |
| Basic128Rsa15                           |  :heavy_check_mark:  | master, Release 0.3  |
| Basic256                                |  :heavy_check_mark:  | master               |
| Basic256Sha256                          |  :heavy_check_mark:  | master               |
| **Authentication**                      |                      |                      |
| Anonymous                               |  :heavy_check_mark:  |                      |
| User Name Password                      |  :heavy_check_mark:  |                      |
| X509 Certificate                        |      :new_moon:      |                      |
| **Server Facets**                       |                      |                      |
| Core Server                             |  :heavy_check_mark:  |                      |
| Data Access Server                      |  :heavy_check_mark:  |                      |
| Embedded Server                         |  :heavy_check_mark:  |                      |
| Nano Embedded Device Server             |  :heavy_check_mark:  |                      |
| Micro Embedded Device Server            |  :heavy_check_mark:  |                      |
| Method Server                           |  :heavy_check_mark:  |                      |
| Embedded DataChange Subscription Server |  :heavy_check_mark:  |                      |
| Node Management Server                  |  :heavy_check_mark:  |                      |
| Standard DataChange Subscription Server | :waning_gibbous_moon: | Only Deadband Filter missing |
| Event Subscription Server               |     :full_moon:      | master               |
| **Client Facets**                       |                      |                      |
| Base Client Behaviour                   |  :heavy_check_mark:  |                      |
| AddressSpace Lookup                     |  :heavy_check_mark:  |                      |
| Attribute Read                          |  :heavy_check_mark:  |                      |
| DataChange Subscription                 |  :heavy_check_mark:  |                      |
| DataAccess                              |  :heavy_check_mark:  |                      |
| Discovery                               |  :heavy_check_mark:  |                      |
| Event Subscription                      |  :heavy_check_mark:  |                      |
| Method call                             |  :heavy_check_mark:  |                      |
| Advanced Type                           |  :heavy_check_mark:  |                      |
| **Discovery**                           |                      | See Discovery Service Set |
| Local Disovery Server                   |  :heavy_check_mark:  | master, Release 0.3  |
| Local Discovery Server Multicast Ext.   |  :heavy_check_mark:  | master, Release 0.3  |
| Global Discovery Server                 |      :new_moon:      |                      |
