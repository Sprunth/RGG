#ifndef macro_helpers_h
#define macro_helpers_h

#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)

#ifndef RGG_VERSION
#  error The version need to be defined.
#endif

#ifndef RGG_BUILD_ID
#  error Build ID needs to be defined.
#endif

#define RGG_VERSION_STR STR(RGG_VERSION)
#define RGG_BUILD_ID_STR STR(RGG_BUILD_ID)

#endif
