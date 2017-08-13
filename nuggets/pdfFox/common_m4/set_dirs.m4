## Expand ${prefix} in sysconfdir if its the default value.

AS_AC_EXPAND(SYSCONFDIR, "$sysconfdir/razorback")
sysconfdir=$SYSCONFDIR

AS_AC_EXPAND(LIBEXECDIR, "$libexecdir/razorback/$PACKAGE_NAME")
libexecdir=$LIBEXECDIR

AC_DEFINE_UNQUOTED(ETC_DIR, "$SYSCONFDIR", ["Location of config files"])

AC_DEFINE_UNQUOTED(DEFAULT_CONFIG_FILE, "$SYSCONFDIR/rzb.conf", ["Location of config files"])
