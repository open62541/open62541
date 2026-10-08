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
``-DUA_ENABLE_RBAC=ON``; CMake requires ``UA_ENABLE_METHODCALLS`` and
``UA_NAMESPACE_ZERO=FULL``.  The API is documented in :ref:`server-rbac`.

Roles
-----

The server registers the well-known Roles of Part 18: Anonymous,
AuthenticatedUser, TrustedApplication, Observer, Operator, Engineer,
Supervisor, ConfigureAdmin, SecurityAdmin and the three SecurityKeyServer
Roles.  They cannot be removed and carry no permissions by themselves.  Custom
Roles come from ``UA_ServerConfig::roles`` (JSON ``rbac.roles``), from
``UA_Server_addRole`` and from the ``AddRole`` Method.  Configured Roles are
protected; Roles added at runtime can be removed with ``UA_Server_removeRole``
or ``RemoveRole``.  Every Role appears as a RoleType Object under
``Server/ServerCapabilities/RoleSet``.

Assigning Roles to a Session
----------------------------

Every Session gets Anonymous.  AuthenticatedUser is added for any
non-anonymous identity token, TrustedApplication for a validated client
certificate on a signed SecureChannel with a SecurityPolicy other than None.
These three mappings are fixed.  Any other Role is granted when one of its
identity mapping rules matches.  The rule types ``Anonymous``,
``AuthenticatedUser`` and ``TrustedApplication`` (empty criteria) match as
just described, the others when the criteria equals:

- ``UserName`` -- the user name of the UserNameIdentityToken,
- ``Thumbprint`` -- the SHA-1 thumbprint of the user certificate (40
  upper-case hex digits),
- ``X509Subject`` -- the subject or issuer of the user certificate, see
  ``UA_CertificateUtils_getRoleSubjectCriteria``,
- ``Application`` -- the ApplicationUri in the validated client certificate,
- ``GroupId`` -- a group returned by the AccessControl callback
  ``getUserGroups``,
- ``Role`` -- a Role claim of an IssuedIdentityToken, from
  ``getUserTokenRoles``.

A matching rule grants the Role only if the Session also passes the Role's
Application and Endpoint filters.  With ``applicationsExclude`` /
``endpointsExclude`` set (the ``UA_Role_init`` default) the list excludes; with
the flag cleared it includes, and an empty include list grants nothing.
Endpoints are compared with the configured ServerUrl of the listener that
accepted the SecureChannel.  A Role without rules is never granted
automatically.

- ``wellKnownRoleMappings`` in the config (or ``UA_Server_updateRole``) sets
  the rules and filters of the other well-known Roles.
- The Session attribute ``UA_QUALIFIEDNAME(0, "roles")`` (a NodeId array, set
  with ``UA_Server_setSessionAttribute``) pins the Roles of a Session, e.g. for
  Roles with ``customConfiguration``.  Deleting it returns to the rules.
- Changes of the RoleSet re-evaluate all active Sessions immediately.

Permissions
-----------

The services and the default AccessControl plugin check the PermissionType
bits (``UA_PERMISSIONTYPE_*``) as follows.  The derived UserAccessLevel,
UserWriteMask and UserExecutable never exceed the Node's own attributes.

.. list-table::
   :header-rows: 1
   :widths: 34 66

   * - Permission
     - Gates
   * - ``BROWSE``
     - Browse, TranslateBrowsePathsToNodeIds; reading attributes other than
       Value and RolePermissions.
   * - ``READROLEPERMISSIONS``
     - Reading the RolePermissions attribute.
   * - ``WRITEATTRIBUTE``, ``WRITEROLEPERMISSIONS``, ``WRITEHISTORIZING``
     - UserWriteMask: other attributes, RolePermissions, Historizing.
   * - ``READ``, ``WRITE``, ``READHISTORY``, ``INSERTHISTORY``,
       ``MODIFYHISTORY``, ``DELETEHISTORY``
     - UserAccessLevel CurrentRead, CurrentWrite, HistoryRead; HistoryUpdate
       insert, replace/update, delete.
   * - ``RECEIVEEVENTS``
     - Event delivery; needed on the EventType and on the SourceNode.
   * - ``CALL``
     - Call and UserExecutable; needed on the Object and on the Method.
   * - ``ADDREFERENCE``, ``REMOVEREFERENCE``, ``DELETENODE``, ``ADDNODE``
     - AddReferences / DeleteReferences on the source Node, DeleteNodes;
       AddNodes against the default of the new Node's namespace.

A Node's own RolePermissions apply first: ``UA_Server_setNodeRolePermissions``
replaces the list, ``UA_Server_addRolePermissions`` /
``UA_Server_removeRolePermissions`` change one Role's entry (all optionally
recursive), ``UA_Server_removeNodeRolePermissions`` drops the list, as does
removing its last entry.  ``rolePermissionPresets`` preloads lists that Nodes
share.  Without a list the namespace default of
``UA_Server_setNamespaceDefaultRolePermissions`` applies, without that
``allPermissionsForAnonymous``.  A Node's list must name every Role that needs
access; an empty list denies everything.  The local admin Session of the C
API bypasses all checks.

Default rights
--------------

The default configurations (``UA_ServerConfig_setDefault()`` and friends) set
``allPermissionsForAnonymous = true`` for backwards compatibility: a Node with
neither own RolePermissions nor a namespace default grants *all* permissions
to every Session, whatever its Roles.  With ``false`` it grants nothing.
Explicit lists are enforced either way, also one emptied by
``UA_Server_removeRole``.

No namespace has a default after startup.  The server only protects its own
RBAC Nodes with explicit RolePermissions:

.. list-table::
   :header-rows: 1
   :widths: 46 54

   * - Nodes
     - Default RolePermissions
   * - RoleSet, AddRole, RemoveRole, the RoleType Methods, every Role Object
       and its Methods
     - SecurityAdmin: Browse, Read, Call, ReadRolePermissions (Role Objects
       and their Methods also ReceiveEvents).  Their Properties: Browse,
       Read, and Write on the two ``*Exclude`` flags.
   * - ``RoleMappingRuleChangedAuditEventType``
     - SecurityAdmin: ReceiveEvents
   * - ``UserManagement``, ``Users``, ``AddUser``, ``ModifyUser``,
       ``RemoveUser`` (only with a UserManagement provider)
     - SecurityAdmin: Browse, Read, Call, ReadRolePermissions; Anonymous: Call
       on ``UserManagement`` and ``ChangePassword``
   * - All other Nodes
     - none -- all permissions for every Session

Except for the EventType, these Nodes and ``ChangePassword`` also require
encryption (see below).  The Methods that change Roles or users check for
SecurityAdmin over SignAndEncrypt themselves.  Only the three mandatory Roles
have identity mappings, so no network client holds SecurityAdmin (or Operator,
Engineer, ...) until one is configured.  To secure a server:

- set ``allPermissionsForAnonymous = false`` before creating the server,
- give every namespace, including Namespace Zero, a default,
- map users to Roles, including a SecurityAdmin for remote administration,
- require encryption with AccessRestrictions where needed.

AccessRestrictions
------------------

AccessRestrictions (Part 3 §5.2.11) tie a Node to the channel security:
``UA_ACCESSRESTRICTIONTYPE_SIGNINGREQUIRED`` and ``ENCRYPTIONREQUIRED``
(else ``Bad_SecurityModeInsufficient``), ``SESSIONREQUIRED`` (else
``Bad_UserAccessDenied``).  Read, Write, Call, the history and NodeManagement
services and Event delivery enforce them, Browse and
TranslateBrowsePathsToNodeIds only with ``APPLYRESTRICTIONSTOBROWSE``.  Set
them with ``UA_Server_setNodeAccessRestrictions`` or, for all Nodes without
own value, ``UA_Server_setNamespaceDefaultAccessRestrictions``.

Configuration
-------------

Map the user ``alice`` to Operator, let authenticated users browse and read
Namespace Zero and Operator also write one Variable (``config`` comes from
``UA_ServerConfig_setDefault()``; ``examples/access_control/server_rbac.c``
shows a complete server):

.. code-block:: c

   config.allPermissionsForAnonymous = false;
   UA_Role *op = (UA_Role*)UA_calloc(1, sizeof(UA_Role));
   UA_Role_init(op);
   op->roleName = UA_QUALIFIEDNAME_ALLOC(0, "Operator");
   op->identityMappingRules = UA_IdentityMappingRuleType_new();
   op->identityMappingRulesSize = 1;
   op->identityMappingRules[0].criteriaType = UA_IDENTITYCRITERIATYPE_USERNAME;
   op->identityMappingRules[0].criteria = UA_STRING_ALLOC("alice");
   config.wellKnownRoleMappings = op; /* freed with the config */
   config.wellKnownRoleMappingsSize = 1;
   UA_Server *server = UA_Server_newWithConfig(&config);

   UA_RolePermission rp[2] = {
       {UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER),
        UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ},
       {UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR),
        UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ |
        UA_PERMISSIONTYPE_WRITE}};
   UA_Server_setNamespaceDefaultRolePermissions(server, 0, 1, rp);
   UA_Server_setNodeRolePermissions(server, setpointId, 2, rp, false, NULL);

The ``rbac`` object of a JSON configuration uses the same field names:

.. code-block:: json

   { "rbac": {
       "allPermissionsForAnonymous": false,
       "wellKnownRoleMappings": [ { "roleName": "SecurityAdmin",
         "identityMappingRules": [
           { "criteriaType": "UserName", "criteria": "admin" } ] } ],
       "roles": [ { "roleName": "Maintenance",
         "identityMappingRules": [
           { "criteriaType": "GroupId", "criteria": "maint" } ] } ] } }

UserManagement
--------------

The UserManagement Object of Part 18 §5 (under ``ServerConfiguration``) is
published when the seven callbacks ``getUsers``, ``getPasswordPolicy``,
``getUserConfiguration``, ``addUser``, ``modifyUser``, ``removeUser`` and
``changePassword`` of ``UA_AccessControl`` are set.  The server enforces:

- ``Users``, ``AddUser``, ``ModifyUser``, ``RemoveUser`` -- SecurityAdmin over
  an encrypted channel.  Disabling or removing a user closes its Sessions.
- ``ChangePassword`` -- only the Session's own UserName identity, encrypted.
- A disabled user is refused at ActivateSession like an unknown user.
- A user with ``MustChangePassword`` gets ``Good_PasswordChangeRequired`` and
  only the Anonymous Role, which suffices to call ``ChangePassword``.

Auditing
--------

With ``UA_ENABLE_AUDITING`` and ``UA_ServerConfig::auditingEnabled``, every
change of a Role's rules or filters -- through the C API or the RoleSet
Methods -- emits a ``RoleMappingRuleChangedAuditEventType`` Event with the
Role Object as SourceNode, which only SecurityAdmin Sessions receive.  Audit
Events never contain passwords, access tokens, private keys or PubSub
security keys.
