.. _security-rbac:

Role-Based Access Control
=========================

:doc:`authentication` establishes *who* uses a Session.  Role-Based Access
Control (RBAC, OPC UA Part 18 and Part 3 §4.9) decides *what* the Session may
do.  At ``ActivateSession`` the server grants the Session a set of *Roles*.
Nodes carry *RolePermissions*, a list of Role/PermissionType pairs.  The
effective permissions of a Session on a Node are the union of the permissions
of all its Roles.

RBAC is experimental and off by default.  Enable it with
``-DUA_ENABLE_RBAC=ON``.  For its RoleSet information model it currently
needs ``UA_NAMESPACE_ZERO=FULL`` and ``UA_ENABLE_METHODCALLS``.  The API is
documented in :ref:`server-rbac`.

Permissions
-----------

A Node's own RolePermissions apply first: ``UA_Server_setNodeRolePermissions``
replaces the list, ``UA_Server_addRolePermissions`` /
``UA_Server_removeRolePermissions`` change one Role's entry (all optionally
recursive), ``UA_Server_removeNodeRolePermissions`` drops the list, as does
removing its last entry.  ``rolePermissionPresets`` preloads lists that Nodes
share.  Without a list the namespace default of
``UA_Server_setNamespaceDefaultRolePermissions`` applies, without that
``allPermissionsForAnonymous``.  A list must name every Role that needs
access.  Clients cannot write the RolePermissions attribute.  The local admin
Session of the C API bypasses all checks.

The services and the default AccessControl plugin check the PermissionType
bits (``UA_PERMISSIONTYPE_*``) as follows.  The derived UserAccessLevel,
UserWriteMask and UserExecutable never exceed the Node's own attributes.

.. list-table::
   :header-rows: 1
   :widths: 34 66

   * - Permission
     - Gates
   * - ``BROWSE``
     - Browse (also of the reference targets), TranslateBrowsePathsToNodeIds;
       reading attributes other than Value and RolePermissions.
   * - ``READROLEPERMISSIONS``, ``WRITEATTRIBUTE``,
       ``WRITEROLEPERMISSIONS``, ``WRITEHISTORIZING``
     - Reading RolePermissions; UserWriteMask: other attributes,
       RolePermissions, Historizing.
   * - ``READ``, ``WRITE``, ``READHISTORY``, ``INSERTHISTORY``,
       ``MODIFYHISTORY``, ``DELETEHISTORY``
     - UserAccessLevel CurrentRead, CurrentWrite, HistoryRead; HistoryUpdate
       insert, replace/update, delete.
   * - ``RECEIVEEVENTS``
     - Event delivery; needed on the EventType and on the SourceNode.
   * - ``CALL``
     - Call, needed on the Object and on the Method; UserExecutable of the
       Method.
   * - ``ADDREFERENCE``, ``REMOVEREFERENCE``, ``DELETENODE``
     - AddReferences / DeleteReferences on the source Node (AddNodes also on
       the parent), DeleteNodes on every deleted Node and child.  The
       references to deleted Nodes are removed without ``REMOVEREFERENCE``.
   * - ``ADDNODE``
     - AddNodes, checked on the parent Node.

MonitoredItems are sampled with the rights of the subscribing Session, also
when a write of another Session triggers the sample.

Default rights
--------------

The default configurations (``UA_ServerConfig_setDefault()`` and friends) set
``allPermissionsForAnonymous = true`` for backwards compatibility: a Node with
neither own RolePermissions nor a namespace default grants *all* permissions
to every Session, whatever its Roles.  Explicit lists are enforced either
way.  No namespace has a default after startup.  To secure a server:

- set ``allPermissionsForAnonymous = false`` before startup (every check
  reads it),
- give every namespace, including Namespace Zero, a default,
- map users to Roles, including a SecurityAdmin for remote administration.

Auditing
--------

With ``UA_ENABLE_AUDITING`` and ``UA_ServerConfig::auditingEnabled``, the
AuditActivateSessionEvent omits the password and the issued token data.  The
AuditUpdateMethodEvents of UpdateCertificate, UpdateCredential, AddUser,
ModifyUser, ChangePassword, GetSecurityKeys and SetSecurityKeys omit their
secret arguments.  The redaction is keyed on the Namespace Zero NodeIds of
these Methods and of their ObjectType declarations.  Method copies made with
``copyMethodsOnInstances`` have other NodeIds and are not covered.
