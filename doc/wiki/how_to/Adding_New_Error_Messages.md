# Adding New Error Messages

The error messages are described using a shorthand name that begins with
two asterisks. This name is used as an argument to the routine that
creates MPI error codes, `MPIR_Err_create_code`. These shorthands refer
to a longer message that is stored in the file
`src/mpi/errhan/errnames.txt`. To add a new message,

1.  Check that you really need a new message. Look in `errnames.txt` and
    see if you can use one of the existing messages
2.  If not, add your message to errnames.txt. One of these should be
    your shorthand name with no printf-style format descriptors (e.g.,
    something like "\*\*nomem"). The format of the line is
    `shortname:long text` i.e., `**nomem:Out of memory`.
3.  You can also provide a variation on your message that accepts
    arguments (see the documentation on `MPIR_Err_create_code`). This
    must have the same initial name, but may include one or more format
    descriptors. For example, `"**nomem %s"`. Your message for this form
    must also include the same format specifiers; when this message is
    used, the corresponding argument from `MPIR_Err_create_code` is used
    to provide that value in the final string. For example, the entry in
    `errnames.txt` would be:

```
**nomem %s:Out of memory (unable to allocate a '%s')
```

Multiline messages may be provided by placing a backslash (\\) at the
end of any line that is continued.

Once you have added the message text, you must rebuild the header file
that contains the messages (this file is `src/mpi/errhan/defmsg.h`).

## Rebuilding the table of messages

Normally, you should use the `maint/updatefiles` script to update the
table of error messages. However, this step can be run independently.
One reason to do this is to perform more detailed checks on the error
message usage to ensure that messages meet the coding requirements.
Assuing the sh shell,

```
./maint/extracterrmsgs -strict `cat maint/errmsgdirs` > newmsg.h
```

will output suspected problems with the error messages.

## Internationalized Error Messages

To provide for internationalized error messages, the first step is to
have the file errnames.txt translated.

To do: describe the configure options to support the
internationalization options, including getmsg.

## Error Messages Intended for Developers Only

Messages intended only for the maintainers and developers of MPICH can
use a different mechanism because they do not need to be
internationalized and do not need to be encoded within an MPI error
code. For such messages, use `MPIU_Internal_error_printf`, which has the
same calling sequence as `printf`. To simplify reporting internal errors
returned by system calls, you can use `MPIU_Internal_sys_error_printf`

```
errnum = select( ... );
MPIU_Internal_sys_error_printf( "select", errnum, "printf message" );
```
