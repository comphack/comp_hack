.Func<@VAR_TYPE@ (@OBJECT_NAME@::*)(size_t) const>(
    "Get@VAR_CAMELCASE_NAME@", &@OBJECT_NAME@::Get@VAR_CAMELCASE_NAME@)
.Func<bool (@OBJECT_NAME@::*)(size_t, @VAR_ARG_TYPE@)>(
    "Set@VAR_CAMELCASE_NAME@", &@OBJECT_NAME@::Set@VAR_CAMELCASE_NAME@)
.Func<size_t (@OBJECT_NAME@::*)() const>(
    "@VAR_CAMELCASE_NAME@Count", &@OBJECT_NAME@::@VAR_CAMELCASE_NAME@Count)