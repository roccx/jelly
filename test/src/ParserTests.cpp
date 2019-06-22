#include <gtest/gtest.h>
#include <JellyCore/JellyCore.h>
#include <string>
#include <dirent.h>
#include <fstream>
#include <regex>

#include "FileTestDiagnostic.h"

static inline void WriteFileContent(std::string filePath, std::string content) {
    std::fstream file;
    file.open(filePath, std::fstream::out);
    assert(file.is_open());
    file.write(content.c_str(), content.length());
    file.close();
}

class ParserTest : public testing::TestWithParam<FileTest> {
};

TEST_P(ParserTest, run) {
    auto test = GetParam();
    printf("[   TEST   ] %s\n", test.context.filePath.substr(test.context.filePath.rfind("/")).c_str());

    if (!test.context.reports.empty()) {
        for (auto error : test.context.reports) {
            printf("[  FAILED  ] %s!\n", error.c_str());
        }

        FAIL();
    } else {
        DiagnosticEngineSetDefaultHandler(&FileTestDiagnosticHandler, &test.context);

        Char dumpBuffer[65535];
        FILE *dumpStream = fmemopen(dumpBuffer, sizeof(dumpBuffer) / sizeof(Char), "w");
        assert(dumpStream);

        AllocatorRef allocator = AllocatorGetSystemDefault();
        StringRef absoluteFilePath = StringCreate(allocator, test.context.filePath.c_str());
        StringRef workingDirectory = StringCreateCopyUntilLastOccurenceOf(allocator, absoluteFilePath, '/');
        StringRef filePath = StringCreateCopyFromLastOccurenceOf(allocator, absoluteFilePath, '/');
        ASTDumperRef dumper = ASTDumperCreate(allocator, dumpStream);
        WorkspaceRef workspace = WorkspaceCreate(allocator, workingDirectory);
        ASTContextRef context = WorkspaceGetContext(workspace);
        WorkspaceAddSourceFile(workspace, filePath);
        WorkspaceStartAsync(workspace);
        WorkspaceWaitForFinish(workspace);
        ASTDumperDump(dumper, (ASTNodeRef)ASTContextGetModule(context));
        WorkspaceDestroy(workspace);
        ASTDumperDestroy(dumper);
        StringDestroy(workingDirectory);
        StringDestroy(filePath);
        StringDestroy(absoluteFilePath);

        fclose(dumpStream);

        if (test.context.index < test.context.records.size()) {
            for (auto index = test.context.index; index < test.context.records.size(); index++) {
                printf("[ EXPECTED ] %s\n", test.context.records[index].message.c_str());
            }

            FAIL();
        }

        if (test.hasDumpRecord) {
            printf("[ RUN      ] %s\n", test.relativeFilePath.c_str());
            EXPECT_STREQ(test.dumpRecordContent.c_str(), dumpBuffer);
        } else {
            printf("[ REC      ] %s\n", test.relativeFilePath.c_str());
            WriteFileContent(test.dumpFilePath, dumpBuffer);
            printf("[       OK ] %s\n", test.relativeFilePath.c_str());
        }
    }
}

INSTANTIATE_TEST_CASE_P(run, ParserTest, testing::ValuesIn(FileTest::ReadFromDirectory("parser")));
