open62541 Supported Features
============================

OPC UA Stack
------------

|                                         |                    |                      |
| --------------------------------------- |:------------------:| -------------------- |
| **Encoding**                            |                    |                      |
| OPC UA Binary                           | :heavy_check_mark: |                      |
| OPC UA JSON                             |     :new_moon:     | WIP for Release 0.4  |
| OPC UA XML                              |     :new_moon:     |                      |
| **Transport**                           |                    |                      |
| UA-TCP UA-SC UA Binary                  | :heavy_check_mark: |                      |
| OPC UA HTTPS                            |     :new_moon:     |                      |
| SOAP-HTTP WS-SC UA Binary               |     :new_moon:     |                      |
| SOAP-HTTP WS-SC UA XML                  |     :new_moon:     |                      |
| SOAP-HTTP WS-SC UA XML-UA Binary        |     :new_moon:     |                      |
| **Encryption**                          |                    |                      |
| None                                    | :heavy_check_mark: |                      |
| Basic128Rsa15                           | :heavy_check_mark: | master, Release 0.3  |
| Basic256                                | :heavy_check_mark: | master, Release 0.3  |
| Basic256Sha256                          | :heavy_check_mark: | master, Release 0.3  |
| **Authentication**                      |                    |                      |
| Anonymous                               | :heavy_check_mark: |                      |
| User Name Password                      | :heavy_check_mark: |                      |
| X509 Certificate                        |     :new_moon:     |                      |

OPC UA Server
-------------

| **Service-Set**             | **Service**                     | **Support**          | **Comment**          |
| --------------------------- | ------------------------------- |:--------------------:| -------------------- |
| Discovery Service Set       | FindServers()                   |  :heavy_check_mark:  |                      |
|                             | FindServersOnNetwork()          |     :full_moon:      | master, Release 0.3  |
|                             | GetEndpoints()                  |  :heavy_check_mark:  |                      |
|                             | RegisterServer()                |  :heavy_check_mark:  |                      |
|                             | RegisterServer2()               |     :full_moon:      | master, Release 0.3  |
| Secure Channel Service Set  | OpenSecureChannel()             |  :heavy_check_mark:  |                      |
|                             | CloseSecureChannel()            |  :heavy_check_mark:  |                      |
| Session Service Set         | CreateSession()                 |  :heavy_check_mark:  |                      |
|                             | CloseSession()                  |  :heavy_check_mark:  |                      |
|                             | ActivateSession()               |  :heavy_check_mark:  |                      |
|                             | Cancel()                        |      :new_moon:      |                      |
| Node Management Service Set | AddNodes()                      |  :heavy_check_mark:  |                      |
|                             | AddReferences()                 |  :heavy_check_mark:  |                      |
|                             | DeleteNodes()                   |  :heavy_check_mark:  |                      |
|                             | DeleteReferences()              |  :heavy_check_mark:  |                      |
| View Service Set            | Browse()                        |  :heavy_check_mark:  |                      |
|                             | BrowseNext()                    |  :heavy_check_mark:  |                      |
|                             | TranslateBrowsePathsToNodeIds() |  :heavy_check_mark:  |                      |
|                             | RegisterNodes()                 |  :heavy_check_mark:  |                      |
|                             | UnregisterNodes()               |  :heavy_check_mark:  |                      |
| Query Service Set           | QueryFirst()                    |      :new_moon:      |                      |
|                             | QueryNext()                     |      :new_moon:      |                      |
| Attribute Service Set       | Read()                          |  :heavy_check_mark:  |                      |
|                             | Write()                         |  :heavy_check_mark:  |                      |
|                             | HistoryRead()                   | :waning_gibbous_moon: | [WIP](https://github.com/open62541/open62541/pull/1740), Release 0.4 |
|                             | HistoryUpdate()                 | :waning_gibbous_moon: | [WIP](https://github.com/open62541/open62541/pull/1740), Release 0.4 |
| Method Service Set          | Call()                          |  :heavy_check_mark:  |                      |
| MonitoredItems Service Set  | CreateMonitoredItems()          |  :heavy_check_mark:  | See below for Events |
|                             | DeleteMonitoredItems()          |  :heavy_check_mark:  |                      |
|                             | ModifyMonitoredItems()          |  :heavy_check_mark:  |                      |
|                             | SetMonitoringMode()             |  :heavy_check_mark:  |                      |
|                             | SetTriggering()                 |      :new_moon:      |                      |
| Subscription Service Set    | CreateSubscription()            |  :heavy_check_mark:  |                      |
|                             | ModifySubscription()            |  :heavy_check_mark:  |                      |
|                             | SetPublishingMode()             |  :heavy_check_mark:  |                      |
|                             | Publish()                       |  :heavy_check_mark:  |                      |
|                             | Republish()                     |  :heavy_check_mark:  |                      |
|                             | DeleteSubscriptions()           |  :heavy_check_mark:  |                      |
|                             | TransferSubscriptions()         |      :new_moon:      |                      |

| **Subscriptions**                       |                    |                      |
| --------------------------------------- |:------------------:| -------------------- |
| DataChange MonitoredItems               | :heavy_check_mark: | master, Release 0.3  |
| DataChange Filters                      | :heavy_check_mark: | master               |
| Event MonitoredItems                    | :heavy_check_mark: | master               |
| Event Filters                           |     :new_moon:     |                      |

| **Discovery**                           |                    | See Discovery Service Set |
| --------------------------------------- |:------------------:| -------------------- |
| Local Discovery Server                  | :heavy_check_mark: | master, Release 0.3  |
| Local Discovery Server Multicast Ext.   | :heavy_check_mark: | master, Release 0.3  |
| Global Discovery Server                 |     :new_moon:     |                      |

OPC UA Client
-------------

- All services are supported
- Handling of subscriptions in the background

OPC UA PubSub
-------------

|                                                   |                       |                        |
| ------------------------------------------------- |:---------------------:| ---------------------- |
| **NetworkMessage decoding/encoding**              |                       |                        |
| Binary (UADP)                                     |   :heavy_check_mark:  |                        |
| JSON                                              |       :new_moon:      | WIP                    |
| **PubSub Transport**                              |                       |                        |
| UDP/multicast (send and receive)                  |   :heavy_check_mark:  |                        |
| Ethernet (TSN)                                    | :waning_gibbous_moon: | Defined API to plug in custom networking implementation |
| MQTT                                              |      :new_moon:       | WIP                    |
| AMQP                                              |      :new_moon:       |                        |
| **Publisher Configuration**                       |                       |                        |
| Configure (server-side) Publisher at runtime      |  :heavy_check_mark:   |                        |
| Configuration representation in information model |  :heavy_check_mark:   | Runtime configuration changes by editing the information model representation are possible |
| Security Key Service Model                        |      :new_moon:       |                        |
| **Subscriber Configuration**                      | :waning_gibbous_moon: | Manual Subscriber only |
