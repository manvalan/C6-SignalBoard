/*
 * Native unit tests for RocRailParser
 * Run with: pio test -e native
 */

#include <unity.h>
#include "app/RocRailParser.h"

void setUp() {}
void tearDown() {}

// ===== parseSignalCommand: MAIN signal mapping =====

void test_parse_main_red() {
    String id;
    SignalAspect aspect;
    bool ok = RocRailParser::parseSignalCommand(
        String("<sg id=\"sg1\" state=\"red\"/>"), id, aspect,
        SignalType::MAIN);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_STRING("sg1", id.c_str());
    TEST_ASSERT_EQUAL((int)SignalAspect::ASPECT_RED, (int)aspect);
}

void test_parse_main_green() {
    String id;
    SignalAspect aspect;
    bool ok = RocRailParser::parseSignalCommand(
        String("<sg id=\"sg2\" state=\"green\"/>"), id, aspect,
        SignalType::MAIN);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_STRING("sg2", id.c_str());
    TEST_ASSERT_EQUAL((int)SignalAspect::ASPECT_GREEN, (int)aspect);
}

void test_parse_main_yellow() {
    String id;
    SignalAspect aspect;
    bool ok = RocRailParser::parseSignalCommand(
        String("<sg id=\"sg3\" state=\"yellow\"/>"), id, aspect,
        SignalType::MAIN);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL((int)SignalAspect::ASPECT_YELLOW, (int)aspect);
}

void test_parse_main_unknown_state_defaults_to_red() {
    String id;
    SignalAspect aspect;
    bool ok = RocRailParser::parseSignalCommand(
        String("<sg id=\"sg1\" state=\"blinking\"/>"), id, aspect,
        SignalType::MAIN);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL((int)SignalAspect::ASPECT_RED, (int)aspect);
}

// ===== parseSignalCommand: SHUNT signal mapping =====

void test_parse_shunt_red_maps_to_stop() {
    String id;
    SignalAspect aspect;
    bool ok = RocRailParser::parseSignalCommand(
        String("<sg id=\"sh1\" state=\"red\"/>"), id, aspect,
        SignalType::SHUNT);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL((int)SignalAspect::ASPECT_STOP, (int)aspect);
}

void test_parse_shunt_green_maps_to_go() {
    String id;
    SignalAspect aspect;
    bool ok = RocRailParser::parseSignalCommand(
        String("<sg id=\"sh1\" state=\"green\"/>"), id, aspect,
        SignalType::SHUNT);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL((int)SignalAspect::ASPECT_GO, (int)aspect);
}

void test_parse_shunt_yellow_maps_to_oblique() {
    String id;
    SignalAspect aspect;
    bool ok = RocRailParser::parseSignalCommand(
        String("<sg id=\"sh1\" state=\"yellow\"/>"), id, aspect,
        SignalType::SHUNT);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL((int)SignalAspect::ASPECT_OBLIQUE, (int)aspect);
}

// ===== parseSignalCommand: malformed input =====

void test_parse_rejects_non_sg_element() {
    String id;
    SignalAspect aspect;
    bool ok = RocRailParser::parseSignalCommand(
        String("<lc id=\"loco1\" V=\"50\"/>"), id, aspect, SignalType::MAIN);

    TEST_ASSERT_FALSE(ok);
}

void test_parse_rejects_missing_id() {
    String id;
    SignalAspect aspect;
    bool ok = RocRailParser::parseSignalCommand(
        String("<sg state=\"green\"/>"), id, aspect, SignalType::MAIN);

    TEST_ASSERT_FALSE(ok);
}

void test_parse_rejects_empty_id() {
    String id;
    SignalAspect aspect;
    bool ok = RocRailParser::parseSignalCommand(
        String("<sg id=\"\" state=\"green\"/>"), id, aspect,
        SignalType::MAIN);

    TEST_ASSERT_FALSE(ok);
}

void test_parse_rejects_missing_state() {
    String id;
    SignalAspect aspect;
    bool ok = RocRailParser::parseSignalCommand(
        String("<sg id=\"sg1\"/>"), id, aspect, SignalType::MAIN);

    TEST_ASSERT_FALSE(ok);
}

void test_parse_rejects_empty_message() {
    String id;
    SignalAspect aspect;
    bool ok = RocRailParser::parseSignalCommand(String(""), id, aspect,
                                                SignalType::MAIN);

    TEST_ASSERT_FALSE(ok);
}

// ===== extractAttribute =====

void test_extract_attribute_basic() {
    String value = RocRailParser::extractAttribute(
        String("<sg id=\"sg5\" state=\"red\"/>"), String("id"));
    TEST_ASSERT_EQUAL_STRING("sg5", value.c_str());
}

void test_extract_attribute_second_attribute() {
    String value = RocRailParser::extractAttribute(
        String("<sg id=\"sg5\" state=\"red\"/>"), String("state"));
    TEST_ASSERT_EQUAL_STRING("red", value.c_str());
}

void test_extract_attribute_missing_returns_empty() {
    String value = RocRailParser::extractAttribute(
        String("<sg id=\"sg5\"/>"), String("state"));
    TEST_ASSERT_EQUAL_STRING("", value.c_str());
}

void test_extract_attribute_unterminated_returns_empty() {
    String value = RocRailParser::extractAttribute(
        String("<sg id=\"sg5"), String("id"));
    TEST_ASSERT_EQUAL_STRING("", value.c_str());
}

// ===== Test runner =====

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    RUN_TEST(test_parse_main_red);
    RUN_TEST(test_parse_main_green);
    RUN_TEST(test_parse_main_yellow);
    RUN_TEST(test_parse_main_unknown_state_defaults_to_red);

    RUN_TEST(test_parse_shunt_red_maps_to_stop);
    RUN_TEST(test_parse_shunt_green_maps_to_go);
    RUN_TEST(test_parse_shunt_yellow_maps_to_oblique);

    RUN_TEST(test_parse_rejects_non_sg_element);
    RUN_TEST(test_parse_rejects_missing_id);
    RUN_TEST(test_parse_rejects_empty_id);
    RUN_TEST(test_parse_rejects_missing_state);
    RUN_TEST(test_parse_rejects_empty_message);

    RUN_TEST(test_extract_attribute_basic);
    RUN_TEST(test_extract_attribute_second_attribute);
    RUN_TEST(test_extract_attribute_missing_returns_empty);
    RUN_TEST(test_extract_attribute_unterminated_returns_empty);

    return UNITY_END();
}
