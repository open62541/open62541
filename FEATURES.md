open62541 Supported Features
============================

| __**Service**__             |                                 |                      | Comment              |
|:----------------------------|:--------------------------------|:--------------------:|:---------------------|
| Discovery Service Set       |                                 |                      |                      |
|                             | FindServers()                   |  :white_check_mark:  |                      |
|                             | FindServersOnNetwork()          | :first_quarter_moon: | Branch: feature_mdns |
|                             | GetEndpoints()                  |  :white_check_mark:  |                      |
|                             | RegisterServer()                |  :white_check_mark:  |                      |
|                             | RegisterServer2()               |     :full_moon:      | Branch: master       |
| Secure Channel Service Set  |                                 |                      |                      |
|                             | OpenSecureChannel()             |  :white_check_mark:  |                      |
|                             | CloseSecureChannel()            |  :white_check_mark:  |                      |
| Session Service Set         |                                 |                      |                      |
|                             | CreateSession()                 |  :white_check_mark:  |                      |
|                             | CloseSession()                  |  :white_check_mark:  |                      |
|                             | ActivateSession()               |  :white_check_mark:  |                      |
|                             | Cancel()                        |      :new_moon:      |                      |
| Node Management Service Set |                                 |                      |                      |
|                             | AddNodes()                      |  :white_check_mark:  |                      |
|                             | AddReferences()                 |  :white_check_mark:  |                      |
|                             | DeleteNodes()                   |  :white_check_mark:  |                      |
|                             | DeleteReferences()              |  :white_check_mark:  |                      |
| View Service Set            |                                 |                      |                      |
|                             | Browse()                        |  :white_check_mark:  |                      |
|                             | BrowseNext()                    |  :white_check_mark:  |                      |
|                             | TranslateBrowsePathsToNodeIds() |  :white_check_mark:  |                      |
|                             | RegisterNodes()                 |  :white_check_mark:  |                      |
|                             | UnregisterNodes()               |  :white_check_mark:  |                      |
| Query Service Set           |                                 |                      |                      |
|                             | QueryFirst()                    |      :new_moon:      |                      |
|                             | QueryNext()                     |      :new_moon:      |                      |
| Attribute Service Set       |                                 |                      |                      |
|                             | Read()                          |  :white_check_mark:  |                      |
|                             | Write()                         |  :white_check_mark:  |                      |
|                             | HistoryRead()                   |      :new_moon:      |                      |
|                             | HistoryUpdate()                 |      :new_moon:      |                      |
| Method Service Set          |                                 |                      |                      |
|                             | Call()                          |  :white_check_mark:  |                      |
| MonitoredItems Service Set  |                                 |                      |                      |
|                             | CreateMonitoredItems()          |  :white_check_mark:  |                      |
|                             | DeleteMonitoredItems()          |  :white_check_mark:  |                      |
|                             | ModifyMonitoredItems()          |  :white_check_mark:  |                      |
|                             | SetMonitoringMode()             |  :white_check_mark:  |                      |
|                             | SetTriggering()                 |      :new_moon:      |                      |
| Subscription Service Set    |                                 |                      |                      |
|                             | CreateSubscription()            |  :white_check_mark:  |                      |
|                             | ModifySubscription()            |  :white_check_mark:  |                      |
|                             | SetPublishingMode()             |  :white_check_mark:  |                      |
|                             | Publish()                       |  :white_check_mark:  |                      |
|                             | Republish()                     |  :white_check_mark:  |                      |
|                             | DeleteSubscriptions()           |  :white_check_mark:  |                      |
|                             | TransferSubscriptions()         |      :new_moon:      |                      |


|                                         |                      |                      |
|:----------------------------------------|:--------------------:|:---------------------|
| **Transport**                           |                      |                      |
| UA-TCP UA-SC UA Binary                  |  :white_check_mark:  | OPC.TCP - Binary     |
| SOAP-HTTP WS-SC UA Binary               |      :new_moon:      | HTTP/HTTPS - Binary  |
| SOAP-HTTP WS-SC UA XML                  |      :new_moon:      |                      |
| SOAP-HTTP WS-SC UA XML-UA Binary        |      :new_moon:      |                      |
| **Encryption**                          |                      |                      |
| None                                    |  :white_check_mark:  |                      |
| Basic128Rsa15                           |      :new_moon:      |                      |
| Basic256                                |      :new_moon:      |                      |
| Basic256Sha256                          |      :new_moon:      |                      |
| **Authentication**                      |                      |                      |
| Anonymous                               |  :white_check_mark:  |                      |
| User Name Password                      |  :white_check_mark:  |                      |
| X509 Certificate                        |      :new_moon:      |                      |
| **Server Facets**                       |                      |                      |
| Core Server                             |  :white_check_mark:  |                      |
| Data Access Server                      |  :white_check_mark:  |                      |
| Embedded Server                         |  :white_check_mark:  |                      |
| Nano Embedded Device Server             |  :white_check_mark:  |                      |
| Micro Embedded Device Server            |  :white_check_mark:  |                      |
| Method Server                           |  :white_check_mark:  |                      |
| Embedded DataChange Subscription Server |  :white_check_mark:  |                      |
| Node Management Server                  |  :white_check_mark:  |                      |
| Standard DataChange Subscription Server | :waxing_gibbous_moon: | Only Deadband Filter missing |
| Event Subscription Server               |     :new_moon:       |                      |
| **Client Facets**                       |                      |                      |
| Base Client Behaviour                   |  :white_check_mark:  |                      |
| AddressSpace Lookup                     |  :white_check_mark:  |                      |
| Attribute Read                          |  :white_check_mark:  |                      |
| DataChange Subscription                 |  :white_check_mark:  |                      |
| DataAccess                              |  :white_check_mark:  |                      |
| Discovery                               |  :white_check_mark:  |                      |
| Event Subscription                      |  :white_check_mark:  |                      |
| Method call                             |  :white_check_mark:  |                      |
| Advanced Type                           |  :white_check_mark:  |                      |
| **Discovery**                           |                      | See Discovery Service Set |
| Local Disovery Server                   | :first_quarter_moon: | Branch: feature_mdns |
| Local Discovery Server Multicast Ext.   | :first_quarter_moon: | Branch: feature_mdns |
| Global Discovery Server                 |      :new_moon:      |                      |
