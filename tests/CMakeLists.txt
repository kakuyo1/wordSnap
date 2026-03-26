cmake_minimum_required(VERSION 3.21)

function(add_wordsnap_test test_name)
    add_executable(${test_name}
        ${ARGN}
    )

    target_include_directories(${test_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
    )

    target_link_libraries(${test_name} PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Test
        Qt6::Network
    )

    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

add_wordsnap_test(WordNormalizerTest
    WordNormalizerTest.cpp
    ../src/services/WordNormalizer.cpp
)

add_wordsnap_test(PhoneticExtractorTest
    PhoneticExtractorTest.cpp
    ../src/services/PhoneticExtractor.cpp
)

add_wordsnap_test(StarDictBackendTest
    StarDictBackendTest.cpp
    ../src/services/StarDictBackend.cpp
)

add_wordsnap_test(LookupCoordinatorTest
    LookupCoordinatorTest.cpp
    ../src/app/LookupCoordinator.cpp
    ../src/services/PhoneticExtractor.cpp
)

add_wordsnap_test(QueryHistoryServiceTest
    QueryHistoryServiceTest.cpp
    ../src/services/QueryHistoryService.cpp
)

add_wordsnap_test(AiAssistServiceTest
    AiAssistServiceTest.cpp
    ../src/services/AiAssistService.cpp
)
