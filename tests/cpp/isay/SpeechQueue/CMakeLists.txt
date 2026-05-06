set(ISAY_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/iSay)

add_moos_cpp_test(test_isay_speech_queue
  SOURCES
    UtteranceTest.cpp
    UtterQueueTest.cpp
    SayerTest.cpp
    VoiceUtilsTest.cpp
    ${ISAY_SRC}/Sayer.cpp
    ${ISAY_SRC}/Utterance.cpp
    ${ISAY_SRC}/UtterQueue.cpp
    ${ISAY_SRC}/VoiceUtils.cpp
  LIBS
    apputil
    genutil
    mbutil
    ${MOOS_LIBRARIES}
  INCLUDES
    ${ISAY_SRC}
  LABELS
    isay
    SpeechQueue
  SUITE_LABELS
    UtteranceTest=Utterance
    UtterQueueTest=UtterQueue
    SayerTest=Sayer
    VoiceUtilsTest=VoiceUtils
)
