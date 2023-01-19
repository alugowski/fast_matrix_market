# Fetch from git
include(FetchContent)

FetchContent_Declare(
        blaze
        GIT_REPOSITORY https://bitbucket.org/blaze-lib/blaze.git
        GIT_TAG v3.8.1
        GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(blaze)
