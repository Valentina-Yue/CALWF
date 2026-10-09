# ============================================================
# src/CMakeLists.txt
# ============================================================

add_library(calwf_core STATIC
    core/calwf_version.cpp
    utils/StringUtils.cpp
    utils/FileUtils.cpp
    utils/Logger.cpp
    io/CSVParser.cpp
    io/BaweMetadata.cpp
    io/BaweImporter.cpp
    io/SwecclMeta.cpp
    io/SwecclImporter.cpp
    io/LlmImporter.cpp
    text/SentenceSplitter.cpp
    features/FeatureVector.cpp
    features/SurfaceFeatures.cpp
    features/LexicalFeatures.cpp
    features/SyntacticFeatures.cpp
    features/DiscourseFeatures.cpp
    features/CorpusAnchor.cpp
    features/AnchoringFeatures.cpp
    features/SafetyFeatures.cpp
    model/ModelProfile.cpp
    analysis/Analyzer.cpp
    report/ReportGenerator.cpp
    cli/Menu.cpp
)

target_include_directories(calwf_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

if(NOT WIN32)
    target_link_libraries(calwf_core PUBLIC m stdc++fs)
endif()

add_executable(calwf_cli
    main.cpp
)

target_link_libraries(calwf_cli PRIVATE calwf_core)