[#] start of __file__

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
])

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[

dnl we handle the AM_CONDITIONAL for hydra to keep hydra buildable as a
dnl standalone package
for pm_name in $pm_names ; do
    AS_CASE([$pm_name],
            [hydra],[build_pm_hydra=yes])
done

# we handle these conditionals here in the BODY because they depend on logic in
# the main portion of the top-level configure
AM_CONDITIONAL([BUILD_PM_HYDRA],[test "x$build_pm_hydra" = "xyes"])

dnl AC_MSG_NOTICE([RUNNING CONFIGURE FOR PROCESS MANAGERS])
# do nothing extra here for now

])dnl end _BODY

[#] end of __file__
