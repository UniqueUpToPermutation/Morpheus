#ifndef _TONE_MAPPING_STRUCTURES_FXH_
#define _TONE_MAPPING_STRUCTURES_FXH_

#ifdef __cplusplus

#   ifndef BOOL
#      define BOOL int32_t // Do not use bool, because sizeof(bool)==1 !
#   endif

#   ifndef TRUE
#      define TRUE 1
#   endif

#   ifndef CHECK_STRUCT_ALIGNMENT
        // Note that defining empty macros causes GL shader compilation error on Mac, because
        // it does not allow standalone semicolons outside of main.
        // On the other hand, adding semicolon at the end of the macro definition causes gcc error.
#       define CHECK_STRUCT_ALIGNMENT(s) static_assert( sizeof(s) % 16 == 0, "sizeof(" #s ") is not multiple of 16" )
#   endif

#   ifndef DEFAULT_VALUE
#       define DEFAULT_VALUE(x) =x
#   endif

#else

#   ifndef BOOL
#       define BOOL bool
#   endif

#   ifndef DEFAULT_VALUE
#       define DEFAULT_VALUE(x)
#   endif

#endif


// Tone mapping mode
#define TONE_MAPPING_MODE_REINHARD 0

struct ToneMappingAttribs
{
	float mExposure		DEFAULT_VALUE(1.0f);
	float mPureWhite	DEFAULT_VALUE(1.0f);
	float mPadding0;
	float mPadding1;
};
#ifdef CHECK_STRUCT_ALIGNMENT
    CHECK_STRUCT_ALIGNMENT(ToneMappingAttribs);
#endif

#endif // _TONE_MAPPING_STRUCTURES_FXH_
