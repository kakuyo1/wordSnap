cmake_minimum_required(VERSION 3.21)

function(add_wordsnap_test test_name test_label)
    add_executable(${test_name}
        ${ARGN}
    )

    target_include_directories(${test_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
    )

    target_link_libraries(${test_name} PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Test
        Qt6::Network
    )

    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES LABELS "${test_label}")
endfunction()

add_wordsnap_test(WordNormalizerTest unit
    WordNormalizerTest.cpp
    ../src/services/WordNormalizer.cpp
)

add_wordsnap_test(PhoneticExtractorTest unit
    PhoneticExtractorTest.cpp
    ../src/services/PhoneticExtractor.cpp
)

add_wordsnap_test(StarDictBackendTest unit
    StarDictBackendTest.cpp
    ../src/services/StarDictBackend.cpp
)

add_wordsnap_test(LookupCoordinatorTest integration
    LookupCoordinatorTest.cpp
    ../src/app/LookupCoordinator.cpp
    ../src/services/PhoneticExtractor.cpp
)

add_wordsnap_test(QueryHistoryServiceTest integration
    QueryHistoryServiceTest.cpp
    ../src/services/QueryHistoryService.cpp
)

add_wordsnap_test(AiAssistServiceTest unit
    AiAssistServiceTest.cpp
    ../src/services/AiAssistService.cpp
)

add_wordsnap_test(AiAssistPolicyTest unit
    AiAssistPolicyTest.cpp
    ../src/app/AiAssistPolicy.cpp
)

add_wordsnap_test(ResultCardWidgetTest ui
    ResultCardWidgetTest.cpp
    ../src/ui/ResultCardWidget.cpp
    ../src/ui/theme/ResultCardTheme.cpp
)
