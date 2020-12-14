*******************
README for comproot
*******************

a fakeroot replacement using ``seccomp`` notifications

:Authors:
  **Max Rees**, maintainer
:Status:
  Development
:Releases and source code:
  `Foxkit Code Syndicate <https://code.foxkit.us/sroracle/comproot>`_
:Copyright:
  © 2020 Max Rees. NCSA open source license.

Dependencies
------------

* `libseccomp <https://github.com/seccomp/libseccomp>`_
* Kernels with ``seccomp`` API >= 5 (``SECCOMP_RET_USER_NOTIF``;
  introduced in 5.0-rc1)
