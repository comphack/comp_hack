.Func<@VAR_TYPE@ (@OBJECT_NAME@::*)(size_t)>(
    "Get@VAR_CAMELCASE_NAME@ByIndex", &@OBJECT_NAME@::Get@VAR_CAMELCASE_NAME@)
.Func<bool (@OBJECT_NAME@::*)(size_t, @VAR_ARG_TYPE@)>(
    "Set@VAR_CAMELCASE_NAME@ByIndex", &@OBJECT_NAME@::Set@VAR_CAMELCASE_NAME@)
.Func<size_t (@OBJECT_NAME@::*)() const>(
    "@VAR_CAMELCASE_NAME@Count", &@OBJECT_NAME@::@VAR_CAMELCASE_NAME@Count)