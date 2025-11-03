// tests/unit/test_parser.cpp
#include <gtest/gtest.h>
#include <bpmn/parser.h>

class ParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Инициализация
    }
};

TEST_F(ParserTest, ParseSimpleProcess) {
    std::string bpmnXml = "<process id=\"test\"><startEvent id=\"start\"/></process>";

    EXPECT_NO_THROW({
        bpmn::BpmnParser parser;
        auto process = parser.parse(bpmnXml);
        EXPECT_EQ(process->getId(), "test");
        });
}