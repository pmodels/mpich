# Rules For Installation Targets

The general point of these rules is to ensure that the installation
process follows generally accepted practice, particularly for GNU tools.
Much of this happens "automatically" because of the use of configure and
simplemake, but special-purpose targets must follow these rules. In
addition, there are some goals that are more challenging and may involve
trade-offs between generality and convenience (see directory
independence below).

## `DESTDIR`

`DESTDIR` is used to allow a user to stage the installation into an
intermediate directory, using the command

```
make DESTDIR=dirname install`
```

Installation targets should contains `$(DESTDIR)` before any
installation directory name.

## `make -n`

Users should be able to use `make -n install` to see what commands will
be executed. This means that commands in the install target should not
suppress output (`@` in `make`) without providing an echo command to
display the command.

## Default `--prefix`

One suggestion is that the default installation prefix (i.e., the
directories used when configure is used with a `--prefix` option select
the build directory rather than `/usr/local`) or `NONE`, which happens
when some values of `$prefix` are used within configure without checking
that no other value has been set).

## `install.sh`

The `install` program is not standardized under Unix, and a number of
incompatible and partially broken versions exist. For this reason,
configure may choose to use a script, install.sh, to provide the install
function. Because we have had trouble with CVS occasionally losing the
execute bit, it is important to check that this file is executable
before using it. However, do not assume that you can always `chmod` it;
in a VPATH build, the source files may be owned by another user, causing
a `chmod` to fail. In that case, if the execute bit is missing and the
script is needed, an error message about the missing execute bit should
be generated. The best time to do this is during the configure step,
since this is when `install.sh` may be selected.

## Directory independence of installed files

Particularly for third-party installations, it is valuable to have an
installation that can be copied to a new location without changes and
still run properly (i.e., no directory paths within the executables or
libraries). There are some trade-offs here; we may want an option to
support this at the cost of requiring an environment variable be set to
the installed location (some commercial tools use this approach).

## Testing the install

The make target `installcheck` is the recommended way to perform tests
on an installation. To add code to this target when using `simplemake`,
add your code to the `Makefile.sm` under the target
`installcheck-local`.
