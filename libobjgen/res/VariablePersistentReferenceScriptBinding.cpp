Sqrat::Class<libcomp::ObjectReference<@OBJECT_NAME@>> refBinding(mVM, "@OBJECT_NAME@Ref");
refBinding
    .Func<const std::shared_ptr<@OBJECT_NAME@>
        (libcomp::ObjectReference<@OBJECT_NAME@>::*)()>(
        "Get", &libcomp::ObjectReference<@OBJECT_NAME@>::Get);

Bind<libcomp::ObjectReference<@OBJECT_NAME@>>("@OBJECT_NAME@Ref", refBinding);