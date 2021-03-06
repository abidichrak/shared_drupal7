IMPORTANT NOTICE
================

Enforce revlog will only be enabled for a node when a revision is about to be created. Therefore it WON'T require users to enter a log message when:

- The node is being created (no previous version exists)

- The node is being previewed

- "Create new revision" is disabled (on the node form or set as a default through the content type configuration form)

Make sure that "Create new revision" is checked as the default behavior for the content types on which Enforce revlog is set.


USAGE
=====

- Enable the module "Enforce revlog" in admin/build/modules

- Go to admin/user/permissions to set the permission "skip revision log message" for relevant roles

- Go to admin/settings/enforce_revlog to configure the module and activate it on the content types where it is needed.

- Alternatively you can (de)activate Enforce revlog for a given content type on its content type editing form

- Now when a user without "skip revision log message" creates or reverts (if enabled) a node revision, he/she will have to enter a log message.

- If the "Create new revision" checkbox on the node editing form is unchecked while editing a node, Enforce Revlog will be disabled. If it's checked again, Enforce Revlog will be enabled again.
