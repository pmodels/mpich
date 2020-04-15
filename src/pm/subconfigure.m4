[#] start of __file__

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
])

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[

dnl we handle the AM_CONDITIONAL for hydra to keep hydra buildable as a
dnl standalone package
for pm_name in $pm_names ; do
    AS_CASE([$pm_name],
            [hydra],[build_pm_hydra=yes],
            [hydra2],[build_pm_hydra2=yes])
done

# we handle these conditionals here in the BODY because they depend on logic in
# the main portion of the top-level configure
AM_CONDITIONAL([BUILD_PM_HYDRA],[test "x$build_pm_hydra" = "xyes"])
AM_CONDITIONAL([BUILD_PM_HYDRA2],[test "x$build_pm_hydra2" = "xyes"])

dnl AC_MSG_NOTICE([RUNNING CONFIGURE FOR PROCESS MANAGERS])
# do nothing extra here for now

])dnl end _BODY

m4_include([src/pm/remshell/subconfigure.m4])
m4_include([src/pm/gforker/subconfigure.m4])
m4_include([src/pm/util/subconfigure.m4])

m4_define([PAC_SRC_PM_SUBCFG_MODULE_LIST],
[src_pm,
 src_pm_remshell,
 src_pm_gforker,
 src_pm_util])

[#] end of __file__
