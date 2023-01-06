# Fetch Dragonbox floating-point formatting library

include(FetchContent)
FetchContent_Declare(
        dragonbox
        GIT_REPOSITORY https://github.com/jk-jeon/dragonbox
        GIT_TAG tags/1.1.3
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(dragonbox)
