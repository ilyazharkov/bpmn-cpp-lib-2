#pragma once
#include <string>
#include <memory>
#include <libxml/parser.h>
#include <libxml/xpath.h>

#include "model.h"
#include "process.h"


namespace bpmn {

    class BpmnParser {
    public:
        BpmnParser();
        ~BpmnParser();

        std::unique_ptr<Process> parse(const std::string& file_path);
        std::unique_ptr<Process> parseFromString(const std::string& xml_content);

    private:
        std::unique_ptr<Process> parseDocument();
        void parseProcess(xmlNodePtr node, Process& process);
        void parseStartEvent(xmlNodePtr node, Process& process);
        void parseUserTask(xmlNodePtr node, Process& process);
        void parseServiceTask(xmlNodePtr node, Process& process);
        void parseEndEvent(xmlNodePtr node, Process& process);
        void parseParallelGateway(xmlNodePtr node, Process& process);
        void parseExclusiveGateway(xmlNodePtr node, Process& process);
        void parseSequenceFlow(xmlNodePtr node, Process& process);

        std::string getAttribute(xmlNodePtr node, const std::string& attribute_name);
        std::string getNodeName(xmlNodePtr node);

        xmlDocPtr doc_ = nullptr;
    };

} // namespace bpmn